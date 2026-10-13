// ---------- blocos com modelo (nao-cubo): tocha, alavanca, bigorna, grama/flores ----------
// Os modelos vem do PROPRIO resource pack (assets/minecraft/models/block/*.json, com "parent", "textures", "elements", "rotation", "rescale", uv por face);
// a bigorna nao existe no pack, entao usa o modelo vanilla embutido abaixo (mesmos elementos/uv do Minecraft) com as texturas do pack.
// Ids fixos 900..963 (< PK0, entao os saves nao precisam de remapeamento). Ver main.cpp (XB_*).
static void netSendBlock(int x, int y, int z, int id, int meta = 0);   // net.inc (meta = metadado do bloco, ver Chunk::m)

// ---- JSON minimo ----
struct J { char t = 0; double n = 0; std::string s; std::vector<J> a; std::vector<std::pair<std::string, J>> o;
    const J* get(const char* k) const { for (auto& p : o) if (p.first == k) return &p.second; return nullptr; } };
static void jws(const char*& p) { while (*p && (unsigned char)*p <= ' ') ++p; }
static bool jparse(const char*& p, J& v) {
    jws(p);
    if (*p == '{') { v.t = 'o'; ++p; jws(p); if (*p == '}') { ++p; return true; }
        for (;;) { jws(p); J k; if (*p != '"' || !jparse(p, k)) return false; jws(p); if (*p != ':') return false; ++p;
            J val; if (!jparse(p, val)) return false; v.o.emplace_back(k.s, std::move(val)); jws(p);
            if (*p == ',') { ++p; continue; } if (*p == '}') { ++p; return true; } return false; } }
    if (*p == '[') { v.t = 'a'; ++p; jws(p); if (*p == ']') { ++p; return true; }
        for (;;) { J e; if (!jparse(p, e)) return false; v.a.push_back(std::move(e)); jws(p);
            if (*p == ',') { ++p; continue; } if (*p == ']') { ++p; return true; } return false; } }
    if (*p == '"') { v.t = 's'; ++p; while (*p && *p != '"') { if (*p == '\\' && p[1]) ++p; v.s += *p++; } if (!*p) return false; ++p; return true; }
    if (!strncmp(p, "true", 4)) { v.t = 'b'; v.n = 1; p += 4; return true; }
    if (!strncmp(p, "false", 5)) { v.t = 'b'; p += 5; return true; }
    if (!strncmp(p, "null", 4)) { p += 4; return true; }
    char* e; v.n = strtod(p, &e); if (e == p) return false; v.t = 'n'; p = e; return true;
}

// ---- modelo (com heranca "parent") ----
struct XModel { std::map<std::string, std::string> tex; std::vector<J> elems; };
static bool xbParse(const std::string& txt, XModel& m, int depth);
static bool xbLoadModel(std::string name, XModel& m, int depth = 0) {
    if (name.compare(0, 10, "minecraft:") == 0) name = name.substr(10);
    std::vector<uint8_t> d; if (!gPack.get("assets/minecraft/models/" + name + ".json", d)) return false;
    return xbParse(std::string(d.begin(), d.end()), m, depth);
}
static bool xbParse(const std::string& txt, XModel& m, int depth) {
    const char* p = txt.c_str(); if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB) p += 3;
    J root; if (!jparse(p, root)) return false;
    if (const J* par = root.get("parent")) if (depth < 8) xbLoadModel(par->s, m, depth + 1);   // pai primeiro; o filho sobrescreve
    if (const J* t = root.get("textures")) for (auto& kv : t->o) m.tex[kv.first] = kv.second.s;
    if (const J* e = root.get("elements")) m.elems = e->a;
    return true;
}
// bigorna vanilla (1.12-1.20): #body = anvil, #top = anvil_top; eixo comprido em Z
static const char* ANVIL_JSON = R"({"textures":{"body":"block/anvil","top":"block/anvil_top"},"elements":[
{"from":[2,0,2],"to":[14,4,14],"faces":{"down":{"uv":[2,2,14,14],"texture":"#body"},"up":{"uv":[2,2,14,14],"texture":"#body"},"north":{"uv":[2,12,14,16],"texture":"#body"},"south":{"uv":[2,12,14,16],"texture":"#body"},"west":{"uv":[2,12,14,16],"texture":"#body"},"east":{"uv":[2,12,14,16],"texture":"#body"}}},
{"from":[4,4,3],"to":[12,5,13],"faces":{"up":{"uv":[4,3,12,13],"texture":"#body"},"north":{"uv":[4,11,12,12],"texture":"#body"},"south":{"uv":[4,11,12,12],"texture":"#body"},"west":{"uv":[3,11,13,12],"texture":"#body"},"east":{"uv":[3,11,13,12],"texture":"#body"}}},
{"from":[6,5,4],"to":[10,10,12],"faces":{"north":{"uv":[6,6,10,11],"texture":"#body"},"south":{"uv":[6,6,10,11],"texture":"#body"},"west":{"uv":[4,6,12,11],"texture":"#body"},"east":{"uv":[4,6,12,11],"texture":"#body"}}},
{"from":[3,10,0],"to":[13,16,16],"faces":{"down":{"uv":[3,0,13,16],"texture":"#body"},"up":{"uv":[3,0,13,16],"texture":"#top"},"north":{"uv":[3,0,13,6],"texture":"#body"},"south":{"uv":[3,0,13,6],"texture":"#body"},"west":{"uv":[0,0,16,6],"texture":"#body"},"east":{"uv":[0,0,16,6],"texture":"#body"}}}]})";

