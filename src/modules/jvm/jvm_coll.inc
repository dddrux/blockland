// ---------- jvm: java/util (ArrayList, HashMap, HashSet, Iterator, Arrays, Collections) ----------
// Lista = Obj.f (elementos); Mapa = Obj.f [k0,v0,k1,v1...] em ordem de insercao (busca linear: mapas de mods sao pequenos).
namespace jv {
    static bool objEq(VM& vm, Obj* a, Obj* b) {
        if (a == b) return true; if (!a || !b) return false; if (a->kind == 1 || b->kind == 1) return a->kind == b->kind && a->s == b->s;
        if (a->c == b->c && !a->f.empty() && a->c->name.compare(0, 10, "java/lang/") == 0) return a->f[0].l == b->f[0].l;
        V x[2], r; x[0].o = a; x[1].o = b; r.l = 0; if (vm.callVirt(a, "equals", "(Ljava/lang/Object;)Z", x, r)) return r.i != 0; vm.exc = nullptr; return false;
    }
    static int lfind(VM& vm, Obj* l, Obj* x) { for (size_t i = 0; i < l->f.size(); i++) if (objEq(vm, l->f[i].o, x)) return (int)i; return -1; }
    static int mfind(VM& vm, Obj* m, Obj* k) { for (size_t i = 0; i + 1 < m->f.size(); i += 2) if (objEq(vm, m->f[i].o, k)) return (int)i; return -1; }
    static Obj* newList(VM& vm) { return vm.mk(vm.cls("java/util/ArrayList")); }
    static bool cmpCall(VM& vm, Obj* cmp, Obj* x, Obj* y, int& out) {
        if (cmp) { V a[3], r; a[0].o = cmp; a[1].o = x; a[2].o = y; r.l = 0; if (!vm.callVirt(cmp, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I", a, r)) return false; out = r.i; return true; }
        if (x->kind == 1) { int c = x->s.compare(y->s); out = c < 0 ? -1 : c > 0; return true; }
        V a[2], r; a[0].o = x; a[1].o = y; r.l = 0; if (!vm.callVirt(x, "compareTo", "(Ljava/lang/Object;)I", a, r)) return false; out = r.i; return true;
    }
    static bool sortList(VM& vm, Obj* l, Obj* cmp) {   // insercao estavel
        for (size_t i = 1; i < l->f.size(); i++) { V k = l->f[i]; size_t j = i; for (; j > 0; j--) { int c; if (!cmpCall(vm, cmp, l->f[j - 1].o, k.o, c)) return false; if (c <= 0) break; l->f[j] = l->f[j - 1]; } l->f[j] = k; }
        return true;
    }
    static void setupColl(VM& vm) {
        for (const char* n : {"java/lang/Iterable", "java/util/Collection", "java/util/List", "java/util/Set", "java/util/Map", "java/util/Iterator", "java/util/Map$Entry", "java/lang/Comparable", "java/lang/Runnable", "java/util/Comparator", "java/util/function/Supplier", "java/util/function/Function", "java/util/function/Consumer", "java/util/function/Predicate", "java/util/function/BiFunction", "java/util/function/BiConsumer", "java/lang/CharSequence"}) vm.defClass(n, "java/lang/Object");
        for (const char* n : {"java/util/ArrayList", "java/util/LinkedList", "java/util/HashMap", "java/util/LinkedHashMap", "java/util/TreeMap", "java/util/HashSet", "java/util/LinkedHashSet", "java/util/ArrayList$Itr", "java/util/AbstractMap$SimpleEntry", "java/util/Arrays", "java/util/Collections", "java/util/Objects"}) vm.defClass(n, "java/lang/Object");
        Cls* L = vm.cls("java/util/ArrayList"); L->ifs = {vm.cls("java/util/List"), vm.cls("java/util/Collection"), vm.cls("java/lang/Iterable")};
        vm.cls("java/util/LinkedList")->super = L; vm.cls("java/util/HashSet")->super = L; vm.cls("java/util/LinkedHashSet")->super = L;   // Set = lista sem repeticao (add filtra)
        vm.cls("java/util/HashSet")->ifs = {vm.cls("java/util/Set")};
        for (const char* n : {"java/util/HashMap", "java/util/LinkedHashMap", "java/util/TreeMap"}) vm.cls(n)->ifs = {vm.cls("java/util/Map")};
        vm.cls("java/util/LinkedHashMap")->super = vm.cls("java/util/HashMap"); vm.cls("java/util/TreeMap")->super = vm.cls("java/util/HashMap");
        vm.cls("java/util/ArrayList$Itr")->ifs = {vm.cls("java/util/Iterator")}; vm.cls("java/util/ArrayList$Itr")->nf = 2;
        // ---- lista ----
        vm.def("java/util/ArrayList", "<init>", NF { Obj* o = a[0].o; if (d[1] == 'L' && a[1].o) o->f = a[1].o->f; return true; });
        vm.def("java/util/ArrayList", "add", NF { Obj* o = a[0].o; bool set = o->c->name.find("Set") != std::string::npos;
            if (d[1] == 'I') { if ((uint32_t)a[1].i > o->f.size()) return vm.throwNew("java/lang/IndexOutOfBoundsException", std::to_string(a[1].i)); o->f.insert(o->f.begin() + a[1].i, a[2]); return true; }
            if (set && lfind(vm, o, a[1].o) >= 0) { r.i = 0; return true; } o->f.push_back(a[1]); r.i = 1; return true; });
        vm.def("java/util/ArrayList", "get", NF { Obj* o = a[0].o; if ((uint32_t)a[1].i >= o->f.size()) return vm.throwNew("java/lang/IndexOutOfBoundsException", "Index: " + std::to_string(a[1].i) + ", Size: " + std::to_string(o->f.size())); r = o->f[a[1].i]; return true; });
        vm.def("java/util/ArrayList", "set", NF { Obj* o = a[0].o; if ((uint32_t)a[1].i >= o->f.size()) return vm.throwNew("java/lang/IndexOutOfBoundsException", std::to_string(a[1].i)); r = o->f[a[1].i]; o->f[a[1].i] = a[2]; return true; });
        vm.def("java/util/ArrayList", "size", NF { r.i = (int)a[0].o->f.size(); return true; });
        vm.def("java/util/ArrayList", "isEmpty", NF { r.i = a[0].o->f.empty(); return true; });
        vm.def("java/util/ArrayList", "clear", NF { a[0].o->f.clear(); return true; });
        vm.def("java/util/ArrayList", "contains", NF { r.i = lfind(vm, a[0].o, a[1].o) >= 0; return true; });
        vm.def("java/util/ArrayList", "indexOf", NF { r.i = lfind(vm, a[0].o, a[1].o); return true; });
        vm.def("java/util/ArrayList", "remove", NF { Obj* o = a[0].o; if (d[1] == 'I') { if ((uint32_t)a[1].i >= o->f.size()) return vm.throwNew("java/lang/IndexOutOfBoundsException", std::to_string(a[1].i)); r = o->f[a[1].i]; o->f.erase(o->f.begin() + a[1].i); return true; }
            int i = lfind(vm, o, a[1].o); if (i >= 0) o->f.erase(o->f.begin() + i); r.i = i >= 0; return true; });
        vm.def("java/util/ArrayList", "addAll", NF { Obj* o = a[0].o; Obj* s = a[1].o; if (!s) return vm.throwNew("java/lang/NullPointerException"); bool ch = false; bool set = o->c->name.find("Set") != std::string::npos;
            for (V v : std::vector<V>(s->f)) if (!set || lfind(vm, o, v.o) < 0) { o->f.push_back(v); ch = true; } r.i = ch; return true; });
        vm.def("java/util/ArrayList", "iterator", NF { Obj* it = vm.mk(vm.cls("java/util/ArrayList$Itr")); it->f.resize(2); it->f[0].o = a[0].o; it->f[1].i = 0; r.o = it; return true; });
        vm.def("java/util/ArrayList", "toArray", NF { Obj* ar = vm.arr('L', (int)a[0].o->f.size()); ar->f = a[0].o->f; r.o = ar; return true; });
        vm.def("java/util/ArrayList", "sort", NF { return sortList(vm, a[0].o, a[1].o); });
        vm.def("java/util/ArrayList", "forEach", NF { Obj* l = a[0].o; for (size_t i = 0; i < l->f.size(); i++) { V x[2], rv; x[0] = a[1]; x[1] = l->f[i]; rv.l = 0; if (!vm.callVirt(a[1].o, "accept", "(Ljava/lang/Object;)V", x, rv)) return false; } return true; });
        vm.def("java/util/ArrayList", "equals", NF { Obj *x = a[0].o, *y = a[1].o; if (x == y) { r.i = 1; return true; } r.i = 0; if (!y || y->f.size() != x->f.size()) return true; for (size_t i = 0; i < x->f.size(); i++) if (!objEq(vm, x->f[i].o, y->f[i].o)) return true; r.i = 1; return true; });
        vm.def("java/util/ArrayList", "toString", NF { std::string s = "["; Obj* o = a[0].o; for (size_t i = 0; i < o->f.size(); i++) s += (i ? ", " : "") + objStr(vm, o->f[i].o); r.o = vm.str(s + "]"); return true; });
        vm.def("java/util/ArrayList$Itr", "hasNext", NF { Obj* it = a[0].o; r.i = it->f[1].i < (int)it->f[0].o->f.size(); return true; });
        vm.def("java/util/ArrayList$Itr", "next", NF { Obj* it = a[0].o; if (it->f[1].i >= (int)it->f[0].o->f.size()) return vm.throwNew("java/util/NoSuchElementException"); r = it->f[0].o->f[it->f[1].i++]; return true; });
        // ---- mapa ----
        vm.def("java/util/HashMap", "<init>", NF { return true; });
        vm.def("java/util/HashMap", "put", NF { Obj* m = a[0].o; int i = mfind(vm, m, a[1].o); if (i >= 0) { r = m->f[i + 1]; m->f[i + 1] = a[2]; } else { m->f.push_back(a[1]); m->f.push_back(a[2]); r.o = nullptr; } return true; });
        vm.def("java/util/HashMap", "putIfAbsent", NF { Obj* m = a[0].o; int i = mfind(vm, m, a[1].o); if (i >= 0 && m->f[i + 1].o) { r = m->f[i + 1]; return true; } if (i >= 0) m->f[i + 1] = a[2]; else { m->f.push_back(a[1]); m->f.push_back(a[2]); } r.o = nullptr; return true; });
        vm.def("java/util/HashMap", "get", NF { Obj* m = a[0].o; int i = mfind(vm, m, a[1].o); r.o = i >= 0 ? m->f[i + 1].o : nullptr; return true; });
        vm.def("java/util/HashMap", "getOrDefault", NF { Obj* m = a[0].o; int i = mfind(vm, m, a[1].o); r.o = i >= 0 ? m->f[i + 1].o : a[2].o; return true; });
        vm.def("java/util/HashMap", "containsKey", NF { r.i = mfind(vm, a[0].o, a[1].o) >= 0; return true; });
        vm.def("java/util/HashMap", "remove", NF { Obj* m = a[0].o; int i = mfind(vm, m, a[1].o); r.o = nullptr; if (i >= 0) { r = m->f[i + 1]; m->f.erase(m->f.begin() + i, m->f.begin() + i + 2); } return true; });
        vm.def("java/util/HashMap", "size", NF { r.i = (int)a[0].o->f.size() / 2; return true; });
        vm.def("java/util/HashMap", "isEmpty", NF { r.i = a[0].o->f.empty(); return true; });
        vm.def("java/util/HashMap", "clear", NF { a[0].o->f.clear(); return true; });
        vm.def("java/util/HashMap", "keySet", NF { Obj* l = newList(vm); for (size_t i = 0; i < a[0].o->f.size(); i += 2) l->f.push_back(a[0].o->f[i]); r.o = l; return true; });
        vm.def("java/util/HashMap", "values", NF { Obj* l = newList(vm); for (size_t i = 1; i < a[0].o->f.size(); i += 2) l->f.push_back(a[0].o->f[i]); r.o = l; return true; });
        vm.def("java/util/HashMap", "entrySet", NF { Obj* l = newList(vm); Cls* ec = vm.cls("java/util/AbstractMap$SimpleEntry"); for (size_t i = 0; i + 1 < a[0].o->f.size(); i += 2) { Obj* e = vm.mk(ec); e->f = {a[0].o->f[i], a[0].o->f[i + 1]}; V v; v.o = e; l->f.push_back(v); } r.o = l; return true; });
        vm.def("java/util/AbstractMap$SimpleEntry", "getKey", NF { r = a[0].o->f[0]; return true; });
        vm.def("java/util/AbstractMap$SimpleEntry", "getValue", NF { r = a[0].o->f[1]; return true; });
        vm.cls("java/util/AbstractMap$SimpleEntry")->ifs = {vm.cls("java/util/Map$Entry")};
        // ---- Arrays / Collections ----
        vm.def("java/util/Arrays", "asList", NF { Obj* l = newList(vm); if (a[0].o) l->f = a[0].o->f; r.o = l; return true; });
        vm.def("java/util/Collections", "emptyList", NF { r.o = newList(vm); return true; });
        vm.def("java/util/Collections", "singletonList", NF { Obj* l = newList(vm); l->f.push_back(a[0]); r.o = l; return true; });
        vm.def("java/util/Collections", "sort", NF { return sortList(vm, a[0].o, strstr(d, "Comparator") ? a[1].o : nullptr); });
        vm.def("java/util/Objects", "requireNonNull", NF { if (!a[0].o) return vm.throwNew("java/lang/NullPointerException", d[1] == 'L' && d[2] == 'j' && strstr(d, "String") && a[1].o ? a[1].o->s : ""); r = a[0]; return true; });
        vm.def("java/util/Objects", "equals", NF { r.i = objEq(vm, a[0].o, a[1].o); return true; });
        vm.def("java/util/Collections", "unmodifiableList", NF { r.o = a[0].o; return true; });
    }
}
