// patchSplit.5: START no controle abre/fecha a janela do split (1 controle = 1 jogador, J1 = controle sem extra)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint64 SDL_GetPerformanceCounter(void) { return 123456789ULL; }
Uint32 SDL_GetTicks() { return 5000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 60;
static void flat() {
    for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) { if (!getC(cx, cz)) gen(cx, cz); Chunk* c = getC(cx, cz); if (!c->lc) { c->lc = new uint8_t[CW * CH * CW]; memset(c->lc, 0xF0, CW * CH * CW); } }
    for (int z = -40; z <= 40; z++) for (int x = -40; x <= 40; x++) for (int y = AY - 1; y < AY + 20; y++) setB(x, y, z, y < AY ? STONE : AIR);
}
int main() {
    chdir("/tmp"); system("rm -rf /tmp/saves /tmp/gamepad.cfg"); ensureWorld(); gCurDir = "mundo"; flat(); creative = true; gGm = 1;
    gSplitSkinOff = true; CHECK(gpCount() == 0 && gpPadFor(1) == nullptr);
    gPads[0].c = (SDL_GameController*)1; gPads[0].id = 1;
    gSplit = false; CHECK(gpPadFor(1) == &gPads[0] && gpPadFor(2) == nullptr);
    gSplit = true; gSplitN = 2; gXPadId[0] = 1; CHECK(gpPadFor(1) == nullptr && gpPadFor(2) == &gPads[0]);   // o controle que apertou START e do J2
    gPads[2].c = (SDL_GameController*)2; gPads[2].id = 2; CHECK(gpPadFor(1) == &gPads[2] && gpPadFor(2) == &gPads[0]);   // J1 fica com o controle que nao e de extra
    gPads[0] = gPads[2] = GpPad(); gSplit = false; gXPadId[0] = -1;
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
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
