// patchJungle.1: arvore gigante da selva (2x2) + vinhas; gera uma regiao e confere
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    int fails = 0;
    resetWorldData(); WORLD_SEED = 424242u; gDim = 0; gTcx = 0; gTcz = 0;
    Chunk* c = new Chunk(); memset(c->b, 0, sizeof c->b); memset(c->m, 0, sizeof c->m);
    for (int x = 0; x < CW; x++) for (int z = 0; z < CW; z++) for (int y = 0; y < 60; y++) c->b[idx(x, y, z)] = (BT)STONE;
    treeSpecies(3); const int logId = gTreeLog;
    TreeCtx t{c, JR2(777)}; mcMegaJungle(t, 6, 60, 6);
    int h[4]; int q = 0; for (int xo = 0; xo < 2; xo++) for (int zo = 0; zo < 2; zo++) { int n = 0; for (int y = 60; y < CH; y++) if (c->b[idx(6 + xo, y, 6 + zo)] == logId) n++; h[q++] = n; }
    printf("troncos 2x2: %d %d %d %d\n", h[0], h[1], h[2], h[3]);
    if (h[0] < 10 || h[0] > 31) { printf("FALHOU altura\n"); fails++; }
    int leaves = 0, vines = 0; for (size_t i = 0; i < (size_t)CW * CH * CW; i++) { if (isLeafB(c->b[i])) leaves++; if (c->b[i] == XB_VINE && c->m[i]) vines++; }
    printf("folhas %d, vinhas %d\n", leaves, vines);
    if (leaves < 150) { printf("FALHOU folhas\n"); fails++; } if (vines < 5) { printf("FALHOU vinhas\n"); fails++; }
    // mundo real: regiao grande, deve gerar sem travar e ter troncos gigantes (se houver selva)
    int tv = 0; for (int cx = -8; cx < 8; cx++) for (int cz = -8; cz < 8; cz++) { gen(cx, cz); }
    for (int cx = -8; cx < 8; cx++) for (int cz = -8; cz < 8; cz++) if (Chunk* d = getC(cx, cz)) for (size_t i = 0; i < (size_t)CW * CH * CW; i++) if (d->b[i] == XB_VINE) tv++;
    printf("vinhas no mundo (16x16 chunks): %d\n", tv);
    printf("%s\n", fails ? "FALHAS" : "OK"); return fails ? 1 : 0;
}
