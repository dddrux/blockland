// ================= formas dos blocos: colisao, selecao e raycast =================
// Cada id de bloco aponta para duas formas (indices no pool gShapes): COLISAO (o que bloqueia o jogador/mobs) e SELECAO (o que o raio de mira
// acerta e o contorno desenha). Uma forma e uma lista de ate SH_MAXB caixas em coordenadas LOCAIS do bloco (0..1; a colisao pode passar de 1, p.ex.
// cerca = 1,5) ou uma funcao que as calcula olhando vizinhos/meta (cerca, painel, escada...). Nao existe mais "AABB universal": tocha, alavanca, planta,
// bigorna e (nas proximas etapas) cerca/painel/porta/trilho/escada registram a propria forma aqui. Valores: Minecraft 1.12.
// Dependencias: so ids de main.cpp e getB/getM (world.inc).
struct AABB {
    double x0, y0, z0, x1, y1, z1;
    AABB offset(double x, double y, double z) const { return {x0 + x, y0 + y, z0 + z, x1 + x, y1 + y, z1 + z}; }
    AABB addCoord(double x, double y, double z) const {
        AABB r = *this;
        if (x < 0) r.x0 += x; else r.x1 += x;
        if (y < 0) r.y0 += y; else r.y1 += y;
        if (z < 0) r.z0 += z; else r.z1 += z;
        return r;
    }
    bool intersects(const AABB& o) const { return o.x1 > x0 && o.x0 < x1 && o.y1 > y0 && o.y0 < y1 && o.z1 > z0 && o.z0 < z1; }
    // calculateXOffset/YOffset/ZOffset do Java: "this" é o bloco, "e" é a entidade.
    double calcX(const AABB& e, double x) const {
        if (e.y1 > y0 && e.y0 < y1 && e.z1 > z0 && e.z0 < z1) {
            if (x > 0 && e.x1 <= x0) { double d = x0 - e.x1; if (d < x) x = d; }
            else if (x < 0 && e.x0 >= x1) { double d = x1 - e.x0; if (d > x) x = d; }
        }
        return x;
    }
    double calcY(const AABB& e, double y) const {
        if (e.x1 > x0 && e.x0 < x1 && e.z1 > z0 && e.z0 < z1) {
            if (y > 0 && e.y1 <= y0) { double d = y0 - e.y1; if (d < y) y = d; }
            else if (y < 0 && e.y0 >= y1) { double d = y1 - e.y0; if (d > y) y = d; }
        }
        return y;
    }
    double calcZ(const AABB& e, double z) const {
        if (e.x1 > x0 && e.x0 < x1 && e.y1 > y0 && e.y0 < y1) {
            if (z > 0 && e.z1 <= z0) { double d = z0 - e.z1; if (d < z) z = d; }
            else if (z < 0 && e.z0 >= z1) { double d = z1 - e.z0; if (d > z) z = d; }
        }
        return z;
    }
};

static int getB(int x, int y, int z);   // world.inc
static int getM(int x, int y, int z);

static const int SH_MAXB = 8;
typedef int (*ShapeFn)(int x, int y, int z, int id, int meta, bool sel, AABB* out);   // preenche caixas LOCAIS; devolve quantas. meta = metadado do bloco (ou do que sera colocado)
struct ShapeDef { int n = 0; AABB b[SH_MAXB]; ShapeFn fn = nullptr; };
static std::vector<ShapeDef> gShapes;
enum { SHAPE_NONE = 0, SHAPE_CUBE = 1 };
static int shapeAdd(std::initializer_list<AABB> l) {
    ShapeDef s; for (const AABB& a : l) if (s.n < SH_MAXB) s.b[s.n++] = a;
    gShapes.push_back(s); return (int)gShapes.size() - 1;
}
static int shapeAddFn(ShapeFn f) { ShapeDef s; s.fn = f; gShapes.push_back(s); return (int)gShapes.size() - 1; }
static void blockShapeSet(int id, int coll, int sel) { if ((unsigned)id < 65536u) { gBlkColl[id] = (uint8_t)coll; gBlkSel[id] = (uint8_t)sel; } }

