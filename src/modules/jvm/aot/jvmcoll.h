// patchJVM.2: colecoes do JDK para o runtime AOT (ArrayList, HashMap/HashSet com a MESMA ordem de iteracao do Java, Iterator, Entry, Collections, Arrays). Incluido por jvmrt.h dentro de namespace rt.
inline V callFn(Obj* f, const char* name, const char* desc, V* a, int n) { if (!f) throwNew("java/lang/NullPointerException", ""); if (f->lm) return callLambda(f, a, n); std::vector<V> b(1); b[0].a = f; b.insert(b.end(), a, a + n); MRef m{"java/lang/Object", name, desc}; return dispatch(f->ci, m, b.data(), n + 1); }
inline int32_t hashOf(Obj* o) { if (!o) return 0; V a[1]; a[0].a = o; MRef m{"java/lang/Object", "hashCode", "()I"}; return dispatch(o->ci, m, a, 1).i; }
inline bool eqObj(Obj* x, Obj* y) { if (x == y) return true; if (!x || !y) return false; V a[2]; a[0].a = x; a[1].a = y; MRef m{"java/lang/Object", "equals", "(Ljava/lang/Object;)Z"}; return dispatch(x->ci, m, a, 2).i != 0; }
inline int32_t cmpObj(Obj* x, Obj* y) {
    if (x && y && x->ci->name == "java/lang/String") { int c = x->s.compare(y->s); return c < 0 ? -1 : c > 0 ? 1 : 0; }
    if (x && y && x->ci->name == "java/lang/Integer") { int32_t p = x->f["value"].i, q = y->f["value"].i; return (p > q) - (p < q); }
    V a[2]; a[0].a = nn(x); a[1].a = y; MRef m{"java/lang/Comparable", "compareTo", "(Ljava/lang/Object;)I"}; return dispatch(x->ci, m, a, 2).i; }
