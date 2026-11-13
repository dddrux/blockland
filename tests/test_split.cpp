// gamepad (mapeamento, zona morta, atribuicao de controles) + split screen (jogador 2: fisica, layout, troca de inventario, mineracao)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }   // patchFix.16: sem stub = chamada nula (randomSeed)
Uint64 SDL_GetPerformanceCounter(void) { return 123456789ULL; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 60;
static void flat() {
    for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) { if (!getC(cx, cz)) gen(cx, cz); Chunk* c = getC(cx, cz); if (!c->lc) { c->lc = new uint8_t[CW * CH * CW]; memset(c->lc, 0xF0, CW * CH * CW); } }
    for (int z = -40; z <= 40; z++) for (int x = -40; x <= 40; x++) for (int y = AY - 1; y < AY + 20; y++) setB(x, y, z, y < AY ? STONE : AIR);
}
int main() {
    chdir("/tmp"); system("rm -rf /tmp/saves /tmp/gamepad.cfg"); ensureWorld(); gCurDir = "mundo"; flat(); creative = true; gGm = 1;
    // ---- gamepad: atribuicao de controles ----
    CHECK(gpCount() == 0 && gpPadFor(1) == nullptr);
    gPads[0].c = (SDL_GameController*)1; gPads[0].id = 1;
    gSplit = false; CHECK(gpPadFor(1) == &gPads[0] && gpPadFor(2) == nullptr);
    gSplit = true; gSplitN = 2; gXPadId[0] = 1; CHECK(gpPadFor(1) == nullptr && gpPadFor(2) == &gPads[0]);   // patchSplit.5: o controle que apertou START e do J2
    gPads[2].c = (SDL_GameController*)2; gPads[2].id = 2;
    CHECK(gpPadFor(1) == &gPads[2] && gpPadFor(2) == &gPads[0]);                         // J1 fica com o controle que nao e de extra
    gPads[2] = GpPad();
    // ---- zona morta + analogico ----
    GpPad& p = gPads[0]; p.ax[1] = -.1f; CHECK(gpRead(&p).fwd == 0.f);                    // dentro da zona morta
    p.ax[1] = -1.f; p.ax[0] = 0; CHECK(gpRead(&p).fwd > .95f && fabsf(gpRead(&p).str) < .01f); // frente total
    p.ax[0] = 1.f; p.ax[1] = 0; CHECK(gpRead(&p).str > .95f);
    p.ax[0] = p.ax[1] = 0; p.ax[2] = .1f; CHECK(gpRead(&p).lx == 0.f); p.ax[2] = 1.f; CHECK(gpRead(&p).lx > .95f);
    // ---- mapeamento: padrao, rebind, bordas ----
    p.ax[2] = 0; CHECK(gGpMap[GA_JUMP] == GB_A && gGpMap[GA_ATTACK] == GB_RT);
    p.b[GB_A] = true; CHECK(gpRead(&p).jump && gpHeld(&p, GA_JUMP) && gpPressed(&p, GA_JUMP)); p.pb[GB_A] = true; CHECK(!gpPressed(&p, GA_JUMP));
    p.b[GB_A] = p.pb[GB_A] = false; gGpBind = GA_JUMP; CHECK(!gpHeld(&p, GA_JUMP));       // capturando: nada dispara
    p.b[GB_X] = true; p.pb[GB_X] = false;   // gpPoll leria do SDL: simula a captura direto
    gGpMap[GA_JUMP] = GB_X; gGpBind = -1; gpSave(); gGpMap[GA_JUMP] = GB_A; gpLoad(); CHECK(gGpMap[GA_JUMP] == GB_X);   // salvo em gamepad.cfg
    gpResetMap(); CHECK(gGpMap[GA_JUMP] == GB_A);
    // ---- layout dos viewports ----
    int x, y, w, h; split2Rect(0, 1000, 600, x, y, w, h); CHECK(x == 0 && y == 300 && w == 1000 && h == 300);   // topo (origem GL embaixo)
    split2Rect(1, 1000, 600, x, y, w, h); CHECK(x == 0 && y == 0 && w == 1000 && h == 300);
    gSplitMode = 1; split2Rect(0, 1001, 600, x, y, w, h); CHECK(x == 0 && w == 500 && h == 600); split2Rect(1, 1001, 600, x, y, w, h); CHECK(x == 500 && w == 501); gSplitMode = 0;
    // ---- jogador 2: nasce ao lado, anda, pula, e nao mexe no jogador 1 ----
    Phys P1; P1.x = P1.px = 0.5; P1.z = P1.pz = 0.5; P1.y = P1.py = AY;
    split2Spawn(P1, 0.f); CHECK(gSX[0].init && gSX[0].inv[0].id == AIR && fabs(gSX[0].P.x - P1.x) > .5 && fabs(gSX[0].P.x - P1.x) < 2);
    for (int i = 0; i < 40; i++) split2Tick(P1, true); CHECK(gSX[0].P.onGround && fabs(gSX[0].P.y - AY) < .01);   // assenta no chao
    const double z0 = gSX[0].P.z; gSX[0].in.fwd = 1.f; for (int i = 0; i < 20; i++) split2Tick(P1, true); CHECK(gSX[0].P.z < z0 - 2.0);   // yaw 0 = -Z
    gSX[0].in.fwd = 0; gSX[0].in.jump = true; split2Tick(P1, true); CHECK(gSX[0].P.my > .3 || !gSX[0].P.onGround); gSX[0].in.jump = false;
    for (int i = 0; i < 40; i++) split2Tick(P1, true);
    gSX[0].P.x = 5000; gSX[0].P.px = 5000; const double fy = gSX[0].P.y; split2Tick(P1, true); CHECK(gSX[0].P.y == fy && gSX[0].P.x == 5000);   // chunk nao carregado: fica parado (sem cair no vazio); nao ha mais tether
    gSX[0].P.x = gSX[0].P.px = P1.x + 1;
    // ---- inventario: troca ida e volta ----
    inv[0] = {STONE, 5}; gSX[0].inv[0] = {DIRT, 7}; split2SwapInv(); CHECK(inv[0].id == DIRT && gSX[0].inv[0].id == STONE); split2SwapInv(); CHECK(inv[0].id == STONE && gSX[0].inv[0].id == DIRT);
    // ---- coleta de drops pelo jogador 2 ----
    DROPS.clear(); gSX[0].P.x = gSX[0].P.px = 3.5; gSX[0].P.z = gSX[0].P.pz = 3.5; gSX[0].P.y = gSX[0].P.py = AY; for (int i = 0; i < 3; i++) gSX[0].inv[i] = Slot(); gSX[0].inv[0] = {DIRT, 7};
    spawnDrop(3.5, AY + .5, 3.5, 0, 0, 0, DIRT, 2, 0); for (auto& d : DROPS) d.delay = 0; split2Tick(P1, true);
    CHECK(DROPS.empty() && gSX[0].inv[0].n == 9);
    // ---- minerar/colocar pelo jogador 2 (criativo): pad simulado ----
    gSX[0].P.x = gSX[0].P.px = 3.5; gSX[0].P.z = gSX[0].P.pz = 3.5; gSX[0].P.y = gSX[0].P.py = AY; gSX[0].yaw = 0; gSX[0].pitch = -89.f; gSX[0].sel = 0; gSX[0].inv[0] = {STONE, 64};
    setB(3, AY - 1, 3, STONE); p.b[GB_RT] = true; p.pb[GB_RT] = false;   // RT = atacar
    split2Frame(.05f, true, P1); CHECK(getB(3, AY - 1, 3) == AIR);                          // quebrou o bloco sob os pes (olhando para baixo)
    p.b[GB_RT] = p.pb[GB_RT] = false; p.b[GB_LT] = true; p.pb[GB_LT] = false;               // LT = usar/colocar
    gSX[0].P.y = gSX[0].P.py = AY + 3; setB(3, AY - 1, 3, STONE); setB(3, AY, 3, AIR);
    split2Frame(.05f, true, P1); CHECK(getB(3, AY, 3) == STONE);
    // ---- sobrevivencia do jogador 2: dano proprio, sem afetar o jogador 1, morte e renascer ----
    creative = false; gGm = 0; gSX[0].sv = SurvCtx(); gSX[0].sv.hp = 20; gSX[0].sv.invul = 0; gHP = 20; gSplit = true;
    CHECK(split2Targetable());
    split2Damage(6, "teste", true, 0, 0); CHECK(gSX[0].sv.hp == 14 && gHP == 20);
    gSX[0].sv.invul = 0; split2Damage(100, "teste", true, 0, 0); CHECK(gSX[0].sv.hp <= 0 || gSX[0].sv.dead); CHECK(!split2Targetable());
    gSX[0].sv.deadT = 100; split2Tick(P1, true); CHECK(!gSX[0].sv.dead && gSX[0].sv.hp == 20);
    creative = true; gGm = 1; CHECK(!split2Targetable());
    // ---- bote do jogador 2: monta, o bote o leva, agachar desmonta ----
    creative = true; gGm = 1; for (int z = -6; z <= 6; z++) for (int x = -6; x <= 6; x++) for (int y = AY - 3; y < AY; y++) setB(x, y, z, WATER);
    BOATS.clear(); gBoatRide = 0; gBoat2 = 0; gSX[0].P.x = gSX[0].P.px = 2; gSX[0].P.z = gSX[0].P.pz = 2; gSX[0].P.y = gSX[0].P.py = AY - 1; gSX[0].sv = SurvCtx();
    boatSpawn(0, 2, AY - 1.4, 2, 0.f); const float ey[3] = {2.f, (float)AY + .5f, 3.f}, dr[3] = {0, -.848f, -.53f}; gBoatXP[0] = &gSX[0].P;   // patchFix.16: bote refatorado (gBoatXP/gBX por jogador extra)
    CHECK(boatInteract(ey, dr, 6.f, gBoat2) && gBoat2 == BOATS[0].id);
    gBX[0][2] = true; for (int i = 0; i < 30; i++) { split2Tick(P1, true); Phys dummy; boatTickAll(dummy, false, false, false, false, false); }
    CHECK(fabs(gSX[0].P.x - 2) + fabs(gSX[0].P.z - 2) > .3 && fabs(gSX[0].P.x - BOATS[0].x) < .5);   // o bote andou e o jogador 2 foi junto
    gBX[0][2] = false; gBX[0][4] = true; { Phys dummy; boatTickAll(dummy, false, false, false, false, false); } CHECK(gBoat2 == 0);   // agachou: desceu
    gBX[0][4] = false; BOATS.clear();
    // ---- patchSplit.1: 3 e 4 jogadores ----
    gSplitSkinOff = true;
    { gSplit = true; gSplitN = 3; int x, y, w, h; gSplitMode = 0;
      split2Rect(0, 1000, 600, x, y, w, h); CHECK(x == 0 && y == 300 && w == 1000 && h == 300);   // J1 em cima, largura total
      split2Rect(1, 1000, 600, x, y, w, h); CHECK(x == 0 && y == 0 && w == 500 && h == 300);
      split2Rect(2, 1000, 600, x, y, w, h); CHECK(x == 500 && y == 0 && w == 500 && h == 300);
      gSplitMode = 1; split2Rect(0, 1000, 600, x, y, w, h); CHECK(x == 0 && w == 500 && h == 600);
      split2Rect(1, 1000, 600, x, y, w, h); CHECK(x == 500 && y == 300 && h == 300); split2Rect(2, 1000, 600, x, y, w, h); CHECK(x == 500 && y == 0 && h == 300);
      gSplitN = 4; gSplitMode = 0; long area = 0; for (int v = 0; v < 4; v++) { split2Rect(v, 1001, 601, x, y, w, h); area += (long)w * h; CHECK(x >= 0 && y >= 0 && x + w <= 1001 && y + h <= 601); }
      CHECK(area == 1001L * 601);                                                                  // grade 2x2 cobre a tela inteira, sem sobrepor
      split2Rect(0, 1000, 600, x, y, w, h); CHECK(x == 0 && y == 300); split2Rect(3, 1000, 600, x, y, w, h); CHECK(x == 500 && y == 0);
      // controles: 4 jogadores
      gPads[1].c = (SDL_GameController*)3; gPads[1].id = 3; gPads[2].c = (SDL_GameController*)2; gPads[2].id = 2;   // 3 controles (0,1,2)
      gXPadId[0] = 1; gXPadId[1] = 3; gXPadId[2] = 2;
      CHECK(gpPadFor(1) == nullptr && gpPadFor(2) == &gPads[0] && gpPadFor(3) == &gPads[1] && gpPadFor(4) == &gPads[2]);   // J1 teclado; J2..J4 controles
      gPads[3].c = (SDL_GameController*)4; gPads[3].id = 4; CHECK(gpPadFor(1) == &gPads[3] && gpPadFor(4) == &gPads[2]);   // 4o controle (sem START) fica com o J1
      gPads[1] = gPads[2] = gPads[3] = GpPad(); gSplitN = 2; gSplit = true; gXPadId[0] = gXPadId[1] = gXPadId[2] = -1;
    }
    {   // patchSplit.5: START abre/fecha a janela de cada controle; 2x START no mesmo controle nao abre duas
      gSplit = false; gSplitN = 2; gXPadId[0] = gXPadId[1] = gXPadId[2] = -1; for (int k = 0; k < 3; k++) gSX[k] = SplitP(); Phys P1; P1.x = 3.5; P1.z = 3.5; P1.y = AY;
      split2PadStart(11, P1, 0.f); CHECK(gSplit && gSplitN == 2 && gXPadId[0] == 11 && gSX[0].init);          // 1o START: abre (2 jogadores)
      split2PadStart(11, P1, 0.f); CHECK(!gSplit && gXPadId[0] == -1);                                       // mesmo controle de novo: fecha, nao divide de novo
      split2PadStart(11, P1, 0.f); split2PadStart(22, P1, 0.f); CHECK(gSplit && gSplitN == 3 && gXPadId[0] == 11 && gXPadId[1] == 22);   // outro controle: nova janela
      gSX[0].inv[0] = {DIRT, 3}; gSX[1].inv[0] = {STONE, 5};
      split2PadStart(11, P1, 0.f); CHECK(gSplit && gSplitN == 2 && gXPadId[0] == 22 && gXPadId[1] == -1 && gSX[0].inv[0].id == STONE && gSX[0].pvpP == &gSX[0].P);   // saiu o 1o: o 2o sobe de lugar com o inventario
      split2PadStart(33, P1, 0.f); split2PadStart(44, P1, 0.f); CHECK(gSplitN == 4); split2PadStart(55, P1, 0.f); CHECK(gSplitN == 4 && gXPadId[0] != 55 && gXPadId[1] != 55 && gXPadId[2] != 55);   // limite de 4
      split2PadStart(22, P1, 0.f); split2PadStart(33, P1, 0.f); split2PadStart(44, P1, 0.f); CHECK(!gSplit && gXPadId[0] == -1 && gXPadId[1] == -1 && gXPadId[2] == -1);
      CHECK(GP_DEF[GA_PAUSE] == GB_BACK && GP_DEF[GA_CHAT] == GB_LB);                                       // START reservado ao split
    }
    { gSplitN = 3; gSplit = true; creative = true; gGm = 1; Phys P1; P1.x = 3.5; P1.z = 3.5; P1.y = AY;
      for (int k = 0; k < 2; k++) { SxSel q(k); split2Spawn(P1, 0.f); }
      CHECK(gSX[0].init && gSX[1].init && gSX[1].skinTex == gSX[1].skinTex && fabs(gSX[0].P.x - gSX[1].P.x) + fabs(gSX[0].P.z - gSX[1].P.z) > 1.0);   // nascem em lugares diferentes
      CHECK(gSX[1].inv[0].id == AIR && gS2i == 0);
      gSX[0].inv[0] = {DIRT, 3}; gSX[1].inv[0] = {STONE, 5}; { SxSel q(1); split2SwapInv(); CHECK(inv[0].id == STONE); split2SwapInv(); } CHECK(gSX[1].inv[0].id == STONE && gSX[0].inv[0].id == DIRT && gS2i == 0);   // inventarios separados
      gSX[0].sv = SurvCtx(); gSX[1].sv = SurvCtx(); creative = false; gGm = 0; gSX[0].sv.invul = gSX[1].sv.invul = 0;
      split2DamageK(1, 6, "t", true, 0, 0); CHECK(gSX[1].sv.hp == 14 && gSX[0].sv.hp == 20 && gHP == 20);   // dano so no J3
      CHECK(split2TargetableK(0) && split2TargetableK(1) && !split2TargetableK(2) && split2N() == 2);
      gSX[0].P.x = 10; gSX[0].P.z = 0; gSX[1].P.x = 2; gSX[1].P.z = 0; CHECK(split2Dist2(0, 0) == 4.0);   // mais proximo = J3
      CHECK(pistonAll().size() == 3);
      creative = true; gGm = 1; gSplitN = 2; }
    {   // patchSplit.3: jogador do split visivel na rede (NP_XSTATE -> remotesX)
      Slot arm[4] = {{IT_COAL, 1}, {}, {}, {}};
      NBuf pk = netXPacket(1, 12.5, 64.0, -3.25, 90.f, 10.f, 0.f, STONE, false, 1, -1, false, true, 45.f, arm);
      const int ty = pk.get<uint8_t>(), snd = pk.get<uint8_t>(), xk = pk.get<uint8_t>() & 3, on = pk.get<uint8_t>();
      CHECK(ty == NP_XSTATE && xk == 1 && on == 1 && snd == netSelf);
      remotesX.clear(); const int key = netXKey(3, xk); netStateParse(remotesX[key], pk, 100); netTick();
      const RemotePl& r = remotesX[key];
      CHECK(r.init && fabs(r.x - 12.5) < 1e-9 && fabs(r.z + 3.25) < 1e-9 && r.held == STONE && r.sit && r.worn[0] == arm[0].id && pk.r == pk.n);
      CHECK(netLabel(key).find("(J3)") != std::string::npos && netBaseId(key) == 3 && remotes.count(key) == 0);   // extra nao vira jogador "real" (PvP/mobs)
    }
    // ---- patchSplit.4: PvP entre jogadores do split (J1 -> J2, J2 -> J1, J2 -> J3) ----
    {
        creative = false; gGm = 0; gSplit = true; gSplitN = 3; gPvpP = &P1; gDead = false; netOn = false;
        P1.x = P1.px = 10.5; P1.z = P1.pz = 10.5; P1.y = P1.py = AY; P1.sneak = false; gHP = 20; gInvul = 0;
        for (int k = 0; k < 2; k++) { gSX[k].init = true; gSX[k].sv = SurvCtx(); gSX[k].sv.hp = 20; gSX[k].sv.invul = 0; gSX[k].pvpP = &gSX[k].P; gSX[k].P.y = gSX[k].P.py = AY; gSX[k].P.sneak = false; }
        gSX[0].P.x = gSX[0].P.px = 10.5; gSX[0].P.z = gSX[0].P.pz = 12.0;   // J2 2 blocos a frente (+z) do J1
        gSX[1].P.x = gSX[1].P.px = 30.5; gSX[1].P.z = gSX[1].P.pz = 30.5;   // J3 longe
        const float e1[3] = {10.5f, (float)AY + 1.62f, 10.5f}, d1[3] = {0.f, 0.f, 1.f};
        CHECK(pvpAttack(e1, d1, 0, false, false, 3.f));   // J1 acerta J2
        CHECK(gSX[0].sv.hp < 20 && gHP == 20 && gSX[1].sv.hp == 20);
        const int hpJ1 = gHP; const float e2[3] = {10.5f, (float)AY + 1.62f, 12.0f}, d2[3] = {0.f, 0.f, -1.f};
        { SxSel q(0); CHECK(pvpAttackK(0, e2, d2, 0, false, false, 3.f)); }   // J2 acerta J1
        CHECK(gHP < hpJ1 && gSX[1].sv.hp == 20);
        gSX[1].P.x = gSX[1].P.px = 10.5; gSX[1].P.z = gSX[1].P.pz = 14.0; gSX[0].P.z = gSX[0].P.pz = 30.5;   // J3 a frente de J2 (J2 em 30.5 -> mira para -z... usa outro ponto)
        const float e3[3] = {10.5f, (float)AY + 1.62f, 12.0f}, d3[3] = {0.f, 0.f, 1.f}; const int hp3 = gSX[1].sv.hp; gSX[1].sv.invul = 0;
        CHECK(pvpAttack(e3, d3, 0, false, false, 3.f) && gSX[1].sv.hp < hp3);   // golpe em J3
        CHECK(!pvpAttack(e1, d3, 0, false, false, 1.0f));   // fora de alcance
        creative = true; gGm = 1; gSX[1].sv.invul = 0; CHECK(!pvpAttack(e3, d3, 0, false, false, 3.f));   // criativo: sem dano
        creative = false; gGm = 0; gSplit = false; gSplitN = 2;
    }
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
