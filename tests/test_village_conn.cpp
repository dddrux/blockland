// Cercas/paineis gerados pela vila: o meta de conexao tem de ser igual a rsConnMask (regra da colocacao manual), inclusive nas bordas de chunk.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    if (chdir("..") != 0) return 99;
    int tot = 0, bad = 0, edge = 0, shown = 0;
    for (long seed = 1; seed <= 6; seed++) {
        WORLD_SEED = seed; resetWorldData();
        for (int cz = -12; cz <= 12; cz++) for (int cx = -12; cx <= 12; cx++) if (!getC(cx, cz)) gen(cx, cz);
        for (int t = 0; t < 40; t++) runWorldTick();
        for (int cz = -12; cz <= 12; cz++) for (int cx = -12; cx <= 12; cx++) { Chunk* c = getC(cx, cz); if (!c) continue;
            for (int y = 2; y < 200; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {
                const int b = c->b[idx(x, y, z)]; if (b != XB_FENCE && b != XB_PANE) continue;
                const int wx = cx * 16 + x, wz = cz * 16 + z, m = c->m[idx(x, y, z)];
                if (cx == -12 || cx == 12 || cz == -12 || cz == 12) continue;   // vizinho de fora da area gerada
                ++tot; const bool isEdge = x == 0 || x == 15 || z == 0 || z == 15; if (isEdge) ++edge;
                const int want = rsConnMask(b, (m >> 4) & 7, wx, y, wz);
                if ((m & 15) != want) { ++bad; if (shown++ < 8) printf("seed %ld %d %d %d tipo %d meta %d esperado %d%s\n", seed, wx, y, wz, b, m & 15, want, isEdge ? " (borda)" : ""); }
            } }
    }
    printf("cercas/paineis=%d (borda=%d) conexao errada=%d\n", tot, edge, bad); return bad ? 1 : 0;
}