// caixas do bloco em coordenadas de MUNDO (colisao ou selecao); id ja lido pelo chamador
// meta < 0 => le o metadado do mundo (so se a forma depender dele); >= 0 => forma de um bloco AINDA NAO colocado (teste de sobreposicao ao colocar)
static int blockBoxesId(int id, int x, int y, int z, bool sel, AABB* out, int meta = -1) {
    const int si = (unsigned)id < 65536u ? (sel ? gBlkSel[id] : gBlkColl[id]) : 0;
    if (!si) return 0;
    const ShapeDef& s = gShapes[si];
    int n;
    if (s.fn) { n = s.fn(x, y, z, id, meta >= 0 ? meta : getM(x, y, z), sel, out); if (n > SH_MAXB) n = SH_MAXB; }
    else { n = s.n; for (int i = 0; i < n; i++) out[i] = s.b[i]; }
    for (int i = 0; i < n; i++) out[i] = out[i].offset(x, y, z);
    return n;
}
static int blockBoxes(int x, int y, int z, bool sel, AABB* out) { return blockBoxesId(getB(x, y, z), x, y, z, sel, out); }

// ids das formas dos blocos que ja existem (para os testes e para registrar novas variantes)
// blocos com EIXO (tronco, feno, osso, pilar...): meta & 3 = 0 Y, 1 X, 2 Z. O mesher troca topo/lado e gira o grao; placement.inc escolhe o eixo pela face clicada
static uint8_t gBlkAxis[65536];
// blocos com FRENTE (abobora entalhada, fornalha, dispenser...): meta & 3 = direcao em que a frente aponta (0 N -Z, 1 L +X, 2 S +Z, 3 O -X); a colocacao a vira para o jogador
static uint8_t gBlkFacing[65536];
static int gEggMdl = 0;   // patchEgg.1: id do ovo do dragao (pack) com modelo de 8 camadas (models.inc: gEggQ)
static const char* const HEAD_N[6] = {"skeleton", "wither_skeleton", "zombie", "creeper", "steve", "dragon"};   // patchHead.2: nomes das cabecas (heads.inc)
static int gHeadId[6];   // ids dos blocos de cabeca (0 = ausente); modelo 8x8x8 em models.inc (gHeadQ)
static inline int headIdx(int b) { if (b < 1) return -1; for (int i = 0; i < 6; i++) if (gHeadId[i] == b) return i; return -1; }
static int gEnchId = 0, gEndTopF = -1, gEndSideF = -1, gEndEye = -1;   // mesa de encantamento (12/16); tiles da moldura com olho
static int gEndFrameId = 0;   // moldura do portal do End do pack (13/16 de altura)
static inline float pkHeight(int b) { return !b ? 1.f : b == gEndFrameId ? .8125f : b == gEnchId ? .75f : 1.f; }   // altura do modelo (1.12: moldura 13/16, mesa 12/16)
static int gCactusId = 0;   // id do cacto do pack (0 = pack sem cacto); definido em pkBuildProps
static struct ShapeIds { int torchStand, torchWall[4], leverWall[4], leverFloor, leverCeil, anvilZ, anvilX, grass, flower, tall; } gSh;

