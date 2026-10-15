// ---------- receitas (Minecraft Java 1.8.9) para os itens que existem no Blockland ----------
// Grade 2x2 (inventário) e 3x3 (mesa de trabalho). Com ordem fixa ("shaped", aceita espelhado) ou sem ordem ("shapeless").
// Cada ingrediente usado gasta 1 unidade, igual ao Java.
struct Recipe { std::vector<std::string> rows; std::unordered_map<char, int> k; std::vector<int> list; int out = 0, n = 1; bool less = false; };
static std::vector<Recipe>& recipes() {
    static std::vector<Recipe> R; static bool init = false;
    if (init) return R;
    init = true;
    auto sh = [&](std::vector<std::string> rows, std::initializer_list<std::pair<char, int>> k, int out, int n = 1) {
        Recipe r; r.rows = rows; for (auto& p : k) r.k[p.first] = p.second; r.out = out; r.n = n; R.push_back(r); };
    auto sl = [&](std::initializer_list<int> in, int out, int n = 1) { Recipe r; r.list = in; std::sort(r.list.begin(), r.list.end()); r.out = out; r.n = n; r.less = true; R.push_back(r); };
    // madeira e básicos
    sl({LOG}, PLANKS, 4);
    sh({"P", "P"}, {{'P', PLANKS}}, IT_STICK, 4);
    sh({"PP", "PP"}, {{'P', PLANKS}}, CTABLE);
    sh({"PPP", "BBB", "PPP"}, {{'P', PLANKS}, {'B', IT_BOOK}}, BOOKSHELF);
    sl({IT_PAPER, IT_PAPER, IT_PAPER, IT_LEATHER}, IT_BOOK);
    // ferramentas: madeira, pedra, ferro, ouro, diamante
    const int mat[5] = {PLANKS, COBBLE, IT_IRON, IT_GOLD, IT_DIAMOND};
    for (int m = 0; m < 5; m++) {
        const int M = mat[m];
        sh({"MMM", " S ", " S "}, {{'M', M}, {'S', IT_STICK}}, m == 0 ? IT_WPICK : m == 1 ? IT_SPICK : IT_IPICK + (m - 2));
        sh({"M", "M", "S"}, {{'M', M}, {'S', IT_STICK}}, m == 0 ? IT_WSWORD : IT_SSWORD + (m - 1));
        sh({"MM", "MS", " S"}, {{'M', M}, {'S', IT_STICK}}, IT_WAXE + m);
        sh({"M", "S", "S"}, {{'M', M}, {'S', IT_STICK}}, IT_WSHOVEL + m);
        sh({"MM", " S", " S"}, {{'M', M}, {'S', IT_STICK}}, IT_WHOE + m);
    }
    if (gBktEmpty) sh({"I I", " I "}, {{'I', IT_IRON}}, gBktEmpty);   // balde
    if (gChiselIron) { sh({"I", "S"}, {{'I', IT_IRON}, {'S', IT_STICK}}, gChiselIron); sh({"D", "S"}, {{'D', IT_DIAMOND}, {'S', IT_STICK}}, gChiselDia); }   // cinzeis
    if (gFlintSteel) sl({IT_IRON, IT_FLINT}, gFlintSteel);   // isqueiro (lingote de ferro + pederneira)
    sh({" ST", "S T", " ST"}, {{'S', IT_STICK}, {'T', IT_STRING}}, IT_BOW);
    sh({"F", "S", "H"}, {{'F', IT_FLINT}, {'S', IT_STICK}, {'H', IT_FEATHER}}, IT_ARROW, 4);
    // armaduras: couro, ferro, ouro, diamante
    const int am[4] = {IT_LEATHER, IT_IRON, IT_GOLD, IT_DIAMOND};
    for (int m = 0; m < 4; m++) {
        sh({"MMM", "M M"}, {{'M', am[m]}}, IT_A0 + m * 4);
        sh({"M M", "MMM", "MMM"}, {{'M', am[m]}}, IT_A0 + m * 4 + 1);
        sh({"MMM", "M M", "M M"}, {{'M', am[m]}}, IT_A0 + m * 4 + 2);
        sh({"M M", "M M"}, {{'M', am[m]}}, IT_A0 + m * 4 + 3);
    }
    // blocos de minério <-> 9 itens
    const int bi[4][2] = {{GOLDBLK, IT_GOLD}, {IRONBLK, IT_IRON}, {DIAMBLK, IT_DIAMOND}, {COALBLK, IT_COAL}};
    for (auto& b : bi) { sh({"MMM", "MMM", "MMM"}, {{'M', b[1]}}, b[0]); sl({b[0]}, b[1], 9); }
    sh({"GGG", "GAG", "GGG"}, {{'G', IT_GOLD}, {'A', IT_APPLE}}, IT_GAPPLE);
    // blocos 2x2
    sh({"TT", "TT"}, {{'T', IT_STRING}}, WOOL);
    sh({"SS", "SS"}, {{'S', SAND}}, SANDSTONE);
    sh({"SS", "SS"}, {{'S', STONE}}, STONEBRICK);
    return R;
}
static std::vector<Recipe>& packRecipes();   // definida em chest.inc (precisa de gRsDrop, de redstone.inc)
// g = grade gw x gh (ids; 0 = vazio). Devolve o item de saída (0 = nenhuma receita) e a quantidade em n.
static int craftResult(const int* g, int gw, int gh, int& n) {
    int x0 = gw, x1 = -1, y0 = gh, y1 = -1, cnt = 0; std::vector<int> sorted;
    for (int y = 0; y < gh; y++) for (int x = 0; x < gw; x++) if (g[y * gw + x]) { x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y); cnt++; sorted.push_back(g[y * gw + x]); }
    if (!cnt) return 0;
    std::sort(sorted.begin(), sorted.end());
    const int bw = x1 - x0 + 1, bh = y1 - y0 + 1;
    for (int pass = 0; pass < 2; pass++) for (const Recipe& r : (pass ? packRecipes() : recipes())) {
        if (r.less) { if (r.list == sorted) { n = r.n; return r.out; } continue; }
        if ((int)r.rows.size() != bh || (int)r.rows[0].size() != bw) continue;
        for (int mir = 0; mir < 2; mir++) {
            bool ok = true;
            for (int y = 0; y < bh && ok; y++) for (int x = 0; x < bw && ok; x++) {
                const char c = r.rows[y][mir ? bw - 1 - x : x];
                const int want = c == ' ' ? 0 : r.k.at(c);
                if (g[(y0 + y) * gw + x0 + x] != want) ok = false;
            }
            if (ok) { n = r.n; return r.out; }
        }
    }
    return 0;
}
