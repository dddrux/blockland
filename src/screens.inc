// ================= telas do Minecraft 1.8.9 (Opções, Vídeo OptiFine, Controles, Idioma, Multijogador, Criar mundo, Mods...) =================
// Tudo guiado por dados: pageBuild() monta a lista de widgets de cada tela (usada tanto no desenho quanto nos eventos).
// Botões alternam valores/abrem outras telas; nada altera o jogo (placeholders visuais). Coordenadas em unidades da GUI (como o Java).
enum { WK_BTN, WK_SLD, WK_TXT, WK_FLD, WK_LST };
struct Wd { int k = 0, x = 0, y = 0, w = 0, h = 20, fmt = 0, n = 0; float def = 0, a = 0, b = 1; unsigned col = 0xFFFFFF; bool dis = false; std::string id, label, go; std::vector<std::string> opt; };
static std::string gPage, gFocus, gDrag; static std::vector<std::string> gPStack;
static std::map<std::string, int> gSt; static std::map<std::string, float> gSl; static std::map<std::string, std::string> gTx;
static int gsi(const std::string& k, int d) { auto i = gSt.find(k); return i == gSt.end() ? d : i->second; }
static float gsf(const std::string& k, float d) { auto i = gSl.find(k); return i == gSl.end() ? d : i->second; }
static void cmdSetFov(int f) { gSl["fov"] = (std::max(30, std::min(110, f)) - 30) / 80.f; }   // /fov <30..110>
// ---- distancia de renderizacao (slider "v1": 2..32 chunks, padrao 7) + persistencia em options.cfg ----
#ifdef BL_RPI
static const float RD_DEF = 2.f / 30.f;   // patchRpiOpt.1: padrao 4 chunks   // patchRpi.1: padrao 3 chunks (49 chunks em vez de 169)
#else
static const float RD_DEF = 5.f / 30.f;
#endif   // (7-2)/30
#ifdef BL_RPI
static bool gRpiSplit = false;   // split ligado: 4 chunks e 30 FPS
static inline int rdChunks() { return std::min(gRpiSplit ? 4 : BL_RPI_MAXRD, (int)floorf(2 + 30 * gsf("v1", RD_DEF) + .5f)); }   // Raspberry: no maximo 6 chunks
#else
static inline int rdChunks() { return (int)floorf(2 + 30 * gsf("v1", RD_DEF) + .5f); }
#endif
static inline float fogEndD() { const float sc = !gSplit ? 1.f : gSplitN == 2 ? .8f : .6f; return std::max(24.f, (RAD * 16.f - 4.f) * sc); }   // split: menos distancia por tela (GPU)   // RAD 7 => 108 (valor original)
static inline float fogStartD() { const int fi = (int)floorf(gsf("d9", .33f) * 3.f + .5f); return fogEndD() * (.35f + .15f * std::max(0, std::min(3, fi))); }   // patchOpt.3: Inicio do nevoeiro (0.2/0.4/0.6/0.8)                  // RAD 7 => ~38
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
#ifdef BL_RPI
#define BL_NO_FANCY 1
#define BL_CLOUD_DEF 3   // Raspberry: nuvens (modo imediato, 841 celulas) desligadas por padrao
#else
#define BL_CLOUD_DEF 0
#define BL_NO_FANCY 0
#endif
//   // celular agora usa o pacote LEVE (agua simples + sombras 1 cascata); reflexo planar, god rays e flare ficam so no desktop
#ifdef __ANDROID__
#define BL_LITE 1
#else
#define BL_LITE 0
#endif
static bool gApiOpenGL() { return gsi("gfxapi", BL_GFX_DEF) == 1; }   // renderizador escolhido (0 = Vulkan, padrao; 1 = OpenGL). Vale no proximo inicio: o BlocklandActivity le "renderer=" do options.cfg
static void sndSyncVol() { gSndMaster = gsf("m0", 1.f); for (int i = 1; i <= 8; i++) gSndCat[i] = gsf(std::string("m") + (char)('0' + i), 1.f); }   // sliders de Musica & Sons -> audio.inc
static void gfxTogglesSync() {   // patchGfx.1: d17 = shader do Nether, d18 = culling de folhas (remonta a malha ao mudar)
    gNetherFx = false;   /* efeitos do Nether (shader/brasas) removidos */ const int d2 = gsi("d2", 0); const bool fastLv = d2 == 1 || (d2 == 0 && gsi("v0", 0) == 0); const bool lc = gsi("d18", 1) != 0 && fastLv;   // Graficos Bonitos = folhas por inteiro (sem culling)
    { const int sm = BL_SMOOTH_FORCE(std::max(0, std::min(2, gsi("v2", 2)))); if (sm != gSmoothMode) { gSmoothMode = sm; for (auto& kv : W) kv.second->dirty = true; gKick = true; } }   // patchLight.1: Iluminacao Suave remonta a malha
    { gGuiScaleOpt = std::max(0, std::min(4, gsi("v6", 0))); if (gGuiScaleOpt > 0) guiScaleCfg = (float)gGuiScaleOpt; else if (gGuiScaleOpt != gGuiScalePrev) guiScaleCfg = 0.f; gGuiScalePrev = gGuiScaleOpt; }   // patchOpt.1: Interface (v6) controla a escala da GUI
    gOptSky = gsi("d4", 1) != 0; gOptStars = gsi("d5", 1) != 0; gOptSunMoon = gsi("d6", 1) != 0; gCloudH = gsf("d1", 0.f);   // patchOpt.3
    gViewBobOn = gsi("v5", 1) != 0; gGfxFancy = gsi("v0", 0) == 1;   // Movimentos na Visao; Graficos Bonitos
    { const bool lf = fastLv; if (lf != gLeafFast) { gLeafFast = lf; for (auto& kv : W) kv.second->dirty = true; gKick = true; } leafGfxApply(lf); }   // patchGfx.2: Rapido = folhas opacas + culling total; Bonito = folhas translucidas
    if (lc != gLeafCull) { gLeafCull = lc; for (auto& kv : W) kv.second->dirty = true; gKick = true; }
}
static void optSave() { sndSyncVol(); FILE* f = fopen("options.cfg", "wb"); if (f) { fprintf(f, "renderdist=%d\nrenderer=%s\nrenderer_set=%d\nshadows=%d\n", rdChunks(), gApiOpenGL() ? "opengl" : "vulkan", gsi("gfxset", 0), gsi("shd.mob", 0));
    fprintf(f, "netherfx=%d\nleafcull=%d\nsmooth=%d\n", gsi("d17", 1), gsi("d18", 1), gsi("v2", 2)); fprintf(f, "sndmute=%d\nsnddev=%s\n", gSndMuted ? 1 : 0, gSndDevName.c_str()); fprintf(f, "fortress=%d\ngfxfancy=%d\nguiscale=%d\nviewbob=%d\ncloudmode=%d\nmaxfps=%d\nbright=%d\n", gsi("fort", 33), gsi("v0", 0), gsi("v6", 0), gsi("v5", 1), gsi("d0", 0), (int)floorf(gsf("v3", 1.f) * 100.f + .5f), (int)floorf(gsf("v8", 1.f) * 100.f + .5f)); fprintf(f, "soundvol="); for (int i = 0; i <= 8; i++) fprintf(f, "%s%d", i ? "," : "", (int)floorf(gsf(std::string("m") + (char)('0' + i), 1.f) * 100.f + .5f)); fprintf(f, "\n"); modsOffLoad(); fprintf(f, "modsoff=%s\n", modsOffLine().c_str());
    auto okId = [](const std::string& k) { return !k.empty() && (k == "fov" || k.compare(0, 3, "an.") == 0 || (strchr("vdqpx", k[0]) && k.size() > 1 && isdigit((unsigned char)k[1]))); };   // patchOpt.3: TODAS as opcoes de video/animacao/qualidade/desempenho/diversas persistem
    for (auto& kv : gSt) if (okId(kv.first)) fprintf(f, "opt.%s=%d\n", kv.first.c_str(), kv.second);
    for (auto& kv : gSl) if (okId(kv.first)) fprintf(f, "osl.%s=%d\n", kv.first.c_str(), (int)floorf(kv.second * 1000.f + .5f));
    fclose(f); } }
