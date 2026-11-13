// ---------- POCOES (patchBrew.1): itens (beber / arremessar / persistente), tabela de efeitos e regras de preparo (PotionHelper 1.12.2) ----------
// Cada pocao e um item do pack gerado em loadPackItems() (potionRegister): garrafa do pack (potion_bottle_*) + potion_overlay tingido pela cor do efeito.
// Um item por (forma, tipo): forma 0 = beber, 1 = arremessavel, 2 = persistente. Os efeitos em si (aplicar/HUD/arremesso) ficam em patchBrew.2.
struct PotType { const char* key; const char* nm; const char* eff; int dur, amp, color; const char* ext; const char* str; const char* fer; };   // dur em ticks; ext = versao longa; str = versao forte; fer = com olho de aranha fermentado
static const PotType POT_T[] = {
    {"water", "Garrafa d'Água", "", 0, 0, 0x385DC6, "", "", "weakness"},
    {"mundane", "Poção Mundana", "", 0, 0, 0x385DC6, "", "", ""},
    {"thick", "Poção Espessa", "", 0, 0, 0x385DC6, "", "", ""},
    {"awkward", "Poção Estranha", "", 0, 0, 0x385DC6, "", "", ""},
    {"night_vision", "Visão Noturna", "night_vision", 3600, 0, 0x1F1FA1, "night_vision_long", "", "invisibility"},
    {"night_vision_long", "Visão Noturna", "night_vision", 9600, 0, 0x1F1FA1, "", "", "invisibility_long"},
    {"invisibility", "Invisibilidade", "invisibility", 3600, 0, 0x7F8392, "invisibility_long", "", ""},
    {"invisibility_long", "Invisibilidade", "invisibility", 9600, 0, 0x7F8392, "", "", ""},
    {"leaping", "Salto", "jump", 3600, 0, 0x22FF4C, "leaping_long", "leaping_strong", "slowness"},
    {"leaping_long", "Salto", "jump", 9600, 0, 0x22FF4C, "", "", "slowness_long"},
    {"leaping_strong", "Salto", "jump", 1800, 1, 0x22FF4C, "", "", "slowness"},
    {"fire_resistance", "Resistência ao Fogo", "fire_resistance", 3600, 0, 0xE49A3A, "fire_resistance_long", "", "slowness"},
    {"fire_resistance_long", "Resistência ao Fogo", "fire_resistance", 9600, 0, 0xE49A3A, "", "", "slowness_long"},
    {"swiftness", "Velocidade", "speed", 3600, 0, 0x7CAFC6, "swiftness_long", "swiftness_strong", "slowness"},
    {"swiftness_long", "Velocidade", "speed", 9600, 0, 0x7CAFC6, "", "", "slowness_long"},
    {"swiftness_strong", "Velocidade", "speed", 1800, 1, 0x7CAFC6, "", "", "slowness"},
    {"slowness", "Lentidão", "slowness", 1800, 0, 0x5A6C81, "slowness_long", "", ""},
    {"slowness_long", "Lentidão", "slowness", 4800, 0, 0x5A6C81, "", "", ""},
    {"water_breathing", "Respiração Aquática", "water_breathing", 3600, 0, 0x2E5299, "water_breathing_long", "", ""},
    {"water_breathing_long", "Respiração Aquática", "water_breathing", 9600, 0, 0x2E5299, "", "", ""},
    {"healing", "Cura", "instant_health", 1, 0, 0xF82423, "", "healing_strong", "harming"},
    {"healing_strong", "Cura", "instant_health", 1, 1, 0xF82423, "", "", "harming_strong"},
    {"harming", "Dano", "instant_damage", 1, 0, 0x430A09, "", "harming_strong", ""},
    {"harming_strong", "Dano", "instant_damage", 1, 1, 0x430A09, "", "", ""},
    {"poison", "Veneno", "poison", 900, 0, 0x4E9331, "poison_long", "poison_strong", "harming"},
    {"poison_long", "Veneno", "poison", 1800, 0, 0x4E9331, "", "", "harming"},
    {"poison_strong", "Veneno", "poison", 432, 1, 0x4E9331, "", "", "harming_strong"},
    {"regeneration", "Regeneração", "regeneration", 900, 0, 0xCD5CAB, "regeneration_long", "regeneration_strong", ""},
    {"regeneration_long", "Regeneração", "regeneration", 1800, 0, 0xCD5CAB, "", "", ""},
    {"regeneration_strong", "Regeneração", "regeneration", 450, 1, 0xCD5CAB, "", "", ""},
    {"strength", "Força", "strength", 3600, 0, 0x932423, "strength_long", "strength_strong", ""},
    {"strength_long", "Força", "strength", 9600, 0, 0x932423, "", "", ""},
    {"strength_strong", "Força", "strength", 1800, 1, 0x932423, "", "", ""},
    {"weakness", "Fraqueza", "weakness", 1800, 0, 0x484D48, "weakness_long", "", ""},
    {"weakness_long", "Fraqueza", "weakness", 4800, 0, 0x484D48, "", "", ""},
    {"luck", "Sorte", "luck", 6000, 0, 0x339900, "", "", ""},
};
static const int POT_N = (int)(sizeof POT_T / sizeof POT_T[0]);
static int gPotId[3][POT_N];                       // [forma][tipo] -> id do item (0 = nao gerado)
static std::unordered_map<int, std::pair<int, int>> gPotOf;   // id do item -> (forma, tipo)
static int potIdx(const char* key) { if (!key || !*key) return -1; for (int i = 0; i < POT_N; i++) if (!strcmp(POT_T[i].key, key)) return i; return -1; }
static bool potIs(int id) { return gPotOf.count(id) > 0; }
static int potForm(int id) { auto it = gPotOf.find(id); return it == gPotOf.end() ? -1 : it->second.first; }
static const PotType* potType(int id) { auto it = gPotOf.find(id); return it == gPotOf.end() ? nullptr : &POT_T[it->second.second]; }
static int potItem(int form, const char* key) { const int t = potIdx(key); return t < 0 || form < 0 || form > 2 ? 0 : gPotId[form][t]; }
static std::string potTime(int ticks) { const int s = ticks / 20; char b[16]; snprintf(b, sizeof b, "%d:%02d", s / 60, s % 60); return b; }
static std::string potName(int form, const PotType& t) {
    std::string n;
    if (!strcmp(t.key, "water")) n = form == 0 ? "Garrafa d'Água" : form == 1 ? "Garrafa d'Água Arremessável" : "Garrafa d'Água Persistente";
    else if (!t.eff[0]) n = form == 0 ? t.nm : (std::string(t.nm) + (form == 1 ? " Arremessável" : " Persistente"));
    else n = std::string(form == 0 ? "Poção de " : form == 1 ? "Poção Arremessável de " : "Poção Persistente de ") + t.nm;
    if (t.amp > 0) n += " II"; else if (strstr(t.key, "_long")) n += " (" + potTime(t.dur) + ")";
    return n;
}
static bool potLoadTile(const char* const* names, uint8_t* out) { for (int i = 0; names[i]; i++) if (packTileRel(std::string(names[i]) + ".png", out)) return true; return false; }
static void potionRegister(int cap) {   // chamado por loadPackItems() logo depois dos ovos de spawn
    memset(gPotId, 0, sizeof gPotId); gPotOf.clear();
    static const char* BD[] = {"item/potion_bottle_drinkable", "item/potion", nullptr}, *BS[] = {"item/potion_bottle_splash", "item/splash_potion", nullptr}, *BL[] = {"item/potion_bottle_lingering", "item/lingering_potion", nullptr}, *OV[] = {"item/potion_overlay", nullptr};
    uint8_t bot[3][1024], ov[1024]; if (!potLoadTile(OV, ov)) return;
    if (!potLoadTile(BD, bot[0])) return; if (!potLoadTile(BS, bot[1])) memcpy(bot[1], bot[0], 1024); if (!potLoadTile(BL, bot[2])) memcpy(bot[2], bot[1], 1024);
    static const char* FN[3] = {"drink", "splash", "linger"};
    for (int f = 0; f < 3; f++) for (int t = 0; t < POT_N; t++) {
        if ((int)PK.size() + 1 >= cap) return;
        const PotType& T = POT_T[t]; const int cr = (T.color >> 16) & 255, cg = (T.color >> 8) & 255, cb = T.color & 255; uint8_t px[1024]; memcpy(px, bot[f], 1024);
        for (int i = 0; i < 256; i++) {   // overlay (cinza) * cor, por cima da garrafa
            const int a = ov[i * 4 + 3]; if (!a) continue; uint8_t* d = px + i * 4;
            const int r = ov[i * 4] * cr / 255, g = ov[i * 4 + 1] * cg / 255, b = ov[i * 4 + 2] * cb / 255;
            d[0] = (uint8_t)((r * a + d[0] * (255 - a)) / 255); d[1] = (uint8_t)((g * a + d[1] * (255 - a)) / 255); d[2] = (uint8_t)((b * a + d[2] * (255 - a)) / 255); d[3] = (uint8_t)std::max<int>(d[3], a);
        }
        PkItem it; it.file = std::string("potion/") + FN[f] + "_" + T.key; it.name = potName(f, T); it.cube = false; it.stack = 1; it.tile = PKTILE0 + (int)PK.size();
        putTile16(PKTILE0 + (int)PK.size(), px); gPotId[f][t] = PK0 + (int)PK.size(); gPotOf[PK0 + (int)PK.size()] = {f, t}; PK.push_back(std::move(it));
    }
}
static void potionCreative(std::vector<int>& tab) {   // aba Pocoes: garrafa d'agua, bases, depois cada efeito (beber, arremessar, persistente)
    auto add = [&](int id) { if (id && std::find(tab.begin(), tab.end(), id) == tab.end()) tab.push_back(id); };
    for (int f = 0; f < 3; f++) for (int t = 0; t < POT_N; t++) add(gPotId[f][t]);
}
// ---- ingredientes ----
struct PotIng { int wart = 0, glow = 0, red = 0, gun = 0, dragon = 0, fse = 0, carrot = 0, magma = 0, foot = 0, sugar = 0, puff = 0, melon = 0, seye = 0, tear = 0, blaze = 0, bottle = 0; };
static const PotIng& potIngs() {
    static PotIng I; static size_t sz = (size_t)-1; if (sz == PK.size()) return I; sz = PK.size(); I = PotIng();
    I.wart = cxId("nether_wart|netherwart"); I.glow = cxId("glowstone_dust|glowstone"); I.red = IT_REDSTONE; I.gun = cxId("gunpowder"); I.dragon = cxId("dragon_breath");
    I.fse = cxId("fermented_spider_eye"); I.carrot = cxId("golden_carrot|carrot_golden"); I.magma = cxId("magma_cream"); I.foot = cxId("rabbit_foot"); I.sugar = cxId("sugar");
    I.puff = cxId("fish_pufferfish_raw|pufferfish"); I.melon = cxId("speckled_melon|glistering_melon_slice|melon_glistering"); I.seye = cxId("spider_eye"); I.tear = cxId("ghast_tear");
    I.blaze = cxId("blaze_powder"); I.bottle = cxId("glass_bottle|potion_bottle_empty");
    return I;
}
static bool potIsIngredient(int id) {
    if (id <= 0) return false; const PotIng& I = potIngs();
    for (int v : {I.wart, I.glow, I.red, I.gun, I.dragon, I.fse, I.carrot, I.magma, I.foot, I.sugar, I.puff, I.melon, I.seye, I.tear, I.blaze}) if (v && v == id) return true;
    return false;
}
// resultado de pocao + ingrediente numa garrafa (forma preservada); 0 = sem receita
static int potBrew(int bottle, int ing) {
    auto it = gPotOf.find(bottle); if (it == gPotOf.end() || ing <= 0) return 0;
    const int f = it->second.first; const PotType& T = POT_T[it->second.second]; const PotIng& I = potIngs();
    if (ing == I.gun) return f == 0 ? gPotId[1][it->second.second] : 0;          // po de polvora: beber -> arremessavel
    if (ing == I.dragon) return f == 1 ? gPotId[2][it->second.second] : 0;       // folego de dragao: arremessavel -> persistente
    auto res = [&](const char* k) { return potItem(f, k); };
    if (!strcmp(T.key, "water")) {
        if (ing == I.wart) return res("awkward"); if (ing == I.glow) return res("thick"); if (ing == I.red) return res("mundane"); if (ing == I.fse) return res("weakness");
        for (int v : {I.tear, I.seye, I.melon, I.sugar, I.magma, I.blaze, I.foot}) if (v && v == ing) return res("mundane");
        return 0;
    }
    if (!strcmp(T.key, "awkward")) {
        if (ing == I.carrot) return res("night_vision"); if (ing == I.magma) return res("fire_resistance"); if (ing == I.foot) return res("leaping"); if (ing == I.sugar) return res("swiftness");
        if (ing == I.puff) return res("water_breathing"); if (ing == I.melon) return res("healing"); if (ing == I.seye) return res("poison"); if (ing == I.tear) return res("regeneration"); if (ing == I.blaze) return res("strength");
        return 0;
    }
    if (ing == I.red && T.ext[0]) return res(T.ext);
    if (ing == I.glow && T.str[0]) return res(T.str);
    if (ing == I.fse && T.fer[0]) return res(T.fer);
    return 0;
}

// patchBoat4.1: botes de 4 lugares removidos
