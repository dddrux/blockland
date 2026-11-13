// ---------- patchMod.15: regras declarativas dos mods 1.12.2 (receitas.txt, drops.txt) geradas por conversor/modconv/forge.py ----------
// receitas.txt: `shaped <saida> <n> <linha|linha> <c=item;...>` | `shapeless <saida> <n> <a,b>` | `smelt <entrada> <saida> <xp>`
// drops.txt:    `<bloco> -> <item> <min>-<max> [se <cond>]`   (bloco = nome do loot table, sem namespace)
// Nomes: `ns:nome[@dado]` (mod: item de prop `ns_nome`; minecraft: pack/nativo), `ore:<nome>` (ore dict).
struct ModDropRule { std::string item; int lo = 1, hi = 1; bool chance = false; };
static std::map<std::string, std::vector<ModDropRule>> gModDrops;   // nome do bloco -> regras
static std::map<int, int> gModSmelt;                                // entrada -> saida (fornalha)
static int gModRecOk = 0, gModRecBad = 0;
static std::vector<Recipe>& gModRecipes() { static std::vector<Recipe> R; return R; }

static std::vector<std::string> modSplit(const std::string& s, char c) {
    std::vector<std::string> v; size_t a = 0; for (;;) { const size_t b = s.find(c, a); v.push_back(s.substr(a, b == std::string::npos ? b : b - a)); if (b == std::string::npos) break; a = b + 1; } return v;
}
static int modPropByStr(const std::string& id) {   // item de prop de qualquer pacote pelo id do conversor (`ns_nome`)
    for (size_t p = 0; p < gPropPacks.size() && p < gPropBase.size(); p++) { const auto& its = gPropPacks[p].items;
        for (size_t j = 0; j < its.size(); j++) if (its[j].id == id) return PROP0 + gPropBase[p] + (int)j; }
    return 0;
}
static int modOre(const std::string& name) {   // ore dict: tabela vanilla; senao 1o item de mod cujo id tem todas as palavras (ingotCopper -> *_copper_ingot)
    static std::map<std::string, int> cache; auto c = cache.find(name); if (c != cache.end()) return c->second;
    int r = 0;
    static const struct { const char* n; int id; } T[] = {{"ingotIron", IT_IRON}, {"ingotGold", IT_GOLD}, {"gemDiamond", IT_DIAMOND}, {"dustRedstone", IT_REDSTONE}, {"plankWood", PLANKS},
        {"logWood", LOG}, {"stickWood", IT_STICK}, {"cobblestone", COBBLE}, {"stone", STONE}, {"sand", SAND}, {"string", IT_STRING}, {"leather", IT_LEATHER}, {"feather", IT_FEATHER}, {"coal", IT_COAL}};
    for (auto& t : T) if (name == t.n) r = t.id;
    if (!r) { static const struct { const char* n; const char* f; } F[] = {{"gemEmerald", "emerald"}, {"gemQuartz", "quartz"}, {"gemLapis", "dye_powder_blue|lapis_lazuli"}, {"slimeball", "slimeball|slime_ball"}, {"blockGlass", "glass"}, {"paneGlass", "glass_pane"}, {"blockGold", "gold_block"}, {"blockIron", "iron_block"}, {"blockDiamond", "diamond_block"}};
        for (auto& f : F) if (name == f.n) r = fidv(f.f); }
    if (!r) {   // camelCase -> palavras
        std::vector<std::string> w; std::string cur; for (char ch : name) { if (isupper((unsigned char)ch) && !cur.empty()) { w.push_back(cur); cur.clear(); } cur += (char)tolower((unsigned char)ch); } if (!cur.empty()) w.push_back(cur);
        size_t best = (size_t)-1;
        for (size_t p = 0; p < gPropPacks.size() && p < gPropBase.size(); p++) { const auto& its = gPropPacks[p].items;
            for (size_t j = 0; j < its.size(); j++) { bool all = !w.empty(); for (auto& x : w) if (its[j].id.find(x) == std::string::npos) { all = false; break; }
                if (all && its[j].id.size() < best) { best = its[j].id.size(); r = PROP0 + gPropBase[p] + (int)j; } } }
        if (!r && w.size() == 2) r = fidv((w[1] + "_" + w[0] + "|" + w[0] + "_" + w[1]).c_str());
    }
    cache[name] = r; return r;
}
// patchOre.1: ore dict com varios membros (plankWood = todas as madeiras, logWood = todos os troncos...): a receita vira UMA por membro (ate 6)
static std::vector<int> modOreAll(const std::string& name) {
    static const char* WD[6] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"}; std::vector<int> r;
    if (name == "plankWood" || name == "logWood") { const bool pl = name == "plankWood"; r.push_back(pl ? PLANKS : LOG);
        for (int i = 1; i < 6; i++) { const std::string w = WD[i]; const int id = fidv((pl ? "planks_" + w + "|" + w + "_planks" : "log_" + w + "|" + w + "_log").c_str()); if (id > 0) r.push_back(id); } return r; }
    const int one = modOre(name); if (one > 0) r.push_back(one); return r;
}
static int modResolve(const std::string& s0);
static int modResolveV(const std::string& s, int v) { if (s.rfind("ore:", 0) == 0) { const auto a = modOreAll(s.substr(4)); return a.empty() ? 0 : a[v % a.size()]; } return modResolve(s); }
static int modOreVariants(const std::string& body) {
    int V = 1; size_t p = 0; while ((p = body.find("ore:", p)) != std::string::npos) { size_t e = body.find_first_of(",;| ", p); const std::string n = body.substr(p + 4, e == std::string::npos ? e : e - p - 4); V = std::max(V, (int)modOreAll(n).size()); p += 4; }
    return std::min(V, 6);
}
static int modResolve(const std::string& s0) {   // 0 = nao resolvido
    static std::map<std::string, int> cache; auto c = cache.find(s0); if (c != cache.end()) return c->second;
    std::string s = s0; int meta = 0; { const size_t at = s.find('@'); if (at != std::string::npos) { meta = atoi(s.c_str() + at + 1); s.resize(at); } }
    int r = 0;
    if (s.rfind("ore:", 0) == 0) r = modOre(s.substr(4));
    else {
        const size_t co = s.find(':'); const std::string ns = co == std::string::npos ? "minecraft" : s.substr(0, co), p = co == std::string::npos ? s : s.substr(co + 1);
        if (ns != "minecraft") r = modPropByStr(ns + "_" + p);
        else {
            static const char* WD[6] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
            static const struct { const char* n; int id; } N[] = {{"stick", IT_STICK}, {"iron_ingot", IT_IRON}, {"gold_ingot", IT_GOLD}, {"diamond", IT_DIAMOND}, {"coal", IT_COAL}, {"redstone", IT_REDSTONE},
                {"cobblestone", COBBLE}, {"stone", STONE}, {"string", IT_STRING}, {"leather", IT_LEATHER}, {"feather", IT_FEATHER}, {"flint", IT_FLINT}, {"sand", SAND}, {"wool", WOOL}};
            if (p == "planks" || p == "log") {
                if (meta <= 0 || meta > 5) r = p == "planks" ? PLANKS : LOG;
                else { const std::string w = WD[meta]; r = fidv((p == "planks" ? "planks_" + w + "|" + w + "_planks" : "log_" + w + "|" + w + "_log").c_str()); }
            } else if (p == "dye" && meta == 4) r = fidv("dye_powder_blue|lapis_lazuli");
            else if (meta == 0 || p == "coal") { for (auto& n : N) if (p == n.n) { r = n.id; break; } if (!r) r = fidv(p.c_str()); }
        }
    }
    cache[s0] = r; return r;
}