// ---- texturas: tile do pack (item extra ou bloco do jogo) ou novo tile alocado no fim do atlas ----
static int gXbNext = ATL * ATLR0 - 1;
static std::unordered_map<std::string, int> gXbTex;
static std::vector<std::array<uint8_t, 1024>> gTorchPx;   // pixels de cada quadro (para animar o tile da tocha colocada no mundo)
static int gTorchAnim[5] = {-1, -1, -1, -1, -1}, gTorchAnimN = 0;   // quadros de block/torch.png (tira 16x80 animada) como tiles do atlas; usados na tocha da mao (1a/3a pessoa)
static int xbTexTile(std::string ref, const std::map<std::string, std::string>& tex) {
    for (int i = 0; i < 8 && !ref.empty() && ref[0] == '#'; i++) { auto it = tex.find(ref.substr(1)); if (it == tex.end()) return -1; ref = it->second; }
    if (ref.empty() || ref[0] == '#') return -1;
    if (ref.compare(0, 10, "minecraft:") == 0) ref = ref.substr(10);
    auto c = gXbTex.find(ref); if (c != gXbTex.end()) return c->second;
    int tile = -1;
    auto pk = gPkByFile.find(ref);
    const bool anvDmg = ref == "block/chipped_anvil_top" || ref == "block/damaged_anvil_top";   // itens nomeados do criativo reusam o tile da bigorna normal: carrega a PNG propria do topo
    if (!anvDmg && pk != gPkByFile.end() && isPk(pk->second)) tile = PK[pk->second - PK0].tile;
    else if (!anvDmg) { auto g = gPackId.find(ref); if (g != gPackId.end() && isBlk(g->second) && g->second < 256) tile = blkTile(g->second, 1); }   // ex.: cobblestone (base da alavanca)
    if (tile < 0) { uint8_t px[1024]; if (packTileRel(ref + ".png", px) && gXbNext > PKTILE0 + (int)PK.size() + 64) { tile = gXbNext--; putTile16(tile, px); } }
    gXbTex[ref] = tile; return tile;
}

// ---- geometria ----
struct XQuad { float p[4][3]; float u[4], v[4]; int tile; bool tint, shade; float tc[3] = {1, 1, 1}; };   // tc = cor fixa (poeira de redstone)
static const std::vector<XQuad>& rsQuads(int b, int meta);   // redstone.inc   // p em unidades de bloco (0..1); ordem BL,BR,TR,TL; u/v em 0..1 dentro do tile
static const std::vector<XQuad>& rsStairQuadsAt(int x, int y, int z, int meta);   // redstone.inc: escada com cantos
static std::vector<XQuad> gXM[XB_END - XB0];
static std::vector<XQuad> gRtOff[5];   // tocha de redstone APAGADA (meta bit0 = apagada; redstone.inc faz a logica)
static std::vector<XQuad> gEndPortalQ;   // portal do End (nether.inc): camadas horizontais com paralaxe
static std::vector<XQuad> gPortalZ;   // portal com plano em Z (nether.inc); o de plano em X fica em gXM[XB_PORTAL]
static std::vector<XQuad> gSfHead;   // cabeca do girassol (2 quads: frente e costas, pose de repouso do pack), desenhada a cada quadro girando para o sol (drawSunflowers)
static std::map<std::tuple<int, int, int>, char> gSfPos;   // girassois (metade de cima) vistos pelo mesher; validado a cada quadro
static std::vector<XQuad> gRailQ[4][20];   // malhas dos trilhos por tipo e (forma + 10*ligado); montadas em railInit (minecart.inc)
static void railInit();
static int gXbDrop[XB_END - XB0];
static std::vector<XQuad> gAnvQ[3][4];   // bigorna: [dano 0 normal/1 rachada/2 quebrada][rotacao]
static std::unordered_map<int, int> gXbVar;   // item -> variante (dano da bigorna)
static int gAnvDrop[3];   // dano -> item que dropa
static int gXbPlaceVar = 0;   // variante do item sendo colocado (main_loop)   // bloco -> item (PK) que dropa (0 = nada)
static void xRot(float* v, int axis, float deg) {   // rotacao destra padrao (como o "rotation" dos modelos do Minecraft)
    const float a = deg * .0174533f, c = cosf(a), s = sinf(a), x = v[0], y = v[1], z = v[2];
    if (axis == 0) { v[1] = y * c - z * s; v[2] = y * s + z * c; }
    else if (axis == 1) { v[0] = x * c + z * s; v[2] = -x * s + z * c; }
    else { v[0] = x * c - y * s; v[1] = x * s + y * c; }
}
static void xbBake(const XModel& m, int bsx, int bsy, bool tint, std::vector<XQuad>& out) {   // bsx/bsy = rotacao do blockstate (x depois y; sentido horario => angulo negativo)
    static const char* FN[6] = {"east", "west", "up", "down", "south", "north"};
    for (const J& el : m.elems) {
        const J *fr = el.get("from"), *to = el.get("to"), *fc = el.get("faces");
        if (!fr || !to || !fc || fr->a.size() < 3 || to->a.size() < 3) continue;
        float A[3], B[3]; for (int i = 0; i < 3; i++) { A[i] = (float)fr->a[i].n; B[i] = (float)to->a[i].n; if (A[i] > B[i]) std::swap(A[i], B[i]); }   // "from" > "to" (ex.: base interna da haste do End): sem normalizar, as faces saem viradas ao contrario e somem (buraco no modelo)
        bool shade = true; if (const J* s = el.get("shade")) shade = s->n != 0;
        int rax = -1; float rang = 0, org[3] = {8, 8, 8}; bool resc = false;
        if (const J* r = el.get("rotation")) {
            const J *ax = r->get("axis"), *an = r->get("angle"), *o = r->get("origin"), *rs = r->get("rescale");
            if (ax && an) { rax = ax->s == "x" ? 0 : ax->s == "y" ? 1 : 2; rang = (float)an->n; }
            if (o && o->a.size() >= 3) for (int i = 0; i < 3; i++) org[i] = (float)o->a[i].n;
            resc = rs && rs->n != 0;
        }
        for (int f = 0; f < 6; f++) {
            const J* face = fc->get(FN[f]); if (!face) continue;
            const J* tx = face->get("texture"); if (!tx) continue;
            const int tile = xbTexTile(tx->s, m.tex); if (tile < 0) continue;
            const float x1 = A[0], y1 = A[1], z1 = A[2], x2 = B[0], y2 = B[1], z2 = B[2];
            float C[4][3];   // TL, TR, BR, BL vistos de fora
            const float tab[6][4][3] = {
                {{x2,y2,z2},{x2,y2,z1},{x2,y1,z1},{x2,y1,z2}},   // east
                {{x1,y2,z1},{x1,y2,z2},{x1,y1,z2},{x1,y1,z1}},   // west
                {{x1,y2,z1},{x2,y2,z1},{x2,y2,z2},{x1,y2,z2}},   // up
                {{x1,y1,z2},{x2,y1,z2},{x2,y1,z1},{x1,y1,z1}},   // down
                {{x1,y2,z2},{x2,y2,z2},{x2,y1,z2},{x1,y1,z2}},   // south
                {{x2,y2,z1},{x1,y2,z1},{x1,y1,z1},{x2,y1,z1}}};  // north
            memcpy(C, tab[f], sizeof C);
            float uv[4] = {0, 0, 16, 16};
            if (const J* u = face->get("uv")) { if (u->a.size() >= 4) for (int i = 0; i < 4; i++) uv[i] = (float)u->a[i].n; }
            else {   // UV automatica identica a do Minecraft (BlockPart.getFaceUvs): caixas parciais (degrau de escada, porta) pegam o trecho certo da textura, sem espelhar
                switch (f) {
                    case 0: uv[0] = 16 - z2; uv[1] = 16 - y2; uv[2] = 16 - z1; uv[3] = 16 - y1; break;   // east
                    case 1: uv[0] = z1; uv[1] = 16 - y2; uv[2] = z2; uv[3] = 16 - y1; break;             // west
                    case 2: uv[0] = x1; uv[1] = z1; uv[2] = x2; uv[3] = z2; break;                       // up
                    case 3: uv[0] = x1; uv[1] = 16 - z2; uv[2] = x2; uv[3] = 16 - z1; break;             // down
                    case 4: uv[0] = x1; uv[1] = 16 - y2; uv[2] = x2; uv[3] = 16 - y1; break;             // south
                    default: uv[0] = 16 - x2; uv[1] = 16 - y2; uv[2] = 16 - x1; uv[3] = 16 - y1; break;  // north
                }
            }
            const float cu[4] = {uv[0], uv[2], uv[2], uv[0]}, cv[4] = {uv[1], uv[1], uv[3], uv[3]};   // TL, TR, BR, BL
            int rot = 0; if (const J* r = face->get("rotation")) rot = ((int)r->n / 90) & 3;
            XQuad q; q.tile = tile; q.tint = tint; q.shade = shade;
            const int ord[4] = {3, 2, 1, 0};   // saida BL, BR, TR, TL
            for (int k = 0; k < 4; k++) {
                const int ci = ord[k], ui = (ci + rot) & 3;
                float p[3] = {C[ci][0], C[ci][1], C[ci][2]};
                if (rax >= 0) {
                    for (int i = 0; i < 3; i++) p[i] -= org[i];
                    xRot(p, rax, rang);
                    if (resc) { const float sc = 1.f / std::max(.2f, cosf(fabsf(rang) * .0174533f)); for (int i = 0; i < 3; i++) if (i != rax) p[i] *= sc; }
                    for (int i = 0; i < 3; i++) p[i] += org[i];
                }
                for (int i = 0; i < 3; i++) p[i] -= 8;
                if (bsx) xRot(p, 0, (float)-bsx);
                if (bsy) xRot(p, 1, (float)-bsy);
                for (int i = 0; i < 3; i++) q.p[k][i] = (p[i] + 8) / 16.f;
                q.u[k] = cu[ui] / 16.f; q.v[k] = cv[ui] / 16.f;
            }
            out.push_back(q);
        }
    }
    // CAUSA DO FLICKERING DO MATO/FLORES: o modelo block/cross do Minecraft tem 2 faces (north+south) por plano; sem culling elas viram 2 quads
    // COPLANARES identicos, que brigam no depth test (z-fighting) e ainda alternam o sombreamento de lado. Mantem 1 quad por plano (desenhado dos 2 lados).
    auto sameQuad = [](const XQuad& a, const XQuad& b) {
        if (a.tile != b.tile) return false;
        for (int i = 0; i < 4; i++) { bool f = false; for (int j = 0; j < 4 && !f; j++) f = fabsf(a.p[i][0] - b.p[j][0]) < 1e-4f && fabsf(a.p[i][1] - b.p[j][1]) < 1e-4f && fabsf(a.p[i][2] - b.p[j][2]) < 1e-4f; if (!f) return false; }
        return true;
    };
    for (size_t i = 0; i < out.size(); i++)
        for (size_t j = out.size(); j-- > i + 1;) if (sameQuad(out[i], out[j])) out.erase(out.begin() + j);
}

