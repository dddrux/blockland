// ---------- props.inc (patchProp.1): leitor do formato BLP1/props.bin gerado por conversor/modconv ----------
// Cada mod convertido (ex.: Decocraft) traz mods/<nome>/props.bin + paginas de textura. Aqui so ha LEITURA e geometria (sem GL):
// o desenho (malha do chunk + textura propria) vem em patchProp.2. Formato descrito em doc/MODCONV.md.
struct PropFace { int8_t slot = -1; float u0 = 0, v0 = 0, u1 = 0, v1 = 0; };   // slot -1 = face oculta; UV normalizada na textura do slot
struct PropBox { float g[10]; PropFace f[6]; };   // g = cx,cy,cz, hx,hy,hz, qw,qx,qy,qz (blocos; x/z centrados no bloco, y=0 no chao)
struct PropModel { float bb[6]; std::vector<PropBox> boxes; };
struct PropTex { bool ref = false; std::string name; int page = 0, x = 0, y = 0, w = 0, h = 0; };
// pilha maxima de um item de mod: ferramentas, armas, armaduras, mochilas etc. NAO empilham (1); o resto 64
static int propStackHeur(const std::string& id, const std::string& beh) {
    if (beh == "Chair" || beh == "Bed" || beh == "Container") return 64;
    std::string l; for (char c : id) l += (char)tolower((unsigned char)c);
    static const char* tok[] = {"sword","pickaxe","axe","shovel","spade","hoe","helmet","helm","chestplate","leggings","boots","bow","crossbow","shield","hammer","wrench","backpack","elytra","rod","gun","pistol","rifle","shotgun","smg","launcher","sickle","scythe","saber","sabre","dagger","knife","armor","armour","mace","spear","staff","wand","drill","chainsaw","jetpack","machete","katana","blaster","revolver","minigun","bazooka","flamethrower","crowbar","hatchet","paxel","greatsword","longsword","battleaxe","lance","trident","whip","tool","goggles","vest","tunic","cuirass","gauntlets","exoskeleton","nanosuit","quantumsuit","jacket","pants","shoes","cap","hat","mask",nullptr};
    std::vector<std::string> parts; std::string cur; for (char c : l) { if (isalnum((unsigned char)c)) cur += c; else { if (!cur.empty()) parts.push_back(cur); cur.clear(); } } if (!cur.empty()) parts.push_back(cur);
    for (auto& p : parts) for (int i = 0; tok[i]; i++) { const std::string t = tok[i]; if (p == t) return 1; if (t.size() >= 5 && p.size() > t.size() && p.compare(p.size() - t.size(), t.size(), t) == 0) return 1; }   // ex.: "ironsword", "diamondpickaxe"
    return 64;
}
// patchMod.12: tipo de item na mao pelo id: 2 = arma de fogo, 1 = arma branca/ferramenta, 0 = outro
static bool propTokHit(const std::string& id, const char* const* tok) {
    std::string l; for (char c : id) l += (char)tolower((unsigned char)c);
    std::vector<std::string> parts; std::string cur; for (char c : l) { if (isalnum((unsigned char)c)) cur += c; else { if (!cur.empty()) parts.push_back(cur); cur.clear(); } } if (!cur.empty()) parts.push_back(cur);
    for (auto& p : parts) for (int i = 0; tok[i]; i++) { const std::string t = tok[i]; if (p == t) return true; if (t.size() >= 5 && p.size() > t.size() && p.compare(p.size() - t.size(), t.size(), t) == 0) return true; }
    return false;
}
static int propHeldKindOf(const std::string& id, const std::string& beh) {
    if (beh == "Chair" || beh == "Bed" || beh == "Container") return 0;
    static const char* const gun[] = {"gun","rifle","pistol","shotgun","smg","launcher","blaster","minigun","bazooka","revolver","flamethrower","musket","carbine","sniper","magnum",nullptr};
    static const char* const hand[] = {"sword","pickaxe","axe","shovel","spade","hoe","hammer","wrench","sickle","scythe","saber","sabre","dagger","knife","mace","spear","staff","wand","machete","katana","crowbar","hatchet","paxel","greatsword","longsword","battleaxe","lance","trident","cleaver","drill","chainsaw","rod","scepter","club","baton",nullptr};
    static const char* const notGun[] = {"magazine","ammo","bullet","bullets","clip","casing","slug","slugs","mold","part","parts","barrel","grip","stock","scope","upgrade",nullptr};
    if (propTokHit(id, notGun)) return 0;
    if (propTokHit(id, gun)) return 2;
    if (propTokHit(id, hand)) return 1;
    return 0;
}
struct PropItem { std::string id, name, tab, beh; int model = 0, light = 0; bool nocoll = false; int stack = 64, held = 0, aslot = -1, apts = 0; std::string atex[2]; GLuint aGl[2] = {0, 0}; int8_t aTry[2] = {0, 0}; std::vector<int> tex; };   // stack: 1 p/ arma/armadura/ferramenta (heuristica pelo id, ver propStackHeur)
struct PropPack {
    std::string dir, mod, modid; std::vector<std::string> pages;   // modid (patchConv.4): id do Forge (mod.txt `modid=`)
    std::vector<int> pageW, pageH;
    std::vector<PropTex> tex; std::vector<PropModel> models; std::vector<PropItem> items;
};
static std::vector<PropPack> gPropPacks;
static void propLoadArmor(PropPack& pk);
// cantos de cada face (sx,sy,sz,cu,cv): MESMA tabela de conversor/modconv/pack.py (CORNERS); face 0=-X 1=+X 2=-Y 3=+Y 4=-Z 5=+Z
static const int8_t PROP_CORNERS[6][4][5] = {
    {{-1,1,-1,0,0},{-1,1,1,1,0},{-1,-1,1,1,1},{-1,-1,-1,0,1}}, {{1,1,1,0,0},{1,1,-1,1,0},{1,-1,-1,1,1},{1,-1,1,0,1}},
    {{-1,-1,1,0,0},{1,-1,1,1,0},{1,-1,-1,1,1},{-1,-1,-1,0,1}}, {{-1,1,-1,0,0},{1,1,-1,1,0},{1,1,1,1,1},{-1,1,1,0,1}},
    {{1,1,-1,0,0},{-1,1,-1,1,0},{-1,-1,-1,1,1},{1,-1,-1,0,1}}, {{-1,1,1,0,0},{1,1,1,1,0},{1,-1,1,1,1},{-1,-1,1,0,1}}};
