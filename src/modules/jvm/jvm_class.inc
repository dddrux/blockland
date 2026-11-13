// ---------- jvm: parser de .class (Java 8 / Forge 1.12.2) ----------
// Parte 1 do tradutor de mods: so le o formato. Sem dependencias do jogo (std only).
namespace jv {
    struct CP { uint8_t tag = 0; uint16_t a = 0, b = 0; int32_t i = 0; int64_t l = 0; float f = 0; double d = 0; std::string s; };
    struct ExcEnt { uint16_t start, end, handler, type; };
    struct Annot { std::string type; std::vector<std::pair<std::string, std::string>> vals; };   // vals: so strings/ints/enums simples
    struct Fld { uint16_t acc = 0; std::string name, desc; std::vector<Annot> ann; int cv = 0; };
    struct Mth { uint16_t acc = 0, maxStack = 0, maxLocals = 0; std::string name, desc; std::vector<uint8_t> code; std::vector<ExcEnt> exc; std::vector<Annot> ann; int nargs = 0; char ret = 'V'; };
    struct Bsm { uint16_t ref; std::vector<uint16_t> args; };
    struct ClassFile {
        uint16_t acc = 0; std::string name, super; std::vector<std::string> ifaces;
        std::vector<CP> cp; std::vector<Fld> fields; std::vector<Mth> methods; std::vector<Annot> ann; std::vector<Bsm> bsm;
        const std::string& utf(int i) const { static std::string e; return (i > 0 && i < (int)cp.size()) ? cp[i].s : e; }
        const std::string& cname(int i) const { return (i > 0 && i < (int)cp.size()) ? utf(cp[i].a) : utf(0); }
    };
    // descritor "(IJLjava/lang/String;)V": nº de slots dos argumentos (long/double = 2, sem 'this') e tipo de retorno
    static int descArgs(const std::string& d, char* ret = nullptr, int* count = nullptr) {
        int n = 0, c = 0; size_t i = 1;
        while (i < d.size() && d[i] != ')') {
            char ch = d[i]; c++;
            if (ch == 'J' || ch == 'D') { n += 2; i++; continue; }
            n++; while (d[i] == '[') i++;
            if (d[i] == 'L') i = d.find(';', i);
            i++;
        }
        if (ret) *ret = (i + 1 < d.size()) ? d[i + 1] : 'V';
        if (count) *count = c;
        return n;
    }
    struct Rd { const uint8_t *p, *e; bool err = false;
        uint32_t u1() { if (p >= e) { err = true; return 0; } return *p++; }
        uint32_t u2() { uint32_t a = u1(); return a << 8 | u1(); }
        uint32_t u4() { uint32_t a = u2(); return a << 16 | u2(); }
        std::string str(size_t n) { if (p + n > e) { err = true; return ""; } std::string s((const char*)p, n); p += n; return s; }
        void skip(size_t n) { if (p + n > e) { err = true; p = e; } else p += n; } };
    static void readElem(Rd& r, const ClassFile& c, std::string& out) {   // element_value: guarda strings/ints/enums, pula o resto
        int t = r.u1();
        switch (t) {
            case 's': out = c.utf(r.u2()); break;
            case 'B': case 'C': case 'I': case 'S': case 'Z': { int k = r.u2(); out = std::to_string(k > 0 && k < (int)c.cp.size() ? c.cp[k].i : 0); break; }
            case 'D': case 'F': case 'J': r.u2(); break;
            case 'e': r.u2(); out = c.utf(r.u2()); break;
            case 'c': r.u2(); break;
            case '@': { r.u2(); int n = r.u2(); for (int i = 0; i < n && !r.err; i++) { r.u2(); std::string d; readElem(r, c, d); } break; }
            case '[': { int n = r.u2(); for (int i = 0; i < n && !r.err; i++) { std::string d; readElem(r, c, d); if (i == 0) out = d; } break; }
            default: r.err = true;
        }
    }
    static void readAnnots(Rd& r, const ClassFile& c, std::vector<Annot>& v) {
        int n = r.u2();
        for (int i = 0; i < n && !r.err; i++) {
            Annot a; a.type = c.utf(r.u2()); int np = r.u2();
            for (int k = 0; k < np && !r.err; k++) { std::string nm = c.utf(r.u2()), val; readElem(r, c, val); a.vals.push_back({nm, val}); }
            v.push_back(a);
        }
    }
    static bool parseClass(const uint8_t* data, size_t n, ClassFile& c) {
        Rd r{data, data + n};
        if (r.u4() != 0xCAFEBABEu) return false;
        r.u2(); r.u2();
        int cn = r.u2(); c.cp.assign(cn, CP());
        for (int i = 1; i < cn && !r.err; i++) {
            CP& e = c.cp[i]; e.tag = r.u1();
            switch (e.tag) {
                case 1: { int len = r.u2(); e.s = r.str(len); break; }
                case 3: e.i = (int32_t)r.u4(); break;
                case 4: { uint32_t b = r.u4(); memcpy(&e.f, &b, 4); break; }
                case 5: { uint64_t h = r.u4(), l = r.u4(); e.l = (int64_t)(h << 32 | l); i++; break; }
                case 6: { uint64_t h = r.u4(), l = r.u4(); uint64_t b = h << 32 | l; memcpy(&e.d, &b, 8); i++; break; }
                case 7: case 8: case 16: e.a = r.u2(); break;
                case 9: case 10: case 11: case 12: case 18: e.a = r.u2(); e.b = r.u2(); break;
                case 15: e.a = r.u1(); e.b = r.u2(); break;
                default: return false;
            }
        }
        c.acc = r.u2(); c.name = c.cname(r.u2()); int sp = r.u2(); if (sp) c.super = c.cname(sp);
        int ni = r.u2(); for (int i = 0; i < ni; i++) c.ifaces.push_back(c.cname(r.u2()));
        int nf = r.u2();
        for (int i = 0; i < nf && !r.err; i++) {
            Fld f; f.acc = r.u2(); f.name = c.utf(r.u2()); f.desc = c.utf(r.u2()); int na = r.u2();
            for (int k = 0; k < na && !r.err; k++) { std::string an = c.utf(r.u2()); uint32_t len = r.u4(); const uint8_t* end = r.p + len;
                if (an == "ConstantValue") f.cv = r.u2(); else if (an == "RuntimeVisibleAnnotations") readAnnots(r, c, f.ann);
                r.p = end; }
            c.fields.push_back(f);
        }
        int nm = r.u2();
        for (int i = 0; i < nm && !r.err; i++) {
            Mth m; m.acc = r.u2(); m.name = c.utf(r.u2()); m.desc = c.utf(r.u2()); int na = r.u2();
            m.nargs = descArgs(m.desc, &m.ret) + ((m.acc & 8) ? 0 : 1);
            for (int k = 0; k < na && !r.err; k++) { std::string an = c.utf(r.u2()); uint32_t len = r.u4(); const uint8_t* end = r.p + len;
                if (an == "Code") {
                    m.maxStack = r.u2(); m.maxLocals = r.u2(); uint32_t cl = r.u4();
                    if (r.p + cl <= r.e) { m.code.assign(r.p, r.p + cl); } r.skip(cl);
                    int ne = r.u2(); for (int j = 0; j < ne; j++) { ExcEnt x; x.start = r.u2(); x.end = r.u2(); x.handler = r.u2(); x.type = r.u2(); m.exc.push_back(x); }
                } else if (an == "RuntimeVisibleAnnotations") readAnnots(r, c, m.ann);
                r.p = end; }
            c.methods.push_back(m);
        }
        int na = r.u2();
        for (int k = 0; k < na && !r.err; k++) { std::string an = c.utf(r.u2()); uint32_t len = r.u4(); const uint8_t* end = r.p + len;
            if (an == "RuntimeVisibleAnnotations") readAnnots(r, c, c.ann);
            else if (an == "BootstrapMethods") { int nb = r.u2(); for (int j = 0; j < nb; j++) { Bsm b; b.ref = r.u2(); int nA = r.u2(); for (int q = 0; q < nA; q++) b.args.push_back(r.u2()); c.bsm.push_back(b); } }
            r.p = end; }
        return !r.err;
    }
}
