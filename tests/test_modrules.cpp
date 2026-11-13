// test_modrules: o que este teste cobre (1 linha). Rodar: tools/t.sh modrules
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    // pacote falso "fake" com 3 itens, sem props.bin (so o que modrules.inc usa)
    const std::string dir = "/tmp/blmod_fake"; std::filesystem::remove_all(dir); std::filesystem::create_directories(dir);
    auto W = [&](const char* n, const char* t) { FILE* f = fopen((dir + "/" + n).c_str(), "w"); fputs(t, f); fclose(f); };
    W("receitas.txt",
      "shaped fake:gear 2 ISI|S S|ISI I=fake:ingot_copper;S=minecraft:stick\n"          // linha com espaco no meio
      "shaped fake:rod 1 X|X X=ore:ingotCopper\n"                                         // ore dict -> item de mod
      "shapeless fake:ingot_copper 9 fake:block_copper\n"
      "shapeless minecraft:stick 4 minecraft:planks\n"
      "shaped fake:gear 1 XX|XX X=fake:nao_existe\n"                                      // ingrediente sem item: descartada
      "smelt fake:ore_copper fake:ingot_copper 0.7\n");
    W("drops.txt", "block_copper -> fake:ingot_copper 2-3\nore_copper -> minecraft:redstone 4-4 se random_chance\n");
    W("acoes.txt", "fake_gear|mensagem|Ola mundo\nfake_gear|som|fake:nao_existe\nfake_rod|mensagem|\nnao_existe|mensagem|x\n");   // patchForge.4
    PropPack pk; pk.dir = dir; pk.mod = "fake";
    for (const char* id : {"fake_ingot_copper", "fake_gear", "fake_rod", "fake_block_copper", "fake_ore_copper"}) { PropItem it; it.id = id; it.name = id; pk.items.push_back(it); }
    gPropPacks.push_back(pk); gPropBase.push_back(0); gPropTotal = (int)pk.items.size();
    const int ING = PROP0 + 0, GEAR = PROP0 + 1, ROD = PROP0 + 2, BLK = PROP0 + 3, ORE = PROP0 + 4;
    SECTION("T1 resolver nomes");
    CHECK(modResolve("fake:ingot_copper") == ING); CHECK(modResolve("fake:nao_existe") == 0);
    CHECK(modResolve("minecraft:stick") == IT_STICK); CHECK(modResolve("minecraft:planks") == PLANKS); CHECK(modResolve("minecraft:iron_ingot") == IT_IRON);
    CHECK(modResolve("ore:ingotCopper") == ING); CHECK(modResolve("ore:ingotIron") == IT_IRON); CHECK(modResolve("ore:plankWood") == PLANKS);
    SECTION("T2 receitas");
    std::vector<Recipe> R; modRecipesAdd(R);
    printf("receitas: %d ok, %d descartadas\n", gModRecOk, gModRecBad);
    CHECK(R.size() == 4); CHECK(gModRecBad == 1);   // 4 craft validas (a 5a foi descartada); a fundicao conta so em ok
    bool gear = false, rod = false, ing9 = false, stick4 = false;
    for (auto& r : R) {
        if (r.out == GEAR && r.n == 2 && r.rows.size() == 3 && r.rows[1] == "S S" && r.k.at('I') == ING && r.k.at('S') == IT_STICK) gear = true;
        if (r.out == ROD && r.rows.size() == 2 && r.k.at('X') == ING) rod = true;
        if (r.out == ING && r.n == 9 && r.less && r.list == std::vector<int>{BLK}) ing9 = true;
        if (r.out == IT_STICK && r.n == 4 && r.less) stick4 = true; }
    CHECK(gear); CHECK(rod); CHECK(ing9); CHECK(stick4);
    SECTION("T3 fornalha");
    CHECK(furnResult(ORE) == ING); CHECK(furnResult(BLK) == 0);
    SECTION("T4 drops de prop");
    const size_t d0 = DROPS.size();
    CHECK(modPropDrop(BLK, 0, 0, 0)); CHECK(DROPS.size() == d0 + 1); CHECK(DROPS.back().id == ING); CHECK(DROPS.back().n >= 2 && DROPS.back().n <= 3);
    CHECK(!modPropDrop(GEAR, 0, 0, 0));   // sem loot table: o chamador solta o proprio item
    SECTION("OreAll: plankWood/logWood expandem por especie (patchOre.1)");
    CHECK(modOreAll("plankWood").size() == 6 || modOreAll("plankWood").size() == 1); CHECK(modOreVariants("x@1,ore:logWood") == (int)std::min<size_t>(6, modOreAll("logWood").size())); CHECK(modOreVariants("a,b") == 1);
    SECTION("ActArgs: acoes.txt (patchForge.4)");
    modRulesLoad();
    CHECK(gPropActArg.count(GEAR) == 1 && gPropActArg[GEAR].size() == 2);   // mensagem + som; linhas vazias/sem item sao ignoradas
    CHECK(gPropActArg[GEAR][0].first == "mensagem" && gPropActArg[GEAR][0].second == "Ola mundo");
    CHECK(gPropActArg.count(ROD) == 0);
    { const size_t n0 = chatLog.size(); modPropActArgs(GEAR, 0, 0, 0); CHECK(chatLog.size() == n0 + 1); }   // mensagem vai pro chat; som sem arquivo e ignorado
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
