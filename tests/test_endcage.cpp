// patchEnd.10: 3 pilares do End guardados por gaiola de barras de ferro; cristais aparecem nos 10
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    gDim = 2; netHost = true; netOn = false;
    for (int cz = -4; cz <= 4; cz++) for (int cx = -4; cx <= 4; cx++) if (!getC(cx, cz)) gen(cx, cz);
    int caged = 0, fires = 0;
    for (int j = 0; j < 10; j++) {
        const double a = 2.0 * (-3.14159265358979 + 3.14159265358979 / 10.0 * j); const int px = (int)floor(42.0 * cos(a)), pz = (int)floor(42.0 * sin(a));
        int hf = -1; for (int y = 120; y > 60; y--) if (getB(px, y, pz) == XB_FIRE && getB(px, y - 1, pz) == BEDROCK) { hf = y; break; }
        if (hf < 0) continue; fires++;
        const bool roof = getB(px, hf + 2, pz) == XB_PANE, wall = getB(px + 2, hf, pz) == XB_PANE && getB(px - 2, hf + 2, pz + 2) == XB_PANE;
        if (roof && wall) { caged++; CHECK(getM(px + 2, hf, pz) != 0); }   // barras conectadas
    }
    CHECK(fires == 10); CHECK(caged == 3);
    Phys P; memset(&P, 0, sizeof P); for (int t = 0; t < 25; t++) crystalsTick(P);
    CHECK(crystalsAlive(0, 90, 0, 200.0) == 10);
    printf("test_endcage: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
