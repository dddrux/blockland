// patchGPU.1: conectividade de secoes (culling de cavernas). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static bool has(const Chunk* c, int s, int a, int b) { return (c->conn[s] >> cvPair(a, b)) & 1; }
int main() {
    Chunk* c = new Chunk(); memset(c->b, 0, sizeof c->b); memset(c->m, 0, sizeof c->m);
    for (int y = 0; y < 16; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) c->b[idx(x, y, z)] = STONE;   // secao 0 toda de pedra
    chunkConnCompute(c, 255); CHECK(c->conn[0] == 0); CHECK(c->conn[1] == 0x7FFF); CHECK(c->connOk);   // secao 1 = ar
    for (int x = 0; x < 16; x++) c->b[idx(x, 5, 5)] = AIR;   // tunel em X
    chunkConnCompute(c, 255); CHECK(has(c, 0, 0, 1)); CHECK(!has(c, 0, 0, 3)); CHECK(!has(c, 0, 4, 5)); CHECK(!has(c, 0, 2, 3));
    for (int y = 5; y < 16; y++) c->b[idx(0, y, 5)] = AIR;   // sobe pela borda -X ate o topo
    chunkConnCompute(c, 255); CHECK(has(c, 0, 0, 3)); CHECK(has(c, 0, 1, 3)); CHECK(!has(c, 0, 4, 5));
    for (int x = 0; x < 16; x++) c->b[idx(x, 5, 5)] = WATER;   // agua nao esconde a vista
    chunkConnCompute(c, 255); CHECK(has(c, 0, 0, 1));
    c->b[idx(8, 5, 5)] = STONE; chunkConnCompute(c, 255); CHECK(!has(c, 0, 1, 3) || has(c, 0, 0, 3)); CHECK(!has(c, 0, 1, 0));   // parede no meio separa -X de +X
    chunkConnCompute(c, 15); CHECK(c->conn[1] == 0x7FFF);   // acima do topo: tudo aberto
    printf("test_cave: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
