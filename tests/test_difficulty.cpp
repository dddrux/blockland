// patchDif.1: dificuldade 1.12.2: dano de mob (Facil/Normal/Dificil), inanicao (piso 10/1/0) e regeneracao do Pacifico
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
static Phys mk() { Phys P; P.x = 0.5; P.z = 0.5; P.y = AY - 1; P.px = P.x; P.py = P.y; P.pz = P.z; P.mx = P.my = P.mz = 0; P.flying = false; P.onGround = true; P.inWater = P.inLava = false; return P; }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    arena(); netOn = false; creative = false; gDead = false; gInvul = 0; gFire = 0; gAir = 300;
    // dano de mob: 1.12.2 applyDifficulty
    gDifficulty = 1; CHECK(diffDmg(2) == 2); CHECK(diffDmg(3) == 2); CHECK(diffDmg(6) == 4); CHECK(diffDmg(1) == 1);
    gDifficulty = 2; CHECK(diffDmg(3) == 3);
    gDifficulty = 3; CHECK(diffDmg(2) == 3); CHECK(diffDmg(4) == 6);
    gDifficulty = 0; CHECK(diffDmg(5) == 0);
    // mobDmg: Pacifico nao fere; Dificil = 1,5x
    gMobVictim = -1; gHP = 20; gInvul = 0; gDifficulty = 0; CHECK(!mobDmg(4, "Zumbi", 0, 0) && gHP == 20);
    gDifficulty = 3; gHP = 20; gInvul = 0; CHECK(mobDmg(4, "Zumbi", 0, 0)); CHECK(gHP == 14);
    gDifficulty = 1; gHP = 20; gInvul = 0; CHECK(mobDmg(4, "Zumbi", 0, 0)); CHECK(gHP == 17);
    // inanicao: piso por dificuldade
    Phys P = mk();
    gDifficulty = 2; gHP = 6; gFood = 0; gSat = 0; gRegenT = 0; for (int t = 0; t < 800; t++) { gInvul = 0; survTick(P); } CHECK(gHP == 1 && !gDead);
    gDifficulty = 1; gHP = 20; gFood = 0; gRegenT = 0; for (int t = 0; t < 1200; t++) { gInvul = 0; survTick(P); } CHECK(gHP == 10);
    gDifficulty = 3; gHP = 3; gFood = 0; gRegenT = 0; for (int t = 0; t < 400; t++) { gInvul = 0; if (gDead) break; survTick(P); } CHECK(gDead || gHP == 0);
    // Pacifico: regenera 1 vida/s e enche a fome sem comer
    gDead = false; gDifficulty = 0; gHP = 10; gFood = 5; gRegenT = 0; for (int t = 0; t < 100; t++) survTick(P);
    CHECK(gHP >= 14 && gHP <= 16); CHECK(gFood == 15 || gFood == 20);
    printf("test_difficulty: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
