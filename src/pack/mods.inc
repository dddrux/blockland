// ---------- mods: pasta mods/<nome>/mod.txt ----------
// Formato (uma linha por entrada, '#' comenta):
//   name=Nome exibido na aba
//   block ruby_block Bloco de Rubi color=C0183C light=0
//   item  ruby Rubi color=E02040
// Texturas 16x16 opcionais em mods/<nome>/textures/: <id>.png (todas as faces) ou <id>_top / _side / _bottom.png
// (itens: <id>.png). Sem textura, o jogo gera uma com a cor indicada. Cada mod ganha uma aba própria no criativo.
static void modFallbackTile(int tile, unsigned col, bool item, int seed) {
    const int r = (col >> 16) & 255, g = (col >> 8) & 255, b = col & 255;
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        float n = hash2(x, y, seed) - .5f;
        if (!item) {
            float e = (x == 0 || y == 0 || x == 15 || y == 15) ? .82f : 1.f;
            apx(tile, x, y, clampb((r + n * 40) * e), clampb((g + n * 40) * e), clampb((b + n * 40) * e));
        } else {
            float dx = x - 7.5f, dy = y - 7.5f, d = sqrtf(dx * dx + dy * dy);
            if (d < 5.6f) { float sh = 1.15f - d / 14.f + n * .2f - (d > 4.4f ? .3f : 0.f); apx(tile, x, y, clampb(r * sh), clampb(g * sh), clampb(b * sh)); }
        }
    }
}
// patchMod.9: desligar mods individualmente. options.cfg: modsoff=pasta1,pasta2 (nome da pasta em mods/). Vale na proxima abertura do jogo.
static std::unordered_set<std::string> gModsOff, gModsOff0; static bool gModsOffLoaded = false;
static void modsOffLoad() {
    if (gModsOffLoaded) return; gModsOffLoaded = true;
    FILE* f = fopen("options.cfg", "rb"); if (!f) return; char ln[2048];
    while (fgets(ln, sizeof ln, f)) if (!strncmp(ln, "modsoff=", 8)) {
        std::string cur; for (const char* c = ln + 8; ; c++) { if (*c == ',' || *c == '\n' || *c == '\r' || !*c) { if (!cur.empty()) gModsOff.insert(cur); cur.clear(); if (!*c) break; } else cur += *c; } }
    fclose(f); gModsOff0 = gModsOff;
}
static bool modIsOff(const std::string& dir) { modsOffLoad(); return gModsOff.count(dir) > 0; }
static std::string modsOffLine() { std::string r; for (const std::string& s : gModsOff) { if (!r.empty()) r += ','; r += s; } return r; }
static std::vector<std::string> modDirNames() {   // pastas de mods/ com mod.txt, em ordem alfabetica
    namespace fs = std::filesystem; std::error_code ec; std::vector<std::string> r;
    if (fs::is_directory("mods", ec)) for (auto& e : fs::directory_iterator("mods", ec)) if (e.is_directory(ec) && fs::exists(e.path() / "mod.txt", ec)) r.push_back(e.path().filename().string());
    std::sort(r.begin(), r.end()); return r;
}
static void loadMods() {
    namespace fs = std::filesystem; std::error_code ec;
    if (!fs::is_directory("mods", ec)) return;
    std::vector<fs::path> dirs;
    for (auto& e : fs::directory_iterator("mods", ec)) if (e.is_directory(ec)) dirs.push_back(e.path());
    std::sort(dirs.begin(), dirs.end());
    for (auto& d : dirs) {
        if (modIsOff(d.filename().string())) continue;   // patchMod.9: mod desligado nao registra blocos/itens
        FILE* fp = fopen((d / "mod.txt").string().c_str(), "r"); if (!fp) continue;
        ModDef m; m.name = d.filename().string();
        char line[512];
        while (fgets(line, sizeof line, fp)) {
            std::string s = line;
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
            size_t p0 = s.find_first_not_of(" \t"); if (p0 == std::string::npos || s[p0] == '#') continue;
            s = s.substr(p0);
            if (s.rfind("name=", 0) == 0) { m.name = s.substr(5); continue; }
            std::vector<std::string> tk; { std::string cur; for (char ch : s) { if (ch == ' ' || ch == '\t') { if (!cur.empty()) tk.push_back(cur); cur.clear(); } else cur += ch; } if (!cur.empty()) tk.push_back(cur); }
            if (tk.size() < 2 || (tk[0] != "block" && tk[0] != "item")) continue;
            if (modNext > 255) { fprintf(stderr, "mods: limite de ids (%d) atingido\n", 256 - MOD0); break; }
            const bool blk = tk[0] == "block";
            std::string disp; unsigned col = 0; bool hasCol = false; int light = 0;
            for (size_t i = 2; i < tk.size(); i++) {
                if (tk[i].rfind("color=", 0) == 0) { col = (unsigned)strtoul(tk[i].c_str() + 6, nullptr, 16); hasCol = true; }
                else if (tk[i].rfind("light=", 0) == 0) light = atoi(tk[i].c_str() + 6);
                else disp += (disp.empty() ? "" : " ") + tk[i];
            }
            if (disp.empty()) disp = tk[1];
            if (!hasCol) { unsigned h = 7; for (char ch : tk[1]) h = h * 131 + (unsigned char)ch; col = ((80 + (h & 127)) << 16) | ((80 + ((h >> 7) & 127)) << 8) | (80 + ((h >> 14) & 127)); }
            const int id = modNext++;
            modName[id] = disp; modKey[id] = m.name + "/" + tk[1]; gModByHash[netFnvMod(modKey[id])] = id; modBlk[id] = blk; modEmit[id] = std::max(0, std::min(15, light));
            const std::string base = (d / "textures" / tk[1]).string();
            if (blk) {
                static const char* sfx[3] = {"_top", "_side", "_bottom"};
                for (int f = 0; f < 3; f++) {
                    int tile = 336 + 3 * (id - MOD0) + f; TILES[id][f] = tile;
                    if (!loadSpriteTile((base + sfx[f] + ".png").c_str(), tile) && !loadSpriteTile((base + ".png").c_str(), tile)) modFallbackTile(tile, col, false, id * 3 + f);
                }
            } else {
                int tile = itemTile(id);
                if (!loadSpriteTile((base + ".png").c_str(), tile)) modFallbackTile(tile, col, true, id);
            }
            m.ids.push_back(id);
        }
        fclose(fp);
        if (m.ids.empty()) continue;
        std::stable_partition(m.ids.begin(), m.ids.end(), [](int i) { return modBlk[i]; });   // blocos primeiro, depois itens
        TABS.push_back({strdup(m.name.c_str()), m.ids[0], false, false, -1});
        printf("Mod carregado: %s (%d entradas)\n", m.name.c_str(), (int)m.ids.size());
        MODS.push_back(m);
    }
}

