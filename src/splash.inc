// ================= tela de loading estilo Mojang (LoadingOverlay do Minecraft) =================
// Mesma sequencia do Minecraft: (1) barra avancando com currentProgress = currentProgress*0,95 + real*0,05 por quadro (a 60 fps);
// (2) carga pronta -> 1 s com a tela SOLIDA e a barra sumindo (alpha 1-f); (3) mais 1 s em que o MENU ja aparece por baixo (fundo, logo, botoes)
// enquanto o overlay inteiro (fundo + logo) faz fade-out. O menu e desenhado antes do overlay a cada quadro (splashOverlay, no loop principal).
static const float SPLASH_BG[3] = {0.031f, 0.039f, 0.020f};   // #080A05 (quase preto, tom amarelo-oliva da referencia); troque aqui para mudar o fundo
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
// bgA = alpha do fundo, logoA = (ignorado, sem logo), barA = alpha da barra e do texto (0 = nao desenha)
static void splashDraw(int w, int h, float prog, float bgA, float logoA, float barA) {
    blViewport(w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (bgA >= .999f) { glClearColor(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    glColor4f(SPLASH_BG[0], SPLASH_BG[1], SPLASH_BG[2], bgA); glBegin(GL_QUADS); splashRect(0, 0, (float)w, (float)h); glEnd();
    if (barA > 0.f) {
        const int px = std::max(1, std::min(h / 220, w / 400)) + (h >= 700 ? 1 : 0); const float bw = floorf(w * .8f), x0 = floorf((w - bw) / 2), bh = (float)(px * 3), y1 = (float)h - px * 14, y0 = y1 - bh;
        const float fill = floorf((bw - 2 * px) * std::max(0.f, std::min(1.f, prog)));
        glColor4f(.35f, .35f, .35f, barA); glBegin(GL_QUADS); splashRect(x0, y0, x0 + bw, y0 + px); splashRect(x0, y1 - px, x0 + bw, y1); splashRect(x0, y0, x0 + px, y1); splashRect(x0 + bw - px, y0, x0 + bw, y1); glEnd();   // contorno
        glColor4f(.33f, 1.f, .33f, barA); glBegin(GL_QUADS); if (fill > 0.f) splashRect(x0 + px, y0 + px, x0 + px + fill, y1 - px); glEnd();   // preenchimento verde
        const int shown = std::min(gSplashLogN, 6); const float lh = px * 10.f;   // ultimos arquivos: o mais novo em branco logo acima da barra, os antigos sobem e apagam
        for (int k = 0; k < shown; k++) { const std::string& n = gSplashLog[(gSplashLogN - 1 - k) % 7]; const float a = barA * (k == 0 ? 1.f : .55f - k * .08f); if (a <= 0.f) continue;
            std::string line = "loaded: " + n; const float tw = line.size() * 6.f * px; splashText(floorf(x0), y0 - px * 6 - (k + 1) * lh, px, line, k ? .6f : 1.f, k ? .6f : 1.f, k ? .6f : 1.f, a); (void)tw; }
        char pc[16]; snprintf(pc, sizeof pc, "%d%%", (int)(std::max(0.f, std::min(1.f, prog)) * 100.f + .5f)); const std::string ps = pc; splashText(x0 + bw - ps.size() * 6.f * px, y0 - px * 6 - lh, px, ps, .75f, .75f, .75f, barA);
    }
    glColor4f(1, 1, 1, 1); glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
static void splashAdvance(float target) {   // currentProgress = currentProgress*0.95 + actual*0.05 por quadro; aqui ajustado pelo tempo real (60 fps = 1 quadro)
    const Uint32 now = SDL_GetTicks(); const float dt = std::min(.1f, (now - gSplashLast) / 1000.f); gSplashLast = now;
    const float k = 1.f - powf(.85f, dt * 60.f);
    gSplashCur = std::max(0.f, std::min(1.f, gSplashCur + (target - gSplashCur) * k));
}
static void splashInit() {   // chamar logo apos criar o contexto GL (antes de qualquer carga pesada)
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
    do { const Uint32 f0 = SDL_GetTicks(); splashFrame(win, target + (gStageTo - target) * .1f); const Uint32 d = SDL_GetTicks() - f0; if (d < 16) SDL_Delay(16 - d); } while (SDL_GetTicks() - t0 < 16);
    gPollLast = SDL_GetTicks();
}
// fim da carga: a barra termina de alcancar (max 1,2 s) e comeca a sequencia de saida (fases 2 e 3)
static void splashFinish(SDL_Window* win) {
    gSplashLoading = false;
    const Uint32 t0 = SDL_GetTicks();
    while (gSplashCur < .97f && SDL_GetTicks() - t0 < 350) { const Uint32 f0 = SDL_GetTicks(); splashFrame(win, 1.f); const Uint32 d = SDL_GetTicks() - f0; if (d < 16) SDL_Delay(16 - d); }
    gSplashFadeT0 = SDL_GetTicks();
}
#ifdef __ANDROID__
static const float SPLASH_TAIL = 0.2f;   // celular: saida do loading bem mais curta (padrao 1 s + 1 s)
#else
static const float SPLASH_TAIL = .25f;
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