// ---- tabelas ----
static const char* XB_PLANTS[24] = {"short_grass|grass", "fern", "dandelion", "poppy", "blue_orchid", "allium", "azure_bluet", "red_tulip", "orange_tulip", "white_tulip", "pink_tulip", "oxeye_daisy", "cornflower", "lily_of_the_valley",
    "tall_grass_bottom", "tall_grass_top", "large_fern_bottom", "large_fern_top", "sunflower_bottom", "sunflower_top", "lilac_bottom", "lilac_top", "rose_bush_bottom", "rose_bush_top"};
static std::unordered_map<int, int> gXbItem;   // item (PK) -> bloco base
static inline int xbItemBlock(int item) {
    auto it = gXbItem.find(item); if (it != gXbItem.end()) return it->second;
    if (item >= PK0 && (size_t)(item - PK0) < PK.size()) { const std::string& f = PK[item - PK0].file;   // tabela pode estar velha (pack recarregado): confere pelo nome
        if (f == "block/torch") return XB_TORCH; if (f == "block/redstone_torch") return XB_RTORCH; if (f == "block/lever") return XB_LEVER; }
    return 0; }
static int entSprite(int kind, int color);   // entmodels.inc
static int gBnDrop[16];   // cor -> item estandarte
static inline int xbDropItem(int b, int meta = 0) {
    if (!isXB(b)) return 0;
    if (isBanner(b)) return gBnDrop[(meta >> 4) & 15];
    if (b >= XB_ANVIL && b < XB_ANVIL + 4) { const int dm = std::min(meta & 3, 2); if (gAnvDrop[dm]) return gAnvDrop[dm]; } const int d = gXbDrop[b - XB0]; if (d) return d;
    const char* f = (b >= XB_TORCH && b < XB_TORCH + 5) ? "block/torch" : (b >= XB_RTORCH && b < XB_RTORCH + 5) ? "block/redstone_torch" : nullptr;
    if (f) { auto it = gPkByFile.find(f); if (it != gPkByFile.end()) return it->second; }
    return d; }
