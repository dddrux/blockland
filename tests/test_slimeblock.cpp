// patchSlime.3: bloco de slime: mob/item quicam e sem dano de queda; pistao empurra/puxa blocos grudados. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
int main() {
    PK.resize(1); PK[0].file = "block/slime_block"; PK[0].name = "block/slime_block"; PK[0].tile = 10; PK[0].cube = true; gPkByFile["block/slime_block"] = PK0;
    std::error_code ec; fs::create_directories("/tmp/bl_sb/saves/t", ec); if (chdir("/tmp/bl_sb") != 0) return 2;
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY - 1; y < AY + 40; y++) setB(x, y, z, y < AY ? STONE : AIR);
    gDead = false; creative = false; gGm = 0; gInvul = 0;
    // id do bloco de slime: se o pack nao esta carregado nos testes, registra um id de teste
    int SL = slimeBlkId();
    CHECK(SL == PK0);
    SECTION("mob: sem dano de queda e quica");
    { MOBS.clear(); setB(0, AY - 1, 0, SL); spawnMob(M_COW, 0.5, AY + 20, 0.5, 0.f); MobEnt& e0 = MOBS[0]; (void)e0; const int hp0 = MOBS[0].hp; double maxY = 0; bool bounced = false;
      for (int i = 0; i < 120; i++) { tickMob(MOBS[0], 1000, AY, 1000); if (MOBS[0].y > AY + 1.0 && i > 25) bounced = true; maxY = std::max(maxY, MOBS[0].y); }
      CHECK(MOBS[0].hp == hp0); CHECK(bounced); }
    SECTION("mob: chao comum causa dano");
    { MOBS.clear(); spawnMob(M_COW, 5.5, AY + 20, 5.5, 0.f); const int hp0 = MOBS[0].hp; for (int i = 0; i < 80; i++) tickMob(MOBS[0], 1000, AY, 1000); CHECK(MOBS[0].hp < hp0 || MOBS[0].dead); }
    SECTION("item: quica no slime, nao no chao");
    { DROPS.clear(); setB(10, AY - 1, 10, SL); spawnDrop(10.5, AY + 6, 10.5, 0, 0, 0, STONE, 1, 100000); spawnDrop(15.5, AY + 6, 15.5, 0, 0, 0, STONE, 1, 100000);
      double maxA = 0, maxB = 0; bool a_fell = false;
      for (int i = 0; i < 70; i++) { tickDrops(10.5, AY + 50, 10.5); if (i > 20) { for (const DropEnt& d : DROPS) { if (fabs(d.x - 10.5) < 1) maxA = std::max(maxA, d.y); else maxB = std::max(maxB, d.y); } } }
      CHECK(maxA > AY + 1.5); CHECK(maxB < AY + .5); (void)a_fell; }
    SECTION("pistao empurra slime com blocos grudados");
    { const int YY = AY + 6; auto clr = [&]() { for (int x = -4; x < 22; x++) for (int y = YY - 2; y < YY + 4; y++) for (int z = 0; z < 8; z++) setBM(x, y, z, AIR, 0); setBM(2, YY - 1, 1, STONE, 0); };
      const int LV = XB_LEVER + 12 + 4, LV0 = XB_LEVER + 4;
      clr(); setBM(2, YY, 2, XB_PIST, 3); setB(3, YY, 2, SL); setB(3, YY, 3, STONE); setB(3, YY + 1, 2, DIRT);
      setB(2, YY, 1, LV); pistonScan(2, YY, 1);
      CHECK(getM(2, YY, 2) == (16 | 3)); CHECK(getB(3, YY, 2) == XB_PIST); CHECK(getB(4, YY, 2) == SL); CHECK(getB(4, YY, 3) == STONE); CHECK(getB(4, YY + 1, 2) == DIRT);
      CHECK(getB(3, YY, 3) == AIR); CHECK(getB(3, YY + 1, 2) == AIR);
      // pistao normal nao puxa; mas o grudento puxa tudo de volta
      clr(); setBM(2, YY, 2, XB_PIST + 1, 3); setB(3, YY, 2, SL); setB(3, YY, 3, STONE); setB(3, YY + 1, 2, DIRT); setB(2, YY, 1, LV); pistonScan(2, YY, 1);
      CHECK(getB(4, YY, 2) == SL && getB(4, YY, 3) == STONE);
      setB(2, YY, 1, LV0); pistonScan(2, YY, 1);
      CHECK(getM(2, YY, 2) == 3); CHECK(getB(3, YY, 2) == SL); CHECK(getB(3, YY, 3) == STONE); CHECK(getB(3, YY + 1, 2) == DIRT); CHECK(getB(4, YY, 2) == AIR && getB(4, YY, 3) == AIR);
      // imovel grudado (obsidiana) nao vem junto e nao trava
      clr(); setBM(2, YY, 2, XB_PIST, 3); setB(3, YY, 2, SL); setB(3, YY, 3, OBSIDIAN); setB(2, YY, 1, LV); pistonScan(2, YY, 1);
      CHECK(getB(4, YY, 2) == SL); CHECK(getB(3, YY, 3) == OBSIDIAN);
      // grudado em obsidiana na linha: bloqueia
      clr(); setBM(2, YY, 2, XB_PIST, 3); setB(3, YY, 2, SL); setB(4, YY, 2, OBSIDIAN); setB(2, YY, 1, LV); pistonScan(2, YY, 1);
      CHECK(getM(2, YY, 2) == 3 && getB(3, YY, 2) == SL); }
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
