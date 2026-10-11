// ================= gamepad.inc: controles (SDL_GameController) =================
// - Ate 4 controles; hot-plug (SDL_CONTROLLERDEVICEADDED/REMOVED). Sem controle conectado nada muda no jogo.
// - Jogo: stick esquerdo anda (analogico), direito olha, gatilhos atacam/usam, botoes mapeados em gGpMap (pagina Opcoes > Gamepad..., salvo em gamepad.cfg).
// - Menus/inventario/launcher: o controle move um cursor virtual e gera eventos SDL de mouse/teclado (A = clique, X = botao direito, B/Start = Esc,
//   stick direito ou direcionais = rolagem). Com teclado virtual aberto (screens.inc) o direcional escolhe a tecla.
// - Split screen: gpPadFor(jogador) decide qual controle e de quem (splitscreen.inc).
enum { GB_A, GB_B, GB_X, GB_Y, GB_BACK, GB_GUIDE, GB_START, GB_LS, GB_RS, GB_LB, GB_RB, GB_UP, GB_DOWN, GB_LEFT, GB_RIGHT, GB_LT, GB_RT, GB_N };   // 0..14 = SDL_CONTROLLER_BUTTON_*, 15/16 = gatilhos
enum { GA_JUMP, GA_SNEAK, GA_SPRINT, GA_ATTACK, GA_USE, GA_INV, GA_DROP, GA_SWAP, GA_PREV, GA_NEXT, GA_CAM, GA_CHAT, GA_PAUSE, GA_N };
static const char* const GA_NAME[GA_N] = {"Pular", "Agachar", "Correr", "Atacar/Destruir", "Usar/Colocar", "Inventário", "Largar Item", "Trocar de Mão", "Slot Anterior", "Próximo Slot", "Câmera (F5)", "Bate-Papo", "Pausa"};
static const char* const GB_NAME[GB_N] = {"A", "B", "X", "Y", "Select", "Guia", "Start", "L3", "R3", "LB", "RB", "Cima", "Baixo", "Esquerda", "Direita", "LT", "RT"};
static const int GP_DEF[GA_N] = {GB_A, GB_B, GB_LS, GB_RT, GB_LT, GB_Y, GB_DOWN, GB_X, GB_LB, GB_RB, GB_UP, GB_BACK, GB_START};
static int gGpMap[GA_N] = {GB_A, GB_B, GB_LS, GB_RT, GB_LT, GB_Y, GB_DOWN, GB_X, GB_LB, GB_RB, GB_UP, GB_BACK, GB_START};
static float gGpSens = 1.f;     // multiplicador da velocidade de olhar (main_loop le "gpsens" da pagina de opcoes)
static int gGpBind = -1;        // acao esperando o proximo botao (pagina Gamepad)
static bool gGpSprint = false;  // correr "travado" (clique do L3 liga; soltar o stick para frente desliga)
static Uint32 gGpMouse = 0;     // botoes de mouse "segurados" pelo controle (OU com SDL_GetMouseState)
static Uint32 gpMouseState() { return SDL_GetMouseState(nullptr, nullptr) | gGpMouse; }

