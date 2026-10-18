// patchEnd.1: portal do End visivel (4 tiles + malha), abre com 12 olhos, entrar leva ao End e volta.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) { printf("SKIP\n"); return 0; }
    gCurDir = "te_test"; WORLD_SEED = 5; gEndFrameId = 1100; netherInit();
    CHECK(gEndTile[0] >= 0 && gEndTile[3] >= 0);
    CHECK(gEndPortalQ.size() == 8);
    float top = 0; for (auto& q : gEndPortalQ) top = std::max(top, q.p[0][1]); CHECK(top > .74f && top <= .75f);
    gen(0, 0); gNoBlockHooks = true;
    const int y = 100, cx = 5, cz = 5;
    for (int z = cz - 4; z <= cz + 4; z++) for (int x = cx - 4; x <= cx + 4; x++) { setB(x, y - 1, z, STONE); setB(x, y, z, AIR); setB(x, y + 1, z, AIR); }
    for (int k = -1; k <= 1; k++) { int a[4][2] = {{cx + k, cz - 2}, {cx + k, cz + 2}, {cx - 2, cz + k}, {cx + 2, cz + k}}; for (auto& p : a) setBM(p[0], y, p[1], gEndFrameId, 4); }
    CHECK(tryOpenEnd(cx + 1, y, cz - 2));
    CHECK(getB(cx, y, cz) == XB_PORTAL && (getM(cx, y, cz) & 2));
    CHECK(getB(cx + 1, y, cz + 1) == XB_PORTAL);
    Phys P; memset(&P, 0, sizeof P); P.x = cx + .5; P.z = cz + .5; P.y = y + .5; P.px = P.x; P.py = P.y; P.pz = P.z;
    CHECK(gDim == 0);
    netherTick(P); CHECK(gDim == 2);
    printf("%d checks, %d falhas\n", checks, fails); return fails != 0;
}
