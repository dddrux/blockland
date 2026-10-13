// ---------- HUD touch estilo Minecraft PE (liga/desliga com F8) ----------
// Joystick flutuante (estilo Roblox, zona esquerda) = andar (8 direcoes) | losango central = agachar (liga/desliga; voando = descer) | losango da direita = pular (voando = subir)
// (a zona esquerda da tela e so do joystick) Arrastar no resto da tela = olhar | toque rápido = colocar bloco | segurar parado = quebrar | botões do topo = câmera, chat, pausa | "..." = inventário
// Funciona com dedos (multitoque) e com o mouse (1 ponteiro).
static bool touchHud = false, tvF = false, tvB = false, tvL = false, tvR = false, tvJump = false, tvSneakT = false, tvSprint = false, tvSprintL = false;
static bool touchBreak = false, touchPlace = false, gHideHud = false;   // gHideHud = F1 (vale para teclado e toque)
enum { TA_LIGHT, TA_CHAT, TA_F3, TA_PAUSE, TA_INV, TA_F1, TA_F5, TA_DROP, TA_SWAP, TA_SPRINT, TA_TAB, TA_PICK, TA_N };
static bool tAct[TA_N] = {};
static int touchSel = -1; static float tDX = 0, tDY = 0;
struct TPt { bool on = false, moved = false, brk = false; int64_t id = 0; int kind = 0, idx = 0; float sx = 0, sy = 0, lx = 0, ly = 0; Uint32 t0 = 0; float cx = 0, cy = 0; };   // cx,cy = centro do joystick flutuante (kind 1)
static TPt tpts[8];
struct TRect { float x, y, w, h; bool in(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; } };
// TODAS as medidas abaixo estao em UNIDADES DE GUI (1 unidade = 1 pixel de arte = G pixels de tela), a mesma grade da hotbar.
struct TLay { int G; float jcx, jcy, jr, jR; TRect jzone, top[7], inv, jump, sneak, side[4], hot, hotHit; };
static void relMouse(bool g) { SDL_SetRelativeMouseMode(g && !touchHud ? SDL_TRUE : SDL_FALSE); }   // touch: cursor sempre livre
static TLay touchLay(int w, int h) {
    TLay L; L.G = guiScaleFor(w, h); const float g = (float)L.G; const float sw = (float)((w + L.G - 1) / L.G), sh = (float)((h + L.G - 1) / L.G);
    auto R = [&](float x, float y, float bw, float bh) { return TRect{x * g, y * g, bw * g, bh * g}; };
    // topo: pausa, chat, F5, F1, F3, luz, Tab (20x20, espaco de 2) centralizados
    const float tx = floorf((sw - (7 * 20 + 6 * 2)) / 2);
    for (int i = 0; i < 7; i++) L.top[i] = R(tx + i * 22, 2, 20, 20);
    L.hot = R(floorf((sw - 182) / 2), sh - 22, 182, 22); L.hotHit = {L.hot.x, L.hot.y - 12 * g, L.hot.w, L.hot.h + 12 * g};
    // botoes de baixo (inventario, pular, agachar, coluna lateral): maiores que os do topo para o dedo acertar com conforto;
    // k encolhe tudo um pouco so em telas estreitas (4:3), onde o espaco ao lado da hotbar e menor
    const float sideRoom = (sw - 182) / 2, k = std::max(.8f, std::min(1.f, sideRoom / 84.f));
    auto S = [&](float v) { return floorf(v * k + .5f); };
    const float jsz = S(40), ssz = S(34), bsz = S(28), isz = S(30), bgap = 3;
    L.inv = R(floorf((sw - 182) / 2) + 182 + 3, sh - isz, isz, isz);   // botao do inventario: encostado na hotbar, mais alto que ela
    // joystick FLUTUANTE (estilo Roblox): a base aparece onde o dedo encosta, dentro da zona esquerda (jzone), e acompanha o dedo se ele passar da borda.
    // jR = raio da base ativa (~36% da altura da GUI, bem maior que antes); jr/jcx/jcy = so a "dica" apagada no canto, para saber onde tocar
    const float jbA = 2.f * floorf(std::max(64.f, std::min(88.f, sh * .36f)) / 2);
    L.jR = jbA * g / 2;
    const float jb = std::max(40.f, std::min(jbA, 2.f * floorf(((sw - 182) / 2 - 12) / 2)));   // dica parada: cabe ao lado da hotbar
    L.jr = jb * g / 2; L.jcx = (10 + jb / 2) * g; L.jcy = (sh - 10 - jb / 2) * g;
    L.jzone = TRect{0.f, h * .3f, w * .4f, h * .7f};   // toque aqui (e fora de botoes/hotbar) = andar; o resto da tela = olhar
    L.jump = R(sw - 10 - jsz, sh - 10 - jsz, jsz, jsz); L.sneak = R(sw - 10 - jsz + floorf((jsz - ssz) / 2), sh - 10 - jsz - 4 - ssz, ssz, ssz);   // pular e agachar em coluna, como no Bedrock
    for (int i = 0; i < 4; i++) L.side[i] = R(sw - 10 - jsz - 4 - bsz, sh - isz - 2 - bsz * (i + 1) - bgap * i, bsz, bsz);   // coluna ao lado: trocar mao (F), soltar (Q), correr (acima do inventario, sem sobrepor)
    return L;
}
static void touchRecalc(const TLay& L) {
    tvF = tvB = tvL = tvR = tvJump = tvSprint = false;
    for (const TPt& p : tpts) {
        if (!p.on) continue;
        if (p.kind == 2) tvJump = true;
        if (p.kind == 1) {   // joystick analogico (Bedrock): vetor normalizado pelo raio da base; zona morta 18%; 8 direcoes; borda da frente = correr
            const float vx = (p.lx - p.cx) / L.jR, vy = (p.ly - p.cy) / L.jR, m = hypotf(vx, vy);
            if (m < .18f) continue;
            const float t = .38f * m;
            tvF |= vy < -t; tvB |= vy > t; tvL |= vx < -t; tvR |= vx > t;
            if (m >= .95f && vy < -.7f * m) tvSprint = true;
        }
    }
    if (tvF && tvSprintL) tvSprint = true; else if (!tvF) tvSprintL = false;   // botao de correr: liga ate parar de andar para a frente
}
static float touchBrkProg = 0, touchBrkX = 0, touchBrkY = 0, touchPlaceX = 0, touchPlaceY = 0;
static bool touchRing = false;   // trilho do anel visivel (segurando, antes de comecar a quebrar)
static bool touchAim = false;   // true = dedo segurando: o alvo é o bloco sob o dedo (como o MCPE sem controles divididos)   // 0..1: círculo de quebra (segurar / minerar)
static void touchReset() { touchBrkProg = 0; for (TPt& p : tpts) p = TPt(); tvF = tvB = tvL = tvR = tvJump = tvSprint = tvSprintL = touchBreak = touchPlace = touchUse = false; tDX = tDY = 0; touchSel = -1; for (bool& a : tAct) a = false; }
// mira por toque: tipo de IA do mob sob o raio (2 = aldeao), -1 = nenhum (ignora mob atras de bloco)
static int touchMobAim(const float* eye, const float* dir, float reach) {
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead) continue;
        const float hw = MD[e.type].w * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + MD[e.type].h + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return -1;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return -1;
    return MD[MOBS[best].type].ai;
}
static void touchFrame(Uint32 now) {   // segurar parado => quebrar / bater (alvo = sob o dedo); com comida ou arco na mao => usar o item (comer / puxar)
    touchBreak = false; touchBrkProg = 0; touchAim = false; touchRing = false; touchUse = false;
    for (TPt& p : tpts) if (p.on && p.kind == 4) {
        if (p.moved) continue;
        if (gTouchUseItem) { if (now - p.t0 > 180) { touchUse = true; touchAim = true; touchBrkX = p.lx; touchBrkY = p.ly; } continue; }   // Bedrock: segurar com comida/arco usa o item, nao quebra
        if (now - p.t0 > 180) { touchRing = true; touchBrkX = p.lx; touchBrkY = p.ly; }   // o anel surge sob o dedo logo que voce segura
        if (!p.brk && now - p.t0 > 350) p.brk = true;
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
        bool hit = false; const bool hid = gHideHud;   // F1: so o botao F1 responde; o resto da tela continua olhando/batendo/colocando
        static const int topAct[7] = {TA_PAUSE, TA_CHAT, TA_F5, TA_F1, TA_F3, TA_LIGHT, TA_TAB};
        for (int i = 0; i < 7 && !hit; i++) if (L.top[i].in(x, y) && (!hid || topAct[i] == TA_F1)) { tAct[topAct[i]] = true; p->kind = 3; p->idx = topAct[i]; hit = true; }
        if (!hid) {
            if (!hit && L.inv.in(x, y)) { tAct[TA_INV] = true; p->kind = 3; p->idx = TA_INV; hit = true; }
            static const int sideAct[4] = {TA_SWAP, TA_DROP, TA_SPRINT, TA_PICK};
            for (int i = 0; i < 4 && !hit; i++) if (L.side[i].in(x, y)) { tAct[sideAct[i]] = true; p->kind = 3; p->idx = sideAct[i]; hit = true; }
            if (!hit && L.hotHit.in(x, y)) { touchSel = std::max(0, std::min(8, (int)((x - L.hot.x) / (20.f * L.G)))); p->kind = 5; hit = true; }
            bool joyOn = false; for (const TPt& q : tpts) if (q.on && q.kind == 1) joyOn = true;
            if (!hit && !joyOn && L.jzone.in(x, y)) { p->kind = 1; p->cx = x; p->cy = y; hit = true; }   // joystick flutuante: a base nasce sob o dedo
            if (!hit && L.sneak.in(x, y)) { p->kind = 6; hit = true; tvSneakT = !tvSneakT; }
            if (!hit && L.jump.in(x, y)) { p->kind = 2; hit = true; }
        }
        if (!hit) p->kind = 4;
    } else if (p) {
        if (ph == 1) {
            if (p->kind == 4) { tDX += x - p->lx; tDY += y - p->ly; if (hypotf(x - p->sx, y - p->sy) > 14.f) p->moved = true; }
            if (p->kind == 1) { const float dx = x - p->cx, dy = y - p->cy, d = hypotf(dx, dy); if (d > L.jR) { const float f = (d - L.jR) / d; p->cx += dx * f; p->cy += dy * f; } }   // passou da borda: a base anda junto
            p->lx = x; p->ly = y;
        } else {
            if (p->kind == 4 && !p->moved && !p->brk && SDL_GetTicks() - p->t0 < 300) { touchPlace = true; touchPlaceX = p->lx; touchPlaceY = p->ly; }   // toque rápido
            *p = TPt();
        }
    }
    touchRecalc(L);
    return true;
}
// ---- desenho: TUDO na mesma grade de pixel da GUI (1 unidade = G pixels de tela); nenhum "pixel" maior ou menor que o da hotbar ----
#define TP(var, ...) static const char* const var[] = {__VA_ARGS__}
TP(pSun, "....#....", ".#..#..#.", "..#####..", "#.#####.#", "..#####..", ".#..#..#.", "....#....");
TP(pChat, "###########", "###########", "###########", "###########", "###########", "..####.....", "..###......", "..##.......");
TP(pPause, "###...###", "###...###", "###...###", "###...###", "###...###", "###...###", "###...###", "###...###");
TP(pTab, "#.#####", ".......", "#.#####", ".......", "#.#####");   // lista de jogadores (Tab): segurar
TP(pF1, "###..#.", "#...##.", "##...#.", "#....#.", "#...###");
TP(pF3, "###.###", "#.....#", "##...##", "#.....#", "#...###");
TP(pF5, "###.###", "#...#..", "##..###", "#.....#", "#...###");
TP(pSneak, "...###...", "...###...", "#########", ".#######.", "..#####..", "...###...", "....#....", ".........", "#########");
TP(pJump, "....#....", "...###...", "..#####..", ".#######.", "#########", "...###...", "...###...", "...###...");
TP(pSwap, "......#..", ".......#.", "#########", ".......#.", "......#..", "..#......", ".#.......", "#########", ".#.......", "..#......");
TP(pPick, "..#####..", ".#.....#.", "#..###..#", "#.#...#.#", "#.#.#.#.#", "#.#...#.#", "#..###..#", ".#.....#.", "..#####..");   // alvo (VIS)
TP(pDrop, "..###..", "..###..", "#######", ".#####.", "..###..", "...#...", ".......", "#######");
TP(pRun, "....#....", "...###...", "..##.##..", ".##...##.", "....#....", "...###...", "..##.##..", ".##...##.");
TP(pX, "##.....##", "###...###", ".###.###.", "..#####..", "...###...", "..#####..", ".###.###.", "###...###", "##.....##");
TP(pDots, "##.##.##");
static bool touchShadowOn();
static void tPix(float cx, float cy, int g, const char* const* rows, int nr, int nc, float v, float al = .95f, int ps = 1) {   // cx,cy em pixels de tela; cada '#' = ps unidades (ps*g pixels; ps=1 = pixel da hotbar)
    const float q = (float)(g * ps), x0 = floorf(cx / g - nc * ps / 2.f) * g, y0 = floorf(cy / g - nr * ps / 2.f) * g;
    for (int r = 0; r < nr; r++) for (int c = 0; c < nc; c++) if (rows[r][c] == '#') flat(x0 + c * q, y0 + r * q, q, q, v, v, v, al);
}
static int tIconScale(const TRect& r, int g, int nr, int nc) {   // botoes grandes de baixo: icone em 2x quando sobra pelo menos 3 unidades de folga de cada lado
    const float u = std::min(r.w, r.h) / g - 6.f; return (nr * 2 <= u && nc * 2 <= u) ? 2 : 1;
}
static void tBtn(const TRect& r, int g, bool down, float ma = 1.f) {   // botao de pedra do MCPE: contorno escuro de 1 unidade (cantos recortados), bisel de 1 unidade; apertado = mais claro e bisel invertido
    const float x = r.x, y = r.y, w = r.w, h = r.h, P = (float)g, fv = down ? .88f : .66f, fa = (down ? .92f : .58f) * ma, dk = .06f, da = .80f * ma;
    flat(x + P, y, w - 2 * P, P, dk, dk, dk, da); flat(x + P, y + h - P, w - 2 * P, P, dk, dk, dk, da);
    flat(x, y + P, P, h - 2 * P, dk, dk, dk, da); flat(x + w - P, y + P, P, h - 2 * P, dk, dk, dk, da);
    flat(x + P, y + P, w - 2 * P, h - 2 * P, fv, fv, fv, fa);
    const float hi = down ? 0.f : 1.f, lo = down ? 1.f : 0.f, ha = (down ? .18f : .40f) * ma, la = (down ? .30f : .32f) * ma;
    flat(x + P, y + P, w - 2 * P, P, hi, hi, hi, ha); flat(x + P, y + P, P, h - 2 * P, hi, hi, hi, ha);
    flat(x + P, y + h - 2 * P, w - 2 * P, P, lo, lo, lo, la); flat(x + w - 2 * P, y + P, P, h - 2 * P, lo, lo, lo, la);
}
static void tIconBtn(const TRect& r, int g, bool down, const char* const* rows, int nr, int nc, bool on = false, float ma = 1.f, bool big = false) {   // big = botao grande de baixo (icone pode dobrar)
    tBtn(r, g, down || on, ma); tPix(r.x + r.w / 2, r.y + r.h / 2, g, rows, nr, nc, down || on ? .08f : .15f, .95f * ma, big ? tIconScale(r, g, nr, nc) : 1);
}
static void tSetup(int w, int h) {
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static TRect touchCloseRect(int w, int h) { const TLay L = touchLay(w, h); const float g = (float)L.G, sw = (float)((w + L.G - 1) / L.G); return {(sw - 22) * g, 2 * g, 20 * g, 20 * g}; }
static void drawTouchClose(int w, int h) {   // botão "X" (só no modo celular) para sair do chat / inventário
    tSetup(w, h); const TRect r = touchCloseRect(w, h); const int g = guiScaleFor(w, h);
    tBtn(r, g, false); tPix(r.x + r.w / 2, r.y + r.h / 2, g, pX, 9, 9, .15f);
    glEnable(GL_DEPTH_TEST);
}
static void drawTouch(int w, int h, bool hidden = false) {   // hidden (F1): so o botao F1, com opacidade baixa
    tSetup(w, h);
    const TLay L = touchLay(w, h); const int g = L.G; const float gf = (float)g;
    bool pr[TA_N] = {}; for (const TPt& p : tpts) if (p.on && p.kind == 3) pr[p.idx] = true;
    {   // botoes do topo (ordem: pausa, chat, F5, F1, F3, luz)
        static const int ta[7] = {TA_PAUSE, TA_CHAT, TA_F5, TA_F1, TA_F3, TA_LIGHT, TA_TAB};
        static const char* const* ic[7] = {pPause, pChat, pF5, pF1, pF3, pSun, pTab}; static const int nr[7] = {8, 8, 5, 5, 5, 7, 5}, nc[7] = {9, 11, 7, 7, 7, 9, 7};
        for (int i = 0; i < 7; i++) {
            if (hidden && ta[i] != TA_F1) continue;
            tIconBtn(L.top[i], g, pr[ta[i]], ic[i], nr[i], nc[i], ta[i] == TA_LIGHT && touchShadowOn(), hidden ? .35f : 1.f);
        }
    }
    if (hidden) { glEnable(GL_DEPTH_TEST); return; }
    {   // joystick estilo Bedrock: quadrados arredondados em passos de 1 unidade (so quads inteiros); a bolinha segue o dedo encaixada na grade
        glDisable(GL_TEXTURE_2D);
        auto sq = [&](float cx, float cy, int n, int cr, float v, float al) {
            const float x0 = floorf(cx / gf - n / 2.f) * gf, y0 = floorf(cy / gf - n / 2.f) * gf; glColor4f(v, v, v, al); glBegin(GL_QUADS);
            for (int r = 0; r < n; r++) {
                const int d = std::min(r, n - 1 - r); int ind = 0;
                if (d < cr) { const float dy = cr - d - .5f; ind = cr - (int)floorf(sqrtf(cr * cr - dy * dy) + .5f); }
                const float xa = x0 + ind * gf, xb = x0 + (n - ind) * gf, ya = y0 + r * gf;
                glVertex2f(xa, ya); glVertex2f(xb, ya); glVertex2f(xb, ya + gf); glVertex2f(xa, ya + gf);
            }
            glEnd();
        };
        const TPt* ja = nullptr; for (const TPt& p : tpts) if (p.on && p.kind == 1) ja = &p;
        const float jcx = ja ? ja->cx : L.jcx, jcy = ja ? ja->cy : L.jcy, jrr = ja ? L.jR : L.jr, am = ja ? 1.f : .45f;   // sem dedo: so a dica apagada no canto
        const int jb = (int)lroundf(jrr * 2 / gf), kn = std::max(10, jb * 3 / 8); float kx = jcx, ky = jcy;
        if (ja) { float vx = (ja->lx - jcx) / jrr, vy = (ja->ly - jcy) / jrr; const float m = hypotf(vx, vy); if (m > 1.f) { vx /= m; vy /= m; } kx = jcx + vx * jrr * .62f; ky = jcy + vy * jrr * .62f; }
        kx = jcx + roundf((kx - jcx) / gf) * gf; ky = jcy + roundf((ky - jcy) / gf) * gf;
        sq(jcx, jcy, jb, jb / 4, .05f, .40f * am); sq(jcx, jcy, jb - 2, jb / 4 - 1, .62f, .30f * am);   // base: borda escura de 1 unidade + miolo translucido
        sq(kx, ky, kn, kn / 4, .05f, .60f * am); sq(kx, ky, kn - 2, kn / 4 - 1, tvSprint ? .95f : .75f, .85f * am);   // bolinha (correndo = mais clara)
        { const float x0 = floorf(kx / gf - (kn - 2) / 2.f) * gf + 2 * gf, y0 = floorf(ky / gf - (kn - 2) / 2.f) * gf; flat(x0, y0, (kn - 6) * gf, gf, 1, 1, 1, .30f * am); }   // brilho de 1 unidade no topo
        glEnable(GL_TEXTURE_2D);
    }
    tIconBtn(L.sneak, g, tvSneakT, pSneak, 9, 9, false, 1.f, true);
    tIconBtn(L.jump, g, tvJump, pJump, 8, 9, false, 1.f, true);
    tIconBtn(L.side[0], g, pr[TA_SWAP], pSwap, 10, 9, false, 1.f, true);
    tIconBtn(L.side[1], g, pr[TA_DROP], pDrop, 8, 7, false, 1.f, true);
    tIconBtn(L.side[2], g, pr[TA_SPRINT], pRun, 8, 9, tvSprintL, 1.f, true);
    tIconBtn(L.side[3], g, pr[TA_PICK], pPick, 9, 9, false, 1.f, true);
    {   // inventário: mesmo tamanho e visual da hotbar (fundo preto 60% + slot rebaixado de 18)
        const TRect& r = L.inv;
        flat(r.x, r.y, r.w, r.h, 0, 0, 0, .6f);
        const float bx = r.x + 2 * gf, by = r.y + 2 * gf, bw = r.w - 4 * gf, v = pr[TA_INV] ? .30f : .43f;   // slot rebaixado = botao menos 2 unidades de moldura de cada lado
        flat(bx, by, bw, bw, v, v, v, .9f); flat(bx, by, bw - gf, gf, .22f, .22f, .22f, .9f); flat(bx, by, gf, bw - gf, .22f, .22f, .22f, .9f);
        flat(bx + gf, by + bw - gf, bw - gf, gf, 1, 1, 1, .9f); flat(bx + bw - gf, by + gf, gf, bw - gf, 1, 1, 1, .9f);
        tPix(bx + bw / 2, by + bw / 2, g, pDots, 1, 8, 1.f, .95f, bw / gf >= 24.f ? 2 : 1);
    }
    // anel de quebra do MCPE: trilho com a espessura de 2 unidades; o arco claro percorre o anel (horario, a partir do topo) conforme quebra
    if (touchBrkProg > .01f || touchRing) {
        const float R = 16.f * gf, T = 2.f * gf, cx = touchBrkX, cy = touchBrkY; const int N = 96;
        glDisable(GL_TEXTURE_2D);
        for (int pass = 0; pass < 2; pass++) {
            if (pass && touchBrkProg <= .01f) break;
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