// ---------- grama/folhas cinza + colormap por bioma ----------
static void cmLoad(float (*cm)[CMS][3], const char* path) {   // PNG de qualquer tamanho (x = temperatura, y = umidade)
    std::vector<uint8_t> px; int w = 0, h = 0;
    if (!loadPNG(path, px, w, h) || w < 2 || h < 2) return;
    for (int y = 0; y < CMS; y++) for (int x = 0; x < CMS; x++) {
        int sx = x * (w - 1) / (CMS - 1), sy = y * (h - 1) / (CMS - 1);
        for (int k = 0; k < 3; k++) cm[y][x][k] = px[(sy * w + sx) * 4 + k] / 255.f;
    }
}
static void initColormaps() {
    static const unsigned G[4] = {0x80B497, 0xBFB755, 0x4FBF6B, 0x59C93C}, F[4] = {0x60A17B, 0xAEA42A, 0x4E9E64, 0x30BB0B};
    cmDefault(cmGrass, G); cmDefault(cmFoliage, F);
    cmLoad(cmGrass, "assets/textures/colormap/grass.png"); cmLoad(cmFoliage, "assets/textures/colormap/foliage.png");
}
static uint8_t* tpx(int tile, int x, int y) { return &atlas[(((tile / ATL) * 16 + y) * ATW + (tile % ATL) * 16 + x) * 4]; }
static bool isGrayPx(const uint8_t* p) { const int mx = std::max(p[0], std::max(p[1], p[2])), mn = std::min(p[0], std::min(p[1], p[2])); return mx - mn <= 6 || (mx - mn <= 32 && p[2] >= p[0] - 4 && p[2] >= p[1]); }   /* patchTintFix.1: pack 1.12 pinta a grama em cinza-ARROXEADO (123,112,129): R,G,B diferem >6 e a lateral caia na terra sem tinta; terra marrom tem R>B e continua fora */
static bool isGreenPx(const uint8_t* p) { return p[1] > p[0] + 25 && p[1] > p[2] + 25; }
static void grayify(uint8_t* p, float k) { int g = (int)((p[0] * .3f + p[1] * .59f + p[2] * .11f) * k); p[0] = p[1] = p[2] = (uint8_t)std::min(g, 255); }
static void bakeBiomeTiles() {
    initColormaps();
    float gc[3], fc[3]; cmSample(cmGrass, .5f, .5f, gc); cmSample(cmFoliage, .5f, .5f, fc);
    // texturas verdes antigas viram cinza (as já cinzas ficam como estão)
    for (int px = 0; px < 256; px++) { int x = px % 16, y = px / 16;
        uint8_t* p = tpx(TILES[GRASS][0], x, y); if (isGreenPx(p)) grayify(p, 1.6f);
        for (int f = 0; f < 3; f++) { uint8_t* q = tpx(TILES[LEAVES][f], x, y); if (q[3] && isGreenPx(q)) grayify(q, 1.9f); } }
    // lateral da grama: base de terra + sobreposição cinza (tingida pelo bioma)
    const int side = TILES[GRASS][1], dirt = TILES[DIRT][1];
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        uint8_t s[4]; memcpy(s, tpx(side, x, y), 4); uint8_t *base = tpx(TILE_GRASS_BASE, x, y), *ov = tpx(TILE_GRASS_OVER, x, y);
        if (isGreenPx(s) || isGrayPx(s)) { if (isGreenPx(s)) grayify(s, 1.6f); memcpy(ov, s, 4); ov[3] = 255; memcpy(base, tpx(dirt, x, y), 4); }
        else { memcpy(base, s, 4); memset(ov, 0, 4); }
    }
    // ícones (inventário/mão) com a cor padrão de planície
    auto mul = [](uint8_t* o, const uint8_t* p, const float* c) { for (int k = 0; k < 3; k++) o[k] = (uint8_t)std::min(255.f, p[k] * c[k]); o[3] = p[3]; };
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        mul(tpx(TILE_ICON_GTOP, x, y), tpx(TILES[GRASS][0], x, y), gc);
        mul(tpx(TILE_ICON_LEAF, x, y), tpx(TILES[LEAVES][0], x, y), fc);
        const uint8_t* ov = tpx(TILE_GRASS_OVER, x, y); uint8_t* o = tpx(TILE_ICON_GSIDE, x, y);
        if (ov[3]) mul(o, ov, gc); else memcpy(o, tpx(TILE_GRASS_BASE, x, y), 4);
    }
    for (int id = 0; id < 256; id++) for (int f = 0; f < 3; f++) ICONTILE[id][f] = TILES[id][f];
    ICONTILE[GRASS][0] = TILE_ICON_GTOP; ICONTILE[GRASS][1] = TILE_ICON_GSIDE;
    for (int f = 0; f < 3; f++) ICONTILE[LEAVES][f] = TILE_ICON_LEAF;
}

