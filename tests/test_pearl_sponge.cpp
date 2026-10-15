// patch5.62: perola do Ender (teleporta o dono ao ponto de impacto, 5 de dano) e esponja (absorve agua). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
static void clearArena() { for (int z = -8; z < 16; z++) for (int x = -8; x < 16; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 6; y++) setB(x, y, z, AIR); } }
static int waterCount() { int n = 0; for (int y = AY; y < AY + 4; y++) for (int z = -8; z < 16; z++) for (int x = -8; x < 16; x++) if (isWater(getB(x, y, z))) n++; return n; }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    // ---- perola ----
    clearArena(); for (int x = -4; x < 12; x++) for (int y = AY; y < AY + 5; y++) setB(x, y, -2, STONE);   // parede em z=-2
    gPearlId = IT_BOW;   // sem pack nos testes: usa um id de item qualquer como "perola"
    Phys P; P.x = 4.5; P.y = AY; P.z = 4.5; gHP = 20; gDead = false; creative = false; gInvul = 0; gFall = 3.f;
    Slot s; s.id = IT_BOW; s.n = 2; int cd = 0;
    CHECK(eggThrow(P, 0.f, 0.f, s, cd)); CHECK(s.n == 1 && EGGS.size() == 1 && EGGS[0].kind == 1);
    for (int i = 0; i < 40 && !gPearlTp.ok; i++) eggsTick(P);
    CHECK(gPearlTp.ok && EGGS.empty());
    pearlApply(P);
    CHECK(P.z < 0.5 && P.z > -1.5 && P.x > 3 && P.x < 6);   // chegou perto da parede
    CHECK(gHP == 15);   // 5 de dano
    CHECK(gFall == 0.f); CHECK(!gPearlTp.ok);
    // criativo: sem dano; egg normal continua sendo egg
    gHP = 20; creative = true; P.x = 4.5; P.z = 4.5; s.n = 2; cd = 0;
    eggThrow(P, 0.f, 0.f, s, cd); for (int i = 0; i < 40 && !gPearlTp.ok; i++) eggsTick(P); pearlApply(P); CHECK(gHP == 20 && P.z < 0.5);
    creative = false; Slot e; e.id = IT_EGG; e.n = 1; cd = 0; CHECK(eggThrow(P, 0.f, 0.f, e, cd) && EGGS.back().kind == 0); EGGS.clear();
    // ---- esponja ----
    gPkByFile["block/sponge"] = 1100; gPkByFile["block/wet_sponge"] = 1101; spongeHooksInit();
    CHECK(gSpongeId == 1100 && gWetSpongeId == 1101);
    clearArena(); setB(5, AY, 5, 1100); CHECK(getB(5, AY, 5) == 1100);   // sem agua: continua seca
    clearArena(); for (int z = 4; z <= 6; z++) for (int x = 4; x <= 6; x++) if (x != 5 || z != 5) setB(x, AY, z, WATER);
    CHECK(waterCount() == 8); setB(5, AY, 5, 1100);   // colocar a esponja absorve (gancho added)
    CHECK(waterCount() == 0 && getB(5, AY, 5) == 1101);   // piscina pequena: toda absorvida, vira molhada
    clearArena(); for (int z = 0; z <= 11; z++) for (int x = 0; x <= 11; x++) for (int y = AY; y < AY + 2; y++) setB(x, y, z, WATER);
    const int before = waterCount(); setB(5, AY, 5, 1100);
    CHECK(before - waterCount() == 66 && getB(5, AY, 5) == 1101);   // 65 absorvidos + a celula que a esponja ocupou (era agua)
    clearArena(); setB(5, AY, 5, 1100); CHECK(getB(5, AY, 5) == 1100);
    setB(6, AY, 5, WATER);   // agua encostando depois: o gancho neighbor aciona
    CHECK(getB(5, AY, 5) == 1101 && !isWater(getB(6, AY, 5)));
    clearArena(); setB(5, AY, 5, 1100); setB(5, AY + 3, 5, WATER); CHECK(getB(5, AY, 5) == 1100);   // agua longe nao aciona
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
