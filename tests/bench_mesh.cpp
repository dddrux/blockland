// bench: tempo de malha (build) por chunk
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    resetWorldData(); WORLD_SEED = 123456789u; gDim = 0;
    for (int cx = -1; cx < 7; cx++) for (int cz = -1; cz < 7; cz++) gen(cx, cz);
    double t0 = perfNow(); int n = 0; size_t nv = 0;
    for (int rep = 0; rep < 3; rep++) for (int cx = 0; cx < 6; cx++) for (int cz = 0; cz < 6; cz++) { Chunk* c = getC(cx, cz); if (!c) continue; build(cx, cz, c); n++; nv += c->mesh.size(); }
    printf("malha: %d builds, %.2f ms/build, %zu verts/build\n", n, (perfNow() - t0) / n, nv / n); return 0;
}
