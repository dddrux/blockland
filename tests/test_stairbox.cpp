#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static const int AY = 200;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -4; z < 12; z++) for (int x = -4; x < 12; x++) for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    for (int r = 0; r < 4; r++) for (int t = 0; t < 2; t++) {
        setBM(2, AY, 2, XB_STAIR, (11 << 3) | (t << 2) | r); AABB o[SH_MAXB]; const int n = blockBoxesId(XB_STAIR, 2, AY, 2, false, o);
        char buf[400]; int k = snprintf(buf, 400, "rot%d top%d:", r, t); for (int i = 0; i < n; i++) k += snprintf(buf + k, 400 - k, " [x%.2f-%.2f y%.2f-%.2f z%.2f-%.2f]", o[i].x0 - 2, o[i].x1 - 2, o[i].y0 - AY, o[i].y1 - AY, o[i].z0 - 2, o[i].z1 - 2); fprintf(stderr, "%s\n", buf);
    }
    return 0;
}
