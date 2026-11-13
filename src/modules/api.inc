// ---------- mini-APIs para modulos (patchMod.1) ----------
// Fachada FINA e estavel sobre o jogo: um modulo novo usa so api::* e nao precisa ler world.inc/chat_debug.inc/etc.
// Nada aqui muda o comportamento do jogo: so chama funcoes que ja existem. Ver src/modules/README.md.
//   api::blk   blocos        api::chat  mensagens        api::hud   desenho 2D        api::item  itens
//   api::mob   mobs          api::arg   argumentos de /comando   api::Cmd  tabela de comandos   api::Cfg  config propria
namespace api {
    // ---- blocos (multiplayer sincronizado) ----
    namespace blk {
        static inline int get(int x, int y, int z)  { return getB(x, y, z); }
        static inline int meta(int x, int y, int z) { return getM(x, y, z); }
        static inline void set(int x, int y, int z, int id, int meta = 0) { setBM(x, y, z, id, meta); netSendBlock(x, y, z, id, meta); }   // local + rede
        static inline void setLocal(int x, int y, int z, int id, int meta = 0) { setBM(x, y, z, id, meta); }                              // so neste cliente
        static inline bool isAir(int x, int y, int z) { return getB(x, y, z) == AIR; }
    }
    // ---- chat ----
    namespace chat {
        static inline void say(const std::string& s, unsigned rgb = 0xFFFFFF) { chatAdd(s, rgb); }
        static inline void ok(const std::string& s)  { chatAdd(s, 0x55FF55); }
        static inline void err(const std::string& s) { chatAdd(s, 0xFF5555); }
    }
    // ---- HUD 2D (chame hud::begin/end dentro de onDrawHud) ----
    namespace hud {
        static inline void begin(int w, int h) { uiOrtho(w, h); rdDisable(GL_TEXTURE_2D); }
        static inline void end() { uiEnd(); }
        static inline void rect(float x, float y, float w, float h, unsigned rgb, float a = 1.f) { flat(x, y, w, h, ((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, a); }
        static inline void text(float x, float y, float sc, const std::string& s, unsigned rgb = 0xFFFFFF) {
            rdEnable(GL_TEXTURE_2D); ::text(x, y, sc, s.c_str(), ((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, sc); rdDisable(GL_TEXTURE_2D); }
        static inline float width(const std::string& s, float sc) { return textWidth(s.c_str(), sc); }
    }
    // ---- itens ----
    namespace item {
        static inline const char* name(int id) { return itemName(id); }
        static inline int give(int id, int n) { return addItem(id, n); }   // devolve o que NAO coube
    }
    // ---- mobs ----
    namespace mob {
        static inline void spawn(int type, double x, double y, double z, float yaw = 0.f) { spawnMob(type, x, y, z, yaw); }   // type = M_* (mobs.inc)
    }
    // ---- argumentos de /comando (puros, sem estado) ----
    namespace arg {
        static inline std::string str(const std::vector<std::string>& a, size_t i, const std::string& def = "") { return i < a.size() ? a[i] : def; }
        static inline int num(const std::vector<std::string>& a, size_t i, int def = 0) { if (i >= a.size()) return def; char* e = nullptr; const long v = strtol(a[i].c_str(), &e, 10); return e && e != a[i].c_str() && !*e ? (int)v : def; }
        static inline double flt(const std::vector<std::string>& a, size_t i, double def = 0) { if (i >= a.size()) return def; char* e = nullptr; const double v = strtod(a[i].c_str(), &e); return e && e != a[i].c_str() && !*e ? v : def; }
        static inline bool has(const std::vector<std::string>& a, size_t n) { return a.size() >= n; }
    }
    // ---- tabela de comandos: um modulo com varios /comandos vira uma lista ----
    //   static const api::Cmd CMDS[] = { {"meu_a", "/meu_a <n>", [](const std::vector<std::string>& a, Phys& P) { ...; return true; }}, ... };
    //   onCommand: return api::dispatch(CMDS, sizeof CMDS / sizeof *CMDS, cmd, args, P);   (retorna false se o nome nao for do modulo)
    struct Cmd { const char* name; const char* usage; bool (*fn)(const std::vector<std::string>& args, Phys& P); };   // fn devolve false = uso errado (mostra usage)
    static inline bool dispatch(const Cmd* t, size_t n, const std::string& cmd, const std::vector<std::string>& args, Phys& P) {
        for (size_t i = 0; i < n; ++i) if (cmd == t[i].name) { if (!t[i].fn(args, P)) chat::err(std::string("Uso: ") + t[i].usage); return true; }
        return false;
    }
    // ---- config propria do modulo: arquivo modulo_<nome>.cfg (chave=valor), sem colidir com options.cfg ----
    struct Cfg {
        std::string path; std::vector<std::pair<std::string, std::string>> kv;
        explicit Cfg(const std::string& modName) : path("modulo_" + modName + ".cfg") { load(); }
        void load() { kv.clear(); FILE* f = fopen(path.c_str(), "rb"); if (!f) return; char ln[512];
            while (fgets(ln, sizeof ln, f)) { std::string s(ln); while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back(); const size_t q = s.find('='); if (q != std::string::npos && q) kv.push_back({s.substr(0, q), s.substr(q + 1)}); }
            fclose(f); }
        bool save() const { FILE* f = fopen(path.c_str(), "wb"); if (!f) return false; for (auto& p : kv) fprintf(f, "%s=%s\n", p.first.c_str(), p.second.c_str()); fclose(f); return true; }
        std::string get(const std::string& k, const std::string& def = "") const { for (auto& p : kv) if (p.first == k) return p.second; return def; }
        int getInt(const std::string& k, int def = 0) const { const std::string v = get(k); if (v.empty()) return def; char* e = nullptr; const long r = strtol(v.c_str(), &e, 10); return e && e != v.c_str() ? (int)r : def; }
        void set(const std::string& k, const std::string& v) { for (auto& p : kv) if (p.first == k) { p.second = v; return; } kv.push_back({k, v}); }
        void setInt(const std::string& k, int v) { set(k, std::to_string(v)); }
    };
}