// ---------- aba "Resource pack": todos os blocos/itens do pack (ids >= PK0), com busca na aba Pesquisar ----------
// ---------- criativo igual ao 1.12.2: assets/creative/mc-1.12.2.cfg (gerado por tools/gen_creative_1122.py) ----------
static int tileOfId(int id) { return isPk(id) ? PK[id - PK0].tile : isBlk(id) ? TILES[id][0] : itemTile(id); }
// ---------- faces soltas de bloco (cactus_side, furnace_front, tnt_side...) ----------
// O pack traz cada face como PNG separado; loadPackItems registra todos como item. Quando o bloco real existe (X_top + X_side => bloco "X"),
// a face solta e um item "fantasma": plano, nao colocavel. Aqui detectamos e redirecionamos para o bloco de verdade.
static bool pkFaceStem(const std::string& file, std::string& stem) {   // "block/cactus_side" -> "cactus"
    if (file.compare(0, 6, "block/") != 0) return false;
    static const char* SFX[] = {"_front_off", "_front_on", "_front_horizontal", "_front_vertical", "_top", "_bottom", "_side", "_front", "_back", "_end", "_inner", "_outer", "_overlay", "_particle"};
    const std::string n = file.substr(6);
    for (const char* x : SFX) { const size_t l = strlen(x); if (n.size() > l && n.compare(n.size() - l, l, x) == 0) { stem = n.substr(0, n.size() - l); return true; } }
    return false;
}
static bool pkIsFaceOnly(int id) {   // face solta SEM funcao: nao e bloco, nao e item colocavel por modelo (bigorna, plantas altas)
    if (!isPk(id)) return false;
    const PkItem& e = PK[id - PK0]; std::string st;
    return !e.cube && !xbItemBlock(id) && pkFaceStem(e.file, st);
}
static int pkRealBlock(int id) {   // face solta -> bloco/item real da mesma familia (ou o proprio id se nao houver)
    if (!pkIsFaceOnly(id)) return id;
    std::string st; pkFaceStem(PK[id - PK0].file, st);
    auto good = [&](const std::string& f) { auto it = gPkByFile.find(f); return (it != gPkByFile.end() && isPk(it->second) && (PK[it->second - PK0].cube || xbItemBlock(it->second))) ? it->second : 0; };
    if (int r = good("block/" + st)) return r;                    // cactus_side -> cactus, furnace_front -> furnace, anvil_top -> anvil
    if (int r = good("block/" + st + "_top")) return r;           // sunflower_front -> sunflower_top (item colocavel da planta alta)
    for (const char* pre : {"chipped_", "damaged_"}) { const size_t l = strlen(pre); if (st.compare(0, l, pre) == 0) if (int r = good("block/" + st.substr(l))) return r; }   // bigorna danificada usa o icone da bigorna
    return id;
}
// spec = "tex1,tex2" (primeira que existir). Sem nome: o proprio bloco/item (do jogo ou extra do pack). Com nome: item novo de inventario que reusa a textura (escadas, lajes, pocoes...)
static std::unordered_set<int> gCreativeBase;   // ids de PK ja usados por uma aba do criativo (nao repetir na aba "Resource pack")
static int creativeResolve(const std::string& spec, const std::string& name, const std::unordered_map<std::string, int>& pkIdx) {
    int base = 0; std::string used;
    for (size_t a = 0; a <= spec.size() && !base;) {
        size_t c = spec.find(',', a); if (c == std::string::npos) c = spec.size();
        const std::string t = spec.substr(a, c - a); a = c + 1;
        auto g = gPackId.find(t); if (g != gPackId.end()) { base = g->second; used = t; continue; }
        auto p = pkIdx.find(t); if (p != pkIdx.end()) { base = PK0 + p->second; used = t; }
    }
    if (!base) return base;
    if (isPk(base)) gCreativeBase.insert(base);
    if (gFurnId && (used == "block/furnace_front" || used == "block/furnace")) { gCreativeBase.insert(gFurnId); return gFurnId; }   // a fornalha do criativo e a FUNCIONAL (gFurnId), nao uma copia so com a textura
    const int real = pkRealBlock(base);   // face solta -> bloco real (icone 3D, colocavel)
    if (real != base) {
        base = real; if (isPk(base)) gCreativeBase.insert(base);   // patchFlor.1: nao repetir o bloco real na aba "Resource pack"
        if (name.empty() || foldAscii(name.c_str()) == foldAscii(PK[base - PK0].name.c_str())) return base;   // "Furnace" via furnace_front = o proprio bloco
    } else if (name.empty() && isPk(base) && pkIsFaceOnly(base)) {   // sem bloco real (ex.: glass_pane_top): tira o sufixo do nome, "Glass Pane Top" -> "Glass Pane"
        std::string st; pkFaceStem(PK[base - PK0].file, st);
        PK[base - PK0].name = pkTitle(st); PK[base - PK0].fold = foldAscii(PK[base - PK0].name.c_str());
    }
    if (name.empty()) return base;
    PkItem it; it.file = used; it.name = name; it.fold = foldAscii(name.c_str()); it.cube = isCubeIcon(base); it.tile = tileOfId(base);
    PK.push_back(it); return PK0 + (int)PK.size() - 1;
}
static void potionCreative(std::vector<int>& tab);   // potions.inc
static int potItem(int form, const char* key);   // potions.inc
static void loadCreativeCfg() {
    FILE* f = fopen("assets/creative/mc-1.12.2.cfg", "r");
    if (!f) { fprintf(stderr, "criativo 1.12.2: assets/creative/mc-1.12.2.cfg nao encontrado (usando abas antigas)\n"); return; }
    static const char* NM[NTAB] = {"BUILD", "DECO", "REDSTONE", "TRANSPORT", "MISC", "", "FOOD", "TOOLS", "COMBAT", "BREW", "MATERIALS", ""};
    std::unordered_map<std::string, int> pkIdx; for (int i = 0; i < (int)PK.size(); i++) pkIdx[PK[i].file] = i;
    for (int i = 0; i < (int)PK.size(); i++) {   // patchBlk.3: pack 1.12 usa nomes antigos (concrete_white, planks_oak); o cfg usa os canonicos (white_concrete): registra o alias pk112
        const std::string& fl = PK[i].file; const size_t sl = fl.find('/'); if (sl == std::string::npos || fl.compare(0, 6, "chisel") == 0) continue;
        const std::string nm = fl.substr(sl + 1), al = fl.substr(0, sl + 1) + (fl.compare(0, 5, "item/") == 0 ? pk112Item(nm) : pk112(nm)); if (!pkIdx.count(al)) pkIdx[al] = i;
    }
    static const char* CA[][2] = {   // nome do cfg -> arquivo do pack 1.12 quando a regra geral nao acha
        {"block/furnace_front", "block/furnace_front_off"}, {"block/anvil_top", "block/anvil_top_damaged_0"}, {"block/chipped_anvil_top", "block/anvil_top_damaged_1"}, {"block/damaged_anvil_top", "block/anvil_top_damaged_2"},
        {"block/beacon_cage", "block/beacon"}, {"block/piston_top", "block/piston_top_normal"}, {"block/dispenser_front", "block/dispenser_front_horizontal"}, {"block/dropper_front", "block/dropper_front_horizontal"},
        {"block/nether_quartz_ore", "block/quartz_ore"}, {"block/chiseled_quartz_block", "block/quartz_block_chiseled"}, {"block/brown_mushroom", "block/mushroom_brown"}, {"block/red_mushroom", "block/mushroom_red"},
        {"block/iron_trapdoor_face", "block/iron_trapdoor"}, {"block/smooth_stone", "block/stone_slab_top"}, {"block/carved_pumpkin", "block/pumpkin_face_off"}, {"block/jack_o_lantern", "block/pumpkin_face_on"},
        {"block/podzol", "block/dirt_podzol_top"}, {"item/sugar_cane", "item/reeds"}, {"item/fire_charge", "item/fireball"}, {"item/enchanted_golden_apple", "item/apple_golden"}, {"item/totem_of_undying", "item/totem"}};
    for (auto& a : CA) { auto it = pkIdx.find(a[1]); if (it != pkIdx.end() && !pkIdx.count(a[0])) pkIdx[a[0]] = it->second; }
    for (auto& kv : std::unordered_map<std::string, int>(gPackId)) {   // itens do jogo ligados a textura do pack (wood_pickaxe -> wooden_pickaxe, record_13 -> music_disc_13)
        const size_t sl = kv.first.find('/'); if (sl == std::string::npos) continue;
        const std::string al = kv.first.substr(0, sl + 1) + (kv.first.compare(0, 5, "item/") == 0 ? pk112Item(kv.first.substr(sl + 1)) : pk112(kv.first.substr(sl + 1)));
        if (!gPackId.count(al)) gPackId[al] = kv.second;
    }
    char line[300]; int cur = -1, n = 0, miss = 0;
    while (fgets(line, sizeof line, f)) {
        std::string s = line; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        if (s.empty() || s[0] == '#') continue;
        if (s[0] == '@') {
            const size_t sp = s.find(' '); const std::string nm = s.substr(1, sp == std::string::npos ? std::string::npos : sp - 1); cur = -1;
            for (int t = 0; t < NTAB; t++) if (NM[t][0] && nm == NM[t]) cur = t;
            if (cur >= 0 && sp != std::string::npos) { const int ic = creativeResolve(s.substr(sp + 1), "", pkIdx); if (ic) TABS[cur].icon = ic; }
            continue;
        }
        if (cur < 0) continue;
        const size_t bar = s.find('|');
        { const std::string fl0 = s.substr(0, bar); if (fl0 == "item/potion" || fl0 == "item/splash_potion" || fl0 == "item/lingering_potion") continue; }   // patchPot.4: garrafa crua do pack (sem cor/efeito, nao usavel); as pocoes reais entram em potionCreative()
        const int id = creativeResolve(s.substr(0, bar), bar == std::string::npos ? "" : s.substr(bar + 1), pkIdx);
        if (id) { if (std::find(gCreative[cur].begin(), gCreative[cur].end(), id) == gCreative[cur].end()) { gCreative[cur].push_back(id); n++; } } else { miss++; if (getenv("BL_CREATIVE_DEBUG")) fprintf(stderr, "CFGMISS %d %s\n", cur, s.c_str()); }   // faces redirecionadas podem repetir o bloco real na mesma aba
    }
    potionCreative(gCreative[TAB_BREW]);   // patchBrew.1: pocoes (beber/arremessar/persistente) na aba Pocoes
    { const int ic = potItem(0, "healing"); if (ic) TABS[TAB_BREW].icon = ic; }   // patchPot.4: icone da aba = pocao colorida
    fclose(f); rsMapItems(); printf("Criativo 1.12.2: %d itens nas abas (%d sem textura no pack)\n", n, miss);
}
static void propsRegisterTab();
static void registerPackTab() {
    propsRegisterTab();   // patchProp.1b: aba "Decocraft" (props.bin) mesmo sem resource pack
    if (PK.empty()) return;
    loadCreativeCfg();
    pkApplyItemData(); rsNames(); ptbrApply();   // nomes em portugues e tamanho de pilha (item_data.inc)
    std::unordered_set<int> inTab; for (auto& v : gCreative) for (int x : v) inTab.insert(x);
    ModDef m; m.name = "Resource pack"; m.ids.reserve(PK.size()); ModDef mc; mc.name = "Chisel";   // aba extra (pagina 2): o que o pack tem alem do 1.12.2
    for (int i = 0; i < (int)PK.size(); i++) { if (PK[i].fold.empty()) PK[i].fold = foldAscii(PK[i].name.c_str()); if (PK[i].file.rfind("chisel/", 0) == 0) { mc.ids.push_back(PK0 + i); continue; } if (!inTab.count(PK0 + i) && !gCreativeBase.count(PK0 + i) && !pkIsFaceOnly(PK0 + i)) m.ids.push_back(PK0 + i); }   // faces soltas (cactus_bottom, furnace_front...) nao viram item
    TABS.push_back({"Resource pack", IT_DIAMOND, false, false, -1});
    MODS.push_back(m);
    if (!mc.ids.empty()) { TABS.push_back({"Chisel", mc.ids[0], false, false, -1}); MODS.push_back(mc); }
}
