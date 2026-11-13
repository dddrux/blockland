// ---------- jvm: invokedynamic (lambdas e method refs via LambdaMetafactory) ----------
namespace jv {
    struct Lam { int kind = 6; int ncap = 0; Cls* ic = nullptr; std::string name, desc, samDesc; std::vector<V> cap; };   // kind: REF_invokeStatic 6, Virtual 5, Interface 9, Special 7, newInvokeSpecial 8
    static std::vector<char> ptypes(const std::string& d) {   // tipos dos parametros: 'L' (objeto/array) ou primitivo
        std::vector<char> t; size_t i = 1; while (i < d.size() && d[i] != ')') { char c = d[i]; if (c == '[') { while (d[i] == '[') i++; if (d[i] == 'L') i = d.find(';', i); t.push_back('L'); } else if (c == 'L') { i = d.find(';', i); t.push_back('L'); } else t.push_back(c); i++; }
        return t;
    }
    static const char* boxName(char c) { switch (c) { case 'I': return "java/lang/Integer"; case 'J': return "java/lang/Long"; case 'F': return "java/lang/Float"; case 'D': return "java/lang/Double"; case 'Z': return "java/lang/Boolean"; case 'C': return "java/lang/Character"; case 'S': return "java/lang/Short"; default: return "java/lang/Byte"; } }
    static char retType(const std::string& d) { size_t q = d.find(')'); char c = d[q + 1]; return c == '[' ? 'L' : c; }
    static bool lamCall(VM& vm, V* a, V& r, const char*) {
        Lam* L = (Lam*)a[0].o->nat; const bool stc = L->kind == 6 || L->kind == 8;
        std::vector<char> it = ptypes(L->desc); if (!stc) it.insert(it.begin(), 'L'); std::vector<char> sm = ptypes(L->samDesc);
        V args[24]; int n = 0, sp = 1; for (size_t i = 0; i < L->cap.size() && n < 24; i++) args[n++] = L->cap[i];
        for (size_t j = 0; j < sm.size() && L->ncap + j < it.size(); j++) {
            char sc = sm[j], ic = it[L->ncap + j]; V v = a[sp]; sp += (sc == 'J' || sc == 'D') ? 2 : 1;
            if (sc == 'L' && ic != 'L') { if (!v.o) return vm.throwNew("java/lang/NullPointerException", "unboxing"); v = v.o->f[0]; }
            else if (sc != 'L' && ic == 'L') { V b = v; v.o = box(vm, boxName(sc), b); }
            if (n >= 23) return vm.throwNew("java/lang/InternalError", "lambda: aridade"); args[n] = v; n += (ic == 'J' || ic == 'D') ? 2 : 1;
        }
        if (!vm.init(L->ic)) return false;
        if (L->kind == 8) { Obj* o = vm.mk(L->ic); V in[25]; in[0].o = o; for (int i = 0; i < n; i++) in[i + 1] = args[i]; Res rs = vm.find(L->ic, "<init>", L->desc); V rv; rv.l = 0;
            if (!vm.invoke(rs, in, rv, "<init>", L->desc, L->ic)) return false; r.o = o; return true; }
        Cls* tg = L->ic; if (L->kind == 5 || L->kind == 9) { if (!args[0].o) return vm.throwNew("java/lang/NullPointerException"); tg = args[0].o->c; }
        Res rs = vm.find(tg, L->name, L->desc); V rv; rv.l = 0; if (!vm.invoke(rs, args, rv, L->name, L->desc, tg)) return false;
        char ir = retType(L->desc), sr = retType(L->samDesc);
        if (ir != 'V' && ir != 'L' && sr == 'L') rv.o = box(vm, boxName(ir), rv); else if (ir == 'L' && sr != 'L' && sr != 'V') { if (!rv.o) return vm.throwNew("java/lang/NullPointerException", "unboxing"); rv = rv.o->f[0]; }
        r = rv; return true;
    }
    bool VM::indy(Cls* c, int idx, V* st, int& sp) {
        ClassFile& cf = *c->cf; Ref& r = c->rc[idx]; const CP& e = cf.cp[idx];
        if (!r.ok) {
            const CP& nt = cf.cp[e.b]; r.name = cf.utf(nt.a); r.desc = cf.utf(nt.b); r.nargs = descArgs(r.desc, &r.ret);
            if (e.a >= cf.bsm.size() || cf.bsm[e.a].args.size() < 3 || cf.cp[cf.bsm[e.a].args[1]].tag != 15) return throwNew("java/lang/InternalError", "invokedynamic nao suportado (so lambdas): " + r.name);
            const CP& h = cf.cp[cf.bsm[e.a].args[1]]; const CP& mr = cf.cp[h.b]; const CP& mnt = cf.cp[mr.b];
            Lam* L = new Lam; L->kind = h.a; L->ic = cls(cf.cname(mr.a)); L->name = cf.utf(mnt.a); L->desc = cf.utf(mnt.b); L->samDesc = cf.utf(cf.cp[cf.bsm[e.a].args[0]].a); int cnt = 0; descArgs(r.desc, nullptr, &cnt); L->ncap = cnt;
            size_t q = r.desc.find(')'); std::string ifn = r.desc.substr(q + 2, r.desc.size() - q - 3);   // Lpkg/Iface; -> pkg/Iface
            Cls* k = new Cls; k->name = "lambda$" + std::to_string(idx); k->native = true; k->inited = true; k->super = cls("java/lang/Object"); k->ifs.push_back(cls(ifn)); k->nm[r.name] = lamCall;
            r.c = k; r.sv = (V*)L; r.ok = true;
        }
        Lam* base = (Lam*)r.sv; Lam* inst = new Lam(*base); sp -= r.nargs; for (int i = 0; i < r.nargs; i++) inst->cap.push_back(st[sp + i]);
        Obj* o = mk(r.c); o->nat = inst; st[sp++].o = o; return true;
    }
}
