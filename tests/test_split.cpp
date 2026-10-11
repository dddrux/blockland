// gamepad (mapeamento, zona morta, atribuicao de controles) + split screen (jogador 2: fisica, layout, troca de inventario, mineracao)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
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
    gSplit = true;  CHECK(gpPadFor(1) == nullptr && gpPadFor(2) == &gPads[0]);          // 1 controle no split: do jogador 2
    gPads[2].c = (SDL_GameController*)2; gPads[2].id = 2;
    CHECK(gpPadFor(1) == &gPads[0] && gpPadFor(2) == &gPads[2]);                         // 2 controles: 1o = J1, 2o = J2
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
    split2Spawn(P1, 0.f); CHECK(S2.init && S2.inv[0].id == GRASS && fabs(S2.P.x - P1.x) > .5 && fabs(S2.P.x - P1.x) < 2);
    for (int i = 0; i < 40; i++) split2Tick(P1, true); CHECK(S2.P.onGround && fabs(S2.P.y - AY) < .01);   // assenta no chao
    const double z0 = S2.P.z; S2.in.fwd = 1.f; for (int i = 0; i < 20; i++) split2Tick(P1, true); CHECK(S2.P.z < z0 - 2.0);   // yaw 0 = -Z
    S2.in.fwd = 0; S2.in.jump = true; split2Tick(P1, true); CHECK(S2.P.my > .3 || !S2.P.onGround); S2.in.jump = false;
    for (int i = 0; i < 40; i++) split2Tick(P1, true);
    S2.P.x = 5000; S2.P.px = 5000; const double fy = S2.P.y; split2Tick(P1, true); CHECK(S2.P.y == fy && S2.P.x == 5000);   // chunk nao carregado: fica parado (sem cair no vazio); nao ha mais tether
    S2.P.x = S2.P.px = P1.x + 1;
    // ---- inventario: troca ida e volta ----
    inv[0] = {STONE, 5}; S2.inv[0] = {DIRT, 7}; split2SwapInv(); CHECK(inv[0].id == DIRT && S2.inv[0].id == STONE); split2SwapInv(); CHECK(inv[0].id == STONE && S2.inv[0].id == DIRT);
    // ---- coleta de drops pelo jogador 2 ----
    DROPS.clear(); S2.P.x = S2.P.px = 3.5; S2.P.z = S2.P.pz = 3.5; S2.P.y = S2.P.py = AY; for (int i = 0; i < 3; i++) S2.inv[i] = Slot(); S2.inv[0] = {DIRT, 7};
    spawnDrop(3.5, AY + .5, 3.5, 0, 0, 0, DIRT, 2, 0); for (auto& d : DROPS) d.delay = 0; split2Tick(P1, true);
    CHECK(DROPS.empty() && S2.inv[0].n == 9);
    // ---- minerar/colocar pelo jogador 2 (criativo): pad simulado ----
    S2.P.x = S2.P.px = 3.5; S2.P.z = S2.P.pz = 3.5; S2.P.y = S2.P.py = AY; S2.yaw = 0; S2.pitch = -89.f; S2.sel = 0; S2.inv[0] = {STONE, 64};
    setB(3, AY - 1, 3, STONE); p.b[GB_RT] = true; p.pb[GB_RT] = false;   // RT = atacar
    split2Frame(.05f, true, P1); CHECK(getB(3, AY - 1, 3) == AIR);                          // quebrou o bloco sob os pes (olhando para baixo)
    p.b[GB_RT] = p.pb[GB_RT] = false; p.b[GB_LT] = true; p.pb[GB_LT] = false;               // LT = usar/colocar
    S2.P.y = S2.P.py = AY + 3; setB(3, AY - 1, 3, STONE); setB(3, AY, 3, AIR);
    split2Frame(.05f, true, P1); CHECK(getB(3, AY, 3) == STONE);
    // ---- sobrevivencia do jogador 2: dano proprio, sem afetar o jogador 1, morte e renascer ----
    creative = false; gGm = 0; S2.sv = SurvCtx(); S2.sv.hp = 20; S2.sv.invul = 0; gHP = 20; gSplit = true;
    CHECK(split2Targetable());
    split2Damage(6, "teste", true, 0, 0); CHECK(S2.sv.hp == 14 && gHP == 20);
    S2.sv.invul = 0; split2Damage(100, "teste", true, 0, 0); CHECK(S2.sv.hp <= 0 || S2.sv.dead); CHECK(!split2Targetable());
    S2.sv.deadT = 100; split2Tick(P1, true); CHECK(!S2.sv.dead && S2.sv.hp == 20);
    creative = true; gGm = 1; CHECK(!split2Targetable());
    // ---- bote do jogador 2: monta, o bote o leva, agachar desmonta ----
    creative = true; gGm = 1; for (int z = -6; z <= 6; z++) for (int x = -6; x <= 6; x++) for (int y = AY - 3; y < AY; y++) setB(x, y, z, WATER);
    BOATS.clear(); gBoatRide = 0; gBoat2 = 0; S2.P.x = S2.P.px = 2; S2.P.z = S2.P.pz = 2; S2.P.y = S2.P.py = AY - 1; S2.sv = SurvCtx();
    boatSpawn(0, 2, AY - 1.4, 2, 0.f); const float ey[3] = {2.f, (float)AY + .5f, 3.f}, dr[3] = {0, -.848f, -.53f}; gBoat2P = &S2.P;
    CHECK(boatInteract(ey, dr, 6.f, gBoat2) && gBoat2 == BOATS[0].id);
    gB2[2] = true; for (int i = 0; i < 30; i++) { split2Tick(P1, true); Phys dummy; boatTickAll(dummy, false, false, false, false, false); }
    CHECK(fabs(S2.P.x - 2) + fabs(S2.P.z - 2) > .3 && fabs(S2.P.x - BOATS[0].x) < .5);   // o bote andou e o jogador 2 foi junto
    gB2[2] = false; gB2[4] = true; { Phys dummy; boatTickAll(dummy, false, false, false, false, false); } CHECK(gBoat2 == 0);   // agachou: desceu
    gB2[4] = false; BOATS.clear();
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