struct GpPad { SDL_GameController* c = nullptr; int id = -1; bool b[GB_N] = {}, pb[GB_N] = {}; float ax[4] = {}; };
static GpPad gPads[4];
static int gpCount() { int n = 0; for (const GpPad& p : gPads) if (p.c) n++; return n; }
static GpPad* gpNth(int n) { for (GpPad& p : gPads) if (p.c && n-- == 0) return &p; return nullptr; }   // n-esimo controle CONECTADO (0 = o primeiro)
static void gpSave() {
    FILE* f = fopen("gamepad.cfg", "wb"); if (!f) return;
    for (int i = 0; i < GA_N; i++) fprintf(f, "m%d=%d\n", i, gGpMap[i]);
    fclose(f);
}
static void gpLoad() {
    FILE* f = fopen("gamepad.cfg", "rb"); if (!f) return; char ln[64]; int i, v;
    while (fgets(ln, sizeof ln, f)) if (sscanf(ln, "m%d=%d", &i, &v) == 2 && i >= 0 && i < GA_N && v >= 0 && v < GB_N) gGpMap[i] = v;
    fclose(f);
}
static void gpResetMap() { for (int i = 0; i < GA_N; i++) gGpMap[i] = GP_DEF[i]; gpSave(); }
static void gpOpen(int dev) {
    if (!SDL_IsGameController(dev)) return;
    SDL_GameController* c = SDL_GameControllerOpen(dev); if (!c) return;
    const int id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c));
    for (GpPad& p : gPads) if (p.c && p.id == id) return;   // ja aberto
    for (GpPad& p : gPads) if (!p.c) { p = GpPad(); p.c = c; p.id = id; logInfo("Controle conectado: %s", SDL_GameControllerName(c) ? SDL_GameControllerName(c) : "?"); return; }
    SDL_GameControllerClose(c);
}
static void gpClose(int id) { for (GpPad& p : gPads) if (p.c && p.id == id) { SDL_GameControllerClose(p.c); p = GpPad(); logInfo("Controle desconectado"); } }
static bool gpEvent(const SDL_Event& e) {   // hot-plug; true = evento consumido
    if (e.type == SDL_CONTROLLERDEVICEADDED) { gpOpen(e.cdevice.which); return true; }
    if (e.type == SDL_CONTROLLERDEVICEREMOVED) { gpClose(e.cdevice.which); return true; }
    return false;
}
static float gpDead(float v, float dz = .22f) { const float a = fabsf(v); if (a < dz) return 0.f; return (v < 0 ? -1.f : 1.f) * std::min(1.f, (a - dz) / (1.f - dz)); }
static void gpStick(float& x, float& y, float dz = .22f) {   // zona morta radial (nao quadrada) + curva suave
    const float m = sqrtf(x * x + y * y); if (m < dz) { x = y = 0; return; }
    const float s = std::min(1.f, (m - dz) / (1.f - dz)) / m; x *= s; y *= s;
}
static void gpPoll() {   // 1x por quadro: le botoes e eixos de todos os controles (b/pb permitem detectar o "aperto" = borda de subida)
    for (GpPad& p : gPads) if (p.c) {
        for (int i = 0; i < GB_N; i++) p.pb[i] = p.b[i];
        for (int i = 0; i < GB_LT; i++) p.b[i] = SDL_GameControllerGetButton(p.c, (SDL_GameControllerButton)i) != 0;
        p.b[GB_LT] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000;
        p.b[GB_RT] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000;
        p.ax[0] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_LEFTX) / 32768.f; p.ax[1] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_LEFTY) / 32768.f;
        p.ax[2] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_RIGHTX) / 32768.f; p.ax[3] = SDL_GameControllerGetAxis(p.c, SDL_CONTROLLER_AXIS_RIGHTY) / 32768.f;
    }
    if (gGpBind >= 0)   // pagina Gamepad: o proximo botao apertado vira o novo atalho da acao
        for (GpPad& p : gPads) if (p.c) for (int i = 0; i < GB_N; i++) if (i != GB_GUIDE && p.b[i] && !p.pb[i]) { gGpMap[gGpBind] = i; gGpBind = -1; gpSave(); return; }
}
static bool gpHeld(const GpPad* p, int act) { return p && gGpBind < 0 && p->b[gGpMap[act]]; }
static bool gpPressed(const GpPad* p, int act) { return p && gGpBind < 0 && p->b[gGpMap[act]] && !p->pb[gGpMap[act]]; }
static bool gpBtnPressed(const GpPad* p, int btn) { return p && gGpBind < 0 && p->b[btn] && !p->pb[btn]; }

