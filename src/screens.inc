// ================= telas do Minecraft 1.8.9 (Opções, Vídeo OptiFine, Controles, Idioma, Multijogador, Criar mundo, Mods...) =================
// Tudo guiado por dados: pageBuild() monta a lista de widgets de cada tela (usada tanto no desenho quanto nos eventos).
// Botões alternam valores/abrem outras telas; nada altera o jogo (placeholders visuais). Coordenadas em unidades da GUI (como o Java).
enum { WK_BTN, WK_SLD, WK_TXT, WK_FLD, WK_LST };
struct Wd { int k = 0, x = 0, y = 0, w = 0, h = 20, fmt = 0, n = 0; float def = 0, a = 0, b = 1; unsigned col = 0xFFFFFF; bool dis = false; std::string id, label, go; std::vector<std::string> opt; };
static std::string gPage, gFocus, gDrag; static std::vector<std::string> gPStack;
static std::map<std::string, int> gSt; static std::map<std::string, float> gSl; static std::map<std::string, std::string> gTx;
static int gsi(const std::string& k, int d) { auto i = gSt.find(k); return i == gSt.end() ? d : i->second; }
static float gsf(const std::string& k, float d) { auto i = gSl.find(k); return i == gSl.end() ? d : i->second; }
// ---- distancia de renderizacao (slider "v1": 2..32 chunks, padrao 7) + persistencia em options.cfg ----
static const float RD_DEF = 5.f / 30.f;   // (7-2)/30
static inline int rdChunks() { return (int)floorf(2 + 30 * gsf("v1", RD_DEF) + .5f); }
static inline float fogEndD() { return std::max(24.f, RAD * 16.f - 4.f); }   // RAD 7 => 108 (valor original)
static inline float fogStartD() { return fogEndD() * .35f; }                  // RAD 7 => ~38
#ifdef __ANDROID__
#define BL_GFX_DEF 1   // celular: OpenGL e o padrao
#else
#define BL_GFX_DEF 0
#endif
static SDL_Window* gBlWin = nullptr;   // janela principal (usada para medir o drawable real)
// Viewport em PIXELS REAIS do drawable. No celular o SurfaceView e reduzido (renderwidth) e SDL_GetWindowSize pode divergir do drawable:
// usar a janela como viewport deixava o jogo em ~1/4 da tela com o resto cinza. w,h continuam sendo o tamanho logico (UI/toque).
#if defined(__ANDROID__) && !defined(BL_VULKAN)
static bool blEglSize(int& w, int& h) {   // tamanho REAL do buffer EGL atual (o SDL informa o tamanho da janela, que pode divergir do buffer)
    typedef void* (*FGS)(int); typedef void* (*FGD)(); typedef unsigned (*FQS)(void*, void*, int, int*);
    static FGS gs = nullptr; static FGD gd = nullptr; static FQS qs = nullptr; static bool init = false;
    if (!init) { init = true; if (void* hl = dlopen("libEGL.so", RTLD_NOW)) { gs = (FGS)dlsym(hl, "eglGetCurrentSurface"); gd = (FGD)dlsym(hl, "eglGetCurrentDisplay"); qs = (FQS)dlsym(hl, "eglQuerySurface"); } }
    if (!gs || !gd || !qs) return false;
    void* d = gd(); void* sf = gs(0x3059); int a = 0, b = 0;   // EGL_DRAW
    if (!d || !sf || !qs(d, sf, 0x3057, &a) || !qs(d, sf, 0x3056, &b) || a <= 0 || b <= 0) return false;
    w = a; h = b; return true;
}
#endif
static void blViewport(int w, int h) {
#if defined(__ANDROID__) && !defined(BL_VULKAN)
    int ew = 0, eh = 0;
    if (blEglSize(ew, eh)) {
        static int lw = 0, lh = 0;
        if (ew != lw || eh != lh) {   // diagnostico: aparece no log sempre que o tamanho muda
            lw = ew; lh = eh; int dw = 0, dh = 0; if (gBlWin) SDL_GL_GetDrawableSize(gBlWin, &dw, &dh);
            SDL_DisplayMode dm; dm.w = dm.h = 0; SDL_GetCurrentDisplayMode(0, &dm);
            logInfo("Viewport: janela SDL %dx%d, drawable SDL %dx%d, superficie EGL %dx%d, tela %dx%d", w, h, dw, dh, ew, eh, dm.w, dm.h);
        }
        glViewport(0, 0, ew, eh); return;
    }
#endif
    glViewport(0, 0, w, h);
}
// Agua realista, sol realista (god rays/lens flare) e shaders extras: DESLIGADOS no celular (pesados e o foco la e iniciar/rodar rapido)
#define BL_NO_FANCY 0   // celular agora usa o pacote LEVE (agua simples + sombras 1 cascata); reflexo planar, god rays e flare ficam so no desktop
#ifdef __ANDROID__
#define BL_LITE 1
#else
#define BL_LITE 0
#endif
static bool gApiOpenGL() { return gsi("gfxapi", BL_GFX_DEF) == 1; }   // renderizador escolhido (0 = Vulkan, padrao; 1 = OpenGL). Vale no proximo inicio: o BlocklandActivity le "renderer=" do options.cfg
static void optSave() { FILE* f = fopen("options.cfg", "wb"); if (f) { fprintf(f, "renderdist=%d\nrenderer=%s\nrenderer_set=%d\nshadows=%d\n", rdChunks(), gApiOpenGL() ? "opengl" : "vulkan", gsi("gfxset", 0), gsi("shd.mob", 0)); fclose(f); } }
static void optLoad() {
    FILE* f = fopen("options.cfg", "rb"); if (!f) return; char ln[128];
    while (fgets(ln, sizeof ln, f)) { char rn[16]; if (sscanf(ln, "renderer=%15s", rn) == 1) gSt["gfxapi"] = !strcmp(rn, "opengl") ? 1 : 0;
        int rs; if (sscanf(ln, "renderer_set=%d", &rs) == 1) gSt["gfxset"] = rs;
        int sm; if (sscanf(ln, "shadows=%d", &sm) == 1) gSt["shd.mob"] = sm;
        int v; if (sscanf(ln, "renderdist=%d", &v) == 1) gSl["v1"] = (std::max(2, std::min(32, v)) - 2) / 30.f; }
    fclose(f);
}
static std::vector<std::string> splitBar(const std::string& s) {
    std::vector<std::string> r; if (s.empty()) return r; size_t p = 0;
    for (;;) { const size_t q = s.find('|', p); if (q == std::string::npos) { r.push_back(s.substr(p)); break; } r.push_back(s.substr(p, q - p)); p = q + 1; }
    return r;
}
struct PB {
    std::vector<Wd>& v;
    void t(int x, int y, const std::string& s, unsigned c = 0xFFFFFF, int al = 1) { Wd d; d.k = WK_TXT; d.x = x; d.y = y; d.label = s; d.col = c; d.fmt = al; v.push_back(d); }
    void b(int x, int y, int w, const std::string& id, const std::string& lb, const std::string& go = "", const std::string& op = "", int def = 0, bool dis = false) {
        Wd d; d.k = WK_BTN; d.x = x; d.y = y; d.w = w; d.id = id; d.label = lb; d.go = go; d.opt = splitBar(op); d.def = (float)def; d.dis = dis; v.push_back(d);
    }
    void s(int x, int y, int w, const std::string& id, const std::string& lb, float def, int fmt = 0, const std::string& op = "", float a = 0, float bb = 1) {
        Wd d; d.k = WK_SLD; d.x = x; d.y = y; d.w = w; d.id = id; d.label = lb; d.def = def; d.fmt = fmt; d.opt = splitBar(op); d.a = a; d.b = bb; v.push_back(d);
    }
    void f(int x, int y, int w, const std::string& id) { Wd d; d.k = WK_FLD; d.x = x; d.y = y; d.w = w; d.id = id; v.push_back(d); }
    void l(int x, int y, int w, int h, const std::string& id, int style, int n, int bx) { Wd d; d.k = WK_LST; d.x = x; d.y = y; d.w = w; d.h = h; d.id = id; d.fmt = style; d.n = n; d.a = (float)bx; v.push_back(d); }
    void done(int x, int y, int w) { b(x, y, w, "done", "Concluído", "<"); }
    void tog(int x, int y, int w, const std::string& id, const std::string& lb, int on) { b(x, y, w, id, lb, "", "OFF|ON", on); }
};
// ---------------- dados ----------------
static const char* const LANGS[] = {"Afrikaans (Suid-Afrika)", "Asturianu (Asturies)", "Català (Catalunya)", "Cymraeg (Cymru)", "Dansk (Danmark)", "Deutsch (Deutschland)", "Eesti (Eesti)", "English (US)", "English (UK)", "Español (Argentina)", "Español (España)", "Español (México)", "Euskara (Euskadi)", "Français (France)", "Galego (Galicia)", "Hrvatski (Hrvatska)", "Íslenska (Ísland)", "Italiano (Italia)", "Magyar (Magyarország)", "Nederlands (Nederland)", "Norsk (Norge)", "Polski (Polska)", "Português (Brasil)", "Português (Portugal)", "Română (România)", "Slovenčina (Slovensko)", "Suomi (Suomi)", "Svenska (Sverige)", "Türkçe (Türkiye)"};
static const char* const SHD[] = {"Não", "(interno)", "EclipseLite_v1.2.zip", "PotatoLite_v3_Final.zip", "RedHat_v34.3.1.zip", "TrueLightFX Lite v5.6.3.zip", "VTXS_PotatoWay-V1.0.0.zip"};
static const char* const CTRL[] = {"#EntityCulling", "key.entityculling.toggle|Botão 100", "#Inventário", "Inventário|E", "Slot 1 da Barra Rápida|1", "Slot 2 da Barra Rápida|2", "Slot 3 da Barra Rápida|3", "Slot 4 da Barra Rápida|4", "Slot 5 da Barra Rápida|5", "Slot 6 da Barra Rápida|6", "Slot 7 da Barra Rápida|7", "Slot 8 da Barra Rápida|8", "Slot 9 da Barra Rápida|9", "Largar Item|Q",
    "#Movimento", "Andar para Frente|W", "Andar para Esquerda|A", "Andar para Trás|S", "Andar para Direita|D", "Pular|Espaço", "Agachar|Shift Esq.", "Correr|Ctrl Esq.",
    "#Jogabilidade", "Atacar/Destruir|Botão 1", "Usar Item/Colocar Bloco|Botão 2", "Escolher Bloco|Botão 3",
    "#Multijogador", "Listar Jogadores|Tab", "Abrir Bate-Papo|T", "Abrir Comando|/",
    "#Diversos", "Tirar Captura de Tela|F2", "Alternar Perspectiva|F5", "Tela Cheia|F11"};
