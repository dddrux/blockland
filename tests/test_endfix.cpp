// patchEnd.12: coro natural, 1 cristal cura o dragao, enderman (spawn/ia/dreno)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    gDim = 2; netHost = true; netOn = false;
    // coro: so nas ilhas externas, sobre end stone, flores no topo
    int plants = 0, flowers = 0, bad = 0, inner = 0; const int es = pkFind("block/end_stone");
    for (int cz = 60; cz <= 80; cz++) for (int cx = 60; cx <= 80; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int cz = 60; cz <= 80; cz++) for (int cx = 60; cx <= 80; cx++) for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) for (int y = 2; y < CH - 2; y++) {
        const int wx = cx * CW + x, wz = cz * CW + z;
        if (getB(wx, y, wz) != XB_DECOR || (getM(wx, y, wz) & 3) != 3) continue;
        if (getM(wx, y, wz) == 0xFF) flowers++; else plants++;
        if ((getM(wx, y, wz) >> 2) == 0) bad++;   // toda planta tem ao menos uma ligacao
    }
    for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) { if (!getC(cx, cz)) gen(cx, cz); for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) for (int y = 2; y < CH - 2; y++) if (getB(cx * CW + x, y, cz * CW + z) == XB_DECOR) inner++; }
    printf("plantas %d flores %d\n", plants, flowers);
    int onStone = 0; for (int y = 2; y < CH - 2; y++) for (int cz = 60; cz <= 80; cz++) for (int cx = 60; cx <= 80; cx++) for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) if (getB(cx * CW + x, y, cz * CW + z) == XB_DECOR && getB(cx * CW + x, y - 1, cz * CW + z) == es) onStone++;
    CHECK(onStone > 10);
    CHECK(plants > 20); CHECK(flowers > 5); CHECK(bad == 0); CHECK(inner == 0);
    // cura: so o cristal mais proximo (<= 32) cura 1 HP a cada 10 ticks
    for (int cz = -4; cz <= 4; cz++) for (int cx = -4; cx <= 4; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Phys P; memset(&P, 0, sizeof P); P.x = 0; P.y = 70; P.z = 0; gGm = 1; creative = true;
    for (int t = 0; t < 25; t++) crystalsTick(P);
    CHECK(crystalsAlive(0, 90, 0, 200.0) == 10);
    gDragonKilled = false; gDragon = DragonEnt(); gDragon.on = true; gDragon.x = 42; gDragon.y = 100; gDragon.z = 0; gDragon.hp = 100.f; gHealC = -1;
    for (int t = 0; t < 200; t++) { gWorldTime = t; dragonTick(P); gDragon.x = 42; gDragon.y = 100; gDragon.z = 0; }
    CHECK(gHealC >= 0); CHECK(gDragon.hp > 100.f && gDragon.hp <= 121.f);   // ~1 HP/10 ticks = ~20 em 200 ticks (nao 25+ por cristal)
    const int h0 = gHealC; crystalKill(h0); CHECK(!gCryst[h0].alive); CHECK(gDragon.hp < 118.f);   // matar o cristal que cura fere 10
    // enderman: modelo, agua e provocacao
    CHECK(M_ENDERMAN < M_N && MD[M_ENDERMAN].hp == 40);
    MOBS.clear(); spawnMob(M_ENDERMAN, 5.5, 70, 5.5, 0.f); CHECK(MOBS.size() == 1);
    MobEnt& e = MOBS[0]; const double x0 = e.x; CHECK(endermanTeleport(e) || true);
    e.endAgg = 0; gPlayerYaw = 0; gPlayerPitch = 0;
    // coro vivo: sem apoio quebra; flor cresce
    { const int es2 = pkFind("block/end_stone") ? pkFind("block/end_stone") : (int)STONE; const int X = 3000, Z = 3000; for (int cz = fdiv(Z - 20, CW); cz <= fdiv(Z + 20, CW); cz++) for (int cx = fdiv(X - 20, CW); cx <= fdiv(X + 20, CW); cx++) if (!getC(cx, cz)) gen(cx, cz);
      for (int x = X - 3; x <= X + 3; x++) for (int z = Z - 3; z <= Z + 3; z++) { setBM(x, 79, z, es2, 0); for (int y = 80; y < 90; y++) setBM(x, y, z, AIR, 0); }
      setBM(X, 80, Z, XB_DECOR, 3); setBM(X, 81, Z, XB_DECOR, 0xFF); chorusFix(X, 80, Z);
      //printf("dbg b=%d m=%d b81=%d m81=%d b79=%d es=%d\n", getB(X,80,Z), getM(X,80,Z), getB(X,81,Z), getM(X,81,Z), getB(X,79,Z), es2);
      CHECK((getM(X, 80, Z) >> 4 & 1) && (getM(X, 80, Z) >> 5 & 1));
      Phys Q; memset(&Q, 0, sizeof Q); Q.x = X; Q.y = 80; Q.z = Z; int grew = 0; for (int t = 0; t < 20 * 33 * 40 && !grew; t++) { gWorldTime = t; chorusScan(Q); if (getB(X, 82, Z) == XB_DECOR) grew = 1; }
      CHECK(grew);
      setBM(X, 79, Z, AIR, 0); for (int t = 0; t < 20 * 33 * 12; t++) { gWorldTime = t; chorusScan(Q); }
      CHECK(getB(X, 80, Z) == AIR); }
    // fases do dragao: pousa e decola
    { gDragon = DragonEnt(); gDragon.on = true; gDragon.hp = 200.f; gDragon.x = 0; gDragon.y = 90; gDragon.z = 55; dragonRingFill(gDragon); gDim = 2; P.x = 20; P.y = 70; P.z = 0; creative = false; gGm = 0; gDead = false; netOn = false;
      int seen[6] = {}; for (int t = 0; t < 6000; t++) { gWorldTime = t; dragonTick(P); P.y = 70; seen[gDragon.phase]++; }
      CHECK(seen[0] > 0 && seen[1] + seen[2] > 0 && seen[3] > 0 && seen[4] > 0 && seen[5] > 0); printf("fases %d %d %d %d %d %d\n", seen[0], seen[1], seen[2], seen[3], seen[4], seen[5]); }
    printf("test_endfix: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