static struct BlockShapeInit {
    BlockShapeInit() {
        gShapes.assign(2, ShapeDef()); gShapes[1].n = 1; gShapes[1].b[0] = {0, 0, 0, 1, 1, 1};
        // tocha: em pe e 4 de parede. Parede k presa ao bloco em -X,-Z,+X,+Z (ver xbSupported em models.inc)
        gSh.torchStand = shapeAdd({{.4, 0, .4, .6, .6, .6}});
        gSh.torchWall[0] = shapeAdd({{0, .2, .35, .3, .8, .65}});
        gSh.torchWall[1] = shapeAdd({{.35, .2, 0, .65, .8, .3}});
        gSh.torchWall[2] = shapeAdd({{.7, .2, .35, 1, .8, .65}});
        gSh.torchWall[3] = shapeAdd({{.35, .2, .7, .65, .8, 1}});
        // alavanca: parede presa em +Z,-X,-Z,+X (estados 0..3), chao, teto. Mesma caixa ligada ou desligada
        gSh.leverWall[0] = shapeAdd({{.25, .2, .5, .75, .8, 1}});
        gSh.leverWall[1] = shapeAdd({{0, .2, .25, .5, .8, .75}});
        gSh.leverWall[2] = shapeAdd({{.25, .2, 0, .75, .8, .5}});
        gSh.leverWall[3] = shapeAdd({{.5, .2, .25, 1, .8, .75}});
        gSh.leverFloor = shapeAdd({{.25, 0, .25, .75, .6, .75}});
        gSh.leverCeil = shapeAdd({{.25, .4, .25, .75, 1, .75}});
        // bigorna: rotacao par = eixo comprido em Z; impar = em X
        gSh.anvilZ = shapeAdd({{.125, 0, 0, .875, 1, 1}});
        gSh.anvilX = shapeAdd({{0, 0, .125, 1, 1, .875}});
        // plantas (BlockBush do 1.12): grama/samambaia, flores e metades das plantas altas
        gSh.grass = shapeAdd({{.1, 0, .1, .9, .8, .9}});
        gSh.flower = shapeAdd({{.3, 0, .3, .7, .6, .7}});
        gSh.tall = shapeAdd({{.15, 0, .15, .85, 1, .85}});   // patchOutline.1: plantas altas (cada metade) cobrem o modelo inteiro; antes usavam a caixa da flor (altura .6) e o contorno ficava curto e com vao entre as metades

        for (int id = 0; id < 65536; id++) {
            int c = SHAPE_CUBE, s = SHAPE_CUBE;                 // padrao: cubo cheio (blocos do jogo, do pack e de mods)
            if (id == AIR || isLiquid(id)) c = s = SHAPE_NONE;   // ar e agua: sem colisao e sem selecao (o balde pede rayCast(liquids=true))
            else if (isXB(id)) c = s = SHAPE_NONE;              // blocos com modelo: ids sem bloco real continuam vazios
            blockShapeSet(id, c, s);
        }
        { const int rs = shapeAddFn([](int, int, int, int, int meta, bool, AABB* o) { const int sh = meta & 15; o[0] = AABB{0, 0, 0, 1, (sh >= 2 && sh <= 5) ? .5 : .125, 1}; return 1; });   // trilho: selecao fina (subida = meia altura), sem colisao
          for (int k = 0; k < 4; k++) blockShapeSet(XB_RAIL + k, SHAPE_NONE, rs); }
        gBlkAxis[LOG] = 1;   // tronco do jogo base
        blockShapeSet(XB_TORCH, SHAPE_NONE, gSh.torchStand); blockShapeSet(XB_RTORCH, SHAPE_NONE, gSh.torchStand);
        for (int k = 0; k < 4; k++) { blockShapeSet(XB_TORCH + 1 + k, SHAPE_NONE, gSh.torchWall[k]); blockShapeSet(XB_RTORCH + 1 + k, SHAPE_NONE, gSh.torchWall[k]); }
        for (int on = 0; on < 2; on++) for (int st = 0; st < 12; st++) {
            const int face = st >> 2, f = st & 3;
            blockShapeSet(XB_LEVER + on * 12 + st, SHAPE_NONE, face == 0 ? gSh.leverWall[f] : face == 1 ? gSh.leverFloor : gSh.leverCeil);
        }
        for (int k = 0; k < 4; k++) blockShapeSet(XB_ANVIL + k, (k & 1) ? gSh.anvilX : gSh.anvilZ, (k & 1) ? gSh.anvilX : gSh.anvilZ);
        { const int sb = shapeAddFn([](int, int, int, int, int, bool, AABB* o) { o[0] = AABB{.25, 0, .25, .75, 1, .75}; return 1; });   // estandarte em pe: sem colisao
          const int wb = shapeAddFn([](int, int, int, int, int meta, bool, AABB* o) {   // na parede (1.12: BlockBanner.BannerWall): fina, 25/32 de altura, junto ao bloco de suporte
              static const AABB W[4] = {{0, 0, .875, 1, .78125, 1}, {0, 0, 0, .125, .78125, 1}, {0, 0, 0, 1, .78125, .125}, {.875, 0, 0, 1, .78125, 1}}; o[0] = W[meta & 3]; return 1; });
          blockShapeSet(XB_BANNER, SHAPE_NONE, sb); blockShapeSet(XB_WBANNER, SHAPE_NONE, wb); }
        for (int i = 0; i < 6; i++) blockShapeSet(XB_SAPLING + i, SHAPE_NONE, gSh.flower);   // muda: sem colisao, selecao de planta
        for (int i = 0; i < 24; i++) blockShapeSet(XB_PLANT + i, SHAPE_NONE, i < 2 ? gSh.grass : i < 14 ? gSh.flower : gSh.tall);
        blockShapeSet(XB_DEADBUSH, SHAPE_NONE, gSh.flower); blockShapeSet(XB_PEONY, SHAPE_NONE, gSh.tall); blockShapeSet(XB_PEONY + 1, SHAPE_NONE, gSh.tall);
    }
} blockShapeInit;