static const char* const FMODS[] = {"Minecraft Coder Pack|9.19", "Forge Mod Loader|8.0.99.99", "Minecraft Forge|11.15.1.2318", "PolyPatcher|1.10.2"};
static void openSingleplayer(); static void txtKbd() {
#ifdef __ANDROID__
    if (gFocus.empty()) SDL_StopTextInput(); else SDL_StartTextInput();   // celular: teclado virtual so quando um campo de texto tem foco
#endif
}
static std::vector<std::string> snpRows() {
    static std::vector<std::string> r;
    if (r.empty()) {
        const char* c = (const char*)glGetString(GL_VERSION); const char* vd = (const char*)glGetString(GL_VENDOR);
        r = {"client_brand|fml,forge", "cpu_cores|" + std::to_string(SDL_GetCPUCount()), "current_action|singleplayer", "display_frequency|59", "display_type|fullscreen", "endianness|little", "fps|30",
             "opengl_version|" + std::string(c ? c : "?"), "opengl_vendor|" + std::string(vd ? vd : "?")};
        const char* caps[] = {"ARB_arrays_of_arrays", "ARB_base_instance", "ARB_blend_func_extended", "ARB_clear_buffer_object", "ARB_color_buffer_float", "ARB_compatibility", "ARB_compressed_texture_pixel_storage", "ARB_compute_shader", "ARB_conservative_depth", "ARB_copy_buffer", "ARB_copy_image", "ARB_depth_buffer_float", "ARB_depth_clamp", "ARB_draw_buffers", "ARB_draw_instanced", "ARB_framebuffer_object", "ARB_geometry_shader4", "ARB_map_buffer_range", "ARB_multitexture", "ARB_shader_objects", "ARB_texture_float", "ARB_vertex_buffer_object"};
        for (const char* k : caps) r.push_back(std::string("gl_caps[") + k + "]|true");
        r.push_back("gl_max_texture_size|16384"); r.push_back("java_version|1.8.0"); r.push_back("os_name|Linux"); r.push_back("window_extent|1366x768");
    }
    return r;
}
static std::vector<std::string> modRows() {
    std::vector<std::string> r(FMODS, FMODS + 4); const int m = gsi("mods.sort", 0);
    if (m == 1) std::sort(r.begin(), r.end()); else if (m == 2) std::sort(r.rbegin(), r.rend());
    const std::string q = gTx["modbusca"]; if (!q.empty()) { std::vector<std::string> o; for (auto& s : r) { std::string a = s, b = q; for (auto& ch : a) ch = (char)tolower(ch); for (auto& ch : b) ch = (char)tolower(ch); if (a.find(b) != std::string::npos) o.push_back(s); } r = o; }
    return r;
}
static std::string lstItem(int st, int i) {
    if (st == 0) return LANGS[i]; if (st == 1) return SHD[i]; if (st == 3) return CTRL[i]; if (st == 4) return snpRows()[i]; if (st == 5) return modRows()[i]; return "";
}
static int lstRow(int st) { static const int r[] = {18, 16, 36, 20, 10, 35, 36}; return r[st]; }
static int lstPad(int st) { static const int r[] = {0, 2, 4, 4, 4, 4, 4}; return r[st]; }
static int lstMax(const Wd& d) { const int tot = d.n * lstRow(d.fmt) + 2 * lstPad(d.fmt) + (d.fmt == 2 ? 30 : 0); return std::max(0, tot - d.h); }
static int lstScr(const Wd& d) { int& s = gSt[d.id + ".scr"]; s = std::max(0, std::min(lstMax(d), s)); return s; }
// ---------------- construção das telas ----------------
static void pageBuild(const std::string& pg, int sw, int sh, std::vector<Wd>& v) {
    const int cx = sw / 2, L = cx - 155, R = cx + 5, h6 = sh / 6; PB p{v};
    auto r = [&](int i) { return h6 - 12 + 21 * i; };
    auto TG = [&](int x, int y, const char* id, const char* lb, int on) { p.tog(x, y, 150, id, lb, on); };
    if (pg == "opcoes") {
        p.t(cx, 15, "Opções");
        p.s(L, h6 - 12, 150, "fov", "Campo de Visão", .5f, 2); TG(R, h6 - 12, "realms", "Realms Notifications", 1);
        p.b(L, h6 + 42, 150, "o1", "Perfil e Skin...", "launcher"); p.b(R, h6 + 42, 150, "o2", "Super Secret Settings...", "?Super Secret Settings");
        p.b(L, h6 + 66, 150, "o3", "Música & Sons...", "musica"); p.b(R, h6 + 66, 150, "o4", "Gamepad...", "gamepad");
        p.b(L, h6 + 90, 150, "o5", "Opções de Vídeo...", "video"); p.b(R, h6 + 90, 150, "o6", "Controles...", "controles");
        p.b(L, h6 + 114, 150, "o7", "Idioma...", "idioma"); p.b(R, h6 + 114, 150, "o8", "Opções do Bate-Papo...", "chat");
        p.b(L, h6 + 138, 150, "o9", "Pacotes de Recursos...", "?Pacotes de Recursos"); p.b(R, h6 + 138, 150, "o10", "Opções do Snooper...", "snooper");
        p.done(cx - 100, h6 + 168, 200);
    } else if (pg == "launcher") {   // mini launcher: previas das skins, nome (+ teclado virtual), skin J1/J2, split screen, Jogar
        const int s = sh >= 290 ? 2 : 1, top = 22, yN = top + 32 * s + 6, yS = yN + 36; const bool first = gPStack.empty();
        p.t(cx, 7, "Blockland", 0xFFFFFF);
        p.t(cx - 100, yN, "Nome do jogador", 0xA0A0A0, 0); p.f(cx - 100, yN + 11, 176, "lname"); p.b(cx + 80, yN + 11, 20, "l.kb", "Aa");
        auto nm = [&](const std::string& f) { std::string n = f.empty() ? "Padrão" : f; if (n.size() > 4 && n.compare(n.size() - 4, 4, ".png") == 0) n.resize(n.size() - 4); if (n.size() > 18) n = n.substr(0, 17) + "…"; return n; };
        p.b(cx - 100, yS, 20, "l.prev", "<"); p.b(cx - 78, yS, 156, "l.skin", "Skin J1: " + (skinIndex() < 0 && !gSkinList.empty() ? std::string("(personalizada)") : nm(gSkinFile))); p.b(cx + 80, yS, 20, "l.next", ">");
        p.b(cx - 100, yS + 24, 20, "l.prev2", "<"); p.b(cx - 78, yS + 24, 156, "l.skin2", "Skin J2: " + nm(gSkin2File)); p.b(cx + 80, yS + 24, 20, "l.next2", ">");
        TG(L, yS + 48, "splitscr", "Split Screen", 0); p.b(R, yS + 48, 150, "splitmode", "Divisão", "", "Topo/Baixo|Lado a Lado", 0);
        p.b(cx - 100, yS + 74, 200, "l.play", first ? "Jogar" : "Concluído");
        if (sh >= 250) p.t(cx, yS + 98, gpCount() ? "Controle detectado: mova o cursor com o analógico, A clica" : "Arraste um PNG 64x64 para a janela, ou ponha em assets/skins", 0x808080);
    } else if (pg == "gamepad") {
        p.t(cx, 6, "Controle (Gamepad)"); p.t(cx, 17, gpCount() ? (std::to_string(gpCount()) + " controle(s) conectado(s)").c_str() : "Nenhum controle conectado", gpCount() ? 0x55FF55 : 0xFF5555);
        for (int i = 0; i < GA_N; i++) p.b(i % 2 ? R : L, h6 - 4 + 22 * (i / 2), 150, "g" + std::to_string(i), std::string(GA_NAME[i]) + ": " + (gGpBind == i ? "> ? <" : GB_NAME[gGpMap[i]]));
        p.b(R, h6 - 4 + 22 * 6, 150, "g.reset", "Padrão"); p.b(L, h6 - 4 + 22 * 7, 150, "gpsens", "Sensibilidade", "", "50%|75%|100%|150%|200%", 2);
        p.t(cx, h6 + 22 * 7 + 22, "Clique numa ação e aperte o botão novo", 0x808080); p.done(cx - 100, sh - 27, 200);
    } else if (pg == "skin") {
        p.t(cx, 20, "Customização da Skin");
        const char* lb[7] = {"Capa", "Casaco", "Manga Esquerda", "Manga Direita", "Calça Esquerda", "Calça Direita", "Chapéu"};
        for (int i = 0; i < 7; i++) TG(i % 2 ? R : L, h6 + 24 * (i / 2), (std::string("sk") + char('0' + i)).c_str(), lb[i], 1);
        p.b(cx - 100, h6 + 96, 200, "sk9", "Capa do OptiFine...", "?Capa do OptiFine"); p.done(cx - 100, h6 + 120, 200);
    } else if (pg == "chat") {
        p.t(cx, 20, "Opções do Bate-Papo...");
        p.b(L, h6, 150, "c0", "Bate-Papo", "", "Visível|Só Comandos|Oculto", 0); TG(R, h6, "c1", "Cores", 1);
        TG(L, h6 + 24, "c2", "Links da Web", 1); p.s(R, h6 + 24, 150, "c3", "Opacidade", 1.f, 0);
        TG(L, h6 + 48, "c4", "Perguntar em Links", 1); p.s(R, h6 + 48, 150, "c5", "Escala", 1.f, 0);
        p.s(L, h6 + 72, 150, "c6", "Altura com Foco", 1.f, 1, "", 20, 180); p.s(R, h6 + 72, 150, "c7", "Altura sem Foco", .4375f, 1, "", 20, 180);
        p.s(L, h6 + 96, 150, "c8", "Largura", 1.f, 1, "", 40, 320); TG(R, h6 + 96, "c9", "Debug Reduzido", 0);
        p.done(cx - 100, h6 + 120, 200);
    } else if (pg == "musica") {
        p.t(cx, 15, "Opções de Música & Sons");
        p.s(L, h6 - 12, 310, "m0", "Volume Principal", 1.f, 0);
        const char* lb[8] = {"Música", "Blocos Musicais", "Clima", "Blocos", "Criaturas Hostis", "Criaturas Amigáveis", "Jogadores", "Ambiente"};
        for (int i = 0; i < 8; i++) p.s(i % 2 ? R : L, h6 + 12 + 24 * (i / 2), 150, std::string("m") + char('1' + i), lb[i], 1.f, 0);
        p.t(cx, h6 + 110, "Sound Device (Click to Change)"); p.b(cx - 100, h6 + 124, 200, "m9", "ALSA Default");
        p.b(cx - 100, h6 + 146, 98, "m10", "All Sounds"); p.b(cx + 2, h6 + 146, 98, "m11", "Refresh Sounds"); p.done(cx - 100, h6 + 168, 200);
    } else if (pg == "twitch") {
        p.t(cx, 81, "Transmissão do Twitch Indisponível");
        p.t(cx, 99, "Não foi possível carregar as bibliotecas necessárias para o", 0xA0A0A0); p.t(cx, 108, "serviço de transmissão integrada do Twitch.", 0xA0A0A0);
        p.b(L, h6 + 164, 150, "done", "Cancelar", "<"); p.b(R, h6 + 164, 150, "tw", "Reportar à Mojang");
    } else if (pg == "video") {
        p.t(cx, 15, "Opções de Vídeo");
        p.b(L, r(0), 150, "v0", "Gráficos", "", "Rápidos|Bonitos|Fantásticos!", 0); p.s(R, r(0), 150, "v1", "Renderização", RD_DEF, 3);
        p.b(L, r(1), 150, "v2", "Iluminação Suave", "", "OFF|Mínima|Máxima", 0); p.s(R, r(1), 150, "v3", "Taxa de FPS Máxima", 1.f, 4);
        p.s(L, r(2), 150, "v4", "Nível de ilum. suave", 1.f, 0); TG(R, r(2), "v5", "Movimentos na Visão", 1);
        p.b(L, r(3), 150, "v6", "Interface", "", "Automática|Pequena|Normal|Grande", 0); TG(R, r(3), "v7", "VBO", 1);
        p.s(L, r(4), 150, "v8", "Brilho", 1.f, 5); TG(R, r(4), "v9", "Blocos Alternativos", 0);
        TG(L, r(5), "v10", "Luzes dinâmicas", 0); TG(R, r(5), "v11", "Visão dinâmica", 1);
        p.b(L, r(6), 150, "v12", "Shaders...", "shaders"); p.b(R, r(6), 150, "v13", "Qualidade...", "qualidade");
        p.b(L, r(7), 150, "v14", "Detalhes...", "detalhes"); p.b(R, r(7), 150, "v15", "Desempenho...", "desempenho");
        p.b(L, r(8), 150, "v16", "Animações...", "animacoes"); p.b(R, r(8), 150, "v17", "Diversas...", "diversas");
        p.done(cx - 100, h6 + 179, 200);
        p.t(2, sh - 10, "OptiFine HD M5 Ultra", 0x808080, 0); p.t(sw - 2, sh - 10, "Minecraft 1.8.9", 0x808080, 2);
    } else if (pg == "shaders") {
        p.t(cx, 15, "Shaders"); p.l(10, 30, 296, 176, "shd", 1, (int)(sizeof(SHD) / sizeof(*SHD)), 0);
        const char* lb[8] = {"Suavização", "Mapa normal", "Mapa especular", "Qualidade", "Sombra", "Profundidade", "Ilum. pela mão", "Ilum. antiga"};
        const char* op[8] = {"OFF|FXAA 2x|FXAA 4x", "ON|OFF", "ON|OFF", "1x|2x|4x", "1x|2x|4x", "1x|2x|4x", "Padrão|Rápida|Bonita", "Padrão|Rápida|Bonita"};
        for (int i = 0; i < 8; i++) p.b(sw - 128, 31 + 20 * i, 118, std::string("sh") + char('0' + i), lb[i], "", op[i], 0, i > 0);
        p.t(5, 216, std::string("OpenGL: ") + (const char*)glGetString(GL_VERSION) + " " + (const char*)glGetString(GL_RENDERER), 0x808080, 0);
        p.b(6, 232, 124, "sh9", "Pasta de shaders"); p.b(131, 232, 20, "sh10", "v"); p.b(164, 232, 147, "done", "Concluído", "<"); p.b(sw - 128, 232, 118, "sh11", "Shaders...", "", "", 0, true);
    } else if (pg == "detalhes") {
        p.t(cx, 15, "Configurações de detalhes");
        p.b(L, r(0), 150, "d0", "Nuvens", "", "Padrão|Rápidos|Bonitos|OFF", 3); p.s(R, r(0), 150, "d1", "Altura das nuvens", 0.f, 0, "OFF|25%|50%|75%|100%");
        p.b(L, r(1), 150, "d2", "Árvores", "", "Padrão|Rápidos|Bonitos", 1); p.b(R, r(1), 150, "d3", "Chuva e neve", "", "Padrão|Rápidos|Bonitos|OFF", 0);
        p.tog(L, r(2), 150, "d4", "Céu", 0); TG(R, r(2), "d5", "Estrelas", 1);
        TG(L, r(3), "d6", "Sol e Lua", 0); TG(R, r(3), "d7", "Mostrar capas", 1);
        p.b(L, r(4), 150, "d8", "Névoa", "", "Rápidos|Bonitos|OFF", 1); p.s(R, r(4), 150, "d9", "Início do nevoeiro", 1.f, 0, "0.2|0.4|0.6|0.8");
        p.b(L, r(5), 150, "d10", "Blocos transl.", "", "Padrão|Rápidos|Bonitos", 0); TG(R, r(5), "d11", "Descrição de itens", 1);
        p.b(L, r(6), 150, "d12", "Itens no chão", "", "Padrão|Rápidos|Bonitos", 0); TG(R, r(6), "d13", "Entity Shadows", 0);
        p.b(L, r(7), 150, "d14", "Vignette", "", "Padrão|Rápidos|Bonitos", 0); TG(R, r(7), "d15", "Cores do pântano", 1);
        TG(L, r(8), "d16", "Integrar biomas", 1); p.done(cx - 100, h6 + 179, 200);
    } else if (pg == "animacoes") {
        p.t(cx, 15, "Configurações de animação");
        const char* a[16] = {"Água animada", "Lava animada", "Fogo animado", "Portal animado", "Redstone animada", "Explosão animada", "Chama animada", "Fumaça animada", "Partículas do vazio", "Partículas de água", "Respingos de chuva", "Partículas de portal", "Partículas de poção", "Gotas de água e lava", "Terreno animado", "Texturas animadas"};
        for (int i = 0; i < 16; i++) TG(i % 2 ? R : L, r(i / 2), (std::string("an.") + std::to_string(i)).c_str(), a[i], 0);
        TG(L, r(8), "an.16", "Fogos de artifício", 0); p.b(R, r(8), 150, "an.p", "Partículas", "", "Todas|Diminuídas|Mínimas", 2);
        p.b(L, h6 + 179, 69, "an.s", "Sim", "=an.:1"); p.b(L + 81, h6 + 179, 69, "an.n", "Não", "=an.:0"); p.done(R, h6 + 179, 150);
    } else if (pg == "qualidade") {
        p.t(cx, 15, "Configurações de qualidade");
        p.s(L, r(0), 150, "q0", "Níveis de Mipmap", 1.f, 0, "OFF|1|2|3|4|Máximo"); p.s(R, r(0), 150, "q1", "Tipo de mipmap", 0.f, 0, "Básico|Média|Boa|Ótima");
        p.s(L, r(1), 150, "q2", "Filtragem anisotrópica", 0.f, 0, "OFF|2|4|8|16"); p.s(R, r(1), 150, "q3", "Suavização", 0.f, 0, "OFF|FSAA 2x|FSAA 4x|FSAA 8x|FSAA 16x");
        TG(L, r(2), "q4", "Água clara", 0); TG(R, r(2), "q5", "Entidades aleatórias", 1);
        TG(L, r(3), "q6", "Grama aprimorada", 0); TG(R, r(3), "q7", "Neve aprimorada", 0);
        TG(L, r(4), "q8", "Fontes personalizadas", 1); TG(R, r(4), "q9", "Cores personalizadas", 1);
        p.b(L, r(5), 150, "q10", "Texturas lig.", "", "Rápidos|Bonitos", 1); TG(R, r(5), "q11", "Texturas naturais", 0);
        TG(L, r(6), "q12", "Céu personalizado", 1); TG(R, r(6), "q13", "Itens personalizados", 1);
        TG(L, r(7), "q14", "Modelos de entidades", 1); TG(R, r(7), "q15", "Interfaces personal.", 1);
        TG(L, r(8), "q16", "Texturas emissivas", 1); p.done(cx - 100, h6 + 179, 200);
    } else if (pg == "desempenho") {
        p.t(cx, 15, "Configurações de desempenho");
        TG(L, r(0), "p0", "FPS estável", 0); TG(R, r(0), "p1", "Mundo estável", 1);
        TG(L, r(1), "p2", "Render. rápida", 0); TG(R, r(1), "p3", "Otimizar cálculos", 0);
        p.b(L, r(2), 150, "p4", "Atualiz. de chunks", "", "1|2|3|4|5", 0); TG(R, r(2), "p5", "Atualiz. dinâmicas", 0);
        TG(L, r(3), "p6", "Render. de regiões", 0); TG(R, r(3), "p7", "Carreg. limitado", 1);
        TG(L, r(4), "p8", "Animações intel.", 0);
#ifdef __ANDROID__
        p.b(R, r(4), 150, "gfxapi", "Renderizador", "", "Vulkan|OpenGL", 0); p.t(cx, r(5) + 6, "Renderizador: reinicie o jogo para aplicar", 0xA0A0A0);
#endif
        p.b(cx - 100, h6 + 179, 200, "done", "Voltar", "<");   // botao Voltar (antes ficava colado no #endif e nunca era compilado: a pagina ficava sem saida)
    } else if (pg == "diversas") {
        p.t(cx, 15, "Outras configurações");
        TG(L, r(0), "x0", "Lagômetro", 0); TG(R, r(0), "x1", "Análise de depuração", 0);
        TG(L, r(1), "x2", "Mostrar FPS", 0); TG(R, r(1), "x3", "Descrições avançadas", 0);
        TG(L, r(2), "x4", "Clima", 1); p.b(R, r(2), 150, "x5", "Tempo", "", "Padrão|Dia|Noite", 0);
        TG(L, r(3), "x6", "Tela Cheia", 1); p.s(R, r(3), 150, "x7", "Fullscreen Mode", 0.f, 0, "Padrão|1366x768|1920x1080");
        TG(L, r(4), "x8", "Anáglifo 3D", 0); p.s(R, r(4), 150, "x9", "Autossalvamento", 1.f, 0, "OFF|10s|20s|30s|1min|2min|3min|6min");
        p.b(L, r(5), 150, "x10", "Captura de tela", "", "Padrão|Rápida", 0); TG(R, r(5), "x11", "Erros da OpenGL", 1);
        p.b(cx - 100, r(7), 200, "x12", "Restaurar configurações gráficas..."); p.done(cx - 100, h6 + 179, 200);
    } else if (pg == "controles") {
        p.t(cx, 8, "Controles");
        TG(L, 19, "k0", "Inverter Mouse", 0); p.s(R, 19, 150, "k1", "Sensibilidade", .5f, 6); TG(L, 43, "k2", "Modo Touchscreen", 0);
        p.l(0, 63, sw, sh - 96, "ctl", 3, (int)(sizeof(CTRL) / sizeof(*CTRL)), cx + 140);
        p.b(L, sh - 28, 150, "done", "Concluído", "<"); p.b(R, sh - 28, 150, "k3", "Redefinir Teclas");
    } else if (pg == "snooper") {
        p.t(cx, 8, "Coleta de Especificações da Máquina");
        const char* ds[4] = {"Nós queremos coletar informações sobre sua máquina para identificar os", "problemas mais comuns e melhorar o Minecraft. Todas essas informações são", "anônimas e podem ser vistas abaixo. Prometemos que não faremos nada de errado", "com esses dados, mas se você quiser, pode optar por desligar essa opção!"};
        for (int i = 0; i < 4; i++) p.t(cx, 21 + i * 9, ds[i], 0xA0A0A0);
        p.l(0, 80, sw, sh - 113, "snp", 4, (int)snpRows().size(), sw - 9);
        TG(L, sh - 29, "s0", "Permitir Snooper", 1); p.b(R, sh - 29, 150, "done", "Concluído", "<");
    } else if (pg == "idioma") {
        p.t(cx, 16, "Idioma..."); p.l(0, 32, sw, 161, "lang", 0, (int)(sizeof(LANGS) / sizeof(*LANGS)), cx + 125);
        p.t(cx, 199, "(As traduções podem não ser 100% corretas)", 0x808080);
        p.b(L, sh - 38, 150, "l1", "Forçar Fonte Unicode", "", "OFF|ON", 0); p.b(R, sh - 38, 150, "done", "Concluído", "<");
    } else if (pg == "multi") {
        gMultiVis = SDL_GetTicks() | 1;   // mantem a varredura de LAN/ZeroTier ativa enquanto esta aba esta aberta (netlan.inc)
        const std::vector<SrvRow> rows = srvRows(); const int sel = gsi("multi.sel", -1); const bool okSel = sel >= 0 && sel < (int)rows.size(), sv = okSel && rows[sel].saved;
        p.t(cx, 20, "Jogar Multiplayer"); p.l(0, 32, sw, 160, "srv", 2, (int)rows.size(), 0);
        if (rows.empty()) { p.t(cx, 85, "Procurando servidores na LAN / ZeroTier...", 0xA0A0A0); p.t(cx, 99, "Seus IPs: " + netAddrSummary(), 0x808080); p.t(cx, 113, "Abra um mundo com \"Hospedar LAN\" em outro aparelho.", 0x808080); p.t(cx, 127, netZtStatus(), 0x55AAAA); }
        p.b(cx - 154, sh - 52, 100, "ms0", "Entrar no Servidor", "", "", 0, !okSel); p.b(cx - 50, sh - 52, 100, "ms1", "Conexão Direta"); p.b(cx + 54, sh - 52, 100, "ms2", "Adicionar servidor");
        p.b(cx - 154, sh - 28, 70, "ms3", "Editar", "", "", 0, !sv); p.b(cx - 73, sh - 28, 70, "ms4", "Excluir", "", "", 0, !sv); p.b(cx + 5, sh - 28, 70, "ms5", "Atualizar"); p.b(cx + 81, sh - 28, 74, "done", "Cancelar", "<");
        p.b(4, 4, 92, "ms6", "Hospedar LAN");
    } else if (pg == "direto") {
        p.t(cx, 20, "Conexão Direta"); p.t(cx - 100, 47, "Endereço do servidor (IP ou IP:porta)", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "dip");
        p.t(cx - 100, 85, "Ex.: 192.168.0.5  |  10.147.17.20:47777 (ZeroTier)", 0x808080, 0);
        p.b(cx - 100, sh / 4 + 108, 200, "d.join", "Entrar no Servidor", "", "", 0, gTx["dip"].empty()); p.b(cx - 100, sh / 4 + 132, 200, "done", "Cancelar", "<");
    } else if (pg == "addsrv") {
        p.t(cx, 20, gsi("srv.edit", -1) >= 0 ? "Editar servidor" : "Adicionar servidor"); p.t(cx - 100, 47, "Nome do servidor", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "sname");
        p.t(cx - 100, 85, "Endereço do servidor (IP ou IP:porta)", 0xA0A0A0, 0); p.f(cx - 100, 97, 200, "sip");
        p.b(cx - 100, sh / 4 + 108, 200, "as.ok", "Concluído", "", "", 0, gTx["sip"].empty()); p.b(cx - 100, sh / 4 + 132, 200, "done", "Cancelar", "<");
    } else if (pg == "conectando") {
        p.t(cx, 70, gJoin == NJ_FAILED ? "Falha ao conectar" : "Conectando ao servidor...", gJoin == NJ_FAILED ? 0xFF5555 : 0xFFFFFF); p.t(cx, 90, gJoinMsg, gJoin == NJ_FAILED ? 0xFF5555 : 0xA0A0A0);
        p.b(cx - 100, sh / 4 + 120, 200, "done", "Cancelar", "<");
    } else if (pg == "mundos") {
        const int sel = gsi("w.sel", -1), n = (int)gWorlds.size(); const bool no = sel < 0 || sel >= n;
        p.t(cx, 20, gHostNext ? "Hospedar Mundo (LAN)" : "Selecionar Mundo"); p.l(0, 32, sw, sh - 96, "wl", 6, n, cx + 160);
        if (gHostNext) p.t(cx, sh - 62, "Seus IPs: " + netAddrSummary(), 0x808080);
        p.b(cx - 154, sh - 52, 150, "w.play", gHostNext ? "Abrir Mundo para LAN" : "Jogar o Mundo Selecionado", "", "", 0, no); p.b(cx + 4, sh - 52, 150, "w.new", "Criar Novo Mundo");
        p.b(cx - 154, sh - 28, 72, "w.edit", "Editar", "editar", "", 0, no); p.b(cx - 76, sh - 28, 72, "w.del", "Excluir", "excluir", "", 0, no);
        p.b(cx + 4, sh - 28, 72, "w.rec", "Recriar", "", "", 0, no); p.b(cx + 82, sh - 28, 72, "done", "Cancelar", "<");
    } else if (pg == "editar") {
        p.t(cx, 20, "Editar Mundo"); p.t(cx - 100, 47, "Nome do Mundo", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "enome");
        p.b(cx - 100, sh / 4 + 96, 200, "ed.ok", "Salvar"); p.b(cx - 100, sh / 4 + 120, 200, "done", "Cancelar", "<");
    } else if (pg == "excluir") {
        const int sel = gsi("w.sel", -1); const std::string nm = sel >= 0 && sel < (int)gWorlds.size() ? gWorlds[sel].name : "";
        p.t(cx, 70, "Tem certeza de que quer excluir este mundo?"); p.t(cx, 85, "'" + nm + "' será perdido para sempre! (Muito tempo!)", 0xFF5555);
        p.b(cx - 155, sh / 6 + 96, 150, "ex.ok", "Excluir"); p.b(cx + 5, sh / 6 + 96, 150, "done", "Cancelar", "<");
    } else if (pg == "criar") {
        if (!gTx.count("nome")) gTx["nome"] = "Novo Mundo";
        const int m = gsi("mundo.modo", 0);
        p.t(cx, 20, "Criar Novo Mundo"); p.t(cx - 100, 47, "Nome do Mundo", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "nome");
        p.t(cx - 100, 85, "Será salvo em: " + gTx["nome"], 0x808080, 0);
        p.b(cx - 75, 115, 150, "mundo.modo", "Modo de Jogo", "", "Sobrevivência|Hardcore|Criativo|Aventura|Espectador", 0);
        const char* d1[5] = {"Procure por recursos, crie, ganhe", "Igual ao Sobrevivência, mas fica", "Recursos ilimitados, voo livre e", "Explore o mundo, mas não pode", "Voe através de blocos e observe"};
        const char* d2[5] = {"níveis, vida e fome", "bloqueado na dificuldade difícil", "destrua blocos instantaneamente", "quebrar nem colocar blocos", "sem poder interagir com nada"};
        p.t(cx - 100, 137, d1[m % 5], 0xA0A0A0, 0); p.t(cx - 100, 149, d2[m % 5], 0xA0A0A0, 0);
        p.b(cx - 75, 187, 150, "cr1", "Mais Opções do Mundo...", "criar2");
        p.b(L, sh - 28, 150, "cr2", "Criar Novo Mundo"); p.b(R, sh - 28, 150, "done", "Cancelar", "<");
    } else if (pg == "criar2") {
        p.t(cx, 20, "Criar Novo Mundo"); p.t(cx - 100, 47, "Seed para o Gerador de Mundos", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "seed");
        p.t(cx - 100, 85, "Deixe em branco para uma seed aleatória", 0x808080, 0);
        TG(L, 100, "w0", "Gerar Estruturas", 1); p.b(R, 100, 150, "w1", "Tipo", "", "Padrão|Superplano|Mundo Gigante|Amplificado|Personalizado", 0);
        p.t(cx - 150, 122, "Vilas, calabouços, etc", 0x808080, 0);
        TG(L, 152, "w2", "Permitir Cheats", 0); TG(R, 152, "w3", "Baú Bônus", 0);
        p.t(cx - 150, 172, "Comandos como /gamemode, /xp", 0x808080, 0);
        p.done(cx - 75, 187, 150); p.b(L, sh - 28, 150, "cr2", "Criar Novo Mundo"); p.b(R, sh - 28, 150, "cr3", "Cancelar", "<");
    } else if (pg == "mods") {
        const int so = gsi("mods.sort", 0);
        p.t(cx + 80, 15, "Mod List");
        p.b(10, 10, 43, "ms.a", "Normal", "!mods.sort:0", "", 0, so == 0); p.b(57, 10, 43, "ms.b", "A-Z", "!mods.sort:1", "", 0, so == 1); p.b(103, 10, 43, "ms.c", "Z-A", "!mods.sort:2", "", 0, so == 2);
        p.l(10, 32, 136, 140, "mods", 5, (int)modRows().size(), 140);
        p.t(78, 177, "Buscar:", 0xFFFFFF, 1); p.f(11, 188, 133, "modbusca");
        p.b(201, 219, 199, "done", "Concluído", "<");
    } else {   // generico (Realms, acessibilidade, Super Secret Settings, Pacotes de Recursos...)
        p.t(cx, 15, gOtherTitle); p.done(cx - 100, sh - 27, 200);
    }
}
// ---------------- navegação ----------------
static void launcherCommit() { gPlayerName = nickSanitize(gTx["lname"]); gTx["lname"] = gPlayerName; profileSave(); }
static void launcherOpen() { gTx["lname"] = gPlayerName; skinScan(); gPStack.clear(); gPage = "launcher"; gScreen = SC_OTHER; gFocus.clear(); gDrag.clear(); }
static void drawSkinPreview(int cx, int top, int s, const uint8_t* skin = ::skin) {   // frente do personagem (cabeca, corpo, bracos, pernas + camadas) com os pixels da skin atual; s = unidades por pixel
    const int x0 = cx - 4 * s, aw = slim ? 3 : 4;
    auto px = [&](int sx, int sy, int dx, int dy, int w, int h, int ox, int oy) {   // regiao w*h da skin em (sx,sy) -> tela (dx,dy); ox/oy: camada extra em (ox,oy)
        for (int pass = 0; pass < 2; pass++) { const int bx = pass ? ox : sx, by = pass ? oy : sy; if (pass && ox < 0) break;
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
                const uint8_t* q = &skin[((by + j) * 64 + bx + i) * 4]; if (q[3] < 128) continue;
                U((float)(dx + i * s), (float)(dy + j * s), (float)s, (float)s, ((unsigned)q[0] << 16) | ((unsigned)q[1] << 8) | q[2]);
            } }
    };
    px(8, 8, x0 + 0, top, 8, 8, 40, 8);                                       // cabeca + chapeu (8x8 -> topo 8 px)
    px(20, 20, x0, top + 8 * s, 8, 12, 20, 36);                              // corpo + casaco
    px(44, 20, x0 - aw * s, top + 8 * s, aw, 12, 44, 36);                   // braco direito do personagem (esquerda da tela) + manga
    px(36, 52, x0 + 8 * s, top + 8 * s, aw, 12, 52, 52);                    // braco esquerdo + manga
    px(4, 20, x0, top + 20 * s, 4, 12, 4, 36);                               // perna direita + calca
    px(20, 52, x0 + 4 * s, top + 20 * s, 4, 12, 4, 52);                     // perna esquerda + calca
}
static void pageBack(); static void pageAction(const std::string& id);
static void pageGo(const std::string& name) {
    std::string n = name; if (!n.empty() && n[0] == '?') { gOtherTitle = n.substr(1); n = "generico"; }
    if (n == "launcher") { gTx["lname"] = gPlayerName; skinScan(); }
    if (gScreen == SC_TITLE) gScreen = SC_OTHER;
    if (!gPage.empty()) gPStack.push_back(gPage);
    if (n == "editar") { const int sl = gsi("w.sel", -1); if (sl >= 0 && sl < (int)gWorlds.size()) gTx["enome"] = gWorlds[sl].name; }
    gPage = n; gDrag.clear(); gFocus = n == "editar" ? "enome" : n == "criar" ? "nome" : n == "criar2" ? "seed" : n == "direto" ? "dip" : n == "addsrv" ? "sname" : ""; txtKbd();
}
static void pageBack() {
    gDrag.clear(); gFocus.clear(); txtKbd(); gGpBind = -1;
    if (gPage == "launcher") launcherCommit();
    if (gPage == "conectando") netStop();   // cancelar a conexao
    if (!gPStack.empty() && gPStack.back() == "multi") gHostNext = false;   // voltou do "Hospedar Mundo" para a lista
    if (gPStack.empty()) { gPage.clear(); if (gScreen == SC_OTHER) gScreen = SC_TITLE; }
    else { gPage = gPStack.back(); gPStack.pop_back(); }
}
static void leavePages() { gPage.clear(); gPStack.clear(); gFocus.clear(); gDrag.clear(); }
static void pageAction(const std::string& id) {
    if (id.size() > 2 && id[0] == 'l' && id[1] == '.') {   // launcher
        if (id == "l.play") { launcherCommit(); const bool first = gPStack.empty(); if (first) { leavePages(); if (gScreen == SC_OTHER) gScreen = SC_TITLE; } else pageBack(); }
        else if (id == "l.prev") skinStep(-1); else if (id == "l.next") skinStep(1);
        else if (id == "l.prev2") skin2Step(-1); else if (id == "l.next2") skin2Step(1);
        else if (id == "l.kb") { gOskForce = !gOskForce; gFocus = "lname"; }
        return;
    }
    if (id == "g.reset") { gpResetMap(); return; }
    if (id.size() >= 2 && id[0] == 'g' && isdigit((unsigned char)id[1])) { gGpBind = atoi(id.c_str() + 1); return; }
    if (false) {
    }
    if (id.size() > 1 && (id[0] == 'm' && id[1] == 's' || id == "d.join" || id == "as.ok")) {   // aba Multiplayer
        const std::vector<SrvRow> rows = srvRows(); const int ms = gsi("multi.sel", -1); const bool okS = ms >= 0 && ms < (int)rows.size();
        std::string h; int pt;
        if (id == "ms0" && okS) { netJoinBegin(rows[ms].host, rows[ms].port); pageGo("conectando"); }
        else if (id == "ms1") pageGo("direto");
        else if (id == "ms2") { gTx["sname"].clear(); gTx["sip"].clear(); gSt["srv.edit"] = -1; pageGo("addsrv"); }
        else if (id == "ms3" && okS && rows[ms].saved) { gTx["sname"] = rows[ms].name; gTx["sip"] = rows[ms].host + ":" + std::to_string(rows[ms].port); gSt["srv.edit"] = rows[ms].idx; pageGo("addsrv"); }
        else if (id == "ms4" && okS && rows[ms].saved) { srvDel(rows[ms].idx); gSt["multi.sel"] = -1; }
        else if (id == "ms5") lanScanForce();
        else if (id == "ms6") { openSingleplayer(); gHostNext = true; }
        else if (id == "d.join" && srvParse(gTx["dip"], h, pt)) { netJoinBegin(h, pt); pageGo("conectando"); }
        else if (id == "as.ok") { srvAdd(gsi("srv.edit", -1), gTx["sname"], gTx["sip"]); gSt["multi.sel"] = -1; pageBack(); }
        return;
    }
    const int sel = gsi("w.sel", -1); const bool ok = sel >= 0 && sel < (int)gWorlds.size();
    if (id == "w.play" && ok) { gMenuDir = gWorlds[sel].dir; leavePages(); gHostPending = gHostNext; gHostNext = false; gMenuAct = 1; }
    else if (id == "w.new") newWorldPage();
    else if (id == "w.rec" && ok) { gTx["nome"] = gWorlds[sel].name; gTx["seed"] = std::to_string(gWorlds[sel].seed); gSt["mundo.modo"] = gWorlds[sel].hard ? 1 : (gWorlds[sel].mode == 1 ? 2 : gWorlds[sel].mode == 2 ? 3 : gWorlds[sel].mode == 3 ? 4 : 0); pageGo("criar"); }
    else if (id == "ed.ok" && ok) { std::string nn = gTx["enome"]; while (!nn.empty() && nn.back() == ' ') nn.pop_back(); if (!nn.empty()) { renameWorld(gWorlds[sel].dir, nn); scanWorlds(); } pageBack(); }
    else if (id == "ex.ok" && ok) { deleteWorld(gWorlds[sel].dir); scanWorlds(); gSt["w.sel"] = -1; gSt["wl.scr"] = 0; pageBack(); if (gWorlds.empty()) { leavePages(); gScreen = SC_TITLE; } }
    else if (id == "cr2") {   // Criar Novo Mundo: grava level.cfg (nome, seed, modo) e já entra no mundo, como o Minecraft
        std::string nn = gTx["nome"]; while (!nn.empty() && nn.back() == ' ') nn.pop_back(); if (nn.empty()) nn = "Novo Mundo";
        const std::string d = createWorld(nn, gTx["seed"], gsi("mundo.modo", 0)); if (d.empty()) return;
        gTx.erase("nome"); gTx.erase("seed"); gSt["mundo.modo"] = 0; gMenuDir = d; leavePages(); gHostPending = gHostNext; gHostNext = false; gMenuAct = 1;
    }
}
static void newWorldPage() { gTx["nome"] = "Novo Mundo"; gTx["seed"].clear(); gSt["mundo.modo"] = 0; pageGo("criar"); }
static void openSingleplayer() { gHostNext = false; scanWorlds(); gSt["w.sel"] = -1; gSt["wl.scr"] = 0; if (gWorlds.empty()) newWorldPage(); else pageGo("mundos"); }
static std::string sldText(const Wd& d, float v) {
    char b[64]; std::string s; const int n = (int)d.opt.size();
    if (n) s = d.opt[std::max(0, std::min(n - 1, (int)floorf(v * (n - 1) + .5f)))];
    else switch (d.fmt) {
        case 0: snprintf(b, 64, "%d%%", (int)floorf(v * 100 + .5f)); s = b; break;
        case 1: snprintf(b, 64, "%dpx", (int)floorf(d.a + (d.b - d.a) * v + .5f)); s = b; break;
        case 2: { const int f = (int)floorf(30 + 80 * v + .5f); if (f == 70) s = "Normal"; else if (f == 110) s = "Quake Pro"; else { snprintf(b, 64, "%d", f); s = b; } break; }
        case 3: { const int c = (int)floorf(2 + 30 * v + .5f); snprintf(b, 64, "%d ", c); s = b; s += c < 4 ? "Minúsculo" : c < 7 ? "Curto" : c == 7 ? "Normal" : c < 16 ? "Normal+" : c == 16 ? "Longo" : "Extremo"; break; }
        case 4: if (v > .999f) s = "Ilimitada"; else { snprintf(b, 64, "%d", (int)floorf(10 + 250 * v + .5f)); s = b; } break;
        case 5: if (v < .01f) s = "Sombrio"; else if (v > .99f) s = "Claro"; else { snprintf(b, 64, "%d%%", (int)floorf(v * 100 + .5f)); s = b; } break;
        default: snprintf(b, 64, "%d%%", (int)floorf(v * 200 + .5f)); s = b; break;
    }
    return d.label + ": " + s;
}
static void sldSet(const Wd& d, int ux) {
    float v = std::max(0.f, std::min(1.f, (ux - (d.x + 4)) / (float)(d.w - 8))); const int n = (int)d.opt.size();
    if (n > 1) v = floorf(v * (n - 1) + .5f) / (n - 1); gSl[d.id] = v;
}
// ---------------- desenho ----------------
static void oskDraw(int sw, int sh, int ux, int uy);
static void drawPage(int w, int h, bool ingame) {
    blViewport(w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0;
    const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, ux = gMX / G, uy = gMY / G;
    if (ingame) {
        glDisable(GL_TEXTURE_2D); glBegin(GL_QUADS);
        glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 192 / 255.f); glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 208 / 255.f); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd(); glEnable(GL_TEXTURE_2D);
    } else if (mtDirt) mBg(w, h, 0, 0, (float)w, (float)h, .25f);
    std::vector<Wd> v; pageBuild(gPage, sw, sh, v);
    if (gPage == "launcher") { const int ps = sh >= 290 ? 2 : 1; drawSkinPreview(sw / 2 - 28 * ps, 22, ps); if (gSkin2Has) drawSkinPreview(sw / 2 + 28 * ps, 22, ps, gSkin2Px); }
    for (const Wd& d : v) {
        if (d.k == WK_TXT) {
            if (d.fmt == 0) uText((float)d.x, (float)d.y, d.label.c_str(), d.col, true);
            else if (d.fmt == 1) uCenter(d.x, (float)d.y, d.label.c_str(), d.col);
            else uText((float)(d.x - uWidth(d.label.c_str())), (float)d.y, d.label.c_str(), d.col, true);
        } else if (d.k == WK_BTN) {
            std::string lb = d.label; if (!d.opt.empty()) lb += ": " + d.opt[gsi(d.id, (int)d.def) % (int)d.opt.size()];
            mBtn({d.x, d.y, d.w, d.h}, lb.c_str(), !d.dis, ux, uy);
        } else if (d.k == WK_SLD) {
            const float val = gsf(d.id, d.def); PauseBtn tr = {d.x, d.y, d.w, d.h, ""}; pauseButton(tr, 2);
            const float hx = d.x + (d.w - 8) * val; U(hx, (float)d.y, 8, 20, 0x000000); U(hx + 1, d.y + 1.f, 6, 18, 0x555555); U(hx + 1, d.y + 1.f, 5, 17, 0xC6C6C6); U(hx + 2, d.y + 2.f, 4, 16, 0x8B8B8B);
            const bool hov = inR({d.x, d.y, d.w, d.h}, ux, uy) || gDrag == d.id; const std::string lb = sldText(d, val);
            uCenter(d.x + d.w / 2, (float)(d.y + 6), lb.c_str(), hov ? 0xFFFFA0 : 0xFFFFFF);
        } else if (d.k == WK_FLD) mField(d.x, d.y, d.w, gTx[d.id], gFocus == d.id);
        else if (d.k == WK_LST) {
            const int st = d.fmt, rh = lstRow(st), pad = lstPad(st), sc = lstScr(d);
            U((float)d.x, (float)d.y, (float)d.w, (float)d.h, 0x000000, .55f);
            glEnable(GL_SCISSOR_TEST); glScissor(d.x * G, h - (d.y + d.h) * G, d.w * G, d.h * G);
            std::vector<SrvRow> srows; if (st == 2) srows = srvRows();
            const int sel = st == 0 ? gsi("lang", 22) : st == 1 ? gsi("shd.sel", 0) : st == 2 ? gsi("multi.sel", -1) : st == 6 ? gsi("w.sel", -1) : -1;
            for (int i = 0; i < d.n; i++) {
                const int ry = d.y + pad + i * rh - sc; if (ry + rh < d.y || ry > d.y + d.h) continue;
                if (st == 0) uCenter(cx, (float)(ry + 5), LANGS[i], 0xFFFFFF);
                else if (st == 1) {
                    if (sel == i) { U((float)d.x, (float)ry, (float)d.w, 16, 0xFFFFFF); U(d.x + 1.f, ry + 1.f, d.w - 2.f, 14, 0x000000); }
                    uCenter(d.x + d.w / 2, (float)(ry + 4), SHD[i], 0xFFFFFF);
                } else if (st == 2) {
                    if (i >= (int)srows.size()) continue;
                    const SrvRow& s = srows[i];
                    if (sel == i) { U(cx - 151.f, ry - 2.f, 302, 36, 0x808080); U(cx - 150.f, ry - 1.f, 300, 34, 0x000000); }
                    if (mtUnk) mIcon2(mtUnk, (float)(cx - 149), (float)ry, 32, 32); else U(cx - 149.f, (float)ry, 32, 32, 0x404040);
                    uText((float)(cx - 114), (float)(ry + 1), s.name.c_str(), 0xFFFFFF, true); uText((float)(cx - 114), (float)(ry + 12), s.l1.c_str(), s.c1, true); uText((float)(cx - 114), (float)(ry + 23), s.l2.c_str(), s.c2, true);
                    if (!s.cnt.empty()) uText((float)(cx + 137 - uWidth(s.cnt.c_str())), (float)(ry + 1), s.cnt.c_str(), 0x808080, true);
                    if (s.ok) for (int k = 0; k < 5; k++) { const float bh = 2.f + k * 1.5f; U(cx + 141.f + k * 2, ry + 10 - bh, 2, bh, 0x00C800); } else uText((float)(cx + 141), (float)(ry + 1), "x", 0xAA0000, true);
                } else if (st == 3) {
                    const std::string it = CTRL[i];
                    if (it[0] == '#') uCenter(cx, (float)(ry + 6), it.c_str() + 1, 0xFFFFFF);
                    else {
                        const size_t q = it.find('|'); const std::string a = it.substr(0, q), k = it.substr(q + 1);
                        uText((float)(cx - 215), (float)(ry + 7), a.c_str(), 0xFFFFFF, true); mBtn({cx - 18, ry + 1, 72, 18}, k.c_str(), true, ux, uy); mBtn({cx + 68, ry + 1, 48, 18}, "Redefinir", false, ux, uy);
                    }
                } else if (st == 4) {
                    const std::string it = lstItem(4, i); const size_t q = it.find('|');
                    uText(10, (float)(ry + 1), "C", 0xFFFFFF, true); uText(20, (float)(ry + 1), it.substr(0, q).c_str(), 0xFFFFFF, true); uText(230, (float)(ry + 1), it.substr(q + 1).c_str(), 0xFFFFFF, true);
                } else if (st == 5) {
                    const std::string it = lstItem(5, i); const size_t q = it.find('|');
                    uText(d.x + 3.f, (float)(ry + 2), it.substr(0, q).c_str(), 0xFFFFFF, true); uText(d.x + 3.f, (float)(ry + 12), it.substr(q + 1).c_str(), 0xC0C0C0, true); uText(d.x + 3.f, (float)(ry + 22), "0 child mods", 0xC0C0C0, true);
                } else if (st == 6) {
                    const WInfo& wi = gWorlds[i];
                    if (sel == i) { U(cx - 151.f, ry - 2.f, 302, 36, 0x808080); U(cx - 150.f, ry - 1.f, 300, 34, 0x000000); }
                    if (mtUnk) mIcon2(mtUnk, (float)(cx - 149), (float)ry, 32, 32); else U(cx - 149.f, (float)ry, 32, 32, 0x404040);
                    char dt[40] = ""; const time_t tt = (time_t)wi.played; if (wi.played > 0) { struct tm* tm_ = localtime(&tt); if (tm_) strftime(dt, sizeof dt, "%d/%m/%y %H:%M", tm_); }
                    uText((float)(cx - 114), (float)(ry + 1), wi.name.c_str(), 0xFFFFFF, true);
                    uText((float)(cx - 114), (float)(ry + 12), (wi.dir + " (" + dt + ")").c_str(), 0x808080, true);
                    uText((float)(cx - 114), (float)(ry + 23), wi.hard ? "Modo Hardcore" : wi.mode == 3 ? "Modo Espectador" : wi.mode == 2 ? "Modo Aventura" : wi.mode == 1 ? "Modo Criativo" : "Modo Sobrevivência", 0x808080, true);
                }
            }
            if (st == 2) {   // busca de partidas na rede local
                const int y0 = d.y + pad + d.n * rh + 12 - sc; static const char* dots[4] = {"O o o", "o O o", "o o O", "o O o"};
                uCenter(cx, (float)y0, "Procurando partidas em sua rede local", 0xFFFFFF); uCenter(cx, (float)(y0 + 8), dots[(SDL_GetTicks() / 100) % 4], 0x808080);
            }
            glDisable(GL_SCISSOR_TEST);
            const int mx = lstMax(d);
            if (mx > 0 && d.a > 0) {
                const int tot = mx + d.h, bh = std::max(32, d.h * d.h / tot), by = d.y + (d.h - bh) * sc / mx, bx = (int)d.a;
                U((float)bx, (float)d.y, 6, (float)d.h, 0x000000); U((float)bx, (float)by, 6, (float)bh, 0x808080); U((float)bx, (float)by, 5, bh - 1.f, 0xC0C0C0);
            }
        }
    }
    if (oskActive()) oskDraw(sw, sh, ux, uy);
    glDisable(GL_SCISSOR_TEST); glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
