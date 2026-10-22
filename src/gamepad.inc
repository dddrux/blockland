// ================= gamepad.inc: controles (SDL_GameController) =================
// - Ate 4 controles; hot-plug (SDL_CONTROLLERDEVICEADDED/REMOVED). Sem controle conectado nada muda no jogo.
// - Jogo: stick esquerdo anda (analogico), direito olha, gatilhos atacam/usam, botoes mapeados em gGpMap (pagina Opcoes > Gamepad..., salvo em gamepad.cfg).
// - Menus/inventario/launcher: o controle move um cursor virtual e gera eventos SDL de mouse/teclado (A = clique, X = botao direito, B/Start = Esc,
//   stick direito ou direcionais = rolagem). Com teclado virtual aberto (screens.inc) o direcional escolhe a tecla.
// - Split screen: gpPadFor(jogador) decide qual controle e de quem (splitscreen.inc).
enum { GB_A, GB_B, GB_X, GB_Y, GB_BACK, GB_GUIDE, GB_START, GB_LS, GB_RS, GB_LB, GB_RB, GB_UP, GB_DOWN, GB_LEFT, GB_RIGHT, GB_LT, GB_RT, GB_N };   // 0..14 = SDL_CONTROLLER_BUTTON_*, 15/16 = gatilhos
enum { GA_JUMP, GA_SNEAK, GA_SPRINT, GA_ATTACK, GA_USE, GA_INV, GA_DROP, GA_SWAP, GA_PREV, GA_NEXT, GA_CAM, GA_CHAT, GA_PAUSE, GA_PICK, GA_N };
static const char* const GA_NAME[GA_N] = {"Pular", "Agachar", "Correr", "Atacar/Destruir", "Usar/Colocar", "Inventário", "Largar Item", "Trocar de Mão", "Slot Anterior", "Próximo Slot", "Câmera (F5)", "Bate-Papo", "Pausa", "Pegar Bloco"};
static const char* const GB_NAME[GB_N] = {"A", "B", "X", "Y", "Select", "Guia", "Start", "L3", "R3", "LB", "RB", "Cima", "Baixo", "Esquerda", "Direita", "LT", "RT"};
static const int GP_DEF[GA_N] = {GB_A, GB_B, GB_LS, GB_RT, GB_LT, GB_Y, GB_DOWN, GB_X, GB_LEFT, GB_RIGHT, GB_UP, GB_BACK, GB_START, GB_RS};
static int gGpMap[GA_N] = {GB_A, GB_B, GB_LS, GB_RT, GB_LT, GB_Y, GB_DOWN, GB_X, GB_LEFT, GB_RIGHT, GB_UP, GB_BACK, GB_START, GB_RS};
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
static void gpAutoMap(int dev) {   // patchRpiOpt.7: joystick USB generico (sem mapeamento SDL) vira controle com layout padrao (A=0 B=1 X=2 Y=3 LB=4 RB=5 Back=6 Start=7, eixos 0-3, d-pad = hat 0)
    if (dev < 0 || dev >= SDL_NumJoysticks() || SDL_IsGameController(dev)) return;
    char g[64] = {0}; SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(dev), g, sizeof g);
    std::string nm = SDL_JoystickNameForIndex(dev) ? SDL_JoystickNameForIndex(dev) : "Gamepad"; for (char& ch : nm) if (ch == ',') ch = ' ';
    const std::string m = std::string(g) + "," + nm + ",a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,leftx:a0,lefty:a1,rightx:a2,righty:a3,dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,platform:Linux,";
    static std::set<std::string> tried; if (!tried.insert(g).second) return;   // 1 tentativa por GUID (a varredura roda 1x/s)
    if (SDL_GameControllerAddMapping(m.c_str()) >= 0) logInfo("Controle generico mapeado automaticamente: %s", nm.c_str());
}
static void gpOpen(int dev) {
    gpAutoMap(dev);
    if (!SDL_IsGameController(dev)) return;
    SDL_GameController* c = SDL_GameControllerOpen(dev); if (!c) return;
    const int id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c));
    for (GpPad& p : gPads) if (p.c && p.id == id) return;   // ja aberto
    for (GpPad& p : gPads) if (!p.c) { p = GpPad(); p.c = c; p.id = id; logInfo("Controle conectado: %s", SDL_GameControllerName(c) ? SDL_GameControllerName(c) : "?"); return; }
    SDL_GameControllerClose(c);
}
static void gpScanAll() {   // patchPad.9: abre os controles JA conectados (o loading/splash consumia os eventos de "conectado": so funcionava replugando). Roda ~1x/s
    const int n = SDL_NumJoysticks();
    for (int i = 0; i < n; i++) {
        const SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(i); bool have = false;
        for (const GpPad& p : gPads) if (p.c && p.id == jid) { have = true; break; }
        if (!have) gpOpen(i);
    }
}
static void gpClose(int id) { for (GpPad& p : gPads) if (p.c && p.id == id) { SDL_GameControllerClose(p.c); p = GpPad(); logInfo("Controle desconectado"); } }
static bool gpEvent(const SDL_Event& e) {   // hot-plug; true = evento consumido
    if (e.type == SDL_CONTROLLERDEVICEADDED) { gpOpen(e.cdevice.which); return true; }
    if (e.type == 0x605 /* SDL_JOYDEVICEADDED: mesmo layout de cdevice */) { gpOpen(e.cdevice.which); return true; }   // patchRpiOpt.7: joystick sem mapeamento
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
static bool gChatPad = false, gChatPadNext = false;   // chat aberto pelo CONTROLE (so ai aparece o teclado virtual; T no teclado nao)
static bool gChatAs2 = false, gP2ChatReq = false; static int gChatK = 0, gP2ChatReqK = 0;   // patchSplitChat.1: gChatK/gP2ChatReqK = extra (0..2 = J2..J4) que fala/pediu o chat;   // chat aberto pelo jogador 2 (split): fala com o nick dele e o teclado virtual segue o controle dele
#ifdef BL_RPI
static bool gOskForce = true, gOskCaps = false;   // Raspberry: jogo pensado para controle; teclado virtual sempre disponivel nos campos de texto
#else
static bool gOskForce = false, gOskCaps = false;
#endif
//   // teclado virtual (screens.inc): forcado pelo botao "Aa" / maiuscula
//(gSplitCtx fica em survival.inc)      // 2 enquanto o estado de sobrevivencia do jogador 2 esta trocado no lugar do jogador 1 (sem avisos de rede/chat do J1)
static int gUseOverride = -1;  // -1 = botao direito do mouse; 0/1 = "usar item" vindo do controle do jogador 2 (comer / puxar arco)
static int split2N(); static bool split2TargetableK(int k); static const Phys* split2PhysK(int k); static void split2DamageK(int k, int dmg, const char* why, bool force, float kx, float kz); static int gInvK = 0;   // splitscreen.inc: jogadores extras (J2..J4); gInvK = extra dono do inventario aberto
static bool split2Targetable(); static const Phys* split2PhysP(); static void split2Damage(int dmg, const char* why, bool force = false, float kx = 0.f, float kz = 0.f);   // splitscreen.inc: jogador 2 como alvo de mobs/flechas/explosoes
static int gInvOwner = 0;     // 2 = a tela de inventario aberta pertence ao jogador 2 (o cursor vem do controle dele)
static GpPad* gpPadFor(int player) {   // patchSplit.1: ate 4 jogadores. Controles >= jogadores: 1o controle = J1... Senao: J1 fica com teclado/mouse e o J2, J3... pegam os controles em ordem
    const int n = gpCount(); if (!n) return nullptr;
    if (!gSplit) return player == 1 ? gpNth(0) : nullptr;
    if (n >= gSplitN) return gpNth(player - 1);
    return player >= 2 ? gpNth(player - 2) : nullptr;
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
static Uint32 gpCurT = 0;   // patchPad.1: ultimo uso do cursor do controle (mostra o cursor)
static float gpCurX = 0, gpCurY = 0; static bool gpUiWas = false, gpLDown = false, gpRDown = false; static float gpRep = 0.f, gpWheelT = 0.f; static int gpOskDir = 0;
static int gUiOx = 0, gUiOy = 0, gUiOw = 0, gUiOh = 0; static bool gUiSubOn = false;   // patchSplit.4: viewport (coord. da janela, origem no topo) onde a UI aberta aparece; false = janela inteira
static bool gpSlotPoints(int w, int h, std::vector<std::pair<float, float>>& out, float& rad);   // main_loop.inc: centros dos slots do inventario aberto (px, locais ao retangulo da UI)
static void gpPushMouse(SDL_Window* win, int type, int btn, int x, int y) {   // x,y LOCAIS ao retangulo da UI; which = 0x7F000001 marca evento ja local (main_loop nao traduz de novo)
    SDL_Event e; memset(&e, 0, sizeof e); e.type = type;
    if (type == SDL_MOUSEMOTION) { e.motion.x = x; e.motion.y = y; e.motion.which = 0x7F000001u; }
    else { e.button.x = x; e.button.y = y; e.button.button = (Uint8)btn; e.button.state = type == SDL_MOUSEBUTTONDOWN ? 1 : 0; e.button.clicks = 1; e.button.which = 0x7F000001u; }
    SDL_PushEvent(&e);
}
static void gpPushText(const char* t) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_TEXTINPUT; strncpy(e.text.text, t, sizeof e.text.text - 1); SDL_PushEvent(&e); }
static void gpPushKey(SDL_Keycode k) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = k; e.key.state = 1; SDL_PushEvent(&e); e.type = SDL_KEYUP; e.key.state = 0; SDL_PushEvent(&e); }
static void gpUiFrame(SDL_Window* win, float dt) {   // chamado todo quadro em que ha tela/menu/inventario aberto
    GpPad* p = gChatAs2 ? gpPadFor(2 + gChatK) : gInvOwner == 2 ? gpPadFor(gInvK + 2) : (gpPadFor(1) ? gpPadFor(1) : (gSplit ? nullptr : gpNth(0))); if (!p) { gpUiWas = false; return; }
    int ww = 1, wh = 1; SDL_GetWindowSize(win, &ww, &wh); if (gUiSubOn) { ww = gUiOw; wh = gUiOh; }
    if (!gpUiWas) { gpCurT = SDL_GetTicks(); int mx = ww / 2, my = wh / 2; if (!gUiSubOn) SDL_GetMouseState(&mx, &my); gpCurX = (float)mx; gpCurY = (float)my; gpUiWas = true; gpLDown = gpRDown = false; }
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
    // patchPad.2: cursor "grudento" nos slots (estilo Xbox): direcional pula de slot em slot; o stick prende no slot e solta ao empurrar mais forte
    static std::vector<std::pair<float, float>> pts; static int gpSnap = -1; static float gpAccX = 0.f, gpAccY = 0.f; pts.clear(); float rad = 0.f;
    gBtnOn = true; gBtnSnap.swap(gBtnPts); gBtnPts.clear();   // patchRpi.4: botoes do quadro anterior
    bool snapOn = gpSlotPoints(ww, wh, pts, rad); if (!snapOn && !gBtnSnap.empty()) { pts = gBtnSnap; rad = gBtnRad; snapOn = true; } const int np = (int)pts.size();
    auto nearestSlot = [&](float px, float py, float maxd) { int bi = -1; float bd = maxd * maxd; for (int i = 0; i < np; i++) { const float ex = pts[i].first - px, ey = pts[i].second - py, d2 = ex * ex + ey * ey; if (d2 < bd) { bd = d2; bi = i; } } return bi; };
    auto setCur = [&](float nx, float ny) { gpCurX = std::max(0.f, std::min((float)ww - 1, nx)); gpCurY = std::max(0.f, std::min((float)wh - 1, ny)); gpPushMouse(win, SDL_MOUSEMOTION, 0, (int)gpCurX, (int)gpCurY); gpCurT = SDL_GetTicks(); };
    float dx = snapOn ? 0.f : ((p->b[GB_RIGHT] ? 1.f : 0.f) - (p->b[GB_LEFT] ? 1.f : 0.f)) * 2.f, dy = snapOn ? 0.f : ((p->b[GB_DOWN] ? 1.f : 0.f) - (p->b[GB_UP] ? 1.f : 0.f)) * 2.f;   // patchRpi.3: direcional anda nos 4 sentidos nos menus (antes so na horizontal)
    const float sp = wh * 0.85f * dt;   // um quadro de tela em ~1,2 s com o stick no maximo
    const float mx = (x * fabsf(x) * .5f + x * .5f + dx * .5f) * sp, my = (y * fabsf(y) * .5f + y * .5f + dy) * sp;
    if (!snapOn) { gpSnap = -1; if (mx != 0.f || my != 0.f) { gpCurX = std::max(0.f, std::min((float)ww - 1, gpCurX + mx)); gpCurY = std::max(0.f, std::min((float)wh - 1, gpCurY + my)); gpPushMouse(win, SDL_MOUSEMOTION, 0, (int)gpCurX, (int)gpCurY); } }
    else {
        if (gpSnap >= np) gpSnap = -1;
        const int ddx = (gpBtnPressed(p, GB_RIGHT) ? 1 : 0) - (gpBtnPressed(p, GB_LEFT) ? 1 : 0), ddy = (gpBtnPressed(p, GB_DOWN) ? 1 : 0) - (gpBtnPressed(p, GB_UP) ? 1 : 0);
        if (ddx || ddy) {   // direcional: vai para o slot mais proximo naquela direcao
            const int from = gpSnap >= 0 ? gpSnap : nearestSlot(gpCurX, gpCurY, 1e9f); int best = -1; float bs = 1e9f;
            if (from >= 0) for (int i = 0; i < np; i++) {
                if (i == from) continue; const float rx = pts[i].first - pts[from].first, ry = pts[i].second - pts[from].second, along = ddx ? rx * ddx : ry * ddy, perp = ddx ? fabsf(ry) : fabsf(rx);
                if (along < rad * .8f) continue; const float sc = along + perp * 2.f; if (sc < bs) { bs = sc; best = i; }
            }
            if (best >= 0) { gpSnap = best; gpAccX = gpAccY = 0.f; setCur(pts[best].first, pts[best].second); }
        } else if (gpSnap >= 0) {
            if (mx != 0.f || my != 0.f) { gpAccX += mx; gpAccY += my; if (hypotf(gpAccX, gpAccY) > rad) { const float ox = pts[gpSnap].first + gpAccX, oy = pts[gpSnap].second + gpAccY; gpSnap = -1; gpAccX = gpAccY = 0.f; setCur(ox, oy); } }
            if (gpSnap >= 0 && (fabsf(gpCurX - pts[gpSnap].first) > .5f || fabsf(gpCurY - pts[gpSnap].second) > .5f)) setCur(pts[gpSnap].first, pts[gpSnap].second);   // layout mudou: acompanha o slot
        } else {
            if (mx != 0.f || my != 0.f) setCur(gpCurX + mx, gpCurY + my);
            const int ni = nearestSlot(gpCurX, gpCurY, rad * .55f); if (ni >= 0) { gpSnap = ni; gpAccX = gpAccY = 0.f; setCur(pts[ni].first, pts[ni].second); }   // imã: prende no slot
        }
    }
    const bool a = p->b[GB_A], xb = p->b[GB_X];
    if (mx != 0.f || my != 0.f || a || xb) gpCurT = SDL_GetTicks();
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
static void gpUiLeave() { gBtnOn = false; gBtnPts.clear(); gpUiWas = false; gpLDown = gpRDown = false; }
