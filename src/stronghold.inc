// ---- Stronghold 1.12.2 (patchStronghold.1): MapGenStronghold (aneis) + StructureStrongholdPieces (layout com java.util.Random, pecas, portas, sala do portal) ----
// Layout (posicoes dos aneis, pecas, pesos, caixas, filhos, ordem de pendentes, altura) segue o Java passo a passo. Blocos: aleatoriedade por celula (hash), nao pelo Random de populacao.
enum { SH_STRAIGHT, SH_PRISON, SH_LEFT, SH_RIGHT, SH_ROOMX, SH_STAIRSS, SH_STAIRS, SH_CROSS, SH_CHESTC, SH_LIB, SH_PORTAL, SH_CORR, SH_START, SH_N };
struct SHP { int t, dir, bb[6], depth, door = 0, f0 = 0, f1 = 0, f2 = 0, f3 = 0, steps = 0; };   // bb = minX,minY,minZ,maxX,maxY,maxZ; dir 0 N,1 E,2 S,3 W (ordem de Plane.HORIZONTAL)
struct SHS { std::vector<SHP> ps; int cx = 0, cz = 0; bool ok = false; };
struct SHW { int t, weight, maxc, spawned; };
static const int SH_DIM[SH_PORTAL + 1][6] = {{-1,-1,0,5,5,7},{-1,-1,0,9,5,11},{-1,-1,0,5,5,5},{-1,-1,0,5,5,5},{-4,-1,0,11,7,11},{-1,-7,0,5,11,8},{-1,-7,0,5,11,5},{-4,-3,0,10,9,11},{-1,-1,0,5,5,7},{-4,-1,0,14,11,15},{-4,-1,0,11,8,16}};
static inline bool shInter(const int* a, const int* b) { return a[3] >= b[0] && a[0] <= b[3] && a[4] >= b[1] && a[1] <= b[4] && a[5] >= b[2] && a[2] <= b[5]; }
static void shBox(int x, int y, int z, const int* d, int dir, int* o) {   // getComponentToAddBoundingBox
    const int ox = d[0], oy = d[1], oz = d[2], w = d[3], h = d[4], dp = d[5];
    if (dir == 0) { o[0] = x + ox; o[1] = y + oy; o[2] = z - dp + 1 + oz; o[3] = x + w - 1 + ox; o[4] = y + h - 1 + oy; o[5] = z + oz; }
    else if (dir == 2) { o[0] = x + ox; o[1] = y + oy; o[2] = z + oz; o[3] = x + w - 1 + ox; o[4] = y + h - 1 + oy; o[5] = z + dp - 1 + oz; }
    else if (dir == 3) { o[0] = x - dp + 1 + oz; o[1] = y + oy; o[2] = z + ox; o[3] = x + oz; o[4] = y + h - 1 + oy; o[5] = z + w - 1 + ox; }
    else { o[0] = x + oz; o[1] = y + oy; o[2] = z + ox; o[3] = x + dp - 1 + oz; o[4] = y + h - 1 + oy; o[5] = z + w - 1 + ox; }
}
struct SHG { SHS* S; JRng* r; std::vector<SHW> list; int total = 0, last = -1, strong = -1, sx0 = 0, sz0 = 0; std::vector<int> pend; bool portal = false; };
static int shDoor(JRng& r) { const int i = r.nextInt(5); return i == 2 ? 1 : i == 3 ? 2 : i == 4 ? 3 : 0; }   // 0 abertura, 1 porta de madeira, 2 grade, 3 porta de ferro
static bool shBool(JRng& r) { return r.next(1) != 0; }
static bool shCanAdd(SHG& g) { bool f = false; g.total = 0; for (auto& w : g.list) { if (w.maxc > 0 && w.spawned < w.maxc) f = true; g.total += w.weight; } return f; }
static int shCreate(SHG& g, int t, int x, int y, int z, int dir, int depth) {   // findAndCreatePieceFactory + Xxx.createPiece: -1 se nao cabe
    SHP p; p.t = t; p.dir = dir; p.depth = depth; bool ok = false;
    for (int tr = 0; tr < (t == SH_LIB ? 2 : 1) && !ok; tr++) {
        int d[6]; memcpy(d, SH_DIM[t], sizeof d); if (t == SH_LIB && tr == 1) d[4] = 6;
        shBox(x, y, z, d, dir, p.bb); ok = p.bb[1] > 10; for (auto& q : g.S->ps) if (ok && shInter(q.bb, p.bb)) ok = false;
    }
    if (!ok) return -1;
    JRng& r = *g.r;
    switch (t) {
        case SH_STRAIGHT: p.door = shDoor(r); p.f0 = r.nextInt(2) == 0; p.f1 = r.nextInt(2) == 0; break;   // expandsX, expandsZ
        case SH_ROOMX: p.door = shDoor(r); p.f0 = r.nextInt(5); break;
        case SH_CROSS: p.door = shDoor(r); p.f0 = shBool(r); p.f1 = shBool(r); p.f2 = shBool(r); p.f3 = r.nextInt(3) > 0; break;   // leftLow, leftHigh, rightLow, rightHigh
        case SH_PORTAL: break;
        default: p.door = shDoor(r); break;
    }
    g.S->ps.push_back(p); return (int)g.S->ps.size() - 1;
}
static int shFromDoor(SHG& g, int x, int y, int z, int dir, int depth) {   // generatePieceFromSmallDoor
    if (!shCanAdd(g)) return -1;
    if (g.strong >= 0) { const int t = g.strong; g.strong = -1; const int i = shCreate(g, t, x, y, z, dir, depth); if (i >= 0) return i; }
    for (int j = 0; j < 5; j++) {
        int i = g.r->nextInt(g.total);
        for (size_t k = 0; k < g.list.size(); k++) {
            i -= g.list[k].weight; if (i >= 0) continue;
            SHW& w = g.list[k];
            if ((w.maxc > 0 && w.spawned >= w.maxc) || (w.t == SH_LIB && depth <= 4) || (w.t == SH_PORTAL && depth <= 5) || w.t == g.last) break;
            const int c = shCreate(g, w.t, x, y, z, dir, depth);
            if (c >= 0) { w.spawned++; g.last = w.t; if (w.maxc > 0 && w.spawned >= w.maxc) g.list.erase(g.list.begin() + k); return c; }
        }
    }
    int b[6], d[6] = {-1, -1, 0, 5, 5, 4}; shBox(x, y, z, d, dir, b);   // Corridor.findPieceBox
    const SHP* hit = nullptr; for (auto& q : g.S->ps) if (shInter(q.bb, b)) { hit = &q; break; }
    if (!hit || hit->bb[1] != b[1]) return -1;
    for (int n = 3; n >= 1; n--) { d[5] = n; shBox(x, y, z, d, dir, b); if (!shInter(hit->bb, b)) { d[5] = n + 1; SHP p; p.t = SH_CORR; p.dir = dir; p.depth = depth; shBox(x, y, z, d, dir, p.bb); p.steps = (dir == 0 || dir == 2) ? p.bb[5] - p.bb[2] + 1 : p.bb[3] - p.bb[0] + 1; if (p.bb[1] <= 1) return -1; g.S->ps.push_back(p); return (int)g.S->ps.size() - 1; } }
    return -1;
}
static void shAdd(SHG& g, int x, int y, int z, int dir, int parentDepth) {   // generateAndAddPiece
    if (parentDepth > 50 || abs(x - g.sx0) > 112 || abs(z - g.sz0) > 112) return;
    const int i = shFromDoor(g, x, y, z, dir, parentDepth + 1); if (i >= 0) g.pend.push_back(i);
}
static void shNormal(SHG& g, const SHP& p, int ox, int oy) {
    switch (p.dir) { case 0: shAdd(g, p.bb[0] + ox, p.bb[1] + oy, p.bb[2] - 1, 0, p.depth); break; case 2: shAdd(g, p.bb[0] + ox, p.bb[1] + oy, p.bb[5] + 1, 2, p.depth); break;
        case 3: shAdd(g, p.bb[0] - 1, p.bb[1] + oy, p.bb[2] + ox, 3, p.depth); break; default: shAdd(g, p.bb[3] + 1, p.bb[1] + oy, p.bb[2] + ox, 1, p.depth); }
}
static void shSideX(SHG& g, const SHP& p, int oy, int ox) {
    if (p.dir == 0 || p.dir == 2) shAdd(g, p.bb[0] - 1, p.bb[1] + oy, p.bb[2] + ox, 3, p.depth); else shAdd(g, p.bb[0] + ox, p.bb[1] + oy, p.bb[2] - 1, 0, p.depth);
}
static void shSideZ(SHG& g, const SHP& p, int oy, int ox) {
    if (p.dir == 0 || p.dir == 2) shAdd(g, p.bb[3] + 1, p.bb[1] + oy, p.bb[2] + ox, 1, p.depth); else shAdd(g, p.bb[0] + ox, p.bb[1] + oy, p.bb[5] + 1, 2, p.depth);
}
static void shBuildComp(SHG& g, int idx) {   // buildComponent de cada peca (copia: ps pode crescer)
    const SHP p = g.S->ps[idx];
    switch (p.t) {
        case SH_STRAIGHT: shNormal(g, p, 1, 1); if (p.f0) shSideX(g, p, 1, 2); if (p.f1) shSideZ(g, p, 1, 2); break;
        case SH_PRISON: case SH_STAIRSS: case SH_CHESTC: shNormal(g, p, 1, 1); break;
        case SH_LEFT: if (p.dir == 0 || p.dir == 1) shSideX(g, p, 1, 1); else shSideZ(g, p, 1, 1); break;
        case SH_RIGHT: if (p.dir == 0 || p.dir == 1) shSideZ(g, p, 1, 1); else shSideX(g, p, 1, 1); break;
        case SH_ROOMX: shNormal(g, p, 4, 1); shSideX(g, p, 1, 4); shSideZ(g, p, 1, 4); break;
        case SH_STAIRS: case SH_START: if (p.t == SH_START) g.strong = SH_CROSS; shNormal(g, p, 1, 1); break;
        case SH_CROSS: { int i = 3, j = 5; if (p.dir == 3 || p.dir == 0) { i = 5; j = 3; } shNormal(g, p, 5, 1); if (p.f0) shSideX(g, p, i, 1); if (p.f1) shSideX(g, p, j, 7); if (p.f2) shSideZ(g, p, i, 1); if (p.f3) shSideZ(g, p, j, 7); break; }
        case SH_PORTAL: g.portal = true; break;
        default: break;
    }
}
static int64_t shNextLong(JRng& r) { return ((int64_t)r.next(32) << 32) + r.next(32); }
static bool shStart(SHS& S, JRng& r, int cx, int cz) {   // StructureStronghold.Start
    S.ps.clear(); SHG g; g.S = &S; g.r = &r;
    g.list = {{SH_STRAIGHT, 40, 0, 0}, {SH_PRISON, 5, 5, 0}, {SH_LEFT, 20, 0, 0}, {SH_RIGHT, 20, 0, 0}, {SH_ROOMX, 10, 6, 0}, {SH_STAIRSS, 5, 5, 0}, {SH_STAIRS, 5, 5, 0}, {SH_CROSS, 5, 4, 0}, {SH_CHESTC, 5, 4, 0}, {SH_LIB, 10, 2, 0}, {SH_PORTAL, 20, 1, 0}};
    SHP s; s.t = SH_START; s.dir = r.nextInt(4); s.depth = 0; s.door = shDoor(r);
    const int x = (cx << 4) + 2, z = (cz << 4) + 2; s.bb[0] = x; s.bb[1] = 64; s.bb[2] = z; s.bb[3] = x + 4; s.bb[4] = 74; s.bb[5] = z + 4;
    g.sx0 = s.bb[0]; g.sz0 = s.bb[2]; S.ps.push_back(s); shBuildComp(g, 0);
    while (!g.pend.empty()) { const int i = r.nextInt((int)g.pend.size()); const int c = g.pend[i]; g.pend.erase(g.pend.begin() + i); shBuildComp(g, c); }
    int mnY = 1 << 20, mxY = -(1 << 20); for (auto& p : S.ps) { mnY = std::min(mnY, p.bb[1]); mxY = std::max(mxY, p.bb[4]); }   // markAvailableHeight(world, rand, 10)
    const int lim = 63 - 10; int j = (mxY - mnY + 1) + 1; if (j < lim) j += r.nextInt(lim - j);
    const int k = j - mxY; for (auto& p : S.ps) { p.bb[1] += k; p.bb[4] += k; }
    S.cx = cx; S.cz = cz; S.ok = g.portal; return g.portal;
}
// ---- posicoes (MapGenStronghold.generatePositions: 128 fortalezas em aneis; sem o ajuste ao bioma por enquanto) ----
static std::vector<std::pair<int, int>> gShPos; static int64_t gShSeed = INT64_MIN; static std::map<int, SHS*> gShCache;
static void shInit() {
    if (gShSeed == (int64_t)WORLD_SEED && !gShPos.empty()) return;
    for (auto& kv : gShCache) delete kv.second; gShCache.clear(); gShPos.clear(); gShSeed = (int64_t)WORLD_SEED;
    JRng r((int64_t)WORLD_SEED); const int N = 128, dist = 32; int spread = 3, i = 0, j = 0; double d0 = r.nextDouble() * (M_PI * 2.0);
    for (int k = 0; k < N; k++) {
        const double d1 = (double)(4 * dist + dist * j * 6) + (r.nextDouble() - 0.5) * (double)dist * 2.5;
        gShPos.push_back({(int)llround(cos(d0) * d1), (int)llround(sin(d0) * d1)});
        d0 += (M_PI * 2.0) / (double)spread; ++i;
        if (i == spread) { ++j; i = 0; spread += 2 * spread / (j + 1); spread = std::min(spread, N - k); d0 += r.nextDouble() * (M_PI * 2.0); }
    }
}
static SHS* shGet(int i) {
    auto it = gShCache.find(i); if (it != gShCache.end()) return it->second;
    SHS* S = new SHS(); shInit(); const int cx = gShPos[i].first, cz = gShPos[i].second;
    JRng t((int64_t)WORLD_SEED); const int64_t a = shNextLong(t), b = shNextLong(t);   // MapGenBase.generate: seed = cx*a ^ cz*b ^ worldSeed; recursiveGenerate faz nextInt()
    JRng r(((int64_t)cx * a) ^ ((int64_t)cz * b) ^ (int64_t)WORLD_SEED); r.nextInt();
    for (int n = 0; n < 200 && !shStart(*S, r, cx, cz); n++) {}
    gShCache[i] = S; return S;
}
// ---- construcao ----
static int gShStone[3] = {0, 0, 0};   // 0 cracked, 1 mossy
struct SHB { Chunk* c; int cx0, cz0; const SHP* p; };
static SHB gSB;
static inline void sbW(int lx, int lz, int& wx, int& wz) {
    const SHP& p = *gSB.p;
    if (p.dir == 0) { wx = p.bb[0] + lx; wz = p.bb[5] - lz; } else if (p.dir == 2) { wx = p.bb[0] + lx; wz = p.bb[2] + lz; }
    else if (p.dir == 3) { wx = p.bb[3] - lz; wz = p.bb[2] + lx; } else { wx = p.bb[0] + lz; wz = p.bb[2] + lx; }
}
static inline bool sbIn(int wx, int wz) { return wx >= gSB.cx0 && wx < gSB.cx0 + 16 && wz >= gSB.cz0 && wz < gSB.cz0 + 16; }
static void sbPut(int lx, int ly, int lz, int id, int meta = 0, bool onlyExisting = false) {
    int wx, wz; sbW(lx, lz, wx, wz); const int wy = gSB.p->bb[1] + ly; if (!sbIn(wx, wz) || wy < 1 || wy > CH - 2) return;
    const int i = idx(wx - gSB.cx0, wy, wz - gSB.cz0); BT& t = gSB.c->b[i]; if (t == BEDROCK || (onlyExisting && t == AIR)) return;
    t = (BT)id; gSB.c->m[i] = (uint8_t)meta;
}
static void sbStone(int lx, int ly, int lz, bool wall, bool onlyExisting) {   // BlockSelector Stones
    int wx, wz; sbW(lx, lz, wx, wz); const int wy = gSB.p->bb[1] + ly; if (!sbIn(wx, wz) || wy < 1 || wy > CH - 2) return;
    if (!wall) { sbPut(lx, ly, lz, AIR, 0, onlyExisting); return; }
    const float f = (nhash(wx, wy, wz, 7) & 1023) / 1024.f; int id = STONEBRICK;
    if (f < .2f && gShStone[0]) id = gShStone[0]; else if (f >= .2f && f < .5f && gShStone[1]) id = gShStone[1];
    sbPut(lx, ly, lz, id, 0, onlyExisting);
}
static void sbRand(int ax, int ay, int az, int bx, int by, int bz, bool onlyExisting) {
    for (int y = ay; y <= by; y++) for (int x = ax; x <= bx; x++) for (int z = az; z <= bz; z++) sbStone(x, y, z, y == ay || y == by || x == ax || x == bx || z == az || z == bz, onlyExisting);
}
static void sbFill(int ax, int ay, int az, int bx, int by, int bz, int id, int meta = 0) { for (int y = ay; y <= by; y++) for (int x = ax; x <= bx; x++) for (int z = az; z <= bz; z++) sbPut(x, y, z, id, meta); }
static void sbBrick(int x, int y, int z) { sbPut(x, y, z, STONEBRICK); }
static void sbStair(int lx, int ly, int lz, int bdx, int bdz, bool flip = false) {   // costas da escada na direcao local (bdx,bdz)
    const int dr = gSB.p->dir; int dx, dz; if (dr == 0) { dx = bdx; dz = -bdz; } else if (dr == 2) { dx = bdx; dz = bdz; } else if (dr == 3) { dx = -bdz; dz = bdx; } else { dx = bdz; dz = bdx; }
    const int s = dz < 0 ? 0 : dx > 0 ? 1 : dz > 0 ? 2 : 3; sbPut(lx, ly, lz, XB_STAIR, s | (flip ? 4 : 0) | (8 << 3));
}
static void sbTorch(int lx, int ly, int lz, int sx, int sz) {   // 10% de chance (randomlyPlaceBlock); (sx,sz) = direcao local do suporte
    int wx, wz; sbW(lx, lz, wx, wz); if (!sbIn(wx, wz) || (nhash(wx, gSB.p->bb[1] + ly, wz, 9) % 100) >= 10) return;
    const int dr = gSB.p->dir; int dx, dz; if (dr == 0) { dx = sx; dz = -sz; } else if (dr == 2) { dx = sx; dz = sz; } else if (dr == 3) { dx = -sz; dz = sx; } else { dx = sz; dz = sx; }
    sbPut(lx, ly, lz, XB_TORCH + 1 + (dx < 0 ? 0 : dz < 0 ? 1 : dx > 0 ? 2 : 3));
}
static void sbDoor(int type, int x, int y, int z) {   // placeDoor: moldura de pedra (madeira/ferro); porta/grade ainda como vao (AINDA FALTA)
    if (type == 2) { sbFill(x, y, z, x + 2, y + 2, z, STONEBRICK); sbPut(x + 1, y, z, XB_PANE, 0); sbPut(x + 1, y + 1, z, XB_PANE, 0); return; }   // grade de ferro (iron bars)
    if (type == 0) { sbFill(x, y, z, x + 2, y + 2, z, AIR); return; }
    sbFill(x, y, z, x + 2, y + 2, z, STONEBRICK); sbPut(x + 1, y, z, AIR); sbPut(x + 1, y + 1, z, AIR);
}
static const int SB_SLAB = 10, SB_SLABB = 8;
static void sbRandPut(int lx, int ly, int lz, int id, int pct, uint32_t salt) {   // coloca id com pct% de chance (hash da celula: igual em todos os chunks), so onde ja ha ar
    int wx, wz; sbW(lx, lz, wx, wz); if (!sbIn(wx, wz) || (int)(nhash(wx, gSB.p->bb[1] + ly, wz, salt) % 100) >= pct) return;
    const int i = idx(wx - gSB.cx0, gSB.p->bb[1] + ly, wz - gSB.cz0); if (gSB.p->bb[1] + ly < 1 || gSB.c->b[i] != AIR) return; gSB.c->b[i] = (BT)id; gSB.c->m[i] = 0;
}
static int shBlk(const char* n) { const int i = pkFind(n); return i ? i : STONEBRICK; }
static void sbSlab(int ax, int ay, int az, int bx, int by, int bz, int mat, int half) { sbFill(ax, ay, az, bx, by, bz, XB_SLAB, half | (mat << 2)); }
static bool sbLiquid(const SHP& p) {   // isLiquidInStructureBoundingBox (parte no chunk)
    const int x0 = std::max(p.bb[0] - 1, gSB.cx0), x1 = std::min(p.bb[3] + 1, gSB.cx0 + 15), z0 = std::max(p.bb[2] - 1, gSB.cz0), z1 = std::min(p.bb[5] + 1, gSB.cz0 + 15);
    const int y0 = std::max(p.bb[1] - 1, 1), y1 = std::min(p.bb[4] + 1, CH - 2);
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) { const BT t = gSB.c->b[idx(x - gSB.cx0, y, z - gSB.cz0)]; if (t == WATER || isLava(t) || isWater(t)) return true; }
    return false;
}
static void shPiece(const SHP& p) {
    gSB.p = &p; if (sbLiquid(p)) return;
    switch (p.t) {
        case SH_STRAIGHT:
            sbRand(0, 0, 0, 4, 4, 6, true); sbDoor(p.door, 1, 1, 0); sbDoor(0, 1, 1, 6);
            sbTorch(1, 2, 1, -1, 0); sbTorch(3, 2, 1, 1, 0); sbTorch(1, 2, 5, -1, 0); sbTorch(3, 2, 5, 1, 0);
            if (p.f0) sbFill(0, 1, 2, 0, 3, 4, AIR); if (p.f1) sbFill(4, 1, 2, 4, 3, 4, AIR); break;
        case SH_CORR:
            for (int i = 0; i < p.steps; i++) {
                for (int x = 0; x <= 4; x++) sbBrick(x, 0, i);
                for (int y = 1; y <= 3; y++) { sbBrick(0, y, i); sbFill(1, y, i, 3, y, i, AIR); sbBrick(4, y, i); }
                for (int x = 0; x <= 4; x++) sbBrick(x, 4, i);
            } break;
        case SH_PRISON: {   // PrisonHall (9x5x11): parede central com grades; celas do lado leste (aproximado do Java)
            sbRand(0, 0, 0, 8, 4, 10, true); sbDoor(p.door, 1, 1, 0); sbDoor(0, 1, 1, 10);
            for (int z : {1, 3, 7, 9}) sbRand(4, 1, z, 4, 3, z, false);
            for (int z : {2, 8}) sbFill(4, 1, z, 4, 3, z, XB_PANE, 0);   // janelas de grade entre os pilares
            sbFill(4, 1, 4, 4, 3, 6, XB_PANE, 0); sbFill(4, 1, 5, 4, 2, 5, AIR);   // passagem no meio da parede
            sbRand(5, 1, 3, 7, 3, 3, false); sbRand(5, 1, 7, 7, 3, 7, false);   // divisorias das celas
            sbFill(5, 1, 3, 5, 2, 3, XB_PANE, 0); sbFill(5, 1, 7, 5, 2, 7, XB_PANE, 0);
            sbTorch(1, 3, 1, -1, 0); sbTorch(3, 3, 9, 1, 0); break; }
        case SH_LEFT: case SH_RIGHT: {
            sbRand(0, 0, 0, 4, 4, 4, true); sbDoor(p.door, 1, 1, 0);
            const bool ne = p.dir == 0 || p.dir == 1; const bool west = (p.t == SH_LEFT) == ne;
            if (west) sbFill(0, 1, 1, 0, 3, 3, AIR); else sbFill(4, 1, 1, 4, 3, 3, AIR); break; }
        case SH_ROOMX: {   // RoomCrossing: 5 tipos (p.f0); mobilia aproximada do Java
            sbRand(0, 0, 0, 10, 6, 10, true); sbDoor(p.door, 4, 1, 0); sbDoor(0, 4, 1, 10); sbFill(0, 1, 4, 0, 3, 6, AIR); sbFill(10, 1, 4, 10, 3, 6, AIR);
            switch (p.f0) {
                case 0: sbBrick(5, 1, 5); sbBrick(5, 2, 5); sbBrick(5, 3, 5); sbPut(4, 3, 5, XB_TORCH + 1 + 2); sbPut(6, 3, 5, XB_TORCH + 1); sbPut(5, 3, 4, XB_TORCH + 1 + 3); sbPut(5, 3, 6, XB_TORCH + 1 + 1);   // pilar central com 4 tochas
                    for (int x = 4; x <= 6; x++) for (int z = 4; z <= 6; z++) if (x != 5 || z != 5) sbSlab(x, 1, z, x, 1, z, SB_SLAB, 0); break;
                case 1: for (int i = 1; i <= 5; i++) { sbBrick(1, 1, i); sbBrick(9, 1, i); }   // balaustrada baixa nos lados
                    sbSlab(3, 1, 3, 7, 1, 3, SB_SLAB, 0); sbSlab(3, 1, 7, 7, 1, 7, SB_SLAB, 0); break;
                case 2: for (int x = 4; x <= 6; x++) for (int z = 4; z <= 6; z++) sbBrick(x, 1, z);   // fonte: base de pedra com agua no meio
                    sbPut(5, 2, 5, WATER); sbPut(4, 3, 4, XB_TORCH + 1 + 3); sbPut(6, 3, 6, XB_TORCH + 1 + 1); break;
                case 3: sbBrick(5, 1, 5); sbPut(5, 2, 5, XB_CHEST); sbSlab(4, 1, 4, 6, 1, 4, SB_SLAB, 0); sbSlab(4, 1, 6, 6, 1, 6, SB_SLAB, 0); sbSlab(4, 1, 5, 4, 1, 5, SB_SLAB, 0); sbSlab(6, 1, 5, 6, 1, 5, SB_SLAB, 0); break;   // plataforma com bau (sem loot)
                default: break; } break; }
        case SH_STAIRSS:
            sbRand(0, 0, 0, 4, 10, 7, true); sbDoor(p.door, 1, 7, 0); sbDoor(0, 1, 1, 7);
            for (int i = 0; i < 6; i++) { for (int x = 1; x <= 3; x++) sbStair(x, 6 - i, 1 + i, 0, -1); if (i < 5) for (int x = 1; x <= 3; x++) sbBrick(x, 5 - i, 1 + i); } break;
        case SH_STAIRS: case SH_START: {
            sbRand(0, 0, 0, 4, 10, 4, true); sbDoor(p.door, 1, 7, 0); sbDoor(0, 1, 1, 4);
            static const int S1[][3] = {{2,5,1},{1,5,1},{1,5,2},{1,4,3},{2,4,3},{3,3,3},{3,3,2},{3,2,1},{2,2,1},{1,1,1},{1,1,2},{1,1,3}}, S2[][3] = {{2,6,1},{1,6,1},{1,5,3},{3,4,3},{3,3,1},{1,2,1},{1,2,3}};
            for (auto& s : S1) sbBrick(s[0], s[1], s[2]); for (auto& s : S2) sbSlab(s[0], s[1], s[2], s[0], s[1], s[2], SB_SLAB, 0); break; }
        case SH_CROSS:
            sbRand(0, 0, 0, 9, 8, 10, true); sbDoor(p.door, 4, 3, 0);
            if (p.f0) sbFill(0, 3, 1, 0, 5, 3, AIR); if (p.f2) sbFill(9, 3, 1, 9, 5, 3, AIR); if (p.f1) sbFill(0, 5, 7, 0, 7, 9, AIR); if (p.f3) sbFill(9, 5, 7, 9, 7, 9, AIR);
            sbFill(5, 1, 10, 7, 3, 10, AIR);
            sbRand(1, 2, 1, 8, 2, 2, false); sbRand(4, 1, 5, 4, 4, 9, false); sbRand(8, 1, 5, 8, 4, 9, false); sbRand(1, 4, 7, 3, 4, 9, false); sbRand(1, 3, 5, 3, 3, 6, false);
            sbSlab(1, 3, 4, 3, 3, 4, SB_SLAB, 0); sbSlab(1, 4, 6, 3, 4, 6, SB_SLAB, 0);
            sbRand(5, 1, 7, 7, 1, 8, false); sbSlab(5, 1, 9, 7, 1, 9, SB_SLAB, 0); sbSlab(5, 2, 7, 7, 2, 7, SB_SLAB, 0); sbSlab(4, 5, 7, 4, 5, 9, SB_SLAB, 0); sbSlab(8, 5, 7, 8, 5, 9, SB_SLAB, 0);
            sbSlab(5, 5, 7, 7, 5, 9, SB_SLAB, 2); sbTorch(6, 5, 6, 0, 1); break;   // lajes: tipos aproximados
        case SH_CHESTC:
            sbRand(0, 0, 0, 4, 4, 6, true); sbDoor(p.door, 1, 1, 0); sbDoor(0, 1, 1, 6); sbRand(3, 1, 2, 3, 1, 4, false);
            sbSlab(3, 1, 1, 3, 1, 1, SB_SLABB, 0); sbSlab(3, 1, 5, 3, 1, 5, SB_SLABB, 0); sbSlab(3, 2, 2, 3, 2, 2, SB_SLABB, 0); sbSlab(3, 2, 4, 3, 2, 4, SB_SLABB, 0);
            for (int i = 2; i <= 4; i++) sbSlab(2, 1, i, 2, 1, i, SB_SLABB, 0);
            sbPut(3, 2, 3, XB_CHEST); break;   // bau vazio (loot STRONGHOLD_CORRIDOR: AINDA FALTA)
        case SH_LIB: {   // Library: estantes nas paredes e colunas, teias e mesas (sem 2o andar/escada ainda)
            const int h = p.bb[4] - p.bb[1]; sbRand(0, 0, 0, 13, h, 14, true); sbDoor(p.door, 4, 1, 0);
            static const int bk = shBlk("block/bookshelf"), web = shBlk("block/cobweb"); const int top = std::max(2, h - 2);
            for (int x = 1; x <= 12; x++) if (x < 4 || x > 6) sbFill(x, 1, 1, x, top, 1, bk);   // parede da entrada (menos a porta)
            for (int z = 2; z <= 13; z++) { sbFill(1, 1, z, 1, top, z, bk); sbFill(12, 1, z, 12, top, z, bk); }
            for (int x = 2; x <= 11; x++) if (x < 5 || x > 8) sbFill(x, 1, 13, x, top, 13, bk);
            for (int z : {4, 8, 11}) for (int x : {3, 4, 9, 10}) sbFill(x, 1, z, x, top, z, bk);   // fileiras de estantes
            for (int z : {5, 9}) { sbFill(6, 1, z, 7, 1, z, PLANKS); sbSlab(6, 2, z, 7, 2, z, SB_SLAB, 0); }   // mesas
            for (int y = 2; y <= top + 1; y++) for (int z = 2; z <= 12; z++) for (int x = 2; x <= 11; x++) sbRandPut(x, y, z, web, 4, 77 + y);   // teias
            sbTorch(2, 3, 2, -1, 0); sbTorch(11, 3, 12, 1, 0); break; }
        case SH_PORTAL: {
            sbRand(0, 0, 0, 10, 7, 15, false); sbDoor(2, 4, 1, 0);
            const int i = 6; sbRand(1, i, 1, 1, i, 14, false); sbRand(9, i, 1, 9, i, 14, false); sbRand(2, i, 1, 8, i, 2, false); sbRand(2, i, 14, 8, i, 14, false);
            sbRand(1, 1, 1, 2, 1, 4, false); sbRand(8, 1, 1, 9, 1, 4, false); sbFill(1, 1, 1, 1, 1, 3, LAVA); sbFill(9, 1, 1, 9, 1, 3, LAVA);
            sbRand(3, 1, 8, 7, 1, 12, false); sbFill(4, 1, 9, 6, 1, 11, LAVA);
            sbRand(4, 1, 5, 6, 1, 7, false); sbRand(4, 2, 6, 6, 2, 7, false); sbRand(4, 3, 7, 6, 3, 7, false);
            for (int l = 4; l <= 6; l++) { sbStair(l, 1, 4, 0, 1); sbStair(l, 2, 5, 0, 1); sbStair(l, 3, 6, 0, 1); }   // facing NORTH: costas ao sul
            bool eye[12], all = true; JRng er((int64_t)(p.bb[0] * 341873128712LL + p.bb[2] * 132897987541LL) ^ (int64_t)WORLD_SEED);   // por peca: igual em todos os chunks
            for (int k = 0; k < 12; k++) { eye[k] = er.next(24) / (float)(1 << 24) > .9f; all = all && eye[k]; }
            static const int FP[12][3] = {{4,3,8},{5,3,8},{6,3,8},{4,3,12},{5,3,12},{6,3,12},{3,3,9},{3,3,10},{3,3,11},{7,3,9},{7,3,10},{7,3,11}};
            if (gEndFrameId) for (int k = 0; k < 12; k++) sbPut(FP[k][0], FP[k][1], FP[k][2], gEndFrameId, eye[k] ? 4 : 0);
            if (all) for (int z = 9; z <= 11; z++) for (int x = 4; x <= 6; x++) sbPut(x, 3, z, XB_PORTAL, 2);
            if (gSpawnerId) sbPut(5, 3, 6, gSpawnerId);   // spawner de traca: AINDA FALTA (mob)
            break; }
        default: break;
    }
}
static void strongholdChunk(Chunk* c, int cx, int cz) {
    if (gShStone[2] == 0) { gShStone[2] = 1; const int a = pkFind("block/stonebrick_cracked"), b = pkFind("block/stonebrick_mossy"); gShStone[0] = a ? a : 0; gShStone[1] = b ? b : 0;
        gSpawnerId = gSpawnerId ? gSpawnerId : pkFind("block/mob_spawner") ? pkFind("block/mob_spawner") : pkFind("block/spawner"); }
    shInit(); const int x0 = cx * 16, z0 = cz * 16;
    for (int i = 0; i < (int)gShPos.size(); i++) {
        if (abs(gShPos[i].first - cx) > 10 || abs(gShPos[i].second - cz) > 10) continue;
        SHS* S = shGet(i); gSB = {c, x0, z0, nullptr};
        for (const SHP& p : S->ps) if (p.bb[3] >= x0 && p.bb[0] <= x0 + 15 && p.bb[5] >= z0 && p.bb[2] <= z0 + 15) shPiece(p);
    }
}
static bool shNearest(int px, int pz, int& x, int& z) {   // /locate stronghold: posicao (centro do chunk) da fortaleza mais proxima
    shInit(); long best = -1; for (auto& p : gShPos) { const int sx = p.first * 16 + 8, sz = p.second * 16 + 8; const long d = (long)(sx - px) * (sx - px) + (long)(sz - pz) * (sz - pz); if (best < 0 || d < best) { best = d; x = sx; z = sz; } }
    return best >= 0;
}
