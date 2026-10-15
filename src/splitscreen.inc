// ================= splitscreen.inc: split screen local de 2 jogadores =================
// Jogador 1 = o jogador normal do jogo (teclado/mouse ou controle). Jogador 2 = estado proprio (S2) simulado aqui com o MESMO mundo:
// fisica real (moveEntityWithHeading: andar, pular, correr, nadar, escada, voar no criativo), minerar/colocar blocos, bater em mobs, usar alavanca/redstone,
// coletar drops e INVENTARIO COMPLETO (o inventario normal abre trocando o conteudo de inv/armor/craft pelo do jogador 2 enquanto a tela esta aberta).
// A renderizacao roda o bloco de desenho do main_loop 1x por viewport, trocando o contexto (P, yaw, pitch, hotbar...) para o jogador 2 na 2a passada.
// Controle: gpPadFor(2) em gamepad.inc (2+ controles: o 2o e do jogador 2; 1 controle: ele e do jogador 2 e o jogador 1 fica com teclado e mouse).
// Tem tambem (SurvCtx): vida/fome/morte e renascer, comida, arco, bote (gBoat2), bau, e os mobs hostis o miram (codigo; (VIS) conferir no jogo).
// Limites conhecidos: o P2 so tem controle (sem teclado/mouse);
// o jogador 2 fica preso a (RAD-2) chunks do jogador 1 (o mundo so carrega em volta do jogador 1) e volta para perto dele se passar disso ou cair no vazio.
struct SurvCtx {   // estado de sobrevivencia (survival.inc/bow.inc) de UM jogador; o do jogador 2 e trocado com os globais para reusar survTick/survDamage/eatTick/bowTick
    int hp = 20, food = 20, fire = 0, hurt = 0, invul = 0, regenT = 0, deadT = 0, eatCd = 0, lastDmg = 0, eatT = -1, hungerT = 0, regenFx = 0, poisonT = 0, bowT = -1, respHold = 0, air = 300;
    float sat = 5.f, exh = 0.f, fall = 0.f, hurtAtYaw = 0.f; bool dead = false, wantResp = false, eatOn = false, bowOn = false; Uint32 eatMs = 0; std::string why;
};
struct SplitP {
    SurvCtx sv; Phys* pvpP = nullptr; Slot cur;
    Phys P; float yaw = 0, pitch = 0, bodyYaw = 0, prevBodyYaw = 0, limbSwing = 0, limbAmt = 0, prevLimbAmt = 0, swing = 0, ageT = 0;
    bool init = false, swinging = false, sprintL = false, lastJump = false, atkHeld = false;
    int sel = 0, jumpTicks = 0, flyTimer = 0; float rep = 0, mnT = 0, mnNeed = 1, mnCool = 0; bool mnOn = false; int mnX = 0, mnY = 0, mnZ = 0;
    Slot inv[36], armor[4], craftG[9], craftOut, offh;
    GLuint skinTex = 0; GpIn in; int boatId = 0; float boatLast = 0, eggCd = 0;   // eggCd: segundos ate poder arremessar ovo de novo
};
static SplitP S2;
static int gSplitMode = 0;      // 0 = topo/baixo (como o Bedrock), 1 = lado a lado
static void split2SwapInv() {   // troca o inventario global com o do jogador 2 (usado ao abrir/fechar a tela de inventario do jogador 2 e para addItem)
    for (int i = 0; i < 36; i++) std::swap(inv[i], S2.inv[i]);
    for (int i = 0; i < 4; i++) std::swap(armor[i], S2.armor[i]);
    for (int i = 0; i < 9; i++) std::swap(craftG[i], S2.craftG[i]);
    std::swap(craftOut, S2.craftOut); std::swap(offh, S2.offh); std::swap(cur, S2.cur);
}
static void split2SwapSurv() {   // troca vida/fome/fogo/comer/arco... entre os globais (jogador 1) e S2.sv (jogador 2)
    SurvCtx& c = S2.sv;
    std::swap(gHP, c.hp); std::swap(gFood, c.food); std::swap(gFire, c.fire); std::swap(gHurt, c.hurt); std::swap(gInvul, c.invul); std::swap(gRegenT, c.regenT); std::swap(gDeadT, c.deadT);
    std::swap(gEatCd, c.eatCd); std::swap(gLastDmg, c.lastDmg); std::swap(gEatT, c.eatT); std::swap(gHungerT, c.hungerT); std::swap(gRegenFx, c.regenFx); std::swap(gPoisonT, c.poisonT); std::swap(gAir, c.air);
    std::swap(gBowT, c.bowT); std::swap(gRespHold, c.respHold); std::swap(gSat, c.sat); std::swap(gExh, c.exh); std::swap(gFall, c.fall); std::swap(gHurtAtYaw, c.hurtAtYaw);
    std::swap(gDead, c.dead); std::swap(gWantRespawn, c.wantResp); std::swap(gEatOn, c.eatOn); std::swap(gBowOn, c.bowOn); std::swap(gEatMs, c.eatMs); std::swap(gDeathWhy, c.why); std::swap(gPvpP, S2.pvpP);
}
struct S2Ctx {   // enquanto existir, os globais de inventario/sobrevivencia SAO do jogador 2 (RAII: sempre desfaz)
    S2Ctx() { split2SwapInv(); split2SwapSurv(); gSplitCtx = 2; }
    ~S2Ctx() { split2SwapSurv(); split2SwapInv(); gSplitCtx = 0; gUseOverride = -1; }
};
static bool split2Targetable() { return gSplit && S2.init && gSplitCtx != 2 && !S2.sv.dead && !creative && gGm != 3; }
static double split2Dist2(double x, double z) { return split2Targetable() ? (S2.P.x - x) * (S2.P.x - x) + (S2.P.z - z) * (S2.P.z - z) : 1e18; }   // distancia^2 (XZ) ao jogador 2 se ele e alvo valido (usado por mobsync.inc)
static const Phys* split2PhysP() { return &S2.P; }
static Phys* pistonP2() { return split2Targetable() ? &S2.P : nullptr; }   // pistao (redstone.inc) empurra o jogador 2
static void split2Damage(int dmg, const char* why, bool force, float kx, float kz) {
    if (!S2.init) return; bool hit; { S2Ctx c; hit = survDamage(dmg, why, force); }
    if (hit) { S2.P.mx += kx; S2.P.mz += kz; }
}
static void split2LoadSkin() {   // textura propria do jogador 2 (sem mexer no modelo/skin do jogador 1)
    if (S2.skinTex || gSkin2File.empty()) return;
    std::vector<uint8_t> bak(skin, skin + sizeof skin); const bool bs = slim, ba = slimAuto; const GLuint keep = skinTex;
    if (loadSkin(("assets/skins/" + gSkin2File).c_str())) { skinTex = 0; uploadSkin(); S2.skinTex = skinTex; }
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
    gBoat2 = 0; SplitP& s = S2; s = SplitP(); s.init = true;   // (a textura e recriada abaixo)
    const double ox = -cos(yaw * .0174533) * 1.2, oz = sin(yaw * .0174533) * 1.2;   // ao lado do jogador 1
    s.P.x = s.P.px = p1.x + (collEmpty(AABB{p1.x + ox - PW, p1.y, p1.z + oz - PW, p1.x + ox + PW, p1.y + PH, p1.z + oz + PW}) ? ox : 0);
    s.P.z = s.P.pz = p1.z + (collEmpty(AABB{p1.x + ox - PW, p1.y, p1.z + oz - PW, p1.x + ox + PW, p1.y + PH, p1.z + oz + PW}) ? oz : 0);
    s.P.y = s.P.py = p1.y; s.yaw = yaw; s.pvpP = &s.P; s.sv.lastDmg = 0;
    static const int start[9] = {GRASS, DIRT, STONE, SAND, LOG, PLANKS, COBBLE, IT_WSWORD, IT_APPLE};
    for (int i = 0; i < 9; i++) s.inv[i] = {start[i], maxStack(start[i]), 0};
    split2LoadSkin();
}
static void split2Fall(const Phys& p1) {   // volta para perto do jogador 1 (preso ao raio carregado ou caiu no vazio)
    S2.P.x = S2.P.px = p1.x + .8; S2.P.z = S2.P.pz = p1.z; S2.P.y = S2.P.py = p1.y + .2; S2.P.mx = S2.P.my = S2.P.mz = 0;
}
static void split2Enable(bool on, const Phys& p1, float yaw) {
    if (on == gSplit) return;
#ifdef __ANDROID__
    if (on) { chatAdd("Split screen indisponível nesta plataforma", 0xFF5555); return; }   // Android: sem split por enquanto (precisa de scissor+viewport e toque por metade da tela)
#endif
    gSplit = on;
#ifdef BL_RPI
    gRpiSplit = on; RAD = rdChunks();   // Raspberry: split = 4 chunks e 30 FPS
#endif
    if (!on) gBoat2 = 0;
    if (on) { if (S2.init) split2Fall(p1); else split2Spawn(p1, yaw); }   // religar: volta para perto do jogador 1 (mantem o inventario)
}
// ---- por quadro: olhar, botoes (bater/quebrar, usar/colocar/comer/arco, hotbar, largar, inventario/bau/mesa). Devolve true se abriu a tela de inventario ----
static void split2Respawn(const Phys& p1) {   // renasce perto do jogador 1 (inventario ja foi solto na morte) com vida/fome cheias
    gBoat2 = 0; S2.sv = SurvCtx(); S2.sv.invul = 20; S2.sv.lastDmg = 1000; S2.P.flying = false; S2.sprintL = false; split2Fall(p1); chatAdd(p2Nick() + " renasceu", 0xFFFFFF);
}
static bool split2Frame(float dt, bool active, const Phys& p1) {
    SplitP& s = S2; if (!s.init) return false;
    GpPad* pad = gpPadFor(2); const bool alive = !s.sv.dead; s.in = (active && alive) ? gpRead(pad) : GpIn(); Phys& P = s.P; bool wantInv = false;
    if (active && alive && gpPressed(pad, GA_CHAT)) gP2ChatReq = true;   // main_loop abre o chat como jogador 2
    s.ageT += dt * 20.f;
    if (s.swinging) { s.swing += dt * 5.f; if (s.swing >= 1.f) { s.swing = 0; s.swinging = false; } }   // mesmo ritmo do balanco do jogador 1 (~0,2 s)
    s.mnCool = std::max(0.f, s.mnCool - dt); s.eggCd = std::max(0.f, s.eggCd - dt);
    gBoat2P = &P; { const bool ok = active && pad && alive; gB2[0] = ok && s.in.str < -.5f; gB2[1] = ok && s.in.str > .5f; gB2[2] = ok && s.in.fwd > .5f; gB2[3] = ok && s.in.fwd < -.5f; gB2[4] = ok && s.in.sneak; }   // comandos do bote
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
    RayHit r; const bool has = rayCast(eye, dir, 5.f, r, false);
    // ---- atacar / quebrar (gatilho de ataque) ----
    if (gpPressed(pad, GA_PICK)) split2Pick(s, has, r);
    const bool atkPress = gpPressed(pad, GA_ATTACK), atk = s.in.attack;
    if (atkPress && boatHit(eye, dir, held.id, creative, creative ? 6.f : 3.f)) { s.swinging = true; s.swing = 0; s.mnOn = false; }   // bater no bote
    else if (atkPress && mobAttack(eye, dir, held.id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { s.swinging = true; s.swing = 0; s.mnOn = false; }
    else if (atk) {
        if (atkPress) { s.swinging = true; s.swing = 0; }
        const int b = has ? getB(r.x, r.y, r.z) : AIR;
        if (!has || b == AIR || b == BEDROCK || isWater(b) || gGm == 2) s.mnOn = false;
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
    const bool boated = usePress && gGm != 3 && !gBoat2 && boatInteract(eye, dir, creative ? 6.f : 3.f, gBoat2); if (boated) { s.swinging = true; s.swing = 0; }   // montar no bote
    const bool eggT = usePress && !boated && held.id == IT_EGG && gGm != 3 && s.eggCd <= 0.f && (!has || P.sneak || !eggBlockUsable(r.x, r.y, r.z));   // ovo (egg.inc): arremessa
    if (eggT) { int cd = 0; if (eggThrow(P, s.yaw, s.pitch, held, cd)) { s.eggCd = .5f; s.swinging = true; s.swing = 0; } }
    const int bty = (usePress && !boated && held.id) ? boatTypeOfItem(held.id) : -1;   // item de bote: coloca na agua
    if (bty >= 0 && gGm != 2) { if (boatPlace(bty, eye, dir, s.yaw, 5.f)) { s.swinging = true; s.swing = 0; if (!creative && --held.n <= 0) held = Slot(); } }
    else if (!boated && !eggT && has && gGm != 3 && !useItem && (usePress || (s.in.use && s.rep > .3f))) {
        if (!usePress) s.rep = .2f;
        const int hb = getB(r.x, r.y, r.z);
        if (hb == XB_CHEST && usePress && !(P.sneak && held.id)) { const int cu = chestOpenAt(r.x, r.y, r.z); if (cu) { if (cu == 1) wantInv = true; s.swinging = true; s.swing = 0; } }
        else if (hb == CTABLE && !P.sneak && usePress) { gTable = true; wantInv = true; s.swinging = true; s.swing = 0; }
        else if (xbIsLever(hb) && !P.sneak && usePress) { xbToggleLever(r.x, r.y, r.z); s.swinging = true; s.swing = 0; }
        else if (!P.sneak && usePress && rsUse(r.x, r.y, r.z)) { s.swinging = true; s.swing = 0; }
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
static void split2Tick(const Phys& p1, bool active) {
    SplitP& s = S2; if (!s.init) return; Phys& P = s.P;
    P.px = P.x; P.py = P.y; P.pz = P.z; P.prevDistWalked = P.distWalked; s.prevBodyYaw = s.bodyYaw; s.prevLimbAmt = s.limbAmt;
    if (!getC(fdiv((int)floor(P.x), CW), fdiv((int)floor(P.z), CW)) || !getC(fdiv((int)floor(P.x), CW), fdiv((int)floor(P.z), CW))->lc) { P.mx = P.my = P.mz = 0; return; }   // chunk do jogador 2 ainda carregando: fica parado (sem cair no vazio)
    if (s.jumpTicks > 0) --s.jumpTicks;
    P.inWater = inWaterNow(P); P.inLava = inLavaNow(P); if (P.inWater && !P.flying) waterPush(P);
    if (s.sv.dead) gBoat2 = 0;
    const BoatEnt* rb2 = boatById(gBoat2); const bool riding = rb2 != nullptr;   // sentado no bote (a posicao vem de boatTickAll)
    if (riding) { const float bo = boatYawO(*rb2); if (s.boatId != gBoat2) { s.boatId = gBoat2; s.boatLast = bo; } s.yaw = wrap180(s.yaw + wrap180(bo - s.boatLast)); s.boatLast = bo; } else s.boatId = 0;
    const bool alive = !s.sv.dead; const bool act = active && alive;
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
    if (riding) { P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false; s.sv.fall = 0.f; s.jumpTicks = 0; } else { unstuckPlayer(P); moveEntityWithHeading(P, s.yaw, str, fwd); }
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
    if (riding) { s.bodyYaw = boatYawO(*rb2); s.limbAmt = 0.f; while (s.bodyYaw - s.prevBodyYaw < -180.f) s.prevBodyYaw -= 360.f; while (s.bodyYaw - s.prevBodyYaw >= 180.f) s.prevBodyYaw += 360.f; }
    // coleta drops (o inventario global e trocado so durante o addItem)
    if (gGm != 3 && alive) for (size_t i = 0; i < DROPS.size();) {
        DropEnt& d = DROPS[i];
        if (d.delay <= 0 && d.x + .125 > P.x - 1.3 && d.x - .125 < P.x + 1.3 && d.z + .125 > P.z - 1.3 && d.z - .125 < P.z + 1.3 && d.y + .25 > P.y - .5 && d.y < P.y + 2.3) {
            const int got = d.n; split2SwapInv(); const int left = addItem(d.id, d.n, d.nbt); split2SwapInv();
            if (left < got) { netSendDropSet(d.nid, left, left <= 0 ? netSelf : 255); }
            d.n = left; if (left <= 0) { DROPS[i] = DROPS.back(); DROPS.pop_back(); continue; }
        }
        ++i;
    }
    if (P.y < -80 && creative) split2Fall(p1);   // caiu para fora do mundo no criativo: volta (em sobrevivencia o vazio mata e ele renasce)
}
// ---- mobs: o jogador 2 tambem e alvo. Se ele e o mais proximo, o mob roda no contexto dele (dano, vida, knockback caem no jogador 2) ----
static bool split2MobTick(MobEnt& e, Phys& P1) {
    if (!split2Targetable()) return false;
    const double d2 = (S2.P.x - e.x) * (S2.P.x - e.x) + (S2.P.z - e.z) * (S2.P.z - e.z);
    const bool p1ok = !gDead && !creative; const double d1 = p1ok ? (P1.x - e.x) * (P1.x - e.x) + (P1.z - e.z) * (P1.z - e.z) : 1e18;
    if (d2 >= d1) return false;
    S2Ctx c; tickMob(e, S2.P.x, S2.P.y, S2.P.z); hostileTick(e, S2.P); return true;
}
// ---- desenho do corpo de um jogador no mundo do outro ----
struct SplitPoseD { double x, y, z; float bodyYaw, headYaw, pitch, limb, amp, atk, age; int held; bool sneak; int boat; float pt; };
static SplitPoseD split2PoseOf(const Phys& P, float yaw, float pitch, float pBody, float body, float limbSwing, float limbAmt, float pLimbAmt, bool swinging, float swing, int held, float age, float pt) {
    SplitPoseD q; q.x = P.px + (P.x - P.px) * pt; q.y = P.py + (P.y - P.py) * pt; q.z = P.pz + (P.z - P.pz) * pt;
    q.bodyYaw = pBody + (body - pBody) * pt; q.headYaw = yaw; q.pitch = pitch; q.amp = std::min(1.f, pLimbAmt + (limbAmt - pLimbAmt) * pt); q.limb = limbSwing - limbAmt * (1.f - pt);
    q.atk = swinging ? swing : 0.f; q.held = held; q.age = age + pt; q.sneak = P.sneak; q.boat = gBoat2; q.pt = pt; return q;
}
static void split2DrawBody(const SplitPoseD& q, GLuint tex) {
    Pose p; p.headYaw = wrap180(q.headYaw - q.bodyYaw); p.headPitch = q.pitch; p.limb = q.limb; p.amp = q.amp; p.atk = q.atk; p.held = q.held; p.age = q.age; p.sneak = q.sneak;
    const GLuint keep = skinTex; if (tex) skinTex = tex;
    const BoatEnt* bt = q.boat ? boatById(q.boat) : nullptr; if (bt) { p.sit = true; p.sneak = false; boatRowPose(p, *bt, q.pt); }
    glPushMatrix(); if (bt) { const double sp[3] = {q.x, q.y, q.z}; boatRiderBegin(*bt, q.pt, sp); } else glTranslatef((float)q.x, (float)q.y, (float)q.z); glRotatef(q.bodyYaw, 0, 1, 0);
    entLightAt(q.x, q.y + 1.0, q.z, gEntDim); entShaderOn(); drawPlayer(p); skyShaderOff(); entDimReset(); glPopMatrix();
    skinTex = keep;
}
// ---- layout dos viewports (coordenadas do glViewport: origem embaixo a esquerda) ----
static void split2Rect(int vp, int W, int H, int& x, int& y, int& w, int& h) {
    if (gSplitMode == 1) { w = W / 2; h = H; x = vp == 0 ? 0 : W / 2; y = 0; if (vp == 1) w = W - W / 2; }
    else { w = W; h = H / 2; x = 0; y = vp == 0 ? H - H / 2 : 0; if (vp == 0) h = H - H / 2; }
}
static bool splitOn() { return gSplit; }
static bool split2On() { return gSplit; }
static std::string chatNick() { return gChatAs2 ? p2Nick() : netName(); }
static void chatSend(const std::string& m) { if (!gChatAs2 || netHost || netSock < 0) netSendChat(m); }   // J2 como convidado: o host so aceita o nick do proprio convidado (netChatOk), entao a fala dele fica so local
static bool split2IsNick(const std::string& n) {   // alvo do /give: "@p2" ou o nick do jogador 2 (so com split ligado)
    if (!gSplit || !S2.init) return false; std::string l = n; for (char& c : l) c = (char)tolower((unsigned char)c);
    return l == "@p2" || (!gP2Name.empty() && nickEq(n, gP2Name));
}
static int split2Give(int id, int n) { if (gSplitCtx == 2) return n; split2SwapInv(); const int left = addItem(id, n); split2SwapInv(); return left; }
static const char* splitLabel() { return gSplit ? "Split: ligado" : "Split: desligado"; }   // botao do menu de pausa (Esc)
static const char* splitModeLabel() { return gSplitMode == 1 ? "Lado a lado" : "Cima e baixo"; }   // layout do split (Esc)