static void fortSetRate(int v) { gSt["fort"] = std::max(0, std::min(100, v)); optSave(); }
static int fortGetRate() { return gsi("fort", 33); }
static void optLoad() {
    FILE* f = fopen("options.cfg", "rb"); if (!f) return; char ln[384];
    while (fgets(ln, sizeof ln, f)) { char rn[16]; if (sscanf(ln, "renderer=%15s", rn) == 1) gSt["gfxapi"] = !strcmp(rn, "opengl") ? 1 : 0;
        int rs; if (sscanf(ln, "renderer_set=%d", &rs) == 1) gSt["gfxset"] = rs;
        int sm; if (sscanf(ln, "shadows=%d", &sm) == 1) gSt["shd.mob"] = sm;
        { int nf; if (sscanf(ln, "netherfx=%d", &nf) == 1) gSt["d17"] = nf; if (sscanf(ln, "fortress=%d", &nf) == 1) gSt["fort"] = std::max(0, std::min(100, nf)); if (sscanf(ln, "leafcull=%d", &nf) == 1) gSt["d18"] = nf; if (sscanf(ln, "smooth=%d", &nf) == 1) gSt["v2"] = std::max(0, std::min(2, nf)); }
        { int sv[9]; if (sscanf(ln, "soundvol=%d,%d,%d,%d,%d,%d,%d,%d,%d", &sv[0], &sv[1], &sv[2], &sv[3], &sv[4], &sv[5], &sv[6], &sv[7], &sv[8]) == 9) for (int i = 0; i < 9; i++) gSl[std::string("m") + (char)('0' + i)] = std::max(0, std::min(100, sv[i])) / 100.f; }
        { int q; if (sscanf(ln, "sndmute=%d", &q) == 1) gSndMuted = q != 0; if (!strncmp(ln, "snddev=", 7)) { gSndDevName = ln + 7; while (!gSndDevName.empty() && (gSndDevName.back() == '\n' || gSndDevName.back() == '\r')) gSndDevName.pop_back(); } }
        { int q; if (sscanf(ln, "gfxfancy=%d", &q) == 1) gSt["v0"] = std::max(0, std::min(1, q)); if (sscanf(ln, "guiscale=%d", &q) == 1) gSt["v6"] = std::max(0, std::min(4, q)); if (sscanf(ln, "viewbob=%d", &q) == 1) gSt["v5"] = q != 0; if (sscanf(ln, "cloudmode=%d", &q) == 1) gSt["d0"] = std::max(0, std::min(3, q)); if (sscanf(ln, "maxfps=%d", &q) == 1) gSl["v3"] = std::max(0, std::min(100, q)) / 100.f; if (sscanf(ln, "bright=%d", &q) == 1) gSl["v8"] = std::max(0, std::min(100, q)) / 100.f; }
        { char k[40]; int q; if (sscanf(ln, "opt.%39[^=]=%d", k, &q) == 2) gSt[k] = q; else if (sscanf(ln, "osl.%39[^=]=%d", k, &q) == 2) gSl[k] = std::max(0, std::min(1000, q)) / 1000.f; }
        int v; if (sscanf(ln, "renderdist=%d", &v) == 1) gSl["v1"] = (std::max(2, std::min(32, v)) - 2) / 30.f; }
    fclose(f); sndSyncVol();
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
static std::vector<std::string> modRows() {   // patchMod.9: mods reais de mods/ ("pasta|estado")
    std::vector<std::string> r; for (const std::string& n : modDirNames()) r.push_back(n + "|" + (modIsOff(n) ? "0" : "1")); const int m = gsi("mods.sort", 0);
    if (m == 1) std::sort(r.begin(), r.end()); else if (m == 2) std::sort(r.rbegin(), r.rend());
    const std::string q = gTx["modbusca"]; if (!q.empty()) { std::vector<std::string> o; for (auto& s : r) { std::string a = s, b = q; for (auto& ch : a) ch = (char)tolower(ch); for (auto& ch : b) ch = (char)tolower(ch); if (a.find(b) != std::string::npos) o.push_back(s); } r = o; }
    return r;
}
static std::string lstItem(int st, int i) {
    if (st == 0) return LANGS[i]; if (st == 1) return SHD[i]; if (st == 3) return CTRL[i]; if (st == 4) return snpRows()[i]; if (st == 5) return modRows()[i]; return "";
}
static int lstRow(int st) { static const int r[] = {18, 16, 36, 20, 10, 35, 36, 18, 14}; return r[st]; }
static int lstPad(int st) { static const int r[] = {0, 2, 4, 4, 4, 4, 4, 2, 2}; return r[st]; }
static int lstMax(const Wd& d) { const int tot = d.n * lstRow(d.fmt) + 2 * lstPad(d.fmt) + (d.fmt == 2 ? 30 : 0); return std::max(0, tot - d.h); }
static int lstScr(const Wd& d) { int& s = gSt[d.id + ".scr"]; s = std::max(0, std::min(lstMax(d), s)); return s; }
// ---------------- construção das telas ----------------
static bool gLFirst = true;   // launcher aberto na inicializacao (true) ou de dentro do jogo/opcoes (false: mostra a skin do J2)
struct BrEnt { std::string name; bool dir; }; static std::string gBrDir; static std::vector<BrEnt> gBrEnts;   // navegador de arquivos proprio (escolher skin sem seletor do sistema, ex.: Android)
static void browseScan() {
    gBrEnts.clear(); std::error_code ec;
    for (auto& de : std::filesystem::directory_iterator(std::filesystem::path(gBrDir), std::filesystem::directory_options::skip_permission_denied, ec)) {
        const std::string n = de.path().filename().string(); if (n.empty() || n[0] == '.') continue; std::error_code e2; const bool d = de.is_directory(e2);
        if (!d) { std::string l = n; for (char& c : l) c = (char)tolower((unsigned char)c); if (l.size() < 5 || l.compare(l.size() - 4, 4, ".png") != 0) continue; }
        gBrEnts.push_back({n, d});
    }
    std::sort(gBrEnts.begin(), gBrEnts.end(), [](const BrEnt& a, const BrEnt& b) { if (a.dir != b.dir) return a.dir; std::string x = a.name, y = b.name; for (char& c : x) c = (char)tolower((unsigned char)c); for (char& c : y) c = (char)tolower((unsigned char)c); return x < y; });
    gSt["lbr.scr"] = 0;
}
struct LLay { bool logo; int top, listY, listH, ps, nameY, btnY, tw, L, lw, R, rw; };   // layout do launcher (usado ao montar e ao desenhar)
static LLay launcherLayout(int sw, int sh, bool first) {
    const bool kb = oskActive(); LLay l; l.logo = sh >= 240 && !kb; l.top = l.logo ? 40 : 20; l.listY = l.top + 11;
    l.tw = sw >= 430 ? 400 : 310; l.L = sw / 2 - l.tw / 2; l.lw = (l.tw - 10) * 48 / 100; l.R = l.L + l.lw + 10; l.rw = l.tw - 10 - l.lw;   // lista (48%) + previa
    l.listH = kb ? (first ? 50 : 26) : std::max(50, std::min(128, sh - l.listY - 78 - (first ? 0 : 24)));   // teclado virtual aberto: compacta para o campo de nome ficar acima dele
     l.ps = std::max(1, std::min(3, (l.listH - 26) / 32));
    l.nameY = l.listY + l.listH + 6; l.btnY = l.nameY + 46; return l;
}

static float gPkYaw = 25.f; static Uint32 gPkT = 0;   // patchSkin.1: giro do boneco 3D (graus) e relogio do giro automatico
// patchSkin.4: cabeca 3D (cubo isometrico com a camada de chapeu/overlay levemente maior) para o carrossel de skins
static void uHead3D(float cx, float cy, float u, const uint8_t* d, float alpha = 1.f) {
    glDisable(GL_TEXTURE_2D);
    const float KX = .866f, KY = .5f;
    auto P = [&](float X, float Y, float Z, float& sx, float& sy) { sx = OX + (cx + (X - Z) * KX * u) * G; sy = OY + (cy + (X + Z) * KY * u - Y * u) * G; };
    glBegin(GL_QUADS);
    for (int layer = 0; layer < 2; layer++) {
        const float e = layer ? .55f : 0.f, lo = -e, hi = 8.f + e, sz = (hi - lo) / 8.f;
        const uint8_t* L0 = d + layer * 768;
        for (int face = 0; face < 3; face++) {   // 0 frente (Z+), 1 topo (Y+), 2 lado (X+)
            const float sh = face == 1 ? 1.f : face == 0 ? .82f : .62f; const uint8_t* t = L0 + face * 256;
            for (int j = 0; j < 8; j++) for (int i = 0; i < 8; i++) {
                const uint8_t* c = &t[(j * 8 + i) * 4]; if (c[3] < 128) continue;
                float a0 = lo + i * sz, a1 = a0 + sz, b0 = lo + j * sz, b1 = b0 + sz, v[4][3];
                if (face == 0) { const float y1 = hi - j * sz, y0 = y1 - sz; const float q[4][3] = {{a0, y1, hi}, {a1, y1, hi}, {a1, y0, hi}, {a0, y0, hi}}; memcpy(v, q, sizeof q); }
                else if (face == 1) { const float z0 = lo + j * sz, z1 = z0 + sz; const float q[4][3] = {{a0, hi, z0}, {a1, hi, z0}, {a1, hi, z1}, {a0, hi, z1}}; memcpy(v, q, sizeof q); }
                else { const float y1 = hi - j * sz, y0 = y1 - sz, z1 = hi - i * sz, z0 = z1 - sz; const float q[4][3] = {{hi, y1, z1}, {hi, y1, z0}, {hi, y0, z0}, {hi, y0, z1}}; memcpy(v, q, sizeof q); }
                (void)b0; (void)b1;
                glColor4f(c[0] / 255.f * sh, c[1] / 255.f * sh, c[2] / 255.f * sh, alpha);
                for (int k = 0; k < 4; k++) { float sx, sy; P(v[k][0], v[k][1], v[k][2], sx, sy); glVertex2f(sx, sy); }
            }
        }
    }
    glEnd(); glEnable(GL_TEXTURE_2D);
}
struct PkLay { int nVis, x0, carY, carH; };   // patchPE.5: carrossel de skins estilo MCPE
static PkLay peSkinLay(int sw, int sh) { PkLay l; l.nVis = std::max(3, std::min(8, (sw - 70) / 50)); l.x0 = (sw - l.nVis * 48) / 2; l.carH = 44; l.carY = sh - 98; return l; }
static MR peSkinPrev(int sw, int sh) { const PkLay pl = peSkinLay(sw, sh); const int px0 = sw * 44 / 100; return {px0, 26, sw - 8 - px0, pl.carY - 10 - 26}; }   // painel do boneco 3D
static MR peSkinDropZone(int sw, int sh) { const PkLay pl = peSkinLay(sw, sh); const MR pv = peSkinPrev(sw, sh); const int fw = std::max(80, std::min(240, pv.x - 8 - 8 - 2 * 10)); const int y0 = 78 + (splitOn() ? 52 : 0), y1 = pv.y + pv.h - 8; return {18, y0, fw, std::max(36, y1 - y0)}; }   // area de soltar skin (substitui os textos)
static void pageBuild(const std::string& pg, int sw, int sh, std::vector<Wd>& v) {
    const int cx = sw / 2, L = cx - 155, R = cx + 5, h6 = sh / 6; PB p{v};
    auto r = [&](int i) { return h6 - 12 + 21 * i; };
    auto TG = [&](int x, int y, const char* id, const char* lb, int on) { p.tog(x, y, 150, id, lb, on); };
    if (pg == "peset") {   // patchCfg.1: Configuracoes redesenhadas — 5 categorias, secoes com titulo, so opcoes que tem efeito no jogo
        const int sel = std::max(0, std::min(4, gsi("ps.sel", 1))), rx = 136, rw = sw - rx - 8, vt = 28, vb = sh - 8, cw = (rw - 4) / 2;
        static const char* const nm[5] = {"Geral", "Vídeo", "Ambiente", "Áudio", "Controles"};
        p.t(8, 14, "Configurações", 0xFFFFFF, 0); p.t(rx + rw / 2, 14, nm[sel], 0xFFFFFF, 1);
        for (int i = 0; i < 5; i++) p.b(8, vt + i * 24, 112, std::string("ps.") + (char)('0' + i), nm[i], "", "", 0, i == sel);   // selecionada = destacada
        p.done(8, sh - 28, 112);
        const int scr = std::max(0, std::min(gsi("ps.max", 0), gsi("ps.scr", 0))); gSt["ps.scr"] = scr; int y = vt - scr, k = 0;
        auto H = [&](const char* t) { if (k % 2) { y += 24; k = 0; } if (y > vt - scr) y += 6; p.t(rx, y + 2, t, 0xFFFF55, 0); y += 15; };   // titulo de secao
        auto W = [&](int cs = 1) { const int x = cs == 2 ? rx : rx + (k % 2) * (cw + 4); return x; };
        auto bt = [&](const char* id, const char* lb, const char* op, int def) { if (k % 2 == 0 || false) {} p.b(W(), y, cw, id, lb, "", op, def); if (++k % 2 == 0) y += 24; };
        auto sl = [&](const char* id, const char* lb, float def, int fmt, const char* op = "") { p.s(W(), y, cw, id, lb, def, fmt, op); if (++k % 2 == 0) y += 24; };
        auto gob = [&](const char* id, const char* lb, const char* go) { if (k % 2) { y += 24; k = 0; } p.b(rx, y, std::min(rw, 2 * cw + 4), id, lb, go); y += 24; };
        auto tx = [&](const char* t) { if (k % 2) { y += 24; k = 0; } p.t(rx, y, t, 0x909090, 0); y += 12; };
        if (sel == 0) {
            H("Perfil"); gob("ps.skins", "Skins e nome do jogador...", "peskin");
            H("Idioma"); gob("ps.lang", "Idioma...", "idioma");
        } else if (sel == 1) {
            H("Imagem"); sl("fov", "Campo de Visão", .5f, 2); sl("v1", "Distância de renderização", RD_DEF, 3);
            sl("v8", "Brilho", 1.f, 5); sl("v3", "FPS máximo", 1.f, 4);
            bt("v6", "Interface", "Automática|Pequena (1x)|Normal (2x)|Grande (3x)|Enorme (4x)", 0);
            H("Qualidade"); bt("v0", "Gráficos", "Rápidos|Bonitos", 0); bt("v2", "Iluminação suave", "OFF|Mínima|Máxima", 2);
            bt("v5", "Movimento da câmera", "OFF|ON", 1);
            H("Sombras");
            gob("ps.shd", "Sombras e shaders...", "shaders");
#ifdef __ANDROID__
            H("Plataforma"); bt("gfxapi", "Renderizador", "Vulkan|OpenGL", 0); tx("Renderizador: reinicie o jogo para aplicar.");
#endif
        } else if (sel == 2) {
            H("Céu"); bt("d4", "Céu", "OFF|ON", 1); bt("d5", "Estrelas", "OFF|ON", 1); bt("d6", "Sol e Lua", "OFF|ON", 1);
            bt("d0", "Nuvens", "Padrão|Rápidos|Bonitos|OFF", 0); sl("d1", "Altura das nuvens", 0.f, 0, "OFF|25%|50%|75%|100%");
            H("Clima e névoa"); bt("d3", "Chuva e neve", "Padrão|Rápidos|Bonitos|OFF", 0); bt("d8", "Névoa", "Rápidos|Bonitos|OFF", 1);
            H("Terreno"); bt("d2", "Árvores", "Padrão|Rápidos|Bonitos", 0); bt("d18", "Culling de folhas", "OFF|ON", 1); bt("an.0", "Água animada", "OFF|ON", 1);
        } else if (sel == 3) {
            H("Volume"); p.s(rx, y, std::min(rw, 2 * cw + 4), "m0", "Volume principal", 1.f, 0); y += 24;
            H("Categorias");
            { static const char* const lb[8] = {"Música", "Blocos musicais", "Clima", "Blocos", "Criaturas hostis", "Criaturas amigáveis", "Jogadores", "Ambiente"};
              for (int i = 0; i < 8; i++) { const std::string id = std::string("m") + (char)('1' + i); sl(id.c_str(), lb[i], 1.f, 0); } }
            H("Dispositivo"); gob("m9", gSndDevName.empty() ? "Padrão do sistema" : gSndDevName.substr(0, 30).c_str(), "");
            bt("m10", gSndMuted ? "Som: Mudo" : "Som: Ligado", "", 0); bt("m11", "Atualizar sons", "", 0);
        } else {
            H("Entrada"); gob("ps.keys", "Teclado e mouse...", "controles"); gob("ps.gp", "Controle (gamepad)...", "gamepad");
            H("Toque"); gSt["ps.touch"] = touchHud ? 1 : 0; p.tog(rx, y, std::min(rw, 2 * cw + 4), "ps.touch", "HUD touch (F8)", touchHud ? 1 : 0); y += 24;
            tx("Direcional, botões grandes, corações no topo.");
        }
        if (k % 2) y += 24;
        gSt["ps.max"] = std::max(0, (y + scr) - vt - (vb - vt));
    } else if (pg == "peskin") {   // patchPE.5: seletor de skins estilo MCPE (previa 3D grande, carrossel de miniaturas, botoes embaixo)
        const PkLay pl = peSkinLay(sw, sh); const MR pv = peSkinPrev(sw, sh); const int lx = 18, fw = std::max(80, std::min(240, pv.x - 8 - 8 - 2 * 10)), bw = std::min(sw - 24, 260);
        p.t(lx, 38, "Nome do jogador", 0xA0A0A0, 0); p.f(lx, 49, fw - 30, "lname"); p.b(lx + fw - 26, 49, 26, "l.kb", "Aa");
        p.b(pl.x0 - 22, pl.carY + 12, 20, "pk.prev", "<"); p.b(pl.x0 + pl.nVis * 48 + 2, pl.carY + 12, 20, "pk.next", ">");
        if (splitOn()) {   // jogadores extras (tela dividida, J2..J4): nome + skin de cada um
            const int ne = std::max(1, std::min(3, gSplitN - 1)), pitch = ne == 1 ? 46 : ne == 2 ? 44 : 40;
            for (int k = 0; k < ne; k++) {
                const int hy = 74 + k * pitch; const std::string nk = nickSanitize(gTx[xTxKey(k)]); bool dup = !nk.empty() && nickEq(nk, nickSanitize(gTx["lname"]));
                for (int j = 0; j < ne && !dup; j++) if (j != k && !nk.empty() && nickEq(nk, nickSanitize(gTx[xTxKey(j)]))) dup = true;
                p.t(lx, hy + 6, "J" + std::to_string(k + 2), dup ? 0xFF5555 : 0xA0A0A0, 0); p.f(lx + 18, hy, fw - 18, xTxKey(k));
                std::string sk = gXSkin[k].empty() ? std::string("Padrão") : gXSkin[k]; if (sk.size() > 4 && sk.compare(sk.size() - 4, 4, ".png") == 0) sk.resize(sk.size() - 4);
                p.b(lx, hy + 22, 20, "l.xp" + std::to_string(k), "<"); p.b(lx + 22, hy + 22, fw - 44, "l.xs" + std::to_string(k), sk); p.b(lx + fw - 20, hy + 22, 20, "l.xn" + std::to_string(k), ">");
            }
        }
        p.b(cx - bw / 2, sh - 26, bw, "l.play", "Concluído");
    } else if (pg == "skinfile") {   // navegador de arquivos: pastas + PNGs
        p.t(cx, 6, "Escolher skin (PNG)");
        std::string dd = gBrDir; bool cut = false; while (dd.size() > 4 && uWidth(dd.c_str()) > sw - 30) { dd.erase(0, 1); cut = true; } p.t(cx, 18, (cut ? "..." : "") + dd, 0xA0A0A0);
        p.l(10, 30, sw - 20, std::max(40, sh - 106), "lbr", 8, (int)gBrEnts.size(), 0);
        if (gBrEnts.empty()) p.t(cx, 40, "Nada aqui (pasta vazia ou sem permissão)", 0x808080);
        const bool m = !gLMsg.empty() && SDL_GetTicks() - gLMsgT < 4000; if (m) p.t(cx, sh - 72, gLMsg, 0xFFFF55);
        p.b(cx - 155, sh - 58, 100, "br.up", "Subir pasta"); p.b(cx - 50, sh - 58, 100, "br.dl", "Downloads"); 
#ifdef __ANDROID__
        p.b(cx + 55, sh - 58, 100, "br.home", "Galeria");
#else
        p.b(cx + 55, sh - 58, 100, "br.home", "Início");
#endif
        p.b(cx - 100, sh - 30, 200, "done", "Cancelar", "<");
    } else if (pg == "gamepad") {
        p.t(cx, 6, "Controle (Gamepad)"); p.t(cx, 17, gpCount() ? (std::to_string(gpCount()) + " controle(s) conectado(s)").c_str() : "Nenhum controle conectado", gpCount() ? 0x55FF55 : 0xFF5555);
        for (int i = 0; i < GA_N; i++) p.b(i % 2 ? R : L, h6 - 4 + 22 * (i / 2), 150, "g" + std::to_string(i), std::string(GA_NAME[i]) + ": " + (gGpBind == i ? "> ? <" : GB_NAME[gGpMap[i]]));
        p.b(R, h6 - 4 + 22 * 6, 150, "g.reset", "Padrão"); p.b(L, h6 - 4 + 22 * 7, 150, "gpsens", "Sensibilidade", "", "50%|75%|100%|150%|200%", 2);
        p.t(cx, h6 + 22 * 7 + 22, "Clique numa ação e aperte o botão novo", 0x808080); p.done(cx - 100, sh - 27, 200);
    } else if (pg == "shaders") {
        p.t(cx, 15, "Shaders"); p.l(10, 30, 296, 176, "shd", 1, (int)(sizeof(SHD) / sizeof(*SHD)), 0);
        const char* lb[8] = {"Sombras (F7)", "Qualidade sombra", "Mapa especular", "Qualidade", "Sombra", "Profundidade", "Ilum. pela mão", "Ilum. antiga"};
        const char* op[8] = {"OFF|ON", "Mínima|Leve|Normal", "ON|OFF", "1x|2x|4x", "1x|2x|4x", "1x|2x|4x", "Padrão|Rápida|Bonita", "Padrão|Rápida|Bonita"};
        for (int i = 0; i < 8; i++) p.b(sw - 128, 31 + 20 * i, 118, std::string("sh") + char('0' + i), lb[i], "", op[i], i == 1 ? 2 : 0, i > 1);   // patchOpt.1: sh0 liga/desliga sombras, sh1 = qualidade (gShQ); o resto segue desativado
        p.t(5, 216, std::string("OpenGL: ") + (const char*)glGetString(GL_VERSION) + " " + (const char*)glGetString(GL_RENDERER), 0x808080, 0);
        p.b(6, 232, 124, "sh9", "Pasta de shaders"); p.b(131, 232, 20, "sh10", "v"); p.b(164, 232, 147, "done", "Concluído", "<"); p.b(sw - 128, 232, 118, "sh11", "Shaders...", "", "", 0, true);
    } else if (pg == "controles") {
        p.t(cx, 8, "Controles");
        TG(L, 19, "k0", "Inverter Mouse", 0); p.s(R, 19, 150, "k1", "Sensibilidade", .5f, 6); TG(L, 43, "k2", "Modo Touchscreen", 0);
        p.l(0, 63, sw, sh - 96, "ctl", 3, (int)(sizeof(CTRL) / sizeof(*CTRL)), cx + 140);
        p.b(L, sh - 28, 150, "done", "Concluído", "<"); p.b(R, sh - 28, 150, "k3", "Redefinir Teclas");
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
        if (sh < 240) {   // patchRpi.1: tela pequena (Raspberry 320x210 de interface): duas colunas, tudo acima dos botoes de baixo
            const char* e1[5] = {"Recursos, criar, vida e fome", "Como Sobrevivência, difícil", "Recursos ilimitados e voo", "Sem quebrar/colocar blocos", "Voe e observe sem interagir"};
            p.t(cx, 8, "Criar Novo Mundo"); p.t(L, 24, "Nome do Mundo", 0xA0A0A0, 0); p.f(L, 35, 150, "nome");
            p.t(L, 59, "Salvo em: " + gTx["nome"], 0x808080, 0);
            p.b(R, 33, 150, "mundo.modo", "Modo de Jogo", "", "Sobrevivência|Hardcore|Criativo|Aventura|Espectador", 0);
            p.t(R, 57, e1[m % 5], 0xA0A0A0, 0);
            p.b(R, 78, 150, "cr1", "Mais Opções do Mundo...", "criar2");
            p.b(L, sh - 28, 150, "cr2", "Criar Novo Mundo"); p.b(R, sh - 28, 150, "done", "Cancelar", "<");
            return;
        }
        p.t(cx, 20, "Criar Novo Mundo"); p.t(cx - 100, 47, "Nome do Mundo", 0xA0A0A0, 0); p.f(cx - 100, 59, 200, "nome");
        p.t(cx - 100, 85, "Será salvo em: " + gTx["nome"], 0x808080, 0);
        p.b(cx - 75, 115, 150, "mundo.modo", "Modo de Jogo", "", "Sobrevivência|Hardcore|Criativo|Aventura|Espectador", 0);
        const char* d1[5] = {"Procure por recursos, crie, ganhe", "Igual ao Sobrevivência, mas fica", "Recursos ilimitados, voo livre e", "Explore o mundo, mas não pode", "Voe através de blocos e observe"};
        const char* d2[5] = {"níveis, vida e fome", "bloqueado na dificuldade difícil", "destrua blocos instantaneamente", "quebrar nem colocar blocos", "sem poder interagir com nada"};
        p.t(cx - 100, 137, d1[m % 5], 0xA0A0A0, 0); p.t(cx - 100, 149, d2[m % 5], 0xA0A0A0, 0);
        p.b(cx - 75, 187, 150, "cr1", "Mais Opções do Mundo...", "criar2");
        p.b(L, sh - 28, 150, "cr2", "Criar Novo Mundo"); p.b(R, sh - 28, 150, "done", "Cancelar", "<");
    } else if (pg == "criar2") {
        if (sh < 240) {   // patchRpi.1: versao compacta
            p.t(cx, 8, "Criar Novo Mundo"); p.t(cx - 100, 24, "Seed para o Gerador de Mundos", 0xA0A0A0, 0); p.f(cx - 100, 35, 200, "seed");
            p.t(cx - 100, 58, "Em branco = seed aleatória", 0x808080, 0);
            TG(L, 76, "w0", "Gerar Estruturas", 1); p.b(R, 76, 150, "w1", "Tipo", "", "Padrão|Superplano|Mundo Gigante|Amplificado|Personalizado", 0);
            p.t(L, 99, "Vilas, calabouços, etc", 0x808080, 0);
            TG(L, 114, "w2", "Permitir Cheats", 0); TG(R, 114, "w3", "Baú Bônus", 0);
            p.t(L, 137, "Comandos como /gamemode, /xp", 0x808080, 0);
            p.b(L, sh - 28, 150, "cr2", "Criar Novo Mundo"); p.b(R, sh - 28, 150, "cr3", "Cancelar", "<");
            return;
        }
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
        p.l(10, 32, 136, 140, "mods", 5, (int)modRows().size(), 140); p.t(cx + 80, 40, "Clique num mod para ligar/desligar", 0xA0A0A0, 0); p.t(cx + 80, 52, "Vale ao reiniciar o jogo", 0xA0A0A0, 0);
        p.t(78, 177, "Buscar:", 0xFFFFFF, 1); p.f(11, 188, 133, "modbusca");
        p.b(201, 219, 199, "done", "Concluído", "<");
    } else {   // generico (Realms, acessibilidade, Super Secret Settings, Pacotes de Recursos...)
        p.t(cx, 15, gOtherTitle);
        if (gOtherTitle == "Estatísticas") {
            const long sec = (long)((SDL_GetTicks() - gStT0) / 1000); char b[96]; int y = 44;
            snprintf(b, sizeof b, "Tempo de sessão: %ldh %02ldmin %02lds", sec / 3600, sec / 60 % 60, sec % 60); p.t(cx, y, b, 0xFFFFFF, 0); y += 16;
            snprintf(b, sizeof b, "Blocos quebrados: %ld", gStBroken); p.t(cx, y, b, 0xFFFFFF, 0); y += 16;
            snprintf(b, sizeof b, "Blocos colocados: %ld", gStPlaced); p.t(cx, y, b, 0xFFFFFF, 0); y += 20;
            p.t(cx, y, "Valem desde que o jogo foi aberto", 0xA0A0A0, 0);
        } else if (gOtherTitle == "Conquistas") {
            struct Ach { const char* n; bool ok; };
            const long sec = (long)((SDL_GetTicks() - gStT0) / 1000);
            const Ach A[] = {{"Primeira mineração (quebrar 1 bloco)", gStBroken >= 1}, {"Construtor (colocar 1 bloco)", gStPlaced >= 1}, {"Minerador (quebrar 100 blocos)", gStBroken >= 100},
                {"Arquiteto (colocar 100 blocos)", gStPlaced >= 100}, {"Dedicado (30 min de sessão)", sec >= 1800}};
            int y = 40; for (const Ach& a : A) { p.t(cx, y, std::string(a.ok ? "[x] " : "[ ] ") + a.n, a.ok ? 0x55FF55 : 0xA0A0A0, 0); y += 16; }
        }
        p.done(cx - 100, sh - 27, 200);
    }
}
// ---------------- navegação ----------------
static void launcherCommit() {
    gPlayerName = nickSanitize(gTx["lname"]); gTx["lname"] = gPlayerName;
    if (!gLFirst) for (int k = 0; k < 3; k++) { const std::string n2 = nickSanitize(gTx[xTxKey(k)]); bool ok = !n2.empty() && !nickEq(n2, gPlayerName);
        for (int j = 0; j < 3 && ok; j++) if (j != k && nickValid(gXName[j]) && nickEq(n2, gXName[j])) ok = false;   // nao repete nick de outro jogador local
        if (ok) gXName[k] = n2; gTx[xTxKey(k)] = nickValid(gXName[k]) ? gXName[k] : xDefName(k); }
    profileSave();
}
static void browseHome(bool dl) {
    gBrDir.clear(); std::error_code ec;
#ifdef __ANDROID__
    const char* c[] = {"/sdcard/Pictures", "/storage/emulated/0/Pictures", "/sdcard/DCIM", "/storage/emulated/0/DCIM", "/sdcard/Download", "/storage/emulated/0/Download", "/sdcard", "/storage/emulated/0"};   // abre direto na galeria (Imagens), depois Camera/Downloads
    for (int k = 0; k < 8 && gBrDir.empty(); k++) { const char* q = c[dl ? (k + 4) % 8 : k]; if (std::filesystem::is_directory(q, ec)) gBrDir = q; }   // dl: comeca pelos Downloads
    if (gBrDir.empty()) gBrDir = ".";
#else
    const char* h = getenv("HOME"); if (!h) h = getenv("USERPROFILE"); gBrDir = h ? h : ".";
    if (dl) { const std::filesystem::path q = std::filesystem::path(gBrDir) / "Downloads"; if (std::filesystem::is_directory(q, ec)) gBrDir = q.string(); }
#endif
    browseScan();
}
static void skinListFocus() { const int i = skinIndex(); if (i >= 0) gSt["lsk.scr"] = std::max(0, i * 18 - 36); }
static void launcherOpen() { gLFirst = true; gTx["lname"] = gPlayerName; skinScan(); skinListFocus(); gPStack.clear(); gPage = "launcher"; gScreen = SC_OTHER; gFocus.clear(); gDrag.clear(); }
static void drawSkinPreview(int cx, int top, int s, const uint8_t* skin = ::skin, bool back = false) {   // frente do personagem (cabeca, corpo, bracos, pernas + camadas) com os pixels da skin atual; s = unidades por pixel
    const int x0 = cx - 4 * s, aw = slim ? 3 : 4;
    auto px = [&](int sx, int sy, int dx, int dy, int w, int h, int ox, int oy) {   // regiao w*h da skin em (sx,sy) -> tela (dx,dy); ox/oy: camada extra em (ox,oy)
        for (int pass = 0; pass < 2; pass++) { const int bx = pass ? ox : sx, by = pass ? oy : sy; if (pass && ox < 0) break;
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
                const uint8_t* q = &skin[((by + j) * 64 + bx + i) * 4]; if (q[3] < 128) continue;
                U((float)(dx + i * s), (float)(dy + j * s), (float)s, (float)s, ((unsigned)q[0] << 16) | ((unsigned)q[1] << 8) | q[2]);
            } }
    };
    if (back) {   // costas: braco esquerdo do personagem fica na esquerda da tela
        const int lb = slim ? 43 : 44, ls = slim ? 59 : 60, rb = slim ? 51 : 52;
        px(24, 8, x0, top, 8, 8, 56, 8); px(32, 20, x0, top + 8 * s, 8, 12, 32, 36);
        px(lb, 52, x0 - aw * s, top + 8 * s, aw, 12, ls, 52); px(rb, 20, x0 + 8 * s, top + 8 * s, aw, 12, rb, 36);
        px(28, 52, x0, top + 20 * s, 4, 12, 12, 52); px(12, 20, x0 + 4 * s, top + 20 * s, 4, 12, 12, 36); return;
    }
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
    if (n == "launcher") n = "peskin";   // launcher antigo removido: tudo na tela de skins
    if (n == "launcher" || n == "peskin") { gLFirst = false; gTx["lname"] = gPlayerName; for (int k = 0; k < 3; k++) gTx[xTxKey(k)] = nickValid(gXName[k]) ? gXName[k] : xDefName(k); skinScan(); skinListFocus(); gLMsg.clear(); if (n == "peskin") gSt["pk.off"] = std::max(0, skinIndex() - 2); }
    if (gScreen == SC_TITLE) gScreen = SC_OTHER;
    if (!gPage.empty()) gPStack.push_back(gPage);
    if (n == "editar") { const int sl = gsi("w.sel", -1); if (sl >= 0 && sl < (int)gWorlds.size()) gTx["enome"] = gWorlds[sl].name; }
    gPage = n; gDrag.clear(); gFocus = n == "editar" ? "enome" : n == "criar" ? "nome" : n == "criar2" ? "seed" : n == "direto" ? "dip" : n == "addsrv" ? "sname" : ""; txtKbd();
}
static void pageBack() {
    gDrag.clear(); gFocus.clear(); txtKbd(); gGpBind = -1;
    if (gPage == "launcher" || gPage == "peskin") launcherCommit();
    if (gPage == "conectando") netStop();   // cancelar a conexao
    if (!gPStack.empty() && gPStack.back() == "multi") gHostNext = false;   // voltou do "Hospedar Mundo" para a lista
    if (gPStack.empty()) { gPage.clear(); if (gScreen == SC_OTHER) gScreen = SC_TITLE; }
    else { gPage = gPStack.back(); gPStack.pop_back(); }
}
static void leavePages() { gPage.clear(); gPStack.clear(); gFocus.clear(); gDrag.clear(); }
static void pageAction(const std::string& id) {
    if (id == "ps.touch") { touchHud = !touchHud; touchReset(); return; }   // HUD touch pelas Configuracoes
    if (id == "pk.prev" || id == "pk.next") { gSt["pk.off"] = std::max(0, gsi("pk.off", 0) + (id == "pk.next" ? 3 : -3)); return; }   // carrossel de skins
    if (id.size() == 4 && id.compare(0, 3, "ps.") == 0 && id[3] >= '0' && id[3] <= '4') { gSt["ps.sel"] = id[3] - '0'; gSt["ps.scr"] = 0; return; }   // patchCfg.1   // patchSkin.7: Perfil abre direto o launcher novo (tela de skins)   // categoria das Configuracoes PE
    if (id == "m9") { sndDeviceNext(); optSave(); return; } if (id == "m10") { gSndMuted = !gSndMuted; optSave(); return; } if (id == "m11") { sndRefresh(); return; }   // patchAud.1
    if (id.size() > 2 && id[0] == 'l' && id[1] == '.') {   // launcher
        if (id == "l.play") { launcherCommit(); const bool first = gLFirst; if (first) { leavePages(); if (gScreen == SC_OTHER) gScreen = SC_TITLE; } else pageBack(); }
        else if (id == "l.file") { if (skinPickFile() < 0) { if (gBrDir.empty()) browseHome(true); else browseScan(); gLMsg.clear(); pageGo("skinfile"); } }
        else if (id.size() == 5 && (id.compare(0, 4, "l.xp") == 0 || id.compare(0, 4, "l.xn") == 0) && id[4] >= '0' && id[4] <= '2') skinXStep(id[4] - '0', id[3] == 'p' ? -1 : 1);
        else if (id == "l.kb") { gOskForce = !gOskForce; gFocus = "lname"; }
        return;
    }
    if (id == "br.up") { std::string q = std::filesystem::path(gBrDir).parent_path().string(); if (q.empty()) q = "/"; gBrDir = q; browseScan(); return; }
    if (id == "br.dl" || id == "br.home") { browseHome(id == "br.dl"); return; }
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
    } else if (mtDirt) mBg(w, h, 0, 0, (float)w, (float)h, gPage == "peskin" ? .34f : .25f);   // patchSkin.6: Configuracoes tambem com fundo de terra
    const int peRx = 136;   // patchPE.4
    std::vector<Wd> v; pageBuild(gPage, sw, sh, v);
    if (gPage == "peskin") {
        const PkLay pl = peSkinLay(sw, sh); const MR pv = peSkinPrev(sw, sh); const int n = (int)gSkinList.size(), cur = skinIndex();
        if (gSkinFocusPending && cur >= 0) { gSt["pk.off"] = std::max(0, cur - pl.nVis / 2); gSkinFocusPending = false; }
        const int off = std::max(0, std::min(std::max(0, n - pl.nVis), gsi("pk.off", 0))); gSt["pk.off"] = off;
        const Uint32 tn = SDL_GetTicks(); const float dtp = gPkT ? std::min(.1f, (tn - gPkT) / 1000.f) : 0.f; gPkT = tn; if (gDrag != "pkspin") gPkYaw += dtp * 24.f;   // giro automatico (para enquanto arrasta)
        U(0, 0, (float)sw, 22.f, 0x000000, .55f); U(0, 22.f, (float)sw, 1.f, 0x000000, .8f); U(0, 23.f, (float)sw, 1.f, 0xFFFFFF, .08f);   // cabecalho
        U(0, (float)(pl.carY - 8), (float)sw, (float)(sh - pl.carY + 8), 0x000000, .55f); U(0, (float)(pl.carY - 9), (float)sw, 1.f, 0xFFFFFF, .08f);   // rodape
        uCenter(cx, 8, "Skins", 0xFFFFFF);
        const int lpw = pv.x - 8 - 8;   // painel esquerdo (perfil)
        U(8.f, (float)pv.y, (float)lpw, (float)pv.h, 0x000000, .45f); skFrame(7.f, (float)(pv.y - 1), (float)(lpw + 2), (float)(pv.h + 2), 0x000000, 1.f); skFrame(8.f, (float)pv.y, (float)lpw, (float)pv.h, 0xFFFFFF, 1.f, .12f); U(9.f, (float)pv.y + 1.f, (float)lpw - 2.f, 1.f, 0xFFFFFF, .10f); U(9.f, (float)(pv.y + pv.h - 2), (float)lpw - 2.f, 1.f, 0x000000, .35f);
        U((float)pv.x, (float)pv.y, (float)pv.w, (float)pv.h, 0x000000, .45f);   // painel do boneco
        {   // area de soltar skin: borda tracejada, destaque ao passar/arrastar, aviso de resultado
            const MR dz = peSkinDropZone(sw, sh); const bool hv = inR(dz, ux, uy), dr = gSkinDropHover; const Uint32 now = SDL_GetTicks(); const bool fb = gSkinDropT && now - gSkinDropT < 3000;
            const unsigned bc = fb ? (gSkinDropOk ? 0x55FF55 : 0xFF5555) : (dr ? 0xFFFF55 : 0xFFFFFF); const float ba = (fb || dr) ? .9f : (hv ? .7f : .3f);
            U((float)dz.x, (float)dz.y, (float)dz.w, (float)dz.h, 0x000000, hv || dr ? .55f : .35f); U((float)dz.x, (float)dz.y, (float)dz.w, 1.f, 0xFFFFFF, .06f);
            for (int xx = dz.x; xx < dz.x + dz.w; xx += 6) { const float l = (float)std::min(3, dz.x + dz.w - xx); U((float)xx, (float)dz.y, l, 1.f, bc, ba); U((float)xx, (float)(dz.y + dz.h - 1), l, 1.f, bc, ba); }
            for (int yy = dz.y; yy < dz.y + dz.h; yy += 6) { const float l = (float)std::min(3, dz.y + dz.h - yy); U((float)dz.x, (float)yy, 1.f, l, bc, ba); U((float)(dz.x + dz.w - 1), (float)yy, 1.f, l, bc, ba); }
            const int zcx = dz.x + dz.w / 2, zcy = dz.y + dz.h / 2; const float bob = dr ? sinf(now / 120.f) * 2.f : 0.f;
            if (dz.h >= 50) { const float ay = zcy - 16 + bob; U(zcx - 1.f, ay, 3.f, 10.f, bc, .9f); U(zcx - 4.f, ay + 7.f, 9.f, 1.f, bc, .9f); U(zcx - 3.f, ay + 8.f, 7.f, 1.f, bc, .9f); U(zcx - 2.f, ay + 9.f, 5.f, 1.f, bc, .9f); U(zcx - 1.f, ay + 10.f, 3.f, 1.f, bc, .9f); U(zcx - 7.f, ay + 13.f, 15.f, 1.f, bc, .9f); }
            const char* l1 = fb ? (gSkinDropOk ? "Skin carregada!" : "PNG inválido (64x64 ou 64x32)") : (dr ? "Solte aqui" : "Arraste uma skin (.png)"); 
#ifdef __ANDROID__
            const char* l2 = fb ? "" : "ou toque para abrir a galeria";
#else
            const char* l2 = fb ? "" : "ou clique para escolher";
#endif
            uCenter(zcx, (float)(zcy + (dz.h >= 50 ? 2 : -8)), l1, fb ? bc : 0xFFFFFF); if (*l2 && dz.h >= 40) uCenter(zcx, (float)(zcy + (dz.h >= 50 ? 14 : 2)), l2, 0x909090);
        }
        { const bool hv = inR(pv, ux, uy) || gDrag == "pkspin"; skFrame((float)(pv.x - 1), (float)(pv.y - 1), (float)(pv.w + 2), (float)(pv.h + 2), 0x000000, 1.f); skFrame((float)pv.x, (float)pv.y, (float)pv.w, (float)pv.h, 0xFFFFFF, 1.f, hv ? .45f : .12f); }
        const float fx = pv.x + pv.w * .5f, fy = (float)(pv.y + pv.h - 14), scU = std::max(16.f, std::min(120.f, (pv.h - 14 - 14 - 22) / 1.9f));
        drawPlayerPreviewSpin((float)G, fx, fy, scU, gPkYaw, (float)pv.x, (float)pv.y, (float)pv.w, (float)pv.h);
        glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_TEXTURE_2D);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glColor4f(1, 1, 1, 1);
        if (cur >= 0) { std::string nm = gSkinList[cur]; if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".png") == 0) nm.resize(nm.size() - 4); if (nm == "default") nm = "Padrão"; uCenter((int)fx, (float)(pv.y + 8), nm.c_str(), 0xFFFFFF); }
        else uCenter((int)fx, (float)(pv.y + 8), "(personalizada)", 0xFFFFFF);
        U((float)(pl.x0 - 1), (float)(pl.carY - 1), (float)(pl.nVis * 48 + 2), (float)(pl.carH + 2), 0x0A0A0A);
        for (int gy = 0; gy < pl.carH; gy += 2) { const float t = (float)gy / pl.carH; U((float)pl.x0, (float)(pl.carY + gy), (float)(pl.nVis * 48), 2.f, ((unsigned)(26 + 10 * (1 - t)) << 16) | ((unsigned)(30 + 12 * (1 - t)) << 8) | (unsigned)(48 + 20 * (1 - t))); }   // fundo em degrade azul-noite
        int hov = -1;
        for (int i = 0; i < pl.nVis && off + i < n; i++) {
            const int idx = off + i, x = pl.x0 + i * 48;
            if (idx == cur) { U((float)x, (float)pl.carY, 48.f, (float)pl.carH, 0xFFFFFF); for (int gy = 0; gy < pl.carH - 2; gy += 2) { const float t = (float)gy / (pl.carH - 2); U(x + 1.f, pl.carY + 1.f + gy, 46.f, 2.f, ((unsigned)(58 + 34 * (1 - t)) << 16) | ((unsigned)(58 + 34 * (1 - t)) << 8) | (unsigned)(58 + 34 * (1 - t))); } }
            else if (idx > off) U((float)x, pl.carY + 4.f, 1.f, pl.carH - 8.f, 0x000000, .35f);   // divisoria sutil
            else if (inR({x, pl.carY, 48, pl.carH}, ux, uy)) { U((float)x, (float)pl.carY, 48.f, (float)pl.carH, 0xFFFFFF, .15f); }
            if (inR({x, pl.carY, 48, pl.carH}, ux, uy)) hov = idx;
            if (idx < (int)gSkinHead3.size()) { U(x + 9.f, pl.carY + pl.carH - 5.f, 30.f, 3.f, 0x000000, .35f); uHead3D(x + 24.f, pl.carY + 21.f, idx == cur ? 1.95f : 1.75f, gSkinHead3[idx].data()); }   // sombra + cabeca 3D com overlay
        }
        if (hov >= 0) { std::string nm = gSkinList[hov]; if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".png") == 0) nm.resize(nm.size() - 4); if (nm == "default") nm = "Padrão"; const int tw = uWidth(nm.c_str()), tx = std::max(2, std::min(sw - tw - 6, ux - tw / 2));
            U((float)(tx - 2), (float)(pl.carY - 16), (float)(tw + 6), 12.f, 0x000000, .85f); uText((float)(tx + 1), (float)(pl.carY - 14), nm.c_str(), 0xFFFFFF, true); }   // dica com o nome da skin sob o mouse
    }
    if (gPage == "peset") {   // patchCfg.1: painel da barra lateral + area de conteudo + cabecalho
        glDisable(GL_SCISSOR_TEST);
        U(0.f, 0.f, (float)peRx - 4.f, (float)sh, 0x000000, .45f); U((float)peRx - 5.f, 0.f, 1.f, (float)sh, 0xFFFFFF, .14f);
        U((float)peRx - 4.f, 0.f, (float)(sw - peRx + 4), 24.f, 0x000000, .40f); U((float)peRx - 4.f, 23.f, (float)(sw - peRx + 4), 1.f, 0xFFFFFF, .14f);
        U((float)peRx - 4.f, 24.f, (float)(sw - peRx + 4), (float)(sh - 24), 0x000000, .18f);
    }
    for (const Wd& d : v) {
        if (gPage == "peset") { if (d.x >= peRx && !(d.k == WK_TXT && d.y == 14)) { glEnable(GL_SCISSOR_TEST); glScissor(peRx * G, h - (sh - 8) * G, (sw - peRx) * G, (sh - 8 - 28) * G); } else glDisable(GL_SCISSOR_TEST); }   // rolagem da lista
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
                } else if (st == 7) {   // skins do launcher: miniatura do rosto + nome
                    if (i >= (int)gSkinList.size() || i >= (int)gSkinHead.size()) continue;
                    const bool on = gSkinList[i] == gSkinFile; const MR rr = {d.x, ry, d.w, rh};
                    if (on) { U(d.x + 1.f, (float)ry, d.w - 2.f, rh - 1.f, 0xFFFFFF); U(d.x + 2.f, ry + 1.f, d.w - 4.f, rh - 3.f, 0x000000); } else if (inR(rr, ux, uy)) U(d.x + 1.f, (float)ry, d.w - 2.f, rh - 1.f, 0xFFFFFF, .15f);
                    for (int q = 0; q < 64; q++) { const uint8_t* c = &gSkinHead[i][q * 4]; if (c[3] < 128) continue; U(d.x + 5.f + (q % 8) * 2, ry + 1.f + (q / 8) * 2, 2, 2, ((unsigned)c[0] << 16) | ((unsigned)c[1] << 8) | c[2]); }
                    std::string nm = gSkinList[i]; if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".png") == 0) nm.resize(nm.size() - 4); if (nm == "default") nm = "Padrão";
                    while (nm.size() > 3 && uWidth(nm.c_str()) > d.w - 36) nm.pop_back();
                    uText(d.x + 25.f, (float)(ry + 5), nm.c_str(), on ? 0xFFFF55 : 0xFFFFFF, true);
                    { const int tot = d.n * rh + 2 * pad; if (tot > d.h && i == std::max(0, (sc - pad) / rh)) { const float bh = std::max(10.f, (float)d.h * d.h / tot); U(d.x + d.w - 4.f, d.y + (d.h - bh) * sc / (float)(tot - d.h), 3.f, bh, 0xA0A0A0); } }
                } else if (st == 8) {   // navegador de arquivos
                    if (i >= (int)gBrEnts.size()) continue; const BrEnt& be = gBrEnts[i];
                    if (inR({d.x, ry, d.w, rh}, ux, uy)) U(d.x + 1.f, (float)ry, d.w - 2.f, rh - 1.f, 0xFFFFFF, .15f);
                    std::string nm = (be.dir ? "> " : "") + be.name; while (nm.size() > 3 && uWidth(nm.c_str()) > d.w - 10) nm.pop_back();
                    uText(d.x + 4.f, (float)(ry + 3), nm.c_str(), be.dir ? 0xFFFF55 : 0xFFFFFF, true);
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
                    const std::string mn = it.substr(0, q); const bool off = it[q + 1] == '0', chg = (gModsOff0.count(mn) > 0) != off;
                    uText(d.x + 3.f, (float)(ry + 2), mn.c_str(), off ? 0x808080 : 0xFFFFFF, true); uText(d.x + 3.f, (float)(ry + 12), off ? "Desligado" : "Ligado", off ? 0xFF5555 : 0x55FF55, true); if (chg) uText(d.x + 3.f, (float)(ry + 22), "Reinicie o jogo", 0xFFFF55, true);
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
// OSK_H: em chat_debug.inc (altura do teclado virtual (unidades de GUI)
struct OKey { std::string lb; int kind; int x, y, w; };   // kind: 0 caractere, 1 maiuscula, 2 apagar, 3 espaco, 4 OK
static int gOskSel = 0, gOskSW = 427, gOskSH = 240;
static bool oskActive() { return ((chatOpen && (gChatPad || gOskForce)) || !gFocus.empty()) && (gOskForce || gpCount() > 0); }   // campo de texto focado OU chat aberto
static std::string gTxSelFocus;   // patchRpiOpt.7: Ctrl+A nos campos de texto das telas (campo selecionado: proximo texto/Backspace substitui tudo)
static bool txSel() { return !gFocus.empty() && gTxSelFocus == gFocus; }
static void txSelEat() { if (txSel()) { gTx[gFocus].clear(); gTxSelFocus.clear(); } }
static std::string& oskText() { return chatOpen ? chatIn : gTx[gFocus]; }
static std::vector<OKey> oskKeys(int sw, int sh) {
    std::vector<OKey> k; const int cx = sw / 2, py = sh - OSK_H;
    static const std::vector<std::string> R[5] = {{"1","2","3","4","5","6","7","8","9","0"}, {"q","w","e","r","t","y","u","i","o","p"}, {"a","s","d","f","g","h","j","k","l"}, {"z","x","c","v","b","n","m"},
        {"\xC3\xA1","\xC3\xA9","\xC3\xAD","\xC3\xB3","\xC3\xBA","\xC3\xA3","\xC3\xB5","\xC3\xA7","\xC3\xAA","\xC3\xB4","!","?"}};   // ultima fileira: acentos do portugues (a e i o u ~a ~o c ^e ^o)
    for (int r = 0; r < 5; r++) { const int n = (int)R[r].size(), x0 = cx - (n * 20 - 2) / 2;
        for (int i = 0; i < n; i++) { std::string t = R[r][i];
            if (gOskCaps && r > 0) { if (t.size() == 1) t[0] = (char)toupper((unsigned char)t[0]); else if (t.size() == 2 && (unsigned char)t[0] == 0xC3) t[1] = (char)((unsigned char)t[1] - 0x20); }   // letras e acentos viram maiusculas (U+00E0..FE -> U+00C0..DE)
            k.push_back({t, 0, x0 + i * 20, py + 24 + 20 * r, 18}); } }
    struct B { const char* lb; int kind, w; }; static const B bt[8] = {{"Aa", 1, 24}, {"_", 0, 16}, {"-", 0, 16}, {"Espaço", 3, 60}, {".", 0, 16}, {"/", 0, 16}, {"<-", 2, 24}, {"OK", 4, 24}};
    int x = cx - 108; for (const B& b : bt) { k.push_back({b.lb, b.kind, x, py + 24 + 100, b.w}); x += b.w + 2; }
    return k;
}
static int oskHit(int ux, int uy) { const std::vector<OKey> k = oskKeys(gOskSW, gOskSH); for (int i = 0; i < (int)k.size(); i++) if (ux >= k[i].x && ux < k[i].x + k[i].w && uy >= k[i].y && uy < k[i].y + 18) return i; return -1; }
static void oskNav(int dx, int dy) {
    const std::vector<OKey> k = oskKeys(gOskSW, gOskSH); const int n = (int)k.size(); gOskSel = std::max(0, std::min(n - 1, gOskSel)); const OKey c = k[gOskSel];
    if (dx) { const int j = gOskSel + dx; if (j >= 0 && j < n && k[j].y == c.y) gOskSel = j; else { int f = gOskSel; while (f - dx >= 0 && f - dx < n && k[f - dx].y == c.y) f -= dx; gOskSel = f; } }   // na ponta da fileira volta para o outro lado
    if (dy) { int ty = c.y + dy * 20; const int top = k[0].y, bot = k.back().y; if (ty < top) ty = bot; if (ty > bot) ty = top;
        int best = -1, bd = 1 << 30; for (int i = 0; i < n; i++) if (k[i].y == ty) { const int d = abs((k[i].x + k[i].w / 2) - (c.x + c.w / 2)); if (d < bd) { bd = d; best = i; } } if (best >= 0) gOskSel = best; }
}
static void oskPut(const std::string& t) { if (chatOpen) gpPushText(t.c_str()); else if (txSelEat(), gTx[gFocus].size() < 32) gTx[gFocus] += t; }   // chat: vira texto digitado (chatText)
static void oskBack() { if (chatOpen) { gpPushKey(SDLK_BACKSPACE); return; } if (txSel()) { txSelEat(); return; } std::string& s = gTx[gFocus]; while (!s.empty() && (s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); }
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
    const int hk = oskHit(mx / g, my / g); if (hk >= 0) { gOskSel = hk; oskPress(); return true; } return my / g >= sh - OSK_H;
}
static void oskDraw(int sw, int sh, int ux, int uy) {
    gOskSW = sw; gOskSH = sh; const int py = sh - OSK_H; const std::vector<OKey> k = oskKeys(sw, sh); gOskSel = std::max(0, std::min((int)k.size() - 1, gOskSel));
    U(0, (float)py, (float)sw, (float)(sh - py), 0x000000, .88f);
    mField(sw / 2 - 100, py + 2, 200, oskText(), true);   // campo do texto no topo do teclado
    for (int i = 0; i < (int)k.size(); i++) { const OKey& c = k[i]; mBtn({c.x, c.y, c.w, 18}, c.lb.c_str(), true, ux, uy);
        if (i == gOskSel) { U(c.x - 1.f, c.y - 1.f, c.w + 2.f, 1, 0xFFFF55); U(c.x - 1.f, c.y + 18.f, c.w + 2.f, 1, 0xFFFF55); U(c.x - 1.f, (float)c.y, 1, 18, 0xFFFF55); U(c.x + c.w * 1.f, (float)c.y, 1, 18, 0xFFFF55); } }
    if (gpCount()) uCenter(sw / 2, (float)(sh - 9), sw >= 330 ? "Direcional: tecla | A: digitar | B: apagar | X: Aa | Y: espaço | Start: OK" : "A digitar | B apagar | X Aa | Y espaço | Start OK", 0x808080);   // cabe no viewport do split
}
// ---------------- eventos ----------------
static bool pageEvent(const SDL_Event& e, int w, int h) {
    if (gPage.empty()) return false;
    G = guiScaleFor(w, h); const int sw = (w + G - 1) / G, sh = (h + G - 1) / G;
    std::vector<Wd> v; pageBuild(gPage, sw, sh, v);
    if (e.type == SDL_MOUSEMOTION) {
        gMX = e.motion.x; gMY = e.motion.y;
        if (gPage == "peset" && gSt["ps.pend"]) { if (!(e.motion.state & 1u)) gSt["ps.pend"] = 0; else { const int dy = (gMY - gSt["ps.ly"]) / G; if (dy) { gSt["ps.scr"] = std::max(0, std::min(gsi("ps.max", 0), gsi("ps.scr", 0) - dy)); gSt["ps.ly"] += dy * G; gSt["ps.acc"] += abs(dy); } } }   // arrastar = rolar
        if (gDrag == "pkspin") { if (e.motion.state & 1u) gPkYaw += e.motion.xrel * .8f / (float)G; else gDrag.clear(); return true; }
        if (!gDrag.empty()) for (const Wd& d : v) if (d.k == WK_SLD && d.id == gDrag) { sldSet(d, gMX / G); gfxTogglesSync(); sndSyncVol(); }
        return true;
    }
    const int ux = gMX / G, uy = gMY / G;
    if (e.type == SDL_KEYDOWN) {
        const SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_ESCAPE) { if (oskActive()) gFocus.clear(); else pageBack(); }
        else if (gPage == "launcher" && gFocus.empty() && (k == SDLK_UP || k == SDLK_DOWN)) { skinStep(k == SDLK_UP ? -1 : 1); gSkinFocusPending = true; }
        else if (!gFocus.empty() && k == SDLK_a && (SDL_GetModState() & KMOD_CTRL)) gTxSelFocus = gFocus;   // Ctrl+A
        else if (!gFocus.empty() && k == SDLK_BACKSPACE && txSel()) txSelEat();
        else if (!gFocus.empty() && k == SDLK_BACKSPACE) { std::string& s = gTx[gFocus]; while (!s.empty() && (s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) gFocus.clear();
        return true;
    }
    if (e.type == SDL_TEXTINPUT) { if (!gFocus.empty() && e.text.text[0] >= 32) txSelEat(); if (!gFocus.empty() && gTx[gFocus].size() < 32 && e.text.text[0] >= 32) gTx[gFocus] += e.text.text; return true; }
    if (e.type == SDL_MOUSEWHEEL && gPage == "peset") { gSt["ps.scr"] = std::max(0, std::min(gsi("ps.max", 0), gsi("ps.scr", 0) - e.wheel.y * 22)); return true; }
    if (e.type == SDL_MOUSEWHEEL) { for (const Wd& d : v) if (d.k == WK_LST) { gSt[d.id + ".scr"] -= e.wheel.y * lstRow(d.fmt); lstScr(d); break; } return true; }
    if (e.type == SDL_MOUSEBUTTONUP && gPage == "peset" && gSt["ps.pend"]) { gSt["ps.pend"] = 0; if (gSt["ps.acc"] >= 2) return true;   // toque sem arrasto = clique
        SDL_Event e2 = e; e2.type = SDL_MOUSEBUTTONDOWN; e2.button.button = SDL_BUTTON_LEFT; e2.button.x = gSt["ps.px"] * G; e2.button.y = gSt["ps.py"] * G; gMX = e2.button.x; gMY = e2.button.y; gSt["ps.skip"] = 1; return pageEvent(e2, w, h); }
    if (e.type == SDL_MOUSEBUTTONUP) { if (!gDrag.empty() && gDrag != "pkspin") optSave(); gDrag.clear(); return true; }
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) return true;
    if (oskActive()) { gOskSW = sw; gOskSH = sh; const int hk = oskHit(ux, uy); if (hk >= 0) { gOskSel = hk; oskPress(); return true; } if (uy >= sh - OSK_H) return true; }   // clique no teclado virtual
    if (gPage == "peskin" && inR(peSkinDropZone(sw, sh), ux, uy)) { pageAction("l.file"); return true; }   // clicar na area de soltar = escolher arquivo
    if (gPage == "peskin" && inR(peSkinPrev(sw, sh), ux, uy)) { gDrag = "pkspin"; return true; }   // patchSkin.1: arrastar no painel gira o boneco 3D
    if (gPage == "peskin") { const PkLay pl = peSkinLay(sw, sh); const int off = gsi("pk.off", 0), idx = off + (ux - pl.x0) / 48;
        if (ux >= pl.x0 && ux < pl.x0 + pl.nVis * 48 && uy >= pl.carY && uy < pl.carY + pl.carH && idx >= 0 && idx < (int)gSkinList.size()) { if (skinUse(gSkinList[idx])) profileSave(); sndEvtUi("ui.click"); return true; } }
    if (gPage == "peset" && !gSt["ps.skip"] && ux >= 136 && uy >= 28 && uy < sh - 8) { bool sl = false; for (const Wd& d : v) if (d.k == WK_SLD && inR({d.x, d.y, d.w, d.h}, ux, uy)) sl = true;
        if (!sl) { gSt["ps.pend"] = 1; gSt["ps.px"] = ux; gSt["ps.py"] = uy; gSt["ps.ly"] = gMY; gSt["ps.acc"] = 0; return true; } }   // botoes da lista agem ao soltar (permite rolar arrastando)
    gSt["ps.skip"] = 0;
    gFocus.clear();
    for (const Wd& d : v) {
        if (gPage == "peset" && d.x >= 136 && (uy < 28 || uy >= sh - 8)) continue;   // fora da area rolavel
        if (!inR({d.x, d.y, d.w, d.h}, ux, uy)) continue;
        if (d.k == WK_FLD) { gFocus = d.id; sndEvtUi("ui.click"); }   // patchMenuSnd.1: campo/slider/lista tambem clicam
        else if (d.k == WK_SLD) { gDrag = d.id; sldSet(d, ux); sndEvtUi("ui.click"); }
        else if (d.k == WK_BTN && !d.dis) {
            sndEvtUi("ui.click");   // patchAud.6
            if (d.go == "<") pageBack();
            else if (!d.go.empty() && d.go[0] == '=') {   // "=prefixo:valor": define todos os botões alternáveis com esse prefixo
                const size_t q = d.go.find(':'); const std::string pre = d.go.substr(1, q - 1); const int val = atoi(d.go.c_str() + q + 1);
                for (const Wd& o : v) if (o.k == WK_BTN && !o.opt.empty() && o.id.compare(0, pre.size(), pre) == 0) gSt[o.id] = val;
            } else if (!d.go.empty() && d.go[0] == '!') { const size_t q = d.go.find(':'); gSt[d.go.substr(1, q - 1)] = atoi(d.go.c_str() + q + 1); }
            else if (!d.go.empty()) pageGo(d.go);
            else if (!d.opt.empty()) { gSt[d.id] = (gsi(d.id, (int)d.def) + 1) % (int)d.opt.size(); if (d.id == "d17" || d.id == "d18" || d.id == "v2" || d.id == "v0" || d.id == "v5" || d.id == "v6" || d.id == "d0" || d.id[0] == 'd' || d.id[0] == 'v' || d.id[0] == 'q' || d.id[0] == 'p' || d.id[0] == 'x' || d.id.compare(0, 3, "an.") == 0) { gfxTogglesSync(); optSave(); }
                if (d.id == "sh0") gShadowClick = true; if (d.id == "sh1") gShQClick = true;
                if (d.id == "gfxapi") { gSt["gfxset"] = 1; if (!gApiOpenGL()) remove("no_vulkan"); optSave(); } }   // escolher Vulkan de novo limpa o bloqueio deixado por uma falha anterior
            else { pageAction(d.id); return true; }
            break;
        } else if (d.k == WK_LST) {
            const int idx = (uy - d.y - lstPad(d.fmt) + lstScr(d)) / lstRow(d.fmt);
            if (idx >= 0 && idx < d.n) { sndEvtUi("ui.click"); if (d.fmt == 0) gSt["lang"] = idx; else if (d.fmt == 1) { gSt["shd.sel"] = BL_NO_FANCY ? 0 : idx; gShaderPackOn = !BL_NO_FANCY && idx > 0; } else if (d.fmt == 2) { static Uint32 mt = 0; static int mi = -1; const Uint32 tn = SDL_GetTicks(); gSt["multi.sel"] = idx; if (mi == idx && tn - mt < 450) { mi = -1; pageAction("ms0"); return true; } mi = idx; mt = tn; }
                else if (d.fmt == 5) { const std::vector<std::string> mr = modRows(); if (idx < (int)mr.size()) { const std::string mn = mr[idx].substr(0, mr[idx].find('|')); modsOffLoad(); if (gModsOff.count(mn)) gModsOff.erase(mn); else gModsOff.insert(mn); optSave(); } }
                else if (d.fmt == 7) { static Uint32 kt = 0; static int ki = -1; const Uint32 tn = SDL_GetTicks(); skinSelect(idx); if (ki == idx && tn - kt < 450) { ki = -1; pageAction("l.play"); return true; } ki = idx; kt = tn; }
                else if (d.fmt == 8) { if (idx < (int)gBrEnts.size()) { const BrEnt be = gBrEnts[idx]; const std::string q = (std::filesystem::path(gBrDir) / be.name).string();
                        if (be.dir) { gBrDir = q; browseScan(); } else if (skinDropped(q.c_str())) pageBack(); else lmsg("Arquivo inválido: use PNG 64x64 ou 64x32"); } return true; }
                else if (d.fmt == 6) { static Uint32 lt = 0; static int li = -1; const Uint32 tn = SDL_GetTicks(); gSt["w.sel"] = idx; if (li == idx && tn - lt < 450) { li = -1; pageAction("w.play"); return true; } li = idx; lt = tn; } }
        }
    }
    txtKbd();
    return true;
}
