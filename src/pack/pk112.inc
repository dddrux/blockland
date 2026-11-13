// ---------- nomes 1.12.2 -> nomes canonicos (1.13) do pack Default-Java-1.12.2 (patchTex.2) ----------
// O pack 1.12 usa "planks_acacia", "wool_colored_red", "hardened_clay_stained_blue"...; o jogo, ptbr.inc e o criativo usam "acacia_planks", "red_wool"...
// pk112("log_big_oak_top") = "dark_oak_log_top". Sem regra = devolve o proprio nome. Pura (testavel).
static std::string pk112Color(const std::string& c) { return c == "silver" ? "light_gray" : c; }
static std::string pk112Wood(const std::string& w) { return w == "big_oak" || w == "roofed_oak" ? "dark_oak" : w == "wood" ? "oak" : w; }
static std::string pk112(const std::string& n) {
    auto starts = [&](const char* p) { return n.compare(0, strlen(p), p) == 0; };
    auto tail = [&](const char* p) { return n.substr(strlen(p)); };
    auto endsw = [&](const char* x) { const size_t l = strlen(x); return n.size() > l && n.compare(n.size() - l, l, x) == 0; };
    static const std::unordered_map<std::string, std::string> M = {
        {"hardened_clay", "terracotta"}, {"stone_granite", "granite"}, {"stone_granite_smooth", "polished_granite"}, {"stone_diorite", "diorite"},
        {"stone_diorite_smooth", "polished_diorite"}, {"stone_andesite", "andesite"}, {"stone_andesite_smooth", "polished_andesite"},
        {"stonebrick", "stone_bricks"}, {"stonebrick_carved", "chiseled_stone_bricks"}, {"stonebrick_cracked", "cracked_stone_bricks"}, {"stonebrick_mossy", "mossy_stone_bricks"},
        {"sandstone_carved", "chiseled_sandstone"}, {"sandstone_smooth", "cut_sandstone"}, {"red_sandstone_carved", "chiseled_red_sandstone"}, {"red_sandstone_smooth", "cut_red_sandstone"},
        {"red_sandstone_normal", "red_sandstone"}, {"sandstone_normal", "sandstone"}, {"end_bricks", "end_stone_bricks"}, {"nether_brick", "nether_bricks"}, {"red_nether_brick", "red_nether_bricks"},
        {"ice_packed", "packed_ice"}, {"magma", "magma_block"}, {"dirt_podzol_top", "podzol_top"}, {"dirt_podzol_side", "podzol_side"}, {"redstone_lamp_off", "redstone_lamp"},
        {"quartz_block_lines", "quartz_pillar"}, {"quartz_block_lines_top", "quartz_pillar_top"}, {"prismarine_dark", "dark_prismarine"}, {"prismarine_rough", "prismarine"},
        {"mushroom_block_skin_brown", "brown_mushroom_block"}, {"mushroom_block_skin_red", "red_mushroom_block"}, {"mushroom_block_skin_stem", "mushroom_stem"}, {"mushroom_block_inside", "mushroom_block_inside"},
        {"reeds", "sugar_cane"}, {"tallgrass", "grass"}, {"deadbush", "dead_bush"}, {"web", "cobweb"}, {"waterlily", "lily_pad"}, {"mob_spawner", "spawner"}, {"noteblock", "note_block"},
        {"trapdoor", "oak_trapdoor"}, {"flower_rose", "poppy"}, {"flower_dandelion", "dandelion"}, {"flower_houstonia", "azure_bluet"}, {"flower_oxeye_daisy", "oxeye_daisy"},
        {"flower_allium", "allium"}, {"flower_blue_orchid", "blue_orchid"}, {"flower_tulip_red", "red_tulip"}, {"flower_tulip_orange", "orange_tulip"}, {"flower_tulip_pink", "pink_tulip"},
        {"flower_tulip_white", "white_tulip"}, {"flower_paeonia", "peony"}, {"double_plant_rose_bottom", "rose_bush_bottom"}, {"double_plant_rose_top", "rose_bush_top"},
        {"double_plant_paeonia_bottom", "peony_bottom"}, {"double_plant_paeonia_top", "peony_top"}, {"double_plant_syringa_bottom", "lilac_bottom"}, {"double_plant_syringa_top", "lilac_top"},
        {"double_plant_grass_bottom", "tall_grass_bottom"}, {"double_plant_grass_top", "tall_grass_top"}, {"double_plant_fern_bottom", "large_fern_bottom"}, {"double_plant_fern_top", "large_fern_top"},
        {"double_plant_sunflower_bottom", "sunflower_bottom"}, {"double_plant_sunflower_top", "sunflower_top"}, {"double_plant_sunflower_front", "sunflower_front"}, {"double_plant_sunflower_back", "sunflower_back"},
        {"sapling_oak", "oak_sapling"}, {"sapling_spruce", "spruce_sapling"}, {"sapling_birch", "birch_sapling"}, {"sapling_jungle", "jungle_sapling"}, {"sapling_acacia", "acacia_sapling"}, {"sapling_roofed_oak", "dark_oak_sapling"},
        {"rail_normal", "rail"}, {"rail_normal_turned", "rail_corner"}, {"rail_golden", "powered_rail"}, {"rail_golden_powered", "powered_rail_on"}, {"rail_detector", "detector_rail"},
        {"rail_detector_powered", "detector_rail_on"}, {"rail_activator", "activator_rail"}, {"rail_activator_powered", "activator_rail_on"},
        {"melon_stem_connected", "attached_melon_stem"}, {"melon_stem_disconnected", "melon_stem"}, {"pumpkin_stem_connected", "attached_pumpkin_stem"}, {"pumpkin_stem_disconnected", "pumpkin_stem"},
        {"redstone_torch_on", "redstone_torch"}, {"redstone_torch_off", "redstone_torch_off"}, {"torch_on", "torch"}, {"anvil_base", "anvil"}, {"grass_path_top", "grass_path_top"}, {"slime", "slime_block"},
        {"cactus_bottom", "cactus_bottom"}, {"soul_sand", "soul_sand"}, {"hopper_outside", "hopper_outside"}, {"dispenser_front_horizontal", "dispenser_front"}, {"dropper_front_horizontal", "dropper_front"}, {"piston_top_normal", "piston_top"}, {"enchanting_table_top", "enchanting_table_top"}, {"iron_bars", "iron_bars"},
        {"comparator_off", "comparator"}, {"repeater_off", "repeater"}, {"farmland_dry", "farmland"}, {"farmland_wet", "farmland_moist"}, {"portal", "nether_portal"}, {"trip_wire", "tripwire"}, {"trip_wire_source", "tripwire_hook"},
        {"wheat_stage_0", "wheat_stage0"}, {"wheat_stage_1", "wheat_stage1"}, {"wheat_stage_2", "wheat_stage2"}, {"wheat_stage_3", "wheat_stage3"}, {"wheat_stage_4", "wheat_stage4"}, {"wheat_stage_5", "wheat_stage5"}, {"wheat_stage_6", "wheat_stage6"}, {"carrots_stage_0", "carrots_stage0"}, {"carrots_stage_1", "carrots_stage1"}, {"carrots_stage_2", "carrots_stage2"}, {"carrots_stage_3", "carrots_stage3"}, {"potatoes_stage_0", "potatoes_stage0"}, {"potatoes_stage_1", "potatoes_stage1"}, {"potatoes_stage_2", "potatoes_stage2"}, {"potatoes_stage_3", "potatoes_stage3"}, {"beetroots_stage_0", "beetroots_stage0"}, {"beetroots_stage_1", "beetroots_stage1"}, {"beetroots_stage_2", "beetroots_stage2"}, {"beetroots_stage_3", "beetroots_stage3"}, {"wheat_stage_7", "wheat_stage7"}, {"nether_wart_stage_0", "nether_wart_stage0"}, {"nether_wart_stage_1", "nether_wart_stage1"}, {"nether_wart_stage_2", "nether_wart_stage2"}, {"sponge_wet", "wet_sponge"}, {"log_oak", "oak_log"}, {"log_oak_top", "oak_log_top"}, {"leaves_oak", "oak_leaves"},
        {"door_wood_lower", "oak_door_bottom"}, {"door_wood_upper", "oak_door_top"}, {"door_iron_lower", "iron_door_bottom"}, {"door_iron_upper", "iron_door_top"},
        {"planks_oak", "oak_planks"}, {"planks_big_oak", "dark_oak_planks"}, {"brick", "bricks"}, {"cobblestone_mossy", "mossy_cobblestone"},
    };
    auto it = M.find(n); if (it != M.end()) return it->second;
    if (starts("planks_")) return pk112Wood(tail("planks_")) + "_planks";
    if (starts("leaves_")) return pk112Wood(tail("leaves_")) + "_leaves";
    if (starts("log_")) { std::string r = tail("log_"); const bool top = endsw("_top"); if (top) r.resize(r.size() - 4); return pk112Wood(r) + "_log" + (top ? "_top" : ""); }
    if (starts("door_")) { std::string r = tail("door_"); const bool up = endsw("_upper"); const size_t k = r.rfind('_'); if (k != std::string::npos) return pk112Wood(r.substr(0, k)) + "_door_" + (up ? "top" : "bottom"); }
    if (starts("wool_colored_")) return pk112Color(tail("wool_colored_")) + "_wool";
    if (starts("hardened_clay_stained_")) return pk112Color(tail("hardened_clay_stained_")) + "_terracotta";
    if (starts("concrete_powder_")) return pk112Color(tail("concrete_powder_")) + "_concrete_powder";
    if (starts("concrete_")) return pk112Color(tail("concrete_")) + "_concrete";
    if (starts("glazed_terracotta_")) return pk112Color(tail("glazed_terracotta_")) + "_glazed_terracotta";
    if (starts("shulker_top_")) return pk112Color(tail("shulker_top_")) + "_shulker_box";
    if (starts("glass_pane_top_")) return pk112Color(tail("glass_pane_top_")) + "_stained_glass_pane_top";
    if (starts("glass_") && n != "glass_pane_top") return pk112Color(tail("glass_")) + "_stained_glass";
    return n;
}

