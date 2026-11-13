// patchPlate.1: atraso de soltar da placa de pressao (1.12.2): pedra/madeira 20, pesadas 10. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int want[4] = {20, 20, 10, 10};
    for (int k = 0; k < 4; k++) {
        gTicks.clear(); gTickSet.clear(); gNoBlockHooks = true; setBM(5, AY, 5, XB_PLATE, k); gNoBlockHooks = false;
        rsPlateCell(5, AY, 5, false);
        CHECK(gTicks.size() == 1 && gTicks[0].due == gWorldTick + want[k]);
        CHECK(((getM(5, AY, 5) >> 4) & 1) == 1);
    }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