static std::map<std::string, std::vector<std::string>> gModSnd;   // patchSnd.mod1: evento (sons.txt) -> "ns:caminho"
static void modPropSound(int id, double x, double y, double z) {   // usa o prop: toca o evento cujo nome casa com o id (cfm_flush -> evento "flush")
    const PropItem* it = propItem(id); if (!it || gModSnd.empty()) return; const std::string& pid = it->id;
    for (const auto& kv : gModSnd) { const std::string& e = kv.first; if (kv.second.empty()) continue;
        const bool hit = pid == e || (pid.size() > e.size() && pid.compare(pid.size() - e.size(), e.size(), e) == 0 && pid[pid.size() - e.size() - 1] == '_');
        if (hit) { sndPlayAt(kv.second[rand() % kv.second.size()].c_str(), x, y, z); return; } }
}
static void modPropActArgs(int id, double x, double y, double z) {   // patchForge.4: o que o Java do bloco faz ao clicar, com o argumento lido do bytecode
    auto it = gPropActArg.find(id); if (it == gPropActArg.end()) return;
    std::vector<const std::string*> msg;   // varias mensagens = o Java escolhe por estado (ex.: modo Redstone / Clique direito): alterna a cada clique
    for (const auto& a : it->second) {
        if (a.first == "mensagem") msg.push_back(&a.second);
        else if (a.first == "som" && sndFileOk(a.second)) sndPlayAt(a.second.c_str(), x, y, z);
    }
    if (!msg.empty()) { static std::map<int, unsigned> turn; chatAdd(*msg[turn[id]++ % msg.size()], 0xFFFFFF); }
}
static void modRulesLoad() {   // le receitas.txt / drops.txt de cada pacote de props (mod) carregado
    static bool done = false; if (done || gPropBase.size() != gPropPacks.size()) return; done = true;   // so depois de propsRegisterTab (ids dos pacotes)
    for (auto& pk : gPropPacks) {
        int ok = 0, bad = 0;
        gSndModDirs.push_back(pk.dir);   // patchSnd.mod1
        if (FILE* f = fopen((pk.dir + "/mecanicas.txt").c_str(), "r")) { char ln[1024];   // patchMec.1: item|dureza|resistencia|nivel|metodos|classe
            while (fgets(ln, sizeof ln, f)) { std::string t = ln; while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back(); const auto v = modSplit(t, '|'); if (v.size() < 5) continue;
                const int id = modPropByStr(v[0]); if (id <= 0) continue; if (!v[1].empty()) gPropHard[id] = (float)atof(v[1].c_str()); if (!v[2].empty()) gPropRes[id] = (float)atof(v[2].c_str()); if (!v[3].empty()) gPropLvl[id] = atoi(v[3].c_str()); if (!v[4].empty()) gPropMeth[id] = v[4]; if (v.size() > 6 && !v[6].empty()) gPropAct[id] = v[6]; if (v.size() > 7 && !v[7].empty()) gPropTool[id] = v[7] == "axe" ? 2 : v[7] == "shovel" ? 3 : 1; }   // patchMec.5b
            fclose(f); }
        if (FILE* f = fopen((pk.dir + "/acoes.txt").c_str(), "r")) { char ln[1024];   // patchForge.4: item|acao|argumento
            while (fgets(ln, sizeof ln, f)) { std::string t = ln; while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back(); const auto v = modSplit(t, '|'); if (v.size() < 3 || v[2].empty()) continue;
                const int id = modPropByStr(v[0]); if (id > 0) gPropActArg[id].push_back({v[1], v[2]}); }
            fclose(f); }
        if (FILE* f = fopen((pk.dir + "/sons.txt").c_str(), "r")) { char ln[1024];
            while (fgets(ln, sizeof ln, f)) { std::string t = ln; while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back(); const size_t k = t.find(' '); if (k == std::string::npos) continue; gModSnd[t.substr(0, k)].push_back(t.substr(k + 1)); }
            fclose(f); }
        if (FILE* f = fopen((pk.dir + "/receitas.txt").c_str(), "r")) {
            char ln[2048];
            while (fgets(ln, sizeof ln, f)) {
                std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back(); if (s.empty()) continue;
                const size_t k1 = s.find(' '); if (k1 == std::string::npos) continue; const std::string kind = s.substr(0, k1), rest = s.substr(k1 + 1);
                if (kind == "smelt") {
                    const auto v = modSplit(rest, ' '); if (v.size() < 2) continue;
                    const int a = modResolve(v[0]), b = modResolve(v[1]); if (a > 0 && b > 0) { gModSmelt[a] = b; ok++; } else bad++; continue;
                }
                const size_t k2 = rest.find(' '), k3 = k2 == std::string::npos ? k2 : rest.find(' ', k2 + 1); if (k3 == std::string::npos) continue;
                const int out = modResolve(rest.substr(0, k2)), n = std::max(1, atoi(rest.substr(k2 + 1, k3 - k2 - 1).c_str())); const std::string body = rest.substr(k3 + 1);
                const int V = modOreVariants(body);
                for (int vv = 0; vv < V; vv++) {
                Recipe r; r.out = out; r.n = n; bool good = out > 0;
                if (kind == "shapeless") {
                    for (const std::string& t : modSplit(body, ',')) { const int id = modResolveV(t, vv); if (id <= 0) { good = false; break; } r.list.push_back(id); }
                    if (r.list.empty() || r.list.size() > 9) good = false; std::sort(r.list.begin(), r.list.end()); r.less = true;
                } else if (kind == "shaped") {
                    const size_t sp = body.rfind(' '); if (sp == std::string::npos) continue;   // linhas podem ter espacos: a chave e o que vem depois do ultimo espaco
                    r.rows = modSplit(body.substr(0, sp), '|'); size_t w = 0; for (auto& row : r.rows) w = std::max(w, row.size()); for (auto& row : r.rows) row.resize(w, ' ');
                    if (r.rows.empty() || r.rows.size() > 3 || w == 0 || w > 3) good = false;
                    for (const std::string& kv : modSplit(body.substr(sp + 1), ';')) { if (kv.size() < 3 || kv[1] != '=') continue; const int id = modResolveV(kv.substr(2), vv); if (id <= 0) good = false; r.k[kv[0]] = id; }
                    for (auto& row : r.rows) for (char ch : row) if (ch != ' ' && !r.k.count(ch)) good = false;
                } else continue;
                if (good) { gModRecipes().push_back(r); ok++; } else bad++;
                }
            }
            fclose(f);
        }
        if (FILE* f = fopen((pk.dir + "/drops.txt").c_str(), "r")) {
            char ln[512];
            while (fgets(ln, sizeof ln, f)) {
                std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
                const size_t ar = s.find(" -> "); if (ar == std::string::npos) continue;
                const auto v = modSplit(s.substr(ar + 4), ' '); if (v.size() < 2) continue;
                ModDropRule d; d.item = v[0]; const auto lh = modSplit(v[1], '-'); d.lo = atoi(lh[0].c_str()); d.hi = lh.size() > 1 ? atoi(lh[1].c_str()) : d.lo; if (d.hi < d.lo) d.hi = d.lo;
                d.chance = s.find("random_chance") != std::string::npos;
                gModDrops[s.substr(0, ar)].push_back(d); ok++;
            }
            fclose(f);
        }
        if (ok || bad) printf("Regras de mod: %s (%d ok, %d sem item)\n", pk.mod.c_str(), ok, bad);
        gModRecOk += ok; gModRecBad += bad;
    }
}
static std::mutex gModOreMx;
static void modOresEnsure() {   // patchOre.2: minerios.txt (item|quantidade por chunk|tamanho da veia|y min|y max) de cada mod; so depois de propsRegisterTab
    std::lock_guard<std::mutex> lk(gModOreMx); static bool done = false; if (done || gPropBase.size() != gPropPacks.size()) return; done = true;
    for (auto& pk : gPropPacks) { FILE* f = fopen((pk.dir + "/minerios.txt").c_str(), "r"); if (!f) continue; int n = 0; char ln[512];
        while (fgets(ln, sizeof ln, f)) { std::string t = ln; while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back(); const auto v = modSplit(t, '|'); if (v.size() < 5) continue;
            const int id = modPropByStr(v[0]); if (id <= 0) continue; ModOreGen g{id, atoi(v[1].c_str()), atoi(v[2].c_str()), atoi(v[3].c_str()), atoi(v[4].c_str())};
            if (g.cnt > 0 && g.size > 0 && g.y1 > g.y0) { gModOres.push_back(g); n++; } }
        fclose(f); if (n) printf("Minerios de mod: %s (%d)\n", pk.mod.c_str(), n); }
}
static void modRecipesAdd(std::vector<Recipe>& R) { modRulesLoad(); for (const Recipe& r : gModRecipes()) R.push_back(r); }
static bool modSmeltOf(int id, int& out) { modRulesLoad(); auto it = gModSmelt.find(id); if (it == gModSmelt.end()) return false; out = it->second; return true; }