static inline int xbTile(int b) { if (isRs(b)) return rsTile(b); if (isBanner(b)) { const int t = entSprite(1, 0); return t >= 0 ? t : 0; } return isXB(b) && !gXM[b - XB0].empty() ? gXM[b - XB0][0].tile : 0; }
static inline bool xbIsRail(int b) { return b >= XB_RAIL && b < XB_RAIL + 4; }
static inline int railQI(int m) { return std::min(m & 15, 9) + ((m >> 4) & 1) * 10; }   // indice da malha do trilho: forma + 10 se ligado
static inline bool xbIsLever(int b) { return b >= XB_LEVER && b < XB_LEVER + 24; }
static inline bool xbIsTorch(int b) { return (b >= XB_TORCH && b < XB_TORCH + 5) || (b >= XB_RTORCH && b < XB_RTORCH + 5); }
static inline bool xbIsPlant(int b) { return b >= XB_PLANT && b < XB_PLANT + 24; }
static const char* XB_SAPL[6] = {"oak_sapling", "spruce_sapling", "birch_sapling", "jungle_sapling", "acacia_sapling", "dark_oak_sapling"};
static inline bool xbIsSapling(int b) { return b >= XB_SAPLING && b < XB_SAPLING + 6; }
// solo onde a muda pega (BlockSapling/canSustainPlant do 1.12): grama, terra, terra grossa, podzol (e terra arada/micelio no pack)
static bool xbSoilOk(int b) { if (b == GRASS || b == DIRT) return true; if (!isPk(b)) return false; const std::string& f = PK[b - PK0].file; return f == "block/podzol" || f == "block/coarse_dirt" || f == "block/grass_block" || f == "block/dirt" || f == "block/mycelium" || f == "block/rooted_dirt" || f == "block/moss_block"; }
static inline bool xbIsUpper(int b) { return xbIsPlant(b) && (b - XB_PLANT) >= 14 && ((b - XB_PLANT) & 1) != 0; }   // metade de cima das plantas altas
static inline bool xbIsLower(int b) { return xbIsPlant(b) && (b - XB_PLANT) >= 14 && ((b - XB_PLANT) & 1) == 0; }
static inline bool xbReplaceable(int b) { return xbIsPlant(b) && ((b - XB_PLANT) < 2 || (b - XB_PLANT) == 14 || (b - XB_PLANT) == 16); }   // grama/samambaia (e as altas) cedem lugar ao colocar
static float xbHard(int b) { if (isRs(b)) return rsHard(b); if (isBanner(b)) return 1.f; return xbIsRail(b) ? .7f : xbIsLever(b) ? .5f : (b >= XB_ANVIL && b < XB_ANVIL + 4) ? 5.f : 0.f; }
static int xbTool(int b) { if (isRs(b)) return rsTool(b); if (isBanner(b)) return 2; return (b >= XB_ANVIL && b < XB_ANVIL + 4) ? 1 : 0; }
static int xbLevel(int b) { if (isRs(b)) return rsTool(b) == 1 ? 0 : -1; return (b >= XB_ANVIL && b < XB_ANVIL + 4) ? 0 : -1; }

