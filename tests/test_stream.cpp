// stream(): carga completa, ociosidade (custo ~0), reacao a edicao, modo "longe" e custo medio por chunk.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static size_t vtx() { size_t n = 0; for (auto& kv : W) n += kv.second->mesh.size(); return n; }
int main() {
    WORLD_SEED = 777u; g112.ready = false; RAD = 10;
    const double t0 = perfNow(); int passes = 0;
    while (passes < 400) { stream(0, 0, 64, 64, -1.0, 0.f, -1.f); passes++; if (!gKick) break; }
    const double ms = perfNow() - t0;
    CHECK(!gKick); printf("  RAD 10: %zu chunks em %d passadas, %.0f ms | gen %.1f luz %.1f malha %.1f (ms totais) -> %.2f ms/chunk gen\n", W.size(), passes, ms, gPf[PS_GEN], gPf[PS_LIGHT], gPf[PS_MESH], gPf[PS_GEN] / std::max(1, gPfGenN));
    int bad = 0; for (int dz = -9; dz <= 9; dz++) for (int dx = -9; dx <= 9; dx++) if (dx * dx + dz * dz <= 10 * 10) { Chunk* c = getC(dx, dz); if (!c || c->dirty || !c->lc) bad++; }
    CHECK(bad == 0);
    // ocioso: nenhuma geracao nova e retorno imediato
    const size_t n0 = W.size(); const double t1 = perfNow(); for (int i = 0; i < 1000; i++) stream(0, 0, 64, 64, -1.0, 0.f, -1.f);
    printf("  1000 chamadas ociosas: %.3f ms\n", perfNow() - t1); CHECK(W.size() == n0); CHECK(perfNow() - t1 < 5.0);
    // editar um bloco acorda o stream e refaz a malha
    Chunk* c = getC(0, 0); setB(3, 100, 3, STONE); CHECK(c->dirty && gKick); stream(0, 0, 64, 64, -1.0, 0.f, -1.f); CHECK(!c->dirty);
    // andar para outro chunk gera os novos
    for (int i = 0; i < 6; i++) stream(3, 0, 64, 64, -1.0, 1.f, 0.f); CHECK(getC(3 + 10, 0) != nullptr);
    // modo longe: menos vertices, e volta ao normal ao chegar perto
    const size_t vNear = vtx(); for (auto& kv : W) { kv.second->mesh.clear(); } 
    resetWorldData(); WORLD_SEED = 777u; g112.ready = false;
    for (int i = 0; i < 12; i++) stream(0, 0, 64, 64, -1.0, 0.f, -1.f, 5);
    const size_t vFar = vtx(); printf("  vertices (RAD 10): normal %zu | farR=5 %zu (%.0f%%)\n", vNear, vFar, 100.0 * vFar / std::max<size_t>(1, vNear));
    CHECK(vFar < vNear * 0.75);
    Chunk* f = getC(9, 0); CHECK(f && f->farM); Chunk* n = getC(1, 0); CHECK(n && !n->farM);
    for (int i = 0; i < 6; i++) stream(8, 0, 64, 64, -1.0, 1.f, 0.f, 5); Chunk* f2 = getC(9, 0); CHECK(f2 && !f2->farM);   // agora esta a 1 chunk
    // orcamento de tempo: nunca passa muito de 1 tarefa alem do limite
    resetWorldData(); WORLD_SEED = 777u; g112.ready = false; const double t2 = perfNow(); stream(0, 0, 999, 999, 1.0); const double one = perfNow() - t2;
    printf("  orcamento 1 ms: %.1f ms, %zu chunks gerados\n", one, W.size()); CHECK(W.size() >= 1 && W.size() < 40);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
