// test_cactusgrow: cacto cresce ate 3 blocos no random tick, so com topo livre (patchGrow.1). Rodar: tools/t.sh cactusgrow
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    const int AY = 200; gCactusId = PK0 + 7; const int CA = gCactusId;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Phys P; memset(&P, 0, sizeof P); P.x = 2.5; P.y = AY + 1; P.z = 2.5; gPvpP = &P;
    for (int z = -34; z < 40; z++) for (int x = -34; x < 40; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? SAND : AIR);
    setBM(2, AY, 2, CA, 0);
    for (int t = 0; t < 400000 && getB(2, AY + 2, 2) != CA; t++) cropRandomTick();
    CHECK(getB(2, AY + 1, 2) == CA); CHECK(getB(2, AY + 2, 2) == CA);   // cresceu ate 3 de altura
    for (int t = 0; t < 200000; t++) cropRandomTick();
    CHECK(getB(2, AY + 3, 2) == AIR);   // nunca passa de 3
    setB(2, AY + 1, 2, AIR); setB(2, AY + 2, 2, AIR); setBM(2, AY, 2, CA, 0); setB(2, AY + 1, 2, STONE);
    for (int t = 0; t < 100000; t++) cropRandomTick();
    CHECK(getB(2, AY + 1, 2) == STONE);   // topo bloqueado: nao cresce
    printf("test_cactusgrow: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
