// patchGen.1: floresta densa (carvalho-escuro 2x2), taiga gigante (abeto gigante 2x2), mesa (cacto/arbusto em areia vermelha), maxY nao confiavel
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int sq2(int vbWant, const char* nm, int64_t seed) {
    WORLD_SEED = (uint32_t)seed; resetWorldData(); gDim = 0; int fx = 0, fz = 0; bool found = false;
    for (int z = -3000; z < 3000 && !found; z += 48) for (int x = -3000; x < 3000 && !found; x += 48) { const int v = biomeRawAt((float)x, (float)z); if ((v >= 128 ? v - 128 : v) == vbWant && biomeRawAt((float)x + 40, (float)z) == v && biomeRawAt((float)x, (float)z + 40) == v) { fx = x >> 4; fz = z >> 4; found = true; } }
    if (!found) { printf("  (%s: nao achou nesta semente)\n", nm); return -1; }
    int trunks = 0, logs = 0;
    for (int cz = fz; cz < fz + 3; cz++) for (int cx = fx; cx < fx + 3; cx++) gen(cx, cz);
    for (int cz = fz; cz < fz + 3; cz++) for (int cx = fx; cx < fx + 3; cx++) if (Chunk* c = getC(cx, cz)) for (int y = 64; y < 120; y++) for (int z = 0; z < 15; z++) for (int x = 0; x < 15; x++) {
        const int b = c->b[idx(x, y, z)]; if (b == gTreeLog || b == LOG) logs++;
        if ((b == LOG || b == gTreeLog) && b && c->b[idx(x + 1, y, z)] == b && c->b[idx(x, y, z + 1)] == b && c->b[idx(x + 1, y, z + 1)] == b) trunks++;
    }
    printf("  %s: %d logs, %d celulas 2x2 de tronco\n", nm, logs, trunks); return trunks;
}
int main() {
    if (chdir("..") != 0) return 99;
    int a = -1, b = -1;
    for (int64_t s : {12345LL, 777LL, 2024LL, 99LL}) { if (a <= 0) a = sq2(29, "floresta densa", s); if (b <= 0) b = sq2(32, "taiga gigante", s); if (a > 0 && b > 0) break; }
    if (a >= 0) CHECK(a > 0);   // pelo menos uma arvore de tronco 2x2
    if (b >= 0) CHECK(b > 0);
    // maxY velho nao pode esconder blocos: escreve direto em c->b acima do topo em cache e remalha
    WORLD_SEED = 12345u; resetWorldData(); gDim = 0; gen(0, 0); gen(1, 0); gen(0, 1); gen(1, 1); gen(-1, 0); gen(0, -1); gen(-1, -1); gen(1, -1); gen(-1, 1);
    Chunk* c = getC(0, 0); CHECK(c != nullptr);
    if (c) { const int t0 = chunkTop(c); const int y = std::min(CH - 2, t0 + 20); c->b[idx(3, y, 3)] = STONE;   // sem setB: maxY continua velho
             CHECK(chunkTop(c) == t0 && chunkTopFresh(c) == y); build(0, 0, c); CHECK(c->maxY == y); CHECK(!c->mesh.empty()); }
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
