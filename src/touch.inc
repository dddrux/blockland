// ---------- HUD touch estilo Minecraft PE (liga/desliga com F8) ----------
// Setas = andar (os cantos andam na diagonal) | losango central = agachar (liga/desliga; voando = descer) | losango da direita = pular (voando = subir)
// Arrastar na tela = olhar | toque rápido = colocar bloco | segurar parado = quebrar | botões do topo = câmera, chat, pausa | "..." = inventário
// Funciona com dedos (multitoque) e com o mouse (1 ponteiro).
static bool touchHud = false, tvF = false, tvB = false, tvL = false, tvR = false, tvJump = false, tvSneakT = false, tvSprint = false;
static bool touchBreak = false, touchPlace = false, tAct[5] = {false, false, false, false, false};
static int touchSel = -1; static float tDX = 0, tDY = 0;
struct TPt { bool on = false, moved = false, brk = false; int64_t id = 0; int kind = 0, idx = 0; float sx = 0, sy = 0, lx = 0, ly = 0; Uint32 t0 = 0; };
static TPt tpts[8];
struct TRect { float x, y, w, h; bool in(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };
struct TLay { float s, cx, cy; int G; TRect top[4], inv, jump, hot, hotHit, sneak; };
static void relMouse(bool g) { SDL_SetRelativeMouseMode(g && !touchHud ? SDL_TRUE : SDL_FALSE); }   // touch: cursor sempre livre
static TLay touchLay(int w, int h) {
    TLay L; L.s = std::max(64.f, std::min(w, h) * .16f); const float m = L.s * .3f;
    L.cx = m + 1.5f * L.s; L.cy = h - m - 1.5f * L.s;
    const float b = L.s * .7f, gap = b * .15f, tx = w / 2.f - (4 * b + 3 * gap) / 2.f;
    for (int i = 0; i < 4; i++) L.top[i] = {tx + i * (b + gap), m * .5f, b, b};
    L.G = guiScaleFor(w, h); const int sw = (w + L.G - 1) / L.G, sh = (h + L.G - 1) / L.G;
    L.hot = {(float)(((sw - 182) / 2) * L.G), (float)((sh - 22) * L.G), 182.f * L.G, 22.f * L.G};
    L.hotHit = {L.hot.x, L.hot.y - 12.f * L.G, L.hot.w, L.hot.h + 12.f * L.G};
    const float ib = 22.f * L.G;   // botão do inventário = mesmo tamanho da hotbar (22 unidades)
    L.inv = {L.hot.x + L.hot.w + 2.f * L.G, L.hot.y, ib, ib};
    const float j = L.s * 1.25f; L.jump = {w - m * 1.5f - j, L.cy - j / 2, j, j}; { const float sb = L.s * .9f; L.sneak = {L.jump.x - sb - m, L.jump.y + L.jump.h - sb, sb, sb}; }   // agachar: botao separado ao lado do pular (como o Bedrock)
      // pular: lado direito, alinhado ao centro do direcional
    return L;
}
static void touchRecalc(const TLay& L) {
    tvF = tvB = tvL = tvR = tvJump = tvSprint = false;
    for (const TPt& p : tpts) {
        if (!p.on) continue;
        if (p.kind == 2) tvJump = true;
        if (p.kind == 1) {   // joystick analogico (Bedrock): vetor normalizado pelo raio da base; zona morta 18%; 8 direcoes; borda da frente = correr
            const float R = L.s * 1.5f, vx = (p.lx - L.cx) / R, vy = (p.ly - L.cy) / R, m = hypotf(vx, vy);
            if (m < .18f) continue;
            const float t = .38f * m;
            tvF |= vy < -t; tvB |= vy > t; tvL |= vx < -t; tvR |= vx > t;
            if (m >= .95f && vy < -.7f * m) tvSprint = true;
        }
    }
}
static float touchBrkProg = 0, touchBrkX = 0, touchBrkY = 0, touchPlaceX = 0, touchPlaceY = 0;
static bool touchRing = false;   // trilho do anel visivel (segurando, antes de comecar a quebrar)
static bool touchAim = false;   // true = dedo segurando: o alvo é o bloco sob o dedo (como o MCPE sem controles divididos)   // 0..1: círculo de quebra (segurar / minerar)
static void touchReset() { touchBrkProg = 0; for (TPt& p : tpts) p = TPt(); tvF = tvB = tvL = tvR = tvJump = tvSprint = touchBreak = touchPlace = false; tDX = tDY = 0; touchSel = -1; for (bool& a : tAct) a = false; }
static void touchFrame(Uint32 now) {   // segurar parado => quebrar (alvo = bloco sob o dedo)
    touchBreak = false; touchBrkProg = 0; touchAim = false; touchRing = false;
    for (TPt& p : tpts) if (p.on && p.kind == 4) {
        if (!p.moved && now - p.t0 > 180) { touchRing = true; touchBrkX = p.lx; touchBrkY = p.ly; }   // Bedrock: o anel surge sob o dedo logo que voce segura
        if (!p.moved && !p.brk && now - p.t0 > 350) p.brk = true;
        if (p.brk) { touchBreak = true; touchAim = true; touchBrkX = p.lx; touchBrkY = p.ly; touchBrkProg = std::min(1.f, (now - p.t0 - 350) / 300.f); }
    }
}
static bool touchEvent(const SDL_Event& e, int w, int h) {   // true = evento consumido
    int64_t id = -1; float x = 0, y = 0; int ph = -1;
    switch (e.type) {
        case SDL_FINGERDOWN: case SDL_FINGERMOTION: case SDL_FINGERUP:
            id = e.tfinger.fingerId; x = e.tfinger.x * w; y = e.tfinger.y * h; ph = e.type == SDL_FINGERDOWN ? 0 : e.type == SDL_FINGERUP ? 2 : 1; break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
            if (e.button.which == SDL_TOUCH_MOUSEID) return true;   // eco do dedo: já tratado como finger
            if (e.button.button != SDL_BUTTON_LEFT) return true;
            x = (float)e.button.x; y = (float)e.button.y; ph = e.type == SDL_MOUSEBUTTONDOWN ? 0 : 2; break;
        case SDL_MOUSEMOTION:
            if (e.motion.which == SDL_TOUCH_MOUSEID) return true;
            x = (float)e.motion.x; y = (float)e.motion.y; ph = 1; break;
        default: return false;
    }
    const TLay L = touchLay(w, h);
    TPt* p = nullptr; for (TPt& q : tpts) if (q.on && q.id == id) p = &q;
    if (ph == 0) {
        if (p) *p = TPt();
        for (TPt& q : tpts) if (!q.on) { p = &q; break; }
        if (!p) return true;
        *p = TPt(); p->on = true; p->id = id; p->sx = p->lx = x; p->sy = p->ly = y; p->t0 = SDL_GetTicks();
        bool hit = false;
        for (int i = 0; i < 4 && !hit; i++) if (L.top[i].in(x, y)) { tAct[i] = true; p->kind = 3; p->idx = i; hit = true; }
        if (!hit && L.inv.in(x, y)) { tAct[4] = true; p->kind = 3; p->idx = 4; hit = true; }
        if (!hit && L.hotHit.in(x, y)) { touchSel = std::max(0, std::min(8, (int)((x - L.hot.x) / (20.f * L.G)))); p->kind = 5; hit = true; }
        if (!hit && hypotf(x - L.cx, y - L.cy) < L.s * 2.1f) { p->kind = 1; hit = true; }   // joystick: zona de toque um pouco maior que a base visivel
        if (!hit && L.sneak.in(x, y)) { p->kind = 6; hit = true; tvSneakT = !tvSneakT; }
        if (!hit && L.jump.in(x, y)) { p->kind = 2; hit = true; }
        if (!hit) p->kind = 4;
    } else if (p) {
        if (ph == 1) {
            if (p->kind == 4) { tDX += x - p->lx; tDY += y - p->ly; if (hypotf(x - p->sx, y - p->sy) > 14.f) p->moved = true; }
            p->lx = x; p->ly = y;
        } else {
            if (p->kind == 4 && !p->moved && !p->brk && SDL_GetTicks() - p->t0 < 300) { touchPlace = true; touchPlaceX = p->lx; touchPlaceY = p->ly; }   // toque rápido
            *p = TPt();
        }
    }
    touchRecalc(L);
    return true;
}
// ---- desenho (tudo em pixel art: blocos quadrados de tamanho inteiro) ----
static void tPix(float cx, float cy, float cell, const char* const* rows, int nr, int nc, int ori, float g, float al = .95f) {
    const int W = (ori >= 2) ? nr : nc, H = (ori >= 2) ? nc : nr; const float x0 = floorf(cx - W * cell / 2), y0 = floorf(cy - H * cell / 2);
    for (int r = 0; r < nr; r++) for (int c = 0; c < nc; c++) {
        if (rows[r][c] != '#') continue;
        int gx = c, gy = r;   // ori: 0 cima, 1 baixo, 2 esquerda, 3 direita
        if (ori == 1) gy = nr - 1 - r; else if (ori == 2) { gx = r; gy = c; } else if (ori == 3) { gx = nr - 1 - r; gy = c; }
        flat(x0 + gx * cell, y0 + gy * cell, cell, cell, g, g, g, al);
    }
}
#define TP(var, ...) static const char* const var[] = {__VA_ARGS__}
TP(pTri, "...#...", "..###..", ".#####.", "#######");
TP(pDia, "....#....", "...###...", "..#####..", ".#######.", "#########", ".#######.", "..#####..", "...###...", "....#....");
TP(pDia2, "..#..", ".###.", "#####", ".###.", "..#..");
TP(pDUL, "######", "#####.", "####..", "###...", "##....", "#.....");   // seta diagonal (frente-esquerda)
TP(pDUR, "######", ".#####", "..####", "...###", "....##", ".....#");   // frente-direita
TP(pSneak, "...###...", "...###...", "#########", ".#######.", "..#####..", "...###...", "....#....", ".........", "#########");   // seta para baixo sobre uma barra (agachar)
TP(pJump, "....#....", "...###...", "..#####..", ".#######.", "#########", "...###...", "...###...", "...###...");   // seta para cima (pular / subir)
TP(pF3, "###.###", "#.....#", "##...##", "#.....#", "#...###");
TP(pSun, "....#....", ".#..#..#.", "..#####..", "#.#####.#", "..#####..", ".#..#..#.", "....#....");   // simbolo de luz (F7 = sombras)
static bool touchShadowOn();
TP(pChat, "###########", "###########", "###########", "###########", "###########", "..####.....", "..###......", "..##.......");
TP(pPause, "###...###", "###...###", "###...###", "###...###", "###...###", "###...###", "###...###", "###...###");
TP(pDots, "##.##.##");
TP(pX, "##.....##", "###...###", ".###.###.", "..#####..", "...###...", "..#####..", ".###.###.", "###...###", "##.....##");
static void tBtn(float x, float y, float w, float h, bool down) {   // botao de pedra translucido do MCPE: contorno escuro, bisel claro/escuro; apertado = mais claro e bisel invertido
    x = floorf(x); y = floorf(y); w = floorf(w); h = floorf(h);
    const float P = std::max(2.f, floorf(w / 22.f)), fv = down ? .88f : .66f, fa = down ? .92f : .58f;
    const float dk = .06f, da = .80f;
    flat(x + P, y, w - 2 * P, P, dk, dk, dk, da); flat(x + P, y + h - P, w - 2 * P, P, dk, dk, dk, da);   // contorno (cantos recortados)
    flat(x, y + P, P, h - 2 * P, dk, dk, dk, da); flat(x + w - P, y + P, P, h - 2 * P, dk, dk, dk, da);
    flat(x + P, y + P, w - 2 * P, h - 2 * P, fv, fv, fv, fa);
    const float hi = down ? 0.f : 1.f, lo = down ? 1.f : 0.f, ha = down ? .18f : .40f, la = down ? .30f : .32f;
    flat(x + P, y + P, w - 2 * P, P, hi, hi, hi, ha); flat(x + P, y + P, P, h - 2 * P, hi, hi, hi, ha);
    flat(x + P, y + h - 2 * P, w - 2 * P, P, lo, lo, lo, la); flat(x + w - 2 * P, y + P, P, h - 2 * P, lo, lo, lo, la);
}
static void tSetup(int w, int h) {
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static TRect touchCloseRect(int w, int h) { const TLay L = touchLay(w, h); const float b = L.s * .7f, m = L.s * .3f; return {w - b - m, m * .5f, b, b}; }
static void drawTouchClose(int w, int h) {   // botão "X" (só no modo celular) para sair do chat / inventário
    tSetup(w, h); const TRect r = touchCloseRect(w, h);
    tBtn(r.x, r.y, r.w, r.h, false); tPix(r.x + r.w / 2, r.y + r.h / 2, std::max(2.f, floorf(r.w / 14.f)), pX, 9, 9, 0, .15f);
    glEnable(GL_DEPTH_TEST);
}
static void drawTouch(int w, int h) {
    tSetup(w, h);
    const TLay L = touchLay(w, h); const float s = L.s, bs = floorf(s * .94f), o = floorf((s - bs) / 2), K = .15f, cell = std::max(2.f, floorf(bs / 16.f));
    bool pr[5] = {false, false, false, false, false}; for (const TPt& p : tpts) if (p.on && p.kind == 3) pr[p.idx] = true;
    {   // joystick estilo Bedrock: base e bolinha QUADRADAS ARREDONDADAS e PIXELADAS (celulas inteiras, canto escalonado); a bolinha segue o dedo "encaixada" na grade
        const float R = s * 1.5f; glDisable(GL_TEXTURE_2D);
        const float pc = std::max(2.f, floorf(s * 3.f / 16.f));   // 1 "pixel" do joystick; a base tem 16x16 pixels (~3 s) e a bolinha 6x6
        // quadrado n x n pixels com cantos arredondados de raio cr (so quads inteiros, sem circulos/triangulos)
        auto sq = [&](float cx, float cy, int n, int cr, float g, float al) {
            const float x0 = floorf(cx - n * pc / 2.f), y0 = floorf(cy - n * pc / 2.f); glColor4f(g, g, g, al); glBegin(GL_QUADS);
            for (int r = 0; r < n; r++) {
                const int d = std::min(r, n - 1 - r); int ind = 0;
                if (d < cr) { const float dy = cr - d - .5f; ind = cr - (int)floorf(sqrtf(cr * cr - dy * dy) + .5f); }
                const float xa = x0 + ind * pc, xb = x0 + (n - ind) * pc, ya = y0 + r * pc;
                glVertex2f(xa, ya); glVertex2f(xb, ya); glVertex2f(xb, ya + pc); glVertex2f(xa, ya + pc);
            }
            glEnd();
        };
        float kx = L.cx, ky = L.cy;
        for (const TPt& p : tpts) if (p.on && p.kind == 1) { float vx = (p.lx - L.cx) / R, vy = (p.ly - L.cy) / R; const float m = hypotf(vx, vy); if (m > 1.f) { vx /= m; vy /= m; } kx = L.cx + vx * R * .62f; ky = L.cy + vy * R * .62f; }
        kx = L.cx + roundf((kx - L.cx) / pc) * pc; ky = L.cy + roundf((ky - L.cy) / pc) * pc;   // movimento em passos de 1 pixel
        sq(L.cx, L.cy, 16, 4, .05f, .40f); sq(L.cx, L.cy, 14, 3, .62f, .30f);   // base: borda escura + miolo cinza translucido
        const float kg = tvSprint ? .95f : .75f;
        sq(kx, ky, 6, 2, .05f, .60f); sq(kx, ky, 4, 1, kg, .85f);               // bolinha: borda escura + miolo claro (correndo = mais claro)
        { const float x0 = floorf(kx - 2 * pc), y0 = floorf(ky - 2 * pc); glColor4f(1, 1, 1, .30f); glBegin(GL_QUADS); glVertex2f(x0 + pc, y0); glVertex2f(x0 + 3 * pc, y0); glVertex2f(x0 + 3 * pc, y0 + pc); glVertex2f(x0 + pc, y0 + pc); glEnd(); }   // brilho de 1 pixel no topo
        glEnable(GL_TEXTURE_2D);
    }
    { const TRect& r = L.sneak; tBtn(r.x, r.y, r.w, r.h, tvSneakT); tPix(r.x + r.w / 2, r.y + r.h / 2, std::max(2.f, floorf(r.w / 14.f)), pSneak, 9, 9, 0, tvSneakT ? .08f : K); }
    tBtn(L.jump.x, L.jump.y, L.jump.w, L.jump.h, tvJump); tPix(L.jump.x + L.jump.w / 2, L.jump.y + L.jump.h / 2, std::max(2.f, floorf(L.jump.w / 13.f)), pJump, 8, 9, 0, tvJump ? .08f : K);
    // topo: câmera, chat, pausa
    for (int i = 0; i < 4; i++) {
        const TRect& r = L.top[i]; tBtn(r.x, r.y, r.w, r.h, pr[i] || (i == 0 && touchShadowOn())); const float c2 = std::max(2.f, floorf(r.w / 14.f));
        if (i == 0) tPix(r.x + r.w / 2, r.y + r.h / 2, c2, pSun, 7, 9, 0, i == 0 && touchShadowOn() ? .95f : K);
        if (i == 1) tPix(r.x + r.w / 2, r.y + r.h / 2, c2, pChat, 8, 11, 0, K);
        if (i == 2) tPix(r.x + r.w / 2, r.y + r.h / 2, c2, pF3, 5, 7, 0, K);   // F3 = informacoes de debug
        if (i == 3) tPix(r.x + r.w / 2, r.y + r.h / 2, c2, pPause, 8, 9, 0, K);
    }
    {   // inventário: mesmo tamanho e visual da hotbar (fundo preto 60% + slot rebaixado)
        const TRect& r = L.inv; const float g = (float)L.G;
        flat(r.x, r.y, r.w, r.h, 0, 0, 0, .6f);
        const float bx = r.x + 2 * g, by = r.y + 2 * g, bw = 18 * g, v = pr[4] ? .30f : .43f;
        flat(bx, by, bw, bw, v, v, v, .9f); flat(bx, by, bw - g, g, .22f, .22f, .22f, .9f); flat(bx, by, g, bw - g, .22f, .22f, .22f, .9f);
        flat(bx + g, by + bw - g, bw - g, g, 1, 1, 1, .9f); flat(bx + bw - g, by + g, g, bw - g, 1, 1, 1, .9f);
        tPix(bx + bw / 2, by + bw / 2, g, pDots, 1, 8, 0, 1.f);
    }
    // anel de quebra do MCPE (sem mira): círculo fino e grande; o arco claro percorre o anel (sentido horário, a partir do topo) conforme quebra
    if (touchBrkProg > .01f || touchRing) {
        const float R = s * 1.05f, T = std::max(4.f, s * .08f), cx = touchBrkX, cy = touchBrkY; const int N = 96;
        glDisable(GL_TEXTURE_2D);
        for (int pass = 0; pass < 2; pass++) {
            if (pass && touchBrkProg <= .01f) break;   // so o trilho, sem arco, ate comecar a quebrar
            const int n = pass ? std::max(1, (int)(N * touchBrkProg)) : N; if (pass) glColor4f(.88f, .92f, 1.f, .95f); else glColor4f(.16f, .22f, .5f, .9f);
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= n; i++) {
                const float a2 = (float)i / N * 6.2831853f, sn = sinf(a2), cs = -cosf(a2);
                glVertex2f(cx + sn * (R - T / 2), cy + cs * (R - T / 2)); glVertex2f(cx + sn * (R + T / 2), cy + cs * (R + T / 2));
            }
            glEnd();
        }
        glEnable(GL_TEXTURE_2D);
    }
    glEnable(GL_DEPTH_TEST);
}
