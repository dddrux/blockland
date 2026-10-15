// patchTex.2: renomeio 1.12.2 -> nomes canonicos e nomes finais dos extras
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    CHECK(pk112("planks_acacia") == "acacia_planks" && pk112("planks_big_oak") == "dark_oak_planks");
    CHECK(pk112("log_big_oak_top") == "dark_oak_log_top" && pk112("log_birch") == "birch_log" && pk112("leaves_jungle") == "jungle_leaves");
    CHECK(pk112("wool_colored_silver") == "light_gray_wool" && pk112("hardened_clay_stained_lime") == "lime_terracotta" && pk112("hardened_clay") == "terracotta");
    CHECK(pk112("glass_red") == "red_stained_glass" && pk112("glass") == "glass" && pk112("glass_pane_top") == "glass_pane_top" && pk112("glass_pane_top_blue") == "blue_stained_glass_pane_top");
    CHECK(pk112("concrete_powder_pink") == "pink_concrete_powder" && pk112("concrete_cyan") == "cyan_concrete" && pk112("glazed_terracotta_white") == "white_glazed_terracotta");
    CHECK(pk112("door_dark_oak_lower") == "dark_oak_door_bottom" && pk112("door_wood_upper") == "oak_door_top" && pk112("door_iron_lower") == "iron_door_bottom");
    CHECK(pk112("stone_andesite_smooth") == "polished_andesite" && pk112("stonebrick_cracked") == "cracked_stone_bricks" && pk112("sandstone_smooth") == "cut_sandstone");
    CHECK(pk112("reeds") == "sugar_cane" && pk112("web") == "cobweb" && pk112("flower_rose") == "poppy" && pk112("sapling_roofed_oak") == "dark_oak_sapling");
    CHECK(pk112("stone") == "stone" && pk112("anvil_base") == "anvil_base");   // sem regra = igual
    // todo nome com regra deve virar nome presente em PT_NAMES quando o pack 1.12 traz textura correspondente (amostra)
    CHECK(pkTitle(pk112("wool_colored_red")) == "Red Wool" && pkTitle(pk112("log_acacia")) == "Acacia Log");
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
