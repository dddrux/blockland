// BUG-008: concreto em po cai como areia e vira concreto ao tocar agua (lado/cima ou pouso na agua). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 150;
static void ticks(int n) { for (int i = 0; i < n; i++) runWorldTick(); }
int main() {
    gCurDir = "tn_test"; WORLD_SEED = 777; netherInit();
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 4, z, STONE); for (int y = AY - 3; y < AY + 12; y++) setB(x, y, z, AIR); setB(x, AY - 1, z, STONE); }
    const int POW = PK0, CON = PK0 + 1;
    gPkByFile["block/white_concrete_powder"] = POW; gPkByFile["block/white_concrete"] = CON; fallHooksInit();
    CHECK(gPowderConc.count(POW) && gPowderConc[POW] == CON);
    setBM(2, AY + 5, 2, POW, 0); ticks(80); CHECK(getB(2, AY + 5, 2) == AIR && getB(2, AY, 2) == POW);       // cai como areia
    setBM(5, AY, 5, WATER, 0); setBM(6, AY, 5, POW, 0); ticks(10); CHECK(getB(6, AY, 5) == CON);              // agua do lado
    setBM(8, AY, 8, POW, 0); ticks(10); CHECK(getB(8, AY, 8) == POW);                                         // sem agua: continua po
    setBM(8, AY + 1, 8, WATER, 0); ticks(10); CHECK(getB(8, AY, 8) == CON);                                   // agua em cima
    setBM(10, AY, 10, WATER, 0); setBM(10, AY + 4, 10, POW, 0); ticks(80); CHECK(getB(10, AY, 10) == CON || getB(10, AY + 1, 10) == CON);   // cai na agua
    setBM(12, AY - 1, 12, WATER, 0); setBM(13, AY, 12, POW, 0); ticks(10); CHECK(getB(13, AY, 12) == POW);    // agua so embaixo: nao vira
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
