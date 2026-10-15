// patch5.70: facilitador do pick block (pickItemAt). Roda o main.cpp REAL.
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
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    for (int i = 0; i < 32; i++) { gRsDrop[XB_DOOR - XB_DUST][i] = 7001; gRsDrop[XB_PIST - XB_DUST][i] = 7002; gRsDrop[XB_BED - XB_DUST][i] = 7003; }
    setBM(2, AY, 2, XB_DOOR, 0); setBM(2, AY + 1, 2, XB_DOOR, 8);
    CHECK(rsDropItem(2, AY + 1, 2, XB_DOOR) == 0);                     // dropar: metade de cima nao dropa
    CHECK(pickItemAt(2, AY, 2) == 7001); CHECK(pickItemAt(2, AY + 1, 2) == 7001);   // pick: as duas metades dao a porta
    setBM(4, AY, 2, XB_PIST, 3 | 8); CHECK(rsDropItem(4, AY, 2, XB_PIST) == 0); CHECK(pickItemAt(4, AY, 2) == 7002);   // cabeca do pistao
    setBM(6, AY, 2, XB_BED, 4); CHECK(pickItemAt(6, AY, 2) == 7003);   // cabeca da cama
    setB(8, AY, 2, STONE); CHECK(pickItemAt(8, AY, 2) == STONE);
    setB(10, AY, 2, WATER); CHECK(pickItemAt(10, AY, 2) == 0);
    setB(12, AY, 2, XB_FIRE); CHECK(pickItemAt(12, AY, 2) == 0);
    CHECK(pickItemAt(14, AY, 2) == 0);   // ar
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
