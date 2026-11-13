// Bigorna (patchAnvil.1): reparo com material, combinar itens, encantamentos, custo em niveis, retirada.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int lvlTotal(int L) { int t = 0; for (int k = 0; k < L; k++) t += xpToNext(k); return t; }
int main() {
    if (chdir("..") != 0) return 99;
    gDead = false; gGm = 0; creative = false; gSndOn = false; cur = Slot();
    for (Slot& s : gAnv) s = Slot(); for (Slot& s : inv) s = Slot();
    // reparo: picareta de ferro (250) gasta 200: 1 lingote = 62 de durabilidade (250/4)
    gAnv[0].id = IT_IPICK; gAnv[0].n = 1; gAnv[0].d = 200; gAnv[1].id = IT_IRON; gAnv[1].n = 3; anvCompute();
    CHECK(gAnv[2].id == IT_IPICK && gAnv[2].d == 14 && gAnvUsed == 3 && gAnvCost == 3 && gAnvMat);   // 200 -> 138 -> 76 -> 14 com 3 lingotes
    gAnv[1].id = IT_GOLD; anvCompute(); CHECK(!gAnv[2].id);   // material errado
    gAnv[1].id = IT_IRON; gAnv[1].n = 1; anvCompute(); CHECK(gAnv[2].d == 138 && gAnvUsed == 1 && gAnvCost == 1);
    // retirar: precisa de nivel
    gXpTot = lvlTotal(0); anvCompute(); CHECK(!gAnvOk); anvTakeOut(0, false); CHECK(!cur.id && gAnv[0].id);
    gXpTot = lvlTotal(5) + 3; anvCompute(); CHECK(gAnvOk); anvTakeOut(0, false);
    CHECK(cur.id == IT_IPICK && cur.d == 138 && !gAnv[0].id && !gAnv[1].id && xpLevel(gXpTot) == 4);
    // cursor ocupado nao retira
    cur = Slot(); for (Slot& s : gAnv) s = Slot();
    gAnv[0].id = IT_IPICK; gAnv[0].n = 1; gAnv[0].d = 100; gAnv[1] = gAnv[0]; gAnv[1].d = 100; anvCompute();
    CHECK(gAnv[2].id == IT_IPICK && gAnv[2].d == 0 && gAnvCost == 2 && !gAnvMat);   // 150+150+30 >= 250 => durabilidade cheia
    // juntar encantamentos: espada afiacao III + afiacao III = IV (custo 4 * 1 + durabilidade 0 => 4)
    for (Slot& s : gAnv) s = Slot();
    gAnv[0].id = IT_DSWORD; gAnv[0].n = 1; enchAdd(gAnv[0], 16, 3); gAnv[1] = gAnv[0]; anvCompute();
    CHECK(gAnv[2].id == IT_DSWORD && enchLv(gAnv[2], 16) == 4 && gAnvCost == 4);
    // conflito: afiacao + julgamento nao junta; item errado nao aceita
    gAnv[1].nbt.clear(); enchAdd(gAnv[1], 17, 2); anvCompute(); CHECK(!gAnv[2].id || enchLv(gAnv[2], 17) == 0);
    // caro demais (>= 40) no survival; criativo e gratis
    for (Slot& s : gAnv) s = Slot(); gAnv[0].id = IT_DSWORD; gAnv[0].n = 1; gAnv[1] = gAnv[0];
    for (int e : {16, 21, 20, 19, 22, 34}) { enchAdd(gAnv[0], e, 10, true); enchAdd(gAnv[1], e, 10, true); }
    anvCompute(); CHECK(gAnvTooExp && !gAnv[2].id); creative = true; anvCompute(); CHECK(gAnv[2].id && gAnvOk); creative = false;
    // patchAnvil.2: trabalho anterior: 1a vez +0, resultado leva ~w1; 2a vez +1; 3a +3; enchantment preservado
    for (Slot& s : gAnv) s = Slot(); gAnv[0].id = IT_IPICK; gAnv[0].n = 1; gAnv[0].d = 200; gAnv[1].id = IT_IRON; gAnv[1].n = 1; anvCompute();
    CHECK(gAnvCost == 1 && enchWork(gAnv[2].nbt) == 1);
    gAnv[0] = gAnv[2]; gAnv[0].d = 200; anvCompute(); CHECK(gAnvCost == 2 && enchWork(gAnv[2].nbt) == 2);
    gAnv[0] = gAnv[2]; gAnv[0].d = 200; anvCompute(); CHECK(gAnvCost == 4 && enchWork(gAnv[2].nbt) == 3);
    for (Slot& s : gAnv) s = Slot(); gAnv[0].id = IT_DSWORD; gAnv[0].n = 1; enchAdd(gAnv[0], 16, 3); gAnv[0].nbt = enchWithWork(gAnv[0].nbt, 2); gAnv[1].id = IT_DSWORD; gAnv[1].n = 1; enchAdd(gAnv[1], 16, 3);
    anvCompute(); CHECK(enchLv(gAnv[2], 16) == 4 && gAnvCost == 4 + 3 && enchWork(gAnv[2].nbt) == 3);
    // ids de abertura: so bloco de bigorna
    printf("test_anvil: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
