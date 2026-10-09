// ---------- modulo Monitor Touch ----------
// Bloco "Monitor Touch": clique direito / toque no "+" abre um navegador na tela. A URL de cada monitor e SINCRONIZADA no multijogador
// (NP_MON), salva com o mundo (saves/<mundo>/monitors.txt) e a tela e desenhada no mundo em todas as plataformas (Windows/Linux/Android).
// PARTE 1: bloco + interacao + barra de enderecos + teclado/toque + sincronizacao + "Abrir no navegador do sistema" (SDL_OpenURL).
// PARTE 2: motor de pagina. ANDROID = WebView fora da tela (BlocklandActivity.java) que envia quadros ~10/s por JNI (pagina viva, toque, rolagem e digitacao).
//   DESKTOP (Windows/Linux) = Edge/Chrome/Chromium do sistema em modo headless controlado por DevTools Protocol (cdp.inc): pagina viva (~15 quadros/s) e enviada aos outros jogadores como JPEG (NP_MONF).
#if (defined(__ANDROID__) && !defined(BL_MON_DESKTOP)) || defined(BL_MON_FORCE_ANDROID)
#define BL_MON_ANDROID 1
#include <jni.h>
#include <stdarg.h>
#elif defined(_WIN32)
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#endif
namespace mod_monitor {
    namespace pw { struct WebInfo { std::string title, hover, status; int progress = 100; bool canBack = false, canFwd = false, loading = false; float docH = 0, scrollY = 0, viewH = 1; int zoomPct = 100; }; }
    #ifdef BL_MON_ANDROID
    static const char* HOME_URL = "https://www.google.com"; static const char* SEARCH_URL = "https://www.google.com/search?q=";
#else
    static const char* HOME_URL = "about:home"; static const char* SEARCH_URL = "https://html.duckduckgo.com/html/?q=";   // buscador que funciona sem JavaScript
#endif
    struct Mon { std::string url = HOME_URL; Uint32 rev = 0, touched = 0; GLuint tex = 0; int pw = 0, ph = 0; };
    static bool validUrl(const std::string& u) { return ((u.rfind("http://", 0) == 0 || u.rfind("https://", 0) == 0) && u.size() < 200) || u == "about:home"; }   // so http(s): bloqueia file:, javascript:, intent: vindos da rede
    static std::map<std::array<int, 3>, Mon> gMon;
    static int gId = -1; static bool gOpen = false; static std::array<int, 3> gCur{0, 0, 0}; static std::string gEdit; static bool gEditing = false;
    static std::string gDir; static double gPx = 0, gPy = 0, gPz = 0; static int gMx = 0, gMy = 0; static float gAsp = 1.f;
    // ---- telas que se juntam: monitores vizinhos no mesmo plano vertical (ate 8x8, retangulo cheio) viram UMA tela maior ----
    struct Grp { std::array<int, 3> m; int axis = -1; int cols = 1, rows = 1; };   // m = canto (menor u, menor y); axis 0 = plano z (u=x), 1 = plano x (u=z), -1 = bloco solto
    static bool planeRect(int x, int y, int z, int axis, int& u0, int& y0, int& cols, int& rows) {
        const int u = axis == 0 ? x : z; auto at = [&](int uu, int yy) { return axis == 0 ? getB(uu, yy, z) == gId : getB(x, yy, uu) == gId; };
        typedef std::pair<int, int> P; std::vector<P> seen{P(u, y)}, st{P(u, y)}; int mnu = u, mxu = u, mny = y, mxy = y;
        static const int N[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!st.empty()) { const P c = st.back(); st.pop_back(); mnu = std::min(mnu, c.first); mxu = std::max(mxu, c.first); mny = std::min(mny, c.second); mxy = std::max(mxy, c.second);
            for (const auto& n : N) { const P p(c.first + n[0], c.second + n[1]); if (!at(p.first, p.second) || std::find(seen.begin(), seen.end(), p) != seen.end()) continue;
                if (seen.size() >= 64) return false; seen.push_back(p); st.push_back(p); } }
        cols = mxu - mnu + 1; rows = mxy - mny + 1; u0 = mnu; y0 = mny; return cols <= 8 && rows <= 8 && (int)seen.size() == cols * rows;
    }
    static Grp groupOf(int x, int y, int z) {   // deterministico (igual em todos os jogadores): vence o plano de maior area; empate = plano z
        Grp g; g.m = {x, y, z}; int best = 1;
        for (int axis = 0; axis < 2; axis++) { int u0, y0, c, r; if (planeRect(x, y, z, axis, u0, y0, c, r) && c * r > best) { best = c * r; g.axis = axis; g.cols = c; g.rows = r; g.m = axis == 0 ? std::array<int, 3>{u0, y0, z} : std::array<int, 3>{x, y0, u0}; } }
        return g;
    }
    static float monAspect(const Grp& g) { return (g.cols - .25f) / (g.rows - .25f); }   // proporcao do vidro (sem moldura)
    static void monPixels(const std::array<int, 3>& k, int& w, int& h) {   // resolucao da pagina: cresce com o tamanho da tela, sem esticar; orcamento ~0,6 Mpx (leve no celular)
        const Grp g = groupOf(k[0], k[1], k[2]); float W = (g.cols - .25f) * 400.f, H = (g.rows - .25f) * 400.f;
        if (g.cols == 1 && g.rows == 1) W = H = 480.f;
        const float s = std::min(std::min(1.f, sqrtf(600000.f / (W * H))), 1900.f / std::max(W, H));
        w = std::max(240, ((int)(W * s)) & ~7); h = std::max(240, ((int)(H * s)) & ~7);
    }

