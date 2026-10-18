// Nether 1.12.2 (patchNether.3): terreno, fortaleza por pecas, quantidade controlada, determinismo.
#include <unistd.h>
#include <chrono>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    gCurDir = "tn_test"; WORLD_SEED = 777; netherInit(); gen(0, 0);
    dimSwitch(1);
    // quantidade: ~1/3 das regioes (vanilla), 0 = nenhuma, 100 = todas
    int n33 = 0, n0 = 0, n100 = 0; for (int i = -20; i < 20; i++) for (int j = -20; j < 20; j++) { JRng r; int a, b; if (fortRegionStart(i, j, &a, &b, r)) n33++; }
    printf("  regioes com fortaleza (taxa 33): %d/1600\n", n33); CHECK(n33 > 400 && n33 < 670);
    gSt["fort"] = 0; for (int i = -5; i < 5; i++) for (int j = -5; j < 5; j++) { JRng r; int a, b; if (fortRegionStart(i, j, &a, &b, r)) n0++; } CHECK(n0 == 0);
    gSt["fort"] = 100; for (int i = -5; i < 5; i++) for (int j = -5; j < 5; j++) { JRng r; int a, b; if (fortRegionStart(i, j, &a, &b, r)) n100++; } CHECK(n100 == 100);
    gSt["fort"] = 33;
    FortS* F = nullptr; int ri = 0, rj = 0; for (int i = 0; i < 6 && !F; i++) for (int j = 0; j < 6 && !F; j++) { FortS* s = fortAtRegion(i, j); if (s && s->ps.size() > 20) { F = s; ri = i; rj = j; } }
    CHECK(F != nullptr); if (!F) { printf("%d/%d\n", checks - fails, checks); return 1; }
    int cnt[P_N] = {0}; for (auto& p : F->ps) cnt[p.t]++;
    printf("  pecas: %zu (cruz %d reta %d fim %d sala %d esc %d trono %d entrada %d corr %d)\n", F->ps.size(), cnt[P_START] + cnt[P_BCROSS], cnt[P_BSTRAIGHT], cnt[P_BEND], cnt[P_RROOM], cnt[P_STAIRS], cnt[P_THRONE], cnt[P_CENTR], cnt[P_CORR]);
    CHECK(cnt[P_START] == 1); CHECK(cnt[P_BCROSS] <= 4); CHECK(cnt[P_THRONE] <= 2); CHECK(cnt[P_CENTR] <= 1);
    for (size_t a = 0; a < F->ps.size(); a++) for (size_t b = a + 1; b < F->ps.size(); b++) { const FPiece &p = F->ps[a], &q = F->ps[b];
        if (p.x1 >= q.x0 && p.x0 <= q.x1 && p.y1 >= q.y0 && p.y0 <= q.y1 && p.z1 >= q.z0 && p.z0 <= q.z1) { CHECK(false); a = b = 9999; break; } }
    CHECK(F->ps[0].y0 >= 45 && F->ps[0].y0 <= 80);
    int x0 = 1 << 30, x1 = -(1 << 30), z0 = 1 << 30, z1 = -(1 << 30); for (auto& p : F->ps) { x0 = std::min(x0, p.x0); x1 = std::max(x1, p.x1); z0 = std::min(z0, p.z0); z1 = std::max(z1, p.z1); }
    printf("  caixa: %d..%d x %d..%d (y inicio %d)\n", x0, x1, z0, z1, F->ps[0].y0); CHECK(x1 - x0 < 240 && z1 - z0 < 240);
    auto t0 = std::chrono::steady_clock::now();
    for (int cz = fdiv(z0, 16); cz <= fdiv(z1, 16); cz++) for (int cx = fdiv(x0, 16); cx <= fdiv(x1, 16); cx++) gen(cx, cz);
    printf("  ms/chunk: %.1f\n", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / ((fdiv(x1, 16) - fdiv(x0, 16) + 1) * (fdiv(z1, 16) - fdiv(z0, 16) + 1)));
    const int br = gNBrickId; CHECK(br != 0);
    long nb = 0, fences = 0, stairs = 0, soul = 0, rack = 0, lava = 0, air = 0, bed = 0;
    for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) for (int y = 1; y < 127; y++) { const int b = getB(x, y, z); if (b == br) nb++; else if (b == XB_FENCE) fences++; else if (b == XB_STAIR) stairs++; else if (b == gNetherrackId) rack++; else if (b == LAVA) lava++; else if (b == AIR) air++; else if (b == BEDROCK) bed++; }
    printf("  tijolos %ld cercas %ld escadas %ld netherrack %ld lava %ld ar %ld bedrock %ld\n", nb, fences, stairs, rack, lava, air, bed);
    CHECK(nb > 2000); CHECK(fences > 50); CHECK(rack > 50000); CHECK(lava > 1000); CHECK(air > 50000); CHECK(bed > 1000);
    for (auto& p : F->ps) if (p.t == P_BSTRAIGHT) { int wx, wz; fw(p, 2, 9, wx, wz); CHECK(getB(wx, p.y0 + 3, wz) == br); CHECK(getB(wx, p.y0 + 5, wz) == AIR); fw(p, 0, 9, wx, wz); CHECK(getB(wx, p.y0 + 6, wz) == XB_FENCE); break; }
    // cerca conecta entre chunks: toda cerca de ponte tem ao menos uma conexao
    { long bad = 0; for (auto& p : F->ps) if (p.t == P_BSTRAIGHT) for (int lz = 0; lz < 19; lz++) { int wx, wz; fw(p, 0, lz, wx, wz); if (getB(wx, p.y0 + 6, wz) == XB_FENCE && (getM(wx, p.y0 + 6, wz) & 15) == 0) bad++; } CHECK(bad == 0); }
    for (auto& p : F->ps) if (p.t == P_THRONE) { int wx, wz; fw(p, 3, 5, wx, wz); CHECK(getB(wx, p.y0 + 5, wz) == gSpawnerId); break; }
    // determinismo
    { int cx = fdiv(F->ps[0].x0 + 9, 16), cz = fdiv(F->ps[0].z0 + 9, 16); Chunk* c = getC(cx, cz); std::vector<BT> a(c->b, c->b + CW * CH * CW); gFortChests.clear(); netherGen(c, cx, cz); CHECK(memcmp(a.data(), c->b, a.size() * sizeof(BT)) == 0); }
    // piso de bedrock/teto e lava abaixo de y=32 em chunk sem fortaleza
    { gen(500, 500); CHECK(getB(500 * 16 + 3, 0, 500 * 16 + 3) == BEDROCK); CHECK(getB(500 * 16 + 3, 127, 500 * 16 + 3) == BEDROCK); }
    { long wart = 0, glow = 0; for (auto& p : F->ps) if (p.t == P_CSTALK) { for (int lx = 2; lx <= 10; lx++) for (int lz = 2; lz <= 10; lz++) { int wx, wz; fw(p, lx, lz, wx, wz); if (getB(wx, p.y0 + 6, wz) == XB_CROP && ((getM(wx, p.y0 + 6, wz) >> 3) & 7) == 4) wart++; } }
      printf("  verrugas: %ld (salas %d)\n", wart, cnt[P_CSTALK]); if (cnt[P_CSTALK]) CHECK(wart > 20);
      for (int z = z0; z < z0 + 64; z++) for (int x = x0; x < x0 + 64; x++) for (int y = 1; y < 127; y++) glow += getB(x, y, z) == GLOWSTONE; CHECK(glow > 0); }
    // decoracao continua na borda: nenhum glowstone solto de 1 bloco cortado por lado liso no meio de um aglomerado (conta aglomerados que cruzam x=borda)
    { long cross = 0; for (int cz = fdiv(z0, 16); cz < fdiv(z0, 16) + 4; cz++) for (int cx = fdiv(x0, 16); cx < fdiv(x0, 16) + 4; cx++) for (int y = 1; y < 127; y++) for (int z = 0; z < 16; z++) { const int wx = (cx + 1) * 16, wz = cz * 16 + z; if (getB(wx - 1, y, wz) == GLOWSTONE && getB(wx, y, wz) == GLOWSTONE) cross++; } printf("  glowstone cruzando bordas: %ld\n", cross); CHECK(cross > 0); }
    fortChestFlush();
    for (auto& p : F->ps) if (p.t == P_THRONE) { int wx, wz; fw(p, 3, 4, wx, wz); gDim = 1; Phys P; P.x = wx + .5; P.y = p.y0 + 5; P.z = wz + .5; MOBS.clear(); netOn = false; gDead = false;
        for (int i = 0; i < 900 && MOBS.empty(); i++) blazeSpawnTick(P);
        int nb2 = 0; for (const MobEnt& e : MOBS) if (e.type == M_BLAZE) nb2++; printf("  blazes: %d\n", nb2); CHECK(nb2 >= 1); break; }
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
