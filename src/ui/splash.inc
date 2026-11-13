#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
// ================= tela de loading estilo Mojang (LoadingOverlay do Minecraft) =================
// Mesma sequencia do Minecraft: (1) barra avancando com currentProgress = currentProgress*0,95 + real*0,05 por quadro (a 60 fps);
// (2) carga pronta -> 1 s com a tela SOLIDA e a barra sumindo (alpha 1-f); (3) mais 1 s em que o MENU ja aparece por baixo (fundo, logo, botoes)
// enquanto o overlay inteiro (fundo + logo) faz fade-out. O menu e desenhado antes do overlay a cada quadro (splashOverlay, no loop principal).
static const float SPLASH_BG[3] = {1.f, 1.f, 1.f};   // patchIntro.4: Minecraft 1.12.2 (Minecraft.drawSplashScreen): fundo BRANCO + gui/title/mojang.png 256x256 centralizado, sem barra; troque aqui para mudar o fundo
static GLuint gSplashTex = 0;
static float gSplashCur = 0.f;                    // progresso suavizado (o que a barra mostra)
static Uint32 gSplashLast = 0, gSplashFadeT0 = 0; // gSplashFadeT0 != 0 -> carga pronta (fases 2 e 3)
static bool gSplashLoading = false; static bool gSplashLoading_ptr() { return gSplashLoading; }
static void splashRect(float x0, float y0, float x1, float y1) { glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1); }
// ---- loading simples: fundo escuro, "loaded: arquivo" em cima da barra, barra fina embaixo; sem logo (patchLoad.1)
static std::string gSplashLog[7]; static int gSplashLogN = 0; static Uint32 gSplashNoteT = 0;
static void splashNote(const char* path) {   // chamado pelos carregadores de PNG/som: guarda o nome do arquivo e deixa o loading redesenhar
    if (!gSplashLoading_ptr()) return;
    const char* b = path; for (const char* q = path; *q; q++) if (*q == '/' || *q == '\\') b = q + 1;
    if (!*b || (gSplashLogN && gSplashLog[(gSplashLogN - 1) % 7] == b)) return;
    gSplashLog[gSplashLogN++ % 7] = b; gSplashNoteT = SDL_GetTicks(); splashPoll();
}
static void splashText(float x, float y, int px, const std::string& t, float r, float g, float bl, float a) {   // fonte 5x7 embutida (F57), px = tamanho do pixel
    glColor4f(r, g, bl, a); glBegin(GL_QUADS);
    for (unsigned char c : t) { if (c < 32 || c > 126) c = '?'; const uint8_t* col = F57[c - 32];
        for (int cx = 0; cx < 5; cx++) for (int ry = 0; ry < 8; ry++) if (col[cx] >> ry & 1) splashRect(x + cx * px, y + ry * px, x + (cx + 1) * px, y + (ry + 1) * px);
        x += 6 * px; }
    glEnd();
}
// patchIntro.3: barra do LoadingOverlay do Minecraft: contorno branco de 1 unidade + preenchimento branco com folga de 2, 10 unidades de altura, mesma largura do logo, centro a 83,25% da altura
static inline int splashU(int h) { return std::max(1, h / 360); }   // 1 "unidade de GUI" da abertura (escala parecida com a do jogo)
static inline float splashLogoW(int w, int h) { return floorf(std::min(w * .9f, std::max(160.f, std::min(w * .75f, (float)h) * .7f))); }   // largura do logo (e da barra), como d1 do vanilla
static void splashBar(int w, int h, float prog, float a) {
    const float u = (float)splashU(h), lw = splashLogoW(w, h), x0 = floorf(w * .5f - lw * .5f), x1 = x0 + lw, cy = floorf(h * .8325f), y0 = cy - 5.f * u, y1 = cy + 5.f * u;
    const float fill = ceilf((lw / u - 2.f) * std::max(0.f, std::min(1.f, prog))) * u;
    rdDisable(GL_TEXTURE_2D); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glBegin(GL_QUADS); glColor4f(1, 1, 1, a);
    splashRect(x0 + u, y0, x1 - u, y0 + u); splashRect(x0 + u, y1 - u, x1 - u, y1); splashRect(x0, y0, x0 + u, y1); splashRect(x1 - u, y0, x1, y1);   // contorno
    if (fill > 0.f) splashRect(x0 + 2.f * u, y0 + 2.f * u, std::min(x0 + 2.f * u + fill, x1 - 2.f * u), y1 - 2.f * u);
    glEnd();
}
// ---- fundo em cache (patchIntro.2): panorama borrado salvo em cache/intro_bg.png na 1a vez; nas proximas a abertura ja nasce com ele
static void introPngWrite(const char* path, const std::vector<uint8_t>& rgb, int w, int h) {   // PNG minimo (deflate sem compressao)
    static uint32_t crcT[256]; if (!crcT[1]) for (uint32_t n = 0; n < 256; n++) { uint32_t c = n; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; crcT[n] = c; }
    std::vector<uint8_t> raw; raw.reserve((size_t)(w * 3 + 1) * h); for (int y = 0; y < h; y++) { raw.push_back(0); raw.insert(raw.end(), rgb.begin() + (size_t)y * w * 3, rgb.begin() + (size_t)(y + 1) * w * 3); }
    std::vector<uint8_t> z = {0x78, 0x01}; uint32_t a1 = 1, a2 = 0; for (uint8_t c : raw) { a1 = (a1 + c) % 65521; a2 = (a2 + a1) % 65521; }
    for (size_t o = 0; o < raw.size(); o += 65535) { const size_t n = std::min<size_t>(65535, raw.size() - o); z.push_back(o + n >= raw.size()); z.push_back(n & 255); z.push_back(n >> 8); z.push_back(~n & 255); z.push_back((~n >> 8) & 255); z.insert(z.end(), raw.begin() + o, raw.begin() + o + n); }
    uint32_t ad = (a2 << 16) | a1; for (int k = 3; k >= 0; k--) z.push_back(ad >> (k * 8));
    FILE* f = fopen(path, "wb"); if (!f) return; const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; fwrite(sig, 1, 8, f);
    auto chunk = [&](const char* t, const std::vector<uint8_t>& d) { uint8_t l[4] = {(uint8_t)(d.size() >> 24), (uint8_t)(d.size() >> 16), (uint8_t)(d.size() >> 8), (uint8_t)d.size()}; fwrite(l, 1, 4, f); fwrite(t, 1, 4, f); if (!d.empty()) fwrite(d.data(), 1, d.size(), f);
        uint32_t c = 0xFFFFFFFFu; for (int k = 0; k < 4; k++) c = crcT[(c ^ (uint8_t)t[k]) & 255] ^ (c >> 8); for (uint8_t b : d) c = crcT[(c ^ b) & 255] ^ (c >> 8); c ^= 0xFFFFFFFFu; uint8_t cc[4] = {(uint8_t)(c >> 24), (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c}; fwrite(cc, 1, 4, f); };
    chunk("IHDR", {(uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w, (uint8_t)(h >> 24), (uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h, 8, 2, 0, 0, 0}); chunk("IDAT", z); chunk("IEND", {}); fclose(f);
}
static bool gIntroBgSaved = false;
static void bgTexSet(GLuint& tex, const uint8_t* rgba, int W, int H) {
    if (!tex) glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
static GLuint gTitleImgTex = 0;   // patchMenu.5: fundo FIXO do titulo (assets/ui/titulo_bg.png) com parallax; sem o arquivo, o panorama 3D continua
static void introBgLoad() {   // inicio do loading: carrega os 3 fundos em cache (abertura, geracao de mundo, menu); cada um e uma paisagem diferente
    std::vector<uint8_t> px; int W = 0, H = 0;
    if (loadPNG("cache/terreno_abertura.png", px, W, H) && W >= 16 && H >= 16) bgTexSet(gIntroBgTex, px.data(), W, H);
    if (loadPNG("cache/terreno_load.png", px, W, H) && W >= 16 && H >= 16) bgTexSet(gLoadBgTex, px.data(), W, H);
    if (loadPNG("cache/menu_bg.png", px, W, H) && W >= 16 && H >= 16) bgTexSet(gMenuBgTex, px.data(), W, H);
    if (loadPNG("assets/ui/loading_bg.png", px, W, H) && W >= 16 && H >= 16) { bgTexSet(gIntroBgTex, px.data(), W, H); bgTexSet(gLoadBgTex, px.data(), W, H); }   // patchUI.1: fundo fixo (castelo na chuva) na abertura e no carregamento de mundo; tem prioridade sobre o cache de paisagens geradas (e evita gerar o panorama)
    if (getenv("BL_TITLE_IMG") && loadPNG("assets/ui/titulo_bg.png", px, W, H) && W >= 16 && H >= 16) bgTexSet(gTitleImgTex, px.data(), W, H);   /* patchBoot.1: fundo do titulo = panorama 3D girando (igual ao Minecraft); a foto so com BL_TITLE_IMG=1 */   // patchMenu.5
    if (loadPNG("assets/ui/gerando_bg.png", px, W, H) && W >= 16 && H >= 16) bgTexSet(gLoadBgTex, px.data(), W, H);   // patchLoad.2: fundo da GERACAO DE TERRENO (castelo no por do sol); so gLoadBgTex, tem prioridade sobre loading_bg.png e o cache. A abertura continua com loading_bg.png
}
static void bgCapture(int w, int h, const char* path, GLuint& tex) {   // le a tela, reduz para 480 px de largura, grava o PNG em cache e atualiza a textura
    std::vector<uint8_t> src((size_t)w * h * 4); glPixelStorei(GL_PACK_ALIGNMENT, 1); glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, src.data());
    const int ow = 480, oh = std::max(16, 480 * h / w); std::vector<uint8_t> o((size_t)ow * oh * 3), rgba((size_t)ow * oh * 4);
    for (int y = 0; y < oh; y++) for (int x = 0; x < ow; x++) { const int x0 = x * w / ow, x1 = std::max(x0 + 1, (x + 1) * w / ow), y0 = y * h / oh, y1 = std::max(y0 + 1, (y + 1) * h / oh); int c[3] = {0, 0, 0}, n = 0;
        for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) { const uint8_t* q = &src[((size_t)(h - 1 - yy) * w + xx) * 4]; c[0] += q[0]; c[1] += q[1]; c[2] += q[2]; n++; }
        for (int k = 0; k < 3; k++) { o[((size_t)y * ow + x) * 3 + k] = (uint8_t)(c[k] / n); rgba[((size_t)y * ow + x) * 4 + k] = (uint8_t)(c[k] / n); } rgba[((size_t)y * ow + x) * 4 + 3] = 255; }
    #ifdef _WIN32
    _mkdir("cache");
    #else
    mkdir("cache", 0755);
    #endif
    introPngWrite(path, o, ow, oh); bgTexSet(tex, rgba.data(), ow, oh);
}
static void introBgGrab(int w, int h) {   // no titulo, com o panorama borrado na tela (antes do menu): salva 1x por execucao
    if (gIntroBgSaved || splashActive() || w < 64 || h < 64) return; gIntroBgSaved = true; bgCapture(w, h, "cache/menu_bg.png", gMenuBgTex);
}
static void titleImgDraw(int w, int h) {   // patchMenu.5: imagem do titulo cobrindo a tela, zoom 1,08 e deslocamento suave pelo mouse/dedo (parallax)
    static float px = 0.f, py = 0.f; static Uint32 lt = 0; const Uint32 nt = SDL_GetTicks(); const float dt = lt ? std::min(.1f, (nt - lt) / 1000.f) : 0.f; lt = nt;
    if (w > 0 && h > 0) { const float tx = std::max(-1.f, std::min(1.f, ((float)gMX / w - .5f) * 2.f)), ty = std::max(-1.f, std::min(1.f, ((float)gMY / h - .5f) * 2.f)), k = std::min(1.f, dt * 4.f); px += (tx - px) * k; py += (ty - py) * k; }
    const float f = 1.f / 1.08f, m = (1.f - f) / 2.f, u0 = .5f - f / 2 + px * m, u1 = .5f + f / 2 + px * m, v0 = .5f - f / 2 + py * m, v1 = .5f + f / 2 + py * m;
    blViewport(w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdDisable(GL_BLEND);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, gTitleImgTex); glColor4f(.7f, .7f, .7f, 1.f); glBegin(GL_QUADS);   // 30% mais escuro para o logo/botoes lerem bem
    glTexCoord2f(u0, v0); glVertex2f(0, 0); glTexCoord2f(u1, v0); glVertex2f((float)w, 0); glTexCoord2f(u1, v1); glVertex2f((float)w, (float)h); glTexCoord2f(u0, v1); glVertex2f(0, (float)h); glEnd();
    glColor4f(1, 1, 1, 1);
}
static bool bgScreen(GLuint tex, int w, int h) {   // cobre a janela com um fundo em cache (opaco); false se nao ha cache
    if (!tex) return false; blViewport(w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdDisable(GL_BLEND); introBgDraw(tex, w, h, 1.f); glColor4f(1, 1, 1, 1); return true;
}
static GLuint gSplashMojang = 0; static bool gSplashOverlayMode = false; static float gSplashBlack = 0.f; static Uint32 gSplashT00 = 0; static bool gSplashLogoTried = false;
static void splashDraw(int w, int h, float prog, float bgA, float logoA, float barA) {   // patchIntro.3: layout do LoadingOverlay do Minecraft = fundo liso + logo no centro + barra branca com contorno em 83,25% da altura (sem porcentagem, sem fade-in preto)
    blViewport(w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdDisable(GL_TEXTURE_2D); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (bgA >= .999f) { glClearColor(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    glColor4f(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], bgA); glBegin(GL_QUADS); splashRect(0, 0, (float)w, (float)h); glEnd();
    if (!gSplashLogoTried) { gSplashLogoTried = true; gSplashMojang = mtLoad("gui/title/mojang.png", "assets/textures/gui/title/mojang.png", false); if (!gSplashMojang && !mtLogo) mtLogo = mtLoad("__x", "assets/textures/gui/logo.png", false); }   // patchIntro.4: mojang.png (do jar 1.12.2 / pack); sem ele, o logo do jogo no lugar
    if (logoA > 0.f && (gSplashMojang || mtLogo)) {
        const int G0 = guiScaleFor(w, h), sw0 = (w + G0 - 1) / G0, sh0 = (h + G0 - 1) / G0;   // ScaledResolution do 1.12.2: (largura escalada - 256) / 2
        const float lw = 256.f * G0, lh = gSplashMojang ? lw : floorf(lw * 382.f / 1024.f), lx = (float)(((sw0 - 256) / 2) * G0), ly = gSplashMojang ? (float)(((sh0 - 256) / 2) * G0) : floorf(h * .5f - lh * .5f);
        rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, gSplashMojang ? gSplashMojang : mtLogo); glColor4f(1, 1, 1, logoA);
        glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(lx, ly); glTexCoord2f(1, 0); glVertex2f(lx + lw, ly); glTexCoord2f(1, 1); glVertex2f(lx + lw, ly + lh); glTexCoord2f(0, 1); glVertex2f(lx, ly + lh); glEnd(); rdDisable(GL_TEXTURE_2D); }
    (void)prog; (void)barA;   // 1.12.2: sem barra de progresso na abertura
    if (gSplashBlack > 0.f) { rdDisable(GL_TEXTURE_2D); glColor4f(0, 0, 0, std::min(1.f, gSplashBlack)); glBegin(GL_QUADS); splashRect(0, 0, (float)w, (float)h); glEnd(); }   // fade-out final (splashHold)
    glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
}
static void splashAdvance(float target) {   // currentProgress = currentProgress*0.95 + actual*0.05 por quadro; aqui ajustado pelo tempo real (60 fps = 1 quadro)
    const Uint32 now = SDL_GetTicks(); const float dt = std::min(.1f, (now - gSplashLast) / 1000.f); gSplashLast = now;
    const float k = 1.f - powf(.85f, dt * 60.f);
    gSplashCur = std::max(0.f, std::min(1.f, gSplashCur + (target - gSplashCur) * k));
}
static void splashInit() {   // chamar logo apos criar o contexto GL (antes de qualquer carga pesada)
    gSplashLast = SDL_GetTicks(); gSplashT00 = gSplashLast; introBgLoad();
}
static void splashFrame(SDL_Window* win, float target) {   // 1 quadro: eventos, progresso, desenho. Devolve a textura ligada (o resto do init depende dela)
    SDL_Event ev; while (SDL_PollEvent(&ev)) if (ev.type == SDL_QUIT) { logInfo("Stopping!"); logFinalize("fechado durante o loading"); _Exit(0); }
    splashAdvance(target);
    int w, h; SDL_GetWindowSize(win, &w, &h); GLint prevTex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    splashDraw(w, h, gSplashCur, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex); SDL_GL_SwapWindow(win);
}
// ---- progresso continuo: a carga e sincrona, entao (1) cada etapa concluida define o alvo e (2) durante a etapa seguinte os loops pesados chamam
// splashPoll(), que desenha quadros a ~60 fps com o alvo "escorregando" devagar em direcao a proxima etapa (nunca passa dela). Assim a barra nunca fica parada.
static SDL_Window* gSplashWin = nullptr; static bool gTitleNoBlack = false;
static float gStageFrom = 0.f, gStageTo = 0.f; static Uint32 gStageT0 = 0, gPollLast = 0;
static float splashCreepTarget() {   // alvo atual: sobe de gStageFrom para gStageTo com constante de tempo ~1,2 s (assintotico; nao chega a gStageTo)
    const float el = (SDL_GetTicks() - gStageT0) / 1000.f;
    return gStageFrom + (gStageTo - .005f - gStageFrom) * (1.f - expf(-el / .5f));
}
static void splashPoll() {   // barato quando nao ha loading; chamado dentro de loops de carga
    if (!gSplashLoading || !gSplashWin) return;
    const Uint32 now = SDL_GetTicks(); if (now - gPollLast < 16) return; gPollLast = now;
    splashFrame(gSplashWin, splashCreepTarget());
}
static void splashStep(SDL_Window* win, float target, float next) {   // etapa concluida: barra segue ate "target"; a proxima etapa vai ate "next"
    gSplashWin = win; gSplashLoading = true;
    const Uint32 t0 = SDL_GetTicks(); gSplashLast = t0; gStageFrom = target; gStageTo = std::max(next, target + .001f); gStageT0 = t0;
    do { const Uint32 f0 = SDL_GetTicks(); splashFrame(win, target + (gStageTo - target) * .1f); const Uint32 d = SDL_GetTicks() - f0; (void)d; } while (0);   /* patchBoot.1: 1 quadro por etapa, sem esperar 16 ms (eram ~10 etapas) */
    gPollLast = SDL_GetTicks();
}
// fim da carga: a barra termina de alcancar (max 1,2 s) e comeca a sequencia de saida (fases 2 e 3)
static bool gSplashNoBar = false;   // true depois da abertura (patchIntro.2): a saida do overlay nao repete barra/logo
static void splashHold(SDL_Window* win) {   // fim da carga: a abertura dura 5 s no total; a barra termina de encher ate la e a tela faz fade-out (0,45 s)
    gSplashLoading = false; const float DUR = 0.f, FO = 60.f;   /* patchFast.1: era 5000 ms fixos */ const Uint32 now0 = SDL_GetTicks(); const float R0 = std::max(60.f, DUR - (float)(now0 - gSplashT00)), p0 = gSplashCur;
    for (;;) {
        SDL_Event ev; while (SDL_PollEvent(&ev)) if (ev.type == SDL_QUIT) { logInfo("Stopping!"); logFinalize("fechado durante a abertura"); _Exit(0); }
        const Uint32 f0 = SDL_GetTicks(); const float el = (float)(f0 - now0); if (el >= R0) break;
        const float k = el / R0; const float prog = p0 + (1.f - p0) * (1.f - (1.f - k) * (1.f - k)); gSplashBlack = 0.f; (void)FO;
        int w, h; SDL_GetWindowSize(win, &w, &h); GLint prevTex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        splashDraw(w, h, prog, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex); SDL_GL_SwapWindow(win);
        const Uint32 d = SDL_GetTicks() - f0; if (d < 16) SDL_Delay(16 - d);
    }
    gSplashCur = 1.f; gSplashBlack = 0.f; gSplashNoBar = true;
}
static void splashFinish(SDL_Window* win) {
    gSplashLoading = false;
    gSplashFadeT0 = 0; if (gSplashTex) { glDeleteTextures(1, &gSplashTex); gSplashTex = 0; }   // patchIntro.4: 1.12.2 troca direto para o titulo (sem overlay em fade); o fade-in do panorama/logo e do proprio titulo
}
#ifdef __ANDROID__
static const float SPLASH_TAIL = 0.2f;   // celular: saida do loading bem mais curta (padrao 1 s + 1 s)
#else
static const float SPLASH_TAIL = .1f;   // patchBoot.1: era .25 (overlay bloqueava cliques ~0,5 s)
#endif
static inline bool splashActive() { return gSplashFadeT0 != 0; }   // overlay ainda na tela (o menu nao recebe cliques)
static void splashOverlay(int w, int h) {   // desenhar por ultimo no quadro, depois do menu/jogo
    if (!gSplashFadeT0) return;
    const float f = (SDL_GetTicks() - gSplashFadeT0) / 1000.f / SPLASH_TAIL;
    if (f >= 2.f) { gSplashFadeT0 = 0; if (gSplashTex) { glDeleteTextures(1, &gSplashTex); gSplashTex = 0; } return; }
    splashAdvance(1.f);
    GLint prevTex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    gSplashOverlayMode = gSplashNoBar;
    if (f < 1.f) splashDraw(w, h, gSplashCur, 1.f, 1.f, gSplashNoBar ? 0.f : 1.f - f);   // fase 2: tela solida, so a barra some
    else { const float k = 1.f - (f - 1.f); splashDraw(w, h, gSplashCur, k, k, 0.f); }   // fase 3: menu aparece por baixo, fundo + logo em fade-out
    gSplashOverlayMode = false;
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex);
}