    static std::string path() { return "saves/" + gCurDir + "/monitors.txt"; }
    static void save() {
        if (gCurDir.empty()) return; FILE* f = fopen(path().c_str(), "w"); if (!f) return;
        for (auto& kv : gMon) fprintf(f, "%d %d %d %s\n", kv.first[0], kv.first[1], kv.first[2], kv.second.url.c_str()); fclose(f);
    }
    static void load() {
        gMon.clear(); gDir = gCurDir; if (gCurDir.empty()) return; FILE* f = fopen(path().c_str(), "r"); if (!f) return; char ln[400];
        while (fgets(ln, sizeof ln, f)) { int x, y, z, n = 0; if (sscanf(ln, "%d %d %d %n", &x, &y, &z, &n) >= 3 && n > 0) { std::string u = ln + n; while (!u.empty() && (u.back() == '\n' || u.back() == '\r')) u.pop_back(); if (validUrl(u)) gMon[{x, y, z}].url = u; } }
        fclose(f);
    }
    static std::string normalize(std::string s) {   // "youtube.com" -> https://youtube.com ; texto com espacos -> busca
        while (!s.empty() && s.back() == ' ') s.pop_back(); size_t a = s.find_first_not_of(' '); s = a == std::string::npos ? "" : s.substr(a);
        if (s.empty()) return HOME_URL;
        if (s.find("://") != std::string::npos) return validUrl(s) ? s.substr(0, 190) : std::string(HOME_URL);
        if (s.find(' ') != std::string::npos || s.find('.') == std::string::npos) { std::string q; for (unsigned char c : s) { if (isalnum(c)) q += (char)c; else { char h[4]; snprintf(h, 4, "%%%02X", c); q += h; } } return (std::string(SEARCH_URL) + q).substr(0, 190); }
        return ("https://" + s).substr(0, 190);
    }
    static void sendMon(const std::array<int, 3>& k, const std::string& u) {
        if (!netOn) return; NBuf b = netMk(NP_MON); b.put<int32_t>(k[0]); b.put<int32_t>(k[1]); b.put<int32_t>(k[2]);
        const int l = (int)std::min<size_t>(u.size(), 190); b.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)u[i]); netBroadcast(b);
    }

    // ---------- imagem da pagina para os outros jogadores (UDP: JPEG em fragmentos; o host repassa; quadro incompleto e descartado) ----------
    static SDL_mutex* gLock = nullptr; static std::deque<NBuf> gFq; static uint16_t gFid = 0;
    static void queueJpeg(const std::array<int, 3>& k, const uint8_t* d, int n) {   // thread da UI do Android
        if (!netOn || !gLock || n <= 0 || n > 48000) return; const int cnt = (n + 1023) / 1024; std::deque<NBuf> add;
        const uint16_t fid = ++gFid;
        for (int i = 0; i < cnt; i++) { NBuf b = netMk(NP_MONF); b.put<int32_t>(k[0]); b.put<int32_t>(k[1]); b.put<int32_t>(k[2]); b.put<uint16_t>(fid); b.put<uint8_t>((uint8_t)i); b.put<uint8_t>((uint8_t)cnt);
            const int l = std::min(1024, n - i * 1024); b.put<uint16_t>((uint16_t)l); for (int j = 0; j < l; j++) b.put<uint8_t>(d[i * 1024 + j]); add.push_back(b); }
        SDL_LockMutex(gLock); gFq.swap(add); SDL_UnlockMutex(gLock);   // quadro novo substitui o que ainda nao saiu
    }
    static void drainFq() {   // 20x/s: no maximo 6 pacotes por tick (nao estoura o buffer UDP)
        if (!gLock || !netOn) return; SDL_LockMutex(gLock); for (int i = 0; i < 6 && !gFq.empty(); i++) { netBroadcast(gFq.front()); gFq.pop_front(); } SDL_UnlockMutex(gLock);
    }
    struct Asm { uint16_t fid = 0; int cnt = 0, got = 0; std::vector<std::string> part; };
    static std::map<std::array<int, 3>, Asm> gAsm;
    // ---------- backend de pagina ----------
    static std::vector<uint32_t> gPend; static int gPW = 0, gPH = 0; static bool gPDirty = false; static std::array<int, 3> gPKey{0, 0, 0};
    static std::array<int, 3> gBeKey{0, 0, 0}; static std::string gBeUrl, gStatus = ""; static bool gDrag = false, gQuit = false;
    static void setPending(const std::array<int, 3>& k, const uint32_t* px, int w, int h) { SDL_LockMutex(gLock); gPend.assign(px, px + (size_t)w * h); gPW = w; gPH = h; gPKey = k; gPDirty = true; SDL_UnlockMutex(gLock); }
    static void uploadFrame() {   // thread do jogo: sobe o quadro novo para a textura do monitor
        if (!gLock || !gPDirty) return; SDL_LockMutex(gLock);
        auto it = gMon.find(gPKey);
        if (it != gMon.end() && gPW > 0 && gPH > 0) { Mon& m = it->second; m.pw = gPW; m.ph = gPH; if (!m.tex) glGenTextures(1, &m.tex); glBindTexture(GL_TEXTURE_2D, m.tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gPW, gPH, 0, GL_RGBA, GL_UNSIGNED_BYTE, gPend.data()); glBindTexture(GL_TEXTURE_2D, atlasTex); }
        gPDirty = false; SDL_UnlockMutex(gLock);
    }
