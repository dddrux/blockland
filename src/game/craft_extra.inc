// ---------- patchCraft.3: receitas que faltavam (1.12.2) + qualquer madeira nas receitas genericas ----------
// Ids resolvidos pelo NOME canonico (1.13) do pack, com aliases 1.12 ("a|b"); item/ antes de block/. Receita cujo ingrediente ou resultado nao existe
// no pack e PULADA (nunca vira ingrediente vazio). Chamado no fim de packRecipes() (chest.inc), que ja recria a lista quando o pack muda.
// Receitas por madeira/cor/variante usam ids exatos; as genericas (palito, mesa, ferramentas, tocha...) aceitam qualquer tabua via `craftNorm` (crafting.inc).
static std::unordered_map<std::string, int>& cxIndex() {
    static std::unordered_map<std::string, int> M; static size_t sz = (size_t)-1;
    if (sz == PK.size()) return M;
    sz = PK.size(); M.clear();
    for (size_t i = 0; i < PK.size(); i++) {
        const std::string& f = PK[i].file; const int id = PK0 + (int)i;
        if (!M.count(f)) M[f] = id;
        const std::string c = pkCanon(f); if (!M.count(c)) M[c] = id;
    }
    return M;
}
static int cxId(const char* names) {   // "a|b|c": 1o nome que existe como item/ ou block/
    const auto& M = cxIndex(); const std::string t = names; size_t p = 0;
    while (p <= t.size()) {
        size_t e = t.find('|', p); if (e == std::string::npos) e = t.size();
        const std::string n = t.substr(p, e - p); p = e + 1; if (n.empty()) continue;
        for (const char* pre : {"item/", "block/"}) { auto it = M.find(pre + n); if (it != M.end()) return it->second; }
    }
    return 0;
}
static const char* const CX_COLOR[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
static const char* const CX_COLOR12[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "silver", "cyan", "purple", "blue", "brown", "green", "red", "black"};
static int cxColored(const char* pre, int c, const char* post) {   // "<cor>_wool" (1.13) ou "wool_colored_<cor>" (1.12) etc.
    std::string a = std::string(CX_COLOR[c]) + post, b = std::string(pre) + CX_COLOR12[c];
    return cxId((a + "|" + b).c_str());
}
static const char* const CX_WOOD[6] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
static void cxWoodIds(int w, int& planks, int& log, int& wood) {   // tabuas, tronco e "madeira" (casca nos 6 lados) de cada especie; 0 = pack sem o bloco
    if (w == 0) { planks = PLANKS; log = LOG; wood = cxId("oak_wood"); return; }
    const std::string n = CX_WOOD[w], o = w == 5 ? "big_oak" : n;
    planks = cxId((n + "_planks|planks_" + o).c_str()); log = cxId((n + "_log|log_" + o).c_str()); wood = cxId((n + "_wood").c_str());
}
static void craftExtraAdd(std::vector<Recipe>& R) {
    typedef std::initializer_list<std::pair<char, int>> KL;
    auto sh = [&](std::vector<std::string> rows, KL k, int out, int n = 1) {
        if (!out) return; Recipe r; r.rows = rows; for (auto& p : k) { if (!p.second) return; r.k[p.first] = p.second; }
        for (const std::string& s : rows) for (char c : s) if (c != ' ' && !r.k.count(c)) return;   // letra sem ingrediente
        r.out = out; r.n = n; R.push_back(r); };
    auto sl = [&](std::initializer_list<int> in, int out, int n = 1) {
        if (!out) return; for (int i : in) if (!i) return;
        Recipe r; r.list = in; std::sort(r.list.begin(), r.list.end()); r.out = out; r.n = n; r.less = true; R.push_back(r); };
    auto I = [&](const char* n) { return cxId(n); };
    auto rd = [&](int xb, int v) { return (xb >= XB_DUST && xb < XB_END) ? gRsDrop[xb - XB_DUST][v & 31] : 0; };
    const int P = PLANKS, S = IT_STICK, C = COBBLE, IR = IT_IRON, GO = IT_GOLD, DI = IT_DIAMOND, RS = IT_REDSTONE, QZ = I("quartz"), GL = I("glass");
    const int SUG = I("sugar"), CANE = I("sugar_cane|reeds"), WHEAT = I("wheat"), COCOA = I("cocoa_beans|dye_powder_brown|brown_dye"), SLIME = I("slime_ball|slimeball"), GUN = I("gunpowder");
    const int BLZR = I("blaze_rod"), BLZP = I("blaze_powder"), GNUG = I("gold_nugget"), INUG = I("iron_nugget"), GDUST = I("glowstone_dust"), BKT_MILK = gBktMilk;
    const int VINE = rd(XB_VINE, 0), RTORCH = gXbDrop[XB_RTORCH - XB0], CHEST = rd(XB_CHEST, 0), HOOK = rd(XB_HOOK, 0), EMER = I("emerald");
    // ---- madeira: cada tronco/"madeira" -> 4 tabuas da propria especie ----
    gPlankSet.clear();
    for (int w = 0; w < 6; w++) { int pl, lg, wd; cxWoodIds(w, pl, lg, wd); if (!pl) continue; gPlankSet.insert(pl); if (w) sl({lg}, pl, 4); sl({wd}, pl, 4); }
    // ---- comida e itens simples ----
    sh({"P P", " P "}, {{'P', P}}, I("bowl"), 4);
    sh({"WWW"}, {{'W', WHEAT}}, IT_BREAD);
    sh({"WCW"}, {{'W', WHEAT}, {'C', COCOA}}, IT_COOKIE, 8);
    sl({CANE}, SUG); sh({"RRR"}, {{'R', CANE}}, IT_PAPER, 3);
    { const int cake = rd(XB_DECOR, 1); sh({"MMM", "SES", "WWW"}, {{'M', BKT_MILK}, {'S', SUG}, {'E', IT_EGG}, {'W', WHEAT}}, cake); }
    sl({I("pumpkin"), SUG, IT_EGG}, I("pumpkin_pie"));
    sl({I("bowl"), I("red_mushroom"), I("brown_mushroom")}, I("mushroom_stew"));
    sl({I("bowl"), I("baked_potato|potato_baked"), IT_CARROT, I("cooked_rabbit|rabbit_cooked"), I("red_mushroom")}, I("rabbit_stew"));
    sl({I("bowl"), I("beetroot"), I("beetroot"), I("beetroot"), I("beetroot"), I("beetroot"), I("beetroot")}, I("beetroot_soup"));
    sh({"NNN", "NCN", "NNN"}, {{'N', GNUG}, {'C', IT_CARROT}}, I("golden_carrot|carrot_golden"));
    sh({"NNN", "NMN", "NNN"}, {{'N', GNUG}, {'M', I("melon_slice|melon")}}, I("glistering_melon_slice|melon_speckled"));
    sh({"SSS", "SSS", "SSS"}, {{'S', I("melon_slice|melon")}}, I("melon"));   // bloco (block/melon)
    sl({I("melon_slice")}, I("melon_seeds|seeds_melon")); sl({I("pumpkin")}, I("pumpkin_seeds|seeds_pumpkin"), 4);
    sh({"WWW", "WWW", "WWW"}, {{'W', WHEAT}}, I("hay_block|hay")); sl({I("hay_block|hay")}, WHEAT, 9);
    // ---- pepitas e blocos de 9 ----
    sl({GO}, GNUG, 9); sh({"NNN", "NNN", "NNN"}, {{'N', GNUG}}, GO);
    sl({IR}, INUG, 9); sh({"NNN", "NNN", "NNN"}, {{'N', INUG}}, IR);
    sl({IT_BONE}, I("bone_meal|dye_powder_white|white_dye"), 3);
    { const int bm = I("bone_meal|dye_powder_white"); sh({"MMM", "MMM", "MMM"}, {{'M', bm}}, I("bone_block")); sl({I("bone_block")}, bm, 9); }
    const struct { const char* item; const char* blk; } NINE[] = {{"emerald", "emerald_block"}, {"slime_ball|slimeball", "slime_block"}, {"nether_wart", "nether_wart_block"}};
    for (auto& t : NINE) { const int it = I(t.item), bk = I(t.blk); if (!it || !bk) continue; sh({"MMM", "MMM", "MMM"}, {{'M', it}}, bk); sl({bk}, it, 9); }
    { const int it = IT_LAPIS, bk = I("lapis_block"); if (bk) { sh({"MMM", "MMM", "MMM"}, {{'M', it}}, bk); sl({bk}, it, 9); } }
    { const int bk = I("redstone_block"); if (bk) { sh({"MMM", "MMM", "MMM"}, {{'M', RS}}, bk); sl({bk}, RS, 9); } }
    sh({"GG", "GG"}, {{'G', GDUST}}, GLOWSTONE); sh({"SS", "SS"}, {{'S', I("snowball")}}, SNOWBLK); sh({"CC", "CC"}, {{'C', I("clay_ball")}}, CLAY);
    sh({"BB", "BB"}, {{'B', I("brick")}}, BRICKS);
    sh({"SS", "SS"}, {{'S', I("nether_brick|netherbrick")}}, I("nether_bricks|nether_brick"));
    sh({"WN", "NW"}, {{'W', I("nether_wart")}, {'N', I("nether_brick|netherbrick")}}, I("red_nether_bricks|red_nether_brick"));
    sh({"NBN", "NBN"}, {{'N', I("nether_bricks|nether_brick")}, {'B', I("nether_brick|netherbrick")}}, rd(XB_FENCE, 6), 6);
    sh({"GG", "GG"}, {{'G', I("prismarine_shard")}}, I("prismarine")); sh({"GGG", "GGG", "GGG"}, {{'G', I("prismarine_shard")}}, I("prismarine_bricks"));
    sh({"SCS", "CCC", "SCS"}, {{'S', I("prismarine_shard")}, {'C', I("prismarine_crystals")}}, I("sea_lantern"));
    sh({"MM", "MM"}, {{'M', I("magma_cream")}}, I("magma|magma_block"));
    sh({"SS", "SS"}, {{'S', I("popped_chorus_fruit|chorus_fruit_popped")}}, I("purpur_block"), 4);
    sh({"SS", "SS"}, {{'S', I("end_stone")}}, I("end_bricks|end_stone_bricks"), 4);
    sh({"R", "C"}, {{'R', BLZR}, {'C', I("popped_chorus_fruit|chorus_fruit_popped")}}, I("end_rod"), 4);
    // ---- pedra e terra ----
    sh({"SS", "SS"}, {{'S', I("red_sand")}}, I("red_sandstone|red_sandstone_normal"));
    sh({"SS", "SS"}, {{'S', SANDSTONE}}, I("cut_sandstone|sandstone_smooth|smooth_sandstone"), 4);
    sh({"S", "S"}, {{'S', rd(XB_SLAB, 7)}}, I("chiseled_sandstone|sandstone_carved"));
    sh({"S", "S"}, {{'S', rd(XB_SLAB, 8)}}, I("chiseled_stone_bricks|stonebrick_carved"));
    sh({"S", "S"}, {{'S', rd(XB_SLAB, 12)}}, I("chiseled_quartz_block|quartz_block_chiseled"));
    sh({"Q", "Q"}, {{'Q', I("quartz_block|quartz_block_side")}}, I("quartz_pillar|quartz_block_lines"), 2);
    sh({"QQ", "QQ"}, {{'Q', QZ}}, I("quartz_block|quartz_block_side"));
    sl({STONEBRICK, VINE}, I("mossy_stone_bricks|stonebrick_mossy")); sl({COBBLE, VINE}, MOSSY); sl({rd(XB_WALL, 0), VINE}, rd(XB_WALL, 1));
    sh({"CQ", "QC"}, {{'C', COBBLE}, {'Q', QZ}}, I("diorite|stone_diorite"), 2);
    sl({I("diorite|stone_diorite"), QZ}, I("granite|stone_granite")); sl({I("diorite|stone_diorite"), COBBLE}, I("andesite|stone_andesite"), 2);
    for (const char* t : {"granite", "diorite", "andesite"}) { const std::string a = std::string(t) + "|stone_" + t, b = std::string("polished_") + t + "|stone_" + t + "_smooth";
        sh({"SS", "SS"}, {{'S', I(a.c_str())}}, I(b.c_str()), 4); }
    sh({"DG", "GD"}, {{'D', DIRT}, {'G', GRAVEL}}, I("coarse_dirt|dirt_coarse"), 4);
    // ---- redstone e mecanismos ----
    sh({"PPP", "CIC", "CRC"}, {{'P', P}, {'C', C}, {'I', IR}, {'R', RS}}, rd(XB_PIST, 0));
    sl({rd(XB_PIST, 0), SLIME}, gRsDrop[XB_PIST + 1 - XB_DUST][0]);
    sh({"CCC", "CBC", "CRC"}, {{'C', C}, {'B', IT_BOW}, {'R', RS}}, rd(XB_DISP, 0));
    sh({"CCC", "C C", "CRC"}, {{'C', C}, {'R', RS}}, rd(XB_DROP, 0));
    sh({"I I", "ICI", " I "}, {{'I', IR}, {'C', CHEST}}, rd(XB_HOPPER, 0));
    sh({"TRT", "SSS"}, {{'T', RTORCH}, {'R', RS}, {'S', STONE}}, rd(XB_REPT, 0));
    sh({" T ", "TQT", "SSS"}, {{'T', RTORCH}, {'Q', QZ}, {'S', STONE}}, rd(XB_COMP, 0));
    sh({"CCC", "RRQ", "CCC"}, {{'C', C}, {'R', RS}, {'Q', QZ}}, rd(XB_OBS, 0));
    sh({"S", "C"}, {{'S', S}, {'C', C}}, gXbDrop[XB_LEVER - XB0]);
    sh({"R", "S"}, {{'R', RS}, {'S', S}}, RTORCH);
    sh({"GGG", "QQQ", "WWW"}, {{'G', GL}, {'Q', QZ}, {'W', rd(XB_SLAB, 0)}}, rd(XB_DAY, 0));
    sh({"I", "S", "P"}, {{'I', IR}, {'S', S}, {'P', P}}, HOOK, 2);
    sh({"PPP", "PRP", "PPP"}, {{'P', P}, {'R', RS}}, I("noteblock|note_block"));
    sh({"PPP", "PDP", "PPP"}, {{'P', P}, {'D', DI}}, I("jukebox"));
    sh({" R ", "RGR", " R "}, {{'R', RS}, {'G', GLOWSTONE}}, I("redstone_lamp|lit_redstone_lamp"));
    sl({CHEST, HOOK}, rd(XB_CHEST, 1));
    sh({"II"}, {{'I', GO}}, rd(XB_PLATE, 2)); sh({"II"}, {{'I', IR}}, rd(XB_PLATE, 3));
    sh({"II", "II"}, {{'I', IR}}, rd(XB_TRAP, 0));
    sh({"ISI", "ITI", "ISI"}, {{'I', IR}, {'S', S}, {'T', RTORCH}}, gXbDrop[XB_RAIL + 3 - XB0], 6);   // trilho ativador (k=3, ver RAIL_FILE)
    sh({"G G", "GSG", "GRG"}, {{'G', GO}, {'S', S}, {'R', RS}}, gXbDrop[XB_RAIL + 1 - XB0], 6);       // energizado (k=1)
    sh({"I I", "IPI", "IRI"}, {{'I', IR}, {'P', rd(XB_PLATE, 0)}, {'R', RS}}, gXbDrop[XB_RAIL + 2 - XB0], 6);   // detector (k=2)
    { const int mc = I("minecart|minecart_normal"); sh({"I I", "III"}, {{'I', IR}}, mc);
      sl({mc, CHEST}, I("chest_minecart|minecart_chest")); sl({mc, gFurnId ? gFurnId : I("furnace")}, I("furnace_minecart|minecart_furnace"));
      sl({mc, I("tnt")}, I("tnt_minecart|minecart_tnt")); sl({mc, rd(XB_HOPPER, 0)}, I("hopper_minecart|minecart_hopper")); }
    sh({"GSG", "SGS", "GSG"}, {{'G', GUN}, {'S', SAND}}, I("tnt"));
    // ---- utilidades ----
    sh({" I", "I "}, {{'I', IR}}, I("shears"));
    sh({"  S", " ST", "S T"}, {{'S', S}, {'T', IT_STRING}}, I("fishing_rod|fishing_rod_uncast"));
    sl({I("fishing_rod|fishing_rod_uncast"), IT_CARROT}, I("carrot_on_a_stick|carrot_on_stick"));
    sh({"BBB", " I ", "III"}, {{'B', IRONBLK}, {'I', IR}}, gAnvDrop[0]);
    sh({"I I", "I I", "III"}, {{'I', IR}}, rd(XB_DECOR, 0));
    sh({" B ", "CCC"}, {{'B', BLZR}, {'C', C}}, I("brewing_stand"));
    sh({"G G", " G "}, {{'G', GL}}, I("glass_bottle|potion_bottle_empty"), 3);
    sh({" B ", "DOD", "OOO"}, {{'B', IT_BOOK}, {'D', DI}, {'O', OBSIDIAN}}, I("enchanting_table|enchanting_table_top"));
    sh({"OOO", "OEO", "OOO"}, {{'O', OBSIDIAN}, {'E', I("ender_eye")}}, rd(XB_CHEST, 2));
    sh({"GGG", "GNG", "OOO"}, {{'G', GL}, {'N', I("nether_star")}, {'O', OBSIDIAN}}, I("beacon"));
    sh({"SSS", "SLS", "SSS"}, {{'S', S}, {'L', IT_LEATHER}}, I("item_frame"));
    sh({"SSS", "SWS", "SSS"}, {{'S', S}, {'W', WOOL}}, I("painting"));
    sh({"SSS", " S ", "SBS"}, {{'S', S}, {'B', rd(XB_SLAB, 10)}}, I("armor_stand"));
    sh({"PIP", "PPP", " P "}, {{'P', P}, {'I', IR}}, I("shield"));
    sl({IT_BOOK, I("ink_sac|dye_powder_black|black_dye"), IT_FEATHER}, I("writable_book|book_writable"));
    sh({" G ", "GAG", " G "}, {{'G', GDUST}, {'A', IT_ARROW}}, I("spectral_arrow"), 2);
    sl({BLZR}, BLZP, 2); sl({BLZP, SLIME}, I("magma_cream"));
    sl({I("ender_pearl"), BLZP}, I("ender_eye|ender_eye"));
    sl({BLZP, IT_COAL, GUN}, I("fire_charge"), 3);
    sl({I("spider_eye"), SUG, I("brown_mushroom")}, I("fermented_spider_eye|spider_eye_fermented"));
    sh({"P", "T"}, {{'P', I("pumpkin")}, {'T', gXbDrop[XB_TORCH - XB0]}}, I("jack_o_lantern|lit_pumpkin"));
    sh({"PPP", "PCP", "PPP"}, {{'P', IT_PAPER}, {'C', I("compass")}}, I("map|map_empty"));
    sh({"SS ", "SL ", "  S"}, {{'S', IT_STRING}, {'L', SLIME}}, I("lead|lead"), 2);
    // ---- cores: tinta, la, carpete, vidro, terracota, concreto, estandarte ----
    int dye[16] = {0};
    for (int c = 0; c < 16; c++) { std::string a = std::string(CX_COLOR[c]) + "_dye", b = std::string("dye_powder_") + CX_COLOR12[c]; dye[c] = I((a + "|" + b).c_str()); }
    dye[0] = I("white_dye|bone_meal|dye_powder_white"); dye[4] = I("yellow_dye|dye_powder_yellow|dandelion_yellow"); dye[11] = I("blue_dye|lapis_lazuli|dye_powder_blue"); dye[12] = I("brown_dye|cocoa_beans|dye_powder_brown"); dye[15] = I("black_dye|ink_sac|dye_powder_black");
    // misturas de tinta (CraftingManager 1.12.2)
    { const auto mix = [&](int a, int b, int out, int n) { if (dye[a] && dye[b] && dye[out]) { if (a == b) { Recipe r; r.list = {dye[a], dye[b]}; std::sort(r.list.begin(), r.list.end()); r.out = dye[out]; r.n = n; r.less = true; R.push_back(r); } else sl({dye[a], dye[b]}, dye[out], n); } };
      mix(14, 4, 1, 2); mix(14, 0, 6, 2); mix(13, 0, 5, 2); mix(11, 0, 3, 2); mix(7, 0, 8, 2); mix(15, 0, 7, 2); mix(11, 13, 9, 2); mix(11, 14, 10, 2);
      if (dye[10] && dye[6] && dye[2]) sl({dye[10], dye[6]}, dye[2], 2);   // magenta = roxo + rosa
      if (dye[15] && dye[0] && dye[8]) sl({dye[15], dye[0], dye[0]}, dye[8], 3); }
    const struct { const char* fl; int c, n; } FL[] = {{"dandelion", 4, 2}, {"poppy", 14, 2}, {"blue_orchid", 3, 2}, {"allium", 2, 2}, {"azure_bluet", 8, 2}, {"red_tulip", 14, 2}, {"orange_tulip", 1, 2},
        {"white_tulip", 8, 2}, {"pink_tulip", 6, 2}, {"oxeye_daisy", 8, 2}, {"cornflower", 11, 2}, {"lily_of_the_valley", 0, 2}, {"sunflower", 4, 2}, {"lilac", 2, 2}, {"rose_bush", 14, 2}, {"peony", 6, 2}, {"beetroot", 14, 1}};
    for (auto& f : FL) { const int fi = I(f.fl); if (fi && dye[f.c]) sl({fi}, dye[f.c], f.n); }
    for (int c = 0; c < 16; c++) {
        const int d = dye[c]; if (!d) continue;
        const int wl = cxColored("wool_colored_", c, "_wool"), gl = cxColored("glass_", c, "_stained_glass"), tc = cxColored("hardened_clay_stained_", c, "_terracotta"), cp = cxColored("concrete_powder_", c, "_concrete_powder");
        if (c && wl) sl({WOOL, d}, wl);   // la branca + tinta (como o Java aceita qualquer la; so a branca existe como id do jogo)
        if (c && wl) sh({"WW"}, {{'W', wl}}, rd(XB_CARPET, c), 3);
        if (c && wl) sh({"WWW", "WWW", " S "}, {{'W', wl}, {'S', S}}, gBnDrop[c]);
        if (gl) sh({"GGG", "GDG", "GGG"}, {{'G', GL}, {'D', d}}, gl, 8);
        if (gl) sh({"GGG", "GGG"}, {{'G', gl}}, rd(XB_CPANE, c), 16);
        const int tb = I("terracotta|hardened_clay"); if (tc && tb) sh({"TTT", "TDT", "TTT"}, {{'T', tb}, {'D', d}}, tc, 8);
        if (cp) sl({SAND, SAND, SAND, SAND, GRAVEL, GRAVEL, GRAVEL, GRAVEL, d}, cp, 8);   // 4 areia + 4 cascalho + 1 tinta (sem forma)
    }
    // vidro/tinta -> o painel branco tambem
    { const int g0 = cxColored("glass_", 0, "_stained_glass"); if (g0) sh({"GGG", "GGG"}, {{'G', g0}}, rd(XB_CPANE, 0), 16); }
    // cama colorida a partir de la colorida ja existe em packRecipes; carpete branco ja existe
    // escadas/lajes/cercas/portas/baus/barcos por especie ja existem em packRecipes; palito, mesa, ferramentas, tocha etc. com qualquer tabua: craftNorm (crafting.inc)
}
