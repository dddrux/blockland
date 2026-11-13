// patchSapling.1 + patchFire.2: muda vira arvore; fogo envelhece e some; tesoura empilha 1. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 40; y++) setB(x, y, z, y == AY - 1 ? DIRT : AIR);
}
static int countBlocks(bool logs) { int n = 0; for (int y = AY; y < AY + 40; y++) for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { const int b = getB(x, y, z); if (logs ? isLogB(b) : isLeafB(b)) n++; } return n; }
int main() {
    arena(); applyMcPack(); loadPackItems(); xbInit();
    for (int sp = 0; sp < 5; sp++) {
        arena(); setB(4, AY, 4, XB_SAPLING + sp); saplingGrow(4, AY, 4);
        printf("especie %d: troncos %d folhas %d\n", sp, countBlocks(true), countBlocks(false));
        CHECK(!xbIsSapling(getB(4, AY, 4))); CHECK(countBlocks(true) >= 3); CHECK(countBlocks(false) >= 5);
    }
    arena(); for (int a = 0; a < 2; a++) for (int c = 0; c < 2; c++) setB(4 + a, AY, 4 + c, XB_SAPLING + 5);
    saplingGrow(4, AY, 4); CHECK(countBlocks(true) >= 4); CHECK(!xbIsSapling(getB(5, AY, 5)));
    arena(); setB(4, AY, 4, XB_SAPLING); setB(4, AY + 2, 4, STONE); saplingGrow(4, AY, 4); CHECK(xbIsSapling(getB(4, AY, 4)));   // sem espaco: nao cresce
    // fogo: sem inflamavel ao lado e sobre pedra some; sobre madeira envelhece mas acaba
    int gone = 0; for (int i = 0; i < 40; i++) { arena(); setB(2, AY - 1, 2, STONE); setB(2, AY, 2, XB_FIRE); for (int k = 0; k < 200 && getB(2, AY, 2) == XB_FIRE; k++) fireTick(2, AY, 2); if (getB(2, AY, 2) != XB_FIRE) gone++; }
    CHECK(gone == 40);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
