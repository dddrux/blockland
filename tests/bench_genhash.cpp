// hash do mundo gerado por gen() em varias seeds/posicoes (compara o gerador novo com o original: tem de dar os MESMOS hashes)
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    static const struct { uint32_t seed; int ox, oz; } CASES[] = {{123456789u, 0, 0}, {42u, 300, -250}, {2026u, -1000, 777}, {7777777u, 55, 4000}, {99u, -5000, -5000}, {31337u, 2500, 100}};
    for (const auto& cs : CASES) {
        resetWorldData(); WORLD_SEED = cs.seed; gDim = 0; uint64_t h = 1469598103934665603ULL;
        for (int cx = 0; cx < 8; cx++) for (int cz = 0; cz < 8; cz++) gen(cx + cs.ox, cz + cs.oz);
        for (int cx = 0; cx < 8; cx++) for (int cz = 0; cz < 8; cz++) { Chunk* c = getC(cx + cs.ox, cz + cs.oz); if (!c) { printf("FALHOU: sem chunk\n"); return 1; } for (size_t i = 0; i < (size_t)CW * CH * CW; i++) { h = (h ^ (uint64_t)c->b[i]) * 1099511628211ULL; h = (h ^ (uint64_t)c->m[i]) * 1099511628211ULL; } }
        printf("seed %u @%d,%d hash %llx\n", cs.seed, cs.ox, cs.oz, (unsigned long long)h);
    }
    return 0;
}