struct PropRd {
    const unsigned char* p; size_t n, i = 0; bool ok = true;
    bool need(size_t k) { if (!ok || i + k > n) { ok = false; return false; } return true; }
    template <class T> T get() { T v{}; if (need(sizeof(T))) { memcpy(&v, p + i, sizeof(T)); i += sizeof(T); } return v; }
    std::string str() { const unsigned l = get<uint16_t>(); std::string s; if (need(l)) { s.assign((const char*)p + i, l); i += l; } return s; }
};
static bool propsParse(const std::vector<unsigned char>& buf, PropPack& o) {
    if (buf.size() < 12 || memcmp(buf.data(), "BLPB", 4) != 0) return false;
    PropRd r{buf.data(), buf.size(), 4};
    if (r.get<uint32_t>() != 1) return false;
    uint32_t np = r.get<uint32_t>(); if (np > 64) return false;
    for (uint32_t k = 0; k < np && r.ok; k++) { o.pages.push_back(r.str()); o.pageW.push_back((int)r.get<uint32_t>()); o.pageH.push_back((int)r.get<uint32_t>()); }
    uint32_t nt = r.get<uint32_t>(); if (nt > 1000000) return false;
    for (uint32_t k = 0; k < nt && r.ok; k++) {
        PropTex t; t.ref = r.get<int8_t>() != 0;
        if (t.ref) t.name = r.str(); else { t.page = r.get<int32_t>(); t.x = r.get<int32_t>(); t.y = r.get<int32_t>(); t.w = r.get<int32_t>(); t.h = r.get<int32_t>(); }
        o.tex.push_back(t);
    }
    uint32_t nm = r.get<uint32_t>(); if (nm > 1000000) return false;
    for (uint32_t k = 0; k < nm && r.ok; k++) {
        PropModel m; for (int j = 0; j < 6; j++) m.bb[j] = r.get<float>();
        uint32_t nb = r.get<uint32_t>(); if (nb > 100000) return false;
        for (uint32_t b = 0; b < nb && r.ok; b++) {
            PropBox bx; for (int j = 0; j < 10; j++) bx.g[j] = r.get<float>();
            for (int f = 0; f < 6; f++) { PropFace& pf = bx.f[f]; pf.slot = r.get<int8_t>(); if (pf.slot >= 0) { pf.u0 = r.get<float>(); pf.v0 = r.get<float>(); pf.u1 = r.get<float>(); pf.v1 = r.get<float>(); } }
            m.boxes.push_back(bx);
        }
        o.models.push_back(std::move(m));
    }
    uint32_t ni = r.get<uint32_t>(); if (ni > 1000000) return false;
    for (uint32_t k = 0; k < ni && r.ok; k++) {
        PropItem it; it.id = r.str(); it.name = r.str(); it.tab = r.str(); it.model = (int)r.get<uint32_t>(); it.light = (int)r.get<uint32_t>(); it.nocoll = r.get<uint8_t>() != 0; it.beh = r.str(); it.held = propHeldKindOf(it.id, it.beh); it.stack = (strcasecmp(o.mod.c_str(), "decocraft") == 0 || strcasecmp(o.mod.c_str(), "cfm") == 0 || strcasecmp(o.mod.c_str(), "bibliocraft") == 0) ? 64 : propStackHeur(it.id, it.beh);   // moveis/decoracao: sempre 64
        const unsigned nx = r.get<uint8_t>(); for (unsigned j = 0; j < nx && r.ok; j++) it.tex.push_back(r.get<int32_t>());
        o.items.push_back(std::move(it));
    }
    if (!r.ok || r.i != buf.size()) return false;
    for (auto& it : o.items) { if (it.model < 0 || it.model >= (int)o.models.size()) return false; for (int t : it.tex) if (t >= (int)o.tex.size() || (t < 0 && -1 - t >= (int)o.tex.size())) return false; }
    for (auto& t : o.tex) if (!t.ref && (t.page < 0 || t.page >= (int)o.pages.size())) return false;
    return true;
}
static bool propsLoad(const std::string& path, PropPack& o) {
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
    std::vector<unsigned char> b; unsigned char tmp[65536]; size_t n;
    while ((n = fread(tmp, 1, sizeof tmp, f)) > 0) b.insert(b.end(), tmp, tmp + n);
    fclose(f); return propsParse(b, o);
}
// quad de uma face da caixa em coordenadas do bloco (x/z centrados, y=0 no chao): out[k] = {x,y,z,u,v}, u/v normalizados na textura do slot
static void propsFaceQuad(const PropBox& b, int f, float out[4][5]) {
    const float w = b.g[6], x = b.g[7], y = b.g[8], z = b.g[9];
    for (int k = 0; k < 4; k++) {
        const int8_t* c = PROP_CORNERS[f][k]; const float v[3] = {c[0] * b.g[3], c[1] * b.g[4], c[2] * b.g[5]};
        const float t0 = 2 * (y * v[2] - z * v[1]), t1 = 2 * (z * v[0] - x * v[2]), t2 = 2 * (x * v[1] - y * v[0]);   // rotacao pelo quaternio
        out[k][0] = b.g[0] + v[0] + w * t0 + (y * t2 - z * t1); out[k][1] = b.g[1] + v[1] + w * t1 + (z * t0 - x * t2); out[k][2] = b.g[2] + v[2] + w * t2 + (x * t1 - y * t0);
        out[k][3] = c[3] ? b.f[f].u1 : b.f[f].u0; out[k][4] = c[4] ? b.f[f].v1 : b.f[f].v0;
    }
}
// procura mods/*/mod.txt com a linha "props <arquivo>" e carrega; devolve quantos pacotes
// patchConv.4: dependencias.txt (conversor): `modid obrigatoria|opcional status`; avisa no log quando o mod exigido nao esta carregado
static std::vector<std::string> gPropMissingDeps;
static void propDepsCheck() {
    gPropMissingDeps.clear();
    for (const PropPack& pk : gPropPacks) { FILE* f = fopen((pk.dir + "/dependencias.txt").c_str(), "r"); if (!f) continue; char ln[256];
        while (fgets(ln, sizeof ln, f)) { char dep[128] = "", kind[32] = ""; if (sscanf(ln, "%127s %31s", dep, kind) < 2) continue;
            bool have = false; for (const PropPack& o : gPropPacks) if (o.modid == dep || o.mod == dep) { have = true; break; }
            if (have || std::string(kind) != "required") continue;
            gPropMissingDeps.push_back(pk.mod + " -> " + dep); logWarn("[MODS] %s precisa do mod '%s' que nao esta instalado (coloque o .jar em mods/ e rode tools/instalar_mods.sh)", pk.mod.c_str(), dep); }
        fclose(f); }
}
static int propsLoadAll() {
    namespace fs = std::filesystem; std::error_code ec; int n = 0;
    if (!fs::is_directory("mods", ec)) return 0;
    bool anyConv = false; for (auto& e : fs::directory_iterator("mods", ec)) if (e.is_directory(ec) && fs::exists(e.path() / "props.bin", ec)) anyConv = true;   // patchMod.17: o .jar fica em mods/ depois de convertido; so avisa se nada foi convertido
    if (!anyConv) for (auto& e : fs::directory_iterator("mods", ec)) if (e.is_regular_file(ec) && e.path().extension() == ".jar") { logWarn("[MODS] %s e um .jar nao convertido: rode tools/instalar_mods.sh (blocos/itens so aparecem depois)", e.path().filename().string().c_str()); break; }   // patchMod.16
    std::vector<fs::path> dirs; for (auto& e : fs::directory_iterator("mods", ec)) if (e.is_directory(ec)) dirs.push_back(e.path());
    {   // patchMod.8: ids de prop dependem da ordem dos pacotes; os 4 primeiros (ja em mundos/inventarios salvos) mantem a posicao, os demais vem depois em ordem alfabetica
        static const char* LEG[] = {"cgm", "decocraft", "techguns", "thuttech"};
        auto rk = [&](const fs::path& p) { const std::string n = p.filename().string(); for (int i = 0; i < 4; i++) if (n == LEG[i]) return i; return 4; };
        std::sort(dirs.begin(), dirs.end(), [&](const fs::path& x, const fs::path& y) { const int rx = rk(x), ry = rk(y); return rx != ry ? rx < ry : x < y; }); }
    for (auto& d : dirs) {
        if (modIsOff(d.filename().string())) continue;   // patchMod.9: mod desligado nao carrega props
        FILE* fp = fopen((d / "mod.txt").string().c_str(), "r"); if (!fp) continue;
        char line[512]; std::string file, name = d.filename().string(), mid;
        while (fgets(line, sizeof line, fp)) { std::string s = line; while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back(); if (s.rfind("props ", 0) == 0) file = s.substr(6); else if (s.rfind("name=", 0) == 0) name = s.substr(5); else if (s.rfind("modid=", 0) == 0) mid = s.substr(6); }
        fclose(fp); if (file.empty()) continue;
        PropPack pk; pk.dir = d.string(); pk.mod = name; pk.modid = mid;
        if (!propsLoad((d / file).string(), pk)) { fprintf(stderr, "props: %s invalido\n", (d / file).string().c_str()); continue; }
        propLoadArmor(pk);
        printf("Props carregados: %s (%d modelos, %d itens)\n", name.c_str(), (int)pk.models.size(), (int)pk.items.size());
        gPropPacks.push_back(std::move(pk)); n++;
    }
    propDepsCheck();
    return n;
}

