// patch5.76: afogamento e sufocamento. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
static void arena() { for (int z = -4; z < 4; z++) for (int x = -4; x < 4; x++) { setB(x, AY - 2, z, STONE); for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, AIR); } }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    arena(); netOn = false; creative = false; gDead = false; gHP = 20; gInvul = 0; gAir = 300; gFire = 0;
    Phys P; P.x = 0.5; P.z = 0.5; P.y = AY - 1; P.px = P.x; P.py = P.y; P.pz = P.z; P.mx = P.my = P.mz = 0; P.flying = false; P.onGround = true; P.inWater = P.inLava = false;
    // agua ate acima dos olhos (olhos em y = AY-1+1.62 = AY+0.62)
    for (int y = AY - 1; y <= AY + 2; y++) setB(0, y, 0, WATER);
    for (int t = 0; t < 299; t++) survTick(P);
    CHECK(gAir == 1 && gHP == 20);   // ainda ha ar, sem dano
    for (int t = 0; t < 21; t++) survTick(P);   // 300 -> 0 -> -20: 1o dano de afogamento
    CHECK(gHP == 18 && gAir == 0);
    gInvul = 0; for (int t = 0; t < 20; t++) survTick(P);   // mais 20 ticks: outro dano de 2
    CHECK(gHP == 16);
    // sai da agua: o ar enche
    for (int y = AY - 1; y <= AY + 2; y++) setB(0, y, 0, AIR);
    survTick(P); CHECK(gAir == 300);
    // criativo nao se afoga
    creative = true; gAir = 10; survTick(P); CHECK(gAir == 300 && gHP == 16); creative = false;
    // sufocamento: olhos dentro de pedra = 1 de dano; vidro/folha nao sufocam; ar nao
    gInvul = 0; gHP = 20; setB(0, AY + 0, 0, STONE); survTick(P); CHECK(gHP == 19);
    gInvul = 0; gHP = 20; setB(0, AY + 0, 0, AIR); survTick(P); CHECK(gHP == 20);
    // respawn devolve o ar
    gAir = 5; gDead = true; gWantRespawn = true; gHP = 0; survRespawn(P); CHECK(gAir == 300);
    // bolhas de ar do HUD: 300 = nenhuma estourada, 150 = 5 cheias, 20 = 1 cheia + 0 estouradas, 1 = 0 cheias + 1 estourada, 0 = nenhuma
    { int po = 0; CHECK(survBubbles(300, &po) == 10 && po == 0); CHECK(survBubbles(150, &po) == 5 && po == 0); CHECK(survBubbles(1, &po) == 0 && po == 1); CHECK(survBubbles(0, &po) == 0 && po == 0); CHECK(survBubbles(-10, &po) == 0 && po == 0); }
    // jogador 2 (split) tem o proprio ar: a troca de contexto leva gAir junto
    { gAir = 100; S2.sv.air = 300; split2SwapSurv(); CHECK(gAir == 300 && S2.sv.air == 100); split2SwapSurv(); CHECK(gAir == 100); }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
