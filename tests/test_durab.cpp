// Durabilidade (patchDur.1): maximos, desgaste por uso, quebra, criativo, armadura, addItem com dano.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    gDead = false; gGm = 0; creative = false; gSndOn = false;
    CHECK(itemMaxDur(IT_WPICK) == 59 && itemMaxDur(IT_SPICK) == 131 && itemMaxDur(IT_IPICK) == 250 && itemMaxDur(IT_GPICK) == 32 && itemMaxDur(IT_DPICK) == 1561);
    CHECK(itemMaxDur(IT_WSWORD) == 59 && itemMaxDur(IT_DSWORD) == 1561 && itemMaxDur(IT_DAXE) == 1561 && itemMaxDur(IT_SSHOVEL) == 131 && itemMaxDur(IT_GHOE) == 32 && itemMaxDur(IT_BOW) == 384);
    CHECK(itemMaxDur(IT_A0) == 55 && itemMaxDur(IT_A0 + 7) == 195 && itemMaxDur(IT_A0 + 9) == 112 && itemMaxDur(IT_A0 + 15) == 429 && itemMaxDur(STONE) == 0 && itemMaxDur(IT_APPLE) == 0);
    Slot s; s.id = IT_WPICK; s.n = 1;
    toolWear(s); CHECK(s.id == IT_WPICK && s.d == 1);
    for (int i = 0; i < 57; i++) toolWear(s); CHECK(s.id == IT_WPICK && s.d == 58);
    toolWear(s); CHECK(s.id == 0 && s.n == 0 && s.d == 0);   // quebrou
    s = Slot(); s.id = IT_ISWORD; s.n = 1; toolWear(s); CHECK(s.d == 2);   // espada: 2 ao quebrar bloco
    meleeWear(s); CHECK(s.d == 3);                                         // espada: 1 ao bater
    s = Slot(); s.id = IT_IAXE; s.n = 1; meleeWear(s); CHECK(s.d == 2); toolWear(s); CHECK(s.d == 3);
    s = Slot(); s.id = STONE; s.n = 5; toolWear(s); meleeWear(s); CHECK(s.d == 0 && s.n == 5);       // bloco nao gasta
    s = Slot(); s.id = IT_DPICK; s.n = 1; creative = true; toolWear(s); meleeWear(s); CHECK(s.d == 0); creative = false;   // criativo nao gasta
    // armadura: peca perde max(1, dano/4); peitoral de couro (80) quebra
    for (Slot& a : armor) a = Slot(); gInvul = 0; gHP = 20;
    armor[1].id = IT_A0 + 1; armor[1].n = 1; armor[0].id = IT_A0 + 4; armor[0].n = 1;
    CHECK(survDamage(3, "teste")); CHECK(armor[1].d == 1 && armor[0].d == 1);
    gInvul = 0; gHP = 20; CHECK(survDamage(10, "teste")); CHECK(armor[1].d == 3 && armor[0].d == 3);
    gInvul = 0; gHP = 20; survDamage(4, "fome", true); CHECK(armor[1].d == 3);   // dano forcado nao gasta armadura
    armor[1].d = 79; gInvul = 0; gHP = 20; survDamage(3, "teste"); CHECK(armor[1].id == 0);
    // addItem preserva dano
    for (Slot& x : inv) x = Slot(); CHECK(addItem(IT_IPICK, 1, std::string(), 77) == 0 && inv[0].d == 77);
    printf("[INFO]: %d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
