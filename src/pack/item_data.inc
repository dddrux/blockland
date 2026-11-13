// ---------- dados dos itens do pack: nome em portugues e tamanho de pilha (aba "Diversos" do criativo) ----------
// Chave = arquivo do pack ("item/snowball"). Itens que nao estao aqui mantem o nome em ingles gerado por pkTitle e pilha 64.
struct PkItemInfo { const char* file; const char* nome; int pilha; };
static const PkItemInfo PK_INFO[] = {
    {"block/enchanting_table_top", "Mesa de encantamento", 64}, {"block/end_portal_frame_top", "Moldura do portal do End", 64},
    // aba Transporte: trilhos empilham 64; carrinhos, sela, armaduras de cavalo, cenoura no palito, élitro e botes = 1
    {"block/rail", "Trilho", 64}, {"block/powered_rail", "Trilho energizado", 64}, {"block/detector_rail", "Trilho detector", 64}, {"block/activator_rail", "Trilho ativador", 64},
    {"item/compass_00", "Bússola", 1},
    {"item/clock_00", "Relógio", 1},   // patchClock.1: pilha 1 (clock_NN sao quadros da animacao)   // pilha 1 (item/compass_NN sao so quadros da animacao)
    {"item/minecart", "Carrinho de mina", 1}, {"item/chest_minecart", "Carrinho de mina com baú", 1}, {"item/furnace_minecart", "Carrinho de mina com fornalha", 1},
    {"item/tnt_minecart", "Carrinho de mina com TNT", 1}, {"item/hopper_minecart", "Carrinho de mina com funil", 1}, {"item/saddle", "Sela", 1},
    {"item/carrot_on_a_stick", "Cenoura no palito", 1}, {"item/iron_horse_armor", "Armadura de cavalo de ferro", 1}, {"item/golden_horse_armor", "Armadura de cavalo de ouro", 1},
    {"item/diamond_horse_armor", "Armadura de cavalo de diamante", 1}, {"item/elytra", "Élitro", 1}, {"item/oak_boat", "Barco de carvalho", 1}, {"item/spruce_boat", "Barco de abeto", 1},
    {"item/birch_boat", "Barco de bétula", 1}, {"item/jungle_boat", "Barco da selva", 1}, {"item/acacia_boat", "Barco de acácia", 1}, {"item/dark_oak_boat", "Barco de carvalho escuro", 1},
    {"item/bucket", "Balde", 16}, {"item/flint_and_steel", "Isqueiro", 1}, {"item/water_bucket", "Balde de água", 1}, {"item/lava_bucket", "Balde de lava", 1}, {"item/milk_bucket", "Balde de leite", 1},
    {"item/snowball", "Bola de neve", 16}, {"item/ender_pearl", "Pérola do Ender", 16}, {"item/shears", "Tesoura", 1}, {"item/ender_eye", "Olho do Ender", 64},
    {"item/experience_bottle", "Frasco de experiência", 64}, {"item/fire_charge", "Carga de fogo", 64}, {"item/writable_book", "Livro e pena", 1},
    {"item/name_tag", "Etiqueta de nome", 64}, {"item/lead", "Laço", 64}, {"item/firework_rocket", "Foguete de artifício", 64}, {"item/map", "Mapa", 64},
    {"item/music_disc_13", "Disco de música 13", 1}, {"item/music_disc_cat", "Disco de música cat", 1}, {"item/music_disc_blocks", "Disco de música blocks", 1},
    {"item/music_disc_chirp", "Disco de música chirp", 1}, {"item/music_disc_far", "Disco de música far", 1}, {"item/music_disc_mall", "Disco de música mall", 1},
    {"item/music_disc_mellohi", "Disco de música mellohi", 1}, {"item/music_disc_stal", "Disco de música stal", 1}, {"item/music_disc_strad", "Disco de música strad", 1},
    {"item/music_disc_ward", "Disco de música ward", 1}, {"item/music_disc_11", "Disco de música 11", 1}, {"item/music_disc_wait", "Disco de música wait", 1},
    {"item/bat_spawn_egg", "Ovo de morcego", 64}, {"item/blaze_spawn_egg", "Ovo de blaze", 64}, {"item/cave_spider_spawn_egg", "Ovo de aranha da caverna", 64},
    {"item/chicken_spawn_egg", "Ovo de galinha", 64}, {"item/cow_spawn_egg", "Ovo de vaca", 64}, {"item/creeper_spawn_egg", "Ovo de creeper", 64},
    {"item/donkey_spawn_egg", "Ovo de burro", 64}, {"item/elder_guardian_spawn_egg", "Ovo de guardião ancião", 64}, {"item/enderman_spawn_egg", "Ovo de enderman", 64},
    {"item/endermite_spawn_egg", "Ovo de endermite", 64}, {"item/evoker_spawn_egg", "Ovo de evocador", 64}, {"item/ghast_spawn_egg", "Ovo de ghast", 64},
    {"item/guardian_spawn_egg", "Ovo de guardião", 64}, {"item/horse_spawn_egg", "Ovo de cavalo", 64}, {"item/husk_spawn_egg", "Ovo de husk", 64},
    {"item/llama_spawn_egg", "Ovo de lhama", 64}, {"item/magma_cube_spawn_egg", "Ovo de cubo de magma", 64}, {"item/mooshroom_spawn_egg", "Ovo de vaca-cogumelo", 64},
    {"item/mule_spawn_egg", "Ovo de mula", 64}, {"item/ocelot_spawn_egg", "Ovo de jaguatirica", 64}, {"item/parrot_spawn_egg", "Ovo de papagaio", 64},
    {"item/pig_spawn_egg", "Ovo de porco", 64}, {"item/polar_bear_spawn_egg", "Ovo de urso-polar", 64}, {"item/rabbit_spawn_egg", "Ovo de coelho", 64},
    {"item/sheep_spawn_egg", "Ovo de ovelha", 64}, {"item/shulker_spawn_egg", "Ovo de shulker", 64}, {"item/silverfish_spawn_egg", "Ovo de traça", 64},
    {"item/skeleton_spawn_egg", "Ovo de esqueleto", 64}, {"item/skeleton_horse_spawn_egg", "Ovo de cavalo esqueleto", 64}, {"item/slime_spawn_egg", "Ovo de slime", 64},
    {"item/spider_spawn_egg", "Ovo de aranha", 64}, {"item/squid_spawn_egg", "Ovo de lula", 64}, {"item/stray_spawn_egg", "Ovo de esqueleto errante", 64},
    {"item/vex_spawn_egg", "Ovo de vex", 64}, {"item/villager_spawn_egg", "Ovo de aldeão", 64}, {"item/vindicator_spawn_egg", "Ovo de vingador", 64},
    {"item/witch_spawn_egg", "Ovo de bruxa", 64}, {"item/wither_skeleton_spawn_egg", "Ovo de esqueleto wither", 64}, {"item/wolf_spawn_egg", "Ovo de lobo", 64},
    {"item/zombie_spawn_egg", "Ovo de zumbi", 64}, {"item/zombie_horse_spawn_egg", "Ovo de cavalo zumbi", 64},
    {"item/zombified_piglin_spawn_egg", "Ovo de porco-zumbi", 64}, {"item/zombie_pigman_spawn_egg", "Ovo de porco-zumbi", 64}, {"item/zombie_villager_spawn_egg", "Ovo de aldeão zumbi", 64},
};
static const PkItemInfo* pkItemInfo(const std::string& file) {
    static std::unordered_map<std::string, const PkItemInfo*> idx;
    if (idx.empty()) for (const PkItemInfo& e : PK_INFO) idx[e.file] = &e;
    auto it = idx.find(file); return it == idx.end() ? nullptr : it->second;
}
// aplica nome e pilha em todos os itens de PK (chamado depois de montar as abas, porque itens criados pelo cfg copiam o arquivo de origem)
static void pkApplyItemData() {
    for (PkItem& e : PK) if (const PkItemInfo* d = pkItemInfo(e.file)) { e.name = d->nome; e.fold = foldAscii(d->nome); e.stack = d->pilha; }
}