// ---------- patchProp.1b: ids + aba do criativo ----------
static std::vector<int> gPropBase;   // primeiro id de cada pacote (gPropPacks[i]); id = PROP0 + base + indice do item
static const PropItem* propItem(int id, int* pack = nullptr) {
    if (!isProp(id)) return nullptr; const int k = id - PROP0;
    for (size_t i = 0; i < gPropPacks.size(); i++) { const int b = gPropBase[i]; if (k >= b && k < b + (int)gPropPacks[i].items.size()) { if (pack) *pack = (int)i; return &gPropPacks[i].items[k - b]; } }
    return nullptr;
}
static const char* propName(int id) { const PropItem* it = propItem(id); return it ? it->name.c_str() : ""; }
// patchConv.3: conexoes.txt (conversor): `id|grupo|flags|v0,...,v15` (mascara N=1 E=2 S=4 W=8; v = id do item-variante, `-` = o proprio); flag s = liga tambem em bloco solido
struct PropConnR { std::string grp; bool solid = false; int var[16]; };
static std::unordered_map<int, PropConnR> gConn;
static const PropConnR* propConnOf(int id) { if (gConn.empty()) return nullptr; auto it = gConn.find(id); return it == gConn.end() ? nullptr : &it->second; }
static std::unordered_map<int, int> gOpenV;   // patchOpen.1: abertos.txt (`id|variante`): prop com estado aberto (portao); `on` = fechado (1) / aberto (0)
static int propOpenOf(int id) { if (gOpenV.empty()) return 0; auto it = gOpenV.find(id); return it == gOpenV.end() ? 0 : it->second; }
static void propConnLoad() {
    gConn.clear(); gOpenV.clear();
    for (size_t p = 0; p < gPropPacks.size(); p++) { FILE* f = fopen((gPropPacks[p].dir + "/abertos.txt").c_str(), "r"); if (!f) continue;
        std::unordered_map<std::string, int> ix; for (size_t j = 0; j < gPropPacks[p].items.size(); j++) ix[gPropPacks[p].items[j].id] = (int)j;
        char ln[512]; while (fgets(ln, sizeof ln, f)) { std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back(); const size_t b = s.find('|');
            if (b == std::string::npos || !ix.count(s.substr(0, b)) || !ix.count(s.substr(b + 1))) continue; gOpenV[PROP0 + gPropBase[p] + ix[s.substr(0, b)]] = PROP0 + gPropBase[p] + ix[s.substr(b + 1)]; }
        fclose(f); }
    for (size_t p = 0; p < gPropPacks.size(); p++) { FILE* f = fopen((gPropPacks[p].dir + "/conexoes.txt").c_str(), "r"); if (!f) continue;
        std::unordered_map<std::string, int> ix; for (size_t j = 0; j < gPropPacks[p].items.size(); j++) ix[gPropPacks[p].items[j].id] = (int)j;
        char ln[2048]; int n = 0;
        while (fgets(ln, sizeof ln, f)) { std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
            std::vector<std::string> v; size_t a = 0; for (;;) { size_t b = s.find('|', a); v.push_back(s.substr(a, b == std::string::npos ? b : b - a)); if (b == std::string::npos) break; a = b + 1; }
            if (v.size() < 4 || !ix.count(v[0])) continue;
            const int self = PROP0 + gPropBase[p] + ix[v[0]]; PropConnR r; r.grp = v[1]; r.solid = v[2].find('s') != std::string::npos;
            for (int k = 0; k < 16; k++) r.var[k] = self;
            size_t c = 0; for (int k = 0; k < 16 && c <= v[3].size(); k++) { size_t e2 = v[3].find(',', c); const std::string t = v[3].substr(c, e2 == std::string::npos ? e2 : e2 - c); c = e2 == std::string::npos ? v[3].size() + 1 : e2 + 1;
                if (t != "-" && ix.count(t)) r.var[k] = PROP0 + gPropBase[p] + ix[t]; }
            gConn[self] = r; n++; }
        fclose(f); if (n) printf("Conexoes de mod: %s (%d)\n", gPropPacks[p].mod.c_str(), n); }
}
static void propsRegisterTab() {
    if (propsLoadAll() <= 0) return;
    int tot = 0; gPropBase.clear();
    for (auto& pk : gPropPacks) { gPropBase.push_back(tot); tot += (int)pk.items.size(); }
    if (PROP0 + tot > 65535) { fprintf(stderr, "props: %d itens excedem a faixa de ids; cortando\n", tot); tot = 65535 - PROP0; }
    gPropTotal = tot; propConnLoad();
    for (size_t i = 0; i < gPropPacks.size(); i++) {
        ModDef m; m.name = gPropPacks[i].mod; if (!m.name.empty()) m.name[0] = (char)toupper((unsigned char)m.name[0]);
        for (size_t j = 0; j < gPropPacks[i].items.size(); j++) { const int id = PROP0 + gPropBase[i] + (int)j; if (id < PROP0 + gPropTotal && gPropPacks[i].items[j].tab != "~") m.ids.push_back(id); }   // tab "~" = item-variante (vizinhos), fora do criativo
        if (m.ids.empty()) continue;
        TABS.push_back({strdup(m.name.c_str()), m.ids[0], false, false, -1}); MODS.push_back(m);
        printf("Aba criativa: %s (%d itens, ids %d..%d)\n", m.name.c_str(), (int)m.ids.size(), m.ids.front(), m.ids.back());
    }
}
static int propStackOf(int id) { const PropItem* it = propItem(id); return it ? it->stack : 64; }
// armor.txt (conversor, patchMod.13): `id|slot|tex1|tex2|pontos`; tex '-' = usa a armadura nativa de ferro
static void propLoadArmor(PropPack& pk) {
    FILE* f = fopen((pk.dir + "/armor.txt").c_str(), "r"); if (!f) return; char ln[512]; int n = 0;
    while (fgets(ln, sizeof ln, f)) { std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        std::vector<std::string> v; size_t a = 0; for (;;) { size_t b = s.find('|', a); v.push_back(s.substr(a, b == std::string::npos ? b : b - a)); if (b == std::string::npos) break; a = b + 1; }
        if (v.size() < 5) continue; for (auto& it : pk.items) if (it.id == v[0]) { it.aslot = atoi(v[1].c_str()); it.atex[0] = v[2] == "-" ? "" : v[2]; it.atex[1] = v[3] == "-" ? "" : v[3]; it.apts = atoi(v[4].c_str()); it.stack = 1; n++; break; } }
    fclose(f); if (n) printf("Armaduras de mod: %s (%d)\n", pk.mod.c_str(), n);
}
static int propHeldKind(int id) { const PropItem* it = propItem(id); return it ? it->held : 0; }