// Qual controle e de qual jogador (1 ou 2). Sem split: o 1o controle e do jogador 1.
// Split com 2+ controles: 1o = jogador 1, 2o = jogador 2. Split com 1 controle: ele e do jogador 2 (o jogador 1 usa teclado e mouse).
static bool gOskForce = false, gOskCaps = false;   // teclado virtual (screens.inc): forcado pelo botao "Aa" / maiuscula
//(gSplitCtx fica em survival.inc)      // 2 enquanto o estado de sobrevivencia do jogador 2 esta trocado no lugar do jogador 1 (sem avisos de rede/chat do J1)
static int gUseOverride = -1;  // -1 = botao direito do mouse; 0/1 = "usar item" vindo do controle do jogador 2 (comer / puxar arco)
static bool split2Targetable(); static const Phys* split2PhysP(); static void split2Damage(int dmg, const char* why, bool force = false, float kx = 0.f, float kz = 0.f);   // splitscreen.inc: jogador 2 como alvo de mobs/flechas/explosoes
static int gInvOwner = 0;     // 2 = a tela de inventario aberta pertence ao jogador 2 (o cursor vem do controle dele)
static GpPad* gpPadFor(int player) {
    const int n = gpCount(); if (!n) return nullptr;
    if (!gSplit) return player == 1 ? gpNth(0) : nullptr;
    if (n >= 2) return gpNth(player - 1);
    return player == 2 ? gpNth(0) : nullptr;
}
struct GpIn { float fwd = 0, str = 0, lx = 0, ly = 0; bool jump = false, sneak = false, attack = false, use = false; };
static GpIn gpRead(const GpPad* p) {   // fwd/str em -1..1 (analogico); lx/ly = olhar (ly > 0 = para baixo)
    GpIn r; if (!p) return r;
    float x = p->ax[0], y = p->ax[1]; gpStick(x, y);
    const float dx = (p->b[GB_RIGHT] ? 1.f : 0.f) - (p->b[GB_LEFT] ? 1.f : 0.f);   // direcional lateral so troca de slot (nao anda)
    (void)dx; r.str = x; r.fwd = -y;
    float lx = p->ax[2], ly = p->ax[3]; gpStick(lx, ly, .18f);
    r.lx = lx * fabsf(lx) * .35f + lx * .65f; r.ly = ly * fabsf(ly) * .35f + ly * .65f;   // curva: mira fina perto do centro, rapida na borda
    r.jump = gpHeld(p, GA_JUMP); r.sneak = gpHeld(p, GA_SNEAK); r.attack = gpHeld(p, GA_ATTACK); r.use = gpHeld(p, GA_USE);
    return r;
}
static const float GP_LOOK_DEG = 190.f;   // graus/s com o stick no maximo (x gGpSens)

