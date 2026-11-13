// ---------- jvm: VM (objetos, classes, resolucao). Interpretador em jvm_exec.inc, runtime Java em jvm_rt.inc ----------
namespace jv {
    struct Obj; struct Cls; struct VM;
    union V { int32_t i; int64_t l; float f; double d; Obj* o; };   // long/double ocupam 2 slots (valor no 1o), como na JVM
    typedef bool (*Native)(VM&, V*, V&, const char*);                // args (this em [0]), retorno, descritor; false = excecao pendente
    struct Res { Cls* c = nullptr; Mth* m = nullptr; Native n = nullptr; bool phantom = false; };
    struct Ref { bool ok = false; Cls* c = nullptr; int slot = -1; V* sv = nullptr; Res r; std::string name, desc; int nargs = 0; char ret = 'V'; Cls* lastC = nullptr; Res last; };
    struct Obj { Cls* c = nullptr; std::vector<V> f; std::string s; char kind = 0, et = 0; void* nat = nullptr; };   // kind: 0 instancia, 1 String, 2 array
    struct Cls {
        std::string name; Cls* super = nullptr; std::vector<Cls*> ifs; ClassFile* cf = nullptr; bool native = false, phantom = false, inited = false;
        std::unordered_map<std::string, int> fidx; int nf = 0; std::unordered_map<std::string, V> sf;
        std::unordered_map<std::string, Mth*> mt; std::unordered_map<std::string, Native> nm; std::vector<Ref> rc;
    };
    struct VM {
        std::unordered_map<std::string, Cls*> classes; std::vector<Jar*> jars; std::unordered_map<std::string, Obj*> interned;
        std::vector<Obj*> heap; std::vector<V> pool; V* fp = nullptr; int depth = 0; Obj* exc = nullptr;
        std::set<std::string> warned, missing; void (*logfn)(const std::string&) = nullptr;
        VM() { pool.assign(1 << 18, V()); fp = pool.data(); }
        void log(const std::string& s) { if (logfn) logfn(s); else fprintf(stderr, "[jvm] %s\n", s.c_str()); }
        // ---- objetos ----
        Obj* mk(Cls* c) { Obj* o = new Obj; o->c = c; o->f.assign(c ? c->nf : 0, V()); heap.push_back(o); return o; }
        Obj* str(const std::string& s) { Obj* o = mk(cls("java/lang/String")); o->kind = 1; o->s = s; return o; }
        Obj* intern(const std::string& s) { auto it = interned.find(s); if (it != interned.end()) return it->second; return interned[s] = str(s); }
        Obj* arr(char et, int n) { Obj* o = mk(cls(std::string("[") + et)); o->kind = 2; o->et = et; o->f.assign(n, V()); return o; }
        bool throwNew(const std::string& cn, const std::string& msg = "") { Obj* o = mk(cls(cn)); o->s = msg; exc = o; return false; }
        // ---- classes ----
        Cls* defClass(const std::string& n, const std::string& sup) { Cls* c = new Cls; c->name = n; c->native = true; c->inited = true; classes[n] = c; if (!sup.empty()) c->super = cls(sup); return c; }
        void def(const std::string& cn, const std::string& mn, Native f) { Cls* c = cls(cn); c->nm[mn] = f; }   // mn = "nome" (qualquer descritor) ou "nome(desc)ret"
        Cls* cls(const std::string& n) {
            auto it = classes.find(n); if (it != classes.end()) return it->second;
            Cls* c = new Cls; c->name = n; classes[n] = c;
            if (n[0] == '[') { c->native = true; c->inited = true; c->super = cls("java/lang/Object"); return c; }
            for (Jar* j : jars) { const Jar::Ent* e = j->find(n + ".class"); std::vector<uint8_t> b;
                if (e && j->read(*e, b)) { c->cf = new ClassFile; if (parseClass(b.data(), b.size(), *c->cf)) { link(c); return c; } delete c->cf; c->cf = nullptr; } }
            c->native = c->phantom = true; c->inited = true; if (n != "java/lang/Object") c->super = cls("java/lang/Object");
            if (warned.insert("C" + n).second) { missing.insert("classe " + n); log("classe ausente (fantasma): " + n); }
            return c;
        }
        void link(Cls* c) {
            ClassFile& f = *c->cf; if (!f.super.empty()) c->super = cls(f.super);
            for (auto& i : f.ifaces) c->ifs.push_back(cls(i));
            if (c->super) { c->fidx = c->super->fidx; c->nf = c->super->nf; }
            for (auto& fl : f.fields) {
                if (fl.acc & 8) { V v; v.l = 0;
                    if (fl.cv) { const CP& k = f.cp[fl.cv]; if (k.tag == 3) v.i = k.i; else if (k.tag == 4) v.f = k.f; else if (k.tag == 5) v.l = k.l; else if (k.tag == 6) v.d = k.d; else if (k.tag == 8) v.o = intern(f.utf(k.a)); }
                    c->sf[fl.name] = v;
                } else c->fidx[fl.name] = c->nf++;
            }
            for (auto& m : f.methods) c->mt[m.name + m.desc] = &m;
            c->rc.resize(f.cp.size());
        }
        bool isA(Cls* a, Cls* t) { if (!a || !t) return false; if (a == t) return true; if (isA(a->super, t)) return true; for (Cls* i : a->ifs) if (isA(i, t)) return true; return false; }
        bool init(Cls* c) {
            if (c->inited) return true; c->inited = true; if (c->super && !init(c->super)) return false;
            auto it = c->mt.find("<clinit>()V"); if (it == c->mt.end()) return true; V r; return exec(c, it->second, nullptr, r);
        }
        bool findIn(Cls* c, const std::string& k, const std::string& n, Res& r) {
            for (Cls* x = c; x; x = x->super) {
                auto it = x->mt.find(k); if (it != x->mt.end() && !it->second->code.empty()) { r.c = x; r.m = it->second; return true; }
                auto nt = x->nm.find(k); if (nt == x->nm.end()) nt = x->nm.find(n); if (nt != x->nm.end()) { r.c = x; r.n = nt->second; return true; }
                for (Cls* i : x->ifs) if (findIn(i, k, n, r)) return true;
            }
            return false;
        }
        Res find(Cls* c, const std::string& n, const std::string& d) { Res r; if (!findIn(c, n + d, n, r)) r.phantom = true; return r; }
        bool invoke(const Res& r, V* a, V& ret, const std::string& name, const std::string& desc, Cls* named) {
            if (r.m) return exec(r.c, r.m, a, ret);
            if (r.n) return r.n(*this, a, ret, desc.c_str());
            ret.l = 0; std::string k = named->name + "." + name + desc;
            if (warned.insert("M" + k).second) { missing.insert("metodo " + k); log("API ausente (retorna 0/null): " + k); }
            return true;
        }
        // chamada vinda de nativo/host: virtual por nome. args[0] = this
        bool callVirt(Obj* o, const std::string& name, const std::string& desc, V* args, V& ret) { Res r = find(o->c, name, desc); return invoke(r, args, ret, name, desc, o->c); }
        bool callStatic(const std::string& cn, const std::string& name, const std::string& desc, V* args, V& ret) { Cls* c = cls(cn); if (!init(c)) return false; Res r = find(c, name, desc); return invoke(r, args, ret, name, desc, c); }
        bool exec(Cls* c, Mth* m, V* args, V& ret);
        bool indy(Cls* c, int idx, V* st, int& sp);   // invokedynamic (jvm_indy.inc)
    };
}
