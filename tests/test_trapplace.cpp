#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int AY = 200; for (int z = -4; z < 12; z++) for (int x = -4; x < 12; x++) for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    for (int v = 0; v < 2; v++) {
        const int hh[3] = {2 + v * 3, AY - 1, 2}, pp[3] = {2 + v * 3, AY, 2};
        bool ok = rsPlace(XB_TRAP, v, hh, pp, 0.f, 2.5, AY, 5.0);
        printf("var %d: colocou=%d bloco=%d (XB_TRAP=%d) meta=%d\n", v, (int)ok, getB(pp[0], pp[1], pp[2]), (int)XB_TRAP, getM(pp[0], pp[1], pp[2]));
        const int hw[3] = {2 + v * 3, AY, 6}; setB(hw[0], AY, 6, STONE); const int pw[3] = {2 + v * 3, AY, 5};
        ok = rsPlace(XB_TRAP, v, hw, pw, 0.f, 2.5, AY, 3.0); printf("  parede: colocou=%d bloco=%d meta=%d\n", (int)ok, getB(pw[0], pw[1], pw[2]), getM(pw[0], pw[1], pw[2]));
    }
    return 0;
}
