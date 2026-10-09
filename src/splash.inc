// ================= tela de loading estilo Mojang (LoadingOverlay do Minecraft) =================
// Mesma sequencia do Minecraft: (1) barra avancando com currentProgress = currentProgress*0,95 + real*0,05 por quadro (a 60 fps);
// (2) carga pronta -> 1 s com a tela SOLIDA e a barra sumindo (alpha 1-f); (3) mais 1 s em que o MENU ja aparece por baixo (fundo, logo, botoes)
// enquanto o overlay inteiro (fundo + logo) faz fade-out. O menu e desenhado antes do overlay a cada quadro (splashOverlay, no loop principal).
static const float SPLASH_BG[3] = {0.031f, 0.039f, 0.020f};   // #080A05 (quase preto, tom amarelo-oliva da referencia); troque aqui para mudar o fundo
static GLuint gSplashTex = 0; static int gSplashTW = 512, gSplashTH = 180;
static float gSplashCur = 0.f;                    // progresso suavizado (o que a barra mostra)
static Uint32 gSplashLast = 0, gSplashFadeT0 = 0; // gSplashFadeT0 != 0 -> carga pronta (fases 2 e 3)
static void splashRect(float x0, float y0, float x1, float y1) { glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1); }
// bgA = alpha do fundo, logoA = alpha do logo, barA = alpha da barra (0 = nao desenha)
static void splashDraw(int w, int h, float prog, float bgA, float logoA, float barA) {
    int sc = 1; while (w / (sc + 1) >= 320 && h / (sc + 1) >= 240) sc++;   // escala "Auto" da GUI do Java
    const float gw = (float)w / sc, gh = (float)h / sc;
    blViewport(w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, gw, gh, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (bgA >= .999f) { glClearColor(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    glDisable(GL_TEXTURE_2D); glColor4f(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], bgA); glBegin(GL_QUADS); splashRect(0, 0, gw, gh); glEnd();
    const float e = std::min(gw * .75f, gh), cx = floorf(gw * .5f), cy = floorf(gh * .5f);   // e = largura da barra (Minecraft: min(75% da largura, altura))
    if (gSplashTex && logoA > 0.f) {   // logo no lugar da Mojang: centro da tela, levemente acima como no original
        const float lw = e * .94f, lh = lw * gSplashTH / gSplashTW, ly = cy - e * .022f;
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, gSplashTex); glColor4f(1, 1, 1, logoA);
        glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(cx - lw / 2, ly - lh / 2); glTexCoord2f(1, 0); glVertex2f(cx + lw / 2, ly - lh / 2);
        glTexCoord2f(1, 1); glVertex2f(cx + lw / 2, ly + lh / 2); glTexCoord2f(0, 1); glVertex2f(cx - lw / 2, ly + lh / 2); glEnd(); glDisable(GL_TEXTURE_2D);
    }
    if (barA > 0.f) {   // drawProgressBar do Minecraft: contorno de 1 px, vao de 1 px, preenchimento; y central = 83,25% da altura, 10 px de altura
        const float o = floorf(e * .5f), x0 = cx - o, x1 = cx + o, y0 = floorf(gh * .8325f) - 5, y1 = y0 + 10;
        const float fill = ceilf((x1 - x0 - 2) * std::max(0.f, std::min(1.f, prog)) * sc) / sc;   // passo de 1 pixel real (mais suave que 1 px de GUI)
        glColor4f(1, 1, 1, barA); glBegin(GL_QUADS);
        if (fill > 0.f) splashRect(x0 + 2, y0 + 2, x0 + fill, y1 - 2);   // igual ao Minecraft: fill(x0+2, y0+2, x0+i, y1-2) -> com 100% para em x1-2, dentro do contorno
        splashRect(x0 + 1, y0, x1 - 1, y0 + 1); splashRect(x0 + 1, y1 - 1, x1 - 1, y1); splashRect(x0, y0, x0 + 1, y1); splashRect(x1 - 1, y0, x1, y1);
        glEnd();
    }
    glColor4f(1, 1, 1, 1); glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
static void splashAdvance(float target) {   // currentProgress = currentProgress*0.95 + actual*0.05 por quadro; aqui ajustado pelo tempo real (60 fps = 1 quadro)
    const Uint32 now = SDL_GetTicks(); const float dt = std::min(.1f, (now - gSplashLast) / 1000.f); gSplashLast = now;
    const float k = 1.f - powf(.95f, dt * 60.f);
    gSplashCur = std::max(0.f, std::min(1.f, gSplashCur + (target - gSplashCur) * k));
}
static void splashInit() {   // chamar logo apos criar o contexto GL (antes de qualquer carga pesada)
    std::vector<uint8_t> px; int w = 0, h = 0;
    if (loadPNG("assets/textures/gui/pie_dev.png", px, w, h)) { gSplashTW = w; gSplashTH = h; loadTexturePNG("assets/textures/gui/pie_dev.png", gSplashTex, true); }
    else logWarn("Logo da tela de loading nao encontrada: assets/textures/gui/pie_dev.png");
    gSplashLast = SDL_GetTicks();
}
static void splashFrame(SDL_Window* win, float target) {   // 1 quadro: eventos, progresso, desenho. Devolve a textura ligada (o resto do init depende dela)
    SDL_Event ev; while (SDL_PollEvent(&ev)) if (ev.type == SDL_QUIT) { logInfo("Stopping!"); logFinalize("fechado durante o loading"); _Exit(0); }
    splashAdvance(target);
    int w, h; SDL_GetWindowSize(win, &w, &h); GLint prevTex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    splashDraw(w, h, gSplashCur, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex); SDL_GL_SwapWindow(win);
}
// ---- progresso continuo: a carga e sincrona, entao (1) cada etapa concluida define o alvo e (2) durante a etapa seguinte os loops pesados chamam
// splashPoll(), que desenha quadros a ~60 fps com o alvo "escorregando" devagar em direcao a proxima etapa (nunca passa dela). Assim a barra nunca fica parada.
static SDL_Window* gSplashWin = nullptr; static bool gSplashLoading = false, gTitleNoBlack = false;
static float gStageFrom = 0.f, gStageTo = 0.f; static Uint32 gStageT0 = 0, gPollLast = 0;
static float splashCreepTarget() {   // alvo atual: sobe de gStageFrom para gStageTo com constante de tempo ~1,2 s (assintotico; nao chega a gStageTo)
    const float el = (SDL_GetTicks() - gStageT0) / 1000.f;
    return gStageFrom + (gStageTo - .005f - gStageFrom) * (1.f - expf(-el / 1.2f));
}
static void splashPoll() {   // barato quando nao ha loading; chamado dentro de loops de carga
    if (!gSplashLoading || !gSplashWin) return;
    const Uint32 now = SDL_GetTicks(); if (now - gPollLast < 16) return; gPollLast = now;
    splashFrame(gSplashWin, splashCreepTarget());
}
static void splashStep(SDL_Window* win, float target, float next) {   // etapa concluida: barra segue ate "target"; a proxima etapa vai ate "next"
    gSplashWin = win; gSplashLoading = true;
    const Uint32 t0 = SDL_GetTicks(); gSplashLast = t0; gStageFrom = target; gStageTo = std::max(next, target + .001f); gStageT0 = t0;
    do { const Uint32 f0 = SDL_GetTicks(); splashFrame(win, target + (gStageTo - target) * .1f); const Uint32 d = SDL_GetTicks() - f0; if (d < 16) SDL_Delay(16 - d); } while (SDL_GetTicks() - t0 < 50);
    gPollLast = SDL_GetTicks();
}
// fim da carga: a barra termina de alcancar (max 1,2 s) e comeca a sequencia de saida (fases 2 e 3)
static void splashFinish(SDL_Window* win) {
    gSplashLoading = false;
    const Uint32 t0 = SDL_GetTicks();
    while (gSplashCur < .97f && SDL_GetTicks() - t0 < 1200) { const Uint32 f0 = SDL_GetTicks(); splashFrame(win, 1.f); const Uint32 d = SDL_GetTicks() - f0; if (d < 16) SDL_Delay(16 - d); }
    gSplashFadeT0 = SDL_GetTicks();
}
#ifdef __ANDROID__
static const float SPLASH_TAIL = 0.35f;   // celular: saida do loading bem mais curta (padrao 1 s + 1 s)
#else
static const float SPLASH_TAIL = 1.f;
#endif
static inline bool splashActive() { return gSplashFadeT0 != 0; }   // overlay ainda na tela (o menu nao recebe cliques)
static void splashOverlay(int w, int h) {   // desenhar por ultimo no quadro, depois do menu/jogo
    if (!gSplashFadeT0) return;
    const float f = (SDL_GetTicks() - gSplashFadeT0) / 1000.f / SPLASH_TAIL;
    if (f >= 2.f) { gSplashFadeT0 = 0; if (gSplashTex) { glDeleteTextures(1, &gSplashTex); gSplashTex = 0; } return; }
    splashAdvance(1.f);
    GLint prevTex = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    if (f < 1.f) splashDraw(w, h, gSplashCur, 1.f, 1.f, 1.f - f);   // fase 2: tela solida, so a barra some
    else { const float k = 1.f - (f - 1.f); splashDraw(w, h, gSplashCur, k, k, 0.f); }   // fase 3: menu aparece por baixo, fundo + logo em fade-out
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex);
}
