// patchGenSlice.1: stream() com slice=true (1 etapa por chamada) tem de carregar o MESMO mundo que stream() sincrono, na mesma ordem
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static uint64_t hashAll() {
    uint64_t h = 1469598103934665603ULL; std::vector<int64_t> ks; for (auto& kv : W) ks.push_back(kv.first); std::sort(ks.begin(), ks.end());
    for (int64_t k : ks) { Chunk* c = W[k]; h = (h ^ (uint64_t)k) * 1099511628211ULL; for (size_t i = 0; i < (size_t)CW * CH * CW; i++) { h = (h ^ (uint64_t)c->b[i]) * 1099511628211ULL; h = (h ^ (uint64_t)c->m[i]) * 1099511628211ULL; } }
    return h;
}
static int run(bool slice, uint64_t& h, size_t& nch, int& maxStageCalls) {
    resetWorldData(); WORLD_SEED = 987654321u; gDim = 0; gGenSlice = slice; RAD = 3; gKick = true; int calls = 0; maxStageCalls = 0;
    int lastGrow = 0;
    for (int it = 0; it < 20000; it++) {
        const size_t before = W.size(); stream(0, 0, 1, 0, 0.001, 0.f, -1.f, 1 << 20, slice); calls++;
        if (W.size() != before) lastGrow = calls;
        if (calls - lastGrow > 60 && !gGenJob) break;   // nada novo por 60 chamadas e nenhum job pendente
        for (auto& kv : W) if (!kv.second) { printf("FALHOU: chunk nulo em W\n"); return 1; }
    }
    calls = lastGrow;
    genJobFinishNow(); h = hashAll(); nch = W.size(); return calls;
}
int main() {
    uint64_t h1 = 0, h2 = 0; size_t n1 = 0, n2 = 0; int m = 0; int fails = 0;
    const int c1 = run(false, h1, n1, m); const int c2 = run(true, h2, n2, m);
    printf("sync: %zu chunks em %d chamadas | etapas: %zu chunks em %d chamadas\n", n1, c1, n2, c2);
    printf("hash sync %llx etapas %llx\n", (unsigned long long)h1, (unsigned long long)h2);
    if (n1 != n2 || h1 != h2 || !n1) { printf("FALHOU: stream em etapas difere do sincrono\n"); fails++; }
    if (c2 <= c1) { printf("FALHOU: o modo em etapas deveria levar mais chamadas (1 etapa por chamada)\n"); fails++; }
    printf(fails ? "RESULTADO: %d FALHAS\n" : "RESULTADO: ok\n", fails); return fails ? 1 : 0;
}
