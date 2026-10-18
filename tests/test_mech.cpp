// patchMech.1 + patchTerrain.5: gelo/teia/alma de areia, cacto, lagos
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 0;
    gPkByFile["block/ice"] = PK0 + 5; gPkByFile["block/cobweb"] = PK0 + 6; gPkByFile["block/soul_sand"] = PK0 + 7; gPkByFile["block/slime_block"] = PK0 + 8;
    genImpl(0, 0);
    int fy = 150; for (int y = 40; y < 200; y++) if (getB(5, y, 5) == AIR) { fy = y; break; }
    Phys P; P.x = 5.5; P.y = fy; P.z = 5.5; P.onGround = true;
    setB(5, fy - 1, 5, STONE); CHECK(groundSlip(P) == .6f);
    setB(5, fy - 1, 5, PK0 + 5); CHECK(groundSlip(P) > .97f);
    setB(5, fy - 1, 5, PK0 + 8); CHECK(groundSlip(P) > .79f && groundSlip(P) < .81f);
    setB(5, fy - 1, 5, PK0 + 7); P.mx = 1; P.mz = 1; mechSlow(P); CHECK(fabs(P.mx - .4) < 1e-9);
    setB(5, fy - 1, 5, STONE); setB(5, fy, 5, PK0 + 6); P.mx = 1; P.my = 1; P.mz = 1; P.onGround = false; mechSlow(P);
    CHECK(fabs(P.mx - .25) < 1e-9 && fabs(P.my - .05) < 1e-9);
    setB(5, fy, 5, AIR);
    int lakes = 0, waterAbove = 0;   // lagos: contar chunks com agua acima do mar
    for (int cz = -6; cz < 6; cz++) for (int cx = -6; cx < 6; cx++) { genImpl(cx, cz); int n = 0; for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) for (int y = SEA + 2; y < SEA + 40; y++) if (getB(cx * 16 + x, y, cz * 16 + z) == WATER) n++; if (n > 4) lakes++; waterAbove += n; }
    printf("  chunks com lago: %d de 144\n", lakes);
    CHECK(lakes >= 1);
    {   // patchMob.2: vaca empurrada contra a parede pelo jogador — o corpo gira para a direcao do empurrao
        for (int x = 0; x < 16; x++) for (int z = 0; z < 16; z++) for (int y = 59; y < 75; y++) setB(x, y, z, y == 59 ? STONE : AIR);
        for (int y = 60; y < 64; y++) for (int z = 0; z < 16; z++) setB(10, y, z, STONE);
        MOBS.clear(); spawnMob(0, 8.6, 60, 8.5, 0);
        Phys Q; Q.x = 6; Q.y = 60; Q.z = 8.5; Q.px = Q.x; Q.py = Q.y; Q.pz = Q.z; Q.onGround = true;
        for (int t = 0; t < 50; t++) { Q.px = Q.x; Q.py = Q.y; Q.pz = Q.z; mobPush(Q); moveEntityWithHeading(Q, -90, 0, .98f); MobEnt& c = MOBS[0]; c.hasT = false; tickMob(c, Q.x, Q.y, Q.z); }
        CHECK(fabs(mwrap(MOBS[0].yaw)) > 10.f); CHECK(fabs(MOBS[0].hyaw) <= 75.f);
    }
    {   // patchVil.1: vila sem aldeoes ganha um a cada 30 s (ate 3)
        for (int x = 0; x < 16; x++) for (int z = 0; z < 16; z++) for (int y = 59; y < 75; y++) setB(x, y, z, y == 59 ? STONE : AIR);
        MOBS.clear(); gVilQ.clear(); gVilCenters.clear(); gVilCenters.push_back({8.0, 60.0, 5.0});
        Phys V; V.x = 8; V.y = 60; V.z = 8; if (Chunk* cc = getC(0, 0)) { static uint8_t dummyLc[4]; cc->lc = dummyLc; }
        for (int k = 0; k < 5; k++) { for (int t = 0; t < 600; t++) vilNaturalTick(V); vilSpawnPending(); }
        int n = 0; for (auto& m : MOBS) if (m.type == M_VILLAGER) n++;
        printf("  aldeoes repostos: %d\n", n);
        CHECK(n >= 1 && n <= 3);
    }
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