#ifdef BL_MON_ANDROID
    static JNIEnv* jenv() { return (JNIEnv*)SDL_AndroidGetJNIEnv(); }
    static void jcall(const char* name, const char* sig, ...) {   // chama um metodo publico de BlocklandActivity
        JNIEnv* env = jenv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) return;
        jclass c = env->GetObjectClass(act); jmethodID m = env->GetMethodID(c, name, sig);
        if (m) { va_list ap; va_start(ap, sig); env->CallVoidMethodV(act, m, ap); va_end(ap); }
        if (env->ExceptionCheck()) env->ExceptionClear(); env->DeleteLocalRef(c); env->DeleteLocalRef(act);
    }
    static void beSize(int w, int h) { jcall("monSize", "(II)V", (jint)w, (jint)h); }
    static void beShare(bool on) { jcall("monShare", "(Z)V", (jboolean)on); }
    static bool beDecode(const std::string& jpg, std::vector<uint32_t>& out, int& w, int& h) {   // JPEG -> RGBA via BitmapFactory (Java)
        JNIEnv* env = jenv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) return false; bool ok = false; jclass c = env->GetObjectClass(act); jmethodID m = env->GetMethodID(c, "monDecode", "([B)[I");
        if (m) { jbyteArray ja = env->NewByteArray((jsize)jpg.size()); env->SetByteArrayRegion(ja, 0, (jsize)jpg.size(), (const jbyte*)jpg.data());
            jintArray r = (jintArray)env->CallObjectMethod(act, m, ja); if (env->ExceptionCheck()) env->ExceptionClear();
            if (r) { const int n = env->GetArrayLength(r); if (n > 2) { std::vector<jint> t(n); env->GetIntArrayRegion(r, 0, n, t.data()); w = t[0]; h = t[1];
                    if (w > 0 && h > 0 && w <= 2048 && h <= 2048 && n >= 2 + w * h) { out.resize((size_t)w * h); for (int i = 0; i < w * h; i++) { const uint32_t u = (uint32_t)t[2 + i]; out[i] = (u & 0xFF00FF00u) | ((u >> 16) & 0xFFu) | ((u & 0xFFu) << 16); } ok = true; } }
                env->DeleteLocalRef(r); }
            env->DeleteLocalRef(ja); }
        env->DeleteLocalRef(c); env->DeleteLocalRef(act); return ok;
    }
    static void beNavigate(const std::array<int, 3>& k, const std::string& u) { gBeKey = k; gBeUrl = u; gStatus = "WebView: carregando..."; JNIEnv* env = jenv(); if (!env) return; { int sw, sh; monPixels(k, sw, sh); beSize(sw, sh); } jstring js = env->NewStringUTF(u.c_str()); jcall("monLoad", "(Ljava/lang/String;)V", js); env->DeleteLocalRef(js); }
    static void beShow(bool on) { jcall("monShow", "(Z)V", (jboolean)on); }
    static void beTouch(int act, float nx, float ny) { jcall("monTouch", "(IFF)V", (jint)act, (jfloat)nx, (jfloat)ny); }
    static void beText(const char* t) { for (const char* p = t; *p; p++) if ((unsigned char)*p >= 0xF0) return; JNIEnv* env = jenv(); if (!env) return; jstring js = env->NewStringUTF(t); jcall("monText", "(Ljava/lang/String;)V", js); env->DeleteLocalRef(js); }
    static void beKey(int k) { int code = k == SDLK_BACKSPACE ? 67 : (k == SDLK_RETURN || k == SDLK_KP_ENTER) ? 66 : k == 9 ? 61 : k == SDLK_UP ? 19 : k == SDLK_DOWN ? 20 : k == SDLK_LEFT ? 21 : k == SDLK_RIGHT ? 22 : k == SDLK_DELETE ? 112 : k == SDLK_HOME ? 122 : k == SDLK_END ? 123 : 0; if (code) jcall("monKey", "(I)V", (jint)code); }
    static void beScroll(int dy) { jcall("monScroll", "(I)V", (jint)dy); }
    static void beBack() { jcall("monBack", "()V"); }
    static void beHover(float, float) {}
    static void beWheel(float, float, int dy) { beScroll(dy); }
    static void beReload() { jcall("monReload", "()V"); }
    static void beForward() {} static void beStop() {} static void beZoom(int) {}
    static pw::WebInfo webInfo() { return pw::WebInfo(); }
    static std::string getStatus() { return gStatus; }
    static const char* beName() { return "WebView (Android)"; }
#else   // desktop: navegador PROPRIO (web/web.inc): rede + HTML + CSS + layout + rasterizador por software; sem Chrome
#include "web/web.inc"
    static void beNavigate(const std::array<int, 3>& k, const std::string& u) {
        gBeKey = k; gBeUrl = u; ensureWorker();
        SDL_LockMutex(gLock); gWantUrl = u; gWantKey = k; gSeq++; SDL_UnlockMutex(gLock);
    }
    static void beShow(bool on) { if (!gLock) return; SDL_LockMutex(gLock); gVis = on; SDL_UnlockMutex(gLock); }
    static void beSize(int w, int h) { setSize(w, h); }
    static void beShare(bool) {}
    static bool beDecode(const std::string&, std::vector<uint32_t>&, int&, int&) { return false; }   // quadros dos outros jogadores sao decodificados na thread da pagina (queueDecode)
    static void beReload() { cmdPush(pw::CT_RELOAD, 0, 0, 0); }
    static const char* beName() { return gName; }
#endif
    static void beOpen() {   // GUI aberta: mostra o backend e carrega a URL se ainda nao for a dele
        beShow(true); beShare(netOn); { int sw, sh; monPixels(gCur, sw, sh); beSize(sw, sh); } const Mon& m = gMon[gCur]; if (gBeKey != gCur || gBeUrl != m.url) beNavigate(gCur, m.url);
    }
    static void navigate(const std::string& raw) {
        Mon& m = gMon[gCur]; m.url = normalize(raw); m.rev++; m.touched = SDL_GetTicks(); gEdit = m.url; sendMon(gCur, m.url); save();
        if (gOpen) beNavigate(gCur, m.url);
    }
    static void onInit() {   // registra o bloco em codigo (funciona tambem no Android, onde a pasta mods/ nao vem no APK)
        gLock = SDL_CreateMutex();
        if (modNext > 255) return; gId = modNext++; modName[gId] = "Monitor Touch"; modBlk[gId] = true; modEmit[gId] = 4;
        for (int f = 0; f < 3; f++) { const int tile = 336 + 3 * (gId - MOD0) + f; TILES[gId][f] = tile;
            for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
                const bool edge = x < 2 || y < 2 || x > 13 || y > 13; float n = hash2(x, y, gId * 3 + f) * 8.f;
                if (f != 1 || edge) apx(tile, x, y, clampb(34 + n), clampb(36 + n), clampb(40 + n));
                else { const float g = y / 16.f; apx(tile, x, y, clampb(20 + 30 * g + n), clampb(70 + 60 * g + n), clampb(130 + 60 * g + n)); }
            } }
        ModDef m; m.name = "Monitor"; m.ids.push_back(gId); TABS.push_back({strdup("Monitor"), gId, false, false, -1}); MODS.push_back(m);
    }
#ifndef BL_MON_ANDROID
    static void applyNav() {   // thread do jogo: a pagina navegou (link clicado/redirecionamento) -> a URL do monitor acompanha e e sincronizada
        if (!gLock) return; std::string u; SDL_LockMutex(gLock); u.swap(gNavIn); SDL_UnlockMutex(gLock);
        if (u.empty() || !gOpen || gBeKey != gCur || !validUrl(u)) return; u = u.substr(0, 190); Mon& m = gMon[gCur]; if (m.url == u) return;
        m.url = u; m.rev++; gBeUrl = u; if (!gEditing) gEdit = u; sendMon(gCur, u); save();
    }