static void xbInit() {
    for (auto& v : gXM) v.clear(); gXbItem.clear(); gXbTex.clear(); gXbNext = ATL * ATLR0 - 1; memset(gXbDrop, 0, sizeof gXbDrop); gXbVar.clear(); memset(gAnvDrop, 0, sizeof gAnvDrop); for (auto& a : gAnvQ) for (auto& v : a) v.clear();
    if (!gPack.ok) return;
    { std::map<std::string, std::string> none; gLavaTile = xbTexTile("block/lava_still", none); }   // lava: 1o quadro da textura animada do pack
    XModel m;
    // tocha em pe + 4 de parede (y do blockstate: 0 leste, 90 sul, 180 oeste, 270 norte; preso no bloco oposto)
    if (xbLoadModel("block/torch", m)) xbBake(m, 0, 0, false, gXM[XB_TORCH - XB0]);
    gTorchAnimN = 0; gTorchPx.clear(); for (int& t : gTorchAnim) t = -1;
    { std::vector<uint8_t> px; int w = 0, h = 0;   // textura animada real do pack: corta cada quadro wxw da tira vertical
        if (packRGBA("block/torch.png", px, w, h) && w >= 1 && h >= 2 * w) {
            const int n = std::min(h / w, 5); int ok = 0;
            for (int k = 0; k < n && gXbNext > PKTILE0 + (int)PK.size() + 64; k++) {
                std::vector<uint8_t> f((size_t)w * w * 4); memcpy(&f[0], &px[(size_t)k * w * w * 4], (size_t)w * w * 4);
                uint8_t o[1024]; scaleRGBA(f, w, w, o, 16, 16); gTorchAnim[k] = gXbNext--; putTile16(gTorchAnim[k], o); std::array<uint8_t, 1024> a; memcpy(a.data(), o, 1024); gTorchPx.push_back(a); ok++;
            }
            gTorchAnimN = ok == n ? n : 0; if (!gTorchAnimN) gTorchPx.clear();
        } }
    m = XModel();
    if (xbLoadModel("block/wall_torch", m)) for (int k = 0; k < 4; k++) xbBake(m, 0, 90 * k, false, gXM[XB_TORCH + 1 + k - XB0]);
    if (xbLoadModel("block/redstone_torch", m)) xbBake(m, 0, 0, false, gXM[XB_RTORCH - XB0]);   // tocha de redstone: em pe + 4 de parede
    if (xbLoadModel("block/redstone_wall_torch", m)) for (int k = 0; k < 4; k++) xbBake(m, 0, 90 * k, false, gXM[XB_RTORCH + 1 + k - XB0]);
    for (int v = 0; v < 2; v++) { XModel om; if (xbLoadModel(v ? "block/redstone_wall_torch" : "block/redstone_torch", om)) {   // apagada: mesma forma, textura redstone_torch_off
        for (auto& kv : om.tex) if (kv.second.find("redstone_torch") != std::string::npos && kv.second.find("_off") == std::string::npos) kv.second += "_off";
        if (v == 0) xbBake(om, 0, 0, false, gRtOff[0]); else for (int k = 0; k < 4; k++) xbBake(om, 0, 90 * k, false, gRtOff[1 + k]); } }
    // alavanca: estados 0..3 parede (N,E,S,W), 4..7 chao, 8..11 teto; +12 = ligada (lever_on)
    for (int on = 0; on < 2; on++) {
        m = XModel(); if (!xbLoadModel(on ? "block/lever_on" : "block/lever", m)) continue;
        for (int s = 0; s < 12; s++) {
            const int f = s & 3, face = s >> 2; int bx = 0, by = 0;
            if (face == 0) { bx = 90; by = 90 * f; } else if (face == 1) { bx = 0; by = 90 * f; } else { bx = 180; by = (180 + 90 * f) % 360; }
            xbBake(m, bx, by, false, gXM[XB_LEVER + on * 12 + s - XB0]);
        }
    }
    // bigorna (modelo vanilla embutido; 4 rotacoes)
    { XModel am; xbParse(ANVIL_JSON, am, 0); for (int k = 0; k < 4; k++) xbBake(am, 0, 90 * k, false, gXM[XB_ANVIL + k - XB0]); }
    for (int dm = 0; dm < 3; dm++) {   // bigorna normal / rachada / quebrada: so a textura do topo muda (anvil_top, chipped_anvil_top, damaged_anvil_top)
        std::string js = ANVIL_JSON; if (dm) { const std::string o = "block/anvil_top"; js.replace(js.find(o), o.size(), dm == 1 ? "block/chipped_anvil_top" : "block/damaged_anvil_top"); }
        XModel dmod; if (xbParse(js, dmod, 0)) for (int k = 0; k < 4; k++) xbBake(dmod, 0, 90 * k, false, gAnvQ[dm][k]);
    }
    // plantas em "cross" (modelo block/cross do pack, textura trocada); grama/samambaia tingidas pelo bioma
    for (int i = 0; i < 24; i++) {
        XModel cm; if (!xbLoadModel("block/cross", cm)) continue;
        std::string names = XB_PLANTS[i], pick; size_t st = 0;
        while (st <= names.size()) { size_t e = names.find('|', st); if (e == std::string::npos) e = names.size(); const std::string n = names.substr(st, e - st); std::vector<uint8_t> tmp;
            if (pick.empty() && gPack.get("assets/minecraft/textures/block/" + n + ".png", tmp)) pick = n; st = e + 1; }
        if (pick.empty()) continue;
        cm.tex["cross"] = "block/" + pick;
        const bool tint = i < 2 || (i >= 14 && i < 18);
        xbBake(cm, 0, 0, tint, gXM[XB_PLANT + i - XB0]);
    }
    // girassol: metade de cima usa o modelo do pack (haste + flor amarela com front/back), nao o "cross" simples
    { XModel sm; if (xbLoadModel("block/sunflower_top", sm) || xbLoadModel("block/sunflower_top1", sm) || xbLoadModel("block/sunflower_top2", sm)) { std::vector<XQuad> tmp; xbBake(sm, 0, 0, false, tmp); gSfHead.clear();
        if (tmp.size() >= 4) { gSfHead.assign(tmp.end() - 2, tmp.end()); tmp.resize(tmp.size() - 2); }   // xbBake funde north+south iguais: haste = 2 quads (1 por plano cruzado); cabeca = elemento final (east=frente, west=costas)
        gXM[XB_PLANT + 19 - XB0] = tmp; } }
    // mudas: mesmo modelo cross, textura block/<especie>_sapling (sem tinta)
    for (int i = 0; i < 6; i++) { XModel cm; if (!xbLoadModel("block/cross", cm)) continue; std::vector<uint8_t> tmp; if (!gPack.get(std::string("assets/minecraft/textures/block/") + XB_SAPL[i] + ".png", tmp)) continue;
        cm.tex["cross"] = std::string("block/") + XB_SAPL[i]; xbBake(cm, 0, 0, false, gXM[XB_SAPLING + i - XB0]); }
    // itens do pack (PK) -> bloco base; e o inverso (drops)
    auto add = [&](const std::string& f, int blk) { auto it = gPkByFile.find("block/" + f); if (it == gPkByFile.end()) return; gXbItem[it->second] = blk; };
    add("torch", XB_TORCH); add("redstone_torch", XB_RTORCH); add("lever", XB_LEVER); add("anvil", XB_ANVIL);
    for (int i = 0; i < 6; i++) { add(XB_SAPL[i], XB_SAPLING + i); auto it = gPkByFile.find(std::string("block/") + XB_SAPL[i]); if (it != gPkByFile.end()) gXbDrop[XB_SAPLING + i - XB0] = it->second; }
    const int tor = gPkByFile.count("block/torch") ? gPkByFile["block/torch"] : 0, lev = gPkByFile.count("block/lever") ? gPkByFile["block/lever"] : 0, anv = gPkByFile.count("block/anvil") ? gPkByFile["block/anvil"] : 0;
    for (int k = 0; k < 5; k++) gXbDrop[XB_TORCH + k - XB0] = tor;
    { const int rt = gPkByFile.count("block/redstone_torch") ? gPkByFile["block/redstone_torch"] : 0; for (int k = 0; k < 5; k++) gXbDrop[XB_RTORCH + k - XB0] = rt; }
    for (int k = 0; k < 24; k++) gXbDrop[XB_LEVER + k - XB0] = lev;
    for (int k = 0; k < 4; k++) gXbDrop[XB_ANVIL + k - XB0] = anv;
    gAnvDrop[0] = anv;
    for (int i = 2; i < 14; i++) { add(XB_PLANTS[i], XB_PLANT + i); auto it = gPkByFile.find(std::string("block/") + XB_PLANTS[i]); if (it != gPkByFile.end()) gXbDrop[XB_PLANT + i - XB0] = it->second; }
    { std::string a = XB_PLANTS[0]; const size_t bar = a.find('|'); add(a.substr(0, bar), XB_PLANT); if (bar != std::string::npos) add(a.substr(bar + 1), XB_PLANT); add("fern", XB_PLANT + 1); }
    static const int DBL[5] = {14, 16, 18, 20, 22};   // item = textura "_top"; solta so as de flor (girassol, lilas, roseira)
    for (int d = 0; d < 5; d++) { const std::string top = XB_PLANTS[DBL[d] + 1]; add(top, XB_PLANT + DBL[d]);
        auto it = gPkByFile.find("block/" + top); if (it != gPkByFile.end() && d >= 2) { gXbDrop[XB_PLANT + DBL[d] - XB0] = it->second; } }
    railInit(); rsInit();
    printf("Modelos: %d tiles extras, itens colocaveis (modelo): %d\n", ATL * ATLR0 - 1 - gXbNext, (int)gXbItem.size());
}

