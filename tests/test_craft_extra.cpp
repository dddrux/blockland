// patchCraft.3: receitas extras (craft_extra.inc) + qualquer madeira (craftNorm). Pack FALSO (so nomes) — nao precisa do resource pack.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int add(const char* f) { PkItem e; e.file = f; e.name = f; e.fold = f; e.cube = false; PK.push_back(e); return PK0 + (int)PK.size() - 1; }
static int cr(std::vector<int> g, int& n, int w = 3) { int a[9] = {0}; for (size_t i = 0; i < g.size() && i < 9; i++) a[i] = g[i]; return craftResult(a, w, w, n); }
int main() {
    PK.clear(); gPkByFile.clear();
    const int spPl = add("block/spruce_planks"), spLog = add("block/spruce_log"), biPl = add("block/planks_birch" /* 1.12 */), wheat = add("item/wheat"), cocoa = add("item/cocoa_beans"), gun = add("item/gunpowder"),
        tnt = add("block/tnt"), bowl = add("item/bowl"), glass = add("block/glass"), bottle = add("item/glass_bottle"), blzr = add("item/blaze_rod"), bstand = add("item/brewing_stand"),
        cane = add("item/sugar_cane"), sugar = add("item/sugar"), cakeI = add("item/cake"), dyeRed = add("item/red_dye"), wlRed = add("block/red_wool"), nug = add("item/gold_nugget"),
        carrotG = add("item/golden_carrot"), pBlk = add("block/pumpkin"), shears = add("item/shears"), bm = add("item/bone_meal"), boneBlk = add("block/bone_block"), dyeOld = add("item/dye_powder_blue");
    gBktMilk = add("item/milk_bucket"); gBktEmpty = add("item/bucket");
    gRsDrop[XB_DECOR - XB_DUST][1] = cakeI; gRsDrop[XB_PIST - XB_DUST][0] = 777; int n = 0, o;
    gRsDrop[XB_CARPET - XB_DUST][14] = 888;
    // madeira
    o = cr({spLog}, n); CHECK(o == spPl && n == 4);
    o = cr({spPl, 0, 0, spPl}, n); CHECK(o == IT_STICK && n == 4);               // palito de abeto
    o = cr({biPl, 0, 0, biPl}, n); CHECK(o == IT_STICK && n == 4);               // palito de betula (nome 1.12)
    o = cr({spPl, biPl, spPl, biPl}, n, 2); CHECK(o == CTABLE);                  // mesa com madeiras misturadas
    o = cr({spPl, spPl, spPl, 0, IT_STICK, 0, 0, IT_STICK, 0}, n); CHECK(o == IT_WPICK);   // picareta de madeira com tabuas de abeto
    o = cr({PLANKS, 0, 0, PLANKS}, n); CHECK(o == IT_STICK && n == 4);           // oak continua igual
    o = cr({PLANKS, 0, 0, 0, PLANKS}, n); CHECK(o == 0);                          // diagonal nao e palito
    // comida
    o = cr({wheat, wheat, wheat}, n); CHECK(o == IT_BREAD && n == 1);
    o = cr({wheat, cocoa, wheat}, n); CHECK(o == IT_COOKIE && n == 8);
    o = cr({cane}, n, 2); CHECK(o == sugar && n == 1);
    o = cr({cane, cane, cane}, n); CHECK(o == IT_PAPER && n == 3);
    o = cr({gBktMilk, gBktMilk, gBktMilk, sugar, IT_EGG, sugar, wheat, wheat, wheat}, n); CHECK(o == cakeI && n == 1);
    o = cr({nug, nug, nug, nug, IT_CARROT, nug, nug, nug, nug}, n); CHECK(o == carrotG);
    o = cr({wheat, wheat, wheat, wheat, wheat, wheat, wheat, wheat, wheat}, n); CHECK(o == 0);   // hay_block nao existe no pack falso: pulado
    o = cr({IT_BONE}, n, 2); CHECK(o == bm && n == 3);
    o = cr({bm, bm, bm, bm, bm, bm, bm, bm, bm}, n); CHECK(o == boneBlk);
    // itens que dependem de ingrediente que o pack nao tem: pulados, nunca "vazio"
    o = cr({IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER, IT_PAPER}, n); CHECK(o == 0);
    // bloco / mecanismos
    o = cr({gun, SAND, gun, SAND, gun, SAND, gun, SAND, gun}, n); CHECK(o == tnt);
    o = cr({PLANKS, PLANKS, PLANKS, COBBLE, IT_IRON, COBBLE, COBBLE, IT_REDSTONE, COBBLE}, n); CHECK(o == 777);   // pistao
    o = cr({glass, 0, glass, 0, glass, 0}, n); CHECK(o == bottle && n == 3);
    o = cr({0, blzr, 0, COBBLE, COBBLE, COBBLE}, n); CHECK(o == bstand);
    o = cr({0, IT_IRON, IT_IRON, 0}, n, 2); CHECK(o == shears);   // tesoura = 2 ferros na diagonal (espelhada aceita)
    o = cr({IT_IRON, IT_IRON, 0, 0}, n, 2); CHECK(o != shears);    // lado a lado nao e tesoura
    // cores
    o = cr({WOOL, dyeRed}, n, 2); CHECK(o == wlRed);
    o = cr({wlRed, wlRed}, n, 2); CHECK(o == 888 && n == 3);
    // consumir: balde de leite volta vazio
    craftG[0] = {gBktMilk, 1, 0}; craftG[1] = {sugar, 2, 0}; consumeCraft(); CHECK(craftG[0].id == gBktEmpty && craftG[1].n == 1);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
