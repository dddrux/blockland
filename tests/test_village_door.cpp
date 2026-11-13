// Portas geradas pela vila: as duas metades existem, mesmo meta (so o bit 8 difere) e a de baixo tem suporte.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    if (chdir("..") != 0) return 99;
    int doors = 0, noTop = 0, metaBad = 0, noSup = 0, stray = 0;
    for (long seed = 1; seed <= 6 && doors < 10; seed++) {
        WORLD_SEED = seed; resetWorldData();
        for (int cz = -12; cz <= 12; cz++) for (int cx = -12; cx <= 12; cx++) if (!getC(cx, cz)) gen(cx, cz);
        for (int cz = -12; cz <= 12; cz++) for (int cx = -12; cx <= 12; cx++) { Chunk* c = getC(cx, cz); if (!c) continue;
            for (int y = 2; y < 200; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) if (c->b[idx(x, y, z)] == XB_DOOR) {
                const int wx = cx * 16 + x, wz = cz * 16 + z, m = c->m[idx(x, y, z)];
                if (m & 8) { if (getB(wx, y - 1, wz) != XB_DOOR) { stray++; printf("metade de cima solta em %d %d %d (seed %ld)\n", wx, y, wz, seed); } continue; }
                doors++;
                if (getB(wx, y + 1, wz) != XB_DOOR) { noTop++; printf("sem metade de cima em %d %d %d m=%d (seed %ld)\n", wx, y, wz, m, seed); continue; }
                if ((getM(wx, y + 1, wz) ^ 8) != m) { metaBad++; printf("meta difere %d vs %d\n", m, getM(wx, y + 1, wz)); }
                if (doors <= 6) { rsUse(wx, y, wz); const int a = getB(wx, y, wz), b2 = getB(wx, y + 1, wz); printf("apos abrir: %d/%d meta %d/%d\n", a, b2, getM(wx, y, wz), getM(wx, y + 1, wz));
                    static const int D[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}}; for (auto& d : D) { blockChangedHook(wx + d[0], y + d[1], wz + d[2], AIR, STONE); blockChangedHook(wx + d[0], y + 1 + d[1], wz + d[2], AIR, STONE); }
                    for (int t = 0; t < 40; t++) runWorldTick(); if (getB(wx, y, wz) != XB_DOOR || getB(wx, y + 1, wz) != XB_DOOR) { noTop += 100; printf("PORTA QUEBROU apos abrir/vizinho em %d %d %d\n", wx, y, wz); } }
                if (!rsSupported(XB_DOOR, wx, y, wz)) { noSup++; printf("sem suporte em %d %d %d (abaixo=%d)\n", wx, y, wz, getB(wx, y - 1, wz)); }
            } }
    }
    printf("portas=%d semTopo=%d metaDifere=%d semSuporte=%d topoSolto=%d\n", doors, noTop, metaBad, noSup, stray); return 0;
}