#else
    static void applyNav() {}
#endif
    static void onTick(Phys& P) { applyNav(); gPx = P.x; gPy = P.y; gPz = P.z; if (gDir != gCurDir) load(); drainFq(); static Uint32 st = 0; const Uint32 n = SDL_GetTicks(); if (gOpen && n - st > 1000) { st = n; beShare(netOn); }
        static bool want = false; const double dx = gBeKey[0] + .5 - gPx, dz = gBeKey[2] + .5 - gPz;   // pagina ao vivo tambem quando o monitor esta perto (como a TV do WebDisplays); longe: pausa e economiza CPU
        const bool w = gOpen || (!gBeUrl.empty() && dx * dx + dz * dz < 24.0 * 24.0); if (w != want) { want = w; beShow(w); if (w) beShare(netOn); } }
    static void onShutdown() { save(); gQuit = true; beShow(false);
#ifndef BL_MON_ANDROID
        killBrowser();
#endif
    }
    struct TQ { GLuint tex; bool flip; float q[4][3]; };
    static void drawQuad(const float q[4][3], float r, float g, float b) { glColor4f(r, g, b, 1.f); for (int k = 0; k < 4; k++) glVertex3f(q[k][0], q[k][1], q[k][2]); }
    static void onDrawWorld(float) {   // tela em todas as faces expostas; monitores vizinhos desenham UMA tela maior (so o canto "mestre" desenha)
        if (gId < 0 || gMon.empty()) return; const Uint32 now = SDL_GetTicks(); uploadFrame(); std::vector<TQ> tq;
        std::vector<std::pair<std::array<int, 3>, std::string>> mig; std::vector<std::array<int, 3>> drop;
        glDisable(GL_TEXTURE_2D); glBegin(GL_QUADS);
        for (auto it = gMon.begin(); it != gMon.end();) {
            const int x = it->first[0], y = it->first[1], z = it->first[2];
            if (fabs(x + .5 - gPx) < 48 && fabs(z + .5 - gPz) < 48 && getC(fdiv(x, CW), fdiv(z, CW)) && getB(x, y, z) != gId) { it = gMon.erase(it); continue; }   // bloco quebrado (so se o chunk ja carregou: antes o monitor sumia no cliente enquanto o mundo chegava)
            if (fabs(x + .5 - gPx) < 40 && fabs(z + .5 - gPz) < 40) {
                const Grp G = groupOf(x, y, z);
                if (G.m != it->first) { mig.push_back({G.m, it->second.url}); drop.push_back(it->first); ++it; continue; }   // virou parte de uma tela maior
                const bool act = now - it->second.touched < 4000; const float pulse = act ? .75f + .25f * sinf(now * .01f) : 1.f;
                static const int D[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
                const float cols = (float)G.cols, rows = (float)G.rows, e = .004f;
                for (int d = G.axis == 1 ? 2 : 0; d < (G.axis == 0 ? 2 : 4); d++) {
                    bool clear = true;   // todas as celulas do grupo precisam ter ar na frente
                    for (int c = 0; c < G.cols && clear; c++) for (int r = 0; r < G.rows; r++) {
                        const int bx = G.axis == 1 ? x : x + c, bz = G.axis == 1 ? z + c : z;
                        if (getB(bx + D[d][0], y + r, bz + D[d][1]) != 0) { clear = false; break; } }
                    if (!clear) continue;
                    float q[4][3];
                    auto face = [&](float u0, float v0, float u1, float v1) {   // u = ao longo da tela, v = altura
                        for (int k = 0; k < 4; k++) { const float u = (k == 0 || k == 3) ? u0 : u1, v = k < 2 ? v0 : v1;
                            if (d < 2) { q[k][0] = x + u; q[k][2] = z + (d == 0 ? -e : 1 + e); } else { q[k][2] = z + u; q[k][0] = x + (d == 2 ? -e : 1 + e); } q[k][1] = y + v; } };
                    face(.0625f, .0625f, cols - .0625f, rows - .0625f); drawQuad(q, .03f, .04f, .06f);                                   // moldura
                    face(.125f, .125f, cols - .125f, rows - .125f); drawQuad(q, .08f * pulse, .13f * pulse, .22f * pulse);               // vidro
                    if (it->second.tex) { TQ t; t.tex = it->second.tex; t.flip = d == 0 || d == 3;
                        for (int k = 0; k < 4; k++) { t.q[k][0] = q[k][0]; t.q[k][1] = q[k][1]; t.q[k][2] = q[k][2]; if (d < 2) t.q[k][2] += d == 0 ? -.002f : .002f; else t.q[k][0] += d == 2 ? -.002f : .002f; }
                        tq.push_back(t); }
                    face(.125f, rows - .22f, cols - .125f, rows - .125f); drawQuad(q, .18f, .20f, .26f);                                 // barra de enderecos
                    face(cols / 2 - .1f, rows / 2 - .2f, cols / 2 + .1f, rows / 2 + .1f); drawQuad(q, .9f * pulse, .1f, .1f);           // "play" estilizado
                }
            }
            ++it;
        }
        glEnd(); glColor4f(1, 1, 1, 1); glEnable(GL_TEXTURE_2D);
        for (const TQ& t : tq) { glBindTexture(GL_TEXTURE_2D, t.tex); glBegin(GL_QUADS);   // paginas (so monitores com quadro)
            for (int k = 0; k < 4; k++) { const float su = (k == 0 || k == 3) ? 0.f : 1.f, sv = k < 2 ? 1.f : 0.f; glTexCoord2f(t.flip ? 1.f - su : su, sv); glVertex3f(t.q[k][0], t.q[k][1], t.q[k][2]); } glEnd(); }
        if (!tq.empty()) glBindTexture(GL_TEXTURE_2D, atlasTex);
        for (size_t i = 0; i < mig.size(); i++) { if (!gMon.count(mig[i].first)) gMon[mig[i].first].url = mig[i].second; gMon.erase(drop[i]); }   // URL do bloco absorvido passa para o canto mestre
    }
    // ---------- GUI (estilo Minecraft: escala inteira da GUI do jogo, botoes Rethoughted, fonte 5x7 com sombra) ----------
    // Medidas em UNIDADES DA GUI (como o menu de pausa): ::G pixels por unidade, G = guiScaleFor(w, h). Entrada em pixels da janela.
    struct R { float x, y, w, h; bool in(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };
    struct Lay { int g, sw, sh; R bar, go, open, close, view, back, fwd, reload, kb, fit, status, sbar; R fav[4]; };
    #ifdef BL_MON_ANDROID
    static const char* FAV[4][2] = {{"YouTube", "https://m.youtube.com"}, {"Google", "https://www.google.com"}, {"Wikipedia", "https://pt.wikipedia.org"}, {"Bing", "https://www.bing.com"}};
#else
    static const char* FAV[4][2] = {{"Inicio", "about:home"}, {"Wikipedia", "https://pt.wikipedia.org"}, {"Hacker News", "https://news.ycombinator.com"}, {"NPR", "https://text.npr.org"}};
#endif
#ifdef BL_MON_ANDROID
    static const bool gDirectBack = true;
#else
    static const bool gDirectBack = false;
#endif
    static bool gFinger = false, gFingerSeen = false, gKb = false, gScroll = false; static int64_t gFinId = 0; static float gSx = 0, gSy = 0, gLy = 0;
    static float clamp01(float v) { return std::max(0.f, std::min(1.f, v)); }
    static float curAspect() { auto it = gMon.find(gCur); return it != gMon.end() && it->second.pw > 0 && it->second.ph > 0 ? (float)it->second.pw / (float)it->second.ph : gAsp; }   // proporcao REAL da imagem (nao estica)
    static bool touchUi() {
#ifdef BL_MON_ANDROID
        return true;
#else
        return gFingerSeen;
#endif
    }
    static Lay lay(int w, int h) {
        Lay L; L.g = guiScaleFor(w, h); L.sw = (w + L.g - 1) / L.g; L.sh = (h + L.g - 1) / L.g;
        const float m = 4, bh = 20, gp = 2, W = (float)L.sw - 2 * m, y = m; float x = m;
        L.back = {x, y, bh, bh}; x += bh + gp; L.fwd = {x, y, bh, bh}; x += bh + gp; L.reload = {x, y, bh, bh}; x += bh + gp;
        if (touchUi()) { L.kb = {x, y, bh, bh}; x += bh + gp; } else L.kb = {-100.f, -100.f, 0.f, 0.f};
        L.close = {m + W - bh, y, bh, bh};
        const float ow = (float)uTextW(L.sw >= 400 ? "Abrir no sistema" : "Sistema") + 12; L.open = {L.close.x - gp - ow, y, ow, bh};
        L.go = {L.open.x - gp - 28, y, 28, bh}; L.bar = {x, y, std::max(20.f, L.go.x - gp - x), bh};
        const float y2 = y + bh + gp, fw = floorf((W - 3 * gp) / 4);
        for (int i = 0; i < 4; i++) L.fav[i] = {m + i * (fw + gp), y2, fw, bh};
        const float vy = y2 + bh + 4; L.status = {m, (float)L.sh - m - 10, W, 10}; L.view = {m, vy, W, std::max(40.f, (float)L.sh - m - vy - 12)};
        const float asp = curAspect(), fw2 = std::max(8.f, std::min(L.view.w - 2, (L.view.h - 2) * asp)), fh2 = fw2 / asp;
        L.fit = {L.view.x + (L.view.w - fw2) / 2, L.view.y + (L.view.h - fh2) / 2, fw2, fh2}; L.sbar = {L.fit.x + L.fit.w + 1, L.fit.y, 3, L.fit.h}; return L;
    }
    static void mbtn(const R& r, const char* s, int st) {   // GuiButton: 0 normal, 1 realcado, 2 desativado
        if (r.w <= 0) return;
        if (rtBtn[0]) uNine(rtBtn[st == 2 ? 2 : st == 1 ? 1 : 0], 200, 20, 20, 4, 20, 4, r.x, r.y, r.w, r.h);
        else { const float hw = floorf(r.w / 2.f), v0 = (46.f + 20.f * st) / 256.f, v1 = (66.f + 20.f * st) / 256.f;   // reserva: widgets.png do Java
            glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, guiWidgetsTex); glColor4f(1, 1, 1, 1);
            quad(r.x * ::G, r.y * ::G, hw * ::G, 20.f * ::G, 0.f, v0, hw / 256.f, v1); quad((r.x + hw) * ::G, r.y * ::G, (r.w - hw) * ::G, 20.f * ::G, (200.f - (r.w - hw)) / 256.f, v0, 200.f / 256.f, v1); }
        const float n = (float)uTextW(s); uText(floorf(r.x + (r.w - n) / 2), r.y + (r.h - 8) / 2, s, st == 2 ? 0xA0A0A0 : 0xFFFFFF, true);
    }
    static void popU8(std::string& s) { while (!s.empty() && (s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); }
    static void field(const R& r, const std::string& s, bool edit) {   // GuiTextField: borda cinza (branca com foco), fundo preto, texto 0xE0E0E0
        U(r.x, r.y, r.w, r.h, edit ? 0xFFFFFF : 0xA0A0A0); U(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 0x000000);
        std::string show = s; const float maxw = r.w - 10;
        if (edit) { while (uTextW(show.c_str()) > maxw - 6 && show.size() > 1) { size_t k = 1; while (k < show.size() && (show[k] & 0xC0) == 0x80) k++; show.erase(0, k); } }   // mostra o fim ao digitar
        else if (uTextW(show.c_str()) > maxw) { while (show.size() > 1 && uTextW((show + "...").c_str()) > maxw) popU8(show); show += "..."; }
        const float ty = r.y + (r.h - 8) / 2; uText(r.x + 4, ty, show.c_str(), 0xE0E0E0, true);
        if (edit && (SDL_GetTicks() / 400) % 2) uText(r.x + 4 + (float)uTextW(show.c_str()), ty, "_", 0xE0E0E0, true);
    }
    static void centered(const R& r, float y, const char* s, unsigned c) { if ((float)uTextW(s) <= r.w - 4) uText(floorf(r.x + (r.w - (float)uTextW(s)) / 2), y, s, c, true); }
    static void onDrawHud(int w, int h) {
        if (!gOpen) return; const Lay L = lay(w, h); ::G = L.g; OX = 0; OY = 0; Mon& m = gMon[gCur]; const float oa = gUiA; gUiA = 1.f;
        glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_TEXTURE_2D); glBegin(GL_QUADS);   // fundo escurecido como o drawDefaultBackground do Java
        glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 224 / 255.f); glVertex2f(0, 0); glVertex2f((float)w, 0); glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 240 / 255.f); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
        glEnd(); glEnable(GL_TEXTURE_2D);
        const float mx = (float)gMx / L.g, my = (float)gMy / L.g; auto st = [&](const R& r) { return (!gFinger && r.in(mx, my)) ? 1 : 0; };
        const pw::WebInfo wi = webInfo(); mbtn(L.back, "<", wi.canBack || gDirectBack ? st(L.back) : 2); mbtn(L.fwd, ">", wi.canFwd ? st(L.fwd) : 2); mbtn(L.reload, wi.loading ? "X" : "R", st(L.reload)); mbtn(L.kb, "Aa", st(L.kb)); mbtn(L.close, "X", st(L.close));
        mbtn(L.go, "Ir", st(L.go)); mbtn(L.open, L.sw >= 400 ? "Abrir no sistema" : "Sistema", st(L.open));
        field(L.bar, gEditing ? gEdit : m.url, gEditing);
        for (int i = 0; i < 4; i++) mbtn(L.fav[i], FAV[i][0], st(L.fav[i]));
        U(L.view.x, L.view.y, L.view.w, L.view.h, 0x000000, .5f);
        uploadFrame(); const R& f = L.fit; const float gg = (float)L.g;
        U(f.x - 1, f.y - 1, f.w + 2, f.h + 2, 0x555555); U(f.x, f.y, f.w, f.h, 0x000000);
        const std::string stat = getStatus();
        if (m.tex) {
            glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, m.tex); glColor4f(1, 1, 1, 1); quad(f.x * gg, f.y * gg, f.w * gg, f.h * gg, 0, 0, 1, 1); glBindTexture(GL_TEXTURE_2D, atlasTex);
            if (!stat.empty()) { U(f.x, f.y + f.h - 12, f.w, 12, 0x000000, .65f); uText(f.x + 3, f.y + f.h - 10, stat.c_str(), 0xFFFF55, true); }
        } else {
            std::string host = m.url; { size_t a = host.find("://"); if (a != std::string::npos) host = host.substr(a + 3); size_t b = host.find('/'); if (b != std::string::npos) host = host.substr(0, b); }
            float sc = gg * 2.f; if (textWidth(host.c_str(), sc) > f.w * gg - 8 * gg) sc = gg;   // titulo em escala 2 (como o do Java), ou 1 se nao couber
            const float cy = f.y + f.h / 2;
            text(floorf(f.x * gg + (f.w * gg - textWidth(host.c_str(), sc)) / 2), floorf((cy - 22) * gg), sc, host.c_str(), 1, 1, 1, sc);
            centered(f, cy - 4, "Monitor sincronizado", 0xAAAAAA); centered(f, cy + 6, "Todos os jogadores veem esta página", 0xAAAAAA);
            std::string l2 = stat; if (l2.empty()) { l2 = "Carregando página"; for (int i = 0; i < (int)((SDL_GetTicks() / 400) % 4); i++) l2 += "."; }
            centered(f, cy + 20, l2.c_str(), 0xFFAA00); const char* bn = beName(); centered(f, f.y + f.h - 12, bn, 0x707070);
        }
        if (wi.loading || wi.progress < 100) { const float pg = std::max(4, wi.progress) / 100.f; U(L.bar.x, L.bar.y + L.bar.h, L.bar.w * pg, 2, 0x55AAFF); }   // barra de progresso sob o endereco
        if (wi.docH > wi.viewH + 1 && wi.viewH > 0) { const R& sb = L.sbar; U(sb.x, sb.y, sb.w, sb.h, 0x202020, .8f); const float th = std::max(10.f, sb.h * wi.viewH / wi.docH), ty = sb.y + (sb.h - th) * std::min(1.f, wi.scrollY / std::max(1.f, wi.docH - wi.viewH)); U(sb.x, ty, sb.w, th, 0xAAAAAA); }   // rolagem
        { std::string sl = !wi.hover.empty() ? wi.hover : !stat.empty() ? stat : wi.title; if (wi.zoomPct != 100) sl += "  [" + std::to_string(wi.zoomPct) + "%]"; U(L.status.x, L.status.y, L.status.w, L.status.h, 0x000000, .55f); std::string sh = sl; while (sh.size() > 1 && (float)uTextW(sh.c_str()) > L.status.w - 6) popU8(sh); if (sh != sl) sh += "..."; uText(L.status.x + 3, L.status.y + 1, sh.c_str(), !wi.hover.empty() ? 0x7CC4FF : 0xC0C0C0, true); }
