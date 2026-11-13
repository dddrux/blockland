// ================= ponte de resource packs do Minecraft =================
// Le packs no formato Java (assets/minecraft/textures/...) direto de um .zip ou de uma pasta,
// em assets/resourcepacks/. Texturas de outras resolucoes sao reduzidas para 16x16 (atlas) e
// animacoes (tira vertical) usam o primeiro quadro. Nomes antigos (pre-1.13) tambem sao aceitos.
#include <map>
#include <string>
#include <list>
struct McPack {
    std::list<McPack> under;   // patchRP.3: camadas de baixo (o pack de cima vence; o de baixo completa o que faltar)
    bool ok = false, zip = false; std::string dir, prefix, name;
    std::vector<uint8_t> blob; struct E { uint32_t off, cs, us; int method; }; std::map<std::string, E> ents;
    static bool readAll(const std::string& p, std::vector<uint8_t>& d) {
        FILE* f = fopen(p.c_str(), "rb"); if (!f) return false;
        uint8_t tmp[65536]; size_t r; d.clear();
        while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) d.insert(d.end(), tmp, tmp + r);
        fclose(f); return true;
    }
    static uint32_t u16(const uint8_t* p) { return p[0] | p[1] << 8; }
    static uint32_t u32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
    bool open(const std::string& path) {
        ok = false; ents.clear(); blob.clear(); prefix.clear(); name = path;
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec)) { zip = false; dir = path; if (dir.back() != '/') dir += '/'; ok = true; return true; }
        zip = true; if (!readAll(path, blob) || blob.size() < 22) return false;
        size_t e = blob.size() - 22; bool found = false;
        for (size_t i = 0; i <= std::min<size_t>(e, 70000); i++) if (u32(&blob[e - i]) == 0x06054b50) { e -= i; found = true; break; }
        if (!found) return false;
        uint32_t n = u16(&blob[e + 10]), cd = u32(&blob[e + 16]);
        for (uint32_t k = 0; k < n && cd + 46 <= blob.size(); k++) {
            const uint8_t* h = &blob[cd]; if (u32(h) != 0x02014b50) break;
            uint32_t nl = u16(h + 28), xl = u16(h + 30), cl = u16(h + 32);
            if (cd + 46 + nl > blob.size()) break;
            std::string nm((const char*)h + 46, nl);
            ents[nm] = {u32(h + 42), u32(h + 20), u32(h + 24), (int)u16(h + 10)};
            cd += 46 + nl + xl + cl;
        }
        for (auto& kv : ents) { size_t p = kv.first.find("pack.mcmeta"); if (p != std::string::npos && (p == 0 || kv.first[p - 1] == '/')) { prefix = kv.first.substr(0, p); break; } }
        ok = !ents.empty(); return ok;
    }
    static std::string legacyRel(const std::string& rel) {   // packs 1.12: textures/blocks/ e textures/items/ (1.13+ = block/ e item/)
        std::string r = rel; size_t p;
        if ((p = r.find("/textures/block/")) != std::string::npos) r.replace(p, 16, "/textures/blocks/");
        else if ((p = r.find("/textures/item/")) != std::string::npos) r.replace(p, 15, "/textures/items/");
        return r; }
    bool get(const std::string& rel, std::vector<uint8_t>& out) const { if (getOne(rel, out)) return true; for (const auto& u : under) if (u.getOne(rel, out)) return true; return false; }   /* patchRP.3 */
    bool getOne(const std::string& rel, std::vector<uint8_t>& out) const {
        if (!ok) return false;
        { const std::string lg = legacyRel(rel); if (lg != rel && zip && !ents.count(prefix + rel) && ents.count(prefix + lg)) return getOne(lg, out);
          if (lg != rel && !zip && !std::filesystem::exists(dir + rel) && std::filesystem::exists(dir + lg)) return getOne(lg, out); }
        if (!zip) return readAll(dir + rel, out);
        auto it = ents.find(prefix + rel); if (it == ents.end()) return false;
        const E& e = it->second; if ((size_t)e.off + 30 > blob.size() || u32(&blob[e.off]) != 0x04034b50) return false;
        size_t st = (size_t)e.off + 30 + u16(&blob[e.off + 26]) + u16(&blob[e.off + 28]);
        if (st + e.cs > blob.size()) return false;
        if (e.method == 0) { out.assign(blob.begin() + st, blob.begin() + st + e.cs); return true; }
        if (e.method != 8) return false;
        std::vector<uint8_t> in; in.reserve(e.cs + 2); in.push_back(0x78); in.push_back(0x9c); in.insert(in.end(), blob.begin() + st, blob.begin() + st + e.cs);
        return inflateZ(in.data(), in.size(), out);   // zip = deflate "cru"; o cabecalho zlib falso e ignorado pelo inflateZ
    }
    // nomes (sem ".png") dos PNGs diretamente dentro de rel (ex.: "assets/minecraft/textures/block/"), em ordem alfabetica; sem subpastas
    std::vector<std::string> listPng(const std::string& rel) const { auto r = listPng1(rel); if (under.empty()) return r; for (const auto& u : under) { auto x = u.listPng1(rel); r.insert(r.end(), x.begin(), x.end()); } std::sort(r.begin(), r.end()); r.erase(std::unique(r.begin(), r.end()), r.end()); return r; }   /* patchRP.3: uniao das camadas */
    std::vector<std::string> listPng1(const std::string& rel) const {
        std::vector<std::string> r; if (!ok) return r;
        { const std::string lg = legacyRel(rel); if (lg != rel) { r = listPng0(rel); if (r.empty()) r = listPng0(lg); return r; } }
        return listPng0(rel); }
    std::vector<std::string> listPng0(const std::string& rel) const {
        std::vector<std::string> r; if (!ok) return r;
        if (!zip) {
            std::error_code ec;
            for (auto& e : std::filesystem::directory_iterator(dir + rel, ec)) { std::string n = e.path().filename().string(); if (!e.is_directory(ec) && n.size() > 4 && n.compare(n.size() - 4, 4, ".png") == 0) r.push_back(n.substr(0, n.size() - 4)); }
            std::sort(r.begin(), r.end()); return r;
        }
        const std::string pre = prefix + rel;
        for (auto it = ents.lower_bound(pre); it != ents.end() && it->first.compare(0, pre.size(), pre) == 0; ++it) {
            const std::string n = it->first.substr(pre.size());
            if (n.size() > 4 && n.find('/') == std::string::npos && n.compare(n.size() - 4, 4, ".png") == 0) r.push_back(n.substr(0, n.size() - 4));
        }
        return r;   // std::map ja esta ordenado
    }
};
static McPack gPack;
static std::mutex gPackMx;   // patchRP.3: camadas de packs (gPack = topo; gPack.under = os de baixo): texturas E sons
static std::vector<std::string> packNames(const std::string& root, const std::string&) { std::vector<std::string> o, nm; std::error_code ec; for (auto& e : std::filesystem::directory_iterator(root, ec)) { const std::string n = e.path().filename().string(), x = n.size() > 4 ? n.substr(n.size() - 4) : ""; if (e.is_directory() || x == ".zip" || x == ".ZIP") nm.push_back(n); } std::sort(nm.begin(), nm.end()); char ln[256]; if (FILE* f = fopen((root + "pack.cfg").c_str(), "r")) { while (fgets(ln, sizeof ln, f)) { char v[200]; if (sscanf(ln, " pack = %199[^\r\n]", v) == 1 && std::find(nm.begin(), nm.end(), v) != nm.end() && std::find(o.begin(), o.end(), v) == o.end()) o.push_back(v); } fclose(f); } for (auto& n : nm) if (std::find(o.begin(), o.end(), n) == o.end()) o.push_back(n); return o; }   // ordem = linhas pack= do pack.cfg (de cima p/ baixo), depois o resto em ordem alfabetica
static void packChoose(const std::string&, std::string&) {}
static void packLoadSnd(const std::string& root, const std::string& active) { gPack.under.clear(); for (const auto& n : packNames(root, active)) { if (n == active) continue; gPack.under.emplace_back(); if (!gPack.under.back().open(root + n)) gPack.under.pop_back(); } }   // abre as camadas de baixo
static bool packGetFile(const std::string& rel, std::vector<uint8_t>& out) { std::lock_guard<std::mutex> lk(gPackMx); return gPack.get(rel, out); }
static bool packHasFile(const std::string& rel) { if (!gPack.ok) return true; /* pack ainda nao aberto: nao descarta evento */ auto has = [&](const McPack& p) { return p.zip ? p.ents.count(p.prefix + rel) > 0 : std::filesystem::exists(p.dir + rel); }; if (has(gPack)) return true; for (auto& p : gPack.under) if (has(p)) return true; return false; }
#include <list>
// PNG da pasta assets/minecraft/textures/<rel> do pack ativo
// patchRP.4: PRIORIDADE POR CAMADA. Camada 0 = topo (gPack); 1.. = gPack.under. gPackOnly >= 0 restringe a leitura de texturas a UMA camada,
// para resolver "todos os nomes possiveis" camada por camada (o pack de cima vence mesmo usando outro formato/nome de arquivo).
static thread_local int gPackOnly = -1;
static int packLayerCount() { return gPack.ok ? 1 + (int)gPack.under.size() : 0; }
static const McPack* packLayerAt(int k) { if (k < 0 || !gPack.ok) return nullptr; if (k == 0) return &gPack; if (k - 1 >= (int)gPack.under.size()) return nullptr; auto it = gPack.under.begin(); std::advance(it, k - 1); return &*it; }
static bool packRGBA(const std::string& rel, std::vector<uint8_t>& px, int& w, int& h) {
    std::vector<uint8_t> d;
    if (gPackOnly >= 0) { const McPack* L = packLayerAt(gPackOnly); if (!L || !L->getOne("assets/minecraft/textures/" + rel, d)) return false; }
    else if (!gPack.get("assets/minecraft/textures/" + rel, d)) return false;
    return loadPNGMem(d, px, w, h);
}
// reduz/amplia (vizinho mais proximo) para tw x th; media ponderada pelo alfa ao reduzir
static void scaleRGBA(const std::vector<uint8_t>& s, int w, int h, uint8_t* o, int tw, int th) {
    for (int y = 0; y < th; y++) for (int x = 0; x < tw; x++) {
        int x0 = x * w / tw, x1 = std::max(x0 + 1, (x + 1) * w / tw), y0 = y * h / th, y1 = std::max(y0 + 1, (y + 1) * h / th);
        if (w <= tw && h <= th) { memcpy(o + (y * tw + x) * 4, &s[((size_t)y0 * w + x0) * 4], 4); continue; }
        float r = 0, g = 0, b = 0, a = 0; int c = 0;
        for (int j = y0; j < y1; j++) for (int i = x0; i < x1; i++) { const uint8_t* p = &s[((size_t)j * w + i) * 4]; float al = p[3]; r += p[0] * al; g += p[1] * al; b += p[2] * al; a += al; c++; }
        uint8_t* q = o + (y * tw + x) * 4;
        if (a > 0) { q[0] = (uint8_t)(r / a); q[1] = (uint8_t)(g / a); q[2] = (uint8_t)(b / a); q[3] = (uint8_t)(a / c); } else q[0] = q[1] = q[2] = q[3] = 0;
    }
}
// carrega "block/xxx.png" / "item/xxx.png" (relativo a assets/minecraft/textures/) como tile 16x16 (tira vertical de animacao: 1o quadro)
static bool packTileRel(const std::string& rel, uint8_t* out) {
    std::vector<uint8_t> px; int w, h;
    if (!packRGBA(rel, px, w, h) || w < 1 || h < 1) return false;
    int S = std::min(w, h);
    std::vector<uint8_t> f((size_t)S * S * 4); for (int y = 0; y < S; y++) memcpy(&f[(size_t)y * S * 4], &px[(size_t)y * w * 4], (size_t)S * 4);
    scaleRGBA(f, S, S, out, 16, 16); return true;
}
static std::vector<uint8_t> gWaterFrames; static int gWaterNF = 0;   // patchOpt.2: frames 16x16 da agua (tira vertical do pack)
static void packFrames16(const std::string& rel, std::vector<uint8_t>& out, int& nf) {
    std::vector<uint8_t> px; int w, h; nf = 0; out.clear();
    if (!packRGBA(rel, px, w, h) || w < 1 || h < w) return;
    const int S = w, n = std::min(h / S, 64); std::vector<uint8_t> f((size_t)S * S * 4); out.resize((size_t)n * 16 * 16 * 4);
    for (int k = 0; k < n; k++) { memcpy(f.data(), &px[(size_t)k * S * S * 4], (size_t)S * S * 4); scaleRGBA(f, S, S, &out[(size_t)k * 1024], 16, 16); }
    nf = n;
}
static std::string gLastPack;   // ultimo "kind/nome" carregado com sucesso por packTile16
static std::unordered_map<std::string, int> gPackId;   // "block/stone" / "item/apple" -> id do jogo (bloco/item real, nao item extra do pack)
static std::unordered_set<std::string> gPackUsed;   // "block/xxx" / "item/xxx" ja ligados a um bloco/item do jogo (nao viram item extra)
// "kind" = "block" ou "item"; names = alternativas separadas por '|' (nome moderno primeiro, antigo depois)
// nomes 1.13 que o jogo pede -> nome do pack 1.12 (so usado quando o 1.13 nao existe)
static const char* packLegacyAlias(const std::string& n) {
    static const std::unordered_map<std::string, const char*> M = {
        {"redstone", "redstone_dust"}, {"book", "book_normal"}, {"beef", "beef_raw"}, {"bow", "bow_standby"},
        {"pig_spawn_egg", "spawn_egg"}, {"chicken_spawn_egg", "spawn_egg"}, {"cow_spawn_egg", "spawn_egg"}, {"sheep_spawn_egg", "spawn_egg"},
        {"grass_block_side_overlay", "grass_side_overlay"}, {"porkchop", "porkchop_raw"}, {"cooked_beef", "beef_cooked"}, {"cooked_porkchop", "porkchop_cooked"},
        {"chicken", "chicken_raw"}, {"cooked_chicken", "chicken_cooked"}, {"mutton", "mutton_raw"}, {"cooked_mutton", "mutton_cooked"}, {"rabbit", "rabbit_raw"},
        {"cooked_rabbit", "rabbit_cooked"}, {"cod", "fish_cod_raw"}, {"salmon", "fish_salmon_raw"}, {"cooked_cod", "fish_cod_cooked"}, {"cooked_salmon", "fish_salmon_cooked"},
        {"gold_ingot", "gold_ingot"}, {"clock", "clock_00"}, {"compass", "compass_00"}, {"oak_door", "door_wood"}, {"iron_door", "door_iron"}, {"oak_boat", "boat"},
        {"writable_book", "book_writable"}, {"written_book", "book_written"}, {"enchanted_book", "book_enchanted"}, {"glistering_melon_slice", "melon_speckled"},
        {"melon_slice", "melon"}, {"slime_ball", "slimeball"}, {"gunpowder", "gunpowder"}, {"charcoal", "charcoal"}, {"nether_brick", "netherbrick"}, {"sugar_cane", "reeds"},
        {"lily_pad", "waterlily"}, {"oak_sign", "sign"}, {"flint_and_steel", "flint_and_steel"}, {"water_bucket", "bucket_water"}, {"lava_bucket", "bucket_lava"},
        {"milk_bucket", "bucket_milk"}, {"bucket", "bucket_empty"}, {"fishing_rod", "fishing_rod_uncast"}, {"ender_eye", "ender_eye"}, {"experience_bottle", "experience_bottle"},
        {"minecart", "minecart_normal"}, {"chest_minecart", "minecart_chest"}, {"furnace_minecart", "minecart_furnace"}, {"tnt_minecart", "minecart_tnt"}, {"hopper_minecart", "minecart_hopper"},
        {"command_block_minecart", "minecart_command_block"}, {"filled_map", "map_filled"}, {"map", "map_empty"}, {"golden_apple", "apple_golden"}, {"golden_carrot", "carrot_golden"},
        {"poisonous_potato", "potato_poisonous"}, {"baked_potato", "potato_baked"}, {"carrot", "carrot"}, {"potato", "potato"}, {"pumpkin_pie", "pumpkin_pie"},
        {"dandelion_yellow", "dye_powder_yellow"}, {"iron_horse_armor", "iron_horse_armor"},
    };
    auto it = M.find(n); return it == M.end() ? nullptr : it->second;
}
static std::unordered_map<std::string, int> gCoreTile;   // "block/sandstone_top" -> tile do atlas ja carregado para um bloco do jogo (laje/escada reusam; tile novo depois do upload do atlas ficaria em branco)
static std::vector<std::string> gPackMiss;   // "kind/nomes" que nenhum pack resolveu (auditoria)
// patchV.8: ovos de spawn do 1.12 = spawn_egg (base, tingido com a cor 1) + spawn_egg_overlay (manchas, cor 2); o pack nao traz um PNG por mob
static bool packSpawnEgg(const std::string& kind, const std::string& n, uint8_t* out) {
    static const struct { const char* k; unsigned a, b; } C[] = {
        {"creeper",0x0DA70B,0},{"skeleton",0xC1C1C1,0x494949},{"spider",0x342D27,0xA80E0E},{"zombie",0x00AFAF,0x799C65},{"slime",0x51A03E,0x7EBF6E},{"ghast",0xF9F9F9,0xBCBCBC},
        {"zombie_pigman",0xEA9393,0x4C7129},{"enderman",0x161616,0},{"cave_spider",0x0C424E,0xA80E0E},{"silverfish",0x6E6E6E,0x303030},{"blaze",0xF6B201,0xFFF87E},
        {"magma_cube",0x340000,0xFCFC00},{"bat",0x4C3E30,0xAFAFAF},{"witch",0x340000,0x51A03E},{"endermite",0x161616,0x6E6E6E},{"guardian",0x5A8272,0xF17D30},
        {"shulker",0x946794,0x4D3852},{"pig",0xF0A5A2,0xDB635F},{"sheep",0xE7E7E7,0xFFB5B5},{"cow",0x443626,0xA1A1A1},{"chicken",0xA1A1A1,0xFF0000},{"squid",0x223B4D,0x708899},
        {"wolf",0xD7D3D3,0xCEAF96},{"mooshroom",0xA00F10,0xB7B7B7},{"ocelot",0xEFDE7D,0x564434},{"horse",0xC09E7D,0xEEE500},{"rabbit",0x995F40,0x734831},
        {"polar_bear",0xF2F2F2,0x959590},{"llama",0xC09E7D,0x995F40},{"parrot",0x0DA70B,0xFF0000},{"villager",0x563C33,0xBD8B72},{"vex",0x8B9AA7,0x7A8FA0},
        {"evoker",0x959B9B,0x1E1C1A},{"vindicator",0x959B9B,0x284D4D},{"elder_guardian",0xCDCDB6,0x707070},{"wither_skeleton",0x141414,0x474D4D},{"stray",0x617677,0xDDEAEA},
        {"husk",0x7A7A6A,0xE0D4A8},{"zombie_villager",0x563C33,0x799C65},{"skeleton_horse",0x68685C,0xD1D1C9},{"zombie_horse",0x2B4B3B,0xA0B9A0},{"donkey",0x534237,0x867566},{"mule",0x1B0200,0x51331D}};
    if (n.size() <= 10 || n.compare(n.size() - 10, 10, "_spawn_egg") != 0) return false;
    std::string k = n.substr(0, n.size() - 10); if (k == "zombified_piglin") k = "zombie_pigman";
    unsigned c1 = 0x888888, c2 = 0xCCCCCC; for (auto& e : C) if (k == e.k) { c1 = e.a; c2 = e.b; break; }
    uint8_t base[1024], ov[1024]; bool okb = false, oko = false;
    for (const char* suf : {"s", ""}) { if (!okb) okb = packTileRel(kind + suf + "/spawn_egg.png", base); if (!oko) oko = packTileRel(kind + suf + "/spawn_egg_overlay.png", ov); }
    if (!okb) return false;
    for (int i = 0; i < 256; ++i) {
        uint8_t* p = base + i * 4; for (int c = 0; c < 3; ++c) p[c] = (uint8_t)(p[c] * ((c1 >> (16 - 8 * c)) & 255) / 255);
        if (oko && ov[i * 4 + 3] > 0) { const float al = ov[i * 4 + 3] / 255.f; for (int c = 0; c < 3; ++c) { const float t = ov[i * 4 + c] * ((c2 >> (16 - 8 * c)) & 255) / 255.f; p[c] = (uint8_t)(p[c] * (1 - al) + t * al); } p[3] = std::max<int>(p[3], ov[i * 4 + 3]); }
    }
    memcpy(out, base, 1024); gPackUsed.insert(kind + "/spawn_egg"); gPackUsed.insert(kind + "/spawn_egg_overlay"); return true;
}
static bool packTile16Body(const char* kind, const char* names, uint8_t* out) {
    std::string all = names; size_t a = 0;
    while (a <= all.size()) {
        size_t b = all.find('|', a); if (b == std::string::npos) b = all.size();
        std::string n = all.substr(a, b - a); a = b + 1;
        for (const char* suf : {"", "s"}) {
            if (!packTileRel(std::string(kind) + suf + "/" + n + ".png", out) && packSpawnEgg(std::string(kind), n, out)) { gPackUsed.insert(std::string(kind) + "/" + n); gLastPack = std::string(kind) + "/" + n; return true; }
            if (!packTileRel(std::string(kind) + suf + "/" + n + ".png", out)) { const char* lg = packLegacyAlias(n);   // pack 1.12: nome antigo
                if (lg && packTileRel(std::string(kind) + suf + "/" + lg + ".png", out)) { gPackUsed.insert(std::string(kind) + "/" + lg); gLastPack = std::string(kind) + "/" + n; return true; } }
            if (!packTileRel(std::string(kind) + suf + "/" + n + ".png", out)) continue;
            gPackUsed.insert(std::string(kind) + "/" + n); gLastPack = std::string(kind) + "/" + n; return true;
        }
    }
    return false;
}
static bool packTile16(const char* kind, const char* names, uint8_t* out) {   // patchRP.4: camada por camada (topo primeiro); em cada uma tenta TODOS os nomes
    const int n = packLayerCount();
    for (int k = 0; k < n; k++) { gPackOnly = k; const bool ok = packTile16Body(kind, names, out); gPackOnly = -1; if (ok) return true; }
    gPackMiss.push_back(std::string(kind) + "/" + names); return false;
}