// ---- malha (chamada por build() em world.inc) ----
static const std::vector<XQuad>& bnQuads(int b, int meta);   // entmodels.inc
static void emitXB(Chunk* c, int b, int x, int y, int z, int wx, int wz) {
    const std::vector<XQuad>& qs = (b >= XB_RTORCH && b < XB_RTORCH + 5 && (c->m[idx(x, y, z)] & 1) && !gRtOff[b - XB_RTORCH].empty()) ? gRtOff[b - XB_RTORCH] : (b == XB_PORTAL && (c->m[idx(x, y, z)] & 2)) ? gEndPortalQ : (b == XB_PORTAL && (c->m[idx(x, y, z)] & 1)) ? gPortalZ : xbIsRail(b) ? gRailQ[b - XB_RAIL][railQI(c->m[idx(x, y, z)])] : isRs(b) ? (b == XB_STAIR ? rsStairQuadsAt(wx, y, wz, c->m[idx(x, y, z)]) : rsQuads(b, c->m[idx(x, y, z)])) : isBanner(b) ? bnQuads(b, c->m[idx(x, y, z)]) : (b >= XB_ANVIL && b < XB_ANVIL + 4) ? gAnvQ[std::min(c->m[idx(x, y, z)] & 3, 2)][b - XB_ANVIL] : gXM[b - XB0]; if (qs.empty()) return;
    if (b == XB_PLANT + 19 && !gSfHead.empty()) gSfPos[std::make_tuple(wx, y, wz)] = 1;   // a cabeca nao entra na malha: e desenhada girando
    std::vector<V>& dst = ((b == XB_CPANE || (b == XB_PORTAL && !(c->m[idx(x, y, z)] & 2))) && gXbGlass)   /* portal do End: malha opaca (a passada translucida usa o shader da agua e apagava a textura) */ ? *gXbGlass : c->mesh;   // painel colorido: passe translucido
    const uint8_t Lc = c->lc[idx(x, y, z)]; const int sk = Lc >> 4, bk = Lc & 15;
    const float sf = ltab((float)sk), bf = bk > 0 ? ltabB((float)bk) : 0.f; const int cc = regionChroma(x, y, z); const float pk = packLight((float)sk, (float)bk, cc);
    float tt[3] = {1, 1, 1}, tl[3]; bool haveT = false;
    // laje/escada: a face de baixo (plano y=0) / de cima (plano y=1) encostada num bloco inteiro brigava com a face dele (z-fighting = topo e base piscando/ficando errados); some como no MC
    const bool slabLike = isRs(b) && (b - XB_DUST == 26 || b - XB_DUST == 27);
    const bool oBelow = slabLike && y > 0 && lightOpac(getB(wx, y - 1, wz)) >= 15, oAbove = slabLike && y < CH - 1 && lightOpac(getB(wx, y + 1, wz)) >= 15;
    for (const XQuad& q : qs) {
        if (slabLike && (oBelow || oAbove)) {
            const float py = q.p[0][1];
            if (fabsf(q.p[1][1] - py) < 1e-4f && fabsf(q.p[2][1] - py) < 1e-4f && fabsf(q.p[3][1] - py) < 1e-4f && (py < 1e-4f || py > 1.f - 1e-4f)) {
                const float ny = (q.p[1][2] - q.p[0][2]) * (q.p[3][0] - q.p[0][0]) - (q.p[1][0] - q.p[0][0]) * (q.p[3][2] - q.p[0][2]);
                if ((py < 1e-4f && ny < 0 && oBelow) || (py > 1.f - 1e-4f && ny > 0 && oAbove)) continue;
            }
        }
        float m = 1.f;
        if (q.shade) {   // sombreamento por direcao da face (cima 1, baixo .5, lados .8/.65)
            float e1[3], e2[3], n[3]; for (int i = 0; i < 3; i++) { e1[i] = q.p[1][i] - q.p[0][i]; e2[i] = q.p[3][i] - q.p[0][i]; }
            n[0] = e1[1] * e2[2] - e1[2] * e2[1]; n[1] = e1[2] * e2[0] - e1[0] * e2[2]; n[2] = e1[0] * e2[1] - e1[1] * e2[0];
            const float l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]); if (l > 1e-6f) { for (int i = 0; i < 3; i++) n[i] /= l; }
            m = n[1] > 0 ? lerpf(.8f, 1.f, fabsf(n[1])) : lerpf(.8f, .5f, fabsf(n[1])); if (fabsf(n[1]) < .5f) m = fabsf(n[0]) > fabsf(n[2]) ? .7f : .8f;
        }
        float r, g, bb, a = 1.f;
        if (gShaderOK) { r = g = bb = m; a = pk; }
        else { float ch[3]; ccRGB(cc, ch); r = std::max(sf, bf * ch[0]) * m; g = std::max(sf, bf * ch[1]) * m; bb = std::max(sf, bf * ch[2]) * m; }
        r *= q.tc[0]; g *= q.tc[1]; bb *= q.tc[2];
        if (q.tint) { if (!haveT) { biomeTints(wx + .5f, wz + .5f, tt, tl); haveT = true; } r *= tt[0]; g *= tt[1]; bb *= tt[2]; }
        const int tx = q.tile % ATL, ty = q.tile / ATL;
        for (int k = 0; k < 4; k++)
            dst.push_back({wx + q.p[k][0], y + q.p[k][1], wz + q.p[k][2], (tx + q.u[k]) / (float)ATL, (ty + q.v[k]) / (float)ATLR, r, g, bb, a});
    }
}

