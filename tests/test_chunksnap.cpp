// test_chunksnap: o que este teste cobre (1 linha). Rodar: tools/t.sh chunksnap
#include <unistd.h>
#include <thread>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    resetWorldData(); WORLD_SEED = 123456789u; gDim = 0;
    for (int cx = -1; cx < 6; cx++) for (int cz = -1; cz < 6; cz++) gen(cx, cz);
    for (int cx = 0; cx < 5; cx++) for (int cz = 0; cz < 5; cz++) build(cx, cz, getC(cx, cz));
    const int tx = 2, tz = 2; Chunk* c = getC(tx, tz);
    SECTION("T1 malha em thread == malha direta (solida+agua)");
    build(tx, tz, c); std::vector<V> m0 = c->mesh, w0 = c->wmesh; const int cube0 = c->cubeN, nv0 = c->nv;
    uint8_t* lc0 = new uint8_t[CW * CH * CW]; memcpy(lc0, c->lc, CW * CH * CW);
    ChunkSnap* sn = snapTake(tx, tz, false, false, 0); CHECK(sn != nullptr);
    std::thread th([&] { meshRun(sn); }); th.join();
    CHECK(sn->c[4]->mesh.size() == m0.size() && memcmp(sn->c[4]->mesh.data(), m0.data(), m0.size() * sizeof(V)) == 0);
    CHECK(sn->c[4]->wmesh.size() == w0.size() && memcmp(sn->c[4]->wmesh.data(), w0.data(), w0.size() * sizeof(V)) == 0);
    CHECK(memcmp(sn->c[4]->lc, lc0, CW * CH * CW) == 0);
    SECTION("T2 snapApply devolve o resultado ao chunk real");
    c->mesh.clear(); c->wmesh.clear(); c->nv = 0; c->cubeN = -1;
    snapApply(sn, c); delete sn;
    CHECK(c->mesh.size() == m0.size() && c->nv == nv0 && c->cubeN == cube0 && c->lc != nullptr && memcmp(c->lc, lc0, CW * CH * CW) == 0);
    SECTION("T3 snapshot nao toca o cache global nem W (getC cai no 3x3)");
    Chunk* far = getC(0, 0); ChunkSnap* s2 = snapTake(tx, tz, true, false, 0);
    tlSnapOn = true; tlSnapCx = tx; tlSnapCz = tz; for (int i = 0; i < 9; i++) tlSnapC[i] = s2->c[i];
    CHECK(getC(tx, tz) == s2->c[4] && getC(0, 0) == nullptr && getC(tx + 1, tz - 1) == s2->c[2]);
    tlSnapOn = false; for (int i = 0; i < 9; i++) tlSnapC[i] = nullptr; delete s2;
    CHECK(getC(0, 0) == far);
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
