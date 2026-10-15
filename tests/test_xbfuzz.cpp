// Fuzz: toda combinacao id (XB_DUST..XB_END) x meta (0..255) em rsShape/rsQuads/xbSupported/xbNeighbor nao pode estourar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 8; z++) for (int x = -8; x < 8; x++) for (int y = 199; y < 216; y++) setB(x, y, z, y == 199 ? STONE : AIR);
    int n = 0;
    for (int id = XB_DUST; id < XB_END; id++) for (int m = 0; m < 256; m++) {
        AABB o[SH_MAXB + 8]; for (int s = 0; s < 2; s++) { const int r = rsShape(1, 200, 1, id, m, s, o); if (r > SH_MAXB || r < 0) { printf("  FALHOU rsShape id=%d m=%d n=%d\n", id, m, r); } }
        rsQuads(id, m); setBM(2, 200, 2, id, m); xbNeighbor(2, 200, 2, 2, 199, 2); setBM(2, 200, 2, AIR, 0); ++n;
    }
    printf("test_xbfuzz: %d combinacoes sem estouro\n", n); return 0;
}
