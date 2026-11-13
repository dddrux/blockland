// patchClock.1/2: formula do quadro do relogio e receitas de relogio/bussola.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    CHECK(clockFrame(6000, 0, 0) == 0 && clockFrame(18000, 0, 0) == 32 && clockFrame(12000, 0, 0) == 13 && clockFrame(0, 0, 0) == 50);
    CHECK(clockFrame(6000 + 24000 * 5, 0, 0) == 0 && clockFrame(-6000 + 24000, 0, 0) == 32);   // ciclo de 24000 e tempo negativo
    int last = -1, back = 0; for (int t = 6000; t < 30000; t += 50) { const int f = clockFrame(t, 0, 0); if (last >= 0 && f < last && !(last > 60 && f < 3)) back++; last = f; } CHECK(back == 0);   // so avanca
    CHECK(clockFrame(0, 1, 1000) == 10 && clockFrame(0, 1, 6500) == 1);   // fora da superficie gira
    gPkByFile["item/clock_00"] = 9100; gPkByFile["item/compass_00"] = 9101;
    int n = 0; int g1[9] = {0, IT_GOLD, 0, IT_GOLD, IT_REDSTONE, IT_GOLD, 0, IT_GOLD, 0}; CHECK(craftResult(g1, 3, 3, n) == 9100 && n == 1);
    int g2[9] = {0, IT_IRON, 0, IT_IRON, IT_REDSTONE, IT_IRON, 0, IT_IRON, 0}; CHECK(craftResult(g2, 3, 3, n) == 9101 && n == 1);
    int g3[9] = {0, IT_GOLD, 0, IT_GOLD, IT_COAL, IT_GOLD, 0, IT_GOLD, 0}; CHECK(craftResult(g3, 3, 3, n) == 0);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