// ---- menus: cursor virtual + eventos sinteticos ----
static bool oskActive(); static void oskNav(int dx, int dy); static void oskPress(); static void oskBack(); static void oskSpace(); static void oskShift(); static void oskClose();   // screens.inc
static float gpCurX = 0, gpCurY = 0; static bool gpUiWas = false, gpLDown = false, gpRDown = false; static float gpRep = 0.f, gpWheelT = 0.f; static int gpOskDir = 0;
static void gpPushMouse(SDL_Window* win, int type, int btn, int x, int y) {
    SDL_Event e; memset(&e, 0, sizeof e); e.type = type;
    if (type == SDL_MOUSEMOTION) { e.motion.x = x; e.motion.y = y; e.motion.which = 0; }
    else { e.button.x = x; e.button.y = y; e.button.button = (Uint8)btn; e.button.state = type == SDL_MOUSEBUTTONDOWN ? 1 : 0; e.button.clicks = 1; }
    SDL_PushEvent(&e);
}
static void gpPushText(const char* t) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_TEXTINPUT; strncpy(e.text.text, t, sizeof e.text.text - 1); SDL_PushEvent(&e); }
static void gpPushKey(SDL_Keycode k) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = k; e.key.state = 1; SDL_PushEvent(&e); e.type = SDL_KEYUP; e.key.state = 0; SDL_PushEvent(&e); }
static void gpUiFrame(SDL_Window* win, float dt) {   // chamado todo quadro em que ha tela/menu/inventario aberto
    GpPad* p = gInvOwner == 2 ? gpPadFor(2) : (gpPadFor(1) ? gpPadFor(1) : gpNth(0)); if (!p) { gpUiWas = false; return; }
    int ww = 1, wh = 1; SDL_GetWindowSize(win, &ww, &wh);
    if (!gpUiWas) { int mx = ww / 2, my = wh / 2; SDL_GetMouseState(&mx, &my); gpCurX = (float)mx; gpCurY = (float)my; gpUiWas = true; gpLDown = gpRDown = false; }
    if (gGpBind >= 0) return;   // capturando botao: nada de cliques
    if (oskActive()) {   // teclado virtual: direcional / stick escolhe a tecla, A digita, B apaga, X shift, Y espaco, Start confirma
        int dx = 0, dy = 0; float sx = p->ax[0], sy = p->ax[1]; gpStick(sx, sy, .5f);
        if (gpBtnPressed(p, GB_LEFT)) dx = -1; if (gpBtnPressed(p, GB_RIGHT)) dx = 1; if (gpBtnPressed(p, GB_UP)) dy = -1; if (gpBtnPressed(p, GB_DOWN)) dy = 1;
        const int sd = sx < -.5f ? 1 : sx > .5f ? 2 : sy < -.5f ? 3 : sy > .5f ? 4 : 0;   // stick: repete a cada ~0,18 s
        if (!dx && !dy && sd) { gpRep -= dt; if (sd != gpOskDir || gpRep <= 0.f) { gpRep = sd == gpOskDir ? .18f : .35f; gpOskDir = sd; dx = sd == 1 ? -1 : sd == 2 ? 1 : 0; dy = sd == 3 ? -1 : sd == 4 ? 1 : 0; } } else if (!sd) gpOskDir = 0;
        if (dx || dy) oskNav(dx, dy);
        if (gpBtnPressed(p, GB_A)) oskPress(); if (gpBtnPressed(p, GB_B)) oskBack(); if (gpBtnPressed(p, GB_X)) oskShift(); if (gpBtnPressed(p, GB_Y)) oskSpace();
        if (gpBtnPressed(p, GB_START)) oskClose();
        return;
    }
    float x = p->ax[0], y = p->ax[1]; gpStick(x, y, .2f);
    float dx = (p->b[GB_RIGHT] ? 1.f : 0.f) - (p->b[GB_LEFT] ? 1.f : 0.f), dy = 0.f;
    const float sp = wh * 0.85f * dt;   // um quadro de tela em ~1,2 s com o stick no maximo
    const float mx = (x * fabsf(x) * .5f + x * .5f + dx * .5f) * sp, my = (y * fabsf(y) * .5f + y * .5f + dy) * sp;
    if (mx != 0.f || my != 0.f) { gpCurX = std::max(0.f, std::min((float)ww - 1, gpCurX + mx)); gpCurY = std::max(0.f, std::min((float)wh - 1, gpCurY + my)); gpPushMouse(win, SDL_MOUSEMOTION, 0, (int)gpCurX, (int)gpCurY); }
    const bool a = p->b[GB_A], xb = p->b[GB_X];
    if (a && !gpLDown) { gpLDown = true; gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, (int)gpCurX, (int)gpCurY); }
    if (!a && gpLDown) { gpLDown = false; gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, (int)gpCurX, (int)gpCurY); }
    if (xb && !gpRDown) { gpRDown = true; gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, (int)gpCurX, (int)gpCurY); }
    if (!xb && gpRDown) { gpRDown = false; gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, (int)gpCurX, (int)gpCurY); }
    if (gpBtnPressed(p, GB_B) || gpBtnPressed(p, GB_START)) gpPushKey(SDLK_ESCAPE);
    // rolagem: stick direito (ou LB/RB) gera a roda do mouse (listas, abas do criativo)
    gpWheelT -= dt; const float ry = gpDead(p->ax[3], .5f);
    if (gpWheelT <= 0.f && (ry != 0.f || p->b[GB_LB] || p->b[GB_RB])) {
        gpWheelT = .12f; SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_MOUSEWHEEL; e.wheel.y = (ry > 0 || p->b[GB_RB]) ? -1 : 1; SDL_PushEvent(&e);
    }
}
static void gpUiLeave() { gpUiWas = false; gpLDown = gpRDown = false; }
