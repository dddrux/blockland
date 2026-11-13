// ================= splitscreen.inc: split screen local de 2 jogadores =================
// Jogador 1 = o jogador normal do jogo (teclado/mouse ou controle). Jogador 2 = estado proprio (SX) simulado aqui com o MESMO mundo:
// fisica real (moveEntityWithHeading: andar, pular, correr, nadar, escada, voar no criativo), minerar/colocar blocos, bater em mobs, usar alavanca/redstone,
// coletar drops e INVENTARIO COMPLETO (o inventario normal abre trocando o conteudo de inv/armor/craft pelo do jogador 2 enquanto a tela esta aberta).
// A renderizacao roda o bloco de desenho do main_loop 1x por viewport, trocando o contexto (P, yaw, pitch, hotbar...) para o jogador 2 na 2a passada.
// Controle: gpPadFor(2) em gamepad.inc (2+ controles: o 2o e do jogador 2; 1 controle: ele e do jogador 2 e o jogador 1 fica com teclado e mouse).
// Tem tambem (SurvCtx): vida/fome/morte e renascer, comida, arco, bote (gBoatX[k], J2..J4), bau, e os mobs hostis o miram (codigo; (VIS) conferir no jogo).
// Limites conhecidos: o P2 so tem controle (sem teclado/mouse);
// o jogador 2 fica preso a (RAD-2) chunks do jogador 1 (o mundo so carrega em volta do jogador 1) e volta para perto dele se passar disso ou cair no vazio.
struct SurvCtx {   // estado de sobrevivencia (survival.inc/bow.inc) de UM jogador; o do jogador 2 e trocado com os globais para reusar survTick/survDamage/eatTick/bowTick
    int hp = 20, food = 20, fire = 0, hurt = 0, invul = 0, regenT = 0, deadT = 0, eatCd = 0, lastDmg = 0, eatT = -1, hungerT = 0, hungerA = 0, regenFx = 0, poisonT = 0, bowT = -1, respHold = 0, air = 300;
    float sat = 5.f, exh = 0.f, fall = 0.f, hurtAtYaw = 0.f; bool dead = false, wantResp = false, eatOn = false, bowOn = false; Uint32 eatMs = 0; std::string why;
};
struct SplitP {
    SurvCtx sv; Phys* pvpP = nullptr; Slot cur;
    Phys P; float yaw = 0, pitch = 0, bodyYaw = 0, prevBodyYaw = 0, limbSwing = 0, limbAmt = 0, prevLimbAmt = 0, swing = 0, ageT = 0;
    bool init = false, swinging = false, sprintL = false, lastJump = false, atkHeld = false;
    int sel = 0, jumpTicks = 0, flyTimer = 0; float rep = 0, mnT = 0, mnNeed = 1, mnCool = 0; bool mnOn = false; int mnX = 0, mnY = 0, mnZ = 0;
    Slot inv[36], armor[4], craftG[9], craftOut, offh;
    float hb[4] = {0, 0, 0, 0};   // balanco da mao ao girar a camera (proprio de cada jogador)
    float fovMod = 1.f, prevFovMod = 1.f, cbY = 0, pcbY = 0, cbP = 0, pcbP = 0;   // patchSplit.2: FOV dinamico e balanco de camera PROPRIOS de cada jogador
    CapeAnim cape; int capeSel = 0, capeLoaded = 0;   // patchSplit.6: capa PROPRIA de cada corpo (capeSel = indice como em "capesel"; carrega sozinha)
    int slpT = 0, slpBx = 0, slpBy = 0, slpBz = 0, slpHx = 0, slpHz = 0; float slpYaw = 0;   // patchSleep.2: sono PROPRIO do extra (slpT > 0 = deitado; slpYaw so p/ futura pose)
    GLuint skinTex = 0; GpIn in; int boatId = 0; float boatLast = 0, eggCd = 0;   // eggCd: segundos ate poder arremessar ovo de novo
};
static SplitP gSX[3];   // patchSplit.1: jogadores extras (J2, J3, J4)
#define SX (gSX[gS2i])   // extra "atual" (troque com SxSel)
struct SxSel { int old; explicit SxSel(int k) : old(gS2i) { gS2i = k; } ~SxSel() { gS2i = old; } };
// patchSplit.6: F9 = teclado/mouse assumem o proximo jogador. O corpo (P, yaw, inventario, vida, skin...) do J1 troca de lugar com o do extra; o controle que mexia no corpo vai junto.
// gBodyId[0] = corpo que o teclado controla agora; gBodyId[k+1] = corpo do extra k (0 = o 1o jogador, k+1 = extra k). gPossNext = proximo extra a trocar (roda 0,1,2,0...).
static GLuint split2CapeTex(SplitP& s) {   // textura da capa do extra (carrega/troca o arquivo quando a selecao muda); 0 = sem capa
    if (s.capeSel != s.capeLoaded || (s.capeSel > 0 && s.cape.fr.empty())) {
        s.capeLoaded = s.capeSel; s.cape.clear(); const int c = s.capeSel;
        if (c >= 1 && c <= CAPE_NBI) capeBuiltin(c, s.cape);
        else if (c > CAPE_NBI) { capeScan(); if (c - 1 - CAPE_NBI < (int)gCapeFiles.size()) { if (!capeFromFile("assets/capes/" + gCapeFiles[(size_t)(c - 1 - CAPE_NBI)], s.cape)) { s.cape.clear(); s.capeSel = 0; } } else s.capeSel = 0; }
    }
    return s.capeSel > 0 && !s.cape.fr.empty() ? s.cape.texAt(SDL_GetTicks()) : 0;
}
static int gPossNext = 0, gBodyId[4] = {0, 1, 2, 3};
static void split2PossessReset() { gPossNext = 0; for (int i = 0; i < 4; i++) gBodyId[i] = i; }
static int split2PossessPick() {   // extra k que sera trocado agora (-1 = nenhum disponivel)
    const int n = gSplitN - 1; if (!gSplit || n < 1) return -1;
    for (int t = 0; t < n; t++) { const int k = (gPossNext + t) % n; if (gSX[k].init) return k; }
    return -1;
}
static void split2PossessPads(int k) {   // o controle do J1 passa para o extra k e o do extra k passa a ser o do J1 (cada controle segue o seu corpo)
    int jp = -1; for (GpPad& q : gPads) if (q.c && !gpIsExtra(q.id)) { jp = q.id; break; }
    gXPadId[k] = jp;
}
static void split2PossessDone(int k) { std::swap(gBodyId[0], gBodyId[k + 1]); gPossNext = (k + 1) % std::max(1, gSplitN - 1); }
static int gSplitMode = 0;      // 0 = topo/baixo (como o Bedrock), 1 = lado a lado
static void split2SwapInv() {   // troca o inventario global com o do jogador 2 (usado ao abrir/fechar a tela de inventario do jogador 2 e para addItem)
    for (int i = 0; i < 36; i++) std::swap(inv[i], SX.inv[i]);
    for (int i = 0; i < 4; i++) std::swap(armor[i], SX.armor[i]);
    for (int i = 0; i < 9; i++) std::swap(craftG[i], SX.craftG[i]);
    std::swap(craftOut, SX.craftOut); std::swap(offh, SX.offh); std::swap(cur, SX.cur);
}
static void split2SwapSurv() {   // troca vida/fome/fogo/comer/arco... entre os globais (jogador 1) e SX.sv (jogador 2)
    SurvCtx& c = SX.sv;
    std::swap(gHP, c.hp); std::swap(gFood, c.food); std::swap(gFire, c.fire); std::swap(gHurt, c.hurt); std::swap(gInvul, c.invul); std::swap(gRegenT, c.regenT); std::swap(gDeadT, c.deadT);
    std::swap(gEatCd, c.eatCd); std::swap(gLastDmg, c.lastDmg); std::swap(gEatT, c.eatT); std::swap(gHungerT, c.hungerT); std::swap(gHungerA, c.hungerA); std::swap(gRegenFx, c.regenFx); std::swap(gPoisonT, c.poisonT); std::swap(gAir, c.air);
    std::swap(gBowT, c.bowT); std::swap(gRespHold, c.respHold); std::swap(gSat, c.sat); std::swap(gExh, c.exh); std::swap(gFall, c.fall); std::swap(gHurtAtYaw, c.hurtAtYaw);
    std::swap(gDead, c.dead); std::swap(gWantRespawn, c.wantResp); std::swap(gEatOn, c.eatOn); std::swap(gBowOn, c.bowOn); std::swap(gEatMs, c.eatMs); std::swap(gDeathWhy, c.why); std::swap(gPvpP, SX.pvpP);
}
struct S2Ctx {   // enquanto existir, os globais de inventario/sobrevivencia SAO do jogador 2 (RAII: sempre desfaz)
    S2Ctx() { split2SwapInv(); split2SwapSurv(); gSplitCtx = 2; }
    ~S2Ctx() { split2SwapSurv(); split2SwapInv(); gSplitCtx = 0; gUseOverride = -1; }
};
static int split2N() { return gSplit ? gSplitN - 1 : 0; }   // quantidade de extras ativos
static bool split2TargetableK(int k) { return gSplit && k >= 0 && k < gSplitN - 1 && gSX[k].init && gSplitCtx != 2 && !gSX[k].sv.dead && !creative && gGm != 3; }
static bool split2Targetable() { for (int k = 0; k < split2N(); k++) if (split2TargetableK(k)) return true; return false; }
static double split2Dist2(double x, double z) { double b = 1e18; for (int k = 0; k < split2N(); k++) if (split2TargetableK(k)) b = std::min(b, (gSX[k].P.x - x) * (gSX[k].P.x - x) + (gSX[k].P.z - z) * (gSX[k].P.z - z)); return b; }   // distancia^2 (XZ) ao extra alvo mais proximo (mobsync.inc)
static const Phys* split2PhysK(int k) { return &gSX[k].P; }
static const Phys* split2PhysP() { return &gSX[0].P; }
static std::vector<Phys*> pistonAll() { std::vector<Phys*> v; v.push_back(gPvpP); for (int k = 0; k < split2N(); k++) if (split2TargetableK(k)) v.push_back(&gSX[k].P); return v; }   // pistao/placa de pressao/colocar bloco: todos os jogadores
static void split2DamageK(int k, int dmg, const char* why, bool force, float kx, float kz) {
    if (k < 0 || k > 2 || !gSX[k].init) return; SxSel q(k); bool hit; { S2Ctx c; hit = survDamage(dmg, why, force); }
    if (hit) { SX.P.mx += kx; SX.P.mz += kz; }
}
static void split2Damage(int dmg, const char* why, bool force, float kx, float kz) { split2DamageK(0, dmg, why, force, kx, kz); }
static bool split2PvpHit(int k, int dmg, const char* why, float kx, float kz) {   // patchSplit.4: golpe de jogador no extra k (recuo do 1.8.9)
    if (k < 0 || k > 2 || !gSX[k].init) return false; SxSel q(k); bool hit; { S2Ctx c; hit = survDamage(dmg, why, false); }
    if (hit) { Phys& P = SX.P; P.mx /= 2; P.mz /= 2; P.my = std::min(.4, P.my / 2 + .4); P.mx += kx; P.mz += kz; }
    return hit;
}
static void split2PvpReceive(int k, int from, int dmg, float kx, float kz, float kb, float oax, float oaz) {   // NP_PVP vindo da rede para o extra k deste cliente
    if (k < 0 || k > 2 || !gSX[k].init || !gSplit) return; SxSel q(k); S2Ctx c;
    pvpApply1(netLabel(from), true, oax, oaz, dmg, kx, kz, kb);
}
static bool gSplitSkinOff = false;   // so para testes headless (sem GL)
static void split2LoadSkin() {   // textura propria do jogador 2 (sem mexer no modelo/skin do jogador 1)
    static const char* const kXSkins[3] = {"", "azy.png", "drux.png"};   // reserva se o perfil estiver vazio
    const std::string sf = !gXSkin[gS2i].empty() ? gXSkin[gS2i] : std::string(kXSkins[gS2i]);
    if (gSplitSkinOff || SX.skinTex || sf.empty()) return;
    std::vector<uint8_t> bak(skin, skin + sizeof skin); const bool bs = slim, ba = slimAuto; const GLuint keep = skinTex;
    if (loadSkin(("assets/skins/" + sf).c_str())) { skinTex = 0; uploadSkin(); SX.skinTex = skinTex; }
    skinTex = keep; memcpy(skin, bak.data(), sizeof skin); slim = bs; slimAuto = ba; uploadSkin(); buildModel();
}
static void split2Pick(SplitP& s, bool has, const RayHit& r) {   // pick block do P2 (mesma regra do P1: hotbar > vazio > substitui; so bloco)
    if (!has || gGm == 3) return; const int pb = getB(r.x, r.y, r.z); if (pb == AIR || isLiquid(pb)) return;
    const int id = pickItemAt(r.x, r.y, r.z); if (id <= 0) return;
    for (int i = 0; i < 9; i++) if (s.inv[i].id == id) { s.sel = i; return; }
    int src = -1; for (int i = 9; i < 36; i++) if (s.inv[i].id == id) { src = i; break; }
    if (src < 0 && !creative) return;
    if (s.inv[s.sel].id) { int e = -1; for (int i = 0; i < 9; i++) if (!s.inv[i].id) { e = i; break; } if (e >= 0) { s.inv[e] = s.inv[s.sel]; s.inv[s.sel] = Slot(); } }
    if (src >= 0) std::swap(s.inv[src], s.inv[s.sel]); else { s.inv[s.sel] = Slot(); s.inv[s.sel].id = id; s.inv[s.sel].n = 1; }
}
static void split2Spawn(const Phys& p1, float yaw) {
    if (gS2i >= 0 && gS2i < 3) gBoatX[gS2i] = 0; SplitP& s = SX; s = SplitP(); s.init = true;   // (a textura e recriada abaixo)
    static const float kAng[3] = {0.f, 180.f, 90.f}; const float ang = yaw + kAng[gS2i];
    const double ox = -cos(ang * .0174533) * 1.2, oz = sin(ang * .0174533) * 1.2;   // ao lado do jogador 1
    s.P.x = s.P.px = p1.x + (collEmpty(AABB{p1.x + ox - PW, p1.y, p1.z + oz - PW, p1.x + ox + PW, p1.y + PH, p1.z + oz + PW}) ? ox : 0);
    s.P.z = s.P.pz = p1.z + (collEmpty(AABB{p1.x + ox - PW, p1.y, p1.z + oz - PW, p1.x + ox + PW, p1.y + PH, p1.z + oz + PW}) ? oz : 0);
    s.P.y = s.P.py = p1.y; s.yaw = yaw; s.pvpP = &s.P; s.sv.lastDmg = 0;
    // patchGm.2: sem kit inicial
    split2LoadSkin();
}
static void split2Fall(const Phys& p1) {   // volta para perto do jogador 1 (preso ao raio carregado ou caiu no vazio)
    static const double kdx[3] = {.8, -.8, 0}, kdz[3] = {0, 0, .8}; SX.P.x = SX.P.px = p1.x + kdx[gS2i]; SX.P.z = SX.P.pz = p1.z + kdz[gS2i]; SX.P.y = SX.P.py = p1.y + .2; SX.P.mx = SX.P.my = SX.P.mz = 0;
}
static void split2Follow(const Phys& p1) { if (!gSplit) return; for (int k = 0; k < gSplitN - 1; k++) { if (!gSX[k].init) continue; SxSel q(k); SX.slpT = 0; split2Fall(p1); } }   // patchSleep.2: troca de dimensao: os extras vao junto, acordados, para perto do jogador 1
static std::string splitNick(int k) { return xNickK(k); }   // nick do extra k (0 = J2)
static void split2SetN(int n, const Phys& p1, float yaw) {   // n = 1 (desligado) .. 4 jogadores
    n = std::max(1, std::min(4, n)); const bool on = n >= 2;
    split2PossessReset();   // patchSplit.6
#ifdef BL_RPI
    if (on) { gSt["splitscr"] = 0; chatAdd("Split screen desativado no Raspberry (desempenho)", 0xFF5555); return; }   // patchRpiOpt.1: sem splitscreen no Pi
#endif
    if (on == gSplit && (!on || n == gSplitN)) return;
#ifdef __ANDROID__
    if (on) { chatAdd("Split screen indisponível nesta plataforma", 0xFF5555); return; }   // Android: sem split por enquanto (precisa de scissor+viewport e toque por metade da tela)
#endif
    const bool was = gSplit; gSplit = on; if (on) gSplitN = n;
#ifdef BL_RPI
    gRpiSplit = on; RAD = rdChunks();   // Raspberry: split = 4 chunks e 30 FPS
#endif
    for (int k = on ? n - 1 : 0; k < 3; k++) netSendXGone(k);   // patchSplit.3: quem saiu do split some para a rede
    if (!on) for (int k = 0; k < 3; k++) gBoatX[k] = 0;
    if (on) for (int k = 0; k < n - 1; k++) { SxSel q(k); if (gSX[k].init) { if (!was) split2Fall(p1); } else split2Spawn(p1, yaw); }   // religar: volta para perto do jogador 1 (mantem o inventario)
}
static void split2PadStart(int padId, const Phys& p1, float yaw) {   // patchSplit.5: START de um controle: abre uma janela para ele; START de novo fecha (1 controle = 1 janela, nunca duas)
    if (gInvOwner == 2 || gChatAs2) return;
    int k = -1; for (int i = 0; i < 3; i++) if (gXPadId[i] == padId && gSplit && i < gSplitN - 1) k = i;
    if (k < 0) {   // entrar
        const int n = gSplit ? gSplitN : 1; if (n >= 4) { chatAdd("Tela dividida: limite de 4 jogadores", 0xFF5555); return; }
        gXPadId[n - 1] = padId; split2SetN(n + 1, p1, yaw); if (!gSplit) gXPadId[n - 1] = -1;   // SetN pode recusar (Android/RPi)
        if (gSplit) { gSt["splitscr"] = 1; gSt["splitn"] = gSplitN; chatAdd("Jogador " + std::to_string(gSplitN) + " entrou (START de novo sai)", 0xFFFFFF); }
        return;
    }
    const int last = gSplitN - 2;   // sair: troca com o ultimo extra ativo (mantem inventarios dos outros) e desliga o ultimo
    if (k != last) {
        std::swap(gSX[k], gSX[last]); gSX[k].pvpP = &gSX[k].P; gSX[last].pvpP = &gSX[last].P;
        std::swap(gXPadId[k], gXPadId[last]); std::swap(gXName[k], gXName[last]); std::swap(gXSkin[k], gXSkin[last]); std::swap(gBoatX[k], gBoatX[last]);
        std::swap(gTx[xTxKey(k)], gTx[xTxKey(last)]);
    }
    gXPadId[last] = -1; split2SetN(gSplitN - 1, p1, yaw);
    gSt["splitscr"] = gSplit ? 1 : 0; if (gSplit) gSt["splitn"] = gSplitN;
    chatAdd(gSplit ? "Jogador saiu da tela dividida" : "Tela dividida desligada", 0xFFFFFF);
}
// ---- por quadro: olhar, botoes (bater/quebrar, usar/colocar/comer/arco, hotbar, largar, inventario/bau/mesa). Devolve true se abriu a tela de inventario ----
static void split2Respawn(const Phys& p1) {   // renasce perto do jogador 1 (inventario ja foi solto na morte) com vida/fome cheias
    if (gS2i >= 0 && gS2i < 3) gBoatX[gS2i] = 0; SX.sv = SurvCtx(); SX.sv.invul = 20; SX.sv.lastDmg = 1000; SX.P.flying = false; SX.sprintL = false; split2Fall(p1); chatAdd(splitNick(gS2i) + " renasceu", 0xFFFFFF);
}
static bool split2Frame1(float dt, bool active, const Phys& p1) {
    SplitP& s = SX; if (!s.init) return false;
    GpPad* pad = gpPadFor(gS2i + 2); const bool alive = !s.sv.dead; s.in = (active && alive) ? gpRead(pad) : GpIn(); Phys& P = s.P; bool wantInv = false;
    if (active && alive && gpPressed(pad, GA_CHAT)) if (gS2i >= 0 && gS2i < 3) { gP2ChatReq = true; gP2ChatReqK = gS2i; }   // main_loop abre o chat como jogador 2
    s.ageT += dt * 20.f;
    if (s.swinging) { s.swing += dt * 5.f; if (s.swing >= 1.f) { s.swing = 0; s.swinging = false; } }   // mesmo ritmo do balanco do jogador 1 (~0,2 s)
    s.mnCool = std::max(0.f, s.mnCool - dt); s.eggCd = std::max(0.f, s.eggCd - dt);
    if (gS2i >= 0 && gS2i < 3) { gBoatXP[gS2i] = &P; const bool ok = active && pad && alive; bool* bx = gBX[gS2i]; bx[0] = ok && s.in.str < -.5f; bx[1] = ok && s.in.str > .5f; bx[2] = ok && s.in.fwd > .5f; bx[3] = ok && s.in.fwd < -.5f; bx[4] = ok && s.in.sneak; }   // comandos do bote
    if (!active || !pad || !alive || gGm == 3) { s.mnOn = false; s.atkHeld = false; if (gGm == 3) P.noclip = true; return false; }
    s.yaw = wrap180(s.yaw - s.in.lx * GP_LOOK_DEG * gGpSens * dt);
    s.pitch = std::max(-89.f, std::min(89.f, s.pitch - s.in.ly * GP_LOOK_DEG * gGpSens * dt));
    if (gpPressed(pad, GA_SPRINT) && s.in.fwd > .5f) s.sprintL = true; else if (s.in.fwd < .3f) s.sprintL = false;
    if (gpPressed(pad, GA_NEXT) || gpBtnPressed(pad, GB_RIGHT)) s.sel = (s.sel + 1) % 9;
    if (gpPressed(pad, GA_PREV) || gpBtnPressed(pad, GB_LEFT)) s.sel = (s.sel + 8) % 9;
    if (gpPressed(pad, GA_INV)) wantInv = true;
    if (gpPressed(pad, GA_SWAP) && gGm != 3) std::swap(s.inv[s.sel], s.offh);
    Slot& held = s.inv[s.sel];
    const float eyeH = P.sneak ? 1.54f : 1.62f; const float eye[3] = {(float)P.x, (float)(P.y + eyeH), (float)P.z};
    const float cy = cosf(s.yaw * .0174533f), sy = sinf(s.yaw * .0174533f), cp = cosf(s.pitch * .0174533f), sp = sinf(s.pitch * .0174533f);
    const float dir[3] = {-sy * cp, sp, -cy * cp};
    if (gpPressed(pad, GA_DROP) && held.id) { const double e3[3] = {eye[0], eye[1], eye[2]}; throwItem(e3, s.yaw, s.pitch, held.id, 1); if (--held.n <= 0) held = Slot(); s.swinging = true; s.swing = 0; }
    RayHit r; const bool has = rayCast(eye, dir, gBlockReach, r, false);
    // ---- atacar / quebrar (gatilho de ataque) ----
    if (gpPressed(pad, GA_PICK)) split2Pick(s, has, r);
    const bool atkPress = gpPressed(pad, GA_ATTACK), atk = s.in.attack;
    if (atkPress && boatHit(eye, dir, held.id, creative, creative ? 6.f : 3.f)) { s.swinging = true; s.swing = 0; s.mnOn = false; }   // bater no bote
    else if (atkPress && pvpAttackK(gS2i, eye, dir, held.id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { s.swinging = true; s.swing = 0; s.mnOn = false; if (P.sprint) { P.mx *= .6; P.mz *= .6; } P.sprint = false; s.sprintL = false; }   // patchSplit.4: PvP
    else if (atkPress && mobAttack(eye, dir, held.id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { s.swinging = true; s.swing = 0; s.mnOn = false; }
    else if (atk) {
        if (atkPress) { s.swinging = true; s.swing = 0; }
        const int b = has ? getB(r.x, r.y, r.z) : AIR;
        if (!has || b == AIR || b == BEDROCK || isWater(b) || gGm == 2 || (creative && isSwordId(held.id))) s.mnOn = false;   // patchSword.1: espada no criativo nao quebra
        else if (creative) { if (atkPress || s.mnCool <= 0.f) {
                spawnBreak(r.x, r.y, r.z, b); setB(r.x, r.y, r.z, AIR); netSendBlock(r.x, r.y, r.z, AIR);
                if (isLogB(b) || isLeafB(b)) { keepLeaves.erase(k3(r.x, r.y, r.z)); leafNeighbors(r.x, r.y, r.z); }
                s.mnCool = .25f; s.swinging = true; s.swing = 0; } }
        else {   // sobrevivencia: o tempo depende da dureza e da ferramenta (mesma tabela do jogador 1)
            bool hv = true;
            if (!s.mnOn || s.mnX != r.x || s.mnY != r.y || s.mnZ != r.z) { s.mnOn = true; s.mnX = r.x; s.mnY = r.y; s.mnZ = r.z; s.mnT = 0; s.mnNeed = breakTime(b, held.id, hv); }
            if (!s.swinging) { s.swinging = true; s.swing = 0; }
            if (s.mnCool <= 0.f) { s.mnT += dt; if (s.mnT >= s.mnNeed) {
                hv = true; breakTime(b, held.id, hv);
                spawnBreak(r.x, r.y, r.z, b); setB(r.x, r.y, r.z, AIR); netSendBlock(r.x, r.y, r.z, AIR);
                if (isLogB(b) || isLeafB(b)) { keepLeaves.erase(k3(r.x, r.y, r.z)); leafNeighbors(r.x, r.y, r.z); }
                if (hv) blockDrops(r.x, r.y, r.z, b);
                { S2Ctx c; survExh(.025f); }   // minerar cansa
                s.mnT = 0; s.mnCool = .25f; s.mnOn = false; } }
        }
    } else s.mnOn = false;
    // ---- usar (gatilho de uso): comer / arco ficam por conta do tick (segurar); aqui: alavanca, redstone, bau, mesa, colocar bloco ----
    const bool usePress = gpPressed(pad, GA_USE);
    bool useItem = held.id == IT_BOW; if (!useItem) { S2Ctx c; useItem = survCanEat(inv[s.sel].id); }   // item que se "usa" segurando
    if (s.in.use) { s.rep += dt; } else s.rep = 0.f;
    const bool boated = usePress && gS2i >= 0 && gS2i < 3 && gGm != 3 && !gBoatX[gS2i] && boatInteract(eye, dir, creative ? 6.f : 3.f, gBoatX[gS2i]); if (boated) { s.swinging = true; s.swing = 0; }   // montar no bote
    const bool eggT = usePress && !boated && (held.id == IT_EGG || potThrowable(held.id)) && gGm != 3 && s.eggCd <= 0.f && (!has || P.sneak || !eggBlockUsable(r.x, r.y, r.z));   // ovo (egg.inc): arremessa
    if (eggT) { int cd = 0; if (eggThrow(P, s.yaw, s.pitch, held, cd)) { s.eggCd = .5f; s.swinging = true; s.swing = 0; } }
    const int bty = (usePress && !boated && held.id) ? boatTypeOfItem(held.id) : -1;   // item de bote: coloca na agua
    if (bty >= 0 && gGm != 2) { if (boatPlace(bty, eye, dir, s.yaw, 5.f)) { s.swinging = true; s.swing = 0; if (!creative && --held.n <= 0) held = Slot(); } }
    else if (!boated && !eggT && has && gGm != 3 && !useItem && (usePress || (s.in.use && s.rep > .3f))) {
        if (!usePress) s.rep = .2f;
        const int hb = getB(r.x, r.y, r.z);
        if (hb == XB_CHEST && usePress && !(P.sneak && held.id)) { const int cu = chestOpenAt(r.x, r.y, r.z); if (cu) { if (cu == 1) wantInv = true; s.swinging = true; s.swing = 0; } }
        else if (hb == CTABLE && !P.sneak && usePress) { gTable = true; wantInv = true; s.swinging = true; s.swing = 0; }
        else if (xbIsLever(hb) && !P.sneak && usePress) { xbToggleLever(r.x, r.y, r.z); s.swinging = true; s.swing = 0; }
        else if (!P.sneak && usePress && ([&] { gS2Use = true; const bool u = rsUse(r.x, r.y, r.z); gS2Use = false; return u; })()) { s.swinging = true; s.swing = 0; }
        else if (gGm != 2 && held.id && isBlk(held.id)) {
            PlaceCtx pc{r.x, r.y, r.z, r.face, r.hx - r.x, r.hy - r.y, r.hz - r.z, s.yaw, s.pitch, P.sneak, P.x, P.y, P.z}; int tp[3];
            if (placeBlock(held.id, pc, tp)) {
                s.swinging = true; s.swing = 0; if (isLeafB(held.id)) keepLeaves.insert(k3(tp[0], tp[1], tp[2]));
                if (!creative && --held.n <= 0) held = Slot();
            }
        }
    }
    (void)p1; return wantInv;
}
// ---- por tick (20/s): fisica identica a do jogador 1 + sobrevivencia (queda, lava, fogo, fome, regeneracao, comer, arco) no contexto do jogador 2 ----
// patchSleep.2: sono dos jogadores extras do split (J2..J4): mesmas regras de sleep.inc (noite/tempestade, monstro perto, todos precisam dormir 100+ ticks)
static bool sleepExtraUse(int fx, int y, int fz, int hx, int hz, float yaw) {   // chamado por sleepUseBlockBed com SX = extra atual
    SplitP& s = SX; if (s.slpT > 0 || s.sv.dead || gGm == 3) return true;
    if (gDim != 3) {
        if (!gDayCycle) { chatAdd(splitNick(gS2i) + ": o tempo está parado.", 0xFFFF55); return true; }
        if (!sleepNightNow()) { chatAdd(splitNick(gS2i) + ": você só pode dormir à noite ou durante tempestades.", 0xFFFF55); return true; }
        if (sleepMonsterNear(fx + .5, y, fz + .5)) { chatAdd(splitNick(gS2i) + ": há monstros por perto.", 0xFFFF55); return true; }
    }
    s.slpT = 1; s.slpBx = fx; s.slpBy = y; s.slpBz = fz; s.slpHx = hx; s.slpHz = hz; s.slpYaw = yaw; return true;
}
static void sleepExtraCount(int& tot, int& sleeping, int& full) {
    tot = sleeping = full = 0;
    for (int k = 0; k < gSplitN - 1; k++) { const SplitP& s = gSX[k]; if (!s.init || s.sv.dead) continue; tot++; if (s.slpT > 0) { sleeping++; if (s.slpT >= SLEEP_FULL_TICKS) full++; } }
}
static bool split2SleepStep(SplitP& s, bool leave) {   // 1 tick; true = esta deitado (o tick pula o movimento)
    if (s.slpT <= 0) return false;
    Phys& P = s.P;
    const bool ok = getB(s.slpBx, s.slpBy, s.slpBz) == XB_BED && getB(s.slpHx, s.slpBy, s.slpHz) == XB_BED, stay = gDim == 3 || (gDayCycle && sleepNightNow());
    if (!ok || leave || !stay || s.sv.dead) { s.slpT = 0; if (ok) { double o[3]; sleepWakeSpot(s.slpBx, s.slpBy, s.slpBz, s.slpHx, s.slpHz, o); P.x = P.px = o[0]; P.y = P.py = o[1]; P.z = P.pz = o[2]; } return false; }
    P.x = P.px = (s.slpBx + s.slpHx) / 2.0 + .5; P.z = P.pz = (s.slpBz + s.slpHz) / 2.0 + .5; P.y = P.py = s.slpBy + .5625;
    ++s.slpT; return true;
}
static void split2Tick1(const Phys& p1, bool active) {
    SplitP& s = SX; if (!s.init) return; Phys& P = s.P;
    P.px = P.x; P.py = P.y; P.pz = P.z; P.prevDistWalked = P.distWalked; s.prevBodyYaw = s.bodyYaw; s.prevLimbAmt = s.limbAmt; s.prevFovMod = s.fovMod; s.pcbY = s.cbY; s.pcbP = s.cbP;
    for (int i = 0; i < 4; i++) std::swap(gHandBob[i], s.hb[i]); handBobTick(s.yaw, s.pitch); for (int i = 0; i < 4; i++) std::swap(gHandBob[i], s.hb[i]);
    if (!getC(fdiv((int)floor(P.x), CW), fdiv((int)floor(P.z), CW)) || !getC(fdiv((int)floor(P.x), CW), fdiv((int)floor(P.z), CW))->lc) { P.mx = P.my = P.mz = 0; return; }   // chunk do jogador 2 ainda carregando: fica parado (sem cair no vazio)
    if (s.jumpTicks > 0) --s.jumpTicks;
    P.inWater = inWaterNow(P); P.inLava = inLavaNow(P); if (P.inWater && !P.flying) waterPush(P);
    if (s.sv.dead && gS2i >= 0 && gS2i < 3) gBoatX[gS2i] = 0;
    const BoatEnt* rb2 = (gS2i >= 0 && gS2i < 3) ? boatById(gBoatX[gS2i]) : nullptr; const bool riding = rb2 != nullptr;   // sentado no bote (a posicao vem de boatTickAll)
    if (riding) { const float bo = boatYawO(*rb2); if (s.boatId != gBoatX[gS2i]) { s.boatId = gBoatX[gS2i]; s.boatLast = bo; } s.yaw = wrap180(s.yaw + wrap180(bo - s.boatLast)); s.boatLast = bo; } else s.boatId = 0;
    const bool alive = !s.sv.dead; const bool act = active && alive;
    const bool slp = split2SleepStep(s, act && (s.in.sneak || s.in.jump));   // patchSleep.2: deitado = sem andar/pular (corpo preso na cama)
    float fwd = act ? s.in.fwd : 0.f, str = act ? s.in.str : 0.f; const bool jump = act && s.in.jump, sneak = act && s.in.sneak;
    P.sneak = sneak && !P.flying;
    if (sneak) { str *= .3f; fwd *= .3f; }
    if (s.sv.bowOn || s.sv.eatOn) { str *= .2f; fwd *= .2f; }   // carregando o arco / comendo: 20% da velocidade
    if (creative && !s.lastJump && jump) { if (s.flyTimer == 0) s.flyTimer = 7; else { P.flying = !P.flying; s.flyTimer = 0; } }   // 2 toques rapidos em pular = voar
    if (s.flyTimer > 0) --s.flyTimer;
    s.lastJump = jump; if (!creative) P.flying = false; if (gGm == 3) P.flying = true; P.noclip = gGm == 3;
    if (P.flying) { if (sneak) P.my -= .15; if (jump) P.my += .15; }
    P.sprint = s.sprintL && fwd >= .8f && !sneak && !P.collH && !s.sv.bowOn && !s.sv.eatOn;
    if (fabs(P.mx) < .005) P.mx = 0; if (fabs(P.my) < .005) P.my = 0; if (fabs(P.mz) < .005) P.mz = 0;
    if (jump) {
        if (P.inWater || P.inLava) P.my += .04;
        else if (P.onGround && s.jumpTicks == 0) { P.my = .42; if (!creative) s.sv.exh += P.sprint ? .8f : .2f; if (P.sprint) { P.mx += -sin(s.yaw * .0174533) * .2; P.mz += -cos(s.yaw * .0174533) * .2; } s.jumpTicks = 10; }
    } else s.jumpTicks = 0;
    str *= .98f; fwd *= .98f;
    const bool ladder = !P.flying && climbBlk(getB((int)floor(P.x), (int)floor(P.y), (int)floor(P.z)));
    if (ladder) { P.mx = std::max(-.15, std::min(.15, P.mx)); P.mz = std::max(-.15, std::min(.15, P.mz)); P.my = std::max(P.my, -.15); if (sneak && P.my < 0) P.my = 0; s.sv.fall = 0.f; }
    if (riding || slp) { P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false; s.sv.fall = 0.f; s.jumpTicks = 0; } else { unstuckPlayer(P); moveEntityWithHeading(P, s.yaw, str, fwd); }
    if (ladder && P.collH) P.my = (.2 - .08) * .98;
    {   // sobrevivencia do jogador 2: no contexto dele os globais de vida/fome/inventario sao os dele
        S2Ctx c; gUseOverride = (act && s.in.use) ? 1 : 0;
        survTick(P);
        const bool can = act && gGm != 3;
        gBowSlot = &inv[s.sel]; bowTick(P, can, inv[s.sel].id, s.yaw, s.pitch); eatTick(P, can, inv[s.sel]); arrowsPickTick(P);
        if (can) hostileSpawn(P);   // tambem nasce monstro em volta do jogador 2
    }
    if (s.sv.dead && s.sv.deadT >= 60) split2Respawn(p1);
    if (P.onGround && P.flying && gGm != 3) P.flying = false;
    // corpo/membros (EntityLivingBase.onUpdate)
    { const double d0 = P.x - P.px, d1 = P.z - P.pz; float f1 = s.bodyYaw; if (d0 * d0 + d1 * d1 > .0025) f1 = (float)(atan2(-d0, -d1) * 57.29577951308232);
      if (s.swinging) f1 = s.yaw; s.bodyYaw += wrap180(f1 - s.bodyYaw) * .3f;
      float h = std::max(-75.f, std::min(75.f, wrap180(s.yaw - s.bodyYaw))); s.bodyYaw = s.yaw - h; if (h * h > 2500.f) s.bodyYaw += h * .2f;
      while (s.bodyYaw - s.prevBodyYaw < -180.f) s.prevBodyYaw -= 360.f; while (s.bodyYaw - s.prevBodyYaw >= 180.f) s.prevBodyYaw += 360.f;
      const float f6 = std::min(1.f, (float)sqrt(d0 * d0 + d1 * d1) * 4.f); s.limbAmt += (f6 - s.limbAmt) * .4f; s.limbSwing += s.limbAmt; }
    { const float tg = P.sprint ? 1.15f : 1.f; s.fovMod += (tg - s.fovMod) * .5f; s.fovMod = std::max(.1f, std::min(1.5f, s.fovMod));   // FOV ao correr (igual ao jogador 1)
      float f = (float)sqrt(P.mx * P.mx + P.mz * P.mz), f1 = (float)(atan(-P.my * .2) * 15.0); if (f > .1f) f = .1f; if (!P.onGround) f = 0.f; if (P.onGround) f1 = 0.f;
      s.cbY += (f - s.cbY) * .4f; s.cbP += (f1 - s.cbP) * .8f; }
    if (riding) { s.bodyYaw = boatYawO(*rb2); s.limbAmt = 0.f; while (s.bodyYaw - s.prevBodyYaw < -180.f) s.prevBodyYaw -= 360.f; while (s.bodyYaw - s.prevBodyYaw >= 180.f) s.prevBodyYaw += 360.f; }
    // coleta drops (o inventario global e trocado so durante o addItem)
    if (gGm != 3 && alive) for (size_t i = 0; i < DROPS.size();) {
        DropEnt& d = DROPS[i];
        if (d.delay <= 0 && d.x + .125 > P.x - 1.3 && d.x - .125 < P.x + 1.3 && d.z + .125 > P.z - 1.3 && d.z - .125 < P.z + 1.3 && d.y + .25 > P.y - .5 && d.y < P.y + 2.3) {
            if (gInvOwner == 2 && gInvK == gS2i) { ++i; continue; }   // inventario dele aberto agora: nao coleta
            const int got = d.n; split2SwapInv(); const int left = addItem(d.id, d.n, d.nbt); split2SwapInv();
            if (left < got) { netSendDropSet(d.nid, left, left <= 0 ? netSelf : 255); }
            d.n = left; if (left <= 0) { DROPS[i] = DROPS.back(); DROPS.pop_back(); continue; }
        }
        ++i;
    }
    if (netOn) { if (s.sv.dead) netSendXGone(gS2i); else netSendStateX(gS2i, P.x, P.y, P.z, s.yaw, s.pitch, s.swinging ? s.swing : 0.f, s.inv[s.sel].id, P.sneak, gGm, s.sv.bowOn ? std::max(0, (int)s.sv.bowT) : -1, s.sv.fire > 0 && !creative, riding, s.yaw, s.armor); }   // patchSplit.3: os outros jogadores da rede veem este jogador local
    if (P.y < -80 && creative) split2Fall(p1);   // caiu para fora do mundo no criativo: volta (em sobrevivencia o vazio mata e ele renasce)
}
// ---- mobs: o jogador 2 tambem e alvo. Se ele e o mais proximo, o mob roda no contexto dele (dano, vida, knockback caem no jogador 2) ----
static bool split2MobTick(MobEnt& e, Phys& P1) {
    int bk = -1; double d2 = 1e18; for (int k = 0; k < split2N(); k++) if (split2TargetableK(k)) { const double d = (gSX[k].P.x - e.x) * (gSX[k].P.x - e.x) + (gSX[k].P.z - e.z) * (gSX[k].P.z - e.z); if (d < d2) { d2 = d; bk = k; } }
    if (bk < 0) return false;
    const bool p1ok = !gDead && !creative; const double d1 = p1ok ? (P1.x - e.x) * (P1.x - e.x) + (P1.z - e.z) * (P1.z - e.z) : 1e18;
    if (d2 >= d1) return false;
    SxSel q(bk); S2Ctx c; tickMob(e, SX.P.x, SX.P.y, SX.P.z); hostileTick(e, SX.P); return true;
}
// ---- desenho do corpo de um jogador no mundo do outro ----
struct SplitPoseD { double x, y, z; float bodyYaw, headYaw, pitch, limb, amp, atk, age; int held; bool sneak; int boat; float pt; bool fire = false; };
static SplitPoseD split2PoseOf(const Phys& P, float yaw, float pitch, float pBody, float body, float limbSwing, float limbAmt, float pLimbAmt, bool swinging, float swing, int held, float age, float pt) {
    SplitPoseD q; q.x = P.px + (P.x - P.px) * pt; q.y = P.py + (P.y - P.py) * pt; q.z = P.pz + (P.z - P.pz) * pt;
    q.bodyYaw = pBody + (body - pBody) * pt; q.headYaw = yaw; q.pitch = pitch; q.amp = std::min(1.f, pLimbAmt + (limbAmt - pLimbAmt) * pt); q.limb = limbSwing - limbAmt * (1.f - pt);
    q.atk = swinging ? swing : 0.f; q.held = held; q.age = age + pt; q.sneak = P.sneak; q.boat = 0; q.pt = pt; return q;   // q.boat: o chamador define (so o extra 0 anda de bote)
}
static void split2DrawBody(const SplitPoseD& q, GLuint tex, bool shadowPass = false, GLuint capeTex = 0, float capeVy = 0.f) {   // shadowPass: mapa de sombras (F7): so a geometria, sem luz/shader de entidade
    Pose p; p.headYaw = wrap180(q.headYaw - q.bodyYaw); p.headPitch = q.pitch; p.limb = q.limb; p.amp = q.amp; p.atk = q.atk; p.held = q.held; p.age = q.age; p.sneak = q.sneak; p.capeTex = capeTex; p.capeVy = capeVy;   // patchSplit.6: capa dos extras
    const GLuint keep = skinTex; if (tex) skinTex = tex;
    const BoatEnt* bt = q.boat ? boatById(q.boat) : nullptr; if (bt) { p.sit = true; p.sneak = false; boatRowPose(p, *bt, q.pt); }
    rdPushMatrix(); if (bt) { const double sp[3] = {q.x, q.y, q.z}; boatRiderBegin(*bt, q.pt, sp); } else rdTranslatef((float)q.x, (float)q.y, (float)q.z); rdRotatef(q.bodyYaw, 0, 1, 0);
    if (shadowPass) drawPlayer(p); else { entLightAt(q.x, q.y + 1.0, q.z, gEntDim); entShaderOn(); drawPlayer(p); if (q.fire) drawFireWH(.6f, 1.8f, q.bodyYaw); skyShaderOff(); entDimReset(); } rdPopMatrix();   // patchFire.2: chamas nos extras do split
    skinTex = keep;
}
// ---- layout dos viewports (coordenadas do glViewport: origem embaixo a esquerda) ----
static void split2Rect(int vp, int W, int H, int& x, int& y, int& w, int& h) {   // patchSplit.1: 2 (cima/baixo ou lado a lado), 3 (1 grande + 2) e 4 (grade 2x2)
    const int N = gSplitN; const int hx = W / 2, hy = H - H / 2;   // metade esquerda/baixo; (H/2 = baixo, hy = cima)
    if (N >= 4) { const int c = vp & 1, r = vp >> 1; w = c ? W - hx : hx; x = c ? hx : 0; h = r ? H / 2 : hy; y = r ? 0 : H / 2; return; }   // r=0 topo
    if (N == 3) {
        if (gSplitMode == 1) { if (vp == 0) { x = 0; y = 0; w = hx; h = H; } else { x = hx; w = W - hx; if (vp == 1) { y = H / 2; h = hy; } else { y = 0; h = H / 2; } } }
        else { if (vp == 0) { x = 0; y = H / 2; w = W; h = hy; } else { y = 0; h = H / 2; if (vp == 1) { x = 0; w = hx; } else { x = hx; w = W - hx; } } }
        return;
    }
    if (gSplitMode == 1) { w = W / 2; h = H; x = vp == 0 ? 0 : W / 2; y = 0; if (vp == 1) w = W - W / 2; }
    else { w = W; h = H / 2; x = 0; y = vp == 0 ? H - H / 2 : 0; if (vp == 0) h = H - H / 2; }
}
static bool splitOn() { return gSplit; }
static bool split2On() { return gSplit; }
static std::string chatNick() { return gChatAs2 ? xNickK(gChatK) : netName(); }
static void chatSend(const std::string& m) { if (!gChatAs2 || netHost || netSock < 0) netSendChat(m); }   // J2 como convidado: o host so aceita o nick do proprio convidado (netChatOk), entao a fala dele fica so local
static bool split2IsNick(const std::string& n) {   // alvo do /give: "@p2" ou o nick do jogador 2 (so com split ligado)
    if (!gSplit || !SX.init) return false; std::string l = n; for (char& c : l) c = (char)tolower((unsigned char)c);
    return l == "@p2" || (!gP2Name.empty() && nickEq(n, gP2Name));
}
static int split2Give(int id, int n) { if (gSplitCtx == 2) return n; split2SwapInv(); const int left = addItem(id, n); split2SwapInv(); return left; }
static char gSplitLbl[2][40]; static int gSplitLblI = 0;
static const char* splitLabel() { char* o = gSplitLbl[gSplitLblI ^= 1]; snprintf(o, 40, "%s", gSplit ? ("Jogadores: " + std::to_string(gSplitN)).c_str() : "Tela dividida: não"); return o; }   // botao do menu de pausa (Esc)
static const char* splitModeLabel() { return gSplitMode == 1 ? "Lado a lado" : "Cima e baixo"; }   // layout do split (Esc)
static bool split2Act(int k) { return !(gInvOwner == 2 && gInvK == k) && !(gChatAs2 && k == gChatK); }   // patchSplit.4: so o extra dono da UI aberta (inventario/chat) para de jogar
static int split2Frame(float dt, bool active, const Phys& p1) {   // todos os extras; devolve (k+1) do que pediu o inventario, 0 = nenhum
    int want = 0; for (int k = 0; k < gSplitN - 1; k++) { SxSel q(k); if (split2Frame1(dt, active && split2Act(k), p1) && !want) want = k + 1; } return want;
}
static void split2Tick(const Phys& p1, bool active) { for (int k = 0; k < gSplitN - 1; k++) { SxSel q(k); split2Tick1(p1, active && split2Act(k)); } }
static void split2SwapInvOwner() { SxSel q(gInvK); split2SwapInv(); }   // devolve/troca o inventario do extra dono da tela aberta