// patchBlk.3: nomes de ITEM 1.12 -> canonicos 1.13 (door_wood -> oak_door, record_13 -> music_disc_13, wood_pickaxe -> wooden_pickaxe...). Usado so para achar a textura do cfg do criativo.
static std::string pk112Item(const std::string& n) {
    auto starts = [&](const char* p) { return n.compare(0, strlen(p), p) == 0; };
    auto tail = [&](const char* p) { return n.substr(strlen(p)); };
    static const std::unordered_map<std::string, std::string> M = {
        {"door_wood", "oak_door"}, {"minecart_normal", "minecart"}, {"minecart_chest", "chest_minecart"}, {"minecart_furnace", "furnace_minecart"}, {"minecart_tnt", "tnt_minecart"},
        {"minecart_hopper", "hopper_minecart"}, {"minecart_command_block", "command_block_minecart"}, {"bucket_empty", "bucket"}, {"bucket_water", "water_bucket"}, {"bucket_lava", "lava_bucket"},
        {"bucket_milk", "milk_bucket"}, {"fish_cod_raw", "cod"}, {"fish_salmon_raw", "salmon"}, {"fish_clownfish_raw", "tropical_fish"}, {"fish_pufferfish_raw", "pufferfish"},
        {"fish_cod_cooked", "cooked_cod"}, {"fish_salmon_cooked", "cooked_salmon"}, {"rabbit_raw", "rabbit"}, {"rabbit_cooked", "cooked_rabbit"}, {"melon", "melon_slice"},
        {"melon_speckled", "glistering_melon_slice"}, {"potato_baked", "baked_potato"}, {"potato_poisonous", "poisonous_potato"}, {"carrot_golden", "golden_carrot"},
        {"seeds_wheat", "wheat_seeds"}, {"seeds_melon", "melon_seeds"}, {"seeds_pumpkin", "pumpkin_seeds"}, {"slimeball", "slime_ball"}, {"netherbrick", "nether_brick"},
        {"spider_eye_fermented", "fermented_spider_eye"}, {"chorus_fruit_popped", "popped_chorus_fruit"}, {"gold_horse_armor", "golden_horse_armor"}, {"fishing_rod_uncast", "fishing_rod"},
        {"sign", "oak_sign"}, {"map_empty", "map"}, {"dye_powder_black", "ink_sac"}, {"dye_powder_brown", "cocoa_beans"}, {"dye_powder_blue", "lapis_lazuli"}, {"dye_powder_white", "bone_meal"},
        {"dye_powder_silver", "light_gray_dye"}, {"potion_bottle_empty", "glass_bottle"}, {"potion_bottle_drinkable", "potion"}, {"potion_bottle_splash", "splash_potion"},
        {"potion_bottle_lingering", "lingering_potion"}, {"book_normal", "book"}, {"book_enchanted", "enchanted_book"}, {"book_writable", "writable_book"}, {"book_written", "written_book"},
        {"fireworks", "firework_rocket"}, {"fireworks_charge", "firework_star"}, {"leather_helmet", "leather_helmet"}, {"beef_raw", "beef"}, {"beef_cooked", "cooked_beef"},
        {"clock_00", "clock"}, {"compass_00", "compass"}, {"seeds_beetroot", "beetroot_seeds"}, {"hopper", "hopper"}, {"repeater", "repeater"}, {"comparator", "comparator"},
    };
    auto it = M.find(n); if (it != M.end()) return it->second;
    if (starts("dye_powder_")) return pk112Color(tail("dye_powder_")) + "_dye";
    if (starts("record_")) return "music_disc_" + tail("record_");
    if (starts("wood_")) return "wooden_" + tail("wood_");
    if (starts("gold_") && n != "gold_ingot" && n != "gold_nugget") return "golden_" + tail("gold_");
    if (starts("door_")) { std::string r = tail("door_"); return pk112Wood(r) + "_door"; }
    return n;
}
static std::string pkCanon(const std::string& f) {   // "item/door_wood" -> "item/oak_door"; "block/concrete_white" -> "block/white_concrete"
    const size_t sl = f.find('/'); if (sl == std::string::npos || f.compare(0, 6, "chisel") == 0) return f;
    return f.substr(0, sl + 1) + (f.compare(0, 5, "item/") == 0 ? pk112Item(f.substr(sl + 1)) : pk112(f.substr(sl + 1)));
}
