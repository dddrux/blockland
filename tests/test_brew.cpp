// patchBrew.1: pocoes (tabela/receitas de preparo) + cervejaria (tempo, combustivel, salvar). Pack FALSO, sem texturas.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int add(const char* f) { PkItem e; e.file = f; e.name = f; e.fold = f; e.cube = false; PK.push_back(e); return PK0 + (int)PK.size() - 1; }
int main() {
    PK.clear(); gPkByFile.clear(); gBrews.clear();
    const int wart = add("item/nether_wart"), glow = add("item/glowstone_dust"), gun = add("item/gunpowder"), dragon = add("item/dragon_breath"), fse = add("item/fermented_spider_eye"),
        carrot = add("item/golden_carrot"), magma = add("item/magma_cream"), foot = add("item/rabbit_foot"), sugar = add("item/sugar"), puff = add("item/fish_pufferfish_raw"),
        melon = add("item/speckled_melon"), seye = add("item/spider_eye"), tear = add("item/ghast_tear"), blaze = add("item/blaze_powder"), bottle = add("item/glass_bottle");
    const char* FN[3] = {"drink", "splash", "linger"};
    for (int f = 0; f < 3; f++) for (int t = 0; t < POT_N; t++) { PkItem e; e.file = std::string("potion/") + FN[f] + "_" + POT_T[t].key; e.name = potName(f, POT_T[t]); e.fold = e.name; e.cube = false; e.stack = 1; PK.push_back(e); const int id = PK0 + (int)PK.size() - 1; gPotId[f][t] = id; gPotOf[id] = {f, t}; }
    const PotIng& I = potIngs(); CHECK(I.wart == wart && I.blaze == blaze && I.bottle == bottle && I.dragon == dragon && I.fse == fse && I.red == IT_REDSTONE);
    // nomes unicos
    { std::set<std::string> nm; for (int f = 0; f < 3; f++) for (int t = 0; t < POT_N; t++) nm.insert(PK[gPotId[f][t] - PK0].name); CHECK((int)nm.size() == 3 * POT_N); }
    const int water = potItem(0, "water");
    CHECK(water && potBrew(water, wart) == potItem(0, "awkward") && potBrew(water, glow) == potItem(0, "thick") && potBrew(water, IT_REDSTONE) == potItem(0, "mundane"));
    CHECK(potBrew(water, fse) == potItem(0, "weakness") && potBrew(water, sugar) == potItem(0, "mundane") && potBrew(water, carrot) == 0);
    const int awk = potItem(0, "awkward");
    CHECK(potBrew(awk, carrot) == potItem(0, "night_vision") && potBrew(awk, magma) == potItem(0, "fire_resistance") && potBrew(awk, foot) == potItem(0, "leaping") && potBrew(awk, sugar) == potItem(0, "swiftness"));
    CHECK(potBrew(awk, puff) == potItem(0, "water_breathing") && potBrew(awk, melon) == potItem(0, "healing") && potBrew(awk, seye) == potItem(0, "poison") && potBrew(awk, tear) == potItem(0, "regeneration") && potBrew(awk, blaze) == potItem(0, "strength"));
    CHECK(potBrew(awk, IT_REDSTONE) == 0 && potBrew(awk, glow) == 0);
    // longa / forte / fermentado
    const int sw = potItem(0, "swiftness"); CHECK(potBrew(sw, IT_REDSTONE) == potItem(0, "swiftness_long") && potBrew(sw, glow) == potItem(0, "swiftness_strong") && potBrew(sw, fse) == potItem(0, "slowness"));
    CHECK(potBrew(potItem(0, "swiftness_long"), glow) == 0 && potBrew(potItem(0, "swiftness_strong"), IT_REDSTONE) == 0 && potBrew(potItem(0, "swiftness_long"), fse) == potItem(0, "slowness_long"));
    CHECK(potBrew(potItem(0, "healing"), glow) == potItem(0, "healing_strong") && potBrew(potItem(0, "healing"), IT_REDSTONE) == 0 && potBrew(potItem(0, "healing"), fse) == potItem(0, "harming"));
    CHECK(potBrew(potItem(0, "healing_strong"), fse) == potItem(0, "harming_strong") && potBrew(potItem(0, "poison"), fse) == potItem(0, "harming") && potBrew(potItem(0, "poison_strong"), fse) == potItem(0, "harming_strong"));
    CHECK(potBrew(potItem(0, "night_vision"), fse) == potItem(0, "invisibility") && potBrew(potItem(0, "night_vision_long"), fse) == potItem(0, "invisibility_long") && potBrew(potItem(0, "leaping"), fse) == potItem(0, "slowness"));
    CHECK(potBrew(potItem(0, "fire_resistance"), fse) == potItem(0, "slowness") && potBrew(potItem(0, "slowness"), IT_REDSTONE) == potItem(0, "slowness_long") && potBrew(potItem(0, "slowness"), glow) == 0);
    CHECK(potBrew(potItem(0, "regeneration"), glow) == potItem(0, "regeneration_strong") && potBrew(potItem(0, "strength"), IT_REDSTONE) == potItem(0, "strength_long") && potBrew(potItem(0, "weakness"), IT_REDSTONE) == potItem(0, "weakness_long"));
    // polvora / folego de dragao
    CHECK(potBrew(sw, gun) == potItem(1, "swiftness") && potBrew(potItem(1, "swiftness"), gun) == 0 && potBrew(potItem(1, "swiftness"), dragon) == potItem(2, "swiftness") && potBrew(sw, dragon) == 0);
    CHECK(potBrew(potItem(1, "swiftness"), IT_REDSTONE) == potItem(1, "swiftness_long"));   // ingrediente em pocao arremessavel preserva a forma
    CHECK(potBrew(water, 0) == 0 && potBrew(sugar, wart) == 0 && !potIs(sugar) && potIs(water) && potForm(potItem(2, "luck")) == 2);
    CHECK(potIsIngredient(wart) && potIsIngredient(IT_REDSTONE) && !potIsIngredient(bottle) && !potIsIngredient(0));
    CHECK(std::string(potType(potItem(0, "leaping_strong"))->eff) == "jump" && potType(potItem(0, "leaping_strong"))->amp == 1 && potType(potItem(0, "poison_strong"))->dur == 432);
    CHECK(PK[potItem(0, "swiftness_long") - PK0].name == "Poção de Velocidade (8:00)" && PK[potItem(1, "healing_strong") - PK0].name == "Poção Arremessável de Cura II");
    // cervejaria: 400 ticks, 1 carga por rodada, 3 garrafas de uma vez
    BrewData b; b.s[0] = {water, 1, 0}; b.s[1] = {water, 1, 0}; b.s[3] = {wart, 2, 0}; b.s[4] = {blaze, 1, 0};
    CHECK(brewCanBrew(b)); int ticks = 0; while (ticks < 1000 && !(b.s[0].id == awk)) { brewStep(b); ticks++; }
    CHECK(ticks == 401 && b.s[0].id == awk && b.s[1].id == awk && !b.s[2].id && b.s[3].n == 1 && b.fuel == 19 && b.s[4].id == 0);
    for (int i = 0; i < 405; i++) brewStep(b); CHECK(b.s[3].n == 1 && b.fuel == 19 && b.brew == 0);   // awkward + verruga nao tem receita: nao prepara de novo
    BrewData c; c.s[0] = {water, 1, 0}; c.s[3] = {wart, 1, 0}; for (int i = 0; i < 50; i++) brewStep(c); CHECK(c.brew == 0 && c.s[0].id == water);   // sem combustivel nao prepara
    c.fuel = 20; for (int i = 0; i < 10; i++) brewStep(c); c.s[3] = Slot(); brewStep(c); CHECK(c.brew == 0);   // tirar o ingrediente interrompe
    // meta / variante
    CHECK(rsVariant(XB_DECOR, 6) == 8 && rsVariant(XB_DECOR, 6 | (7 << 3)) == 8 && rsVariant(XB_DECOR, 2) == 2 && rsVariant(XB_DECOR, 1) == 1 && brewIsBlock(XB_DECOR, 6) && !brewIsBlock(XB_DECOR, 2));
    // salvar / carregar
    for (size_t i = 0; i < PK.size(); i++) gPkByFile[PK[i].file] = PK0 + (int)i;
    mkdir("saves", 0777); mkdir("saves/tb", 0777); gBrews.clear(); BrewData& s0 = gBrews[ChestKey(1, 70, 2)]; s0.s[0] = {potItem(1, "healing"), 1, 0}; s0.s[3] = {glow, 5, 0}; s0.fuel = 7; s0.brew = 123;
    brewSave("tb"); gBrews.clear(); brewLoad("tb"); CHECK(gBrews.size() == 1); { BrewData& r = gBrews[ChestKey(1, 70, 2)]; CHECK(r.s[0].id == potItem(1, "healing") && r.s[3].id == glow && r.s[3].n == 5 && r.fuel == 7 && r.brew == 123); }
    printf("test_brew: %d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
