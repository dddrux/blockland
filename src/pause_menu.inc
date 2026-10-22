// ================= menu de pausa (GuiIngameMenu do Minecraft Java, em portugues) =================
// Layout em unidades da GUI (escala inteira, igual ao Java):
//   titulo "Menu do jogo" em y=40; botoes 200x20 centralizados:
//   Voltar para o jogo (h/4+8) | Conquistas + Estatisticas, 98x20 (h/4+32) | Opcoes... (h/4+80) | Salvar e sair para o titulo (h/4+104)
// Apenas "Voltar para o jogo" funciona; os demais sao placeholders (hover igual ao Java, clique nao faz nada).
// Textura dos botoes: Rethoughted GUI (assets/textures/gui/rt/button*.png); reserva: widgets.png / genWidgetsFallback().
enum { PB_NONE = -1, PB_BACK = 0, PB_ACH, PB_STATS, PB_LAN, PB_SPLIT, PB_SPLITMODE, PB_SKIN2, PB_OPT, PB_SAVE, PB_N };
static bool splitOn(); static const char* splitLabel(); static const char* splitModeLabel(); static const char* netLanLabel(); static bool netLanBusy();   // netlan.inc
static int uTextW(const char* t) { int n = 0; for (const char* p = t; *p;) { int f, a; glyphMetrics(u8next(p), f, a); n += a; } return n; }
static long gStBroken = 0, gStPlaced = 0; static Uint32 gStT0 = 0;   // estatisticas da sessao (Esc > Estatisticas / Conquistas)
struct PauseBtn { int x, y, w, h; const char* label; };
static int gPauseTitleY = 34;   // y do titulo "Menu do jogo" (sobe quando o menu precisa se comprimir)
static void pauseLayout(int w, int h, PauseBtn* b) {
    const int g = guiScaleFor(w, h), sw = (w + g - 1) / g, sh = (h + g - 1) / g;
    const int cx = sw / 2, top = sh / 4;
    const bool sp = splitOn(); const int nRows = sp ? 7 : 6;   // linhas: voltar | conquistas+stats | LAN | split+modo | [skin 2] | opcoes | salvar
    int y0 = top + 10, pitch = 24, gap = 8; gPauseTitleY = 34;   // gap = respiro entre grupos (voltar | social | opcoes/sair)
    if (y0 + (nRows - 1) * pitch + 2 * gap + 20 > sh - 4) {   // nao cabe (tela pequena): comprime e centraliza
        pitch = 22; gap = 4; const int total = (nRows - 1) * pitch + 2 * gap + 20;
        y0 = std::max(24, (sh - total) / 2 + 8); if (y0 + total > sh - 4) y0 = std::max(16, sh - 4 - total);
        gPauseTitleY = std::max(4, y0 - 20);
    }
    auto Y = [&](int r) { return y0 + r * pitch + (r >= 1 ? gap : 0) + (r >= nRows - 2 ? gap : 0); };
    b[PB_BACK]  = {cx - 100, Y(0), 200, 20, "Voltar para o jogo"};
    b[PB_ACH]   = {cx - 100, Y(1),  98, 20, "Conquistas"};
    b[PB_STATS] = {cx + 2,   Y(1),  98, 20, "Estatísticas"};
    b[PB_LAN]   = {cx - 100, Y(2), 200, 20, netLanLabel()};   // Abrir para LAN (como o Minecraft)
    b[PB_SPLIT] = {cx - 100, Y(3),  98, 20, splitLabel()}; b[PB_SPLITMODE] = {cx + 2, Y(3), 98, 20, splitModeLabel()};   // Tela dividida (split screen) ligada/desligada
    b[PB_SKIN2] = sp ? PauseBtn{cx - 100, Y(4), 200, 20, "Nomes e skins do split..."} : PauseBtn{0, 0, 0, 0, ""};
    b[PB_OPT]   = {cx - 100, Y(nRows - 2), 200, 20, "Opções..."};
    b[PB_SAVE]  = {cx - 100, Y(nRows - 1), 200, 20, "Salvar e sair para o título"};
}
static int pauseHit(int w, int h, int mx, int my) {   // botao sob o mouse (coordenadas de janela) ou PB_NONE
    PauseBtn b[PB_N]; pauseLayout(w, h, b);
    const int g = guiScaleFor(w, h), ux = mx / g, uy = my / g;
    for (int i = 0; i < PB_N; i++) if (ux >= b[i].x && ux < b[i].x + b[i].w && uy >= b[i].y && uy < b[i].y + b[i].h) return i;
    return PB_NONE;
}
// GuiButton.drawButton. Rethoughted GUI: nove fatias 200x20 (bordas 20/4/20/4) em rt/button*.png (normal, destacado, desativado).
// Sem as texturas, cai no widgets.png do Java (metade esquerda + metade direita, linha v = 46 + 20*estado).
static void pauseButton(const PauseBtn& b, int state) {
    if (state != 2) gBtnReg(OX + (b.x + b.w / 2.f) * G, OY + (b.y + b.h / 2.f) * G, G);
    if (rtBtn[0]) {
        uNine(rtBtn[state == 2 ? 2 : state == 1 ? 1 : 0], 200, 20, 20, 4, 20, 4, (float)b.x, (float)b.y, (float)b.w, (float)b.h, gUiA);
    } else {
        const float x = (float)(b.x * G), y = (float)(b.y * G), hw = (float)(b.w / 2), v0 = (46.f + 20.f * state) / 256.f, v1 = (66.f + 20.f * state) / 256.f;
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, guiWidgetsTex); glColor4f(1, 1, 1, 1);
        quad(x, y, hw * G, 20.f * G, 0.f, v0, hw / 256.f, v1);
        quad(x + hw * G, y, (b.w - hw) * G, 20.f * G, (200.f - hw) / 256.f, v0, 200.f / 256.f, v1);
    }
    const unsigned col = state == 2 ? 0xA0A0A0 : rtBtn[0] ? 0xFFFFFF : state == 1 ? 0xFFFFA0 : 0xE0E0E0;   // Rethoughted: texto branco (o realce ja vem da textura)
    int n = 0; for (const char* p = b.label; *p;) { int f, a; glyphMetrics(u8next(p), f, a); n += a; }   // largura do Java (com o espaco final)
    const int ux = b.x + b.w / 2 - n / 2;
    uText((float)ux, (float)(b.y + (20 - 8) / 2), b.label, col, true);
}
static void drawPauseMenu(int w, int h, int mx, int my) {
    glViewport(gVpX, gVpY, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0;
    // drawDefaultBackground: degrade 0xC0101010 -> 0xD0101010
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 192 / 255.f); glVertex2f(0, 0); glVertex2f((float)w, 0);
    glColor4f(16 / 255.f, 16 / 255.f, 16 / 255.f, 208 / 255.f); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    // titulo centralizado (drawCenteredString, y = 40, branco com sombra)
    PauseBtn b[PB_N]; pauseLayout(w, h, b);
    const int hov = pauseHit(w, h, mx, my);
    for (int i = 0; i < PB_N; i++) if (b[i].w) pauseButton(b[i], (i == PB_LAN && netLanBusy()) ? 2 : i == hov ? 1 : 0);
    glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

// ================= tela de morte (GuiGameOver do Minecraft 1.12) =================
// Fundo em degradê 0x60500000 -> 0xA0803030; título "Você morreu!" em escala 2 (y=30); causa (y=85); "Pontuação: N" com N amarelo (y=100);
// botões 200x20: "Renascer" (h/4+72) e "Menu principal" (h/4+96).
static void deathLayout(int w, int h, PauseBtn* b) {
    const int g = guiScaleFor(w, h), sw = (w + g - 1) / g, sh = (h + g - 1) / g, cx = sw / 2, top = sh / 4;
    b[0] = {cx - 100, top + 72, 200, 20, gHard ? "Modo espectador" : "Renascer"}; b[1] = {cx - 100, top + 96, 200, 20, "Menu principal"};
}
static int deathHit(int w, int h, int mx, int my) {
    PauseBtn b[2]; deathLayout(w, h, b); const int g = guiScaleFor(w, h), ux = mx / g, uy = my / g;
    for (int i = 0; i < 2; i++) if (ux >= b[i].x && ux < b[i].x + b[i].w && uy >= b[i].y && uy < b[i].y + b[i].h) return i;
    return -1;
}
static void drawDeathScreen(int w, int h, int mx, int my) {
    glViewport(gVpX, gVpY, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0; const int sw = (w + G - 1) / G;
    glDisable(GL_TEXTURE_2D); glBegin(GL_QUADS);
    glColor4f(0x50 / 255.f, 0, 0, 0x60 / 255.f); glVertex2f(0, 0); glVertex2f((float)w, 0);
    glColor4f(0x80 / 255.f, 0x30 / 255.f, 0x30 / 255.f, 0xA0 / 255.f); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    glEnd(); glEnable(GL_TEXTURE_2D);
    { const char* t = "Você morreu!"; glPushMatrix(); glScalef(2.f, 2.f, 1.f); uText((float)(sw / 4) - uTextW(t) / 2.f, 30.f, t, 0xFFFFFF, true); glPopMatrix(); }
    { const char* t = gDeathWhy.c_str(); uText((float)(sw / 2 - uTextW(t) / 2), 85.f, t, 0xFFFFFF, true); }
    { const char* a = "Pontuação: "; const int wa = uTextW(a), wt = wa + uTextW("0"), x = sw / 2 - wt / 2;
      uText((float)x, 100.f, a, 0xFFFFFF, true); uText((float)(x + wa), 100.f, "0", 0xFFFF55, true); }
    PauseBtn b[2]; deathLayout(w, h, b); const int hov = deathHit(w, h, mx, my);
    for (int i = 0; i < 2; i++) pauseButton(b[i], i == hov ? 1 : 0);
    glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
