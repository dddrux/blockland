// ---------- jvm: interpretador de bytecode (todos os opcodes de Java 8, exceto jsr/ret e invokedynamic -> jvm_indy.inc) ----------
namespace jv {
    static int32_t d2i(double d) { if (d != d) return 0; if (d >= 2147483647.0) return INT32_MAX; if (d <= -2147483648.0) return INT32_MIN; return (int32_t)d; }
    static int64_t d2l(double d) { if (d != d) return 0; if (d >= 9.2233720368547758e18) return INT64_MAX; if (d <= -9.2233720368547758e18) return INT64_MIN; return (int64_t)d; }
    static Ref& refM(VM& vm, Cls* c, int idx) {   // Methodref/InterfaceMethodref/Fieldref -> nome, descritor, classe
        Ref& r = c->rc[idx]; if (r.ok) return r;
        const CP& e = c->cf->cp[idx]; const CP& nt = c->cf->cp[e.b];
        r.name = c->cf->utf(nt.a); r.desc = c->cf->utf(nt.b); r.c = vm.cls(c->cf->cname(e.a)); r.ok = true;
        if (e.tag == 9) r.ret = r.desc[0]; else r.nargs = descArgs(r.desc, &r.ret);
        return r;
    }
    static V* findSF(Cls* c, const std::string& n, Cls*& own) {
        for (Cls* x = c; x; x = x->super) { auto it = x->sf.find(n); if (it != x->sf.end()) { own = x; return &it->second; } for (Cls* i : x->ifs) if (V* p = findSF(i, n, own)) return p; }
        return nullptr;
    }
    static Obj* classObj(VM& vm, Cls* k) { Obj* o = vm.mk(vm.cls("java/lang/Class")); o->nat = k; return o; }
#define PUSHI(x) (st[sp++].i = (x))
#define PUSHA(x) (st[sp++].o = (x))
#define PUSHF(x) (st[sp++].f = (x))
#define PUSHL(x) do { st[sp].l = (x); sp += 2; } while (0)
#define PUSHD(x) do { st[sp].d = (x); sp += 2; } while (0)
#define POPI() (st[--sp].i)
#define POPA() (st[--sp].o)
#define POPF() (st[--sp].f)
#define POPL() (sp -= 2, st[sp].l)
#define POPD() (sp -= 2, st[sp].d)
#define U1 (code[pc++])
#define U2 (pc += 2, (uint32_t)(code[pc - 2] << 8 | code[pc - 1]))
#define S2 ((int16_t)U2)
#define S4 (pc += 4, (int32_t)((uint32_t)code[pc - 4] << 24 | code[pc - 3] << 16 | code[pc - 2] << 8 | code[pc - 1]))
#define THROW(cn, msg) do { throwNew(cn, msg); goto thrown; } while (0)
#define NPE(p) if (!(p)) THROW("java/lang/NullPointerException", "")
#define BRANCH(cond) do { int16_t off = S2; if (cond) pc = op_pc + off; } while (0)
    bool VM::exec(Cls* c, Mth* m, V* args, V& ret) {
        if (m->code.empty()) return throwNew("java/lang/AbstractMethodError", c->name + "." + m->name);
        const int need = m->maxLocals + m->maxStack + 2;
        if (depth > 300 || fp + need > pool.data() + pool.size()) return throwNew("java/lang/StackOverflowError");
        V* L = fp; fp += need; depth++;
        struct G { VM& v; V* L; ~G() { v.fp = L; v.depth--; } } guard{*this, L};
        if (m->nargs) memcpy(L, args, m->nargs * sizeof(V)); memset(L + m->nargs, 0, (need - m->nargs) * sizeof(V));
        V* st = L + m->maxLocals; int sp = 0; uint32_t pc = 0, op_pc = 0; const uint8_t* code = m->code.data(); ClassFile& cf = *c->cf;
        for (;;) {
            op_pc = pc; const int op = code[pc++];
            switch (op) {
                case 0x00: break;
                case 0x01: PUSHA(nullptr); break;
                case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: PUSHI(op - 3); break;
                case 0x09: case 0x0a: PUSHL(op - 9); break;
                case 0x0b: case 0x0c: case 0x0d: PUSHF((float)(op - 0x0b)); break;
                case 0x0e: case 0x0f: PUSHD((double)(op - 0x0e)); break;
                case 0x10: PUSHI((int8_t)U1); break;
                case 0x11: PUSHI(S2); break;
                case 0x12: case 0x13: case 0x14: { int idx = op == 0x12 ? U1 : U2; const CP& k = cf.cp[idx];
                    switch (k.tag) { case 3: PUSHI(k.i); break; case 4: PUSHF(k.f); break; case 5: PUSHL(k.l); break; case 6: PUSHD(k.d); break;
                        case 8: PUSHA(intern(cf.utf(k.a))); break; case 7: PUSHA(classObj(*this, cls(cf.utf(k.a)))); break; default: PUSHA(nullptr); } break; }
                case 0x15: case 0x17: case 0x19: st[sp++] = L[U1]; break;
                case 0x16: case 0x18: st[sp] = L[U1]; sp += 2; break;
                case 0x1a: case 0x1b: case 0x1c: case 0x1d: st[sp++] = L[op - 0x1a]; break;
                case 0x1e: case 0x1f: case 0x20: case 0x21: st[sp] = L[op - 0x1e]; sp += 2; break;
                case 0x22: case 0x23: case 0x24: case 0x25: st[sp++] = L[op - 0x22]; break;
                case 0x26: case 0x27: case 0x28: case 0x29: st[sp] = L[op - 0x26]; sp += 2; break;
                case 0x2a: case 0x2b: case 0x2c: case 0x2d: st[sp++] = L[op - 0x2a]; break;
                case 0x2e: case 0x2f: case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: { int i = POPI(); Obj* a = POPA(); NPE(a);
                    if ((uint32_t)i >= a->f.size()) THROW("java/lang/ArrayIndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(a->f.size()));
                    V v = a->f[i]; if (op == 0x2f || op == 0x31) { st[sp] = v; sp += 2; } else st[sp++] = v; break; }
                case 0x36: case 0x38: case 0x3a: L[U1] = st[--sp]; break;
                case 0x37: case 0x39: sp -= 2; L[U1] = st[sp]; break;
                case 0x3b: case 0x3c: case 0x3d: case 0x3e: L[op - 0x3b] = st[--sp]; break;
                case 0x3f: case 0x40: case 0x41: case 0x42: sp -= 2; L[op - 0x3f] = st[sp]; break;
                case 0x43: case 0x44: case 0x45: case 0x46: L[op - 0x43] = st[--sp]; break;
                case 0x47: case 0x48: case 0x49: case 0x4a: sp -= 2; L[op - 0x47] = st[sp]; break;
                case 0x4b: case 0x4c: case 0x4d: case 0x4e: L[op - 0x4b] = st[--sp]; break;
                case 0x4f: case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: {
                    V v; if (op == 0x50 || op == 0x52) { sp -= 2; v = st[sp]; } else v = st[--sp]; int i = POPI(); Obj* a = POPA(); NPE(a);
                    if ((uint32_t)i >= a->f.size()) THROW("java/lang/ArrayIndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(a->f.size()));
                    if (op == 0x54) v.i = (int8_t)v.i; else if (op == 0x55) v.i = (uint16_t)v.i; else if (op == 0x56) v.i = (int16_t)v.i;
                    a->f[i] = v; break; }
                case 0x57: sp--; break;
                case 0x58: sp -= 2; break;
                case 0x59: st[sp] = st[sp - 1]; sp++; break;
                case 0x5a: st[sp] = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = st[sp]; sp++; break;
                case 0x5b: st[sp] = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = st[sp - 3]; st[sp - 3] = st[sp]; sp++; break;
                case 0x5c: st[sp] = st[sp - 2]; st[sp + 1] = st[sp - 1]; sp += 2; break;
                case 0x5d: st[sp + 1] = st[sp - 1]; st[sp] = st[sp - 2]; st[sp - 1] = st[sp - 3]; st[sp - 3] = st[sp]; st[sp - 2] = st[sp + 1]; sp += 2; break;
                case 0x5e: st[sp + 1] = st[sp - 1]; st[sp] = st[sp - 2]; st[sp - 1] = st[sp - 3]; st[sp - 2] = st[sp - 4]; st[sp - 4] = st[sp]; st[sp - 3] = st[sp + 1]; sp += 2; break;
                case 0x5f: { V t = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = t; break; }
                case 0x60: { int b = POPI(), a = POPI(); PUSHI((int32_t)((uint32_t)a + (uint32_t)b)); break; }
                case 0x61: { int64_t b = POPL(), a = POPL(); PUSHL((int64_t)((uint64_t)a + (uint64_t)b)); break; }
                case 0x62: { float b = POPF(), a = POPF(); PUSHF(a + b); break; }
                case 0x63: { double b = POPD(), a = POPD(); PUSHD(a + b); break; }
                case 0x64: { int b = POPI(), a = POPI(); PUSHI((int32_t)((uint32_t)a - (uint32_t)b)); break; }
                case 0x65: { int64_t b = POPL(), a = POPL(); PUSHL((int64_t)((uint64_t)a - (uint64_t)b)); break; }
                case 0x66: { float b = POPF(), a = POPF(); PUSHF(a - b); break; }
                case 0x67: { double b = POPD(), a = POPD(); PUSHD(a - b); break; }
                case 0x68: { int b = POPI(), a = POPI(); PUSHI((int32_t)((uint32_t)a * (uint32_t)b)); break; }
                case 0x69: { int64_t b = POPL(), a = POPL(); PUSHL((int64_t)((uint64_t)a * (uint64_t)b)); break; }
                case 0x6a: { float b = POPF(), a = POPF(); PUSHF(a * b); break; }
                case 0x6b: { double b = POPD(), a = POPD(); PUSHD(a * b); break; }
                case 0x6c: { int b = POPI(), a = POPI(); if (!b) THROW("java/lang/ArithmeticException", "/ by zero"); PUSHI(b == -1 ? (int32_t)(0u - (uint32_t)a) : a / b); break; }
                case 0x6d: { int64_t b = POPL(), a = POPL(); if (!b) THROW("java/lang/ArithmeticException", "/ by zero"); PUSHL(b == -1 ? (int64_t)(0ull - (uint64_t)a) : a / b); break; }
                case 0x6e: { float b = POPF(), a = POPF(); PUSHF(a / b); break; }
                case 0x6f: { double b = POPD(), a = POPD(); PUSHD(a / b); break; }
                case 0x70: { int b = POPI(), a = POPI(); if (!b) THROW("java/lang/ArithmeticException", "/ by zero"); PUSHI(b == -1 ? 0 : a % b); break; }
                case 0x71: { int64_t b = POPL(), a = POPL(); if (!b) THROW("java/lang/ArithmeticException", "/ by zero"); PUSHL(b == -1 ? 0 : a % b); break; }
                case 0x72: { float b = POPF(), a = POPF(); PUSHF(fmodf(a, b)); break; }
                case 0x73: { double b = POPD(), a = POPD(); PUSHD(fmod(a, b)); break; }
                case 0x74: { int a = POPI(); PUSHI((int32_t)(0u - (uint32_t)a)); break; }
                case 0x75: { int64_t a = POPL(); PUSHL((int64_t)(0ull - (uint64_t)a)); break; }
                case 0x76: { float a = POPF(); PUSHF(-a); break; }
                case 0x77: { double a = POPD(); PUSHD(-a); break; }
                case 0x78: { int b = POPI(), a = POPI(); PUSHI((int32_t)((uint32_t)a << (b & 31))); break; }
                case 0x79: { int b = POPI(); int64_t a = POPL(); PUSHL((int64_t)((uint64_t)a << (b & 63))); break; }
                case 0x7a: { int b = POPI(), a = POPI(); PUSHI(a >> (b & 31)); break; }
                case 0x7b: { int b = POPI(); int64_t a = POPL(); PUSHL(a >> (b & 63)); break; }
                case 0x7c: { int b = POPI(), a = POPI(); PUSHI((int32_t)((uint32_t)a >> (b & 31))); break; }
                case 0x7d: { int b = POPI(); int64_t a = POPL(); PUSHL((int64_t)((uint64_t)a >> (b & 63))); break; }
                case 0x7e: { int b = POPI(), a = POPI(); PUSHI(a & b); break; }
                case 0x7f: { int64_t b = POPL(), a = POPL(); PUSHL(a & b); break; }
                case 0x80: { int b = POPI(), a = POPI(); PUSHI(a | b); break; }
                case 0x81: { int64_t b = POPL(), a = POPL(); PUSHL(a | b); break; }
                case 0x82: { int b = POPI(), a = POPI(); PUSHI(a ^ b); break; }
                case 0x83: { int64_t b = POPL(), a = POPL(); PUSHL(a ^ b); break; }
                case 0x84: { int n = U1; L[n].i += (int8_t)U1; break; }
                case 0x85: { int a = POPI(); PUSHL(a); break; }
                case 0x86: { int a = POPI(); PUSHF((float)a); break; }
                case 0x87: { int a = POPI(); PUSHD((double)a); break; }
                case 0x88: { int64_t a = POPL(); PUSHI((int32_t)a); break; }
                case 0x89: { int64_t a = POPL(); PUSHF((float)a); break; }
                case 0x8a: { int64_t a = POPL(); PUSHD((double)a); break; }
                case 0x8b: { float a = POPF(); PUSHI(d2i(a)); break; }
                case 0x8c: { float a = POPF(); PUSHL(d2l(a)); break; }
                case 0x8d: { float a = POPF(); PUSHD((double)a); break; }
                case 0x8e: { double a = POPD(); PUSHI(d2i(a)); break; }
                case 0x8f: { double a = POPD(); PUSHL(d2l(a)); break; }
                case 0x90: { double a = POPD(); PUSHF((float)a); break; }
                case 0x91: { int a = POPI(); PUSHI((int8_t)a); break; }
                case 0x92: { int a = POPI(); PUSHI((uint16_t)a); break; }
                case 0x93: { int a = POPI(); PUSHI((int16_t)a); break; }
                case 0x94: { int64_t b = POPL(), a = POPL(); PUSHI(a < b ? -1 : a > b ? 1 : 0); break; }
                case 0x95: case 0x96: { float b = POPF(), a = POPF(); PUSHI(a < b ? -1 : a > b ? 1 : a == b ? 0 : (op == 0x95 ? -1 : 1)); break; }
                case 0x97: case 0x98: { double b = POPD(), a = POPD(); PUSHI(a < b ? -1 : a > b ? 1 : a == b ? 0 : (op == 0x97 ? -1 : 1)); break; }
                case 0x99: { int a = POPI(); BRANCH(a == 0); break; }
                case 0x9a: { int a = POPI(); BRANCH(a != 0); break; }
                case 0x9b: { int a = POPI(); BRANCH(a < 0); break; }
                case 0x9c: { int a = POPI(); BRANCH(a >= 0); break; }
                case 0x9d: { int a = POPI(); BRANCH(a > 0); break; }
                case 0x9e: { int a = POPI(); BRANCH(a <= 0); break; }
                case 0x9f: { int b = POPI(), a = POPI(); BRANCH(a == b); break; }
                case 0xa0: { int b = POPI(), a = POPI(); BRANCH(a != b); break; }
                case 0xa1: { int b = POPI(), a = POPI(); BRANCH(a < b); break; }
                case 0xa2: { int b = POPI(), a = POPI(); BRANCH(a >= b); break; }
                case 0xa3: { int b = POPI(), a = POPI(); BRANCH(a > b); break; }
                case 0xa4: { int b = POPI(), a = POPI(); BRANCH(a <= b); break; }
                case 0xa5: { Obj* b = POPA(); Obj* a = POPA(); BRANCH(a == b); break; }
                case 0xa6: { Obj* b = POPA(); Obj* a = POPA(); BRANCH(a != b); break; }
                case 0xa7: BRANCH(true); break;
                case 0xc6: { Obj* a = POPA(); BRANCH(a == nullptr); break; }
                case 0xc7: { Obj* a = POPA(); BRANCH(a != nullptr); break; }
                case 0xc8: { int32_t off = S4; pc = op_pc + off; break; }
                case 0xaa: { int k = POPI(); pc = (pc + 3) & ~3u; int32_t d = S4, lo = S4, hi = S4; int32_t off = d;
                    if (k >= lo && k <= hi) { uint32_t q = pc + (k - lo) * 4; pc = q; off = S4; } pc = op_pc + off; break; }
                case 0xab: { int k = POPI(); pc = (pc + 3) & ~3u; int32_t d = S4, n = S4, off = d; for (int i = 0; i < n; i++) { int32_t mt = S4, of = S4; if (mt == k) { off = of; } } pc = op_pc + off; break; }
                case 0xac: case 0xae: case 0xb0: ret = st[sp - 1]; return true;
                case 0xad: case 0xaf: ret = st[sp - 2]; return true;
                case 0xb1: return true;
                case 0xb2: case 0xb3: { Ref& r = refM(*this, c, U2); if (!r.sv) { Cls* own = nullptr; V* p = findSF(r.c, r.name, own); if (!p) { own = r.c; p = &r.c->sf[r.name]; } if (!init(own)) goto thrown; r.sv = p; }
                    const bool w = r.ret == 'J' || r.ret == 'D';
                    if (op == 0xb2) { if (w) { st[sp] = *r.sv; sp += 2; } else st[sp++] = *r.sv; } else { if (w) { sp -= 2; *r.sv = st[sp]; } else *r.sv = st[--sp]; } break; }
                case 0xb4: case 0xb5: { Ref& r = refM(*this, c, U2); const bool w = r.ret == 'J' || r.ret == 'D'; V v;
                    if (op == 0xb5) { if (w) { sp -= 2; v = st[sp]; } else v = st[--sp]; }
                    Obj* o = POPA(); NPE(o);
                    if (r.slot < 0) { auto it = r.c->fidx.find(r.name); if (it == r.c->fidx.end()) it = r.c->fidx.emplace(r.name, r.c->nf++).first; r.slot = it->second; }
                    if ((size_t)r.slot >= o->f.size()) o->f.resize(r.slot + 1);
                    if (op == 0xb5) o->f[r.slot] = v; else { if (w) { st[sp] = o->f[r.slot]; sp += 2; } else st[sp++] = o->f[r.slot]; } break; }
                case 0xb6: case 0xb7: case 0xb8: case 0xb9: { Ref& r = refM(*this, c, U2); if (op == 0xb9) pc += 2;
                    const int na = r.nargs + (op == 0xb8 ? 0 : 1); sp -= na; V* a = st + sp; Cls* tg = r.c;
                    if (op == 0xb8) { if (!init(r.c)) goto thrown; } else { NPE(a[0].o); if (op != 0xb7) tg = a[0].o->c; }
                    if (r.lastC != tg) { r.last = find(tg, r.name, r.desc); r.lastC = tg; }
                    V rv; rv.l = 0; if (!invoke(r.last, a, rv, r.name, r.desc, tg)) goto thrown;
                    if (r.ret == 'J' || r.ret == 'D') { st[sp] = rv; sp += 2; } else if (r.ret != 'V') st[sp++] = rv; break; }
                case 0xba: { int idx = U2; pc += 2; if (!indy(c, idx, st, sp)) goto thrown; break; }
                case 0xbb: { int idx = U2; Ref& r = c->rc[idx]; if (!r.ok) { r.c = cls(cf.cname(idx)); r.ok = true; } if (!init(r.c)) goto thrown; PUSHA(mk(r.c)); break; }
                case 0xbc: { int t = U1, n = POPI(); if (n < 0) THROW("java/lang/NegativeArraySizeException", std::to_string(n));
                    static const char T[] = {0, 0, 0, 0, 'Z', 'C', 'F', 'D', 'B', 'S', 'I', 'J'}; PUSHA(arr(T[t & 15], n)); break; }
                case 0xbd: { int idx = U2; int n = POPI(); if (n < 0) THROW("java/lang/NegativeArraySizeException", std::to_string(n)); (void)idx; PUSHA(arr('L', n)); break; }
                case 0xbe: { Obj* a = POPA(); NPE(a); PUSHI((int)a->f.size()); break; }
                case 0xbf: { Obj* e = POPA(); NPE(e); exc = e; goto thrown; }
                case 0xc0: case 0xc1: { int idx = U2; Cls* t = cls(cf.cname(idx)); Obj* o = POPA();
                    if (op == 0xc0) { if (o && o->kind != 2 && t->cf && o->c->cf && !isA(o->c, t)) THROW("java/lang/ClassCastException", o->c->name + " -> " + t->name); PUSHA(o); }
                    else PUSHI(o && (o->kind == 2 ? t->name == "java/lang/Object" || t->name == o->c->name : isA(o->c, t)) ? 1 : 0); break; }
                case 0xc2: case 0xc3: { Obj* o = POPA(); NPE(o); break; }
                case 0xc4: { int o2 = U1; int n = U2; if (o2 == 0x84) L[n].i += S2;
                    else if (o2 >= 0x15 && o2 <= 0x19) { if (o2 == 0x16 || o2 == 0x18) { st[sp] = L[n]; sp += 2; } else st[sp++] = L[n]; }
                    else if (o2 >= 0x36 && o2 <= 0x3a) { if (o2 == 0x37 || o2 == 0x39) { sp -= 2; L[n] = st[sp]; } else L[n] = st[--sp]; } break; }
                case 0xc5: { int idx = U2; int nd = U1; int dims[8]; for (int i = nd - 1; i >= 0; i--) dims[i] = POPI(); const std::string& d = cf.cname(idx); (void)d;
                    struct Mk { static Obj* go(VM& v, int* dm, int n, int lvl) { Obj* a = v.arr(lvl == n - 1 ? 'L' : 'L', dm[0]); if (n > 1) for (int i = 0; i < dm[0]; i++) a->f[i].o = go(v, dm + 1, n - 1, lvl + 1); return a; } };
                    PUSHA(Mk::go(*this, dims, nd, 0)); break; }
                default: THROW("java/lang/InternalError", "opcode nao suportado: " + std::to_string(op));
            }
            continue;
        thrown: {
            Obj* e = exc; bool found = false;
            for (auto& x : m->exc) if (op_pc >= x.start && op_pc < x.end) {
                bool mt = x.type == 0; if (!mt) mt = isA(e->c, cls(cf.cname(x.type)));
                if (mt) { sp = 0; PUSHA(e); exc = nullptr; pc = x.handler; found = true; break; }
            }
            if (!found) return false;
        }
        }
    }
}
