// ---------- aba REDSTONE: modelo + colocacao de todos os itens ----------
// 17 ids XB (XB_DUST..XB_END-1). O ESTADO vai no metadado (1 byte), e a malha e montada sob demanda por (id, meta) e guardada em cache (como os trilhos).
// Modelos vem do PROPRIO pack quando existem (porta, alcapao, portao, funil, gancho); o resto (poeira, pistao, placa, botao, sensor, repetidor,
// comparador, bau) e montado aqui com caixas + texturas do pack. SEM logica de redstone ainda: so modelo, colocacao, suporte, forma, drop e uso (abrir/fechar).
// Metadados: poeira conexoes N,E,S,W=bits0-3 | pistao/dispenser/dropper/observador dir6 (0 baixo,1 cima,2 N,3 L,4 S,5 O) | placa tipo(0-3)|pressionada<<2
// botao face(0 chao,1 teto,2..5 parede N,L,S,O)|tipo<<3|press<<4 | gancho dir(0-3)|lig<<2 | sensor invertido bit0 | funil 0 baixo,1..4 N,L,S,O
// repetidor dir|atraso<<2|lig<<4 | comparador dir|subtrai<<2|lig<<3 | alcapao dir|metade-de-cima<<2|aberto<<3|tipo<<4 | portao dir|aberto<<2|tipo<<3
// porta lado-da-lamina|aberta<<2|parte-de-cima<<3|dobradica-direita<<4|tipo<<5 | bau dir|tipo<<2 (0 normal, 1 armadilhado, 2 ender).   dir horizontal: 0 N(-Z) 1 L(+X) 2 S(+Z) 3 O(-X)
static const int RS_N = XB_END - XB_DUST;
static void pistonCheck(int x, int y, int z); static void pistonScan(int x, int y, int z);
static void rsTorchCheck(int x, int y, int z); static void rsTorchNotify(int x, int y, int z); static void rsTorchAttach(int id, int x, int y, int z, int& ax, int& ay, int& az); static bool rsBlockPowered(int ax, int ay, int az, int tx, int ty, int tz);
static bool rsRepInput(int x, int y, int z); static bool rsRepOutInto(int rx, int ry, int rz, int tx, int ty, int tz); static void rsDustRecalc(int x, int y, int z); static void rsRepCheck(int x, int y, int z);
// Bau duplo (1.12.2): tipo 19..22 no meta = metade de bau duplo (19 normal-esq, 20 normal-dir, 21 armadilhado-esq, 22 armadilhado-dir; esq/dir vistos de frente); bit 7 = tampa aberta.
// CH_RV[dir] = lado DIREITO visto de frente (a metade esquerda tem a parceira nesse lado). Logica em chest.inc.
static const int CH_RV[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
static int chestPlaceMeta(int x, int y, int z, int var, int dir); static void chestAfterPlace(int x, int y, int z, int meta);
static std::string gPlaceNbt; static void chestApplyNbt(int x, int y, int z, const std::string& nbt);   // NBT do item bau sendo colocado (main_loop.inc) -> conteudo do bau novo
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
static const char* RS_WOOD[8] = {"iron", "oak", "spruce", "birch", "jungle", "acacia", "dark_oak", "oak"};   // patchCrash.1: tipo 7 (meta de mundo antigo) = carvalho, em vez de ler fora da tabela
#include "entmodels.inc"   // bau / bau do Ender / cama com texturas de entidade 64x64 do pack
static inline int rsItemVar(int item) { auto it = gRsVar.find(item); return it == gRsVar.end() ? 0 : it->second; }
static inline int rsVariant(int b, int m) {
    if (b == XB_STAIR) return (m >> 3) & 15; if (b == XB_SLAB) return (m >> 2) & 15;
    if (b == XB_FARM) return m & 1; if (b == XB_DECOR) return m & 3; if (b == XB_PLATE) return m & 15; if (b == XB_BTN) return (m >> 3) & 1; if (b == XB_TRAP) return (m >> 4) & 7;
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
static std::string rsRod(float x1, float y1, float z1, float x2, float y2, float z2) {   // haste do pistao: madeira (centro de piston_top normal), nao o corpo de pedra do piston_side
    char b[420]; snprintf(b, sizeof b, "{\"from\":[%g,%g,%g],\"to\":[%g,%g,%g],\"faces\":{", x1, y1, z1, x2, y2, z2); std::string s = b; const float v0 = 16.f - y2, v1 = 16.f - y1;
    static const char* FN[4] = {"east", "west", "south", "north"};
    for (int i = 0; i < 4; i++) { snprintf(b, sizeof b, "%s\"%s\":{\"texture\":\"block/piston_top\",\"uv\":[6,%g,10,%g]}", i ? "," : "", FN[i], v0, v1); s += b; }
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
// FACILITADOR de laje/escada (patch5.71): UMA linha por material = {lado, topo, base}; o numero da linha e a variante (max 16: 4 bits no meta).
// Para adicionar um material: (1) uma linha aqui, (2) as linhas {"<Nome> Stairs"/"<Nome> Slab", XB_STAIR/XB_SLAB, <variante>} em R (rsRegister), (3) o nome em ptbr.inc se faltar.
static const char* const RS_SM[][3] = {
    {"oak_planks", 0, 0}, {"cobblestone", 0, 0}, {"spruce_planks", 0, 0}, {"birch_planks", 0, 0}, {"jungle_planks", 0, 0}, {"acacia_planks", 0, 0}, {"dark_oak_planks", 0, 0},   // 0..6
    {"sandstone", "sandstone_top", "sandstone_bottom"}, {"stone_bricks", 0, 0}, {"bricks", 0, 0}, {"stone_slab_side", "stone_slab_top", "stone_slab_top"},                                                              // 7..10
    {"nether_bricks", 0, 0}, {"quartz_block_side", "quartz_block_top", "quartz_block_bottom"}, {"red_sandstone", "red_sandstone_top", "red_sandstone_bottom"}, {"purpur_block", 0, 0}, {"oak_planks", 0, 0}};   // 11..15 (10 = laje de pedra lisa como no 1.12: stone_slab_side/top; 15 = laje de madeira petrificada)
static const int RS_SM_N = (int)(sizeof RS_SM / sizeof RS_SM[0]);
static void rsMat(int v, std::string& sd, std::string& tp, std::string& bt) {
    const char* const* r = RS_SM[std::max(0, std::min(v, RS_SM_N - 1))];
    sd = std::string("block/") + r[0]; tp = r[1] ? std::string("block/") + r[1] : sd; bt = r[2] ? std::string("block/") + r[2] : sd;
}
static void rsBuild(int b, int m, std::vector<XQuad>& o) {
    const int k = b - XB_DUST; int bx = 0, by = 0; const std::string W = "block/";
    switch (k) {
    case 26: { std::string sd, tp, bt; rsMat((m >> 3) & 15, sd, tp, bt); const bool t = (m >> 2) & 1; const char *a = sd.c_str(), *u = tp.c_str(), *d = bt.c_str();
        rsBake({rsBx(0, t ? 8 : 0, 0, 16, t ? 16 : 8, 16, a, a, u, d, a, a), rsBx(0, t ? 0 : 8, 0, 16, t ? 8 : 16, 8, a, a, t ? 0 : u, t ? d : 0, a, a)}, 0, 90 * (m & 3), o); break; }
    case 27: { std::string sd, tp, bt; rsMat((m >> 2) & 15, sd, tp, bt); const int h = m & 3; const char *a = sd.c_str(), *u = tp.c_str(), *d = bt.c_str();
        rsBake({rsBx(0, h == 1 ? 8 : 0, 0, 16, h == 0 ? 8 : 16, 16, a, a, u, d, a, a)}, 0, 0, o); break; }
    case 28: { static const int MX[5] = {7, 3, 3, 3, 3}; static const char* NM[5] = {"wheat", "carrots", "potatoes", "beetroots", "nether_wart"}; const int ty = (m >> 3) & 7, ag = std::min(m & 7, 7);
        const int st = ty == 4 ? (ag == 0 ? 0 : ag < 3 ? 1 : 2) : ty == 0 ? ag : ty == 3 ? (ag >> 1) : (ag < 2 ? 0 : ag < 4 ? 1 : ag < 7 ? 2 : 3); char t[48]; if (ty == 4) snprintf(t, sizeof t, "block/nether_wart_stage%d", st); else snprintf(t, sizeof t, "block/%s_stage%d", NM[ty], std::min(st, ty == 0 ? 7 : 3)); const char* q = t;
        rsBake({rsBx(4, -1, 0, 4, 15, 16, q, q, 0, 0, 0, 0), rsBx(12, -1, 0, 12, 15, 16, q, q, 0, 0, 0, 0), rsBx(0, -1, 4, 16, 15, 4, 0, 0, 0, 0, q, q), rsBx(0, -1, 12, 16, 15, 12, 0, 0, 0, 0, q, q)}, 0, 0, o); break; }   // modelo crop do vanilla: y -1..15 (na altura da terra arada, 15/16)
    case 32: if ((m & 3) == 2) {   // patchDeco.1: sinalizador (BlockBeacon 1.12): vidro 16^3 + base de obsidiana (2,0,2)-(14,3,14) + nucleo (3,3,3)-(13,13,13)
            rsBake({rsAll(2, 0, 2, 14, 3, 14, "block/obsidian"), rsAll(3, 3, 3, 13, 13, 13, "block/beacon"), rsAll(0, 0, 0, 16, 16, 16, "block/glass")}, 0, 0, o); break; }
        if ((m & 3) == 3) {   // planta de coro: cubo central 8^3 (4..12) + braco 8x4x8 para cada vizinho ligado (bits 2..7 = -X +X -Y +Y -Z +Z)
            static const float A[6][6] = {{0, 4, 4, 4, 12, 12}, {12, 4, 4, 16, 12, 12}, {4, 0, 4, 12, 4, 12}, {4, 12, 4, 12, 16, 12}, {4, 4, 0, 12, 12, 4}, {4, 4, 12, 12, 12, 16}};
            std::vector<std::string> cv = {rsAll(4, 4, 4, 12, 12, 12, "block/chorus_plant")};
            for (int k = 0; k < 6; k++) if ((m >> (2 + k)) & 1) cv.push_back(rsAll(A[k][0], A[k][1], A[k][2], A[k][3], A[k][4], A[k][5], "block/chorus_plant"));
            rsBake(cv, 0, 0, o); break; }
        { const char *a = "block/cauldron_side", *t = "block/cauldron_top", *i = "block/cauldron_inner", *b = "block/cauldron_bottom";
        if ((m & 3) == 1) { const int bt = std::min((m >> 2) & 7, 6); const char *cs = "block/cake_side", *ct = "block/cake_top", *cb = "block/cake_bottom", *ci = bt ? "block/cake_inner" : cs;
            rsBake({rsBx(1 + 2 * bt, 0, 1, 15, 8, 15, ci, cs, ct, cb, cs, cs)}, 0, 0, o); break; }
        { std::vector<std::string> cv = {rsBx(0, 4, 0, 16, 16, 2, a, a, t, b, a, i), rsBx(0, 4, 14, 16, 16, 16, a, a, t, b, i, a), rsBx(0, 4, 2, 2, 16, 14, a, i, t, b, 0, 0), rsBx(14, 4, 2, 16, 16, 14, i, a, t, b, 0, 0), rsBx(2, 4, 2, 14, 4, 14, 0, 0, i, 0, 0, 0),
                rsBx(0, 0, 0, 4, 4, 4, a, a, 0, b, a, a), rsBx(12, 0, 0, 16, 4, 4, a, a, 0, b, a, a), rsBx(0, 0, 12, 4, 4, 16, a, a, 0, b, a, a), rsBx(12, 0, 12, 16, 4, 16, a, a, 0, b, a, a)};
          const int lv = std::min((m >> 2) & 3, 3); if (lv > 0) cv.push_back(rsBx(2, 0, 2, 14, 6 + 3 * lv, 14, 0, 0, "block/water_still", 0, 0, 0));   // patchBucket.3: nivel da agua (1..3) = 9/12/15 px
          rsBake(cv, 0, 0, o); break; } }
    case 29: if (m & 1) {   // caminho de terra (grass_path / dirt_path): 15/16 de altura como o terra arada
            const bool nw = pkFind("block/dirt_path_top") != 0; const char* sd = nw ? "block/dirt_path_side" : "block/grass_path_side"; const char* tp = nw ? "block/dirt_path_top" : "block/grass_path_top";
            rsBake({rsBx(0, 0, 0, 16, 15, 16, sd, sd, tp, "block/dirt", sd, sd)}, 0, 0, o); break; }
        rsBake({rsBx(0, 0, 0, 16, 15, 16, "block/dirt", "block/dirt", "block/farmland", "block/dirt", "block/dirt", "block/dirt")}, 0, 0, o); break;
    case 0: {   // poeira: ponto ou linhas (NS e/ou LO); cor pela potencia (bits 4-7)
        const int cn = m & 15; std::vector<std::string> e;
        e.push_back(rsBx(0, .25f, 0, 16, .25f, 16, 0, 0, cn ? "block/redstone_dust_line0" : "block/redstone_dust_dot", 0, 0, 0));
        if (cn && ((cn & 5) == 0 || (cn & 10) == 0)) { if (cn & 5) rsBake(e, 0, 0, o); else rsBake(e, 0, 90, o); }   // so um eixo (1 ou 2 lados opostos / 1 lado): linha inteira
        else if (cn) {   // patchRed.3: curva/T/cruz = ponto no centro + um braco (meia linha) por lado ligado, como o 1.13+
            rsBake({rsBx(0, .25f, 0, 16, .25f, 16, 0, 0, "block/redstone_dust_dot", 0, 0, 0)}, 0, 0, o);
            for (int d = 0; d < 4; d++) if ((cn >> d) & 1) rsBake({rsBx(0, .25f, 0, 16, .25f, 8, 0, 0, "block/redstone_dust_line0", 0, 0, 0)}, 0, 90 * d, o);
        } else rsBake(e, 0, 0, o);
        const float p = (m >> 4) / 15.f; for (XQuad& q : o) { q.tc[0] = .3f + .7f * p; q.tc[1] = q.tc[2] = p * p * .5f; }
        break; }
    case 1: case 2: case 3: case 4: case 5: {   // pistao, pistao grudento, dispenser, dropper, observador (dir6; frente voltada para o jogador)
        const int d = m & 7; rsDirRot(d, bx, by); const bool vert = d < 2; const char *fr, *sd, *bk, *tp;
        if (k <= 2) {   // pistao (patch RPI.10): modelo com a frente em CIMA (+Y), como o do Minecraft, para a faixa da cabeca de piston_side acompanhar a frente; depois gira para a direcao d (antes a frente era o norte e a textura lateral ficava de pe => parecia andaime)
            const char* ps = "block/piston_side"; const char* pt = k == 2 ? "block/piston_top_sticky" : "block/piston_top";
            const int rx = d == 1 ? 0 : d == 0 ? 180 : 90, ry = d >= 2 ? 90 * (d - 2) : 0;
            if (m & 24) {   // estendido: bit4 = corpo (12px, frente aberta com piston_inner + haste de 4px), bit3 = cabeca (placa de 4px + haste de 12px)
                if (m & 8) rsBake({rsBx(0, 12, 0, 16, 16, 16, ps, ps, pt, pt, ps, ps), rsRod(6, 0, 6, 10, 12, 10)}, rx, ry, o);
                else rsBake({rsBx(0, 0, 0, 16, 12, 16, ps, ps, "block/piston_inner", "block/piston_bottom", ps, ps), rsRod(6, 12, 6, 10, 16, 10)}, rx, ry, o);
            } else rsBake({rsBx(0, 0, 0, 16, 16, 16, ps, ps, pt, "block/piston_bottom", ps, ps)}, rx, ry, o);   // fechado: cubo, cabeca em cima
            break; }
        if (k <= 2) { fr = k == 2 ? "block/piston_top_sticky" : "block/piston_top"; sd = tp = "block/piston_side"; bk = "block/piston_bottom"; }
        else if (k <= 4) { fr = k == 3 ? (vert ? "block/dispenser_front_vertical" : "block/dispenser_front") : (vert ? "block/dropper_front_vertical" : "block/dropper_front"); sd = bk = vert ? "block/furnace_top" : "block/furnace_side"; tp = "block/furnace_top"; }
        else { fr = "block/observer_front"; bk = (m >> 4) & 1 ? "block/observer_back_on" : "block/observer_back"; sd = "block/observer_side"; tp = "block/observer_top"; }
        rsBake({rsBx(0, 0, 0, 16, 16, 16, sd, sd, tp, tp, bk, fr)}, bx, by, o); break; }
    case 6: { static const char* T[16] = {"block/stone", "block/oak_planks", "block/gold_block", "block/iron_block", "block/spruce_planks", "block/birch_planks", "block/jungle_planks", "block/acacia_planks", "block/dark_oak_planks", "block/mangrove_planks", "block/cherry_planks", "block/bamboo_planks", "block/crimson_planks", "block/warped_planks", "block/pale_oak_planks", "block/oak_planks"};
        rsBake({rsAll(1, 0, 1, 15, (m >> 4) & 1 ? .5f : 1.f, 15, T[m & 15])}, 0, 0, o); break; }
    case 7: { const int f = m & 7, pr = (m >> 4) & 1; const char* t = (m >> 3) & 1 ? "block/oak_planks" : "block/stone";
        bx = f == 1 ? 180 : f >= 2 ? 90 : 0; by = f >= 2 ? 90 * (f - 2) : 0; rsBake({rsAll(5, 0, 6, 11, pr ? 1.f : 2.f, 10, t)}, bx, by, o); break; }
    case 8: rsBakeModel(W + ((m >> 2) & 1 ? "tripwire_hook_on" : "tripwire_hook"), 0, 90 * (m & 3), o); break;
    case 9: rsBake({rsBx(0, 0, 0, 16, 6, 16, "block/daylight_detector_side", "block/daylight_detector_side", m & 1 ? "block/daylight_detector_inverted_top" : "block/daylight_detector_top", "block/oak_planks", "block/daylight_detector_side", "block/daylight_detector_side")}, 0, 0, o); break;
    case 10: if (m == 0) rsBakeModel(W + "hopper", 0, 0, o); else rsBakeModel(W + "hopper_side", 0, ((m - 2) & 3) * 90, o);
        if (o.empty()) { const char *ou = "block/hopper_outside", *in = "block/hopper_inside";   // patchHop.2: funil embutido SO com texturas opacas (hopper_top tem buraco transparente 10x10: topo do aro agora usa hopper_outside); aro 6px + piso (inside) + funil + bico, como o hopper.json 1.12.2
            std::vector<std::string> e = {rsBx(0, 10, 0, 16, 11, 16, ou, ou, 0, ou, ou, ou),   // placa de base 16x16 (fecha o fundo da cuba e o encontro das paredes com o funil)
                rsBx(0, 11, 0, 2, 16, 16, ou, ou, ou, 0, ou, ou), rsBx(14, 11, 0, 16, 16, 16, ou, ou, ou, 0, ou, ou), rsBx(2, 11, 0, 14, 16, 2, ou, ou, ou, 0, ou, ou), rsBx(2, 11, 14, 14, 16, 16, ou, ou, ou, 0, ou, ou),
                rsBx(2, 11, 2, 14, 11, 14, 0, 0, in, 0, 0, 0), rsBx(4, 4, 4, 12, 10, 12, ou, ou, 0, ou, ou, ou)};
            e.push_back(m == 0 ? rsBx(6, 0, 6, 10, 4, 10, ou, ou, 0, ou, ou, ou) : rsBx(6, 4, 0, 10, 8, 6, ou, ou, ou, ou, ou, ou));
            rsBake(e, 0, m == 0 ? 0 : ((m - 2) & 3) * 90, o); }
        break;
    case 11: case 12: {   // base de 2px + tochas (saida no norte): repetidor = fixa + movel (atraso); comparador = 2 de tras + 1 da frente (alta no modo subtrair)
        const bool on = k == 11 ? (m >> 4) & 1 : (m >> 3) & 1; const char* t = k == 11 ? (on ? "block/repeater_on" : "block/repeater") : (on ? "block/comparator_on" : "block/comparator");
        std::vector<std::string> e = {rsBx(0, 0, 0, 16, 2, 16, "block/stone", "block/stone", t, "block/stone", "block/stone", "block/stone")};
        if (k == 11) { e.push_back(rsTorch(7, 2, 2, 5, on)); e.push_back(rsTorch(7, 2, 6 + 2 * ((m >> 2) & 3), 5, on)); if ((m >> 5) & 1) e.push_back(rsBx(2, 2, 7, 14, 4, 9, "block/bedrock", "block/bedrock", "block/bedrock", "block/bedrock", "block/bedrock", "block/bedrock")); }
        else { const bool sub = (m >> 2) & 1; e.push_back(rsTorch(3, 2, 11, 3, on)); e.push_back(rsTorch(11, 2, 11, 3, on)); e.push_back(rsTorch(7, 2, 2, sub ? 5 : 3, sub)); }
        rsBake(e, 0, 90 * (m & 3), o); break; }
    case 13: { const int ty = (m >> 4) & 7, op = (m >> 3) & 1, tp = (m >> 2) & 1;
        // caixa solida de 3/16 (como o Minecraft); o modelo do pack e uma moldura vazada com buracos e parecia quebrado
        const std::string tx = ty == 0 ? W + "iron_trapdoor" : W + RS_WOOD[ty] + "_trapdoor"; const char* t = tx.c_str();
        const float e = .02f, f = 16.f - .02f;   // 0,02 de folga nas faces que encostam no vizinho/chao/teto: com textura transparente elas brigavam (z-fighting) e piscavam
        rsBake({op ? rsBx(e, e, 13, f, f, f, t, t, t, t, t, t) : tp ? rsBx(e, 13, e, f, f, f, t, t, t, t, t, t) : rsBx(e, e, e, f, 3, f, t, t, t, t, t, t)}, 0, op ? 90 * (m & 3) : 0, o); break; }
    case 14: { const int ty = (m >> 3) & 7, op = (m >> 2) & 1;
        rsBakeModel(W + RS_WOOD[1 + std::min(ty, 5)] + "_fence_gate" + (op ? "_open" : ""), 0, ((m + 2) & 3) * 90, o);
        if (o.empty()) { const std::string tx = W + RS_WOOD[1 + std::min(ty, 5)] + "_planks"; const char* t = tx.c_str();   // patchCrop.0: o pack nao tem models/: portao embutido (2 postes + 2 travessas + miolo; aberto = folhas dobradas nos postes)
            if (op) rsBake({rsAll(0, 5, 7, 2, 16, 9, t), rsAll(14, 5, 7, 16, 16, 9, t), rsAll(0, 6, 9, 2, 9, 13, t), rsAll(0, 12, 9, 2, 15, 13, t), rsAll(14, 6, 9, 16, 9, 13, t), rsAll(14, 12, 9, 16, 15, 13, t)}, 0, ((m + 2) & 3) * 90, o);
            else rsBake({rsAll(0, 5, 7, 2, 16, 9, t), rsAll(14, 5, 7, 16, 16, 9, t), rsAll(2, 6, 7, 14, 9, 9, t), rsAll(2, 12, 7, 14, 15, 9, t), rsAll(6, 9, 7, 10, 12, 9, t)}, 0, ((m + 2) & 3) * 90, o); }
        break; }
    case 15: { const int f = m & 3, op = (m >> 2) & 1, up = (m >> 3) & 1, hi = (m >> 4) & 1, ty = (m >> 5) & 7;
        int kk = (f + 1) & 3; if (op) kk = (kk + (hi ? 3 : 1)) & 3;   // lamina em O/N/L/S = 0..3; aberta gira 90 em torno da dobradica
        { const size_t n0 = o.size(); rsBakeModel(W + RS_WOOD[ty] + "_door_" + (up ? "top" : "bottom") + (hi ? "_hinge" : ""), 0, 90 * kk, o);
          if (o.size() == n0) { const std::string t = W + RS_WOOD[ty] + "_door_" + (up ? "top" : "bottom"); char jb[900]; snprintf(jb, sizeof jb, "{\"from\":[0,0,0],\"to\":[3,16,16],\"faces\":{\"west\":{\"uv\":[%d,0,%d,16],\"texture\":\"%s\"},\"east\":{\"uv\":[%d,0,%d,16],\"texture\":\"%s\"},\"north\":{\"uv\":[3,0,0,16],\"texture\":\"%s\"},\"south\":{\"uv\":[0,0,3,16],\"texture\":\"%s\"},\"up\":{\"uv\":[13,0,16,16],\"texture\":\"%s\"},\"down\":{\"uv\":[13,0,16,16],\"texture\":\"%s\"}}}", hi ? 16 : 0, hi ? 0 : 16, t.c_str(), hi ? 0 : 16, hi ? 16 : 0, t.c_str(), t.c_str(), t.c_str(), t.c_str(), t.c_str()); rsBake({std::string(jb)}, 0, 90 * kk, o); } }   // patchV.2: pack sem models/ (1.12) -> lamina vanilla 3px embutida (antes sumia/vinha so o contorno)
        break; }
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
        if (!(m & 15)) {   // patchPane.2: sem conexao = cruz de 2 placas COM espessura (2 px, como o modelo noside do 1.12). Antes: placas de espessura 0 => faces frente/tras coplanares piscavam (textura "repetida") e ficava 2D
            const std::string B = "{\"from\":[7,0,0],\"to\":[9,16,16],\"faces\":{\"east\":{\"texture\":\"" + sd + "\"},\"west\":{\"texture\":\"" + sd + "\"},\"north\":{\"uv\":[7,0,9,16],\"texture\":\"" + tp + "\"},\"south\":{\"uv\":[7,0,9,16],\"texture\":\"" + tp + "\"},\"up\":{\"uv\":[7,0,9,16],\"texture\":\"" + tp + "\"}}}";
            rsBake({B}, 0, 0, o); rsBake({B}, 0, 90, o);
            break; }
        // patchPane.2: o poste so tem as faces dos lados SEM braco (as de dentro dos bracos apareciam pela transparencia e "repetiam" a textura); bits: 1 N, 2 L, 4 S, 8 O
        rsBake({rsBx(7, 0, 7, 9, 15.99f, 9, (cn & 2) ? 0 : sd.c_str(), (cn & 8) ? 0 : sd.c_str(), tp.c_str(), 0, (cn & 4) ? 0 : sd.c_str(), (cn & 1) ? 0 : sd.c_str())}, 0, 0, o);
        for (int d = 0; d < 4; d++) if ((cn >> d) & 1) rsBake({rsBx(7, 0, .02f, 9, 15.99f, 7, sd.c_str(), sd.c_str(), tp.c_str(), 0, 0, (m & 15) ? 0 : tp.c_str())}, 0, 90 * d, o);
        break; }
    case 23: { const int f = m & 7;   // patchBox.1: o pack 1.12 nao traz models/ => a haste sumia; modelo vanilla embutido (base 4x1x4 + haste 2x15x2)
        const std::string T = "\"texture\":\"block/end_rod\"";
        const std::string base = "{\"from\":[6,0,6],\"to\":[10,1,10],\"faces\":{\"north\":{\"uv\":[0,0,4,1]," + T + "},\"south\":{\"uv\":[0,0,4,1]," + T + "},\"east\":{\"uv\":[0,0,4,1]," + T + "},\"west\":{\"uv\":[0,0,4,1]," + T + "},\"up\":{\"uv\":[0,2,4,6]," + T + "},\"down\":{\"uv\":[0,2,4,6]," + T + "}}}";
        const std::string rod = "{\"from\":[7,1,7],\"to\":[9,16,9],\"faces\":{\"north\":{\"uv\":[0,0,2,15]," + T + "},\"south\":{\"uv\":[0,0,2,15]," + T + "},\"east\":{\"uv\":[0,0,2,15]," + T + "},\"west\":{\"uv\":[0,0,2,15]," + T + "},\"up\":{\"uv\":[0,2,2,4]," + T + "}}}";
        rsBake({base, rod}, f == 1 ? 180 : f >= 2 ? 90 : 0, f >= 2 ? 90 * (f - 2) : 0, o); break; }   // haste do End: modelo do pack
    case 24: if (entBed(m, o)) break;   // modelo real (entity/bed/<cor>.png); sem a textura, reserva de la
        { const std::string t = W + RS_COLOR[(m >> 3) & 15] + "_wool"; const char* pl = "block/oak_planks"; std::vector<std::string> e;   // cama: colchao + pes (cabeca para o norte)
        if (m & 128) { e = {rsBx(0, 3, -16, 16, 9, 16, t.c_str(), t.c_str(), t.c_str(), pl, t.c_str(), t.c_str()), rsNd(0, 0, -16, 3, 3, -13, pl), rsNd(13, 0, -16, 16, 3, -13, pl), rsNd(0, 0, 13, 3, 3, 16, pl), rsNd(13, 0, 13, 16, 3, 16, pl)}; rsBake(e, 0, 0, o); break; }   // icone: as duas metades
        const bool hd = (m >> 2) & 1; e.push_back(rsBx(0, 3, 0, 16, 9, 16, t.c_str(), t.c_str(), t.c_str(), pl, t.c_str(), t.c_str()));
        e.push_back(rsNd(hd ? 0 : 0, 0, hd ? 0 : 13, 3, 3, hd ? 3 : 16, pl)); e.push_back(rsNd(13, 0, hd ? 0 : 13, 16, 3, hd ? 3 : 16, pl));
        rsBake(e, 0, 90 * (m & 3), o); break; }
    case 25: { int wd, d; bool wl; signDec(m, wd, wl, d); if (entSign(wd, wl, d, o)) break;   // icone/mao (m >= 240): de pe, frente voltada para a camera
        const std::string t = W + (wd == 0 ? "oak" : RS_WOOD[1 + std::min(wd - 1, 5)]) + "_planks";
        rsBake({rsAll(0, 9.333f, 7.333f, 16, 17.333f, 8.667f, t.c_str()), rsAll(7.333f, 0, 7.333f, 8.667f, 9.333f, 8.667f, t.c_str())}, 0, 90 * (wl ? d & 3 : (((d + 2) >> 2) + 2) & 3), o); break; }
    case 30: { const size_t n0 = o.size(); rsBake({rsBx(0, .2f, 0, 16, .2f, 16, 0, 0, "block/lily_pad", "block/lily_pad", 0, 0)}, 0, 0, o); for (size_t i = n0; i < o.size(); i++) { o[i].tc[0] = .125f; o[i].tc[1] = .5f; o[i].tc[2] = .1875f; } break; }   // cor fixa 0x208030 como o MC   // vitoria-regia: placa horizontal
    case 31: { const size_t n0 = o.size(); const char* T = "block/vine"; for (int k = 0; k < 4; k++) if ((m >> k) & 1) rsBake({rsBx(0, 0, 15.2f, 16, 16, 15.2f, 0, 0, 0, 0, T, T)}, 0, 90 * k, o); if (m & 16) rsBake({rsBx(0, 15.2f, 0, 16, 15.2f, 16, 0, 0, T, T, 0, 0)}, 0, 0, o); for (size_t i = n0; i < o.size(); i++) { o[i].tint = true; o[i].foli = true; } break; }   // vinha: uma placa por lado preso
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
static int rsStairShape(int x, int y, int z, int m);   // definida mais abaixo (precisa de rsXf)
static void rsStairBake(int shape, int m, std::vector<XQuad>& out) {   // cantos: so as partes altas mudam; faces internas omitidas
    std::string sd, tp, bt; rsMat((m >> 3) & 15, sd, tp, bt); const bool t = (m >> 2) & 1; const char *a = sd.c_str(), *u = tp.c_str(), *d = bt.c_str();
    const float ya = t ? 0 : 8, yb = t ? 8 : 16; const char *U = t ? nullptr : u, *D = t ? d : nullptr;   // topo/base da parte alta (so a face exposta)
    std::vector<std::string> e; e.push_back(rsBx(0, t ? 8 : 0, 0, 16, t ? 16 : 8, 16, a, a, u, d, a, a));
    switch (shape) {
    case 1: e.push_back(rsBx(0, ya, 0, 8, yb, 8, a, a, U, D, a, a)); break;
    case 2: e.push_back(rsBx(8, ya, 0, 16, yb, 8, a, a, U, D, a, a)); break;
    case 3: e.push_back(rsBx(0, ya, 0, 8, yb, 16, a, a, U, D, a, a)); e.push_back(rsBx(8, ya, 0, 16, yb, 8, a, nullptr, U, D, a, a)); break;
    default: e.push_back(rsBx(8, ya, 0, 16, yb, 16, a, a, U, D, a, a)); e.push_back(rsBx(0, ya, 0, 8, yb, 8, nullptr, a, U, D, a, a)); break;
    }
    rsBake(e, 0, 90 * (m & 3), out);
}
// malha da escada na celula (x,y,z) (mundo): reta = a de sempre; canto = variante em cache
static const std::vector<XQuad>& rsStairQuadsAt(int x, int y, int z, int meta) {
    const int m = meta & 127, sh = rsStairShape(x, y, z, m); if (!sh) return rsQuads(XB_STAIR, meta);
    static std::vector<XQuad> Q[5][128]; static bool B[5][128]; static size_t seen = (size_t)-1; const size_t ep = PK.size() * 64 + (size_t)gEntN;
    if (seen != ep) { seen = ep; memset(B, 0, sizeof B); }
    if (!B[sh][m]) { B[sh][m] = true; Q[sh][m].clear(); rsStairBake(sh, m, Q[sh][m]); }
    return Q[sh][m];
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
static int rsStairBackDir(int m) {   // lado (0=N 1=L 2=S 3=O) da face alta da escada, calculado com a mesma rotacao da colisao (case 26)
    const AABB t = rsXf(AABB{0, 0, 0, 1, 1, .5}, 0, 90 * (m & 3));
    return t.z1 <= .501 ? 0 : t.z0 >= .499 ? 2 : t.x0 >= .499 ? 1 : 3;
}
// Canto da escada = BlockStairs.getStairShape do 1.12, calculado pelos vizinhos (nao e salvo): 0 reta, 1 externo-esq, 2 externo-dir, 3 interno-esq, 4 interno-dir.
// "esq" = anti-horario visto de cima a partir da frente (face alta). Vizinhos: so escadas na mesma metade (baixo/cima).
static int rsStairShape(int x, int y, int z, int m) {
    static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0};
    const int f = rsStairBackDir(m), half = (m >> 2) & 1, ccw = (f + 3) & 3;
    auto stairAt = [&](int d, int& nf) { const int nx = x + DX[d], nz = z + DZ[d]; if (getB(nx, y, nz) != XB_STAIR) return false; const int nm = getM(nx, y, nz); if (((nm >> 2) & 1) != half) return false; nf = rsStairBackDir(nm); return true; };
    auto differs = [&](int d) {   // isDifferentStairs: o bloco em d nao e escada com a mesma frente e metade
        const int nx = x + DX[d], nz = z + DZ[d]; if (getB(nx, y, nz) != XB_STAIR) return true; const int nm = getM(nx, y, nz); return rsStairBackDir(nm) != f || ((nm >> 2) & 1) != half; };
    int nf;
    if (stairAt(f, nf) && (nf & 1) != (f & 1) && differs((nf + 2) & 3)) return nf == ccw ? 1 : 2;
    if (stairAt((f + 2) & 3, nf) && (nf & 1) != (f & 1) && differs(nf)) return nf == ccw ? 3 : 4;
    return 0;
}
static int rsStairBoxes(int shape, int m, AABB* o) {   // caixas LOCAIS (colisao e selecao), ja rotacionadas
    const bool t = (m >> 2) & 1; const double y0 = t ? .5 : 0, y1 = t ? 1 : .5, a = t ? 0 : .5, b = t ? .5 : 1; const int ay = 90 * (m & 3); int n = 0;
    o[n++] = rsXf(AABB{0, y0, 0, 1, y1, 1}, 0, ay);
    switch (shape) {   // canonico: frente (lado alto) ao norte, "esquerda" = oeste (x baixo)
    case 1: o[n++] = rsXf(AABB{0, a, 0, .5, b, .5}, 0, ay); break;
    case 2: o[n++] = rsXf(AABB{.5, a, 0, 1, b, .5}, 0, ay); break;
    case 3: o[n++] = rsXf(AABB{0, a, 0, 1, b, .5}, 0, ay); o[n++] = rsXf(AABB{0, a, .5, .5, b, 1}, 0, ay); break;
    case 4: o[n++] = rsXf(AABB{0, a, 0, 1, b, .5}, 0, ay); o[n++] = rsXf(AABB{.5, a, .5, 1, b, 1}, 0, ay); break;
    default: o[n++] = rsXf(AABB{0, a, 0, 1, b, .5}, 0, ay); }
    return n;
}
static AABB rsSlab(int side) {   // lamina de 3/16 no lado (0 N, 1 L, 2 S, 3 O)
    static const AABB S[4] = {{0, 0, 0, 1, 1, .1875}, {.8125, 0, 0, 1, 1, 1}, {0, 0, .8125, 1, 1, 1}, {0, 0, 0, .1875, 1, 1}}; return S[side & 3]; }
static int rsShape(int x, int y, int z, int id, int m, bool sel, AABB* o) {
    switch (id - XB_DUST) {
    case 1: case 2: {   // pistao: cubo; estendido (bit4) corpo de 12px; cabeca (bit3) placa de 4px + haste de 4x4 (frente = lado d)
        if (!(m & 24)) { o[0] = AABB{0, 0, 0, 1, 1, 1}; return 1; }
        int bx, by; rsDirRot(m & 7, bx, by);
        if (m & 8) { o[0] = rsXf(AABB{0, 0, 0, 1, 1, .25f}, bx, by); o[1] = rsXf(AABB{.375f, .375f, .25f, .625f, .625f, 1}, bx, by); return 2; }
        o[0] = rsXf(AABB{0, 0, .25f, 1, 1, 1}, bx, by); return 1; }
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
    case 26: return rsStairBoxes(rsStairShape(x, y, z, m), m, o);
    case 27: { const int h = m & 3; o[0] = AABB{0, h == 1 ? .5 : 0, 0, 1, h == 0 ? .5 : 1, 1}; return 1; }
    case 28: o[0] = AABB{0, 0, 0, 1, .25, 1}; return 1;
    case 30: o[0] = AABB{.0625, 0, .0625, .9375, .09375, .9375}; return 1;
    case 31: { int n = 0; const double t = .0625; if (m & 1) o[n++] = AABB{0, 0, 1 - t, 1, 1, 1}; if (m & 2) o[n++] = AABB{0, 0, 0, t, 1, 1}; if (m & 4) o[n++] = AABB{0, 0, 0, 1, 1, t}; if (m & 8) o[n++] = AABB{1 - t, 0, 0, 1, 1, 1}; if (m & 16) o[n++] = AABB{0, 1 - t, 0, 1, 1, 1}; if (!n) o[n++] = AABB{0, 0, 0, 1, 1, 1}; return n; }
    case 29: o[0] = AABB{0, 0, 0, 1, .9375, 1}; return 1;
    case 32: if ((m & 3) == 2) { o[0] = AABB{0, 0, 0, 1, 1, 1}; return 1; }
        if ((m & 3) == 3) { static const double A[6][6] = {{0, .25, .25, .25, .75, .75}, {.75, .25, .25, 1, .75, .75}, {.25, 0, .25, .75, .25, .75}, {.25, .75, .25, .75, 1, .75}, {.25, .25, 0, .75, .75, .25}, {.25, .25, .75, .75, .75, 1}};
            int n = 0; o[n++] = AABB{.25, .25, .25, .75, .75, .75}; for (int k = 0; k < 6; k++) if ((m >> (2 + k)) & 1) o[n++] = AABB{A[k][0], A[k][1], A[k][2], A[k][3], A[k][4], A[k][5]}; return n; }
        if ((m & 3) == 1) { o[0] = AABB{(1 + 2 * std::min((m >> 2) & 7, 6)) / 16.0, 0, .0625, .9375, .5, .9375}; return 1; }
        if (sel) { o[0] = AABB{0, 0, 0, 1, 1, 1}; return 1; }   // selecao: cubo; colisao: piso + 4 paredes (o jogador cabe dentro? nao: igual ao Java, paredes de 2px)
        o[0] = AABB{0, 0, 0, 1, .3125, 1}; o[1] = AABB{0, 0, 0, 1, 1, .125}; o[2] = AABB{0, 0, .875, 1, 1, 1}; o[3] = AABB{0, 0, 0, .125, 1, 1}; o[4] = AABB{.875, 0, 0, 1, 1, 1}; return 5;
    case 23: { const int f = m & 7; o[0] = rsXf(AABB{.375, 0, .375, .625, 1, .625}, f == 1 ? 180 : f >= 2 ? 90 : 0, f >= 2 ? 90 * (f - 2) : 0); return 1; }
    case 24: o[0] = AABB{0, 0, 0, 1, .5625, 1}; return 1;
    case 25: { int wd, d; bool wl; signDec(m, wd, wl, d); if (wl) { o[0] = rsXf(AABB{0, .28125, 0, 1, .78125, .125}, 0, 90 * ((d + 2) & 3)); return 1; } } o[0] = AABB{.25, 0, .25, .75, 1, .75}; return 1;
    case 18: case 19: case 21: case 22: { const bool fe = id != XB_PANE && id != XB_CPANE; const double h = fe && !sel ? 1.5 : 1, a = id == XB_FENCE ? .375 : fe ? .3125 : .4375, b = 1 - a; int cn = m & 15; if (!fe && !cn) cn = 15; int n = 0;
        { const bool up = id == XB_WALL && (((cn != 5 && cn != 10)) || (m & 128)); const double pa = up ? .25 : a; o[n++] = AABB{pa, 0, pa, 1 - pa, h, 1 - pa}; }
        if (cn & 1) o[n++] = AABB{a, 0, 0, b, h, a}; if (cn & 2) o[n++] = AABB{b, 0, a, 1, h, b}; if (cn & 4) o[n++] = AABB{a, 0, b, b, h, 1}; if (cn & 8) o[n++] = AABB{0, 0, a, a, h, b};
        if (sel && n > 1) { AABB u = o[0]; for (int i = 1; i < n; i++) { u.x0 = std::min(u.x0, o[i].x0); u.y0 = std::min(u.y0, o[i].y0); u.z0 = std::min(u.z0, o[i].z0); u.x1 = std::max(u.x1, o[i].x1); u.y1 = std::max(u.y1, o[i].y1); u.z1 = std::max(u.z1, o[i].z1); } o[0] = u; n = 1; }   // contorno = UMA caixa (uniao), como o getBoundingBox do MC 1.12 (antes: poste + bracos = 3 caixas)
        return n; }
    }
    return 0;
}
static const struct RsShapeInit {
    RsShapeInit() {
        const int s = shapeAddFn(rsShape);
        for (int id = XB_DUST; id < XB_END; id++) {
            const int k = id - XB_DUST;
            if (k >= 3 && k <= 5) { blockShapeSet(id, SHAPE_CUBE, SHAPE_CUBE); continue; }   // pistao, dispenser, dropper, observador: cubo
            const bool coll = k == 1 || k == 2 || k == 22 || k == 23 || k == 24 || k == 17 || k == 18 || k == 19 || k == 20 || k == 21 || k == 9 || k == 10 || k == 11 || k == 12 || k == 13 || k == 14 || k == 15 || k == 16 || k == 26 || k == 27 || k == 29 || k == 30 || k == 32;   // vitoria-regia colide (pisa-se nela); a vinha nao
            blockShapeSet(id, coll ? s : SHAPE_NONE, s);
        }
    }
} rsShapeInit;

// ---- mundo: suporte, conexoes da poeira, dureza ----
static const int RS_DX[4] = {0, 1, 0, -1}, RS_DZ[4] = {-1, 0, 1, 0};   // N, L, S, O
static bool rsConnects(int nb) {
    return nb == XB_DUST || (nb >= XB_RTORCH && nb < XB_RTORCH + 5) || xbIsLever(nb) || nb == XB_PLATE || nb == XB_BTN || nb == XB_REPT || nb == XB_COMP || nb == XB_OBS || nb == XB_HOOK || (gRsRB && nb == gRsRB);
}
static bool rsLink(int nb, int nx, int ny, int nz, int d) {   // patchRed.3: como o canConnectTo do 1.12.2: repetidor so nos eixos de entrada/saida; observador so na saida (costas)
    if (nb == XB_REPT) return (getM(nx, ny, nz) & 1) == (d & 1);
    if (nb == XB_OBS) { static const int OX[6] = {0, 0, 0, 1, 0, -1}, OY[6] = {-1, 1, 0, 0, 0, 0}, OZ[6] = {0, 0, -1, 0, 1, 0}; const int od = getM(nx, ny, nz) & 7; if (od > 5) return false; return OX[od] == RS_DX[d] && OZ[od] == RS_DZ[d] && OY[od] == 0; }   // a saida (costas) do observador aponta para a poeira
    return rsConnects(nb);
}
static int rsDustConn(int x, int y, int z) {
    int m = 0;
    for (int d = 0; d < 4; d++) {
        const int nx = x + RS_DX[d], nz = z + RS_DZ[d], nb = getB(nx, y, nz);
        if (rsLink(nb, nx, y, nz, d) || (nb == AIR && getB(nx, y - 1, nz) == XB_DUST) || (!solid(getB(x, y + 1, z)) && getB(nx, y + 1, nz) == XB_DUST)) m |= 1 << d;
    }
    return m;
}
static void rsDustRecalc(int x, int y, int z); static bool rsRepLocked(int x, int y, int z); static void rsCompCheck(int x, int y, int z); static void rsCompTick(int x, int y, int z); static int chestSignal(int x, int y, int z); static void rsRepLockSync(int x, int y, int z);
static bool rsObsOut(int ox, int oy, int oz, int tx, int ty, int tz); static void rsObsTick(int x, int y, int z);
static void rsDustFix(int x, int y, int z) {
    const int old = getM(x, y, z), nm = (old & 0xF0) | rsDustConn(x, y, z);
    if (nm != old) { setM(x, y, z, nm); netSendBlock(x, y, z, XB_DUST, nm); }
    rsDustRecalc(x, y, z);   // vizinho mudou (fonte, poeira nova/quebrada): recalcula a rede
}
static bool rsSupported(int b, int x, int y, int z) {
    const int m = getM(x, y, z), below = getB(x, y - 1, z);
    switch (b - XB_DUST) {
    case 0: case 6: case 11: case 12: return solid(below);
    case 7: { const int f = m & 7; if (f == 0) return solid(below); if (f == 1) return solid(getB(x, y + 1, z)); const int d = (f - 2) & 3; return solid(getB(x - RS_DX[d], y, z - RS_DZ[d])); }   // patchCrash.1: meta de mundo antigo (f 6/7) estourava RS_DX
    case 8: case 20: { const int d = m & 3; return solid(getB(x - RS_DX[d], y, z - RS_DZ[d])); }
    case 17: return below != AIR;
    case 28: { static const int ss = pkFind("block/soul_sand"); return ((m >> 3) & 7) == 4 ? (ss && below == ss) : below == XB_FARM; }
    case 32: return (m & 3) != 1 || solid(below);
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
static int rsDropItemM(int b, int m) {
    if (b == XB_DOOR && ((m >> 3) & 1)) return 0;
    if ((b == XB_PIST || b == XB_PIST + 1) && (m & 8)) return 0;   // a cabeca do pistao nao dropa (o corpo dropa)
    if (b == XB_FARM) return DIRT; if (b == XB_DECOR && (m & 3) == 1) return 0; if (b == XB_CROP) return ((m >> 3) & 7) == 1 ? IT_CARROT : 0;
    if (b == XB_BED && ((m >> 2) & 1)) return 0;   // so os pes da cama dropam   // metade de cima da porta nao dropa
    int v = rsVariant(b, m); if (b == XB_CHEST && v >= 19 && v <= 22) v = (v - 19) >> 1;   // metade de bau duplo dropa o bau normal/armadilhado
    const int it = gRsDrop[b - XB_DUST][v & 31];
    if (it) return it;
    for (int i = 0; i < 32; i++) if (gRsDrop[b - XB_DUST][i]) return gRsDrop[b - XB_DUST][i];
    return 0;
}
static int rsDropItem(int x, int y, int z, int b) { return rsDropItemM(b, getB(x, y, z) == b ? getM(x, y, z) : gRsLastMeta); }   // o bloco ja saiu: o setBM guardou o meta
// patch5.70: FACILITADOR do pick block (P1, P2, qualquer controle): item do bloco mirado em (x,y,z), ou 0 se nao ha (ar, liquido, fogo, portal).
// Metade de cima da porta, cabeca do pistao e cabeca da cama NAO dropam, mas o pick devolve o item inteiro (como o Java).
static int pickItemAt(int x, int y, int z) {
    const int b = getB(x, y, z); if (b == AIR || isLiquid(b)) return 0;
    if (!isXB(b)) return b;
    if (b == XB_FIRE || b == XB_PORTAL) return 0;
    const int m = getM(x, y, z);
    if (b == XB_CROP) {   // patch5.74: pick de plantacao devolve o item do TIPO (trigo/cenoura/batata/beterraba), em qualquer idade (no Java: semente/cenoura/batata)
        const int ty = (m >> 3) & 7, vv = rsVariant(b, m) & 31; int it = gRsDrop[b - XB_DUST][vv];
        if (!it) for (int i = 0; i < 32 && !it; i++) if (((i >> 0) & 31) == vv) it = gRsDrop[b - XB_DUST][i];
        if (!it && ty == 1) it = IT_CARROT;
        if (!it && ty == 4) it = pkFind("item/nether_wart");
        return it;
    }
    if (isRs(b)) {
        int pm = m;
        if (b == XB_DOOR) pm &= ~8; else if (b == XB_PIST || b == XB_PIST + 1) pm &= ~8; else if (b == XB_BED) pm &= ~4;
        const int it = rsDropItemM(b, pm); if (it) return it;
    }
    return xbDropItem(b, m);
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
// 1.12: cerca/muro/painel ligam em qualquer face SOLID; a face de tras de uma escada (reta) e SOLID. d = direcao da cerca ate o vizinho
static bool rsStairSolidToward(int nb, int nm, int d) { return nb == XB_STAIR && rsStairBackDir(nm) == ((d + 2) & 3); }
static int rsConnMask(int b, int var, int x, int y, int z) {
    int m = 0;
    for (int d = 0; d < 4; d++) {
        const int nx = x + RS_DX[d], nz = z + RS_DZ[d], nb = getB(nx, y, nz), nm = nb == AIR ? 0 : getM(nx, y, nz); bool c = false;
        if (b == XB_FENCE) { if (nb == XB_FENCE) c = (((nm >> 4) & 7) == 6) == (var == 6); else if (nb == XB_GATE) c = var != 6 && ((d & 1) ? !(nm & 1) : (nm & 1) != 0); else c = rsFullCube(nb) || rsStairSolidToward(nb, nm, d); }
        else if (b == XB_WALL) c = rsStairSolidToward(nb, nm, d) || nb == XB_WALL || (nb == XB_GATE && ((d & 1) ? !(nm & 1) : (nm & 1) != 0)) || rsFullCube(nb);
        else c = nb == XB_PANE || nb == XB_CPANE || rsFullCube(nb) || rsStairSolidToward(nb, nm, d);
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
    if (xb == XB_VINE && getB(t[0], t[1], t[2]) == XB_VINE) {   // vinha em cima de vinha: soma o lado
        int bit; const int nd0 = n[2] == -1 ? 0 : n[0] == 1 ? 1 : n[2] == 1 ? 2 : 3;
        if (n[1] == 0 && solid(getB(hh[0], hh[1], hh[2]))) bit = 1 << nd0; else if (n[1] == -1 && solid(getB(hh[0], hh[1], hh[2]))) bit = 16; else return false;
        const int nm = getM(t[0], t[1], t[2]) | bit; if (nm == getM(t[0], t[1], t[2])) return false; setBM(t[0], t[1], t[2], XB_VINE, nm); netSendBlock(t[0], t[1], t[2], XB_VINE, nm); return true; }
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
    case 29: meta = var & 1; break;
    case 32: if ((var & 3) == 1 && !solid(below)) return false; meta = var & 3;
        if ((var & 3) == 3) {   // planta de coro: liga com plantas vizinhas (e com a pedra do End embaixo); as vizinhas ganham o braco para ca
            static const int D6[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}; static const int ES = pkFind("block/end_stone");
            for (int k = 0; k < 6; k++) { const int nx = t[0] + D6[k][0], ny = t[1] + D6[k][1], nz = t[2] + D6[k][2], nb = getB(nx, ny, nz);
                if (nb == XB_DECOR && (getM(nx, ny, nz) & 3) == 3) { meta |= 1 << (2 + k); const int om = getM(nx, ny, nz) | (1 << (2 + (k ^ 1))); setBM(nx, ny, nz, XB_DECOR, om); netSendBlock(nx, ny, nz, XB_DECOR, om); }
                else if (k == 2 && ES && nb == ES) meta |= 1 << 4; } }
        break;   // bolo precisa de chao; caldeirao nao   // caminho de terra (variante 1); a terra arada (0) so nasce da enxada/aldeia
    case 30: if (getB(t[0], t[1] - 1, t[2]) != WATER) return false; break;   // so sobre fonte de agua (o clique normal usa lilyUse)
    case 31: {   // vinha: lado de um bloco solido (bit = nd, como a escada) ou a face de baixo de um bloco (bit 4); junta com a vinha que ja esta na celula
        int bit; if (n[1] == 0 && solid(getB(hh[0], hh[1], hh[2]))) bit = 1 << nd; else if (n[1] == -1 && solid(getB(hh[0], hh[1], hh[2]))) bit = 16; else return false;
        meta = bit; break; }
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
    if (id == XB_CHEST) { chestAfterPlace(t[0], t[1], t[2], meta); if (!gPlaceNbt.empty()) chestApplyNbt(t[0], t[1], t[2], gPlaceNbt); }   // a parceira assume a frente do bau novo; item com conteudo recria o conteudo
    if (id == XB_PIST || id == XB_PIST + 1) pistonCheck(t[0], t[1], t[2]);
    if (id == XB_DUST) rsDustRecalc(t[0], t[1], t[2]);
    if (id == XB_REPT || id == XB_COMP) rsRepCheck(t[0], t[1], t[2]);
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
    else if (b == XB_DOOR && ((m >> 5) & 7) != 0) {   // abre/fecha as duas metades SEM disparar ganchos: antes cada metade avisava a vizinha com a outra ainda no estado velho, e a checagem de suporte derrubava a porta
        nm = m ^ 4; const int oy = ((m >> 3) & 1) ? y - 1 : y + 1; const bool old = gNoBlockHooks; gNoBlockHooks = true;
        if (getB(x, oy, z) == b) put(x, oy, z, b, getM(x, oy, z) ^ 4);
        put(x, y, z, b, nm); gNoBlockHooks = old; sndEvtAt((nm & 4) ? "door.open" : "door.close", x + .5, y + .5, z + .5); return true; }   // patchAud.3
    else if (b == XB_DECOR && (m & 3) == 1) {   // comer o bolo: +2 de fome por pedaco (sobrevivencia, so com fome; criativo sempre), 7 pedacos
        if (creative ? false : (gDead || gFood >= 20)) return true;
        if (!creative) { gFood = std::min(20, gFood + 2); gSat = std::min((float)gFood, gSat + .4f); }
        const int bt = (m >> 2) & 7; if (bt >= 6) { put(x, y, z, AIR, 0); } else put(x, y, z, b, (m & 3) | ((bt + 1) << 2)); return true; }
    else if (b == XB_BTN) { if ((m >> 4) & 1) return true; nm = m | 16; scheduleTick(x, y, z, ((m >> 3) & 1) ? 30 : 20); }   // botao: aperta e solta sozinho (pedra 20 ticks, madeira 30)
    else return false;
    put(x, y, z, b, nm); if (b == XB_BTN) pistonScan(x, y, z);
    if (b == XB_TRAP) sndEvtAt((nm & 8) ? "door.open" : "door.close", x + .5, y + .5, z + .5); else if (b == XB_GATE) sndEvtAt((nm & 4) ? "door.open" : "door.close", x + .5, y + .5, z + .5);   // patchAud.3
    else if (b == XB_BTN) sndEvtAt(((m >> 3) & 1) ? "button.click.wood" : "button.click", x + .5, y + .5, z + .5); else sndEvtAt("lever.click", x + .5, y + .5, z + .5);
    return true;
}

// ---- icone 3D no inventario (placa, botao, portao, alcapao, sensor, bau): usa a propria malha do bloco ----
static const std::vector<XQuad>* rsIconQuads(int id) {
    auto g = gXbItem.find(id); if (g != gXbItem.end() && g->second == XB_BED && entSprite(0, rsItemVar(id)) >= 0) return nullptr;   // patchV.14: painel de vidro/barras/painel colorido = sprite plano da textura no inventario (como no 1.12.2), nao o modelo em cruz
    if (g != gXbItem.end() && (g->second == XB_PANE || g->second == XB_CPANE)) return nullptr;   // cama no inventario/mao/chao = sprite 16x16 (item plano), nao o modelo 3D
    if (g != gXbItem.end() && g->second == XB_ANVIL) { auto gv = gXbVar.find(id); return &gAnvQ[std::min(gv != gXbVar.end() ? gv->second : 0, 2)][0]; }   // bigorna (normal/rachada/quebrada): modelo real
    if (g != gXbItem.end() && g->second == XB_SIGN && isPk(id) && PK[id - PK0].tile >= 0 && PK[id - PK0].file.compare(0, 5, "item/") == 0) return nullptr;   // patchInv.5: placa no inventario/mao/chao = sprite plano do pack (como no 1.12.2), nao o modelo 3D
    if (g == gXbItem.end() || !isRs(g->second)) return nullptr; const int b = g->second, v = rsItemVar(id); int m;
    if (b == XB_PLATE) m = v; else if (b == XB_BTN || b == XB_GATE) m = v << 3; else if (b == XB_TRAP) m = v << 4; else if (b == XB_DAY) m = 0; else if (b == XB_CHEST) m = (v << 2) | 2; else if (b == XB_CARPET) m = v; else if (b == XB_FENCE) m = (v << 4) | 10; else if (b == XB_WALL) m = (v << 4) | 10 | 128; else if (b == XB_ENDROD) m = 0; else if (b == XB_BED) m = (v << 3) | 128; else if (b == XB_SIGN) m = 240 + v; else if (b == XB_STAIR) m = v << 3; else if (b == XB_SLAB) m = v << 2; else if (b == XB_FARM) m = v & 1; else if (b == XB_DECOR) m = v & 3; else return nullptr;
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
        {"Nether Brick Stairs", XB_STAIR, 11}, {"Quartz Stairs", XB_STAIR, 12}, {"Red Sandstone Stairs", XB_STAIR, 13}, {"Purpur Stairs", XB_STAIR, 14},
        {"Nether Brick Slab", XB_SLAB, 11}, {"Quartz Slab", XB_SLAB, 12}, {"Red Sandstone Slab", XB_SLAB, 13}, {"Purpur Slab", XB_SLAB, 14}, {"Petrified Oak Slab", XB_SLAB, 15},
        {"block/dispenser_front", XB_DISP, 0}, {"block/dropper_front", XB_DROP, 0}, {"block/observer_front", XB_OBS, 0}, {"block/observer_top", XB_OBS, 0}, {"Sticky Piston", XB_PIST, 1}, {"Piston", XB_PIST, 0}, {"block/piston_top", XB_PIST, 0},
        {"Stone Pressure Plate", XB_PLATE, 0}, {"Oak Pressure Plate", XB_PLATE, 1},  {"Light Weighted Pressure Plate", XB_PLATE, 2}, {"Heavy Weighted Pressure Plate", XB_PLATE, 3},
        {"Stone Button", XB_BTN, 0}, {"Oak Button", XB_BTN, 1}, {"block/tripwire_hook", XB_HOOK, 0}, {"Chest", XB_CHEST, 0}, {"Trapped Chest", XB_CHEST, 1}, {"Ender Chest", XB_CHEST, 2}, {"block/daylight_detector_top", XB_DAY, 0},
        {"block/hopper_outside", XB_HOPPER, 0}, {"item/redstone", XB_DUST, 0}, {"item/repeater", XB_REPT, 0}, {"item/comparator", XB_COMP, 0},
        {"Iron Trapdoor", XB_TRAP, 0}, {"Oak Wood Trapdoor", XB_TRAP, 1}, 
        {"Oak Fence Gate", XB_GATE, 0}, {"Spruce Fence Gate", XB_GATE, 1}, {"Birch Fence Gate", XB_GATE, 2}, {"Jungle Fence Gate", XB_GATE, 3}, {"Acacia Fence Gate", XB_GATE, 4}, {"Dark Oak Fence Gate", XB_GATE, 5},
        {"Oak Fence", XB_FENCE, 0}, {"Spruce Fence", XB_FENCE, 1}, {"Birch Fence", XB_FENCE, 2}, {"Jungle Fence", XB_FENCE, 3}, {"Acacia Fence", XB_FENCE, 4}, {"Dark Oak Fence", XB_FENCE, 5}, {"Nether Brick Fence", XB_FENCE, 6},
        {"block/end_rod", XB_ENDROD, 0},
        {"block/white_stained_glass_pane_top", XB_CPANE, 0}, {"block/orange_stained_glass_pane_top", XB_CPANE, 1}, {"block/magenta_stained_glass_pane_top", XB_CPANE, 2}, {"block/light_blue_stained_glass_pane_top", XB_CPANE, 3}, {"block/yellow_stained_glass_pane_top", XB_CPANE, 4}, {"block/lime_stained_glass_pane_top", XB_CPANE, 5}, {"block/pink_stained_glass_pane_top", XB_CPANE, 6}, {"block/gray_stained_glass_pane_top", XB_CPANE, 7}, {"block/light_gray_stained_glass_pane_top", XB_CPANE, 8}, {"block/cyan_stained_glass_pane_top", XB_CPANE, 9}, {"block/purple_stained_glass_pane_top", XB_CPANE, 10}, {"block/blue_stained_glass_pane_top", XB_CPANE, 11}, {"block/brown_stained_glass_pane_top", XB_CPANE, 12}, {"block/green_stained_glass_pane_top", XB_CPANE, 13}, {"block/red_stained_glass_pane_top", XB_CPANE, 14}, {"block/black_stained_glass_pane_top", XB_CPANE, 15},
        {"White Bed", XB_BED, 0}, {"Orange Bed", XB_BED, 1}, {"Magenta Bed", XB_BED, 2}, {"Light Blue Bed", XB_BED, 3}, {"Yellow Bed", XB_BED, 4}, {"Lime Bed", XB_BED, 5}, {"Pink Bed", XB_BED, 6}, {"Gray Bed", XB_BED, 7}, {"Light Gray Bed", XB_BED, 8}, {"Cyan Bed", XB_BED, 9}, {"Purple Bed", XB_BED, 10}, {"Blue Bed", XB_BED, 11}, {"Brown Bed", XB_BED, 12}, {"Green Bed", XB_BED, 13}, {"Red Bed", XB_BED, 14}, {"Black Bed", XB_BED, 15},
        {"item/oak_sign", XB_SIGN, 0}, 
        {"Cobblestone Wall", XB_WALL, 0}, {"Mossy Cobblestone Wall", XB_WALL, 1}, {"block/ladder", XB_LADDER, 0}, {"block/lily_pad", XB_LILY, 0}, {"block/vine", XB_VINE, 0}, {"block/iron_bars", XB_PANE, 0}, {"block/glass_pane_top", XB_PANE, 1},
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
        if (!hit) { const std::string cf = pkCanon(e.file); if (cf != e.file) for (const Row& r : R) if (cf == r.key) { hit = &r; break; } }   // pack 1.12: item/door_wood = item/oak_door
        if (!hit) continue;
        if (hit->blk == XB_DOOR) { static const char* DN[7] = {"Iron Door", "Oak Door", "Spruce Door", "Birch Door", "Jungle Door", "Acacia Door", "Dark Oak Door"}; e.name = DN[hit->var % 7]; e.fold = foldAscii(e.name.c_str()); }
        gXbItem[id] = hit->blk; gRsVar[id] = hit->var;
        if (!gRsDrop[hit->blk - XB_DUST][hit->var & 31]) gRsDrop[hit->blk - XB_DUST][hit->var & 31] = id;
        if (byName || hit->blk == XB_DAY || hit->blk == XB_HOPPER) e.cube = false;   // vira icone do modelo (rsIconQuads) ou plano, nunca cubo
        if (hit->blk == XB_HOPPER) for (const PkItem& q : PK) if (q.file == "item/hopper") { e.tile = q.tile; break; }   // funil: icone do item, como no Minecraft
        if (hit->blk == XB_PIST) {   // patchV.5: normal tambem (antes so o grudento tinha cubo com lados; o normal ficava com a cabeca nas 6 faces)   // pistao grudento: cubo com a cabeca grudenta em cima (antes mostrava o topo do pistao normal)
            int st = -1, sd = -1, bt = -1; for (const PkItem& q : PK) { if (q.file == (hit->var == 1 ? "block/piston_top_sticky" : "block/piston_top_normal") || (hit->var != 1 && q.file == "block/piston_top")) st = q.tile; else if (q.file == "block/piston_side") sd = q.tile; else if (q.file == "block/piston_bottom") bt = q.tile; }
            if (st >= 0 && sd >= 0) { e.cube = true; e.tile = st; e.ft[0] = st; e.ft[1] = sd; e.ft[2] = bt >= 0 ? bt : sd; } }
        if (hit->blk == XB_LADDER || hit->blk == XB_ENDROD || hit->blk == XB_LILY || hit->blk == XB_VINE) e.cube = false;
        if (hit->blk == XB_CPANE) { e.cube = false; const std::string gf = std::string("block/") + RS_COLOR[hit->var & 15] + "_stained_glass"; for (const PkItem& q : PK) if (q.file == gf || (q.file.compare(0, 6, "block/") == 0 && "block/" + pk112(q.file.substr(6)) == gf)) { e.tile = q.tile; break; } }   // painel colorido: sprite do vidro   // escada: item plano (sprite)
        if (hit->blk == XB_PANE) { e.cube = false; if (hit->var == 1) for (const PkItem& q : PK) if (q.file == "block/glass") { e.tile = q.tile; break; } }   // painel/barras: item plano (sprite), como no Minecraft
    }
    }
    for (const char* f : {"block/iron_trapdoor", "block/trapdoor", "block/oak_trapdoor"}) {   // patchTrap.1: itens soltos do pack (aba Resource pack/busca) tinham textura mas nao colocavam: viram o bloco de alcapao (ferro = var 0, madeira = var 1)
        const int ti = pkFind(f); if (!isPk(ti) || gXbItem.count(ti)) continue; const int v = f[6] == 'i' ? 0 : 1;
        gXbItem[ti] = XB_TRAP; gRsVar[ti] = v; PK[ti - PK0].cube = false; if (!gRsDrop[XB_TRAP - XB_DUST][v]) gRsDrop[XB_TRAP - XB_DUST][v] = ti; }
    for (const char* f : {"block/beacon", "block/chorus_plant"}) { const int ci = pkFind(f); if (!isPk(ci)) continue; const int v = f[6] == 'b' ? 2 : 3;   // patchDeco.1: sinalizador (var 2) e planta de coro (var 3) no mesmo bloco do bolo/caldeirao
        gXbItem[ci] = XB_DECOR; gRsVar[ci] = v; gRsDrop[XB_DECOR - XB_DUST][v] = ci; PK[ci - PK0].cube = false; }   // patchOpt.2: icone/mao/chao usam o modelo (vidro+obsidiana+nucleo), nao o cubo da textura
    for (const char* f : {"item/cauldron", "item/cake"}) { const int ci = pkFind(f); if (!isPk(ci)) continue; const int v = f[5] == 'c' && f[6] == 'a' && f[7] == 'u' ? 0 : 1;   // caldeirao (var 0) e bolo (var 1)
        gXbItem[ci] = XB_DECOR; gRsVar[ci] = v; gRsDrop[XB_DECOR - XB_DUST][v] = ci; }
    { int pi = pkFind("block/grass_path"); if (!pi) pi = pkFind("block/dirt_path");   // caminho de terra: o item do pack vira XB_FARM variante 1 (15/16 de altura)
      if (isPk(pi)) { gXbItem[pi] = XB_FARM; gRsVar[pi] = 1; gRsDrop[XB_FARM - XB_DUST][1] = pi; PK[pi - PK0].cube = false; } }
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

// ================= PISTAO (patch5.38): energia simples + estender/recolher + empurrar ate 12 blocos + grudento =================
// Estado no meta (ver rsBuild): bits0-2 dir6, bit3 cabeca, bit4 corpo estendido. Instantaneo (sem animacao). Fontes: alavanca ligada, tocha de redstone, bloco de redstone
// (so com pack), tambem pela alavanca/tocha ao lado de UM bloco solido encostado no pistao. SEM botao/placa/poeira (o jogo ainda nao propaga energia).
static const int PS_DX[6] = {0, 0, 0, 1, 0, -1}, PS_DY[6] = {-1, 1, 0, 0, 0, 0}, PS_DZ[6] = {0, 0, -1, 0, 1, 0};
static int pistonClass(int b, int m) {   // 0 = some/quebra quando empurrado, 1 = movel, 2 = imovel
    if (b == AIR || isLiquid(b) || b == XB_FIRE) return 0;
    if (b == BEDROCK || b == OBSIDIAN || b == XB_PORTAL) return 2;
    if (b == XB_PIST || b == XB_PIST + 1) return (m & 24) ? 2 : 1;
    if (isXB(b) && b < XB_DUST) return (b >= XB_ANVIL && b < XB_ANVIL + 4) ? 1 : 0;
    if (isRs(b)) { const int k = b - XB_DUST;
        if (k == 0 || k == 6 || k == 7 || k == 8 || k == 11 || k == 12 || k == 17 || k == 20 || k == 23 || k == 25) return 0;
        if (k == 3 || k == 4 || k == 10 || k == 16 || k == 24) return 2; }
    return 1;
}
static bool pistonSrc(int x, int y, int z) { const int b = getB(x, y, z); return (b >= XB_LEVER + 12 && b < XB_LEVER + 24) || (b >= XB_RTORCH && b < XB_RTORCH + 5 && !(getM(x, y, z) & 1)) || (gRsRB && b == gRsRB) || ((b == XB_BTN || b == XB_PLATE) && ((getM(x, y, z) >> 4) & 1)) || (b == XB_DUST && ((getM(x, y, z) >> 4) & 15)); }   // botao: bit4 = apertado
static bool pistonPowered(int x, int y, int z, int d) {
    for (int s = 0; s < 6; s++) {
        if (s == d) continue;
        const int nx = x + PS_DX[s], ny = y + PS_DY[s], nz = z + PS_DZ[s], b = getB(nx, ny, nz);
        if (pistonSrc(nx, ny, nz) || rsRepOutInto(nx, ny, nz, x, y, z)) return true;
        if (!solid(b) || isXB(b)) continue;
        for (int t = 0; t < 6; t++) { const int mx = nx + PS_DX[t], my = ny + PS_DY[t], mz = nz + PS_DZ[t]; if (mx == x && my == y && mz == z) continue; if (pistonSrc(mx, my, mz) || rsRepOutInto(mx, my, mz, nx, ny, nz)) return true; }
    }
    return false;
}
static void pistonPutB(int x, int y, int z, int id, int m) { setBM(x, y, z, id, m); netSendBlock(x, y, z, id, m); }
static Phys* pistonP2();   // splitscreen.inc: fisica do jogador 2 (nullptr se o split esta desligado)
static bool pistonPushPt(double& x, double& y, double& z, double hw, double h, int cx, int cy, int cz, int d) {   // (x,y,z) = pes; empurra para fora da celula, na direcao do movimento
    const AABB c{(double)cx, (double)cy, (double)cz, cx + 1.0, cy + 1.0, cz + 1.0}, b{x - hw, y, z - hw, x + hw, y + h, z + hw}; if (!b.intersects(c)) return false;
    if (d == 1) y = cy + 1.001; else if (d == 0) y = cy - h - .001; else if (d == 3) x = cx + 1 + hw + .001; else if (d == 5) x = cx - hw - .001; else if (d == 4) z = cz + 1 + hw + .001; else z = cz - hw - .001;
    return true;
}
static void pistonPushPlayer(int cx, int cy, int cz, int d) {   // a celula acabou de ser ocupada: jogadores, mobs e itens dentro dela saem empurrados
    for (Phys* p : {gPvpP, pistonP2()}) if (p && !p->noclip && pistonPushPt(p->x, p->y, p->z, PW, PH, cx, cy, cz, d)) { if (d == 1) p->my = std::max(p->my, 0.); p->px = p->x; p->py = p->y; p->pz = p->z; }
    for (MobEnt& e : MOBS) if (!e.dead && pistonPushPt(e.x, e.y, e.z, MD[e.type].w * .5, MD[e.type].h, cx, cy, cz, d)) { if (d == 1) e.my = std::max(e.my, 0.); e.px = e.x; e.py = e.y; e.pz = e.z; }
    for (DropEnt& r : DROPS) if (pistonPushPt(r.x, r.y, r.z, .125, .25, cx, cy, cz, d)) { r.px = r.x; r.py = r.y; r.pz = r.z; }
}
static void pAnimAdd(int x, int y, int z, int d, int id, int m, int s0, int e0, bool hide) {   // so blocos que sei desenhar (cubo comum ou modelo de redstone); o resto fica sem animacao
    if (isXB(id) && !isRs(id)) return; if (y < 0 || y >= CH) return;
    gPAnim.push_back({x, y, z, d, id, m, s0, e0, hide, gWorldTick, 0});   // ms0 = 0: o relogio comeca no 1o quadro desenhado (SDL_GetTicks nao existe nos testes headless)
    if (hide) for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) dirtyC(fdiv(x, CW) + dx, fdiv(z, CW) + dz);
}
static void drawPistonAnims() {   // (VIS) bloco deslizando 1 celula em ~100 ms; sem teste headless
    if (gPAnim.empty()) return; const uint32_t now = SDL_GetTicks();
    static const float C[6][4][3] = {{{0,0,0},{0,0,1},{0,1,1},{0,1,0}}, {{1,0,1},{1,0,0},{1,1,0},{1,1,1}}, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    static const float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glDisable(GL_CULL_FACE);
    for (PAnim& a : gPAnim) {
        if (!a.ms0) a.ms0 = now ? now : 1;
        const double p = std::min(1.0, (double)(now - a.ms0) / 100.0), off = a.s0 + (a.e0 - a.s0) * p;
        const float ox = (float)(a.x + PS_DX[a.d] * off), oy = (float)(a.y + PS_DY[a.d] * off), oz = (float)(a.z + PS_DZ[a.d] * off);
        entLightAt(ox + .5, oy + .5, oz + .5, gEntDim); glColor3f(gEntDim[0], gEntDim[1], gEntDim[2]);
        glBegin(GL_QUADS);
        if (isRs(a.id)) { for (const XQuad& q : rsQuads(a.id, a.m)) for (int k = 0; k < 4; k++) { glTexCoord2f((q.tile % ATL + q.u[k]) / (float)ATL, (q.tile / ATL + q.v[k]) / (float)ATLR); glVertex3f(ox + q.p[k][0], oy + q.p[k][1], oz + q.p[k][2]); } }
        else for (int f = 0; f < 6; f++) { const int t = blkTile(a.id, f == 2 ? 2 : f == 3 ? 0 : 1);
            for (int k = 0; k < 4; k++) { glTexCoord2f((t % ATL + UV[k][0]) / (float)ATL, (t / ATL + UV[k][1]) / (float)ATLR); glVertex3f(ox + C[f][k][0], oy + C[f][k][1], oz + C[f][k][2]); } }
        glEnd();
    }
    entDimReset(); glColor4f(1.f, 1.f, 1.f, 1.f);
}
static bool pistonExtend(int x, int y, int z, int id, int d) {
    struct Cell { int x, y, z, id, m; }; std::vector<Cell> cs; bool end = false;
    for (int i = 1; i <= 13 && !end; i++) {
        const int cx = x + PS_DX[d] * i, cy = y + PS_DY[d] * i, cz = z + PS_DZ[d] * i; if (cy < 0 || cy >= CH) return false;
        const int b = getB(cx, cy, cz), m = getM(cx, cy, cz), c = pistonClass(b, m);
        if (c == 2) return false; if (c == 0) { end = true; break; }
        if ((int)cs.size() >= 12) return false; cs.push_back({cx, cy, cz, b, m});
    }
    if (!end && (int)cs.size() > 12) return false;
    const int hx = x + PS_DX[d], hy = y + PS_DY[d], hz = z + PS_DZ[d]; if (hy < 0 || hy >= CH) return false;
    for (int i = (int)cs.size() - 1; i >= 0; i--) { const Cell& c = cs[i]; const int tx = c.x + PS_DX[d], ty = c.y + PS_DY[d], tz = c.z + PS_DZ[d]; if (ty < 0 || ty >= CH) continue; pistonPutB(tx, ty, tz, c.id, c.m); pAnimAdd(tx, ty, tz, d, c.id, c.m, -1, 0, true); pistonPushPlayer(tx, ty, tz, d); }
    pistonPutB(hx, hy, hz, id, 8 | d); pAnimAdd(hx, hy, hz, d, id, 8 | d, -1, 0, true); pistonPushPlayer(hx, hy, hz, d);
    pistonPutB(x, y, z, id, 16 | d); return true;
}
static void pistonRetract(int x, int y, int z, int id, int d) {
    const int fx = x + PS_DX[d], fy = y + PS_DY[d], fz = z + PS_DZ[d];
    pistonPutB(x, y, z, id, d);
    if (getB(fx, fy, fz) != id || getM(fx, fy, fz) != (8 | d)) return;
    const int gx = fx + PS_DX[d], gy = fy + PS_DY[d], gz = fz + PS_DZ[d];
    pAnimAdd(fx, fy, fz, d, id, 8 | d, 0, -1, false);   // a cabeca recolhe deslizando
    if (id == XB_PIST + 1 && gy >= 0 && gy < CH) { const int gb = getB(gx, gy, gz), gm = getM(gx, gy, gz); if (pistonClass(gb, gm) == 1) { pistonPutB(fx, fy, fz, gb, gm); pAnimAdd(fx, fy, fz, d, gb, gm, 1, 0, true); pistonPutB(gx, gy, gz, AIR, 0); return; } }
    pistonPutB(fx, fy, fz, AIR, 0);
}
static void pistonCheck(int x, int y, int z) {
    static int depth = 0; if (depth > 24) return; depth++;
    const int b = getB(x, y, z), m = getM(x, y, z);
    if ((b == XB_PIST || b == XB_PIST + 1) && (m & 7) <= 5) {
        const int d = m & 7;
        if (m & 8) { const int bx = x - PS_DX[d], by = y - PS_DY[d], bz = z - PS_DZ[d]; if (getB(bx, by, bz) != b || getM(bx, by, bz) != (16 | d)) pistonPutB(x, y, z, AIR, 0); }   // cabeca sem corpo some
        else {
            bool ext = (m & 16) != 0;
            if (ext && !(getB(x + PS_DX[d], y + PS_DY[d], z + PS_DZ[d]) == b && getM(x + PS_DX[d], y + PS_DY[d], z + PS_DZ[d]) == (8 | d))) { pistonPutB(x, y, z, b, d); ext = false; }   // cabeca quebrada: corpo volta
            const bool pw = pistonPowered(x, y, z, d);
            if (pw && !ext) pistonExtend(x, y, z, b, d); else if (!pw && ext) pistonRetract(x, y, z, b, d);
        }
    }
    depth--;
}
static bool plateHit(int x, int y, int z, bool items, double ex, double ey, double ez) { return (int)floor(ex) == x && (int)floor(ey + .001) == y && (int)floor(ez) == z; }
static bool plateOccupied(int x, int y, int z, bool items) {   // alguem em cima da placa? (jogadores, mobs; madeira tambem itens)
    for (Phys* p : {gPvpP, pistonP2()}) if (p && !p->noclip && plateHit(x, y, z, items, p->x, p->y, p->z)) return true;
    for (const MobEnt& e : MOBS) if (!e.dead && plateHit(x, y, z, items, e.x, e.y, e.z)) return true;
    if (items) for (const DropEnt& r : DROPS) if (plateHit(x, y, z, items, r.x, r.y, r.z)) return true;
    return false;
}
static void rsPlateCell(int x, int y, int z, bool items) {   // chamado todo tick de mundo para a celula de cada entidade
    if (getB(x, y, z) != XB_PLATE) return; const int m = getM(x, y, z); if ((m >> 4) & 1) return;
    pistonPutB(x, y, z, XB_PLATE, m | 16); scheduleTick(x, y, z, 10); pistonScan(x, y, z);
}
static void rsPlatesTick() {
    for (Phys* p : {gPvpP, pistonP2()}) if (p && !p->noclip) rsPlateCell((int)floor(p->x), (int)floor(p->y + .001), (int)floor(p->z), true);
    for (const MobEnt& e : MOBS) if (!e.dead) rsPlateCell((int)floor(e.x), (int)floor(e.y + .001), (int)floor(e.z), true);
    for (const DropEnt& r : DROPS) { const int cx = (int)floor(r.x), cy = (int)floor(r.y + .001), cz = (int)floor(r.z); if (getB(cx, cy, cz) == XB_PLATE && (getM(cx, cy, cz) & 15) != 0) rsPlateCell(cx, cy, cz, true); }   // pedra (tipo 0) ignora itens
}
static bool rsNbLoaded(int x, int z) { static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; for (const auto& d : D) if (!getC((x + d[0]) >> 4, (z + d[1]) >> 4)) return false; return true; }   // tick de reavaliacao no limite de chunk nao carregado nao decide nada
static void rsTick(int x, int y, int z) {   // tick agendado: solta o botao / a placa (se ninguem esta em cima)
    if (getB(x, y, z) == XB_OBS) { rsObsTick(x, y, z); return; }
    { const int cb = getB(x, y, z); if ((cb == XB_REPT || cb == XB_COMP || (cb >= XB_RTORCH && cb < XB_RTORCH + 5)) && !rsNbLoaded(x, z)) return; }
    if (getB(x, y, z) == XB_COMP) { rsCompTick(x, y, z); return; }
    { const int tb = getB(x, y, z); if (tb >= XB_RTORCH && tb < XB_RTORCH + 5) { int ax, ay, az; rsTorchAttach(tb, x, y, z, ax, ay, az); const int m = getM(x, y, z); const bool pw = rsBlockPowered(ax, ay, az, x, y, z);
        if (pw != (bool)(m & 1)) { pistonPutB(x, y, z, tb, pw ? (m | 1) : (m & ~1)); rsTorchNotify(x, y, z); } return; } }
    if (getB(x, y, z) == XB_REPT) { rsRepLockSync(x, y, z); const int m = getM(x, y, z); const bool in = rsRepInput(x, y, z); if (in == (bool)((m >> 4) & 1) || rsRepLocked(x, y, z)) return;
        pistonPutB(x, y, z, XB_REPT, in ? (m | 16) : (m & ~16)); const int fx = x + RS_DX[m & 3], fz = z + RS_DZ[m & 3], fb = getB(fx, y, fz);
        if (fb == XB_DUST) rsDustRecalc(fx, y, fz); else if (fb == XB_REPT) rsRepCheck(fx, y, fz); else if (fb == XB_PIST || fb == XB_PIST + 1) pistonCheck(fx, y, fz); else if (solid(fb) && !isXB(fb)) pistonScan(fx, y, fz);
        return; }
    if (getB(x, y, z) == XB_PLATE) { const int m = getM(x, y, z); if (!((m >> 4) & 1)) return;
        if (plateOccupied(x, y, z, (m & 15) != 0)) { scheduleTick(x, y, z, 10); return; }
        pistonPutB(x, y, z, XB_PLATE, m & ~16); pistonScan(x, y, z); return; }
    if (getB(x, y, z) != XB_BTN) return; const int m = getM(x, y, z); if (!((m >> 4) & 1)) return;
    pistonPutB(x, y, z, XB_BTN, m & ~16); pistonScan(x, y, z);
}
static bool rsStrongSrc(int x, int y, int z) { const int b = getB(x, y, z); return (b >= XB_LEVER + 12 && b < XB_LEVER + 24) || (b >= XB_RTORCH && b < XB_RTORCH + 5 && !(getM(x, y, z) & 1)) || (gRsRB && b == gRsRB) || ((b == XB_BTN || b == XB_PLATE) && ((getM(x, y, z) >> 4) & 1)); }
static bool rsStrongBlock(int x, int y, int z) {   // bloco solido FORTEMENTE energizado (liga poeira): tocha acesa de pe logo abaixo ou repetidor ligado apontando para ele
    const int b = getB(x, y, z); if (!solid(b) || isXB(b)) return false;
    if (getB(x, y - 1, z) == XB_RTORCH && !(getM(x, y - 1, z) & 1)) return true;
    static const int SUP[4][3] = {{0, 0, 1}, {-1, 0, 0}, {0, 0, -1}, {1, 0, 0}};   // alavanca ligada presa ao bloco (parede/chao/teto) o energiza forte
    for (int k = 0; k < 4; k++) if (getB(x - SUP[k][0], y, z - SUP[k][2]) == XB_LEVER + 12 + k) return true;
    for (int k = 4; k < 8; k++) if (getB(x, y + 1, z) == XB_LEVER + 12 + k) return true;
    for (int k = 8; k < 12; k++) if (getB(x, y - 1, z) == XB_LEVER + 12 + k) return true;
    if (getB(x, y + 1, z) == XB_PLATE && ((getM(x, y + 1, z) >> 4) & 1)) return true;   // placa apertada sobre o bloco
    for (int q = 0; q < 6; q++) { const int ox = x + PS_DX[q], oy = y + PS_DY[q], oz = z + PS_DZ[q]; if (getB(ox, oy, oz) == XB_OBS && rsObsOut(ox, oy, oz, x, y, z)) return true; }   // observador ligado apontando a saida para o bloco
    { const int u = getB(x, y + 1, z), dn = getB(x, y - 1, z);   // botao apertado preso ao bloco (meta: 0 chao, 1 teto, 2..5 parede = dir+2)
      if (u == XB_BTN && ((getM(x, y + 1, z) >> 4) & 1) && (getM(x, y + 1, z) & 7) == 0) return true;
      if (dn == XB_BTN && ((getM(x, y - 1, z) >> 4) & 1) && (getM(x, y - 1, z) & 7) == 1) return true;
      for (int d = 0; d < 4; d++) { const int sx = x + RS_DX[d], sz = z + RS_DZ[d]; if (getB(sx, y, sz) == XB_BTN && ((getM(sx, y, sz) >> 4) & 1) && (getM(sx, y, sz) & 7) == d + 2) return true; } }
    for (int d = 0; d < 4; d++) if (rsRepOutInto(x - RS_DX[d], y, z - RS_DZ[d], x, y, z)) return true;
    return false;
}
static bool rsRepLocked(int x, int y, int z) {   // travamento: repetidor ligado ao lado apontando para este mantem o estado
    if (getB(x, y, z) != XB_REPT) return false; const int d = getM(x, y, z) & 3;
    for (int e = 1; e <= 3; e += 2) { const int sd = (d + e) & 3; if (rsRepOutInto(x + RS_DX[sd], y, z + RS_DZ[sd], x, y, z)) return true; }
    return false;
}
static void rsRepCheck(int x, int y, int z); static void rsRepLockSync(int x, int y, int z);
static bool rsRepInput(int x, int y, int z) {   // repetidor (meta: dir de saida 0-3 N L S O | atraso<<2 | ligado<<4): entrada = celula de tras
    const int m = getM(x, y, z), d = m & 3, bx = x - RS_DX[d], bz = z - RS_DZ[d], b = getB(bx, y, bz);
    if (rsStrongSrc(bx, y, bz) || (b == XB_OBS && rsRepOutInto(bx, y, bz, x, y, z))) return true; if (solid(b) && !isXB(b) && rsBlockPowered(bx, y, bz, x, y, z)) return true; if (b == XB_DUST) return ((getM(bx, y, bz) >> 4) & 15) > 0;
    return b == XB_REPT && ((getM(bx, y, bz) >> 4) & 1) && (getM(bx, y, bz) & 3) == d;
}
// ---- observador (meta: dir 0-5 = frente/rosto, igual ao pistao | pulso<<4): detecta mudanca de bloco/meta na celula da FRENTE e pulsa 2 ticks pela celula de TRAS ----
static std::unordered_map<int64_t, int> gObsSig;   // ultimo (id | meta<<16) visto na frente de cada observador (nao e salvo: o 1o evento so registra)
static int rsObsSeen(int x, int y, int z) { const int d = getM(x, y, z) & 7; if (d > 5) return -1; const int fx = x + PS_DX[d], fy = y + PS_DY[d], fz = z + PS_DZ[d]; return getB(fx, fy, fz) | (getM(fx, fy, fz) << 16); }
static void rsObsAdded(int x, int y, int z) { if (getB(x, y, z) == XB_OBS) gObsSig[k3(x, y, z)] = rsObsSeen(x, y, z); }
static void rsObsNeighbor(int x, int y, int z) {   // algum vizinho mudou: pulsa se foi a celula da frente
    if (getB(x, y, z) != XB_OBS || (getM(x, y, z) & 7) > 5) return;
    const int now = rsObsSeen(x, y, z); auto it = gObsSig.find(k3(x, y, z));
    if (it == gObsSig.end()) { gObsSig[k3(x, y, z)] = now; return; }
    if (it->second == now) return; it->second = now;
    if (!((getM(x, y, z) >> 4) & 1) && !tickPending(x, y, z)) scheduleTick(x, y, z, 2);
}
static void rsObsTick(int x, int y, int z) {   // 2 ticks apos detectar liga; 2 ticks depois desliga
    const int m = getM(x, y, z); if ((m & 7) > 5) return;
    if (!((m >> 4) & 1)) { pistonPutB(x, y, z, XB_OBS, m | 16); scheduleTick(x, y, z, 2); } else pistonPutB(x, y, z, XB_OBS, m & ~16);
    pistonScan(x, y, z);
}
static bool rsObsOut(int ox, int oy, int oz, int tx, int ty, int tz) {   // observador ligado empurra a celula de TRAS
    const int m = getM(ox, oy, oz), d = m & 7; if (!((m >> 4) & 1) || d > 5) return false;
    return ox - PS_DX[d] == tx && oy - PS_DY[d] == ty && oz - PS_DZ[d] == tz;
}
static bool rsRepOutInto(int rx, int ry, int rz, int tx, int ty, int tz) { if (getB(rx, ry, rz) == XB_OBS) return rsObsOut(rx, ry, rz, tx, ty, tz); if (getB(rx, ry, rz) == XB_COMP) { const int cm = getM(rx, ry, rz); return ((cm >> 3) & 1) && ry == ty && rx + RS_DX[cm & 3] == tx && rz + RS_DZ[cm & 3] == tz; } if (getB(rx, ry, rz) != XB_REPT) return false; const int m = getM(rx, ry, rz); return ((m >> 4) & 1) && ry == ty && rx + RS_DX[m & 3] == tx && rz + RS_DZ[m & 3] == tz; }
// ---- comparador (meta: dir de saida 0-3 | modo<<2 (1 = subtrair) | ligado<<3 | forca de saida 0-15 nos bits 4-7): traseiro vs laterais, atraso de 2 ticks ----
static int rsSigInto(int ax, int ay, int az, int tx, int ty, int tz) {   // forca (0-15) que a celula A entrega na celula vizinha T
    const int b = getB(ax, ay, az); if (b == XB_DUST) return (getM(ax, ay, az) >> 4) & 15;
    if (b == XB_CHEST) return std::max(chestSignal(ax, ay, az), 0);   // bau atras do comparador: nivel de enchimento
    if (rsRepOutInto(ax, ay, az, tx, ty, tz) || rsStrongSrc(ax, ay, az)) return 15;
    return (solid(b) && !isXB(b) && rsBlockPowered(ax, ay, az, tx, ty, tz)) ? 15 : 0;
}
static int rsCompOutVal(int x, int y, int z) {
    const int m = getM(x, y, z), d = m & 3; const int rear = rsSigInto(x - RS_DX[d], y, z - RS_DZ[d], x, y, z); int side = 0;
    for (int e = 1; e <= 3; e += 2) { const int sd = (d + e) & 3, sx = x + RS_DX[sd], sz = z + RS_DZ[sd], sb = getB(sx, y, sz); if (sb == XB_DUST || sb == XB_REPT || sb == XB_COMP || sb == XB_OBS) side = std::max(side, rsSigInto(sx, y, sz, x, y, z)); }
    return ((m >> 2) & 1) ? std::max(rear - side, 0) : (rear >= side ? rear : 0);
}
static void rsCompCheck(int x, int y, int z) { if (getB(x, y, z) != XB_COMP) return; if (rsCompOutVal(x, y, z) != ((getM(x, y, z) >> 4) & 15)) scheduleTick(x, y, z, 4); }
static void rsCompTick(int x, int y, int z) {
    const int m = getM(x, y, z), o = rsCompOutVal(x, y, z); if (o == ((m >> 4) & 15)) return;
    pistonPutB(x, y, z, XB_COMP, (m & 7) | (o << 4) | (o ? 8 : 0)); const int fx = x + RS_DX[m & 3], fz = z + RS_DZ[m & 3], fb = getB(fx, y, fz);
    if (fb == XB_DUST) rsDustRecalc(fx, y, fz); else if (fb == XB_REPT || fb == XB_COMP) rsRepCheck(fx, y, fz); else if (fb == XB_PIST || fb == XB_PIST + 1) pistonCheck(fx, y, fz); else if (solid(fb) && !isXB(fb)) pistonScan(fx, y, fz);
}
static void rsRepLockSync(int x, int y, int z) {   // bit 5 do meta = travado (so visual: barra de bedrock); refaz a malha ao mudar
    if (getB(x, y, z) != XB_REPT) return; const int m = getM(x, y, z), lk = rsRepLocked(x, y, z) ? 32 : 0;
    if ((m & 32) != lk) pistonPutB(x, y, z, XB_REPT, (m & ~32) | lk);
}
static void rsRepCheck(int x, int y, int z) { if (getB(x, y, z) == XB_COMP) { rsCompCheck(x, y, z); return; } if (getB(x, y, z) != XB_REPT) return; rsRepLockSync(x, y, z); if (rsRepLocked(x, y, z)) return; const int m = getM(x, y, z); if (rsRepInput(x, y, z) != (bool)((m >> 4) & 1)) scheduleTick(x, y, z, (1 + ((m >> 2) & 3)) * 2); }
static void rsTorchAttach(int id, int x, int y, int z, int& ax, int& ay, int& az) {   // bloco onde a tocha de redstone esta presa
    const int k = id - XB_RTORCH; ax = x; ay = y; az = z; if (k == 0) ay--; else if (k == 1) ax--; else if (k == 2) az--; else if (k == 3) ax++; else az++;
}
static bool rsBlockPowered(int ax, int ay, int az, int tx, int ty, int tz) {   // o bloco A recebe energia de alguma fonte vizinha (menos da propria tocha T)?
    for (int s = 0; s < 6; s++) { const int nx = ax + PS_DX[s], ny = ay + PS_DY[s], nz = az + PS_DZ[s]; if (nx == tx && ny == ty && nz == tz) continue;
        const int b = getB(nx, ny, nz);
        if (b >= XB_RTORCH && b < XB_RTORCH + 5) { if (s == 0 && b == XB_RTORCH && !(getM(nx, ny, nz) & 1)) return true; continue; }   // so tocha de pe logo ABAIXO do bloco o alimenta
        if (rsStrongSrc(nx, ny, nz) || rsRepOutInto(nx, ny, nz, ax, ay, az)) return true;
        if (b == XB_DUST && ((getM(nx, ny, nz) >> 4) & 15) > 0) return true; }
    return false;
}
static bool rsConsumerPowered(int x, int y, int z) {   // patchProp.11: alcapao/porta/portao recebem energia de fonte vizinha OU de bloco solido vizinho fortemente energizado (alavanca presa nele, tocha embaixo, repetidor)
    if (rsBlockPowered(x, y, z, -99999, -99999, -99999)) return true;
    for (int s = 0; s < 6; s++) if (rsStrongBlock(x + PS_DX[s], y + PS_DY[s], z + PS_DZ[s])) return true;
    return false;
}
static void rsTrapCheck(int x, int y, int z) {   // patchMod.2: alcapao (ferro e madeira) abre com redstone (alavanca, botao, placa, tocha, fio) e fecha ao perder energia
    if (getB(x, y, z) != XB_TRAP) return; const int m = getM(x, y, z); const bool op = (m >> 3) & 1, pw = rsConsumerPowered(x, y, z);
    if (pw != op) { const int nm = m ^ 8; setBM(x, y, z, XB_TRAP, nm); netSendBlock(x, y, z, XB_TRAP, nm); }
}
static void rsOpenCheck(int x, int y, int z) {   // patchMod.9: portao e porta (ferro e madeira) abrem/fecham com redstone; a porta olha as duas metades
    const int b = getB(x, y, z); if (b != XB_GATE && b != XB_DOOR) return; const int m = getM(x, y, z); const bool op = (m >> 2) & 1;
    if (b == XB_GATE) { const bool pw = rsConsumerPowered(x, y, z); if (pw != op) { const int nm = m ^ 4; setBM(x, y, z, b, nm); netSendBlock(x, y, z, b, nm); } return; }
    const bool up = (m >> 3) & 1; const int oy = up ? y - 1 : y + 1; const bool two = getB(x, oy, z) == b;
    const bool pw = rsConsumerPowered(x, y, z) || (two && rsConsumerPowered(x, oy, z));
    if (pw == op) return; const bool old = gNoBlockHooks; gNoBlockHooks = true;
    if (two) { const int om = getM(x, oy, z) ^ 4; setBM(x, oy, z, b, om); netSendBlock(x, oy, z, b, om); }
    setBM(x, y, z, b, m ^ 4); netSendBlock(x, y, z, b, m ^ 4); gNoBlockHooks = old;
}
static void rsTorchCheck(int x, int y, int z) {   // tocha de redstone: acesa a menos que o bloco onde esta presa receba energia (atraso de 2 ticks)
    const int id = getB(x, y, z); if (id < XB_RTORCH || id >= XB_RTORCH + 5) return;
    int ax, ay, az; rsTorchAttach(id, x, y, z, ax, ay, az);
    const bool unlit = (getM(x, y, z) & 1) != 0; if (rsBlockPowered(ax, ay, az, x, y, z) != unlit) scheduleTick(x, y, z, 2);
}
static void rsTorchNotify(int x, int y, int z) {   // a tocha mudou: reavalia tochas proximas, poeira, repetidores e pistoes
    for (int dx = -2; dx <= 2; dx++) for (int dy = -2; dy <= 2; dy++) for (int dz = -2; dz <= 2; dz++) if ((dx || dy || dz) && abs(dx) + abs(dy) + abs(dz) <= 2) { const int b = getB(x + dx, y + dy, z + dz); if (b >= XB_RTORCH && b < XB_RTORCH + 5) rsTorchCheck(x + dx, y + dy, z + dz); }
    pistonScan(x, y, z);
}
static void rsDustRecalc(int x, int y, int z) {   // rede de poeira conectada: nivel 15 ao lado de uma fonte, -1 por poeira; guarda o nivel nos bits 4-7 (cor) e reavalia os pistoes vizinhos
    static bool busy = false; if (busy || getB(x, y, z) != XB_DUST) return; busy = true;
    struct C { int x, y, z, l; }; std::vector<C> cs; std::unordered_map<uint64_t, int> idx;
    auto key = [](int a, int b, int c) { return ((uint64_t)(a & 0x1FFFFF) << 42) | ((uint64_t)(b & 0x3FF) << 32) | (uint64_t)(uint32_t)c; };
    auto nbrs = [&](const C& c, std::vector<std::array<int, 3>>& o) {
        o.clear(); for (int d = 0; d < 4; d++) { const int nx = c.x + RS_DX[d], nz = c.z + RS_DZ[d], nb = getB(nx, c.y, nz);
            if (nb == XB_DUST) o.push_back({nx, c.y, nz});
            else if (nb == AIR && c.y > 0 && getB(nx, c.y - 1, nz) == XB_DUST) o.push_back({nx, c.y - 1, nz});
            if (!solid(getB(c.x, c.y + 1, c.z)) && getB(nx, c.y + 1, nz) == XB_DUST) o.push_back({nx, c.y + 1, nz}); } };
    cs.push_back({x, y, z, 0}); idx[key(x, y, z)] = 0; std::vector<std::array<int, 3>> nb;
    for (size_t i = 0; i < cs.size() && cs.size() < 600; i++) { C c = cs[i]; nbrs(c, nb); for (auto& n : nb) if (!idx.count(key(n[0], n[1], n[2]))) { idx[key(n[0], n[1], n[2])] = (int)cs.size(); cs.push_back({n[0], n[1], n[2], 0}); } }
    for (C& c : cs) for (int s = 0; s < 6; s++) { const int sx = c.x + PS_DX[s], sy = c.y + PS_DY[s], sz = c.z + PS_DZ[s]; if (rsStrongSrc(sx, sy, sz) || rsRepOutInto(sx, sy, sz, c.x, c.y, c.z) || rsStrongBlock(sx, sy, sz)) { c.l = 15; break; } }
    for (int pass = 0; pass < 20; pass++) { bool ch = false;
        for (C& c : cs) { nbrs(c, nb); for (auto& n : nb) { auto it = idx.find(key(n[0], n[1], n[2])); if (it != idx.end() && cs[it->second].l - 1 > c.l) { c.l = cs[it->second].l - 1; ch = true; } } }
        if (!ch) break; }
    std::vector<std::array<int, 3>> ps;
    for (const C& c : cs) { const int old = getM(c.x, c.y, c.z), nm = (old & 15) | (c.l << 4);
        if (nm != old) { setM(c.x, c.y, c.z, nm); netSendBlock(c.x, c.y, c.z, XB_DUST, nm); for (int s = 0; s < 6; s++) { const int b = getB(c.x + PS_DX[s], c.y + PS_DY[s], c.z + PS_DZ[s]); if (b == XB_REPT || b == XB_COMP) rsRepCheck(c.x + PS_DX[s], c.y + PS_DY[s], c.z + PS_DZ[s]); if (b >= XB_RTORCH && b < XB_RTORCH + 5) rsTorchCheck(c.x + PS_DX[s], c.y + PS_DY[s], c.z + PS_DZ[s]); if (b == XB_PIST || b == XB_PIST + 1) ps.push_back({c.x + PS_DX[s], c.y + PS_DY[s], c.z + PS_DZ[s]}); } } }
    busy = false;
    for (auto& p : ps) pistonCheck(p[0], p[1], p[2]);
}
static void pistonScan(int x, int y, int z) {   // uma fonte de energia mudou em (x,y,z): reavalia os pistoes a ate 2 blocos de distancia (taxicab)
    for (int q = 0; q < 6; q++) rsDustRecalc(x + PS_DX[q], y + PS_DY[q], z + PS_DZ[q]);
    for (int q = 0; q < 6; q++) rsRepCheck(x + PS_DX[q], y + PS_DY[q], z + PS_DZ[q]);   // poeira encostada na fonte
    for (int q = 0; q < 6; q++) { const int bx = x + PS_DX[q], by = y + PS_DY[q], bz = z + PS_DZ[q]; if (!solid(getB(bx, by, bz)) || isXB(getB(bx, by, bz))) continue;   // bloco energizado: poeira/repetidor ao lado dele
        for (int w = 0; w < 6; w++) { rsDustRecalc(bx + PS_DX[w], by + PS_DY[w], bz + PS_DZ[w]); rsRepCheck(bx + PS_DX[w], by + PS_DY[w], bz + PS_DZ[w]); } }
    for (int dx = -2; dx <= 2; dx++) for (int dy = -2; dy <= 2; dy++) for (int dz = -2; dz <= 2; dz++) if (abs(dx) + abs(dy) + abs(dz) <= 2) { const int b = getB(x + dx, y + dy, z + dz); if (b == XB_PIST || b == XB_PIST + 1) pistonCheck(x + dx, y + dy, z + dz); else if (b >= XB_RTORCH && b < XB_RTORCH + 5) rsTorchCheck(x + dx, y + dy, z + dz); }
}

static void rsInit() { memset(gRsB, 0, sizeof gRsB); for (auto& r : gRsQ) for (auto& v : r) v.clear(); }

// ---- nomes em portugues (aba Redstone); roda DEPOIS de pkApplyItemData ----
static void rsNames() {
    struct N { int blk, var; const char* n; };
    static const N T[] = {{XB_PIST, 0, "Pistão"}, {XB_PIST, 1, "Pistão pegajoso"}, {XB_DISP, 0, "Dispensador"}, {XB_DROP, 0, "Largador"}, {XB_OBS, 0, "Observador"},
        {XB_PLATE, 0, "Placa de pressão de pedra"}, {XB_PLATE, 1, "Placa de pressão de carvalho"},  {XB_PLATE, 2, "Placa de pressão leve"}, {XB_PLATE, 3, "Placa de pressão pesada"},
        {XB_BTN, 0, "Botão de pedra"}, {XB_BTN, 1, "Botão de carvalho"}, {XB_HOOK, 0, "Gancho de fio"}, {XB_CHEST, 0, "Baú"}, {XB_CHEST, 1, "Baú armadilhado"}, {XB_CHEST, 2, "Baú do Ender"}, {XB_CHEST, 3, "Caixa de shulker branca"}, {XB_CHEST, 4, "Caixa de shulker laranja"}, {XB_CHEST, 5, "Caixa de shulker magenta"}, {XB_CHEST, 6, "Caixa de shulker azul-clara"}, {XB_CHEST, 7, "Caixa de shulker amarela"}, {XB_CHEST, 8, "Caixa de shulker verde-limão"}, {XB_CHEST, 9, "Caixa de shulker rosa"}, {XB_CHEST, 10, "Caixa de shulker cinza"}, {XB_CHEST, 11, "Caixa de shulker cinza-clara"}, {XB_CHEST, 12, "Caixa de shulker ciano"}, {XB_CHEST, 13, "Caixa de shulker roxa"}, {XB_CHEST, 14, "Caixa de shulker azul"}, {XB_CHEST, 15, "Caixa de shulker marrom"}, {XB_CHEST, 16, "Caixa de shulker verde"}, {XB_CHEST, 17, "Caixa de shulker vermelha"}, {XB_CHEST, 18, "Caixa de shulker preta"},  {XB_DAY, 0, "Sensor de luz do dia"},
        {XB_HOPPER, 0, "Funil"}, {XB_DUST, 0, "Redstone"}, {XB_REPT, 0, "Repetidor de redstone"}, {XB_COMP, 0, "Comparador de redstone"},
        {XB_TRAP, 0, "Alçapão de ferro"}, {XB_TRAP, 1, "Alçapão de carvalho"}, 
        {XB_GATE, 0, "Portão de carvalho"}, {XB_GATE, 1, "Portão de abeto"}, {XB_GATE, 2, "Portão de bétula"}, {XB_GATE, 3, "Portão da selva"}, {XB_GATE, 4, "Portão de acácia"}, {XB_GATE, 5, "Portão de carvalho escuro"},
        {XB_DOOR, 0, "Porta de ferro"}, {XB_DOOR, 1, "Porta de carvalho"}, {XB_DOOR, 2, "Porta de abeto"}, {XB_DOOR, 3, "Porta de bétula"}, {XB_DOOR, 4, "Porta da selva"}, {XB_DOOR, 5, "Porta de acácia"}, {XB_DOOR, 6, "Porta de carvalho escuro"},
        {XB_SIGN, 0, "Placa de carvalho"}, };
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