// Girassol: a cabeca segue o sol (leste ao amanhecer = pose do pack, para cima ao meio-dia, oeste ao por do sol) e volta devagar para o leste durante a noite.
// Frente e costas ficam no mesmo plano: com culling ligado so a face voltada para a camera aparece (antes as duas brigavam e xadreziam a textura).
static void drawSunflowers(float pt, double cx, double cz) {
    if (gSfPos.empty() || gSfHead.empty()) return;
    const float a = celestialAngle(gWorldTime, pt) * 360.f, ad = a > 180.f ? a - 360.f : a;   // -180..180; 0 = meio-dia, +90 = por do sol
    float tilt;   // graus: 22.5 = leste (pose do pack), 90 = para cima, 157.5 = oeste
    if (fabsf(ad) <= 90.f) tilt = std::max(22.5f, std::min(157.5f, 90.f + ad));
    else { const float u = (ad > 0.f ? ad - 90.f : ad + 270.f) / 180.f, s = u * u * (3.f - 2.f * u); tilt = 157.5f - 135.f * s; }
    const float da = (tilt - 22.5f) * .0174533f, cs = cosf(da), sn = sinf(da);
    glEnable(GL_CULL_FACE); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f);
    for (auto it = gSfPos.begin(); it != gSfPos.end();) {
        const int x = std::get<0>(it->first), y = std::get<1>(it->first), z = std::get<2>(it->first);
        if (getB(x, y, z) != XB_PLANT + 19) { it = gSfPos.erase(it); continue; }   // quebrado ou chunk descarregado (o mesher registra de novo ao recarregar)
        ++it;
        const double dx = x + .5 - cx, dz = z + .5 - cz; if (dx * dx + dz * dz > 96.0 * 96.0) continue;
        entLightAt(x + .5, y + .5, z + .5, gEntDim); glColor3f(gEntDim[0], gEntDim[1], gEntDim[2]);
        glBegin(GL_QUADS);
        for (const XQuad& q : gSfHead) for (int k = 0; k < 4; k++) {
            const float px = q.p[k][0] - .5f, py = q.p[k][1] - .5f;   // gira em torno do topo da haste (centro do bloco, como o pivo do modelo)
            glTexCoord2f((q.tile % ATL + q.u[k]) / (float)ATL, (q.tile / ATL + q.v[k]) / (float)ATLR);
            glVertex3f(x + .5f + px * cs - py * sn, y + .5f + px * sn + py * cs, z + q.p[k][2]);
        }
        glEnd();
    }
    entDimReset(); glDisable(GL_CULL_FACE);
}