inline Obj* newList() { Obj* o = mk("java/util/ArrayList"); return o; }
inline Obj* sortedBy(Obj* l, Obj* cmp) {
    std::vector<V>& v = l->arr; std::vector<V> t(v);
    std::stable_sort(t.begin(), t.end(), [&](const V& x, const V& y) { if (cmp) { V a[2]; a[0] = x; a[1] = y; return callFn(cmp, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I", a, 2).i < 0; } return cmpObj(x.a, y.a) < 0; });
    v = t; return l; }
inline int32_t spread(int32_t h) { return h ^ (int32_t)((uint32_t)h >> 16); }
// mapa: arr = entradas (Obj "java/util/HashMap$Node": key,value) em ordem de insercao; f["cap"] = capacidade da tabela do Java
inline int findKey(Obj* m, Obj* k) { int32_t h = hashOf(k); for (size_t i = 0; i < m->arr.size(); i++) { Obj* e = m->arr[i].a; if (e->f["h"].i == h && eqObj(e->f["key"].a, k)) return (int)i; } return -1; }
inline void mapNew(Obj* m) { m->f["cap"].i = 16; }
inline V mapPut(Obj* m, V k, V v) { int i = findKey(m, k.a); if (i >= 0) { Obj* e = m->arr[i].a; V old = e->f["value"]; e->f["value"] = v; return old; }
    Obj* e = mk("java/util/HashMap$Node"); e->f["key"] = k; e->f["value"] = v; e->f["h"].i = hashOf(k.a); V ev; ev.a = e; m->arr.push_back(ev);
    int cap = m->f["cap"].i ? m->f["cap"].i : 16; while ((double)m->arr.size() > 0.75 * cap) cap *= 2; m->f["cap"].i = cap; return V(); }
inline std::vector<V> mapOrder(Obj* m) { if (m->f["linked"].i) return m->arr; int cap = m->f["cap"].i ? m->f["cap"].i : 16; std::vector<V> t(m->arr);
    std::stable_sort(t.begin(), t.end(), [&](const V& x, const V& y) { return (spread(x.a->f["h"].i) & (cap - 1)) < (spread(y.a->f["h"].i) & (cap - 1)); }); return t; }
inline std::string seqStr(const std::vector<V>& v, int mode) { std::string s = mode == 2 ? "{" : "["; for (size_t i = 0; i < v.size(); i++) { if (i) s += ", "; if (mode == 2) s += toStr(v[i].a->f["key"].a) + "=" + toStr(v[i].a->f["value"].a); else s += toStr(v[i].a); } return s + (mode == 2 ? "}" : "]"); }
inline V iterOf(std::vector<V> items) { Obj* it = mk("java/util/ArrayList$Itr"); it->arr = std::move(items); it->f["i"].i = 0; V r; r.a = it; return r; }
inline void initJdk2(); inline void initMc();
inline void initColl() {
    for (auto p : {"java/util/ArrayList:Object", "java/util/HashMap:Object", "java/util/HashSet:Object", "java/util/HashMap$Node:Object", "java/util/ArrayList$Itr:Object", "java/util/Collections:Object", "java/util/Arrays:Object"}) {
        std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), ("java/lang/" + s.substr(c + 1)).c_str()); }
    ci("java/util/ArrayList")->ifs = {"java/util/List", "java/util/Collection", "java/lang/Iterable"}; ci("java/util/HashSet")->ifs = {"java/util/Set", "java/util/Collection", "java/lang/Iterable"};
    ci("java/util/HashMap")->ifs = {"java/util/Map"}; ci("java/util/HashMap$Node")->ifs = {"java/util/Map$Entry"};
    ci("java/lang/String")->ifs = {"java/lang/Comparable", "java/lang/CharSequence"}; ci("java/lang/Integer")->ifs = {"java/lang/Comparable"};
    const char* OBJ = "Ljava/lang/Object;";
    auto L = [](const char* k, Nat f) { N_((std::string("java/util/ArrayList.") + k).c_str(), f); };
    L("<init>()V", [](V* a) { return V(); }); L("<init>(I)V", [](V* a) { return V(); });
    L("add(Ljava/lang/Object;)Z", [](V* a) { a[0].a->arr.push_back(a[1]); return vi(1); });
    L("add(ILjava/lang/Object;)V", [](V* a) { auto& v = a[0].a->arr; int i = a[1].i; if (i < 0 || i > (int)v.size()) throwNew("java/lang/IndexOutOfBoundsException", "Index: " + std::to_string(i) + ", Size: " + std::to_string(v.size())); v.insert(v.begin() + i, a[2]); return V(); });
    auto chk = [](V* a) { auto& v = a[0].a->arr; int i = a[1].i; if (i < 0 || i >= (int)v.size()) throwNew("java/lang/IndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(v.size())); };
    L("get(I)Ljava/lang/Object;", [chk](V* a) { chk(a); return a[0].a->arr[a[1].i]; });
    L("set(ILjava/lang/Object;)Ljava/lang/Object;", [chk](V* a) { chk(a); V o = a[0].a->arr[a[1].i]; a[0].a->arr[a[1].i] = a[2]; return o; });
    L("remove(I)Ljava/lang/Object;", [chk](V* a) { chk(a); V o = a[0].a->arr[a[1].i]; a[0].a->arr.erase(a[0].a->arr.begin() + a[1].i); return o; });
    L("size()I", [](V* a) { return vi((int32_t)a[0].a->arr.size()); }); L("isEmpty()Z", [](V* a) { return vi(a[0].a->arr.empty()); }); L("clear()V", [](V* a) { a[0].a->arr.clear(); return V(); });
    auto idx = [](Obj* l, Obj* x) { for (size_t i = 0; i < l->arr.size(); i++) if (eqObj(x, l->arr[i].a)) return (int)i; return -1; };
    L("indexOf(Ljava/lang/Object;)I", [idx](V* a) { return vi(idx(a[0].a, a[1].a)); }); L("contains(Ljava/lang/Object;)Z", [idx](V* a) { return vi(idx(a[0].a, a[1].a) >= 0); });
    L("iterator()Ljava/util/Iterator;", [](V* a) { return iterOf(a[0].a->arr); });
    L("sort(Ljava/util/Comparator;)V", [](V* a) { sortedBy(a[0].a, a[1].a); return V(); });
    L("forEach(Ljava/util/function/Consumer;)V", [](V* a) { std::vector<V> t(a[0].a->arr); for (auto& e : t) { V x[1]; x[0] = e; callFn(a[1].a, "accept", "(Ljava/lang/Object;)V", x, 1); } return V(); });
    L("toString()Ljava/lang/String;", [](V* a) { return va(nstr(seqStr(a[0].a->arr, 1))); });
    L("hashCode()I", [](V* a) { int32_t h = 1; for (auto& e : a[0].a->arr) h = 31 * h + hashOf(e.a); return vi(h); });
    N_("java/util/ArrayList$Itr.hasNext()Z", [](V* a) { return vi(a[0].a->f["i"].i < (int)a[0].a->arr.size()); });
    N_("java/util/ArrayList$Itr.next()Ljava/lang/Object;", [](V* a) { Obj* it = a[0].a; if (it->f["i"].i >= (int)it->arr.size()) throwNew("java/util/NoSuchElementException", ""); return it->arr[it->f["i"].i++]; });
    N_("java/util/Collections.sort(Ljava/util/List;)V", [](V* a) { sortedBy(a[0].a, nullptr); return V(); });
    N_("java/util/Collections.sort(Ljava/util/List;Ljava/util/Comparator;)V", [](V* a) { sortedBy(a[0].a, a[1].a); return V(); });
    N_("java/util/Arrays.asList([Ljava/lang/Object;)Ljava/util/List;", [](V* a) { Obj* l = newList(); l->arr = nn(a[0].a)->arr; return va(l); });
    auto M = [](const char* k, Nat f) { N_((std::string("java/util/HashMap.") + k).c_str(), f); };
    M("<init>()V", [](V* a) { mapNew(a[0].a); return V(); }); M("<init>(I)V", [](V* a) { mapNew(a[0].a); return V(); });
    M("put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", [](V* a) { return mapPut(a[0].a, a[1], a[2]); });
    M("get(Ljava/lang/Object;)Ljava/lang/Object;", [](V* a) { int i = findKey(a[0].a, a[1].a); return i < 0 ? V() : a[0].a->arr[i].a->f["value"]; });
    M("getOrDefault(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", [](V* a) { int i = findKey(a[0].a, a[1].a); return i < 0 ? a[2] : a[0].a->arr[i].a->f["value"]; });
    M("containsKey(Ljava/lang/Object;)Z", [](V* a) { return vi(findKey(a[0].a, a[1].a) >= 0); }); M("size()I", [](V* a) { return vi((int32_t)a[0].a->arr.size()); });
    M("isEmpty()Z", [](V* a) { return vi(a[0].a->arr.empty()); }); M("clear()V", [](V* a) { a[0].a->arr.clear(); return V(); });
    M("remove(Ljava/lang/Object;)Ljava/lang/Object;", [](V* a) { int i = findKey(a[0].a, a[1].a); if (i < 0) return V(); V o = a[0].a->arr[i].a->f["value"]; a[0].a->arr.erase(a[0].a->arr.begin() + i); return o; });
    M("entrySet()Ljava/util/Set;", [](V* a) { Obj* l = newList(); l->arr = mapOrder(a[0].a); return va(l); });
    M("keySet()Ljava/util/Set;", [](V* a) { Obj* l = newList(); for (auto& e : mapOrder(a[0].a)) l->arr.push_back(e.a->f["key"]); return va(l); });
    M("values()Ljava/util/Collection;", [](V* a) { Obj* l = newList(); for (auto& e : mapOrder(a[0].a)) l->arr.push_back(e.a->f["value"]); return va(l); });
    M("toString()Ljava/lang/String;", [](V* a) { return va(nstr(seqStr(mapOrder(a[0].a), 2))); });
    N_("java/util/HashMap$Node.getKey()Ljava/lang/Object;", [](V* a) { return a[0].a->f["key"]; }); N_("java/util/HashMap$Node.getValue()Ljava/lang/Object;", [](V* a) { return a[0].a->f["value"]; });
    auto S = [](const char* k, Nat f) { N_((std::string("java/util/HashSet.") + k).c_str(), f); };
    S("<init>()V", [](V* a) { mapNew(a[0].a); return V(); });
    S("add(Ljava/lang/Object;)Z", [](V* a) { if (findKey(a[0].a, a[1].a) >= 0) return vi(0); mapPut(a[0].a, a[1], V()); return vi(1); });
    S("contains(Ljava/lang/Object;)Z", [](V* a) { return vi(findKey(a[0].a, a[1].a) >= 0); }); S("size()I", [](V* a) { return vi((int32_t)a[0].a->arr.size()); });
    S("remove(Ljava/lang/Object;)Z", [](V* a) { int i = findKey(a[0].a, a[1].a); if (i < 0) return vi(0); a[0].a->arr.erase(a[0].a->arr.begin() + i); return vi(1); });
    S("iterator()Ljava/util/Iterator;", [](V* a) { std::vector<V> k; for (auto& e : mapOrder(a[0].a)) k.push_back(e.a->f["key"]); return iterOf(k); });
    S("toString()Ljava/lang/String;", [](V* a) { std::vector<V> k; for (auto& e : mapOrder(a[0].a)) k.push_back(e.a->f["key"]); return va(nstr(seqStr(k, 1))); });
    initJdk2(); initMc();
}
// ---- patchJVM.3: Random (LCG identico ao Java), Enum, String.format, clone de array
inline int32_t rndNext(Obj* r, int bits) { int64_t& s = r->f["seed"].j; s = (int64_t)(((uint64_t)s * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1)); return (int32_t)((uint64_t)s >> (48 - bits)); }
inline void rndSeed(Obj* r, int64_t s) { r->f["seed"].j = (int64_t)(((uint64_t)s ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1)); }
inline std::string jfmt(const std::string& f, Obj* args) {
    std::string o; size_t ai = 0;
    for (size_t i = 0; i < f.size(); i++) {
        if (f[i] != '%') { o += f[i]; continue; }
        size_t j = i + 1; std::string fl; while (j < f.size() && strchr("-0+ ,#", f[j])) fl += f[j++];
        std::string w; while (j < f.size() && isdigit((unsigned char)f[j])) w += f[j++]; std::string pr; if (j < f.size() && f[j] == '.') { j++; while (j < f.size() && isdigit((unsigned char)f[j])) pr += f[j++]; }
        char c = j < f.size() ? f[j] : '%'; i = j; if (c == 'n') { o += '\n'; continue; } if (c == '%') { o += '%'; continue; }
        V a; if (args && ai < args->arr.size()) a = args->arr[ai++]; else throwNew("java/util/MissingFormatArgumentException", std::string("Format specifier '%") + c + "'");
        Obj* b = a.a; std::string r; char buf[128];
        auto fv = [&](const char* k) { return b->f[k]; };
        std::string cf = "%" + fl + w;
        if (c == 'd') { long long x = b->ci->name == "java/lang/Long" ? fv("value").j : fv("value").i; snprintf(buf, 128, (cf + "lld").c_str(), x); r = buf; }
        else if (c == 'x' || c == 'X') { long long x = b->ci->name == "java/lang/Long" ? fv("value").j : (uint32_t)fv("value").i; snprintf(buf, 128, (cf + (c == 'x' ? "llx" : "llX")).c_str(), x); r = buf; }
        else if (c == 'f') { double x = b->ci->name == "java/lang/Float" ? fv("value").f : fv("value").d; snprintf(buf, 128, (cf + "." + (pr.empty() ? "6" : pr) + "f").c_str(), x); r = buf; }
        else { std::string s = c == 'c' ? chr(fv("value").i) : c == 'b' ? (b && (b->ci->name != "java/lang/Boolean" || fv("value").i) ? "true" : "false") : toStr(b); if (c == 'S') for (auto& ch : s) ch = toupper(ch); if (!pr.empty()) s = s.substr(0, atoi(pr.c_str()));
            int wd = atoi(w.c_str()); while ((int)s.size() < wd) s = fl.find('-') != std::string::npos ? s + " " : " " + s; r = s; }
        o += r; }
    return o; }
inline void initJdk2() {
    declare("java/util/Random", "java/lang/Object"); declare("java/lang/Enum", "java/lang/Object"); ci("java/lang/Enum")->ifs = {"java/lang/Comparable"};
    N_("java/util/Random.<init>()V", [](V* a) { rndSeed(a[0].a, (int64_t)time(nullptr) * 1000003LL + (int64_t)(uintptr_t)a[0].a); return V(); }); N_("java/util/Random.<init>(J)V", [](V* a) { rndSeed(a[0].a, a[1].j); return V(); });
    N_("java/util/Random.setSeed(J)V", [](V* a) { rndSeed(a[0].a, a[1].j); return V(); });
    N_("java/util/Random.nextInt()I", [](V* a) { return vi(rndNext(a[0].a, 32)); }); N_("java/util/Random.nextBoolean()Z", [](V* a) { return vi(rndNext(a[0].a, 1) != 0); });
    N_("java/util/Random.nextInt(I)I", [](V* a) { Obj* r = a[0].a; int32_t bound = a[1].i; if (bound <= 0) throwNew("java/lang/IllegalArgumentException", "bound must be positive");
        int32_t x = rndNext(r, 31), m = bound - 1; if ((bound & m) == 0) return vi((int32_t)(((int64_t)bound * (int64_t)x) >> 31)); for (int32_t u = x; (int32_t)((uint32_t)u - (uint32_t)(x = u % bound) + (uint32_t)m) < 0; u = rndNext(r, 31)); return vi(x); });
    N_("java/util/Random.nextLong()J", [](V* a) { int64_t h = (int64_t)rndNext(a[0].a, 32); int64_t l = rndNext(a[0].a, 32); return vj((int64_t)((uint64_t)h << 32) + l); });
    N_("java/util/Random.nextDouble()D", [](V* a) { int64_t h = rndNext(a[0].a, 26); int64_t l = rndNext(a[0].a, 27); return vd((double)((h << 27) + l) * (1.0 / (double)(1LL << 53))); });
    N_("java/util/Random.nextFloat()F", [](V* a) { V v; v.f = rndNext(a[0].a, 24) / (float)(1 << 24); return v; });
    N_("java/lang/Enum.<init>(Ljava/lang/String;I)V", [](V* a) { a[0].a->f["name"] = a[1]; a[0].a->f["ordinal"] = a[2]; return V(); });
    N_("java/lang/Enum.name()Ljava/lang/String;", [](V* a) { return a[0].a->f["name"]; }); N_("java/lang/Enum.toString()Ljava/lang/String;", [](V* a) { return a[0].a->f["name"]; });
    N_("java/lang/Enum.ordinal()I", [](V* a) { return a[0].a->f["ordinal"]; }); N_("java/lang/Enum.compareTo(Ljava/lang/Object;)I", [](V* a) { return vi(a[0].a->f["ordinal"].i - a[1].a->f["ordinal"].i); }); N_("java/lang/Enum.compareTo(Ljava/lang/Enum;)I", [](V* a) { return vi(a[0].a->f["ordinal"].i - a[1].a->f["ordinal"].i); });
    N_("java/lang/Enum.valueOf(Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Enum;", [](V* a) { ClassInfo* c = ci(nn(a[0].a)->s); ensureInit(c); std::string n = nn(a[1].a)->s;
        for (auto& kv : c->statics) { Obj* o = kv.second.a; if (o && o->ci == c && o->f.count("name") && o->f["name"].a && o->f["name"].a->s == n) return va(o); }
        { std::string cn = c->name; for (auto& ch : cn) if (ch == '/' || ch == '$') ch = '.'; throwNew("java/lang/IllegalArgumentException", "No enum constant " + cn + "." + n); } });
    N_("java/lang/Object.clone()Ljava/lang/Object;", [](V* a) { Obj* o = new Obj(*nn(a[0].a)); return va(o); });
    N_("java/lang/String.format(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;", [](V* a) { return va(nstr(jfmt(nn(a[0].a)->s, a[1].a))); });
    N_("java/lang/String.valueOf(Ljava/lang/Object;)Ljava/lang/String;", [](V* a) { return va(nstr(toStr(a[0].a))); });
    N_("java/lang/String.contains(Ljava/lang/CharSequence;)Z", [](V* a) { return vi(a[0].a->s.find(nn(a[1].a)->s) != std::string::npos); });
    N_("java/lang/String.startsWith(Ljava/lang/String;)Z", [](V* a) { return vi(a[0].a->s.rfind(nn(a[1].a)->s, 0) == 0); });
    N_("java/lang/String.compareTo(Ljava/lang/Object;)I", [](V* a) { return vi(cmpObj(a[0].a, a[1].a)); }); N_("java/lang/String.compareTo(Ljava/lang/String;)I", [](V* a) { return vi(cmpObj(a[0].a, a[1].a)); });
    N_("java/lang/String.concat(Ljava/lang/String;)Ljava/lang/String;", [](V* a) { return va(nstr(a[0].a->s + nn(a[1].a)->s)); });
    N_("java/lang/Math.floor(D)D", [](V* a) { return vd(std::floor(a[0].d)); }); N_("java/lang/Math.ceil(D)D", [](V* a) { return vd(std::ceil(a[0].d)); });
    N_("java/lang/Math.max(DD)D", [](V* a) { return vd(std::max(a[0].d, a[1].d)); }); N_("java/lang/Math.min(DD)D", [](V* a) { return vd(std::min(a[0].d, a[1].d)); });
    N_("java/lang/Math.pow(DD)D", [](V* a) { return vd(std::pow(a[0].d, a[1].d)); }); N_("java/lang/Math.round(D)J", [](V* a) { return vj((int64_t)std::floor(a[0].d + 0.5)); });
    N_("java/lang/Math.min(FF)F", [](V* a) { V v; v.f = std::min(a[0].f, a[1].f); return v; }); N_("java/lang/Math.max(FF)F", [](V* a) { V v; v.f = std::max(a[0].f, a[1].f); return v; });
}