// quebrar um prop de mod: se o loot table do bloco existe, solta o que ele manda (devolve false = soltar o proprio item)
static bool modPropDrop(int id, double x, double y, double z) {
    modRulesLoad(); const PropItem* it = propItem(id); if (!it || gModDrops.empty()) return false;
    // patchForge.3: vence a CHAVE MAIS LONGA que termina o id (log nao rouba o drop de oak_log; bath_top nao cai em top)
    const std::vector<ModDropRule>* best = nullptr; size_t bl = 0;
    for (auto& kv : gModDrops) {
        const std::string& b = kv.first; if (it->id.size() <= b.size() + 1 || it->id.compare(it->id.size() - b.size(), b.size(), b) != 0 || it->id[it->id.size() - b.size() - 1] != '_') continue;
        if (b.size() > bl) { bl = b.size(); best = &kv.second; }
    }
    if (!best) return false;
    bool any = false;
    for (const ModDropRule& d : *best) { const int di = modResolve(d.item); if (di <= 0) continue; any = true; if (d.chance && rand() % 2) continue;
        const int n = d.lo + (d.hi > d.lo ? rand() % (d.hi - d.lo + 1) : 0); if (n > 0) spawnDrop(x, y + .5, z, 0, .15, 0, di, n, 10); }
    return any;
}