// ---- colocar / suporte ----
static inline bool xbFree(int b) { return b == AIR || xbReplaceable(b); }
static bool xbPlace(int xb, const int* hh, const int* pp, float yawDeg, double px, double py, double pz) {
    int t[3] = {pp[0], pp[1], pp[2]};
    const int hb = getB(hh[0], hh[1], hh[2]);
    if (isXB(hb) && xbReplaceable(hb)) { t[0] = hh[0]; t[1] = hh[1]; t[2] = hh[2]; }   // substitui a grama/samambaia mirada
    int n[3] = {t[0] - hh[0], t[1] - hh[1], t[2] - hh[2]};
    if (abs(n[0]) + abs(n[1]) + abs(n[2]) != 1) { n[0] = 0; n[1] = 1; n[2] = 0; }
    if (!xbFree(getB(t[0], t[1], t[2]))) return false;
    const float dx = -sinf(yawDeg * .0174533f), dz = -cosf(yawDeg * .0174533f);
    const int pf = fabsf(dx) > fabsf(dz) ? (dx > 0 ? 1 : 3) : (dz > 0 ? 2 : 0);   // 0 N, 1 L, 2 S, 3 O (direcao em que o jogador olha)
    const int below = getB(t[0], t[1] - 1, t[2]);
    int id = 0, id2 = 0, bmeta = 0;
    if (xb == XB_TORCH || xb == XB_RTORCH) {
        if (n[1] == 1) { if (!solid(below)) return false; id = xb; }
        else if (n[1] == 0) { if (!solid(getB(hh[0], hh[1], hh[2]))) return false; id = xb + (n[0] == 1 ? 1 : n[2] == 1 ? 2 : n[0] == -1 ? 3 : 4); }
        else return false;
    } else if (xb == XB_LEVER) {
        if (!solid(getB(hh[0], hh[1], hh[2]))) return false;
        if (n[1] == 0) id = XB_LEVER + (n[2] == -1 ? 0 : n[0] == 1 ? 1 : n[2] == 1 ? 2 : 3);
        else id = XB_LEVER + (n[1] == 1 ? 4 : 8) + pf;
    } else if (xb == XB_ANVIL) {
        static const int AK[4] = {1, 0, 1, 0};   // olhando em Z o eixo comprido vai em X (rotacao 90) e vice-versa
        id = XB_ANVIL + AK[pf] + (pf >= 2 ? 2 : 0);
    } else if (xb == XB_BANNER) {   // estandarte: em pe (topo de bloco solido, 16 rotacoes) ou na parede (face lateral solida)
        const int col = std::min(std::max(gXbPlaceVar, 0), 15);
        if (n[1] == 1) { if (!solid(below)) return false; id = XB_BANNER; bmeta = (((int)floorf(-yawDeg / 22.5f + .5f)) & 15) | (col << 4); }
        else if (n[1] == 0) { if (!solid(getB(hh[0], hh[1], hh[2]))) return false; id = XB_WBANNER; bmeta = (n[2] == -1 ? 0 : n[0] == 1 ? 1 : n[2] == 1 ? 2 : 3) | (col << 4); }
        else return false;
    } else if (xbIsSapling(xb)) {
        if (!xbSoilOk(below) || isWater(getB(t[0], t[1], t[2]))) return false;   // so em terra/grama/podzol, e nao dentro d'agua
        id = xb;
    } else if (xbIsPlant(xb)) {
        if (!solid(below) || isWater(getB(t[0], t[1], t[2]))) return false;
        id = xb;
        if (xb - XB_PLANT >= 14) { if (t[1] + 1 >= CH || !xbFree(getB(t[0], t[1] + 1, t[2]))) return false; id2 = xb + 1; }
    }
    if (!id) return false;
    if (id >= XB_ANVIL && id < XB_ANVIL + 4 && t[0] + 1 > px - .3 && t[0] < px + .3 && t[2] + 1 > pz - .3 && t[2] < pz + .3 && t[1] + 1 > py && t[1] < py + 1.8) return false;
    const int am = isBanner(id) ? bmeta : xb == XB_ANVIL ? std::min(std::max(gXbPlaceVar, 0), 2) : 0; setBM(t[0], t[1], t[2], id, am); netSendBlock(t[0], t[1], t[2], id, am);
    if (id2) { setB(t[0], t[1] + 1, t[2], id2); netSendBlock(t[0], t[1] + 1, t[2], id2); }
    return true;
}
static bool xbSupported(int b, int x, int y, int z) {
    if (isRs(b)) return rsSupported(b, x, y, z);
    if (b == XB_TORCH || b == XB_RTORCH) return solid(getB(x, y - 1, z));
    if ((b > XB_TORCH && b <= XB_TORCH + 4) || (b > XB_RTORCH && b <= XB_RTORCH + 4)) { static const int DX[4] = {-1, 0, 1, 0}, DZ[4] = {0, -1, 0, 1}; const int k = (b - XB_TORCH <= 4 ? b - XB_TORCH : b - XB_RTORCH) - 1; return solid(getB(x + DX[k], y, z + DZ[k])); }
    if (xbIsLever(b)) { const int s = (b - XB_LEVER) % 12, face = s >> 2, f = s & 3;
        if (face == 1) return solid(getB(x, y - 1, z)); if (face == 2) return solid(getB(x, y + 1, z));
        static const int DX[4] = {0, -1, 0, 1}, DZ[4] = {1, 0, -1, 0};   // N: preso ao sul; L: ao oeste; S: ao norte; O: ao leste
        return solid(getB(x + DX[f], y, z + DZ[f])); }
    if (b == XB_BANNER) return solid(getB(x, y - 1, z));
    if (b == XB_WBANNER) { static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0}; const int d = getM(x, y, z) & 3; return solid(getB(x - DX[d], y, z - DZ[d])); }   // preso ao bloco oposto a frente
    if (xbIsRail(b)) return solid(getB(x, y - 1, z));
    if (xbIsUpper(b)) return getB(x, y - 1, z) == b - 1;
    if (xbIsSapling(b)) return xbSoilOk(getB(x, y - 1, z));
    if (xbIsPlant(b)) return solid(getB(x, y - 1, z));
    return true;
}
static void pistonScan(int x, int y, int z);
static bool xbToggleLever(int x, int y, int z) {
    const int b = getB(x, y, z); if (!xbIsLever(b)) return false;
    const int nb = XB_LEVER + ((b - XB_LEVER + 12) % 24); setB(x, y, z, nb); netSendBlock(x, y, z, nb); pistonScan(x, y, z); return true;
}

// ---- vegetacao na geracao do mundo (chamada por gen() em world.inc para cada topo de grama com ar em cima) ----
static void xbVeg(Chunk* c, int x, int h, int z, int wx, int wz, int bio) {
    if (h + 3 >= CH || gXM[XB_PLANT - XB0].empty()) return;
    static const float GD[NBIOME] = {0.f, 0.f, .30f, .20f, 0.f, .24f, .38f, .16f, .20f, 0.f, .08f};   // densidade de mato por bioma
    const float hv = hash2(wx, wz, 311);
    auto put = [&](int id) { c->b[idx(x, h + 1, z)] = (BT)id; };
    auto dbl = [&](int lo) { if (c->b[idx(x, h + 2, z)] != AIR) { put(XB_PLANT); return; } c->b[idx(x, h + 1, z)] = (BT)lo; c->b[idx(x, h + 2, z)] = (BT)(lo + 1); };
    const int rx = fdiv(wx, 20), rz = fdiv(wz, 20);
    const bool patch = hash2(rx, rz, 314) > .55f;   // regioes 20x20 com flores
    if (patch && bio != B_JUNGLE && hash2(wx, wz, 315) < .26f) {
        const float pr = hash2(rx, rz, 313);
        static const int PLN[10] = {2, 3, 6, 11, 7, 8, 9, 10, 12, -18}, FOR[7] = {2, 3, 5, 13, -20, -22, 12}, SWP[2] = {4, 4}, TAI[2] = {2, 3};   // <0: planta alta (indice)
        int t = 2;
        if (bio == B_PLAINS || bio == B_SAVANNA || bio == B_MOUNTAINS) t = PLN[(int)(pr * 10) % 10];
        else if (bio == B_FOREST) t = FOR[(int)(pr * 7) % 7];
        else if (bio == B_SWAMP) t = SWP[(int)(pr * 2) % 2];
        else t = TAI[(int)(pr * 2) % 2];
        if (t < 0) dbl(XB_PLANT - t); else put(XB_PLANT + t);
        return;
    }
    if (hv < GD[bio]) {
        const float r = hash2(wx, wz, 312);
        if (bio == B_TAIGA) { if (r < .5f) put(XB_PLANT + 1); else if (r < .62f) dbl(XB_PLANT + 16); else put(XB_PLANT); }
        else if (bio == B_JUNGLE) { if (r < .45f) put(XB_PLANT); else if (r < .7f) put(XB_PLANT + 1); else if (r < .85f) dbl(XB_PLANT + 16); else dbl(XB_PLANT + 14); }
        else { if (r < .82f) put(XB_PLANT); else dbl(XB_PLANT + 14); }
    }
}
