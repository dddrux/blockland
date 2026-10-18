// Nether: terreno basico, portal (moldura -> acende -> quebra), viagem entre dimensoes. Roda o main.cpp REAL (sem pack).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    printf("[terreno]\n");
    gCurDir = "tn_test"; WORLD_SEED = 777; netherInit();
    gen(0, 0); gen(1, 0);                                  // superficie
    const int sb = getB(3, 70, 3);
    dimSwitch(1); CHECK(gDim == 1); CHECK(W.empty());
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) gen(cx, cz);
    long cnt[8] = {0}; // 0 bedrock 1 rack/stone 2 lava 3 air 4 glow 5 fire 6 other
    for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) for (int y = 0; y < 128; y++) { int b = getB(x, y, z); cnt[b == BEDROCK ? 0 : b == STONE ? 1 : isLava(b) ? 2 : b == AIR ? 3 : b == GLOWSTONE ? 4 : b == XB_FIRE ? 5 : 6]++; }
    printf("  bedrock=%ld stone(sem pack)=%ld lava=%ld ar=%ld glow=%ld fogo=%ld outros=%ld\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], cnt[5], cnt[6]);
    CHECK(cnt[0] > 1000); CHECK(cnt[1] > 5000); { long lv = 0; for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) for (int y = 0; y < 32; y++) lv += isLava(getB(cx * 16 + x, y, cz * 16 + z)); printf("  lava (25 chunks): %ld\n", lv); CHECK(lv > 100); } CHECK(cnt[3] > 5000); CHECK(getB(5, 127, 5) == BEDROCK); CHECK(getB(5, 0, 5) == BEDROCK);
    CHECK(getB(5, 200, 5) == AIR);
    { long gl = 0, fi = 0; for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) for (int y = 0; y < 128; y++) { int b = getB(cx * 16 + x, y, cz * 16 + z); gl += b == GLOWSTONE; fi += b == XB_FIRE; } printf("  25 chunks: glowstone=%ld fogo=%ld\n", gl, fi); CHECK(gl > 20); CHECK(fi > 3); }
    // sem skylight: topo bedrock => teto fechado em toda a coluna
    for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) CHECK(getB(x, 127, z) == BEDROCK);
    dimSwitch(0); CHECK(gDim == 0); CHECK(getB(3, 70, 3) == sb);   // superficie intacta apos ir e voltar
    dimSwitch(1);

    printf("[portal]\n");
    // arena em y=200 no nether (so teste): moldura 4x5 no plano X com interior 2x3
    gNoBlockHooks = true;
    for (int z = 38; z < 46; z++) for (int x = 38; x < 50; x++) for (int y = 199; y < 208; y++) setB(x, y, z, y == 199 ? STONE : AIR);
    for (int y = 199; y <= 203; y++) { setB(40, y, 42, OBSIDIAN); setB(43, y, 42, OBSIDIAN); }
    for (int x = 41; x <= 42; x++) { setB(x, 199, 42, OBSIDIAN); setB(x, 203, 42, OBSIDIAN); }
    gNoBlockHooks = false;
    int bx, by, bz, w, h;
    CHECK(portalFrame(41, 200, 42, 0, bx, by, bz, w, h)); CHECK(w == 2 && h == 3 && bx == 41 && by == 200 && bz == 42);
    CHECK(!portalFrame(41, 200, 42, 1, bx, by, bz, w, h));
    setBM(41, 200, 42, XB_FIRE, 0);                          // fogo dentro da moldura
    CHECK(tryLightPortal(41, 200, 42));
    for (int y = 200; y <= 202; y++) for (int x = 41; x <= 42; x++) CHECK(getB(x, y, 42) == XB_PORTAL && (getM(x, y, 42) & 1) == 0);
    CHECK(getB(41, 203, 42) == OBSIDIAN);
    CHECK(solid(XB_PORTAL) == false); CHECK(solid(XB_FIRE) == false);
    setB(40, 201, 42, AIR);                                  // quebrou a moldura: o portal inteiro some
    int left = 0; for (int y = 200; y <= 202; y++) for (int x = 41; x <= 42; x++) if (getB(x, y, 42) == XB_PORTAL) left++;
    printf("  portais restantes apos quebrar a moldura: %d\n", left); CHECK(left == 0);
    // moldura incompleta (sem o topo) nao acende
    setB(40, 201, 42, OBSIDIAN); setB(41, 203, 42, AIR);
    setBM(41, 200, 42, XB_FIRE, 0); CHECK(!tryLightPortal(41, 200, 42));

    printf("[fogo]\n");
    setB(41, 203, 42, OBSIDIAN);
    setBM(45, 200, 42, XB_FIRE, 0); CHECK(getB(45, 200, 42) == XB_FIRE);
    setB(45, 199, 42, AIR);                                  // perdeu o suporte: apaga
    CHECK(getB(45, 200, 42) == AIR);

    printf("[viagem]\n");
    dimSwitch(0);
    Phys P{}; P.x = 80.5; P.z = 160.5; P.y = 70;
    netherTravel(P);
    CHECK(gDim == 1); CHECK(fabs(P.x - 10.0) < 3 && fabs(P.z - 20.0) < 3);   // 80/8, 160/8
    int px, py, pz, ax; CHECK(findPortal((int)P.x, (int)P.z, px, py, pz, ax));
    CHECK(getB(px, py, pz) == XB_PORTAL); CHECK(!hit((float)P.x, (float)P.y, (float)P.z));   // jogador nao nasce dentro de bloco
    netherTravel(P);                                          // volta: usa/cria portal na superficie
    CHECK(gDim == 0); CHECK(fabs(P.x - 80) < 20 && fabs(P.z - 172) < 20); CHECK(findPortal((int)P.x, (int)P.z, px, py, pz, ax)); CHECK(!hit((float)P.x, (float)P.y, (float)P.z));

    printf("[v2: portal, baus, rede, luz]\n");
    dimSwitch(0);
    { Phys Q{}; Q.x = 80.5; Q.z = 160.5; Q.y = 70; netherTravel(Q);   // o jogador sai DENTRO do portal, centralizado na largura (nao na frente dele)
      CHECK(getB((int)floor(Q.x), (int)floor(Q.y), (int)floor(Q.z)) == XB_PORTAL);
      int qx, qy, qz, qa; CHECK(findPortal((int)floor(Q.x), (int)floor(Q.z), qx, qy, qz, qa));
      const double fz = qa == 0 ? Q.z - floor(Q.z) : Q.x - floor(Q.x); CHECK(fabs(fz - .5) < .01); }   // meio da espessura do plano
    dimSwitch(0);
    { if (!getC(2, 2)) gen(2, 2); setBM(40, 70, 40, XB_CHEST, 0); ChestInv* a = chestAt(40, 70, 40, true); CHECK(a); a->s[0].id = STONE; a->s[0].n = 3;   // bau na superficie
      dimSwitch(1); if (!getC(2, 2)) gen(2, 2); setBM(40, 70, 40, XB_CHEST, 0); ChestInv* b = chestAt(40, 70, 40, false); CHECK(b == nullptr);   // mesma coordenada no Nether: outro bau
      chestAt(40, 70, 40, true)->s[0].id = GLOWSTONE; dimSwitch(0);
      CHECK(chestAt(40, 70, 40, false) && chestAt(40, 70, 40, false)->s[0].id == STONE); }
    { const bool on = netOn; netOn = false; dimSwitch(1); netSendBlock(3, 50, 3, OBSIDIAN, 0);   // edicao feita no Nether viaja com a dimensao em y
      dimSwitch(0); gen(0, 0); netApplyEdit(3, 50 + (1 << 10), 3, GLOWSTONE, 0);             // edicao remota do Nether nao pode aparecer na superficie
      CHECK(getB(3, 50, 3) != GLOWSTONE); netOn = on; }
    dimSwitch(1); for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    { Chunk* c = getC(0, 0); stream(0, 0, 0, 50); bool ambOk = true; for (int y = 40; y < 100 && ambOk; y++) if (c->b[idx(5, y, 5)] == AIR) ambOk = (c->lc[idx(5, y, 5)] & 15) >= NETHER_AMB; CHECK(ambOk); }   // luz ambiente minima
    dimSwitch(0);

    printf("[salvar/carregar]\n");
    system("rm -rf saves/tn_test && mkdir -p saves/tn_test");
    dimSwitch(1); setB(5, 60, 5, OBSIDIAN); gen(0, 0); setB(5, 60, 5, OBSIDIAN);
    saveCurWorld(5.5, 61, 5.5);
    CHECK(access("saves/tn_test/chunks16.dat", F_OK) == 0); CHECK(access("saves/tn_test/chunks16n.dat", F_OK) == 0);
    resetWorldData(); CHECK(gDim == 0 && W.empty());
    gCurDir = "tn_test"; loadSavedChunks(gCurDir); netherOnWorldLoad(gCurDir);
    CHECK(gDim == 1); gen(0, 0); CHECK(getB(5, 60, 5) == OBSIDIAN); CHECK(getB(5, 127, 5) == BEDROCK);
    dimSwitch(0); gen(0, 0); CHECK(getB(3, 70, 3) == sb);
    system("rm -rf saves/tn_test");
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
