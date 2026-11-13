// patchGenSlice.1: a geracao em etapas (genAsync*) tem de dar EXATAMENTE o mesmo mundo que gen() sincrono; mede o custo de cada etapa
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int gOX = 0, gOZ = 0;
static uint64_t hashWorld(int n) {
    uint64_t h = 1469598103934665603ULL;
    for (int cx = 0; cx < n; cx++) for (int cz = 0; cz < n; cz++) { Chunk* c = getC(cx + gOX, cz + gOZ); if (!c) return 0; for (size_t i = 0; i < (size_t)CW * CH * CW; i++) { h = (h ^ (uint64_t)c->b[i]) * 1099511628211ULL; h = (h ^ (uint64_t)c->m[i]) * 1099511628211ULL; } }
    return h;
}
#ifdef GEN_PROFILE
static void popReport(int n) { printf("populate por secao (ms/chunk): lagos=%.2f masmorras=%.2f minerios=%.2f lapis+discos=%.2f nascentes=%.2f\n", gPopT[0]/n, gPopT[1]/n, gPopT[2]/n, gPopT[3]/n, gPopT[4]/n); }
#endif
int main() {
    int fails = 0; const int N = 6;
    static const struct { uint32_t seed; int ox, oz; } CASES[] = {{123456789u, 0, 0}, {42u, 300, -250}, {2026u, -1000, 777}, {7777777u, 55, 4000}};
    for (const auto& cs : CASES) { const uint32_t seed = cs.seed; gOX = cs.ox; gOZ = cs.oz;
    resetWorldData(); WORLD_SEED = seed; gDim = 0;
    for (int cx = 0; cx < N; cx++) for (int cz = 0; cz < N; cz++) gen(cx + gOX, cz + gOZ);
    const uint64_t hs = hashWorld(N); const size_t spillS = gSpill.size();
    resetWorldData(); WORLD_SEED = seed; gDim = 0; for (double& v : gGenStageMs) v = 0;
    int steps = 0; double worst = 0;
    for (int cx = 0; cx < N; cx++) for (int cz = 0; cz < N; cz++) {
        if (getC(cx + gOX, cz + gOZ)) continue; if (!genSliceable(cx + gOX, cz + gOZ)) { printf("FALHOU: nao sliceable %d,%d\n", cx, cz); fails++; continue; }
        genAsyncBegin(cx + gOX, cz + gOZ);
        while (true) { const double t0 = perfNow(); const bool more = genAsyncStep(); worst = std::max(worst, perfNow() - t0); steps++; if (!more) break; if (getC(cx + gOX, cz + gOZ)) { printf("FALHOU: chunk em W antes de terminar\n"); fails++; } }
        if (!getC(cx + gOX, cz + gOZ)) { printf("FALHOU: chunk nao publicado\n"); fails++; }
    }
    const uint64_t ha = hashWorld(N);
    printf("hash sync %llx async %llx | etapas %d | pior etapa %.2f ms\n", (unsigned long long)hs, (unsigned long long)ha, steps, worst);
    if (hs != ha) { printf("FALHOU: mundo em etapas difere do sincrono\n"); fails++; }
    if (gSpill.size() != spillS) { printf("FALHOU: spill difere (%zu vs %zu)\n", spillS, gSpill.size()); fails++; }
    double tot = 0; for (double v : gGenStageMs) tot += v; printf("custo por etapa (%% do total):"); for (int i = 0; i < GEN_NST; i++) printf(" e%d=%.1f%%", i, tot > 0 ? 100.0 * gGenStageMs[i] / tot : 0.0); printf(" | total %.1f ms/chunk\n", tot / (N * N));
#ifdef GEN_PROFILE
    popReport(N * N);
#endif
    }
    gOX = gOZ = 0;
    // gen() sincrono no meio de um job: termina o job e nao duplica o chunk
    resetWorldData(); gDim = 0; genAsyncBegin(0, 0); genAsyncStep(); gen(1, 0);
    if (!getC(0, 0) || !getC(1, 0) || gGenJob) { printf("FALHOU: gen() com job em andamento\n"); fails++; }
    // abort (resetWorldData) com job em andamento nao vaza nem publica
    genAsyncBegin(5, 5); genAsyncStep(); resetWorldData(); if (gGenJob || getC(5, 5)) { printf("FALHOU: abort\n"); fails++; }
    printf(fails ? "RESULTADO: %d FALHAS\n" : "RESULTADO: ok\n", fails); return fails ? 1 : 0;
}