#ifndef BL_MON_ANDROID
        { static SDL_Cursor *cHand = nullptr, *cArrow = nullptr; static bool wasHand = false; if (!cHand) { cHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); cArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW); } const bool hand = !wi.hover.empty(); if (hand != wasHand) { wasHand = hand; if (cHand && cArrow) SDL_SetCursor(hand ? cHand : cArrow); } }   // maozinha sobre links
#endif
        glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); gUiA = oa;
    }
    static void closeGui() { gOpen = false; gEditing = false; gDrag = false; gScroll = false; gKb = false; SDL_StopTextInput(); }
    static void press(float px, float py, int w, int h, int ph = 0, bool finger = false, int64_t fid = 0) {   // ph: 0 = descer, 1 = mover, 2 = soltar; px/py em pixels da janela
        const Lay L = lay(w, h); const float ux = px / L.g, uy = py / L.g; gMx = (int)px; gMy = (int)py; gFinger = finger; if (finger) gFingerSeen = true;
        const float nx = (ux - L.fit.x) / L.fit.w, ny = (uy - L.fit.y) / L.fit.h;
        if (ph != 0) {
            if (!gDrag || (finger && fid != gFinId)) return;
            if (ph == 1 && !gScroll && hypotf(px - gSx, py - gSy) > std::max(8.f, 3.f * L.g)) gScroll = true;   // passou do limite: e um arrasto, nao um toque
#ifndef BL_MON_ANDROID
            if (finger) {   // desktop com tela de toque: arrastar rola a pagina; toque curto vira clique
                if (ph == 1) { if (gScroll) beWheel(clamp01(nx), clamp01(ny), (int)((gLy - py) * (float)gWantH / (L.fit.h * L.g))); gLy = py; }
                else { if (!gScroll) { beTouch(0, clamp01(nx), clamp01(ny)); beTouch(2, clamp01(nx), clamp01(ny)); } gDrag = false; gScroll = false; }
                return; }
#endif
            beTouch(ph, clamp01(nx), clamp01(ny));   // mouse (e Android: o WebView recebe o dedo de verdade e rola/clica sozinho)
            if (ph == 2) {
#ifdef BL_MON_ANDROID
                if (!gScroll) { gKb = true; SDL_StartTextInput(); }   // toque simples: teclado disponivel para campos da pagina (botao "Aa" esconde)
#endif
                gDrag = false; gScroll = false; }
            return; }
        if (L.close.in(ux, uy)) { closeGui(); return; }
        if (L.back.in(ux, uy)) { beBack(); return; }
        if (L.fwd.in(ux, uy)) { beForward(); return; }
        if (L.reload.in(ux, uy)) { if (webInfo().loading) beStop(); else beReload(); return; }
        if (L.kb.w > 0 && L.kb.in(ux, uy)) { gKb = !gKb; if (gKb) SDL_StartTextInput(); else SDL_StopTextInput(); return; }
        if (L.fit.in(ux, uy)) {
            gEditing = false; gDrag = true; gScroll = false; gFinId = fid; gSx = px; gSy = py; gLy = py;
#ifndef BL_MON_ANDROID
            SDL_StartTextInput(); if (!finger) beTouch(0, clamp01(nx), clamp01(ny));   // dedo: espera para decidir entre clique e rolagem
#else
            beTouch(0, clamp01(nx), clamp01(ny));
#endif
            return; }
        if (L.bar.in(ux, uy)) { gEditing = true; gEdit = gMon[gCur].url; SDL_StartTextInput(); return; }
        if (L.go.in(ux, uy)) { navigate(gEditing ? gEdit : gMon[gCur].url); gEditing = false; return; }
        if (L.open.in(ux, uy)) { SDL_OpenURL(gMon[gCur].url.c_str()); return; }
        for (int i = 0; i < 4; i++) if (L.fav[i].in(ux, uy)) { navigate(FAV[i][1]); gEditing = false; return; }
        gEditing = false;
    }
    static bool onEvent(const SDL_Event& e, int w, int h) {
        switch (e.type) {
        case SDL_MOUSEMOTION: if (e.motion.which == SDL_TOUCH_MOUSEID) break; press((float)e.motion.x, (float)e.motion.y, w, h, 1);
            if (!gDrag && !gEditing) { const Lay L = lay(w, h); const float ux = (float)gMx / L.g, uy = (float)gMy / L.g; if (L.fit.in(ux, uy)) beHover(clamp01((ux - L.fit.x) / L.fit.w), clamp01((uy - L.fit.y) / L.fit.h)); } break;
        case SDL_MOUSEBUTTONDOWN: if (e.button.which != SDL_TOUCH_MOUSEID && e.button.button == SDL_BUTTON_LEFT) press((float)e.button.x, (float)e.button.y, w, h, 0); break;
        case SDL_MOUSEBUTTONUP: if (e.button.which != SDL_TOUCH_MOUSEID && e.button.button == SDL_BUTTON_LEFT) press((float)e.button.x, (float)e.button.y, w, h, 2); break;
        case SDL_FINGERDOWN: press(e.tfinger.x * w, e.tfinger.y * h, w, h, 0, true, (int64_t)e.tfinger.fingerId); break;
        case SDL_FINGERMOTION: press(e.tfinger.x * w, e.tfinger.y * h, w, h, 1, true, (int64_t)e.tfinger.fingerId); break;
        case SDL_FINGERUP: press(e.tfinger.x * w, e.tfinger.y * h, w, h, 2, true, (int64_t)e.tfinger.fingerId); break;
        case SDL_MOUSEWHEEL: if ((SDL_GetModState() & KMOD_CTRL) && e.wheel.y) { beZoom(e.wheel.y > 0 ? 1 : -1); break; } if (!gEditing) { const Lay L = lay(w, h); beWheel(clamp01(((float)gMx / L.g - L.fit.x) / L.fit.w), clamp01(((float)gMy / L.g - L.fit.y) / L.fit.h), -e.wheel.y * 120); } break;
        case SDL_TEXTINPUT: if (gEditing) { if (gEdit.size() < 190) gEdit += e.text.text; } else beText(e.text.text); break;
        case SDL_KEYDOWN: { const int k = e.key.keysym.sym; const bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
            if (k == SDLK_ESCAPE || k == 0x4000010E) { if (gEditing) gEditing = false; else closeGui(); }   // 0x4000010E = AC_BACK (botao voltar do Android)
            else if (!gEditing && ctrl && (k == '=' || k == '+' || k == 0x4000005E)) beZoom(1);
            else if (!gEditing && ctrl && (k == '-' || k == 0x40000056)) beZoom(-1);
            else if (!gEditing && ctrl && k == '0') beZoom(0);
            else if (!gEditing && (SDL_GetModState() & KMOD_ALT) && k == 0x40000050) beBack();
            else if (!gEditing && (SDL_GetModState() & KMOD_ALT) && k == 0x4000004F) beForward();
            else if (ctrl && k == 'l') { gEditing = true; gEdit = gMon[gCur].url; SDL_StartTextInput(); }
            else if (!gEditing && k == 0x4000003E) beReload();   // F5
            else if (!gEditing && k == SDLK_v && ctrl) { char* c = SDL_GetClipboardText(); if (c) { beText(c); SDL_free(c); } }   // colar na pagina
            else if (!gEditing) beKey(k);
            else if (k == SDLK_BACKSPACE) popU8(gEdit);
            else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) { navigate(gEdit); gEditing = false; }
            else if (k == SDLK_v && ctrl) { char* c = SDL_GetClipboardText(); if (c) { gEdit += c; if (gEdit.size() > 190) gEdit.resize(190); SDL_free(c); } }
            break; }
        }
        return true;   // enquanto aberto, o monitor consome toda a entrada
    }
    static void frameRecv(NBuf& b) {   // fragmento JPEG de outro jogador
        std::array<int, 3> k; k[0] = b.get<int32_t>(); k[1] = b.get<int32_t>(); k[2] = b.get<int32_t>(); const uint16_t fid = b.get<uint16_t>(); const int idx = b.get<uint8_t>(), cnt = b.get<uint8_t>(), len = b.get<uint16_t>();
        if (cnt <= 0 || cnt > 48 || idx >= cnt || len <= 0 || len > 1024 || b.r + len > b.n || !gMon.count(k)) return;
        if (gOpen && gCur == k) return;   // quem esta com o monitor aberto ja tem o quadro ao vivo
        Asm& a = gAsm[k]; if (a.fid != fid || a.cnt != cnt || (int)a.part.size() != cnt) { a.fid = fid; a.cnt = cnt; a.got = 0; a.part.assign(cnt, std::string()); }
        if (a.part[idx].empty()) { a.part[idx].assign((const char*)b.d + b.r, len); a.got++; }
        if (a.got < cnt) return; std::string j; for (const auto& p : a.part) j += p; a.cnt = 0; a.got = 0; a.part.clear();
        #ifdef BL_MON_ANDROID
        std::vector<uint32_t> px; int w = 0, h = 0; if (beDecode(j, px, w, h) && gLock) setPending(k, px.data(), w, h);
#else
        queueDecode(k, j);   // desktop: decodificador JPEG proprio (cdp.inc), fora da thread do jogo
#endif
    }
    static void netRecv(NBuf& b) {
        std::array<int, 3> k; k[0] = b.get<int32_t>(); k[1] = b.get<int32_t>(); k[2] = b.get<int32_t>(); const int l = b.get<uint8_t>(); std::string u; for (int i = 0; i < l; i++) u += (char)b.get<uint8_t>();
        if (!validUrl(u) || (gMon.size() >= 256 && !gMon.count(k))) return; Mon& m = gMon[k]; m.url = u; m.rev++; m.touched = SDL_GetTicks(); if (gOpen && gCur == k) { if (!gEditing) gEdit = u; beNavigate(k, u); } save();
    }
    static void netSync(NetPeer& p) {
        for (auto& kv : gMon) { NBuf b = netMk(NP_MON); b.put<int32_t>(kv.first[0]); b.put<int32_t>(kv.first[1]); b.put<int32_t>(kv.first[2]);
            const int l = (int)std::min<size_t>(kv.second.url.size(), 190); b.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)kv.second.url[i]); p.q.push_back(b); }
    }
}
GAME_MODULE_REGISTER(monitor, "monitor", mod_monitor::onInit, mod_monitor::onTick, mod_monitor::onDrawWorld, mod_monitor::onDrawHud, nullptr, mod_monitor::onShutdown);
// ---- pontes usadas por main_loop.inc e net.inc ----
static bool monIsOpen() { return mod_monitor::gOpen; }
static bool monUse(int x, int y, int z) {   // clique direito / toque no "+": abre o monitor
    using namespace mod_monitor; if (gId < 0 || getB(x, y, z) != gId) return false;
    { const Grp g = groupOf(x, y, z); gCur = g.m; gAsp = monAspect(g); } gMon[gCur]; gEdit = gMon[gCur].url; gEditing = false; mod_monitor::gDrag = false; mod_monitor::gScroll = false; mod_monitor::gKb = false; gOpen = true; beOpen(); return true;
}
static bool monEvent(const SDL_Event& e, int w, int h) { return mod_monitor::onEvent(e, w, h); }
static void monNetRecv(NBuf& b) { mod_monitor::netRecv(b); }
static void monNetSync(NetPeer& p) { mod_monitor::netSync(p); }
static void monFrameRecv(NBuf& b) { mod_monitor::frameRecv(b); }
#ifdef BL_MON_ANDROID
extern "C" JNIEXPORT void JNICALL Java_com_blockland_BlocklandActivity_nativeMonJpeg(JNIEnv* env, jclass, jbyteArray arr) {   // JPEG do quadro atual (Java) -> fragmentos UDP
    if (!arr || !mod_monitor::gOpen) return; const int n = env->GetArrayLength(arr); if (n <= 0 || n > 48000) return;
    std::vector<uint8_t> d(n); env->GetByteArrayRegion(arr, 0, n, (jbyte*)d.data()); mod_monitor::queueJpeg(mod_monitor::gBeKey, d.data(), n);
}
extern "C" JNIEXPORT void JNICALL Java_com_blockland_BlocklandActivity_nativeMonFrame(JNIEnv* env, jclass, jint w, jint h, jintArray arr) {   // quadro da WebView (ARGB) -> RGBA
    if (!mod_monitor::gLock || w <= 0 || h <= 0 || w > 2048 || h > 2048 || env->GetArrayLength(arr) < w * h) return;
    static std::vector<jint> tmp, prev; static std::array<int, 3> pk{0, 0, 0}; tmp.resize((size_t)w * h); env->GetIntArrayRegion(arr, 0, w * h, tmp.data());
    if (pk == mod_monitor::gBeKey && prev.size() == tmp.size() && memcmp(prev.data(), tmp.data(), tmp.size() * sizeof(jint)) == 0) return;   // pagina parada: nao reconverte nem reenvia a textura
    prev = tmp; pk = mod_monitor::gBeKey;
    for (jint& v : tmp) { const uint32_t u = (uint32_t)v; v = (jint)((u & 0xFF00FF00u) | ((u >> 16) & 0xFFu) | ((u & 0xFFu) << 16)); }
    mod_monitor::setPending(mod_monitor::gBeKey, (const uint32_t*)tmp.data(), w, h);
}
#endif
