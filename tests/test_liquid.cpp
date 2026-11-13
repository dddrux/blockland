// Lava, balde e vazio: roda o main.cpp REAL (funcoes static), sem OpenGL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
}
static void run(int ticks, float dt = .25f) { for (int i = 0; i < ticks; i++) simulateWater(dt, 0, 0); }
int main() {
    WORLD_SEED = 99; arena();
    printf("[ids da lava]\n");
    CHECK(isLava(LAVA) && isLava(LAVA_FALL) && isLava(56) && !isLava(47) && !isLava(57) && !isWater(LAVA));
    for (int m = 1; m < 8; m++) CHECK(lavaMeta(lavaBlock(m)) == m);
    CHECK(!solid(LAVA) && !pickable(LAVA) && lightEmit(LAVA) == 15);
    printf("[lava espalha 3 blocos (decai 2) e cai]\n");
    setB(5, AY, 5, LAVA); run(40, 1.6f);
    CHECK(lavaMeta(getB(6, AY, 5)) == 2 && lavaMeta(getB(7, AY, 5)) == 4 && lavaMeta(getB(8, AY, 5)) == 6 && getB(9, AY, 5) == AIR);
    arena(); setB(5, AY + 3, 5, LAVA); run(40, 1.6f);
    CHECK(getB(5, AY + 2, 5) == LAVA_FALL && getB(5, AY, 5) == LAVA_FALL);
    printf("[lava nao tem fonte infinita]\n");
    arena(); setB(3, AY, 3, LAVA); setB(5, AY, 3, LAVA); setB(4, AY, 3, LAVA_FLOW_MIN + 5); run(40, 1.6f);
    CHECK(getB(4, AY, 3) != LAVA);
    printf("[agua + lava]\n");
    arena(); setB(5, AY, 5, LAVA); setB(5, AY + 1, 5, WATER); run(20, 1.6f);
    CHECK(getB(5, AY, 5) == LAVA || getB(5, AY, 5) == OBSIDIAN);   // agua em cima: so lateral/acima conta -> acima conta
    arena(); setB(5, AY, 5, LAVA); setB(6, AY, 5, WATER); run(20, 1.6f);
    CHECK(getB(5, AY, 5) == OBSIDIAN);                              // fonte + agua ao lado = obsidiana
    arena(); setB(5, AY, 5, LAVA_FLOW_MIN + 4); setB(6, AY, 5, WATER); run(20, 1.6f);   // meta 3
    CHECK(getB(5, AY, 5) == COBBLE || getB(5, AY, 5) == AIR);
    arena(); setB(5, AY, 5, WATER); setB(5, AY + 1, 5, LAVA); run(40, 1.6f);
    CHECK(getB(5, AY, 5) != LAVA);                                  // agua nunca e substituida por lava de cima sem virar pedra
    printf("[mistura imediata (sem tick)]\n");
    arena(); setB(5, AY, 5, LAVA); setB(6, AY, 5, WATER);
    CHECK(getB(5, AY, 5) == OBSIDIAN);                              // fonte + agua ao lado: obsidiana na hora
    arena(); setB(5, AY, 5, LAVA); setB(5, AY + 1, 5, WATER);
    CHECK(getB(5, AY, 5) == OBSIDIAN);                              // agua em cima conta
    arena(); setB(5, AY, 5, WATER); setB(5, AY + 1, 5, LAVA);
    CHECK(getB(5, AY + 1, 5) == LAVA);                              // agua EMBAIXO da lava nao endurece a lava...
    run(40, 1.6f); CHECK(getB(5, AY, 5) == STONE);                  // ...mas a lava que cai nela vira pedra
    arena(); setB(6, AY, 5, WATER); setB(5, AY, 5, LAVA_FLOW_MIN + 5);   // meta 2
    CHECK(getB(5, AY, 5) == COBBLE);                                // fluxo curto colocado ao lado de agua: pedregulho
    arena(); setB(6, AY, 5, WATER); setB(5, AY, 5, LAVA_FLOW_MIN);   // meta 7 (>4)
    CHECK(isLava(getB(5, AY, 5)));                                  // fluxo longo continua lava
    arena(); setB(5, AY, 5, WATER); setB(6, AY, 5, LAVA); run(40, .3f);
    CHECK(getB(6, AY, 5) == OBSIDIAN && getB(5, AY, 5) == WATER);   // agua nunca invade lava
    printf("[agua continua igual]\n");
    arena(); setB(5, AY, 5, WATER); run(40, .3f);
    CHECK(waterLevel(getB(6, AY, 5)) > 0 && getB(5 + 8, AY, 5) == AIR);
    printf("[fisica na lava]\n");
    arena(); setB(5, AY, 5, LAVA); setB(5, AY + 1, 5, LAVA);
    Phys P; P.x = 5.5; P.y = AY + .1; P.z = 5.5; P.inLava = inLavaNow(P);
    CHECK(P.inLava);
    P.x = 12.5; CHECK(!inLavaNow(P));
    fflush(stdout); printf("[balde]\n");
    gBktEmpty = 3001; gBktWater = 3002; gBktLava = 3003; creative = false;
    arena(); setB(5, AY, 5, LAVA);   // (agua corrente so depois: ao lado da fonte ela viraria obsidiana, como no Minecraft)
    float eye[3] = {5.5f, AY + 3.5f, 5.5f}, dir[3] = {0, -1, 0};
    Slot s; s.id = gBktEmpty; s.n = 1;
    CHECK(bucketUse(s, eye, dir) && s.id == gBktLava && getB(5, AY, 5) == AIR);       // pega a fonte de lava
    s.id = gBktLava; CHECK(bucketUse(s, eye, dir));                                   // coloca sobre o piso de pedra
    CHECK(getB(5, AY, 5) == LAVA && s.id == gBktEmpty);
    setB(5, AY, 5, AIR); setB(6, AY, 5, WATER_FALL);
    s.id = gBktEmpty; s.n = 3; eye[0] = 6.5f; dir[0] = 0; eye[0] = 6.5f;
    CHECK(bucketUse(s, eye, dir) && getB(6, AY, 5) == WATER_FALL && s.id == gBktEmpty && s.n == 3);   // agua corrente NAO enche
    arena(); setB(7, AY, 7, WATER); eye[0] = 7.5f; eye[2] = 7.5f;
    s.id = gBktEmpty; s.n = 3; CHECK(bucketUse(s, eye, dir) && getB(7, AY, 7) == AIR && s.n == 2);   // pilha: 1 vira cheio
    creative = true; arena(); setB(7, AY, 7, WATER); s.id = gBktEmpty; s.n = 1;
    CHECK(bucketUse(s, eye, dir) && getB(7, AY, 7) == AIR && s.id == gBktEmpty);       // criativo nao consome
    creative = false;
    printf("[vazio]\n");
    Phys V; V.y = -70; V.py = -69; V.onGround = false; gHP = 20; gDead = false; gInvul = 0; creative = false;
    survTick(V); CHECK(gHP <= 16);
    for (int i = 0; i < 6; i++) survTick(V);
    CHECK(gDead);
    gDead = false; gHP = 20; creative = true; survTick(V); CHECK(gHP == 20 && !gDead); creative = false;
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
