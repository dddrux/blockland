// ---------- aba REDSTONE: modelo + colocacao de todos os itens ----------
// 17 ids XB (XB_DUST..XB_END-1). O ESTADO vai no metadado (1 byte), e a malha e montada sob demanda por (id, meta) e guardada em cache (como os trilhos).
// Modelos vem do PROPRIO pack quando existem (porta, alcapao, portao, funil, gancho); o resto (poeira, pistao, placa, botao, sensor, repetidor,
// comparador, bau) e montado aqui com caixas + texturas do pack. SEM logica de redstone ainda: so modelo, colocacao, suporte, forma, drop e uso (abrir/fechar).
// Metadados: poeira conexoes N,E,S,W=bits0-3 | pistao/dispenser/dropper/observador dir6 (0 baixo,1 cima,2 N,3 L,4 S,5 O) | placa tipo(0-3)|pressionada<<2
// botao face(0 chao,1 teto,2..5 parede N,L,S,O)|tipo<<3|press<<4 | gancho dir(0-3)|lig<<2 | sensor invertido bit0 | funil 0 baixo,1..4 N,L,S,O
// repetidor dir|atraso<<2|lig<<4 | comparador dir|subtrai<<2|lig<<3 | alcapao dir|metade-de-cima<<2|aberto<<3|tipo<<4 | portao dir|aberto<<2|tipo<<3
// porta lado-da-lamina|aberta<<2|parte-de-cima<<3|dobradica-direita<<4|tipo<<5 | bau dir|tipo<<2 (0 normal, 1 armadilhado, 2 ender).   dir horizontal: 0 N(-Z) 1 L(+X) 2 S(+Z) 3 O(-X)
static const int RS_N = XB_END - XB_DUST;
// Bau duplo (1.12.2): tipo 19..22 no meta = metade de bau duplo (19 normal-esq, 20 normal-dir, 21 armadilhado-esq, 22 armadilhado-dir; esq/dir vistos de frente); bit 7 = tampa aberta.
// CH_RV[dir] = lado DIREITO visto de frente (a metade esquerda tem a parceira nesse lado). Logica em chest.inc.
static const int CH_RV[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
static int chestPlaceMeta(int x, int y, int z, int var, int dir); static void chestAfterPlace(int x, int y, int z, int meta);
static std::vector<XQuad> gRsQ[RS_N][256]; static uint8_t gRsB[RS_N][256];
static int gRsDrop[RS_N][32];                   // (bloco, variante) -> item que dropa
static std::unordered_map<int, int> gRsVar;    // item -> variante (tipo de placa, madeira da porta...)
static int gRsRB = 0;                          // id do bloco de redstone (a poeira se liga nele)
static const char* RS_COLOR[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
// placa (XB_SIGN): meta = madeira*20 + (parede ? 16 + dir(0..3) : rotacao(0..15, 22,5 graus, 0 = frente para o sul, horario)); 240+madeira = icone/mao
static inline void signDec(int m, int& wood, bool& wall, int& d) {
    if (m >= 240) { wood = std::min(m - 240, 11); wall = false; d = 0; return; }
    wood = std::min(m / 20, 11); const int q = m % 20; wall = q >= 16; d = wall ? q - 16 : q;
}
static const char* RS_WOOD[7] = {"iron", "oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
#include "entmodels.inc"   // bau / bau do Ender / cama com texturas de entidade 64x64 do pack
static inline int rsItemVar(int item) { auto it = gRsVar.find(item); return it == gRsVar.end() ? 0 : it->second; }
static inline int rsVariant(int b, int m) {
    if (b == XB_STAIR) return (m >> 3) & 15; if (b == XB_SLAB) return (m >> 2) & 15;
    if (b == XB_PLATE) return m & 15; if (b == XB_BTN) return (m >> 3) & 1; if (b == XB_TRAP) return (m >> 4) & 7;
    if (b == XB_GATE) return (m >> 3) & 7; if (b == XB_DOOR) return (m >> 5) & 7; if (b == XB_CHEST) return (m >> 2) & 31;
    if (b == XB_CARPET) return m & 15; if (b == XB_FENCE || b == XB_PANE || b == XB_WALL) return (m >> 4) & 7; if (b == XB_CPANE) return (m >> 4) & 15; if (b == XB_BED) return (m >> 3) & 15; if (b == XB_SIGN) { int w, d; bool wl; signDec(m, w, wl, d); return w; } return 0;
}

// ---- malha ----
static std::string rsBx(float x1, float y1, float z1, float x2, float y2, float z2, const char* E, const char* W, const char* U, const char* D, const char* S, const char* N) {
    static const char* FN[6] = {"east", "west", "up", "down", "south", "north"}; const char* T[6] = {E, W, U, D, S, N};
    char b[160]; snprintf(b, sizeof b, "{\"from\":[%g,%g,%g],\"to\":[%g,%g,%g],\"faces\":{", x1, y1, z1, x2, y2, z2);
    std::string s = b; bool first = true;
    for (int i = 0; i < 6; i++) if (T[i]) { if (!first) s += ','; first = false; s += std::string("\"") + FN[i] + "\":{\"texture\":\"" + T[i] + "\"}"; }
    return s + "}}";
}
static std::string rsAll(float x1, float y1, float z1, float x2, float y2, float z2, const char* t) { return rsBx(x1, y1, z1, x2, y2, z2, t, t, t, t, t, t); }
static std::string rsTorch(float x, float y0, float z, float h, bool on) {   // pilar de tocha de redstone 2x h x 2 (uv do pack: so o miolo da haste)
    const char* t = on ? "block/redstone_torch" : "block/redstone_torch_off"; char b[560];
    snprintf(b, sizeof b, "{\"from\":[%g,%g,%g],\"to\":[%g,%g,%g],\"faces\":{\"east\":{\"uv\":[7,6,9,%g],\"texture\":\"%s\"},\"west\":{\"uv\":[7,6,9,%g],\"texture\":\"%s\"},\"south\":{\"uv\":[7,6,9,%g],\"texture\":\"%s\"},\"north\":{\"uv\":[7,6,9,%g],\"texture\":\"%s\"},\"up\":{\"uv\":[7,6,9,8],\"texture\":\"%s\"}}}",
        x, y0, z, x + 2, y0 + h, z + 2, 6 + h, t, 6 + h, t, 6 + h, t, 6 + h, t, t);
    return b;
}
static std::string rsNd(float x1, float y1, float z1, float x2, float y2, float z2, const char* t) { return rsBx(x1, y1, z1, x2, y2, z2, t, t, t, 0, t, t); }   // sem a face de baixo (evita piscar no chao)
static void rsBake(const std::vector<std::string>& els, int bsx, int bsy, std::vector<XQuad>& out) {
    std::string js = "{\"elements\":["; for (size_t i = 0; i < els.size(); i++) { if (i) js += ','; js += els[i]; } js += "]}";
    XModel m; if (xbParse(js, m, 0)) xbBake(m, bsx, bsy, false, out);
}
static void rsBakeModel(const std::string& name, int bsx, int bsy, std::vector<XQuad>& out) { XModel m; if (xbLoadModel(name, m)) xbBake(m, bsx, bsy, false, out); }
static void rsDirRot(int d, int& bsx, int& bsy) { bsx = d == 0 ? 90 : d == 1 ? -90 : 0; bsy = d >= 2 ? 90 * (d - 2) : 0; }   // frente canonica = norte
// materiais de escada/laje (var 0..10): oak cobble spruce birch jungle acacia dark_oak sandstone stone_bricks bricks stone
static void rsMat(int v, std::string& sd, std::string& tp, std::string& bt) {
    static const char* N[11] = {"oak_planks", "cobblestone", "spruce_planks", "birch_planks", "jungle_planks", "acacia_planks", "dark_oak_planks", "sandstone", "stonebrick", "brick", "stone"};
    sd = tp = bt = std::string("block/") + N[std::min(v, 10)];
    if (v == 7) { tp = "block/sandstone_top"; bt = "block/sandstone_bottom"; } else if (v == 8) sd = tp = bt = "block/stone_bricks"; else if (v == 9) sd = tp = bt = "block/bricks";
}
static void rsBuild(int b, int m, std::vector<XQuad>& o) {
    const int k = b - XB_DUST; int bx = 0, by = 0; const std::string W = "block/";
    switch (k) {
    case 26: { std::string sd, tp, bt; rsMat((m >> 3) & 15, sd, tp, bt); const bool t = (m >> 2) & 1; const char *a = sd.c_str(), *u = tp.c_str(), *d = bt.c_str();
        rsBake({rsBx(0, t ? 8 : 0, 0, 16, t ? 16 : 8, 16, a, a, u, d, a, a), rsBx(0, t ? 0 : 8, 0, 16, t ? 8 : 16, 8, a, a, t ? 0 : u, t ? d : 0, a, a)}, 0, 90 * (m & 3), o); break; }
    case 27: { std::string sd, tp, bt; rsMat((m >> 2) & 15, sd, tp, bt); const int h = m & 3; const char *a = sd.c_str(), *u = tp.c_str(), *d = bt.c_str();
        rsBake({rsBx(0, h == 1 ? 8 : 0, 0, 16, h == 0 ? 8 : 16, 16, a, a, u, d, a, a)}, 0, 0, o); break; }
    case 28: { static const int MX[4] = {7, 3, 3, 3}; static const char* NM[4] = {"wheat", "carrots", "potatoes", "beetroots"}; const int ty = (m >> 3) & 3, ag = std::min(m & 7, MX[ty] == 7 ? 7 : 7);
        const int st = ty == 0 ? ag : ty == 3 ? (ag >> 1) : (ag < 2 ? 0 : ag < 4 ? 1 : ag < 7 ? 2 : 3); char t[48]; snprintf(t, sizeof t, "block/%s_stage%d", NM[ty], std::min(st, ty == 0 ? 7 : 3)); const char* q = t;
        rsBake({rsBx(4, 0, 0, 4, 16, 16, q, q, 0, 0, 0, 0), rsBx(12, 0, 0, 12, 16, 16, q, q, 0, 0, 0, 0), rsBx(0, 0, 4, 16, 16, 4, 0, 0, 0, 0, q, q), rsBx(0, 0, 12, 16, 16, 12, 0, 0, 0, 0, q, q)}, 0, 0, o); break; }
    case 29: rsBake({rsBx(0, 0, 0, 16, 15, 16, "block/dirt", "block/dirt", "block/farmland", "block/dirt", "block/dirt", "block/dirt")}, 0, 0, o); break;
    case 0: {   // poeira: ponto ou linhas (NS e/ou LO); cor pela potencia (bits 4-7)
        const int cn = m & 15; std::vector<std::string> e;
        e.push_back(rsBx(0, .25f, 0, 16, .25f, 16, 0, 0, cn ? "block/redstone_dust_line0" : "block/redstone_dust_dot", 0, 0, 0));
        if (cn & 5) rsBake(e, 0, 0, o);
        if (cn & 10) rsBake(e, 0, 90, o);
        if (!cn) rsBake(e, 0, 0, o);
        const float p = (m >> 4) / 15.f; for (XQuad& q : o) { q.tc[0] = .3f + .7f * p; q.tc[1] = q.tc[2] = p * p * .5f; }
        break; }
    case 1: case 2: case 3: case 4: case 5: {   // pistao, pistao grudento, dispenser, dropper, observador (dir6; frente voltada para o jogador)
        const int d = m & 7; rsDirRot(d, bx, by); const bool vert = d < 2; const char *fr, *sd, *bk, *tp;
        if (k <= 2) { fr = k == 2 ? "block/piston_top_sticky" : "block/piston_top"; sd = tp = "block/piston_side"; bk = "block/piston_bottom"; }
        else if (k <= 4) { fr = k == 3 ? (vert ? "block/dispenser_front_vertical" : "block/dispenser_front") : (vert ? "block/dropper_front_vertical" : "block/dropper_front"); sd = bk = vert ? "block/furnace_top" : "block/furnace_side"; tp = "block/furnace_top"; }
        else { fr = "block/observer_front"; bk = "block/observer_back"; sd = "block/observer_side"; tp = "block/observer_top"; }
        rsBake({rsBx(0, 0, 0, 16, 16, 16, sd, sd, tp, tp, bk, fr)}, bx, by, o); break; }
    case 6: { static const char* T[16] = {"block/stone", "block/oak_planks", "block/gold_block", "block/iron_block", "block/spruce_planks", "block/birch_planks", "block/jungle_planks", "block/acacia_planks", "block/dark_oak_planks", "block/mangrove_planks", "block/cherry_planks", "block/bamboo_planks", "block/crimson_planks", "block/warped_planks", "block/pale_oak_planks", "block/oak_planks"};
        rsBake({rsAll(1, 0, 1, 15, (m >> 4) & 1 ? .5f : 1.f, 15, T[m & 15])}, 0, 0, o); break; }
    case 7: { const int f = m & 7, pr = (m >> 4) & 1; const char* t = (m >> 3) & 1 ? "block/oak_planks" : "block/stone";
        bx = f == 1 ? 180 : f >= 2 ? 90 : 0; by = f >= 2 ? 90 * (f - 2) : 0; rsBake({rsAll(5, 0, 6, 11, pr ? 1.f : 2.f, 10, t)}, bx, by, o); break; }
    case 8: rsBakeModel(W + ((m >> 2) & 1 ? "tripwire_hook_on" : "tripwire_hook"), 0, 90 * (m & 3), o); break;
    case 9: rsBake({rsBx(0, 0, 0, 16, 6, 16, "block/daylight_detector_side", "block/daylight_detector_side", m & 1 ? "block/daylight_detector_inverted_top" : "block/daylight_detector_top", "block/oak_planks", "block/daylight_detector_side", "block/daylight_detector_side")}, 0, 0, o); break;
    case 10: if (m == 0) rsBakeModel(W + "hopper", 0, 0, o); else rsBakeModel(W + "hopper_side", 0, ((m - 2) & 3) * 90, o); break;
    case 11: case 12: {   // base de 2px + tochas (saida no norte): repetidor = fixa + movel (atraso); comparador = 2 de tras + 1 da frente (alta no modo subtrair)
        const bool on = k == 11 ? (m >> 4) & 1 : (m >> 3) & 1; const char* t = k == 11 ? (on ? "block/repeater_on" : "block/repeater") : (on ? "block/comparator_on" : "block/comparator");
        std::vector<std::string> e = {rsBx(0, 0, 0, 16, 2, 16, "block/stone", "block/stone", t, "block/stone", "block/stone", "block/stone")};
        if (k == 11) { e.push_back(rsTorch(7, 2, 2, 5, on)); e.push_back(rsTorch(7, 2, 6 + 2 * ((m >> 2) & 3), 5, on)); }
        else { const bool sub = (m >> 2) & 1; e.push_back(rsTorch(3, 2, 11, 3, on)); e.push_back(rsTorch(11, 2, 11, 3, on)); e.push_back(rsTorch(7, 2, 2, sub ? 5 : 3, sub)); }
        rsBake(e, 0, 90 * (m & 3), o); break; }
    case 13: { const int ty = (m >> 4) & 7, op = (m >> 3) & 1, tp = (m >> 2) & 1;
        // caixa solida de 3/16 (como o Minecraft); o modelo do pack e uma moldura vazada com buracos e parecia quebrado
        const std::string tx = ty == 0 ? W + "iron_trapdoor_face" : W + RS_WOOD[ty] + "_trapdoor"; const char* t = tx.c_str();
        const float e = .02f, f = 16.f - .02f;   // 0,02 de folga nas faces que encostam no vizinho/chao/teto: com textura transparente elas brigavam (z-fighting) e piscavam
        rsBake({op ? rsBx(e, e, 13, f, f, f, t, t, t, t, t, t) : tp ? rsBx(e, 13, e, f, f, f, t, t, t, t, t, t) : rsBx(e, e, e, f, 3, f, t, t, t, t, t, t)}, 0, op ? 90 * (m & 3) : 0, o); break; }
    case 14: { const int ty = (m >> 3) & 7, op = (m >> 2) & 1;
        rsBakeModel(W + RS_WOOD[1 + std::min(ty, 5)] + "_fence_gate" + (op ? "_open" : ""), 0, ((m + 2) & 3) * 90, o); break; }
    case 15: { const int f = m & 3, op = (m >> 2) & 1, up = (m >> 3) & 1, hi = (m >> 4) & 1, ty = (m >> 5) & 7;
        int kk = (f + 1) & 3; if (op) kk = (kk + (hi ? 3 : 1)) & 3;   // lamina em O/N/L/S = 0..3; aberta gira 90 em torno da dobradica
        rsBakeModel(W + RS_WOOD[ty] + "_door_" + (up ? "top" : "bottom") + (hi ? "_hinge" : ""), 0, 90 * kk, o); break; }
    case 17: { const std::string t = W + RS_COLOR[m & 15] + "_wool"; rsBake({rsNd(0, 0, 0, 16, 1, 16, t.c_str())}, 0, 0, o); break; }   // carpete: 1/16 de altura
    case 18: { const int ty = std::min((m >> 4) & 7, 6); const std::string t = ty == 6 ? W + "nether_bricks" : W + RS_WOOD[1 + ty] + "_planks";   // cerca: poste + 2 travessas por lado conectado
        rsBake({rsNd(6, 0, 6, 10, 16, 10, t.c_str())}, 0, 0, o);
        for (int d = 0; d < 4; d++) if ((m >> d) & 1) rsBake({rsAll(7, 12, .02f, 9, 15, 6, t.c_str()), rsAll(7, 6, .02f, 9, 9, 6, t.c_str())}, 0, 90 * d, o);
        break; }
    case 19: case 22: {   // painel / barras / painel colorido; sem conexao = cruz. Sem face de baixo e 0,01 de folga nas pontas: nada pisca no chao/teto/parede
        std::string sd, tp;
        if (k == 22) { sd = W + RS_COLOR[(m >> 4) & 15] + "_stained_glass"; tp = sd + "_pane_top"; }
        else if (((m >> 4) & 7) == 1) { sd = "block/glass"; tp = "block/glass_pane_top"; } else sd = tp = "block/iron_bars";
        const int cn = (m & 15) ? (m & 15) : 15;
        rsBake({rsBx(7, 0, 7, 9, 15.99f, 9, sd.c_str(), sd.c_str(), tp.c_str(), 0, sd.c_str(), sd.c_str())}, 0, 0, o);
        for (int d = 0; d < 4; d++) if ((cn >> d) & 1) rsBake({rsBx(7, 0, .02f, 9, 15.99f, 7, sd.c_str(), sd.c_str(), tp.c_str(), 0, 0, (m & 15) ? 0 : tp.c_str())}, 0, 90 * d, o);
        break; }
    case 23: { const int f = m & 7; rsBakeModel(W + "end_rod", f == 1 ? 180 : f >= 2 ? 90 : 0, f >= 2 ? 90 * (f - 2) : 0, o); break; }   // haste do End: modelo do pack
    case 24: if (entBed(m, o)) break;   // modelo real (entity/bed/<cor>.png); sem a textura, reserva de la
        { const std::string t = W + RS_COLOR[(m >> 3) & 15] + "_wool"; const char* pl = "block/oak_planks"; std::vector<std::string> e;   // cama: colchao + pes (cabeca para o norte)
        if (m & 128) { e = {rsBx(0, 3, -16, 16, 9, 16, t.c_str(), t.c_str(), t.c_str(), pl, t.c_str(), t.c_str()), rsNd(0, 0, -16, 3, 3, -13, pl), rsNd(13, 0, -16, 16, 3, -13, pl), rsNd(0, 0, 13, 3, 3, 16, pl), rsNd(13, 0, 13, 16, 3, 16, pl)}; rsBake(e, 0, 0, o); break; }   // icone: as duas metades
        const bool hd = (m >> 2) & 1; e.push_back(rsBx(0, 3, 0, 16, 9, 16, t.c_str(), t.c_str(), t.c_str(), pl, t.c_str(), t.c_str()));
        e.push_back(rsNd(hd ? 0 : 0, 0, hd ? 0 : 13, 3, 3, hd ? 3 : 16, pl)); e.push_back(rsNd(13, 0, hd ? 0 : 13, 16, 3, hd ? 3 : 16, pl));
        rsBake(e, 0, 90 * (m & 3), o); break; }
    case 25: { int wd, d; bool wl; signDec(m, wd, wl, d); if (entSign(wd, wl, d, o)) break;   // icone/mao (m >= 240): de pe, frente voltada para a camera
        const std::string t = W + (wd == 0 ? "oak" : RS_WOOD[1 + std::min(wd - 1, 5)]) + "_planks";
        rsBake({rsAll(0, 9.333f, 7.333f, 16, 17.333f, 8.667f, t.c_str()), rsAll(7.333f, 0, 7.333f, 8.667f, 9.333f, 8.667f, t.c_str())}, 0, 90 * (wl ? d & 3 : (((d + 2) >> 2) + 2) & 3), o); break; }
    case 20: { rsBakeModel(W + "ladder", 0, 90 * (m & 3), o); if (o.empty()) rsBake({rsBx(0, 0, 15.2f, 16, 16, 15.2f, 0, 0, 0, 0, "block/ladder", "block/ladder")}, 0, 90 * (m & 3), o); break; }   // modelo do pack (placa na parede, frente = lado do meta)
    case 21: { const std::string t = W + (((m >> 4) & 7) == 1 ? "mossy_cobblestone" : "cobblestone"); const int cn = m & 15;   // muro: poste (some em linha reta) + bracos
        if ((cn != 5 && cn != 10) || (m & 128)) rsBake({rsNd(4, 0, 4, 12, 16, 12, t.c_str())}, 0, 0, o);
        for (int d = 0; d < 4; d++) if ((cn >> d) & 1) rsBake({rsNd(5, 0, .02f, 11, 13, 8, t.c_str())}, 0, 90 * d, o);
        break; }
    case 16: { const int ty = (m >> 2) & 31; const bool op = (m >> 7) & 1;   // bit 7 = tampa aberta
        if (ty >= 19 && ty <= 22) { if (entChest((ty - 19) >> 1, m & 3, o, (ty - 19) & 1, op) || entChest((ty - 19) >> 1, m & 3, o, -1, op)) break; }   // textura normal_left/right do pack; sem elas, bau simples
        else if (ty >= 3 ? entShulker(ty - 3, o) : entChest(ty, m & 3, o, -1, op)) break; }   // modelo real (entity/chest/*.png); sem a textura, caixa de reserva
        rsBake({rsAll(1, 0, 1, 15, 14, 15, "block/oak_planks"), rsAll(7, 8, 0, 9, 11, 1, "block/iron_block")}, 0, 90 * (m & 3), o); break;
    }
}
static const std::vector<XQuad>& rsQuads(int b, int meta) {
    const int k = b - XB_DUST, m = meta & 255;
    { static size_t seen = (size_t)-1; const size_t ep = PK.size() * 64 + (size_t)gEntN;   // malha em cache feita ANTES do pack/texturas de entidade carregarem sairia vazia ou com a textura de reserva: refaz tudo quando isso muda
      if (seen != ep) { seen = ep; memset(gRsB, 0, sizeof gRsB); } }
    if (!gRsB[k][m]) { gRsB[k][m] = 1; gRsQ[k][m].clear(); rsBuild(b, m, gRsQ[k][m]); }
    return gRsQ[k][m];
}
static int rsTile(int b) { const std::vector<XQuad>& q = rsQuads(b, b == XB_DOOR ? 0 : b == XB_TRAP ? 16 : 0); return q.empty() ? 0 : q[0].tile; }

// ---- formas (local 0..1) ----
static AABB rsXf(const AABB& b, int ax, int ay) {   // mesma convencao do xbBake: x depois y, horario = angulo negativo
    double lo[3] = {9, 9, 9}, hi[3] = {-9, -9, -9};
    for (int i = 0; i < 8; i++) {
        float p[3] = {(float)((i & 1) ? b.x1 : b.x0) - .5f, (float)((i & 2) ? b.y1 : b.y0) - .5f, (float)((i & 4) ? b.z1 : b.z0) - .5f};
        if (ax) xRot(p, 0, (float)-ax); if (ay) xRot(p, 1, (float)-ay);
        for (int j = 0; j < 3; j++) { lo[j] = std::min(lo[j], (double)p[j] + .5); hi[j] = std::max(hi[j], (double)p[j] + .5); }
    }
    return AABB{lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]};
}
static AABB rsSlab(int side) {   // lamina de 3/16 no lado (0 N, 1 L, 2 S, 3 O)
    static const AABB S[4] = {{0, 0, 0, 1, 1, .1875}, {.8125, 0, 0, 1, 1, 1}, {0, 0, .8125, 1, 1, 1}, {0, 0, 0, .1875, 1, 1}}; return S[side & 3]; }
static int rsShape(int, int, int, int id, int m, bool sel, AABB* o) {
    switch (id - XB_DUST) {
    case 0: o[0] = AABB{0, 0, 0, 1, .0625, 1}; return 1;
    case 6: o[0] = AABB{.0625, 0, .0625, .9375, (m >> 4) & 1 ? .03125 : .0625, .9375}; return 1;
    case 7: { const int f = m & 7; o[0] = rsXf(AABB{.3125, 0, .375, .6875, (m >> 4) & 1 ? .0625 : .125, .625}, f == 1 ? 180 : f >= 2 ? 90 : 0, f >= 2 ? 90 * (f - 2) : 0); return 1; }
    case 8: o[0] = rsXf(AABB{.3125, 0, .625, .6875, .625, 1}, 0, 90 * (m & 3)); return 1;
    case 9: o[0] = AABB{0, 0, 0, 1, .375, 1}; return 1;
    case 10: { o[0] = AABB{0, .625, 0, 1, 1, 1}; o[1] = AABB{.25, .25, .25, .75, .625, .75};
        if (m == 0) o[2] = AABB{.375, 0, .375, .625, .25, .625}; else o[2] = rsXf(AABB{.75, .25, .375, 1, .5, .625}, 0, ((m - 2) & 3) * 90); return 3; }
    case 11: case 12: o[0] = AABB{0, 0, 0, 1, .125, 1}; return 1;
    case 13: case 15: { const std::vector<XQuad>& q = rsQuads(id, m); if (q.empty()) return 0; float mn[3] = {9, 9, 9}, mx[3] = {-9, -9, -9};
        for (const XQuad& a : q) for (int v = 0; v < 4; v++) for (int i = 0; i < 3; i++) { mn[i] = std::min(mn[i], a.p[v][i]); mx[i] = std::max(mx[i], a.p[v][i]); }
        o[0] = AABB{mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]}; return 1; }
    case 99: if ((m >> 3) & 1) { o[0] = rsXf(AABB{0, 0, .8125, 1, 1, 1}, 0, 90 * (m & 3)); return 1; } o[0] = (m >> 2) & 1 ? AABB{0, .8125, 0, 1, 1, 1} : AABB{0, 0, 0, 1, .1875, 1}; return 1;
    case 14: { const bool z = !(m & 1); const bool op = (m >> 2) & 1; if (op && !sel) return 0; const double h = sel ? 1 : 1.5;
        if (!op) { o[0] = z ? AABB{0, 0, .375, 1, h, .625} : AABB{.375, 0, 0, .625, h, 1}; return 1; }
        o[0] = z ? AABB{0, 0, .375, .125, 1, .625} : AABB{.375, 0, 0, .625, 1, .125}; o[1] = z ? AABB{.875, 0, .375, 1, 1, .625} : AABB{.375, 0, .875, .625, 1, 1}; return 2; }
    case 16: { double x0 = .0625, x1 = .9375, z0 = .0625, z1 = .9375; const int ty = (m >> 2) & 31;
        if (ty >= 19 && ty <= 22) { const int sg = ((ty - 19) & 1) ? -1 : 1, ex = CH_RV[m & 3][0] * sg, ez = CH_RV[m & 3][1] * sg; if (ex > 0) x1 = 1; if (ex < 0) x0 = 0; if (ez > 0) z1 = 1; if (ez < 0) z0 = 0; }   // a metade chega ate a emenda
        o[0] = AABB{x0, 0, z0, x1, .875, z1}; return 1; }
    case 17: o[0] = AABB{0, 0, 0, 1, .0625, 1}; return 1;
    case 20: o[0] = rsSlab((m + 2) & 3); return 1;
    case 26: { const bool t = (m >> 2) & 1; o[0] = rsXf(AABB{0, t ? .5 : 0, 0, 1, t ? 1 : .5, 1}, 0, 90 * (m & 3)); o[1] = rsXf(AABB{0, t ? 0 : .5, 0, 1, t ? .5 : 1, .5}, 0, 90 * (m & 3)); return 2; }
    case 27: { const int h = m & 3; o[0] = AABB{0, h == 1 ? .5 : 0, 0, 1, h == 0 ? .5 : 1, 1}; return 1; }
    case 28: o[0] = AABB{0, 0, 0, 1, .25, 1}; return 1;
    case 29: o[0] = AABB{0, 0, 0, 1, .9375, 1}; return 1;
    case 23: { const int f = m & 7; o[0] = rsXf(AABB{.375, 0, .375, .625, 1, .625}, f == 1 ? 180 : f >= 2 ? 90 : 0, f >= 2 ? 90 * (f - 2) : 0); return 1; }
    case 24: o[0] = AABB{0, 0, 0, 1, .5625, 1}; return 1;
    case 25: { int wd, d; bool wl; signDec(m, wd, wl, d); if (wl) { o[0] = rsXf(AABB{0, .28125, 0, 1, .78125, .125}, 0, 90 * ((d + 2) & 3)); return 1; } } o[0] = AABB{.25, 0, .25, .75, 1, .75}; return 1;
    case 18: case 19: case 21: case 22: { const bool fe = id != XB_PANE && id != XB_CPANE; const double h = fe && !sel ? 1.5 : 1, a = id == XB_FENCE ? .375 : fe ? .3125 : .4375, b = 1 - a; int cn = m & 15; if (!fe && !cn) cn = 15; int n = 0;
        { const bool up = id == XB_WALL && (((cn != 5 && cn != 10)) || (m & 128)); const double pa = up ? .25 : a; o[n++] = AABB{pa, 0, pa, 1 - pa, h, 1 - pa}; }
        if (cn & 1) o[n++] = AABB{a, 0, 0, b, h, a}; if (cn & 2) o[n++] = AABB{b, 0, a, 1, h, b}; if (cn & 4) o[n++] = AABB{a, 0, b, b, h, 1}; if (cn & 8) o[n++] = AABB{0, 0, a, a, h, b};
        return n; }
    }
    return 0;
}
static const struct RsShapeInit {
    RsShapeInit() {
        const int s = shapeAddFn(rsShape);
        for (int id = XB_DUST; id < XB_END; id++) {
            const int k = id - XB_DUST;
            if (k >= 1 && k <= 5) { blockShapeSet(id, SHAPE_CUBE, SHAPE_CUBE); continue; }   // pistao, dispenser, dropper, observador: cubo
            const bool coll = k == 22 || k == 23 || k == 24 || k == 17 || k == 18 || k == 19 || k == 20 || k == 21 || k == 9 || k == 10 || k == 11 || k == 12 || k == 13 || k == 14 || k == 15 || k == 16 || k == 26 || k == 27 || k == 29;
            blockShapeSet(id, coll ? s : SHAPE_NONE, s);
        }
    }
} rsShapeInit;

// ---- mundo: suporte, conexoes da poeira, dureza ----
static const int RS_DX[4] = {0, 1, 0, -1}, RS_DZ[4] = {-1, 0, 1, 0};   // N, L, S, O
static bool rsConnects(int nb) {
    return nb == XB_DUST || (nb >= XB_RTORCH && nb < XB_RTORCH + 5) || xbIsLever(nb) || nb == XB_PLATE || nb == XB_BTN || nb == XB_REPT || nb == XB_COMP || nb == XB_OBS || nb == XB_HOOK || (gRsRB && nb == gRsRB);
}
static int rsDustConn(int x, int y, int z) {
    int m = 0;
    for (int d = 0; d < 4; d++) {
        const int nx = x + RS_DX[d], nz = z + RS_DZ[d], nb = getB(nx, y, nz);
        if (rsConnects(nb) || (nb == AIR && getB(nx, y - 1, nz) == XB_DUST) || (!solid(getB(x, y + 1, z)) && getB(nx, y + 1, nz) == XB_DUST)) m |= 1 << d;
    }
    return m;
}
static void rsDustFix(int x, int y, int z) {
    const int old = getM(x, y, z), nm = (old & 0xF0) | rsDustConn(x, y, z);
    if (nm != old) { setM(x, y, z, nm); netSendBlock(x, y, z, XB_DUST, nm); }
}
static bool rsSupported(int b, int x, int y, int z) {
    const int m = getM(x, y, z), below = getB(x, y - 1, z);
    switch (b - XB_DUST) {
    case 0: case 6: case 11: case 12: return solid(below);
    case 7: { const int f = m & 7; if (f == 0) return solid(below); if (f == 1) return solid(getB(x, y + 1, z)); const int d = f - 2; return solid(getB(x - RS_DX[d], y, z - RS_DZ[d])); }
    case 8: case 20: { const int d = m & 3; return solid(getB(x - RS_DX[d], y, z - RS_DZ[d])); }
    case 17: return below != AIR;
    case 28: return below == XB_FARM;
    case 25: { int wd, d; bool wl; signDec(m, wd, wl, d); if (wl) return solid(getB(x - RS_DX[d], y, z - RS_DZ[d])); } return solid(below);
    case 24: { const int d = m & 3, s = (m & 4) ? -1 : 1; return getB(x + s * RS_DX[d], y, z + s * RS_DZ[d]) == b; }   // cama: as duas metades andam juntas
    case 15: return (m >> 3) & 1 ? getB(x, y - 1, z) == b : (solid(below) && getB(x, y + 1, z) == b);
    }
    return true;
}
static float rsHard(int b) {
    { const int kk = b - XB_DUST; if (kk == 26 || kk == 27) return 2.f; if (kk == 28) return 0.f; if (kk == 29) return .6f; }
    switch (b - XB_DUST) { case 0: case 8: case 11: case 12: return 0.f; case 1: case 2: case 6: case 7: return .5f; case 3: case 4: case 5: return 3.5f; case 9: return .2f; case 10: return 3.f;
        case 13: case 15: return 3.f; case 14: return 2.f; case 17: return .1f; case 18: return 2.f; case 19: return .5f; case 20: return .4f; case 21: return 2.f; case 22: return .3f; case 23: return 0.f; case 24: return .2f; case 25: return 1.f; default: return 2.5f; }
}
static int rsTool(int b) { const int k = b - XB_DUST; return (k >= 1 && k <= 6) || k == 10 || k == 21 ? 1 : (k == 13 || k == 14 || k == 15 || k == 16 || k == 18 || k == 25) ? 2 : 0; }
static int rsDropItem(int x, int y, int z, int b) {
    const int m = getB(x, y, z) == b ? getM(x, y, z) : gRsLastMeta;   // o bloco ja saiu: o setBM guardou o meta
    if (b == XB_DOOR && ((m >> 3) & 1)) return 0;
    if (b == XB_FARM) return DIRT; if (b == XB_CROP) return ((m >> 3) & 3) == 1 ? IT_CARROT : 0;
    if (b == XB_BED && ((m >> 2) & 1)) return 0;   // so os pes da cama dropam   // metade de cima da porta nao dropa
    int v = rsVariant(b, m); if (b == XB_CHEST && v >= 19 && v <= 22) v = (v - 19) >> 1;   // metade de bau duplo dropa o bau normal/armadilhado
    const int it = gRsDrop[b - XB_DUST][v & 31];
    if (it) return it;
    for (int i = 0; i < 32; i++) if (gRsDrop[b - XB_DUST][i]) return gRsDrop[b - XB_DUST][i];
    return 0;
}

// ---- conexoes de cerca / painel: bits 0-3 = N L S O ----
static bool rsFullCube(int nb) {   // BlockFaceShape.SOLID do 1.12.2 e nao excluido (Block.isExcludedBlock)
    if (nb == AIR) return false;
    if (isBlk(nb)) return solid(nb) && !isLeafB(nb);
    if (!isPk(nb) || !PK[nb - PK0].cube) return false;
    const std::string& f = PK[nb - PK0].file;
    for (const char* x : {"leaves", "pumpkin", "melon", "jack_o_lantern", "beacon", "cactus", "end_portal_frame", "shulker"}) if (f.find(x) != std::string::npos) return false;
    return true;
}
static int rsConnMask(int b, int var, int x, int y, int z) {
    int m = 0;
    for (int d = 0; d < 4; d++) {
        const int nx = x + RS_DX[d], nz = z + RS_DZ[d], nb = getB(nx, y, nz), nm = nb == AIR ? 0 : getM(nx, y, nz); bool c = false;
        if (b == XB_FENCE) { if (nb == XB_FENCE) c = (((nm >> 4) & 7) == 6) == (var == 6); else if (nb == XB_GATE) c = var != 6 && ((d & 1) ? !(nm & 1) : (nm & 1) != 0); else c = rsFullCube(nb); }
        else if (b == XB_WALL) c = nb == XB_WALL || (nb == XB_GATE && ((d & 1) ? !(nm & 1) : (nm & 1) != 0)) || rsFullCube(nb);
        else c = nb == XB_PANE || nb == XB_CPANE || rsFullCube(nb);
        if (c) m |= 1 << d;
    }
    if (b == XB_WALL && getB(x, y + 1, z) != AIR) m |= 128;   // 1.12: UP = !linha reta || bloco em cima
    return m;
}
static void rsConnFix(int x, int y, int z, int b) {
    const int old = getM(x, y, z), nm = (old & (b == XB_WALL ? 0x70 : 0xF0)) | rsConnMask(b, (old >> 4) & 7, x, y, z);
    if (nm != old) { setM(x, y, z, nm); netSendBlock(x, y, z, b, nm); }
}
// ---- colocar ----
static bool rsPlace(int xb, int var, const int* hh, const int* pp, float yawDeg, double px, double py, double pz) {
    int t[3] = {pp[0], pp[1], pp[2]};
    if (isXB(getB(hh[0], hh[1], hh[2])) && xbReplaceable(getB(hh[0], hh[1], hh[2]))) { t[0] = hh[0]; t[1] = hh[1]; t[2] = hh[2]; }
    int n[3] = {t[0] - hh[0], t[1] - hh[1], t[2] - hh[2]};
    if (abs(n[0]) + abs(n[1]) + abs(n[2]) != 1) { n[0] = 0; n[1] = 1; n[2] = 0; }
    if (t[1] < 0 || t[1] + 1 >= CH || !xbFree(getB(t[0], t[1], t[2]))) return false;
    const int pf = facingFromYaw(yawDeg), opp = (pf + 2) & 3;   // pf = para onde o jogador olha
    const int nd = n[2] == -1 ? 0 : n[0] == 1 ? 1 : n[2] == 1 ? 2 : 3;   // direcao horizontal da normal clicada
    const int below = getB(t[0], t[1] - 1, t[2]);
    const double fy = gRay.hit ? gRay.hy - floor(gRay.hy) : .5, fx = gRay.hit ? gRay.hx - floor(gRay.hx) : .5, fz = gRay.hit ? gRay.hz - floor(gRay.hz) : .5;
    int id = xb, meta = 0, id2 = 0, meta2 = 0;
    switch (xb - XB_DUST) {
    case 0: if (!solid(below)) return false; meta = rsDustConn(t[0], t[1], t[2]); break;
    case 1: case 3: case 4: case 5: {   // frente para o jogador (Block.getFacingFromEntity do 1.12)
        const double dx = px - (t[0] + .5), dy = py + 1.62 - (t[1] + .5), dz = pz - (t[2] + .5);
        int d; if (fabs(dx) < 2 && fabs(dz) < 2 && dy > 2) d = 1; else if (fabs(dx) < 2 && fabs(dz) < 2 && dy < 0) d = 0; else d = fabs(dx) > fabs(dz) ? (dx > 0 ? 3 : 5) : (dz > 0 ? 4 : 2);
        meta = d; if (xb == XB_PIST) id = XB_PIST + (var ? 1 : 0); break; }
    case 6: if (!solid(below)) return false; meta = var & 15; break;
    case 7: { if (!solid(getB(hh[0], hh[1], hh[2]))) return false; const int f = n[1] == 1 ? 0 : n[1] == -1 ? 1 : 2 + nd; meta = f | ((var & 1) << 3); break; }
    case 8: case 20: if (n[1] != 0 || !solid(getB(hh[0], hh[1], hh[2]))) return false; meta = nd; break;
    case 9: break;
    case 10: meta = n[1] != 0 ? 0 : 1 + ((nd + 2) & 3); break;   // o funil aponta para longe da face clicada
    case 11: case 12: if (!solid(below)) return false; meta = pf; break;
    case 26: case 27: { const bool st = xb == XB_STAIR; const int vv = var & 15, hb = getB(hh[0], hh[1], hh[2]);
        if (!st && hb == XB_SLAB && (getM(hh[0], hh[1], hh[2]) >> 2) == vv && (((getM(hh[0], hh[1], hh[2]) & 3) == 0 && n[1] == 1) || ((getM(hh[0], hh[1], hh[2]) & 3) == 1 && n[1] == -1))) {   // laje + laje igual = dupla
            setBM(hh[0], hh[1], hh[2], XB_SLAB, (vv << 2) | 2); netSendBlock(hh[0], hh[1], hh[2], XB_SLAB, (vv << 2) | 2); return true; }
        const bool top = n[1] == -1 || (n[1] == 0 && fy >= .5); meta = st ? (pf | (top << 2) | (vv << 3)) : ((top ? 1 : 0) | (vv << 2)); break; }
    case 13: { int f, top; if (n[1] == 0) { f = nd; top = fy >= .5; } else { f = opp; top = n[1] == -1; } meta = f | (top << 2) | ((var & 7) << 4); break; }
    case 14: meta = pf | ((var & 7) << 3); break;
    case 15: {
        if (!solid(below) || !xbFree(getB(t[0], t[1] + 1, t[2]))) return false;
        const double u = pf == 0 ? fx : pf == 1 ? fz : pf == 2 ? 1 - fx : 1 - fz;   // lado do cursor em relacao ao jogador => dobradica
        meta = opp | ((u >= .5 ? 1 : 0) << 4) | ((var & 7) << 5); meta2 = meta | 8; id2 = xb; break; }
    case 16: meta = opp | ((var & 31) << 2); if (var <= 1) { const int pm = chestPlaceMeta(t[0], t[1], t[2], var, opp); if (pm < 0) return false; meta = pm; } break;   // BlockChest.canPlaceBlockAt + juncao em bau duplo   // 0 bau, 1 armadilhado, 2 ender, 3..18 caixa de shulker (cor = var-3)

    case 17: if (below == AIR) return false; meta = var & 15; break;
    case 18: meta = rsConnMask(XB_FENCE, var, t[0], t[1], t[2]) | ((var & 7) << 4); break;
    case 22: meta = rsConnMask(XB_CPANE, var, t[0], t[1], t[2]) | ((var & 15) << 4); break;
    case 23: if (!solid(getB(hh[0], hh[1], hh[2]))) return false; meta = n[1] == 1 ? 0 : n[1] == -1 ? 1 : 2 + nd; break;   // aponta para longe da face clicada
    case 24: { const int hx = t[0] + RS_DX[pf], hz = t[2] + RS_DZ[pf];   // cabeca na frente do jogador, os dois lados precisam de chao
        if (!solid(below) || !solid(getB(hx, t[1] - 1, hz)) || !xbFree(getB(hx, t[1], hz))) return false;
        meta = pf | ((var & 15) << 3);
        setBM(hx, t[1], hz, xb, meta | 4); netSendBlock(hx, t[1], hz, xb, meta | 4); setBM(t[0], t[1], t[2], xb, meta); netSendBlock(t[0], t[1], t[2], xb, meta); return true; }
    case 25: { const int wd = std::max(0, std::min(var, SIGN_N - 1));   // topo = placa de pe (de frente para o jogador); lado = placa na parede
        if (n[1] == 1) { if (!solid(below)) return false; meta = wd * 20 + (((int)floorf(-yawDeg / 22.5f + .5f)) & 15); }   // 16 rotacoes, frente para o jogador
        else if (n[1] == 0) { if (!solid(getB(hh[0], hh[1], hh[2]))) return false; meta = wd * 20 + 16 + nd; }
        else return false; break; }
    case 21: meta = rsConnMask(XB_WALL, var, t[0], t[1], t[2]) | ((var & 7) << 4); break;
    case 19: meta = rsConnMask(XB_PANE, var, t[0], t[1], t[2]) | ((var & 1) << 4); break;
    default: return false;
    }
    AABB tmp[SH_MAXB]; const int nb = blockBoxesId(id, t[0], t[1], t[2], false, tmp, meta);
    const AABB pl{px - .3, py, pz - .3, px + .3, py + 1.8, pz + .3};
    for (int i = 0; i < nb; i++) if (pl.intersects(tmp[i])) return false;
    if (id2) { setBM(t[0], t[1] + 1, t[2], id2, meta2); netSendBlock(t[0], t[1] + 1, t[2], id2, meta2); }   // porta: metade de cima primeiro
    setBM(t[0], t[1], t[2], id, meta); netSendBlock(t[0], t[1], t[2], id, meta);
    if (id == XB_CHEST) chestAfterPlace(t[0], t[1], t[2], meta);   // a parceira assume a frente do bau novo
    return true;
}

// ---- usar (botao direito): abre/fecha, atraso do repetidor, modo do comparador ----
static bool rsUse(int x, int y, int z) {
    const int b = getB(x, y, z); if (!isRs(b)) return false; const int m = getM(x, y, z); int nm = m;
    if (b == XB_BED) { gSpX = x + .5; gSpY = y + .6; gSpZ = z + .5; if (gWorldTime % DAY_TICKS >= 12500) gWorldTime = (gWorldTime / DAY_TICKS + 1) * DAY_TICKS; return true; }   // dormir: define o spawn e pula a noite
    auto put = [&](int xx, int yy, int zz, int id, int mm) { setBM(xx, yy, zz, id, mm); netSendBlock(xx, yy, zz, id, mm); };
    if (b == XB_TRAP && ((m >> 4) & 7) != 0) nm = m ^ 8;
    else if (b == XB_GATE) nm = m ^ 4;
    else if (b == XB_REPT) nm = (m & ~12) | ((((m >> 2) & 3) + 1) & 3) << 2;
    else if (b == XB_COMP) nm = m ^ 4;
    else if (b == XB_DOOR && ((m >> 5) & 7) != 0) { nm = m ^ 4; const int oy = ((m >> 3) & 1) ? y - 1 : y + 1; if (getB(x, oy, z) == b) put(x, oy, z, b, getM(x, oy, z) ^ 4); }
    else return false;
    put(x, y, z, b, nm); return true;
}

// ---- icone 3D no inventario (placa, botao, portao, alcapao, sensor, bau): usa a propria malha do bloco ----
static const std::vector<XQuad>* rsIconQuads(int id) {
    auto g = gXbItem.find(id); if (g != gXbItem.end() && g->second == XB_BED && entSprite(0, rsItemVar(id)) >= 0) return nullptr;   // cama no inventario/mao/chao = sprite 16x16 (item plano), nao o modelo 3D
    if (g != gXbItem.end() && g->second == XB_ANVIL) { auto gv = gXbVar.find(id); return &gAnvQ[std::min(gv != gXbVar.end() ? gv->second : 0, 2)][0]; }   // bigorna (normal/rachada/quebrada): modelo real
    if (g == gXbItem.end() || !isRs(g->second)) return nullptr; const int b = g->second, v = rsItemVar(id); int m;
    if (b == XB_PLATE) m = v; else if (b == XB_BTN || b == XB_GATE) m = v << 3; else if (b == XB_TRAP) m = v << 4; else if (b == XB_DAY) m = 0; else if (b == XB_CHEST) m = (v << 2) | 2; else if (b == XB_CARPET) m = v; else if (b == XB_FENCE) m = (v << 4) | 10; else if (b == XB_WALL) m = (v << 4) | 10 | 128; else if (b == XB_ENDROD) m = 0; else if (b == XB_BED) m = (v << 3) | 128; else if (b == XB_SIGN) m = 240 + v; else if (b == XB_STAIR) m = v << 3; else if (b == XB_SLAB) m = v << 2; else return nullptr;
    const std::vector<XQuad>& q = rsQuads(b, m); return q.empty() ? nullptr : &q;
}
// ---- itens da aba Redstone -> bloco + variante (chamado por loadCreativeCfg, antes de traduzir os nomes) ----
static void rsMapItems() {
    struct Row { const char* key; int blk, var; };
    static const Row R[] = {
        {"Oak Wood Stairs", XB_STAIR, 0}, {"Cobblestone Stairs", XB_STAIR, 1}, {"Spruce Wood Stairs", XB_STAIR, 2}, {"Birch Wood Stairs", XB_STAIR, 3}, {"Jungle Wood Stairs", XB_STAIR, 4}, {"Acacia Wood Stairs", XB_STAIR, 5},
        {"Dark Oak Wood Stairs", XB_STAIR, 6}, {"Sandstone Stairs", XB_STAIR, 7}, {"Stone Brick Stairs", XB_STAIR, 8}, {"Brick Stairs", XB_STAIR, 9},
        {"Oak Wood Slab", XB_SLAB, 0}, {"Cobblestone Slab", XB_SLAB, 1}, {"Spruce Wood Slab", XB_SLAB, 2}, {"Birch Wood Slab", XB_SLAB, 3}, {"Jungle Wood Slab", XB_SLAB, 4}, {"Acacia Wood Slab", XB_SLAB, 5},
        {"Dark Oak Wood Slab", XB_SLAB, 6}, {"Sandstone Slab", XB_SLAB, 7}, {"Stone Brick Slab", XB_SLAB, 8}, {"Bricks Slab", XB_SLAB, 9}, {"Stone Slab", XB_SLAB, 10},
        {"block/dispenser_front", XB_DISP, 0}, {"block/dropper_front", XB_DROP, 0}, {"block/observer_front", XB_OBS, 0}, {"block/observer_top", XB_OBS, 0}, {"Sticky Piston", XB_PIST, 1}, {"block/piston_top", XB_PIST, 0},
        {"Stone Pressure Plate", XB_PLATE, 0}, {"Oak Pressure Plate", XB_PLATE, 1}, {"Spruce Pressure Plate", XB_PLATE, 4}, {"Birch Pressure Plate", XB_PLATE, 5}, {"Jungle Pressure Plate", XB_PLATE, 6}, {"Acacia Pressure Plate", XB_PLATE, 7}, {"Dark Oak Pressure Plate", XB_PLATE, 8}, {"Mangrove Pressure Plate", XB_PLATE, 9}, {"Cherry Pressure Plate", XB_PLATE, 10}, {"Bamboo Pressure Plate", XB_PLATE, 11}, {"Crimson Pressure Plate", XB_PLATE, 12}, {"Warped Pressure Plate", XB_PLATE, 13}, {"Pale Oak Pressure Plate", XB_PLATE, 14},  {"Light Weighted Pressure Plate", XB_PLATE, 2}, {"Heavy Weighted Pressure Plate", XB_PLATE, 3},
        {"Stone Button", XB_BTN, 0}, {"Oak Button", XB_BTN, 1}, {"block/tripwire_hook", XB_HOOK, 0}, {"Chest", XB_CHEST, 0}, {"Trapped Chest", XB_CHEST, 1}, {"Ender Chest", XB_CHEST, 2}, {"block/daylight_detector_top", XB_DAY, 0},
        {"block/hopper_outside", XB_HOPPER, 0}, {"item/redstone", XB_DUST, 0}, {"item/repeater", XB_REPT, 0}, {"item/comparator", XB_COMP, 0},
        {"Iron Trapdoor", XB_TRAP, 0}, {"Oak Wood Trapdoor", XB_TRAP, 1}, {"Spruce Wood Trapdoor", XB_TRAP, 2}, {"Birch Wood Trapdoor", XB_TRAP, 3}, {"Jungle Wood Trapdoor", XB_TRAP, 4}, {"Acacia Wood Trapdoor", XB_TRAP, 5}, {"Dark Oak Wood Trapdoor", XB_TRAP, 6},
        {"Oak Fence Gate", XB_GATE, 0}, {"Spruce Fence Gate", XB_GATE, 1}, {"Birch Fence Gate", XB_GATE, 2}, {"Jungle Fence Gate", XB_GATE, 3}, {"Acacia Fence Gate", XB_GATE, 4}, {"Dark Oak Fence Gate", XB_GATE, 5},
        {"Oak Fence", XB_FENCE, 0}, {"Spruce Fence", XB_FENCE, 1}, {"Birch Fence", XB_FENCE, 2}, {"Jungle Fence", XB_FENCE, 3}, {"Acacia Fence", XB_FENCE, 4}, {"Dark Oak Fence", XB_FENCE, 5}, {"Nether Brick Fence", XB_FENCE, 6},
        {"block/end_rod", XB_ENDROD, 0},
        {"block/white_stained_glass_pane_top", XB_CPANE, 0}, {"block/orange_stained_glass_pane_top", XB_CPANE, 1}, {"block/magenta_stained_glass_pane_top", XB_CPANE, 2}, {"block/light_blue_stained_glass_pane_top", XB_CPANE, 3}, {"block/yellow_stained_glass_pane_top", XB_CPANE, 4}, {"block/lime_stained_glass_pane_top", XB_CPANE, 5}, {"block/pink_stained_glass_pane_top", XB_CPANE, 6}, {"block/gray_stained_glass_pane_top", XB_CPANE, 7}, {"block/light_gray_stained_glass_pane_top", XB_CPANE, 8}, {"block/cyan_stained_glass_pane_top", XB_CPANE, 9}, {"block/purple_stained_glass_pane_top", XB_CPANE, 10}, {"block/blue_stained_glass_pane_top", XB_CPANE, 11}, {"block/brown_stained_glass_pane_top", XB_CPANE, 12}, {"block/green_stained_glass_pane_top", XB_CPANE, 13}, {"block/red_stained_glass_pane_top", XB_CPANE, 14}, {"block/black_stained_glass_pane_top", XB_CPANE, 15},
        {"White Bed", XB_BED, 0}, {"Orange Bed", XB_BED, 1}, {"Magenta Bed", XB_BED, 2}, {"Light Blue Bed", XB_BED, 3}, {"Yellow Bed", XB_BED, 4}, {"Lime Bed", XB_BED, 5}, {"Pink Bed", XB_BED, 6}, {"Gray Bed", XB_BED, 7}, {"Light Gray Bed", XB_BED, 8}, {"Cyan Bed", XB_BED, 9}, {"Purple Bed", XB_BED, 10}, {"Blue Bed", XB_BED, 11}, {"Brown Bed", XB_BED, 12}, {"Green Bed", XB_BED, 13}, {"Red Bed", XB_BED, 14}, {"Black Bed", XB_BED, 15},
        {"item/oak_sign", XB_SIGN, 0}, {"item/spruce_sign", XB_SIGN, 1}, {"item/birch_sign", XB_SIGN, 2}, {"item/jungle_sign", XB_SIGN, 3}, {"item/acacia_sign", XB_SIGN, 4}, {"item/dark_oak_sign", XB_SIGN, 5}, {"item/mangrove_sign", XB_SIGN, 6}, {"item/cherry_sign", XB_SIGN, 7}, {"item/bamboo_sign", XB_SIGN, 8}, {"item/crimson_sign", XB_SIGN, 9}, {"item/warped_sign", XB_SIGN, 10}, {"item/pale_oak_sign", XB_SIGN, 11},
        {"Cobblestone Wall", XB_WALL, 0}, {"Mossy Cobblestone Wall", XB_WALL, 1}, {"block/ladder", XB_LADDER, 0}, {"block/iron_bars", XB_PANE, 0}, {"block/glass_pane_top", XB_PANE, 1},
        {"White Carpet", XB_CARPET, 0}, {"Orange Carpet", XB_CARPET, 1}, {"Magenta Carpet", XB_CARPET, 2}, {"Light Blue Carpet", XB_CARPET, 3}, {"Yellow Carpet", XB_CARPET, 4}, {"Lime Carpet", XB_CARPET, 5}, {"Pink Carpet", XB_CARPET, 6}, {"Gray Carpet", XB_CARPET, 7},
        {"Light Gray Carpet", XB_CARPET, 8}, {"Cyan Carpet", XB_CARPET, 9}, {"Purple Carpet", XB_CARPET, 10}, {"Blue Carpet", XB_CARPET, 11}, {"Brown Carpet", XB_CARPET, 12}, {"Green Carpet", XB_CARPET, 13}, {"Red Carpet", XB_CARPET, 14}, {"Black Carpet", XB_CARPET, 15},
        {"White Shulker Box", XB_CHEST, 3}, {"Orange Shulker Box", XB_CHEST, 4}, {"Magenta Shulker Box", XB_CHEST, 5}, {"Light Blue Shulker Box", XB_CHEST, 6}, {"Yellow Shulker Box", XB_CHEST, 7}, {"Lime Shulker Box", XB_CHEST, 8}, {"Pink Shulker Box", XB_CHEST, 9}, {"Gray Shulker Box", XB_CHEST, 10}, {"Light Gray Shulker Box", XB_CHEST, 11}, {"Cyan Shulker Box", XB_CHEST, 12}, {"Purple Shulker Box", XB_CHEST, 13}, {"Blue Shulker Box", XB_CHEST, 14}, {"Brown Shulker Box", XB_CHEST, 15}, {"Green Shulker Box", XB_CHEST, 16}, {"Red Shulker Box", XB_CHEST, 17}, {"Black Shulker Box", XB_CHEST, 18}, 
        {"item/iron_door", XB_DOOR, 0}, {"item/oak_door", XB_DOOR, 1}, {"item/spruce_door", XB_DOOR, 2}, {"item/birch_door", XB_DOOR, 3}, {"item/jungle_door", XB_DOOR, 4}, {"item/acacia_door", XB_DOOR, 5}, {"item/dark_oak_door", XB_DOOR, 6}};
    memset(gRsDrop, 0, sizeof gRsDrop); memset(gBnDrop, 0, sizeof gBnDrop); gRsVar.clear();
    auto gb = gPkByFile.find("block/redstone_block"); gRsRB = gb != gPkByFile.end() ? gb->second : 0;
    if (gCreative[2].empty()) return;
    for (int tab = 0; tab <= 2; tab++) {   // abas Decoracao (1) e Redstone (2)
    for (const Row& r : R) { auto g = gPackId.find(r.key); if (g == gPackId.end() || isPk(g->second)) continue;   // item do JOGO (id < 1000) com textura do pack (ex.: item/redstone)
        if (std::find(gCreative[tab].begin(), gCreative[tab].end(), g->second) == gCreative[tab].end()) continue;
        gXbItem[g->second] = r.blk; gRsVar[g->second] = r.var; if (!gRsDrop[r.blk - XB_DUST][r.var & 31]) gRsDrop[r.blk - XB_DUST][r.var & 31] = g->second; }
    for (int id : gCreative[tab]) {
        if (!isPk(id)) continue; PkItem& e = PK[id - PK0]; const Row* hit = nullptr; bool byName = false;
        for (const Row& r : R) if (e.name == r.key) { hit = &r; byName = true; break; }
        if (!hit) for (const Row& r : R) if (e.file == r.key) { hit = &r; break; }
        if (!hit) continue;
        gXbItem[id] = hit->blk; gRsVar[id] = hit->var;
        if (!gRsDrop[hit->blk - XB_DUST][hit->var & 31]) gRsDrop[hit->blk - XB_DUST][hit->var & 31] = id;
        if (byName || hit->blk == XB_DAY || hit->blk == XB_HOPPER) e.cube = false;   // vira icone do modelo (rsIconQuads) ou plano, nunca cubo
        if (hit->blk == XB_HOPPER) for (const PkItem& q : PK) if (q.file == "item/hopper") { e.tile = q.tile; break; }   // funil: icone do item, como no Minecraft
        if (hit->blk == XB_LADDER || hit->blk == XB_ENDROD) e.cube = false;
        if (hit->blk == XB_CPANE) { e.cube = false; const std::string gf = std::string("block/") + RS_COLOR[hit->var & 15] + "_stained_glass"; for (const PkItem& q : PK) if (q.file == gf) { e.tile = q.tile; break; } }   // painel colorido: sprite do vidro   // escada: item plano (sprite)
        if (hit->blk == XB_PANE) { e.cube = false; if (hit->var == 1) for (const PkItem& q : PK) if (q.file == "block/glass") { e.tile = q.tile; break; } }   // painel/barras: item plano (sprite), como no Minecraft
    }
    }
    for (int tab = 1; tab <= 2; tab++) for (int id : gCreative[tab]) {   // bigornas: normal / levemente / muito danificada (mesmo bloco, dano no meta)
        if (!isPk(id)) continue; PkItem& e = PK[id - PK0];
        const int dm = e.name == "Slightly Damaged Anvil" ? 1 : e.name == "Very Damaged Anvil" ? 2 : (e.name == "Anvil" || e.file == "block/anvil") ? 0 : -1; if (dm < 0) continue;
        gXbItem[id] = XB_ANVIL; gXbVar[id] = dm; e.cube = false; if (!gAnvDrop[dm]) gAnvDrop[dm] = id;
    }
    { auto ia = gPkByFile.find("block/anvil"); if (ia != gPkByFile.end() && isPk(ia->second)) { PK[ia->second - PK0].cube = false; gXbItem[ia->second] = XB_ANVIL; gXbVar[ia->second] = 0; } }
    for (int tab = 1; tab <= 2; tab++) for (int id : gCreative[tab]) {   // cama e estandarte: icone = sprite 16x16 gerado da textura do pack (item plano, 100% opaco)
        if (!isPk(id)) continue; PkItem& e = PK[id - PK0];
        auto g = gXbItem.find(id);
        if (g != gXbItem.end() && g->second == XB_BED) { const int t = entSprite(0, rsItemVar(id)); if (t >= 0) { e.tile = t; e.cube = false; } continue; }
        for (int c = 0; c < 16; c++) {
            std::string n; bool up = true; for (const char* p = RS_COLOR[c]; *p; p++) { if (*p == '_') { n += ' '; up = true; continue; } n += up ? (char)toupper(*p) : *p; up = false; }
            if (e.name == n + " Banner") { const int t = entSprite(1, c); if (t >= 0) { e.tile = t; e.cube = false; if (entTile("banner/banner_base") >= 0) { gXbItem[id] = XB_BANNER; gXbVar[id] = c; if (!gBnDrop[c]) gBnDrop[c] = id; } } break; }
        }
    }
    gRsDrop[XB_PIST + 1 - XB_DUST][0] = gRsDrop[XB_PIST - XB_DUST][1]; gRsDrop[XB_PIST - XB_DUST][1] = 0;   // variante 1 do pistao = o grudento (outro id)
}
static void rsInit() { memset(gRsB, 0, sizeof gRsB); for (auto& r : gRsQ) for (auto& v : r) v.clear(); }

// ---- nomes em portugues (aba Redstone); roda DEPOIS de pkApplyItemData ----
static void rsNames() {
    struct N { int blk, var; const char* n; };
    static const N T[] = {{XB_PIST, 0, "Pistão"}, {XB_PIST, 1, "Pistão pegajoso"}, {XB_DISP, 0, "Dispensador"}, {XB_DROP, 0, "Largador"}, {XB_OBS, 0, "Observador"},
        {XB_PLATE, 0, "Placa de pressão de pedra"}, {XB_PLATE, 1, "Placa de pressão de carvalho"}, {XB_PLATE, 4, "Placa de pressão de abeto"}, {XB_PLATE, 5, "Placa de pressão de bétula"}, {XB_PLATE, 6, "Placa de pressão de selva"}, {XB_PLATE, 7, "Placa de pressão de acácia"}, {XB_PLATE, 8, "Placa de pressão de carvalho escuro"}, {XB_PLATE, 9, "Placa de pressão de mangue"}, {XB_PLATE, 10, "Placa de pressão de cerejeira"}, {XB_PLATE, 11, "Placa de pressão de bambu"}, {XB_PLATE, 12, "Placa de pressão de carmesim"}, {XB_PLATE, 13, "Placa de pressão de distorcida"}, {XB_PLATE, 14, "Placa de pressão de carvalho pálido"},  {XB_PLATE, 2, "Placa de pressão leve"}, {XB_PLATE, 3, "Placa de pressão pesada"},
        {XB_BTN, 0, "Botão de pedra"}, {XB_BTN, 1, "Botão de carvalho"}, {XB_HOOK, 0, "Gancho de fio"}, {XB_CHEST, 0, "Baú"}, {XB_CHEST, 1, "Baú armadilhado"}, {XB_CHEST, 2, "Baú do Ender"}, {XB_CHEST, 3, "Caixa de shulker branca"}, {XB_CHEST, 4, "Caixa de shulker laranja"}, {XB_CHEST, 5, "Caixa de shulker magenta"}, {XB_CHEST, 6, "Caixa de shulker azul-clara"}, {XB_CHEST, 7, "Caixa de shulker amarela"}, {XB_CHEST, 8, "Caixa de shulker verde-limão"}, {XB_CHEST, 9, "Caixa de shulker rosa"}, {XB_CHEST, 10, "Caixa de shulker cinza"}, {XB_CHEST, 11, "Caixa de shulker cinza-clara"}, {XB_CHEST, 12, "Caixa de shulker ciano"}, {XB_CHEST, 13, "Caixa de shulker roxa"}, {XB_CHEST, 14, "Caixa de shulker azul"}, {XB_CHEST, 15, "Caixa de shulker marrom"}, {XB_CHEST, 16, "Caixa de shulker verde"}, {XB_CHEST, 17, "Caixa de shulker vermelha"}, {XB_CHEST, 18, "Caixa de shulker preta"},  {XB_DAY, 0, "Sensor de luz do dia"},
        {XB_HOPPER, 0, "Funil"}, {XB_DUST, 0, "Redstone"}, {XB_REPT, 0, "Repetidor de redstone"}, {XB_COMP, 0, "Comparador de redstone"},
        {XB_TRAP, 0, "Alçapão de ferro"}, {XB_TRAP, 1, "Alçapão de carvalho"}, {XB_TRAP, 2, "Alçapão de abeto"}, {XB_TRAP, 3, "Alçapão de bétula"}, {XB_TRAP, 4, "Alçapão da selva"}, {XB_TRAP, 5, "Alçapão de acácia"}, {XB_TRAP, 6, "Alçapão de carvalho escuro"},
        {XB_GATE, 0, "Portão de carvalho"}, {XB_GATE, 1, "Portão de abeto"}, {XB_GATE, 2, "Portão de bétula"}, {XB_GATE, 3, "Portão da selva"}, {XB_GATE, 4, "Portão de acácia"}, {XB_GATE, 5, "Portão de carvalho escuro"},
        {XB_DOOR, 0, "Porta de ferro"}, {XB_DOOR, 1, "Porta de carvalho"}, {XB_DOOR, 2, "Porta de abeto"}, {XB_DOOR, 3, "Porta de bétula"}, {XB_DOOR, 4, "Porta da selva"}, {XB_DOOR, 5, "Porta de acácia"}, {XB_DOOR, 6, "Porta de carvalho escuro"},
        {XB_SIGN, 0, "Placa de carvalho"}, {XB_SIGN, 1, "Placa de abeto"}, {XB_SIGN, 2, "Placa de bétula"}, {XB_SIGN, 3, "Placa de selva"}, {XB_SIGN, 4, "Placa de acácia"}, {XB_SIGN, 5, "Placa de carvalho escuro"}, {XB_SIGN, 6, "Placa de mangue"}, {XB_SIGN, 7, "Placa de cerejeira"}, {XB_SIGN, 8, "Placa de bambu"}, {XB_SIGN, 9, "Placa de carmesim"}, {XB_SIGN, 10, "Placa de distorcida"}, {XB_SIGN, 11, "Placa de carvalho pálido"}};
    struct F { const char* file; const char* n; };
    static const F FL[] = {{"block/note_block", "Bloco de notas"}, {"block/tnt_top", "TNT"}, {"block/redstone_lamp", "Lâmpada de redstone"}, {"block/redstone_block", "Bloco de redstone"}, {"block/lever", "Alavanca"}, {"block/redstone_torch", "Tocha de redstone"}};
    auto set = [](PkItem& e, const char* n) { e.name = n; e.fold = foldAscii(n); };
    for (int tab = 1; tab <= 2; tab++) for (int id : gCreative[tab]) {
        if (!isPk(id)) continue; PkItem& e = PK[id - PK0];
        auto g = gXbItem.find(id);
        if (g != gXbItem.end() && isRs(g->second)) { const int v = rsItemVar(id); for (const N& t : T) if (t.blk == g->second && t.var == v) { set(e, t.n); break; } continue; }
        for (const F& f : FL) if (e.file == f.file) { set(e, f.n); break; }
    }
    static const char* BN[16] = {"Estandarte branco", "Estandarte laranja", "Estandarte magenta", "Estandarte azul-claro", "Estandarte amarelo", "Estandarte verde-limão", "Estandarte rosa", "Estandarte cinza",
        "Estandarte cinza-claro", "Estandarte ciano", "Estandarte roxo", "Estandarte azul", "Estandarte marrom", "Estandarte verde", "Estandarte vermelho", "Estandarte preto"};
    for (auto& kv : gXbVar) if (isPk(kv.first)) {   // gXbVar e compartilhado (dano da bigorna, cor do estandarte): o nome depende do BLOCO do item
        auto g = gXbItem.find(kv.first); if (g == gXbItem.end()) continue;
        if (g->second == XB_ANVIL) set(PK[kv.first - PK0], kv.second == 1 ? "Bigorna levemente danificada" : kv.second == 2 ? "Bigorna muito danificada" : "Bigorna");
        else if (g->second == XB_BANNER) set(PK[kv.first - PK0], BN[kv.second & 15]);
    }
}
