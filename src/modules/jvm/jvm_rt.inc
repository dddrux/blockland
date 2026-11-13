// ---------- jvm: runtime Java minimo (java/lang/*) em C++ nativo ----------
namespace jv {
#define NF [](VM& vm, V* a, V& r, const char* d) -> bool
    static std::string fmtD(double x, bool isF) {
        if (x != x) return "NaN"; if (x == INFINITY) return "Infinity"; if (x == -INFINITY) return "-Infinity";
        char b[64]; if (x == floor(x) && fabs(x) < 1e7) { snprintf(b, sizeof b, "%.1f", x); return b; }
        for (int p = 1; p <= 17; p++) { snprintf(b, sizeof b, "%.*g", p, x); if (isF ? (float)strtod(b, nullptr) == (float)x : strtod(b, nullptr) == x) break; }
        return b;
    }
    static std::string objStr(VM& vm, Obj* o);
    static std::string fmtV(VM& vm, V v, char t) {
        switch (t) { case 'Z': return v.i ? "true" : "false"; case 'C': { std::string s; unsigned c = (uint16_t)v.i; if (c < 128) s += (char)c; else if (c < 0x800) { s += (char)(0xC0 | c >> 6); s += (char)(0x80 | (c & 63)); } else { s += (char)(0xE0 | c >> 12); s += (char)(0x80 | ((c >> 6) & 63)); s += (char)(0x80 | (c & 63)); } return s; }
            case 'I': case 'S': case 'B': return std::to_string(v.i); case 'J': return std::to_string(v.l);
            case 'F': return fmtD(v.f, true); case 'D': return fmtD(v.d, false); default: return objStr(vm, v.o); }
    }
    static Obj* box(VM& vm, const char* cn, V v) { Obj* o = vm.mk(vm.cls(cn)); o->f.assign(1, v); return o; }
    static std::string objStr(VM& vm, Obj* o) {
        if (!o) return "null"; if (o->kind == 1) return o->s;
        const std::string& n = o->c->name;
        if (n == "java/lang/Integer" || n == "java/lang/Short" || n == "java/lang/Byte") return std::to_string(o->f[0].i);
        if (n == "java/lang/Long") return std::to_string(o->f[0].l); if (n == "java/lang/Boolean") return o->f[0].i ? "true" : "false";
        if (n == "java/lang/Character") return fmtV(vm, o->f[0], 'C'); if (n == "java/lang/Float") return fmtD(o->f[0].f, true); if (n == "java/lang/Double") return fmtD(o->f[0].d, false);
        V a[1], rv; a[0].o = o; rv.l = 0; if (vm.callVirt(o, "toString", "()Ljava/lang/String;", a, rv) && rv.o) return rv.o->s; vm.exc = nullptr; return n + "@obj";
    }
    static void defEx(VM& vm, const char* n, const char* sup) { vm.defClass(n, sup); }
    static void setupRuntime(VM& vm) {
        Cls* O = vm.defClass("java/lang/Object", "");
        for (const char* b : {"Integer", "Long", "Float", "Double", "Boolean", "Short", "Byte", "Character"}) { Cls* c = vm.defClass(std::string("java/lang/") + b, "java/lang/Object"); c->nf = 1; }
        for (const char* n : {"java/lang/String", "java/lang/StringBuilder", "java/lang/StringBuffer", "java/lang/Class", "java/lang/Math", "java/lang/System", "java/io/PrintStream", "java/lang/Throwable"}) vm.defClass(n, "java/lang/Object");
        defEx(vm, "java/lang/Exception", "java/lang/Throwable"); defEx(vm, "java/lang/Error", "java/lang/Throwable"); defEx(vm, "java/lang/RuntimeException", "java/lang/Exception");
        for (const char* n : {"NullPointerException", "IllegalArgumentException", "IllegalStateException", "ArithmeticException", "ClassCastException", "IndexOutOfBoundsException", "NegativeArraySizeException", "UnsupportedOperationException"}) defEx(vm, (std::string("java/lang/") + n).c_str(), "java/lang/RuntimeException");
        defEx(vm, "java/lang/ArrayIndexOutOfBoundsException", "java/lang/IndexOutOfBoundsException"); defEx(vm, "java/lang/StringIndexOutOfBoundsException", "java/lang/IndexOutOfBoundsException"); defEx(vm, "java/lang/NumberFormatException", "java/lang/IllegalArgumentException");
        for (const char* n : {"StackOverflowError", "InternalError", "AbstractMethodError"}) defEx(vm, (std::string("java/lang/") + n).c_str(), "java/lang/Error");
        // Object / Class
        vm.def("java/lang/Object", "<init>", NF { return true; });
        vm.def("java/lang/Object", "hashCode", NF { r.i = (int)((intptr_t)a[0].o >> 4); return true; });
        vm.def("java/lang/Object", "equals", NF { r.i = a[0].o == a[1].o; return true; });
        vm.def("java/lang/Object", "toString", NF { char b[32]; snprintf(b, sizeof b, "@%x", (unsigned)((intptr_t)a[0].o >> 4)); std::string n = a[0].o->c->name; for (auto& ch : n) if (ch == '/') ch = '.'; r.o = vm.str(n + b); return true; });
        vm.def("java/lang/Object", "getClass", NF { r.o = classObj(vm, a[0].o->c); return true; });
        vm.def("java/lang/Object", "clone", NF { Obj* s = a[0].o; Obj* o = vm.mk(s->c); o->f = s->f; o->kind = s->kind; o->et = s->et; o->s = s->s; r.o = o; return true; });
        vm.def("java/lang/Class", "getName", NF { std::string n = ((Cls*)a[0].o->nat)->name; for (auto& ch : n) if (ch == '/') ch = '.'; r.o = vm.str(n); return true; });
        vm.def("java/lang/Class", "getSimpleName", NF { std::string n = ((Cls*)a[0].o->nat)->name; size_t p = n.find_last_of("/$"); r.o = vm.str(p == std::string::npos ? n : n.substr(p + 1)); return true; });
        // Throwable
        vm.def("java/lang/Throwable", "<init>", NF { if (d[1] == 'L' && a[1].o) a[0].o->s = a[1].o->s; return true; });
        vm.def("java/lang/Throwable", "getMessage", NF { r.o = a[0].o->s.empty() ? nullptr : vm.str(a[0].o->s); return true; });
        vm.def("java/lang/Throwable", "toString", NF { std::string n = a[0].o->c->name; for (auto& ch : n) if (ch == '/') ch = '.'; r.o = vm.str(a[0].o->s.empty() ? n : n + ": " + a[0].o->s); return true; });
        vm.def("java/lang/Throwable", "printStackTrace", NF { vm.log(objStr(vm, a[0].o)); return true; });
        // String
        vm.def("java/lang/String", "<init>", NF { Obj* o = a[0].o; o->kind = 1; o->s.clear(); if (d[1] == 'L' && a[1].o) o->s = a[1].o->s; else if (d[1] == '[') for (auto& e : a[1].o->f) o->s += fmtV(vm, e, 'C'); return true; });
        vm.def("java/lang/String", "length", NF { r.i = (int)a[0].o->s.size(); return true; });
        vm.def("java/lang/String", "isEmpty", NF { r.i = a[0].o->s.empty(); return true; });
        vm.def("java/lang/String", "charAt", NF { const std::string& s = a[0].o->s; if ((uint32_t)a[1].i >= s.size()) return vm.throwNew("java/lang/StringIndexOutOfBoundsException", std::to_string(a[1].i)); r.i = (uint8_t)s[a[1].i]; return true; });
        vm.def("java/lang/String", "equals", NF { r.i = a[1].o && a[1].o->kind == 1 && a[1].o->s == a[0].o->s; return true; });
        vm.def("java/lang/String", "hashCode", NF { int32_t h = 0; for (unsigned char ch : a[0].o->s) h = 31 * h + ch; r.i = h; return true; });
        vm.def("java/lang/String", "toString", NF { r.o = a[0].o; return true; });
        vm.def("java/lang/String", "concat", NF { r.o = vm.str(a[0].o->s + a[1].o->s); return true; });
        vm.def("java/lang/String", "contains", NF { r.i = a[0].o->s.find(a[1].o->s) != std::string::npos; return true; });
        vm.def("java/lang/String", "startsWith", NF { r.i = a[0].o->s.rfind(a[1].o->s, 0) == 0; return true; });
        vm.def("java/lang/String", "endsWith", NF { const std::string &s = a[0].o->s, &t = a[1].o->s; r.i = s.size() >= t.size() && !s.compare(s.size() - t.size(), t.size(), t); return true; });
        vm.def("java/lang/String", "indexOf", NF { size_t p = d[1] == 'I' ? a[0].o->s.find((char)a[1].i) : a[0].o->s.find(a[1].o->s); r.i = p == std::string::npos ? -1 : (int)p; return true; });
        vm.def("java/lang/String", "substring", NF { const std::string& s = a[0].o->s; int b = a[1].i, e = d[2] == 'I' ? a[2].i : (int)s.size(); if (b < 0 || e > (int)s.size() || b > e) return vm.throwNew("java/lang/StringIndexOutOfBoundsException", std::to_string(b)); r.o = vm.str(s.substr(b, e - b)); return true; });
        vm.def("java/lang/String", "toLowerCase", NF { std::string s = a[0].o->s; for (auto& c : s) c = (char)tolower((unsigned char)c); r.o = vm.str(s); return true; });
        vm.def("java/lang/String", "toUpperCase", NF { std::string s = a[0].o->s; for (auto& c : s) c = (char)toupper((unsigned char)c); r.o = vm.str(s); return true; });
        vm.def("java/lang/String", "trim", NF { const std::string& s = a[0].o->s; size_t b = s.find_first_not_of(" \t\r\n"), e = s.find_last_not_of(" \t\r\n"); r.o = vm.str(b == std::string::npos ? "" : s.substr(b, e - b + 1)); return true; });
        vm.def("java/lang/String", "equalsIgnoreCase", NF { std::string x = a[0].o->s, y = a[1].o ? a[1].o->s : "\x01"; for (auto& c : x) c = (char)tolower((unsigned char)c); for (auto& c : y) c = (char)tolower((unsigned char)c); r.i = x == y; return true; });
        vm.def("java/lang/String", "compareTo", NF { int c = a[0].o->s.compare(a[1].o->s); r.i = c < 0 ? -1 : c > 0 ? 1 : 0; return true; });
        vm.def("java/lang/String", "replace", NF { std::string s = a[0].o->s, f = d[1] == 'C' ? fmtV(vm, a[1], 'C') : a[1].o->s, t = d[1] == 'C' ? fmtV(vm, a[2], 'C') : a[2].o->s; if (!f.empty()) for (size_t p = 0; (p = s.find(f, p)) != std::string::npos; p += t.size()) s.replace(p, f.size(), t); r.o = vm.str(s); return true; });
        vm.def("java/lang/String", "valueOf", NF { r.o = vm.str(fmtV(vm, a[0], d[1])); return true; });
        // StringBuilder / StringBuffer
        for (const char* sb : {"java/lang/StringBuilder", "java/lang/StringBuffer"}) {
            vm.def(sb, "<init>", NF { a[0].o->s = d[1] == 'L' && a[1].o ? a[1].o->s : ""; return true; });
            vm.def(sb, "append", NF { a[0].o->s += fmtV(vm, a[1], d[1]); r.o = a[0].o; return true; });
            vm.def(sb, "toString", NF { r.o = vm.str(a[0].o->s); return true; });
            vm.def(sb, "length", NF { r.i = (int)a[0].o->s.size(); return true; });
            vm.def(sb, "reverse", NF { std::reverse(a[0].o->s.begin(), a[0].o->s.end()); r.o = a[0].o; return true; });
        }
        // boxes
        struct BX { const char* cn; char t; };
        vm.def("java/lang/Integer", "valueOf", NF { V v; v.l = 0; if (d[1] == 'L') { v.i = atoi(a[0].o->s.c_str()); } else v.i = a[0].i; r.o = box(vm, "java/lang/Integer", v); return true; });
        vm.def("java/lang/Short", "valueOf", NF { r.o = box(vm, "java/lang/Short", a[0]); return true; });
        vm.def("java/lang/Byte", "valueOf", NF { r.o = box(vm, "java/lang/Byte", a[0]); return true; });
        vm.def("java/lang/Boolean", "valueOf", NF { r.o = box(vm, "java/lang/Boolean", a[0]); return true; });
        vm.def("java/lang/Character", "valueOf", NF { r.o = box(vm, "java/lang/Character", a[0]); return true; });
        vm.def("java/lang/Long", "valueOf", NF { r.o = box(vm, "java/lang/Long", a[0]); return true; });
        vm.def("java/lang/Float", "valueOf", NF { r.o = box(vm, "java/lang/Float", a[0]); return true; });
        vm.def("java/lang/Double", "valueOf", NF { r.o = box(vm, "java/lang/Double", a[0]); return true; });
        for (const char* b : {"Integer", "Long", "Float", "Double", "Boolean", "Short", "Byte", "Character"}) {
            std::string cn = std::string("java/lang/") + b;
            vm.def(cn, "intValue", NF { const std::string& n = a[0].o->c->name; V x = a[0].o->f[0]; r.i = n == "java/lang/Long" ? (int)x.l : n == "java/lang/Float" ? (int)x.f : n == "java/lang/Double" ? (int)x.d : x.i; return true; });
            vm.def(cn, "longValue", NF { const std::string& n = a[0].o->c->name; V x = a[0].o->f[0]; r.l = n == "java/lang/Long" ? x.l : n == "java/lang/Float" ? (int64_t)x.f : n == "java/lang/Double" ? (int64_t)x.d : x.i; return true; });
            vm.def(cn, "floatValue", NF { const std::string& n = a[0].o->c->name; V x = a[0].o->f[0]; r.f = n == "java/lang/Long" ? (float)x.l : n == "java/lang/Float" ? x.f : n == "java/lang/Double" ? (float)x.d : (float)x.i; return true; });
            vm.def(cn, "doubleValue", NF { const std::string& n = a[0].o->c->name; V x = a[0].o->f[0]; r.d = n == "java/lang/Long" ? (double)x.l : n == "java/lang/Float" ? x.f : n == "java/lang/Double" ? x.d : x.i; return true; });
            vm.def(cn, "compareTo", NF { const std::string& n = a[0].o->c->name; V x = a[0].o->f[0], y = a[1].o->f[0]; r.i = n == "java/lang/Long" ? (x.l < y.l ? -1 : x.l > y.l) : n == "java/lang/Float" ? (x.f < y.f ? -1 : x.f > y.f) : n == "java/lang/Double" ? (x.d < y.d ? -1 : x.d > y.d) : (x.i < y.i ? -1 : x.i > y.i); return true; });
            vm.def(cn, "booleanValue", NF { r.i = a[0].o->f[0].i; return true; });
            vm.def(cn, "charValue", NF { r.i = a[0].o->f[0].i; return true; });
            vm.def(cn, "toString", NF { r.o = vm.str(objStr(vm, a[0].o)); return true; });
            vm.def(cn, "hashCode", NF { V x = a[0].o->f[0]; r.i = (int)(x.l ^ (x.l >> 32)); return true; });
            vm.def(cn, "equals", NF { Obj* b = a[1].o; r.i = b && b->c == a[0].o->c && b->f[0].l == a[0].o->f[0].l; return true; });
        }
        vm.def("java/lang/Integer", "parseInt", NF { char* e; const std::string& s = a[0].o->s; long v = strtol(s.c_str(), &e, 10); if (s.empty() || *e) return vm.throwNew("java/lang/NumberFormatException", "For input string: \"" + s + "\""); r.i = (int)v; return true; });
        vm.cls("java/lang/Integer")->sf["MAX_VALUE"].i = INT32_MAX; vm.cls("java/lang/Integer")->sf["MIN_VALUE"].i = INT32_MIN;
        // Math
        vm.def("java/lang/Math", "abs", NF { switch (d[1]) { case 'I': r.i = a[0].i < 0 ? -a[0].i : a[0].i; break; case 'J': r.l = a[0].l < 0 ? -a[0].l : a[0].l; break; case 'F': r.f = fabsf(a[0].f); break; default: r.d = fabs(a[0].d); } return true; });
        vm.def("java/lang/Math", "min", NF { switch (d[1]) { case 'I': r.i = std::min(a[0].i, a[1].i); break; case 'J': r.l = std::min(a[0].l, a[2].l); break; case 'F': r.f = std::min(a[0].f, a[1].f); break; default: r.d = std::min(a[0].d, a[2].d); } return true; });
        vm.def("java/lang/Math", "max", NF { switch (d[1]) { case 'I': r.i = std::max(a[0].i, a[1].i); break; case 'J': r.l = std::max(a[0].l, a[2].l); break; case 'F': r.f = std::max(a[0].f, a[1].f); break; default: r.d = std::max(a[0].d, a[2].d); } return true; });
        vm.def("java/lang/Math", "sqrt", NF { r.d = sqrt(a[0].d); return true; });
        vm.def("java/lang/Math", "sin", NF { r.d = sin(a[0].d); return true; });
        vm.def("java/lang/Math", "cos", NF { r.d = cos(a[0].d); return true; });
        vm.def("java/lang/Math", "floor", NF { r.d = floor(a[0].d); return true; });
        vm.def("java/lang/Math", "ceil", NF { r.d = ceil(a[0].d); return true; });
        vm.def("java/lang/Math", "pow", NF { r.d = pow(a[0].d, a[2].d); return true; });
        vm.def("java/lang/Math", "round", NF { if (d[1] == 'F') r.i = d2i(floor(a[0].f + 0.5)); else r.l = d2l(floor(a[0].d + 0.5)); return true; });
        // System / PrintStream
        vm.def("java/lang/System", "currentTimeMillis", NF { r.l = (int64_t)time(nullptr) * 1000; return true; });
        vm.def("java/lang/System", "nanoTime", NF { r.l = (int64_t)clock() * (1000000000LL / CLOCKS_PER_SEC); return true; });
        vm.def("java/lang/System", "arraycopy", NF { Obj *s = a[0].o, *t = a[2].o; int sp = a[1].i, tp = a[3].i, n = a[4].i; if (!s || !t) return vm.throwNew("java/lang/NullPointerException");
            if (sp < 0 || tp < 0 || n < 0 || sp + n > (int)s->f.size() || tp + n > (int)t->f.size()) return vm.throwNew("java/lang/ArrayIndexOutOfBoundsException", "arraycopy"); memmove(&t->f[tp], &s->f[sp], n * sizeof(V)); return true; });
        vm.cls("java/lang/System")->sf["out"].o = vm.mk(vm.cls("java/io/PrintStream")); vm.cls("java/lang/System")->sf["err"].o = vm.mk(vm.cls("java/io/PrintStream"));
        vm.def("java/io/PrintStream", "println", NF { printf("%s\n", d[1] == ')' ? "" : fmtV(vm, a[1], d[1]).c_str()); return true; });
        vm.def("java/io/PrintStream", "print", NF { printf("%s", fmtV(vm, a[1], d[1]).c_str()); return true; });
    }
}
