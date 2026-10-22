#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    resetWorldData(); WORLD_SEED = 123456789u; gDim = 0;
    double t0 = perfNow(); int n = 0;
    for (int cx = 0; cx < 6; cx++) for (int cz = 0; cz < 6; cz++) { gen(cx, cz); n++; }
    uint64_t hh=1469598103934665603ULL; for (int cx = 0; cx < 6; cx++) for (int cz = 0; cz < 6; cz++) if (Chunk* c = getC(cx, cz)) for (size_t i = 0; i < (size_t)CW*CH*CW; i++) hh = (hh ^ (uint64_t)c->b[i]) * 1099511628211ULL;
    printf("hash %llx\n", (unsigned long long)hh); printf("gen: %d chunks, %.1f ms/chunk\n", n, (perfNow() - t0) / n); return 0;
}