// ---------------- teclado virtual (para campos de texto com controle) ----------------
// Aparece sobre qualquer campo de texto focado quando ha controle conectado (ou pelo botao "Aa" do launcher). Cliques e direcional/A funcionam.
struct OKey { std::string lb; int kind; int x, y, w; };   // kind: 0 caractere, 1 maiuscula, 2 apagar, 3 espaco, 4 OK
static int gOskSel = 0, gOskSW = 427, gOskSH = 240;
static bool oskActive() { return (chatOpen || !gFocus.empty()) && (gOskForce || gpCount() > 0); }   // campo de texto focado OU chat aberto
static std::string& oskText() { return chatOpen ? chatIn : gTx[gFocus]; }
static std::vector<OKey> oskKeys(int sw, int sh) {
    std::vector<OKey> k; const int cx = sw / 2, py = sh - 124;
    static const char* R[4] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
    for (int r = 0; r < 4; r++) { const int n = (int)strlen(R[r]), x0 = cx - (n * 20 - 2) / 2;
        for (int i = 0; i < n; i++) k.push_back({std::string(1, (gOskCaps && r > 0) ? (char)toupper(R[r][i]) : R[r][i]), 0, x0 + i * 20, py + 22 + 20 * r, 18}); }
    struct B { const char* lb; int kind, w; }; static const B bt[8] = {{"Aa", 1, 24}, {"_", 0, 16}, {"-", 0, 16}, {"Espaço", 3, 60}, {".", 0, 16}, {"/", 0, 16}, {"<-", 2, 24}, {"OK", 4, 24}};
    int x = cx - 108; for (const B& b : bt) { k.push_back({b.lb, b.kind, x, py + 22 + 80, b.w}); x += b.w + 2; }
    return k;
}
static int oskHit(int ux, int uy) { const std::vector<OKey> k = oskKeys(gOskSW, gOskSH); for (int i = 0; i < (int)k.size(); i++) if (ux >= k[i].x && ux < k[i].x + k[i].w && uy >= k[i].y && uy < k[i].y + 18) return i; return -1; }
static void oskNav(int dx, int dy) {
    const std::vector<OKey> k = oskKeys(gOskSW, gOskSH); const int n = (int)k.size(); gOskSel = std::max(0, std::min(n - 1, gOskSel)); const OKey c = k[gOskSel];
    if (dx) { const int j = gOskSel + dx; if (j >= 0 && j < n && k[j].y == c.y) gOskSel = j; else { int f = gOskSel; while (f - dx >= 0 && f - dx < n && k[f - dx].y == c.y) f -= dx; gOskSel = f; } }   // na ponta da fileira volta para o outro lado
    if (dy) { int ty = c.y + dy * 20; const int top = k[0].y, bot = k.back().y; if (ty < top) ty = bot; if (ty > bot) ty = top;
        int best = -1, bd = 1 << 30; for (int i = 0; i < n; i++) if (k[i].y == ty) { const int d = abs((k[i].x + k[i].w / 2) - (c.x + c.w / 2)); if (d < bd) { bd = d; best = i; } } if (best >= 0) gOskSel = best; }
}
static void oskPut(const std::string& t) { if (chatOpen) gpPushText(t.c_str()); else if (gTx[gFocus].size() < 32) gTx[gFocus] += t; }   // chat: vira texto digitado (chatText)
static void oskBack() { if (chatOpen) { gpPushKey(SDLK_BACKSPACE); return; } std::string& s = gTx[gFocus]; while (!s.empty() && (s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); }
static void oskOk() { if (chatOpen) gpPushKey(SDLK_RETURN); else gFocus.clear(); }   // chat: Enter envia
static void oskPress() {
    const std::vector<OKey> k = oskKeys(gOskSW, gOskSH); if (k.empty()) return; gOskSel = std::max(0, std::min((int)k.size() - 1, gOskSel)); const OKey& c = k[gOskSel];
    if (c.kind == 1) gOskCaps = !gOskCaps; else if (c.kind == 2) oskBack(); else if (c.kind == 4) oskOk();
    else { oskPut(c.kind == 3 ? " " : c.lb); if (c.kind == 0 && gOskCaps) gOskCaps = false; }
}
static void oskSpace() { oskPut(" "); }
static void oskShift() { gOskCaps = !gOskCaps; }
static void oskClose() { if (chatOpen) gpPushKey(SDLK_ESCAPE); else gFocus.clear(); }
static bool oskMouse(int mx, int my, int w, int h) {   // clique do mouse no teclado do chat (a janela inteira); true = consumiu
    const int g = guiScaleFor(w, h), sw = (w + g - 1) / g, sh = (h + g - 1) / g; gOskSW = sw; gOskSH = sh;
    const int hk = oskHit(mx / g, my / g); if (hk >= 0) { gOskSel = hk; oskPress(); return true; } return my / g >= sh - 124;
}
static void oskDraw(int sw, int sh, int ux, int uy) {
    gOskSW = sw; gOskSH = sh; const int py = sh - 124; const std::vector<OKey> k = oskKeys(sw, sh); gOskSel = std::max(0, std::min((int)k.size() - 1, gOskSel));
    U(0, (float)py, (float)sw, (float)(sh - py), 0x000000, .88f);
    mField(sw / 2 - 100, py + 2, 200, oskText(), true);
    for (int i = 0; i < (int)k.size(); i++) { const OKey& c = k[i]; mBtn({c.x, c.y, c.w, 18}, c.lb.c_str(), true, ux, uy);
        if (i == gOskSel) { U(c.x - 1.f, c.y - 1.f, c.w + 2.f, 1, 0xFFFF55); U(c.x - 1.f, c.y + 18.f, c.w + 2.f, 1, 0xFFFF55); U(c.x - 1.f, (float)c.y, 1, 18, 0xFFFF55); U(c.x + c.w * 1.f, (float)c.y, 1, 18, 0xFFFF55); } }
    if (gpCount()) uCenter(sw / 2, (float)(sh - 10), "Direcional: tecla | A: digitar | B: apagar | X: maiúscula | Y: espaço | Start: OK", 0x808080);
}
// ---------------- eventos ----------------
static bool pageEvent(const SDL_Event& e, int w, int h) {
    if (gPage.empty()) return false;
    G = guiScaleFor(w, h); const int sw = (w + G - 1) / G, sh = (h + G - 1) / G;
    std::vector<Wd> v; pageBuild(gPage, sw, sh, v);
    if (e.type == SDL_MOUSEMOTION) {
        gMX = e.motion.x; gMY = e.motion.y;
        if (!gDrag.empty()) for (const Wd& d : v) if (d.k == WK_SLD && d.id == gDrag) sldSet(d, gMX / G);
        return true;
    }
    const int ux = gMX / G, uy = gMY / G;
    if (e.type == SDL_KEYDOWN) {
        const SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_ESCAPE) { if (oskActive()) gFocus.clear(); else pageBack(); }
        else if (!gFocus.empty() && k == SDLK_BACKSPACE) { std::string& s = gTx[gFocus]; while (!s.empty() && (s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) gFocus.clear();
        return true;
    }
    if (e.type == SDL_TEXTINPUT) { if (!gFocus.empty() && gTx[gFocus].size() < 32 && e.text.text[0] >= 32) gTx[gFocus] += e.text.text; return true; }
    if (e.type == SDL_MOUSEWHEEL) { for (const Wd& d : v) if (d.k == WK_LST) { gSt[d.id + ".scr"] -= e.wheel.y * lstRow(d.fmt); lstScr(d); break; } return true; }
    if (e.type == SDL_MOUSEBUTTONUP) { if (gDrag == "v1") optSave(); gDrag.clear(); return true; }
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) return true;
    if (oskActive()) { gOskSW = sw; gOskSH = sh; const int hk = oskHit(ux, uy); if (hk >= 0) { gOskSel = hk; oskPress(); return true; } if (uy >= sh - 124) return true; }   // clique no teclado virtual
    gFocus.clear();
    for (const Wd& d : v) {
        if (!inR({d.x, d.y, d.w, d.h}, ux, uy)) continue;
        if (d.k == WK_FLD) gFocus = d.id;
        else if (d.k == WK_SLD) { gDrag = d.id; sldSet(d, ux); }
        else if (d.k == WK_BTN && !d.dis) {
            if (d.go == "<") pageBack();
            else if (!d.go.empty() && d.go[0] == '=') {   // "=prefixo:valor": define todos os botões alternáveis com esse prefixo
                const size_t q = d.go.find(':'); const std::string pre = d.go.substr(1, q - 1); const int val = atoi(d.go.c_str() + q + 1);
                for (const Wd& o : v) if (o.k == WK_BTN && !o.opt.empty() && o.id.compare(0, pre.size(), pre) == 0) gSt[o.id] = val;
            } else if (!d.go.empty() && d.go[0] == '!') { const size_t q = d.go.find(':'); gSt[d.go.substr(1, q - 1)] = atoi(d.go.c_str() + q + 1); }
            else if (!d.go.empty()) pageGo(d.go);
            else if (!d.opt.empty()) { gSt[d.id] = (gsi(d.id, (int)d.def) + 1) % (int)d.opt.size();
                if (d.id == "gfxapi") { gSt["gfxset"] = 1; if (!gApiOpenGL()) remove("no_vulkan"); optSave(); } }   // escolher Vulkan de novo limpa o bloqueio deixado por uma falha anterior
            else { pageAction(d.id); return true; }
            break;
        } else if (d.k == WK_LST) {
            const int idx = (uy - d.y - lstPad(d.fmt) + lstScr(d)) / lstRow(d.fmt);
            if (idx >= 0 && idx < d.n) { if (d.fmt == 0) gSt["lang"] = idx; else if (d.fmt == 1) { gSt["shd.sel"] = BL_NO_FANCY ? 0 : idx; gShaderPackOn = !BL_NO_FANCY && idx > 0; } else if (d.fmt == 2) { static Uint32 mt = 0; static int mi = -1; const Uint32 tn = SDL_GetTicks(); gSt["multi.sel"] = idx; if (mi == idx && tn - mt < 450) { mi = -1; pageAction("ms0"); return true; } mi = idx; mt = tn; }
                else if (d.fmt == 6) { static Uint32 lt = 0; static int li = -1; const Uint32 tn = SDL_GetTicks(); gSt["w.sel"] = idx; if (li == idx && tn - lt < 450) { li = -1; pageAction("w.play"); return true; } li = idx; lt = tn; } }
        }
    }
    txtKbd();
    return true;
}