// ---------- raycast contra as caixas de SELECAO reais ----------
// faces: 0 -X, 1 +X, 2 -Y, 3 +Y, 4 -Z, 5 +Z. Celula de colocacao = bloco acertado + normal da face (como pos.offset(side) do Minecraft).
struct RayHit { bool hit = false; int x = 0, y = 0, z = 0, face = 0, nx = 0, ny = 0, nz = 0; double t = 0, hx = 0, hy = 0, hz = 0; AABB box{0, 0, 0, 0, 0, 0}; };
static const int FACE_N[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
// raio x caixa (metodo dos slabs). So conta se o raio ENTRA na caixa (origem dentro => sem acerto, como o Minecraft)
static bool rayBox(const double* o, const double* d, const AABB& b, double maxT, double& tOut, int& face) {
    const double lo[3] = {b.x0, b.y0, b.z0}, hi[3] = {b.x1, b.y1, b.z1};
    double tmin = -1e30, tmax = 1e30; int fa = -1;
    for (int a = 0; a < 3; a++) {
        if (fabs(d[a]) < 1e-12) { if (o[a] < lo[a] || o[a] > hi[a]) return false; continue; }
        const double nearP = d[a] > 0 ? lo[a] : hi[a], farP = d[a] > 0 ? hi[a] : lo[a];
        const double t1 = (nearP - o[a]) / d[a], t2 = (farP - o[a]) / d[a];
        if (t1 > tmin) { tmin = t1; fa = a * 2 + (d[a] > 0 ? 0 : 1); }
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax) return false;
    }
    if (fa < 0 || tmin < 0 || tmin > maxT) return false;
    tOut = tmin; face = fa; return true;
}
static float gBlockReach = 5.f;   // patchReach.1: alcance de bloco como o MC 1.12 (sobrevivencia 4.5, criativo 5.0); atualizado todo quadro em loop/frame_begin.inc
static bool rayCast(const float* of, const float* df, float maxDist, RayHit& r, bool liquids = false) {
    r = RayHit();
    double o[3] = {of[0], of[1], of[2]}, d[3] = {df[0], df[1], df[2]};
    const double len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]); if (len < 1e-9 || maxDist <= 0) return false;
    for (double& v : d) v /= len;
    int c[3] = {(int)floor(o[0]), (int)floor(o[1]), (int)floor(o[2])};
    int s[3]; double td[3], tn[3];
    for (int a = 0; a < 3; a++) {
        s[a] = d[a] > 0 ? 1 : d[a] < 0 ? -1 : 0;
        td[a] = s[a] ? fabs(1.0 / d[a]) : 1e30;
        tn[a] = s[a] ? fabs(((s[a] > 0 ? c[a] + 1.0 : (double)c[a]) - o[a]) / d[a]) : 1e30;
    }
    AABB bx[SH_MAXB];
    for (int guard = 0; guard < 64; guard++) {
        const int id = getB(c[0], c[1], c[2]);
        int n = 0;
        if (liquids && isLiquid(id)) { bx[0] = AABB{(double)c[0], (double)c[1], (double)c[2], c[0] + 1.0, c[1] + 1.0, c[2] + 1.0}; n = 1; }
        else if (pickable(id)) n = blockBoxesId(id, c[0], c[1], c[2], true, bx);
        double best = 1e30; int bf = 0, bi = -1;
        for (int i = 0; i < n; i++) { double t; int f; if (rayBox(o, d, bx[i], maxDist, t, f) && t < best) { best = t; bf = f; bi = i; } }
        if (bi >= 0) {
            r.hit = true; r.x = c[0]; r.y = c[1]; r.z = c[2]; r.face = bf; r.t = best; r.box = bx[bi];
            r.nx = FACE_N[bf][0]; r.ny = FACE_N[bf][1]; r.nz = FACE_N[bf][2];
            r.hx = o[0] + d[0] * best; r.hy = o[1] + d[1] * best; r.hz = o[2] + d[2] * best;
            return true;
        }
        const int a = (tn[0] <= tn[1] && tn[0] <= tn[2]) ? 0 : (tn[1] <= tn[2] ? 1 : 2);
        if (tn[a] > maxDist) break;
        c[a] += s[a]; tn[a] += td[a];
    }
    return false;
}
