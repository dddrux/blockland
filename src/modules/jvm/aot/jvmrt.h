// patchJVM.1: runtime minimo do codigo gerado por conversor/modconv/jvmlift.py (bytecode -> C++). Objetos, classes, excecoes, despacho virtual e JDK basico (String, StringBuilder, Math, Integer, println).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <climits>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <regex>
#include <algorithm>
#include <ctime>
#include <charconv>
namespace rt {
struct Obj; struct ClassInfo;
struct V { union { int32_t i; int64_t j; float f; double d; Obj* a; }; V() : j(0) {} };
typedef V (*Fn)(const V*);
struct MDef { const char* name; const char* desc; int acc; Fn fn; };
struct MRef { const char* owner; const char* name; const char* desc; int ik = 0; const char* io = nullptr; const char* in = nullptr; const char* id = nullptr; const char* inst = nullptr; };
struct FRef { const char* owner; const char* name; const char* desc; };
struct Thrown { Obj* ex; };
struct ClassInfo { std::string name, sup; std::vector<std::string> ifs; std::vector<MDef> m; std::unordered_map<std::string, V> statics; bool inited = false; Fn clinit = nullptr; };
struct Obj { ClassInfo* ci = nullptr; std::string s; std::vector<V> arr; std::unordered_map<std::string, V> f; char at = 0; const MRef* lm = nullptr; };
typedef std::function<V(V*)> Nat;
inline std::unordered_map<std::string, ClassInfo>& classes() { static std::unordered_map<std::string, ClassInfo> c; return c; }
inline std::unordered_map<std::string, Nat>& natives() { static std::unordered_map<std::string, Nat> n; return n; }
inline std::unordered_map<std::string, Obj*>& interned() { static std::unordered_map<std::string, Obj*> n; return n; }
inline std::vector<std::string>& missing() { static std::vector<std::string> m; return m; }
inline ClassInfo* ci(const std::string& n) { auto& c = classes(); auto it = c.find(n); if (it == c.end()) { it = c.emplace(n, ClassInfo()).first; it->second.name = n; if (n != "java/lang/Object") it->second.sup = "java/lang/Object"; } return &it->second; }
inline void declare(const char* n, const char* sup) { ci(n)->sup = sup; }
struct Reg { Reg(const char* n, const char* sup, std::initializer_list<const char*> ifs, const MDef* d, int cnt) {
    ClassInfo* c = ci(n); c->sup = sup; for (auto i : ifs) c->ifs.push_back(i); for (int k = 0; k < cnt; k++) { c->m.push_back(d[k]); if (!strcmp(d[k].name, "<clinit>")) c->clinit = d[k].fn; } } };
[[noreturn]] inline void unsupported(const char* w) { fprintf(stderr, "jvmrt: nao suportado: %s\n", w); throw Thrown{nullptr}; }
inline float mkf(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }
inline double mkd(uint64_t b) { double d; memcpy(&d, &b, 8); return d; }
inline Obj* mk(const char* cls) { Obj* o = new Obj(); o->ci = ci(cls); return o; }
inline Obj* str(const std::string& s) { auto& m = interned(); auto it = m.find(s); if (it != m.end()) return it->second; Obj* o = mk("java/lang/String"); o->s = s; m[s] = o; return o; }
inline Obj* nstr(const std::string& s) { Obj* o = mk("java/lang/String"); o->s = s; return o; }
inline Obj* cls(const char* n) { Obj* o = mk("java/lang/Class"); o->s = n; return o; }
[[noreturn]] inline void athrow(Obj* o);
inline Obj* mkex(const char* c, const std::string& msg) { Obj* o = mk(c); o->f["message"].a = nstr(msg); return o; }
[[noreturn]] inline void athrow(Obj* o) { if (!o) { o = mkex("java/lang/NullPointerException", ""); } throw Thrown{o}; }
[[noreturn]] inline void throwNew(const char* c, const std::string& m) { athrow(mkex(c, m)); }
inline bool subOf(ClassInfo* c, const std::string& n, int d = 0) {
    if (!c || d > 40) return false; if (c->name == "$lambda") return true; if (c->name == n) return true; if (n == "java/lang/Object") return true;
    for (auto& i : c->ifs) if (subOf(ci(i), n, d + 1)) return true;
    return c->sup.empty() ? false : subOf(ci(c->sup), n, d + 1); }
inline int instanceOfName(Obj* o, const char* n) { return o && subOf(o->ci, n); }
inline int instanceOf(Obj* o, const char* n) { return instanceOfName(o, n); }
inline void checkcast(Obj* o, const char* n) { if (o && !subOf(o->ci, n) && n[0] != '[') throwNew("java/lang/ClassCastException", std::string(o->ci->name) + " cannot be cast to " + n); }
inline void ensureInit(ClassInfo* c) { if (c->inited) return; c->inited = true; if (!c->sup.empty()) ensureInit(ci(c->sup)); if (c->clinit) c->clinit(nullptr); }
inline Obj* newobj(const char* n) { ClassInfo* c = ci(n); ensureInit(c); Obj* o = new Obj(); o->ci = c; return o; }
inline Obj* nn(Obj* o) { if (!o) throwNew("java/lang/NullPointerException", ""); return o; }
// ---- aritmetica (semantica Java)
inline int32_t iadd(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); } inline int32_t isub(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }
inline int32_t imul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
inline int32_t idiv(int32_t a, int32_t b) { if (!b) throwNew("java/lang/ArithmeticException", "/ by zero"); return (a == INT32_MIN && b == -1) ? a : a / b; }
inline int32_t irem(int32_t a, int32_t b) { if (!b) throwNew("java/lang/ArithmeticException", "/ by zero"); return (b == -1) ? 0 : a % b; }
inline int32_t ishl(int32_t a, int32_t b) { return (int32_t)((uint32_t)a << (b & 31)); } inline int32_t ishr(int32_t a, int32_t b) { return a >> (b & 31); }
inline int32_t iushr(int32_t a, int32_t b) { return (int32_t)((uint32_t)a >> (b & 31)); }
inline int32_t iand(int32_t a, int32_t b) { return a & b; } inline int32_t ior(int32_t a, int32_t b) { return a | b; } inline int32_t ixor(int32_t a, int32_t b) { return a ^ b; } inline int32_t ineg(int32_t a) { return (int32_t)(0u - (uint32_t)a); }
inline int64_t ladd(int64_t a, int64_t b) { return (int64_t)((uint64_t)a + (uint64_t)b); } inline int64_t lsub(int64_t a, int64_t b) { return (int64_t)((uint64_t)a - (uint64_t)b); }
inline int64_t lmul(int64_t a, int64_t b) { return (int64_t)((uint64_t)a * (uint64_t)b); }
inline int64_t ldiv(int64_t a, int64_t b) { if (!b) throwNew("java/lang/ArithmeticException", "/ by zero"); return (a == INT64_MIN && b == -1) ? a : a / b; }
inline int64_t lrem(int64_t a, int64_t b) { if (!b) throwNew("java/lang/ArithmeticException", "/ by zero"); return (b == -1) ? 0 : a % b; }
inline int64_t lshl(int64_t a, int32_t b) { return (int64_t)((uint64_t)a << (b & 63)); } inline int64_t lshr(int64_t a, int32_t b) { return a >> (b & 63); }
inline int64_t lushr(int64_t a, int32_t b) { return (int64_t)((uint64_t)a >> (b & 63)); }
inline int64_t land(int64_t a, int64_t b) { return a & b; } inline int64_t lor(int64_t a, int64_t b) { return a | b; } inline int64_t lxor(int64_t a, int64_t b) { return a ^ b; } inline int64_t lneg(int64_t a) { return (int64_t)(0ull - (uint64_t)a); }
inline float fadd(float a, float b) { return a + b; } inline float fsub(float a, float b) { return a - b; } inline float fmul(float a, float b) { return a * b; } inline float fdiv(float a, float b) { return a / b; }
inline float frem(float a, float b) { return std::fmod(a, b); } inline float fneg(float a) { return -a; }
inline double dadd(double a, double b) { return a + b; } inline double dsub(double a, double b) { return a - b; } inline double dmul(double a, double b) { return a * b; } inline double ddiv(double a, double b) { return a / b; }
inline double drem(double a, double b) { return std::fmod(a, b); } inline double dneg(double a) { return -a; }
inline int32_t d2i(double d) { if (d != d) return 0; if (d >= 2147483647.0) return INT32_MAX; if (d <= -2147483648.0) return INT32_MIN; return (int32_t)d; }
inline int64_t d2l(double d) { if (d != d) return 0; if (d >= 9.223372036854775807e18) return INT64_MAX; if (d <= -9.223372036854775808e18) return INT64_MIN; return (int64_t)d; }
inline int32_t f2i(float f) { return d2i(f); } inline int64_t f2l(float f) { return d2l(f); }
inline int32_t lcmp(int64_t a, int64_t b) { return (a > b) - (a < b); }
template <class T> inline int32_t cmp(T a, T b, int nan) { if (a != a || b != b) return nan; return (a > b) - (a < b); }
// ---- campos, arrays, chamadas
inline ClassInfo* statOwner(const FRef& f) { ClassInfo* c = ci(f.owner); for (ClassInfo* k = c; k;) { ensureInit(k); if (k->statics.count(f.name)) return k; k = k->sup.empty() ? nullptr : ci(k->sup); } return c; }
inline V getstatic(const FRef& f);
inline void putstatic(const FRef& f, V v) { ClassInfo* c = ci(f.owner); ensureInit(c); ClassInfo* k = statOwner(f); k->statics[f.name] = v; }
inline V getfield(Obj* o, const FRef& f) { return nn(o)->f[f.name]; }
inline void putfield(Obj* o, const FRef& f, V v) { nn(o)->f[f.name] = v; }
inline V aget(Obj* a, int32_t i) { nn(a); if (i < 0 || i >= (int32_t)a->arr.size()) throwNew("java/lang/ArrayIndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(a->arr.size())); return a->arr[i]; }
inline void aset(Obj* a, int32_t i, V v) { nn(a); if (i < 0 || i >= (int32_t)a->arr.size()) throwNew("java/lang/ArrayIndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(a->arr.size())); a->arr[i] = v; }
inline int32_t alen(Obj* a) { return (int32_t)nn(a)->arr.size(); }
inline Obj* newarray(int32_t n, int t) { if (n < 0) throwNew("java/lang/NegativeArraySizeException", std::to_string(n)); Obj* o = mk("[array"); o->arr.assign(n, V()); o->at = (char)t; return o; }
inline Obj* multianew(const int32_t* d, int n) { Obj* o = newarray(d[0], 0); if (n > 1) for (auto& e : o->arr) e.a = multianew(d + 1, n - 1); return o; }
inline const MDef* findM(ClassInfo* c, const std::string& n, const std::string& d, int depth = 0) {
    if (!c || depth > 40) return nullptr;
    for (auto& m : c->m) if (n == m.name && d == m.desc) return &m;
    if (!c->sup.empty()) if (auto r = findM(ci(c->sup), n, d, depth + 1)) return r;
    for (auto& i : c->ifs) if (auto r = findM(ci(i), n, d, depth + 1)) return r;
    return nullptr; }
inline const Nat* findN(ClassInfo* c, const std::string& n, const std::string& d, int depth = 0) {
    if (!c || depth > 40) return nullptr;
    auto it = natives().find(c->name + "." + n + d); if (it != natives().end()) return &it->second;
    if (!c->sup.empty()) if (auto r = findN(ci(c->sup), n, d, depth + 1)) return r;
    for (auto& i : c->ifs) if (auto r = findN(ci(i), n, d, depth + 1)) return r;
    auto o = natives().find("java/lang/Object." + n + d); return o != natives().end() ? &o->second : nullptr; }
inline V va0(Obj* x) { V v; v.a = x; return v; }
inline V dispatch(ClassInfo* c, const MRef& m, V* a, int n) {
    if (auto f = findM(c, m.name, m.desc)) { if (f->fn) return f->fn(a); }
    if (auto nt = findN(c, m.name, m.desc)) return (*nt)(a);
    std::string k = std::string(m.owner) + "." + m.name + m.desc; auto& ms = missing(); bool seen = false; for (auto& s : ms) if (s == k) { seen = true; break; }
    if (!seen) ms.push_back(k); return V(); }
inline V callLambda(Obj* f, V* a, int n);
inline V invokevirtual(const MRef& m, V* a, int n) { Obj* o = nn(a[0].a); if (o->lm && strcmp(m.name, "equals") && strcmp(m.name, "hashCode") && strcmp(m.name, "toString")) return callLambda(o, a + 1, n - 1); return dispatch(o->ci, m, a, n); }
inline V invokespecial(const MRef& m, V* a, int n) { nn(a[0].a); return dispatch(ci(m.owner), m, a, n); }
// patchJVM.12: @SidedProxy (a anotacao nao e lida): campo estatico nulo cujo tipo termina em "Proxy" vira uma instancia da classe declarada
inline V getstatic(const FRef& f) { V& v = statOwner(f)->statics[f.name]; const size_t L = strlen(f.desc);
    if (!v.a && L > 8 && f.desc[0] == 'L' && !strcmp(f.desc + L - 7, "Proxy;")) { std::string cn(f.desc + 1, L - 2); ClassInfo* c = ci(cn.c_str()); if (c && !c->m.empty()) { Obj* o = newobj(cn.c_str()); if (const MDef* k = findM(c, "<init>", "()V")) { V b[1]; b[0].a = o; k->fn(b); } v.a = o; } }
    if (!v.a && !strncmp(f.owner, "net/minecraft/", 14) && !strncmp(f.desc, "Lnet/minecraft/", 15) && L > 17) { std::string cn(f.desc + 1, L - 2); Obj* o = mk(cn.c_str()); o->f["name"].a = str(f.name); v.a = o; }   // patchJVM.12: campo estatico vanilla (Material/Blocks/Items/SoundEvents...) sem valor: objeto marcador do tipo declarado (nao verificado: sem semantica propria)
    return v; }
inline V invokestatic(const MRef& m, V* a, int n) { ClassInfo* c = ci(m.owner); ensureInit(c); return dispatch(c, m, a, n); }
inline V invokedynamic(const MRef& m, V* a, int n) { if (!m.in) unsupported(m.name); Obj* o = mk("$lambda"); o->arr.assign(a, a + n); o->lm = &m; return va0(o); }
// ---- lambdas (invokedynamic/LambdaMetafactory): objeto "$lambda" = (impl, capturados); adapta boxing/unboxing entre a assinatura erasada e a do metodo
inline std::vector<char> ptys(const char* d, char* ret) { std::vector<char> r; const char* p = d + 1; while (*p != ')') { char c = *p; r.push_back(c == '[' ? 'L' : c); while (*p == '[') p++; if (*p == 'L') p = strchr(p, ';') + 1; else p++; } *ret = p[1] == '[' ? 'L' : p[1]; return r; }
inline V box1(char t, V v);
inline V callLambda(Obj* f, V* a, int n) {
    const MRef& m = *f->lm; std::vector<V> args(f->arr); char ir, sr, dr; auto ip = ptys(m.id, &ir); auto sp = ptys(m.inst, &sr); std::vector<char> imp_args = ip;
    if (m.ik != 6 && m.ik != 8) imp_args.insert(imp_args.begin(), 'L');   // receptor
    for (int k = 0; k < n; k++) args.push_back(a[k]);
    for (size_t k = f->arr.size(); k < args.size() && k < imp_args.size(); k++) { char want = imp_args[k]; size_t sk = k - f->arr.size(); char have = sk < sp.size() ? sp[sk] : 'L';
        if (want != 'L' && have == 'L' && args[k].a) { Obj* b = args[k].a; if (b->f.count("value")) args[k] = b->f["value"]; } }
    V r; MRef im{m.io, m.in, m.id}; int na = (int)args.size();
    if (m.ik == 6) r = invokestatic(im, args.data(), na); else if (m.ik == 7) r = invokespecial(im, args.data(), na);
    else if (m.ik == 8) { Obj* o = newobj(m.io); std::vector<V> b(1); b[0].a = o; b.insert(b.end(), args.begin(), args.end()); invokespecial(im, b.data(), na + 1); r.a = o; ir = 'L'; }
    else r = invokevirtual(im, args.data(), na);
    if (ir != 'V' && ir != 'L' && sr == 'L') return box1(ir, r); return r; }
// ---- JDK basico
inline std::string fmtD(double v, bool isF) {
    if (v != v) return "NaN"; if (std::isinf(v)) return v < 0 ? "-Infinity" : "Infinity"; if (v == 0) return std::signbit(v) ? "-0.0" : "0.0";
    char b[64]; auto r = isF ? std::to_chars(b, b + 64, (float)v, std::chars_format::scientific) : std::to_chars(b, b + 64, v, std::chars_format::scientific); std::string s(b, r.ptr);
    bool neg = s[0] == '-'; if (neg) s = s.substr(1); size_t e = s.find('e'); int ex = atoi(s.c_str() + e + 1); std::string dg; for (size_t k = 0; k < e; k++) if (s[k] != '.') dg += s[k];
    std::string o; double av = std::fabs(v);
    if (av >= 1e-3 && av < 1e7) {
        if (ex >= 0) { std::string ip = dg.substr(0, std::min<size_t>(dg.size(), ex + 1)); while ((int)ip.size() < ex + 1) ip += '0'; std::string fp = (int)dg.size() > ex + 1 ? dg.substr(ex + 1) : "0"; o = ip + "." + fp; }
        else o = "0." + std::string(-ex - 1, '0') + dg;
    } else o = dg.substr(0, 1) + "." + (dg.size() > 1 ? dg.substr(1) : "0") + "E" + std::to_string(ex);
    return neg ? "-" + o : o; }
inline std::string toStr(Obj* o);
inline std::string str_of(Obj* o) { return nn(o)->s; }
inline int32_t jhash(const std::string& s) { uint32_t h = 0; for (unsigned char c : s) h = 31 * h + c; return (int32_t)h; }
inline std::string toStr(Obj* o) {
    if (!o) return "null"; const std::string& n = o->ci->name;
    if (n == "java/lang/String") return o->s;
    if (n == "java/lang/Integer") return std::to_string(o->f["value"].i); if (n == "java/lang/Long") return std::to_string(o->f["value"].j);
    if (n == "java/lang/Double") return fmtD(o->f["value"].d, false); if (n == "java/lang/Float") return fmtD(o->f["value"].f, true);
    if (n == "java/lang/Boolean") return o->f["value"].i ? "true" : "false"; if (n == "java/lang/StringBuilder") return o->s;
    MRef m{"java/lang/Object", "toString", "()Ljava/lang/String;"}; V a[1]; a[0].a = o; V r = dispatch(o->ci, m, a, 1); return r.a ? r.a->s : "null"; }
inline std::string chr(int32_t c) { std::string s; uint32_t u = (uint32_t)c & 0xffff; if (u < 0x80) s += (char)u; else if (u < 0x800) { s += (char)(0xC0 | u >> 6); s += (char)(0x80 | (u & 63)); } else { s += (char)(0xE0 | u >> 12); s += (char)(0x80 | ((u >> 6) & 63)); s += (char)(0x80 | (u & 63)); } return s; }
inline void N_(const char* k, Nat f) { natives()[k] = f; }
inline V vi(int32_t x) { V v; v.i = x; return v; } inline V va(Obj* x) { V v; v.a = x; return v; } inline V vj(int64_t x) { V v; v.j = x; return v; } inline V vd(double x) { V v; v.d = x; return v; }
inline Obj* box(const char* c, V v) { Obj* o = mk(c); o->f["value"] = v; return o; }
inline V box1(char t, V v) { V r; r.a = box(t == 'J' ? "java/lang/Long" : t == 'D' ? "java/lang/Double" : t == 'F' ? "java/lang/Float" : t == 'Z' ? "java/lang/Boolean" : "java/lang/Integer", v); return r; }
inline void initColl();
struct JdkInit { JdkInit() { initColl();
    for (auto p : {"Throwable:Object", "Exception:Throwable", "Error:Throwable", "RuntimeException:Exception", "ArithmeticException:RuntimeException", "NullPointerException:RuntimeException", "IllegalArgumentException:RuntimeException", "IllegalStateException:RuntimeException",
                   "IndexOutOfBoundsException:RuntimeException", "ArrayIndexOutOfBoundsException:IndexOutOfBoundsException", "ClassCastException:RuntimeException", "NegativeArraySizeException:RuntimeException", "UnsupportedOperationException:RuntimeException", "StringBuilder:Object", "String:Object", "Integer:Object", "Long:Object", "Double:Object", "Float:Object", "Boolean:Object"}) {
        std::string s = p; size_t c = s.find(':'); declare(("java/lang/" + s.substr(0, c)).c_str(), ("java/lang/" + s.substr(c + 1)).c_str()); }
    ci("java/lang/Object")->sup = ""; ci("java/io/PrintStream"); ci("java/lang/System");
    N_("java/lang/Object.<init>()V", [](V*) { return V(); }); N_("java/lang/Object.hashCode()I", [](V* a) { return vi((int32_t)(((uintptr_t)a[0].a) >> 4)); });
    N_("java/lang/Object.toString()Ljava/lang/String;", [](V* a) { char b[96]; snprintf(b, 96, "%s@%x", a[0].a->ci->name.c_str(), (unsigned)(((uintptr_t)a[0].a) >> 4)); std::string s = b; for (auto& c : s) if (c == '/') c = '.'; return va(nstr(s)); });
    N_("java/lang/Object.equals(Ljava/lang/Object;)Z", [](V* a) { return vi(a[0].a == a[1].a); });
    N_("java/lang/Throwable.<init>()V", [](V*) { return V(); }); N_("java/lang/Throwable.<init>(Ljava/lang/String;)V", [](V* a) { a[0].a->f["message"] = a[1]; return V(); });
    N_("java/lang/Throwable.getMessage()Ljava/lang/String;", [](V* a) { return a[0].a->f["message"]; });
    N_("java/lang/Throwable.toString()Ljava/lang/String;", [](V* a) { std::string n = a[0].a->ci->name; for (auto& c : n) if (c == '/') c = '.'; Obj* m = a[0].a->f["message"].a; return va(nstr(m ? n + ": " + m->s : n)); });
    auto sys = ci("java/lang/System"); (void)sys; ci("java/lang/System")->inited = true; Obj* out = mk("java/io/PrintStream"); out->f["fd"].i = 1; ci("java/lang/System")->statics["out"] = va(out);
    Obj* err = mk("java/io/PrintStream"); err->f["fd"].i = 2; ci("java/lang/System")->statics["err"] = va(err);
    auto pr = [](const char* d, std::function<std::string(V*)> f, bool nl) { N_((std::string("java/io/PrintStream.") + (nl ? "println" : "print") + d).c_str(), [f, nl](V* a) { std::string s = f(a + 1) + (nl ? "\n" : ""); fwrite(s.data(), 1, s.size(), a[0].a->f["fd"].i == 2 ? stderr : stdout); return V(); }); };
    for (int nl = 0; nl < 2; nl++) {
        pr("(I)V", [](V* a) { return std::to_string(a[0].i); }, nl); pr("(J)V", [](V* a) { return std::to_string(a[0].j); }, nl); pr("(D)V", [](V* a) { return fmtD(a[0].d, false); }, nl);
        pr("(F)V", [](V* a) { return fmtD(a[0].f, true); }, nl); pr("(Z)V", [](V* a) { return std::string(a[0].i ? "true" : "false"); }, nl); pr("(C)V", [](V* a) { return chr(a[0].i); }, nl);
        pr("(Ljava/lang/String;)V", [](V* a) { return toStr(a[0].a); }, nl); pr("(Ljava/lang/Object;)V", [](V* a) { return toStr(a[0].a); }, nl); }
    N_("java/io/PrintStream.println()V", [](V* a) { fputc('\n', stdout); return V(); });
    N_("java/lang/StringBuilder.<init>()V", [](V* a) { return V(); }); N_("java/lang/StringBuilder.<init>(Ljava/lang/String;)V", [](V* a) { a[0].a->s = toStr(a[1].a); return V(); });
    auto ap = [](const char* d, std::function<std::string(V*)> f) { N_((std::string("java/lang/StringBuilder.append") + d).c_str(), [f](V* a) { a[0].a->s += f(a + 1); return a[0]; }); };
    ap("(I)Ljava/lang/StringBuilder;", [](V* a) { return std::to_string(a[0].i); }); ap("(J)Ljava/lang/StringBuilder;", [](V* a) { return std::to_string(a[0].j); }); ap("(D)Ljava/lang/StringBuilder;", [](V* a) { return fmtD(a[0].d, false); });
    ap("(F)Ljava/lang/StringBuilder;", [](V* a) { return fmtD(a[0].f, true); }); ap("(Z)Ljava/lang/StringBuilder;", [](V* a) { return std::string(a[0].i ? "true" : "false"); }); ap("(C)Ljava/lang/StringBuilder;", [](V* a) { return chr(a[0].i); });
    ap("(Ljava/lang/String;)Ljava/lang/StringBuilder;", [](V* a) { return toStr(a[0].a); }); ap("(Ljava/lang/Object;)Ljava/lang/StringBuilder;", [](V* a) { return toStr(a[0].a); });
    N_("java/lang/StringBuilder.toString()Ljava/lang/String;", [](V* a) { return va(nstr(a[0].a->s)); }); N_("java/lang/StringBuilder.length()I", [](V* a) { return vi((int32_t)a[0].a->s.size()); });
    N_("java/lang/String.length()I", [](V* a) { return vi((int32_t)a[0].a->s.size()); });
    N_("java/lang/String.charAt(I)C", [](V* a) { auto& s = a[0].a->s; int i = a[1].i; if (i < 0 || i >= (int)s.size()) throwNew("java/lang/StringIndexOutOfBoundsException", "Index " + std::to_string(i) + " out of bounds for length " + std::to_string(s.size())); return vi((unsigned char)s[i]); });
    N_("java/lang/String.substring(I)Ljava/lang/String;", [](V* a) { auto& s = a[0].a->s; int i = a[1].i; if (i < 0 || i > (int)s.size()) throwNew("java/lang/StringIndexOutOfBoundsException", "begin " + std::to_string(i) + ", end " + std::to_string(s.size()) + ", length " + std::to_string(s.size())); return va(nstr(s.substr(i))); });
    N_("java/lang/String.substring(II)Ljava/lang/String;", [](V* a) { auto& s = a[0].a->s; int i = a[1].i, j = a[2].i; if (i < 0 || j > (int)s.size() || i > j) throwNew("java/lang/StringIndexOutOfBoundsException", "begin " + std::to_string(i) + ", end " + std::to_string(j) + ", length " + std::to_string(s.size())); return va(nstr(s.substr(i, j - i))); });
    N_("java/lang/String.indexOf(Ljava/lang/String;)I", [](V* a) { size_t p = a[0].a->s.find(nn(a[1].a)->s); return vi(p == std::string::npos ? -1 : (int32_t)p); });
    N_("java/lang/String.toUpperCase()Ljava/lang/String;", [](V* a) { std::string s = a[0].a->s; for (auto& c : s) if (c >= 'a' && c <= 'z') c -= 32; return va(nstr(s)); });
    N_("java/lang/String.toLowerCase()Ljava/lang/String;", [](V* a) { std::string s = a[0].a->s; for (auto& c : s) if (c >= 'A' && c <= 'Z') c += 32; return va(nstr(s)); });
    N_("java/lang/String.equals(Ljava/lang/Object;)Z", [](V* a) { return vi(a[1].a && a[1].a->ci == a[0].a->ci && a[1].a->s == a[0].a->s); });
    N_("java/lang/String.hashCode()I", [](V* a) { return vi(jhash(a[0].a->s)); }); N_("java/lang/String.toString()Ljava/lang/String;", [](V* a) { return a[0]; });
    N_("java/lang/String.valueOf(I)Ljava/lang/String;", [](V* a) { return va(nstr(std::to_string(a[0].i))); }); N_("java/lang/String.isEmpty()Z", [](V* a) { return vi(a[0].a->s.empty()); });
    N_("java/lang/Math.sqrt(D)D", [](V* a) { return vd(std::sqrt(a[0].d)); }); N_("java/lang/Math.abs(D)D", [](V* a) { return vd(std::fabs(a[0].d)); }); N_("java/lang/Math.abs(I)I", [](V* a) { return vi(a[0].i < 0 ? -a[0].i : a[0].i); });
    N_("java/lang/Math.max(II)I", [](V* a) { return vi(std::max(a[0].i, a[1].i)); }); N_("java/lang/Math.min(II)I", [](V* a) { return vi(std::min(a[0].i, a[1].i)); });
    N_("java/lang/Math.floor(D)D", [](V* a) { return vd(std::floor(a[0].d)); }); N_("java/lang/Math.sin(D)D", [](V* a) { return vd(std::sin(a[0].d)); }); N_("java/lang/Math.cos(D)D", [](V* a) { return vd(std::cos(a[0].d)); });
    N_("java/lang/Integer.valueOf(I)Ljava/lang/Integer;", [](V* a) { return va(box("java/lang/Integer", a[0])); }); N_("java/lang/Integer.intValue()I", [](V* a) { return a[0].a->f["value"]; });
    N_("java/lang/Long.valueOf(J)Ljava/lang/Long;", [](V* a) { return va(box("java/lang/Long", a[0])); }); N_("java/lang/Long.longValue()J", [](V* a) { return a[0].a->f["value"]; });
    N_("java/lang/Double.valueOf(D)Ljava/lang/Double;", [](V* a) { return va(box("java/lang/Double", a[0])); }); N_("java/lang/Double.doubleValue()D", [](V* a) { return a[0].a->f["value"]; });
    N_("java/lang/Boolean.valueOf(Z)Ljava/lang/Boolean;", [](V* a) { return va(box("java/lang/Boolean", a[0])); }); N_("java/lang/Boolean.booleanValue()Z", [](V* a) { return a[0].a->f["value"]; });
    N_("java/lang/Integer.hashCode()I", [](V* a) { return a[0].a->f["value"]; });
    N_("java/lang/Integer.equals(Ljava/lang/Object;)Z", [](V* a) { return vi(a[1].a && a[1].a->ci == a[0].a->ci && a[1].a->f["value"].i == a[0].a->f["value"].i); });
} };
inline JdkInit& jdk() { static JdkInit j; return j; }
inline int runMain(const char* cn) {
    jdk(); ClassInfo* c = ci(cn); try { ensureInit(c); const MDef* m = findM(c, "main", "([Ljava/lang/String;)V"); if (!m) { fprintf(stderr, "sem main em %s\n", cn); return 2; } V a[1]; a[0].a = newarray(0, 0); m->fn(a); }
    catch (Thrown& t) { fflush(stdout); fprintf(stderr, "Exception in thread \"main\" %s\n", t.ex ? toStr(t.ex).c_str() : "?"); return 1; }
    for (auto& s : missing()) fprintf(stderr, "ausente: %s\n", s.c_str()); return 0; }
#include "jvmcoll.h"
#include "mcshim.h"
}  // namespace rt
