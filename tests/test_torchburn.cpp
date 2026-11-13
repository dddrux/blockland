// patchFix.17 (BUG-050): tocha de redstone queima apos 8 apagadas em 60 ticks e reacende depois de esfriar (1.12.2). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static bool lit() { return !(getM(5, AY, 5) & 1); }
static void step(bool power) { gNoBlockHooks = true; setM(4, AY - 1, 5, power ? (1 | 16) : 1); gNoBlockHooks = false; rsTorchCheck(5, AY, 5); for (int i = 0; i < 3; i++) runWorldTick(); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = 0; z < 12; z++) for (int x = 0; x < 12; x++) for (int y = AY - 2; y < AY + 4; y++) setB(x, y, z, y == AY - 2 ? STONE : AIR);
    setB(5, AY - 1, 5, STONE); setB(4, AY - 1, 5, XB_REPT); setM(4, AY - 1, 5, 1);   // repetidor desligado, leste: alimenta o bloco da tocha
    setB(5, AY, 5, XB_RTORCH); setM(5, AY, 5, 0); gNoBlockHooks = false;
    gTicks.clear(); gTickSet.clear();
    step(true); CHECK(!lit());      // energizou: apaga
    step(false); CHECK(lit());      // sem energia: acende (1a vez, sem queimar)
    for (int i = 0; i < 6; i++) { step(true); step(false); }   // 7 apagadas ao todo ate aqui
    CHECK(lit());                   // ainda nao queimou
    step(true); CHECK(!lit());      // 8a apagada
    step(false); CHECK(!lit());     // queimada: nao reacende mesmo sem energia
    for (int i = 0; i < 170; i++) runWorldTick();   // MC reagenda 160 ticks na queima
    CHECK(lit());                   // esfriou: reacende
    printf("%d checagens, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
