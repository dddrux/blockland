// patchOpt.2: corrente de agua empurra item solto (Entity.handleWaterMovement 0,014/tick) + frames da agua
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    setB(2, AY, 5, WATER); for (int k = 1; k <= 5; k++) setB(2 + k, AY, 5, 48 - k); gNoBlockHooks = false;
    CHECK(isWater(getB(4, AY, 5)));
    double fx, fy, fz; flowFull(4, AY, 5, fx, fy, fz); CHECK(fx > .5);
    DROPS.clear(); spawnDrop(4.5, AY + .3, 5.5, 0, 0, 0, STONE, 1, 10, 0, std::string());
    const double x0 = DROPS[0].x; for (int t = 0; t < 20; t++) tickDrops(100, 300, 100);
    CHECK(!DROPS.empty() && DROPS[0].x > x0 + .05);   // levado para +X pela corrente
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
