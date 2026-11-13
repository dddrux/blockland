// patchComp.1: comparador agenda com atraso 2 e prioridade -1 se a saida aponta para outro diodo (1.12.2). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void put(int x, int id, int m) { gNoBlockHooks = true; setBM(x, AY, 5, id, m); gNoBlockHooks = false; }
static void reset() { gTicks.clear(); gTickSet.clear(); gTickSeq = 0; for (int x = 2; x <= 8; x++) put(x, AIR, 0); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    // entrada: poeira com forca 15 atras (oeste) de um comparador que aponta para L (dir 1)
    reset(); put(4, XB_DUST, 15 << 4); put(5, XB_COMP, 1); rsCompCheck(5, AY, 5);
    CHECK(gTicks.size() == 1 && gTicks[0].due == gWorldTick + 2 && gTicks[0].prio == 0);
    // saida para repetidor virado de volta: prioridade -1
    reset(); put(4, XB_DUST, 15 << 4); put(5, XB_COMP, 1); put(6, XB_REPT, 3); rsCompCheck(5, AY, 5);
    CHECK(gTicks.size() == 1 && gTicks[0].due == gWorldTick + 2 && gTicks[0].prio == -1);
    // saida para diodo no mesmo sentido: normal
    reset(); put(4, XB_DUST, 15 << 4); put(5, XB_COMP, 1); put(6, XB_REPT, 1); rsCompCheck(5, AY, 5);
    CHECK(gTicks.size() == 1 && gTicks[0].prio == 0);
    // sem mudanca de saida: nada
    reset(); put(5, XB_COMP, 1); rsCompCheck(5, AY, 5); CHECK(gTicks.empty());
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
