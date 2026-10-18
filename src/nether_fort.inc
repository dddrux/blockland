// ---- Fortaleza do Nether 1.12.2 (patchNether.3): MapGenNetherBridge + StructureNetherBridgePieces, por pecas (lista deterministica por regiao 16x16 chunks) ----
enum { FK_AIR, FK_BRICK, FK_FENCE, FK_STAIR, FK_SOUL, FK_CHEST, FK_SPAWNER, FK_WART };
enum { P_START, P_BCROSS, P_BSTRAIGHT, P_BEND, P_RROOM, P_STAIRS, P_THRONE, P_CENTR, P_CORR, P_CCROSS, P_CLEFT, P_CRIGHT, P_CSTAIRS, P_CBAL, P_CSTALK, P_N };
struct FPiece { int t, dir, x0, y0, z0, x1, y1, z1, depth; bool chest; };
struct FortS { int sx, sz, sy = 0; std::vector<FPiece> ps; std::vector<int> pend; int cnt[P_N] = {0}; int last = -1; };
struct FW { int t, weight, maxc; bool inRow; };
static const FW FW_PRI[] = {{P_BSTRAIGHT, 30, 0, true}, {P_BCROSS, 10, 4, false}, {P_RROOM, 10, 4, false}, {P_STAIRS, 10, 3, false}, {P_THRONE, 5, 2, false}, {P_CENTR, 5, 1, false}};
static const FW FW_SEC[] = {{P_CORR, 25, 0, true}, {P_CCROSS, 15, 5, false}, {P_CRIGHT, 5, 10, false}, {P_CLEFT, 5, 10, false}, {P_CSTAIRS, 10, 3, true}, {P_CBAL, 7, 2, false}, {P_CSTALK, 5, 2, false}};
static const int FD[P_N][7] = {{-8,-3,0,19,10,19},{-8,-3,0,19,10,19},{-1,-3,0,5,10,19},{-1,-3,0,5,10,8},{-2,0,0,7,9,7},{-2,0,0,7,11,7},{-2,0,0,7,8,9},{-5,-3,0,13,14,13},{-1,0,0,5,7,5},{-1,0,0,5,7,5},{-1,0,0,5,7,5},{-1,0,0,5,7,5},{-1,-7,0,5,14,10},{-3,0,0,9,7,9},{-5,-3,0,13,14,13}};
static void fBox(int x, int y, int z, const int* d, int dir, int* o) {   // getComponentToAddBoundingBox
    const int ox = d[0], oy = d[1], oz = d[2], w = d[3], h = d[4], dp = d[5];
    if (dir == 0) { o[0] = x + ox; o[1] = y + oy; o[2] = z - dp + 1 + oz; o[3] = x + w - 1 + ox; o[4] = y + h - 1 + oy; o[5] = z + oz; }
    else if (dir == 2) { o[0] = x + ox; o[1] = y + oy; o[2] = z + oz; o[3] = x + w - 1 + ox; o[4] = y + h - 1 + oy; o[5] = z + dp - 1 + oz; }
    else if (dir == 3) { o[0] = x - dp + 1 + oz; o[1] = y + oy; o[2] = z + ox; o[3] = x + oz; o[4] = y + h - 1 + oy; o[5] = z + w - 1 + ox; }
    else { o[0] = x + oz; o[1] = y + oy; o[2] = z + ox; o[3] = x + dp - 1 + oz; o[4] = y + h - 1 + oy; o[5] = z + w - 1 + ox; }
}
static void fGen(FortS& S, JRng& r, int x, int y, int z, int dir, int depth, bool castle);
static void fChild(FortS& S, JRng& r, const FPiece& p, int kind, int a, int b, bool castle) {   // kind 0 = frente (offX=a, offY=b); 1/2 = esquerda/direita (offY=a, offX=b)
    int x, y, z, d;
    if (kind == 0) { y = p.y0 + b; if (p.dir == 0) { x = p.x0 + a; z = p.z0 - 1; } else if (p.dir == 2) { x = p.x0 + a; z = p.z1 + 1; } else if (p.dir == 3) { x = p.x0 - 1; z = p.z0 + a; } else { x = p.x1 + 1; z = p.z0 + a; } d = p.dir; }
    else if (kind == 1) { y = p.y0 + a; if (p.dir <= 2 && p.dir != 1) { x = p.x0 - 1; z = p.z0 + b; d = 3; } else if (p.dir == 0 || p.dir == 2) { x = p.x0 - 1; z = p.z0 + b; d = 3; } else { x = p.x0 + b; z = p.z0 - 1; d = 0; } }
    else { y = p.y0 + a; if (p.dir == 0 || p.dir == 2) { x = p.x1 + 1; z = p.z0 + b; d = 1; } else { x = p.x0 + b; z = p.z1 + 1; d = 2; } }
    fGen(S, r, x, y, z, d, p.depth, castle);
}
static void fBuild(FortS& S, JRng& r, int idx) {
    const FPiece p = S.ps[idx];
    switch (p.t) {
        case P_START: case P_BCROSS: fChild(S, r, p, 0, 8, 3, false); fChild(S, r, p, 1, 3, 8, false); fChild(S, r, p, 2, 3, 8, false); break;
        case P_BSTRAIGHT: fChild(S, r, p, 0, 1, 3, false); break;
        case P_RROOM: fChild(S, r, p, 0, 2, 0, false); fChild(S, r, p, 1, 0, 2, false); fChild(S, r, p, 2, 0, 2, false); break;
        case P_STAIRS: fChild(S, r, p, 2, 6, 2, false); break;
        case P_CENTR: fChild(S, r, p, 0, 5, 3, true); break;
        case P_CSTALK: fChild(S, r, p, 0, 5, 3, true); fChild(S, r, p, 1, 3, 5, true); break;
        case P_CORR: case P_CSTAIRS: fChild(S, r, p, 0, 1, 0, true); break;
        case P_CCROSS: fChild(S, r, p, 0, 1, 0, true); fChild(S, r, p, 1, 0, 1, true); fChild(S, r, p, 2, 0, 1, true); break;
        case P_CLEFT: fChild(S, r, p, 1, 0, 1, true); break;
        case P_CRIGHT: fChild(S, r, p, 2, 0, 1, true); break;
        case P_CBAL: { const int i = (p.dir == 3 || p.dir == 0) ? 5 : 1; const bool a = r.nextInt(8) > 0; fChild(S, r, p, 1, 0, i, a); const bool b = r.nextInt(8) > 0; fChild(S, r, p, 2, 0, i, b); break; }
    }
}
static void fGen(FortS& S, JRng& r, int x, int y, int z, int dir, int depth, bool castle) {
    if (depth > 30) return;
    const FPiece& st = S.ps[0]; int bb[6];
    auto add = [&](int t, bool chest) { FPiece p; p.t = t; p.dir = dir; p.x0 = bb[0]; p.y0 = bb[1]; p.z0 = bb[2]; p.x1 = bb[3]; p.y1 = bb[4]; p.z1 = bb[5]; p.depth = depth + 1; p.chest = chest; S.ps.push_back(p); S.pend.push_back((int)S.ps.size() - 1); };
    auto ok = [&]() { if (bb[1] <= 10) return false; for (const FPiece& q : S.ps) if (q.x1 >= bb[0] && q.x0 <= bb[3] && q.y1 >= bb[1] && q.y0 <= bb[4] && q.z1 >= bb[2] && q.z0 <= bb[5]) return false; return true; };
    auto filler = [&]() { fBox(x, y, z, FD[P_BEND], dir, bb); if (ok()) add(P_BEND, false); };
    if (std::abs(x - st.x0) > 112 || std::abs(z - st.z0) > 112) { filler(); return; }
    const FW* tb = castle ? FW_SEC : FW_PRI; const int n = castle ? 7 : 6;
    int tot = 0; bool lim = false; for (int i = 0; i < n; i++) { if (tb[i].maxc > 0 && S.cnt[tb[i].t] < tb[i].maxc) lim = true; tot += tb[i].weight; }
    if (!lim) tot = -1;
    for (int tries = 0; tries < 5 && tot > 0; tries++) {
        int j = r.nextInt(tot);
        for (int i = 0; i < n; i++) { j -= tb[i].weight; if (j < 0) {
            if (!(tb[i].maxc == 0 || S.cnt[tb[i].t] < tb[i].maxc) || (tb[i].t == S.last && !tb[i].inRow)) break;
            fBox(x, y, z, FD[tb[i].t], dir, bb);
            if (ok()) { const int t = tb[i].t; add(t, (t == P_CLEFT || t == P_CRIGHT) ? r.nextInt(3) == 0 : false); S.cnt[t]++; S.last = t; return; }
            break; } }
    }
    filler();
}
static void fortBuildStruct(FortS& S, JRng& r, int cx, int cz) {
    const int dir = r.nextInt(4); const int x = cx * 16 + 2, z = cz * 16 + 2; 
    FPiece p; p.t = P_START; p.dir = dir; p.x0 = x; p.y0 = 64; p.z0 = z; p.x1 = x + 18; p.y1 = 73; p.z1 = z + 18; p.depth = 0; p.chest = false; S.ps.push_back(p);
    fBuild(S, r, 0);
    while (!S.pend.empty()) { const int i = r.nextInt((int)S.pend.size()); const int id = S.pend[i]; S.pend.erase(S.pend.begin() + i); fBuild(S, r, id); }
    int ymin = 9999, ymax = -9999; for (const FPiece& q : S.ps) { ymin = std::min(ymin, q.y0); ymax = std::max(ymax, q.y1); }
    const int sz = ymax - ymin + 1, k = 70 - 48 + 1 - sz; const int j = k > 1 ? 48 + r.nextInt(k) : 48; const int off = j - ymin;
    for (FPiece& q : S.ps) { q.y0 += off; q.y1 += off; } S.sy = off;
}
static bool fortRegionStart(int i, int j, int* scx, int* scz, JRng& r) {
    r.setSeed((int64_t)(int)(i ^ (j << 4)) ^ (int64_t)WORLD_SEED); r.nextInt();
    const int rate = gsi("fort", 33);
    if (rate <= 0) return false; if (rate == 33) { if (r.nextInt(3) != 0) return false; } else if (r.nextInt(100) >= rate) return false;
    *scx = (i << 4) + 4 + r.nextInt(8); *scz = (j << 4) + 4 + r.nextInt(8); return true;
}
static std::map<std::pair<int, int>, FortS*> gFortCache; static int64_t gFortSeed = 0; static int gFortRateC = -1;
static FortS* fortAtRegion(int i, int j) {
    const int rate = gsi("fort", 33); if (gFortSeed != (int64_t)WORLD_SEED || gFortRateC != rate) { for (auto& kv : gFortCache) delete kv.second; gFortCache.clear(); gFortSeed = (int64_t)WORLD_SEED; gFortRateC = rate; }
    auto it = gFortCache.find({i, j}); if (it != gFortCache.end()) return it->second;
    JRng r; int scx, scz; FortS* S = nullptr;
    if (fortRegionStart(i, j, &scx, &scz, r)) { S = new FortS(); S->sx = scx; S->sz = scz; fortBuildStruct(*S, r, scx, scz); }
    gFortCache[{i, j}] = S; return S;
}
// fortalezas que tocam a area [x0..x1] x [z0..z1] (coordenadas de bloco)
static void fortNear(int x0, int z0, int x1, int z1, std::vector<FortS*>& out) {
    for (int i = ((x0 >> 4) - 8) >> 4; i <= ((x1 >> 4) + 8) >> 4; i++) for (int j = ((z0 >> 4) - 8) >> 4; j <= ((z1 >> 4) + 8) >> 4; j++) { FortS* s = fortAtRegion(i, j); if (s) out.push_back(s); }
}
// ---- rasterizacao: coordenadas locais da peca -> mundo (como StructureComponent.getXWithOffset), recortadas ao chunk ----
struct FR { int cx0, cx1, cz0, cz1; void* ud; void (*put)(void*, int, int, int, int, int); int (*get)(void*, int, int, int); };
static FR gFR; static const FPiece* gFP;
static inline void fw(const FPiece& p, int lx, int lz, int& wx, int& wz) {
    if (p.dir == 0) { wx = p.x0 + lx; wz = p.z1 - lz; } else if (p.dir == 2) { wx = p.x0 + lx; wz = p.z0 + lz; }
    else if (p.dir == 3) { wx = p.x1 - lz; wz = p.z0 + lx; } else { wx = p.x0 + lz; wz = p.z0 + lx; }
}
static void fPut(int lx, int ly, int lz, int kind, int meta = 0) {
    const FPiece& p = *gFP; int wx, wz; fw(p, lx, lz, wx, wz);
    if (wx < gFR.cx0 || wx > gFR.cx1 || wz < gFR.cz0 || wz > gFR.cz1) return; gFR.put(gFR.ud, wx, p.y0 + ly, wz, kind, meta);
}
static void fFill(int ax, int ay, int az, int bx, int by, int bz, int kind, int inner = -2) {
    const FPiece& p = *gFP; int w0, z0, w1, z1; fw(p, ax, az, w0, z0); fw(p, bx, bz, w1, z1);
    if (std::max(w0, w1) < gFR.cx0 || std::min(w0, w1) > gFR.cx1 || std::max(z0, z1) < gFR.cz0 || std::min(z0, z1) > gFR.cz1) return;
    for (int y = ay; y <= by; y++) for (int z = az; z <= bz; z++) for (int x = ax; x <= bx; x++) {
        const bool edge = x == ax || x == bx || y == ay || y == by || z == az || z == bz; fPut(x, y, z, (inner == -2 || edge) ? kind : inner); }
}
static void fDown(int lx, int ly, int lz) {   // replaceAirAndLiquidDownwards: pilar de tijolo ate o chao
    const FPiece& p = *gFP; int wx, wz; fw(p, lx, lz, wx, wz); if (wx < gFR.cx0 || wx > gFR.cx1 || wz < gFR.cz0 || wz > gFR.cz1 || !gFR.get) return;
    for (int y = p.y0 + ly; y > 1; y--) { if (gFR.get(gFR.ud, wx, y, wz) == 0) gFR.put(gFR.ud, wx, y, wz, FK_BRICK, 0); else break; }
}
static void fStair(int lx, int ly, int lz, int bdx, int bdz, bool flip = false) {   // costas da escada na direcao local (bdx,bdz)
    const FPiece& p = *gFP; int dx, dz; if (p.dir == 0) { dx = bdx; dz = -bdz; } else if (p.dir == 2) { dx = bdx; dz = bdz; } else if (p.dir == 3) { dx = -bdz; dz = bdx; } else { dx = bdz; dz = bdx; }
    const int s = dz < 0 ? 0 : dx > 0 ? 1 : dz > 0 ? 2 : 3; fPut(lx, ly, lz, FK_STAIR, s | (flip ? 4 : 0));
}
static void fPiece(const FPiece& p) {
    gFP = &p; const int B = FK_BRICK, A = FK_AIR, F = FK_FENCE;
    switch (p.t) {
    case P_START: case P_BCROSS:
        fFill(7,3,0,11,4,18,B); fFill(0,3,7,18,4,11,B); fFill(8,5,0,10,7,18,A); fFill(0,5,8,18,7,10,A);
        fFill(7,5,0,7,5,7,B); fFill(7,5,11,7,5,18,B); fFill(11,5,0,11,5,7,B); fFill(11,5,11,11,5,18,B);
        fFill(0,5,7,7,5,7,B); fFill(11,5,7,18,5,7,B); fFill(0,5,11,7,5,11,B); fFill(11,5,11,18,5,11,B);
        fFill(7,6,0,7,6,7,F); fFill(7,6,11,7,6,18,F); fFill(11,6,0,11,6,7,F); fFill(11,6,11,11,6,18,F);
        fFill(0,6,7,7,6,7,F); fFill(11,6,7,18,6,7,F); fFill(0,6,11,7,6,11,F); fFill(11,6,11,18,6,11,F);
        fFill(7,2,0,11,2,5,B); fFill(7,2,13,11,2,18,B); fFill(7,0,0,11,1,3,B); fFill(7,0,15,11,1,18,B);
        fFill(0,2,7,5,2,11,B); fFill(13,2,7,18,2,11,B); fFill(0,0,7,3,1,11,B); fFill(15,0,7,18,1,11,B);
        for (int i = 7; i <= 11; i++) for (int j = 0; j <= 2; j++) { fDown(i,-1,j); fDown(i,-1,18-j); fDown(j,-1,i); fDown(18-j,-1,i); }
        break;
    case P_BSTRAIGHT:
        fFill(0,3,0,4,4,18,B); fFill(1,5,0,3,7,18,A); fFill(0,5,0,0,5,18,B); fFill(4,5,0,4,5,18,B); fFill(0,6,0,0,6,18,F); fFill(4,6,0,4,6,18,F);
        fFill(0,2,0,4,2,5,B); fFill(0,2,13,4,2,18,B); fFill(0,0,0,4,1,3,B); fFill(0,0,15,4,1,18,B);
        for (int i = 0; i <= 4; i++) for (int j = 0; j <= 2; j++) { fDown(i,-1,j); fDown(i,-1,18-j); }
        break;
    case P_BEND:
        fFill(0,3,0,4,4,7,B); fFill(1,5,0,3,7,7,A); fFill(0,5,0,0,5,7,B); fFill(4,5,0,4,5,7,B); fFill(0,6,0,0,6,7,F); fFill(4,6,0,4,6,7,F);
        fFill(0,5,7,4,5,7,B); fFill(0,6,7,4,6,7,F); fFill(0,2,0,4,2,2,B); fFill(0,2,4,4,2,7,B); fFill(0,0,0,4,1,1,B);
        for (int i = 0; i <= 4; i++) for (int j = 0; j <= 1; j++) fDown(i,-1,j);
        break;
    case P_RROOM:
        fFill(0,0,0,6,1,6,B); fFill(0,2,0,6,7,6,A);
        fFill(0,2,0,1,6,0,B); fFill(0,2,6,1,6,6,B); fFill(5,2,0,6,6,0,B); fFill(5,2,6,6,6,6,B);
        fFill(0,2,0,0,6,1,B); fFill(0,2,5,0,6,6,B); fFill(6,2,0,6,6,1,B); fFill(6,2,5,6,6,6,B);
        fFill(2,6,0,4,6,0,B); fFill(2,5,0,4,5,0,F); fFill(2,6,6,4,6,6,B); fFill(2,5,6,4,5,6,F);
        fFill(0,6,2,0,6,4,B); fFill(0,5,2,0,5,4,F); fFill(6,6,2,6,6,4,B); fFill(6,5,2,6,5,4,F);
        fFill(0,7,0,6,7,6,B);
        for (int i = 0; i <= 6; i++) for (int j = 0; j <= 6; j++) fDown(i,-1,j);
        break;
    case P_STAIRS:
        fFill(0,0,0,6,1,6,B); fFill(0,2,0,6,10,6,A); fFill(0,2,0,6,10,6,B,A);
        fFill(2,2,0,4,5,0,A); fFill(6,8,2,6,10,4,A);
        for (int i = 0; i < 5; i++) { fFill(1,2,1+i,1,1+i,1+i,B); fPut(1,2+i,1+i,FK_AIR); fStair(1,2+i,1+i,0,-1); fFill(1,2,1+i,1,1+i,1+i,B); }
        fFill(1,7,6,5,7,6,B); fFill(2,7,3,5,7,5,B); fFill(0,9,2,0,9,4,F);
        for (int i = 0; i <= 6; i++) for (int j = 0; j <= 6; j++) fDown(i,-1,j);
        break;
    case P_THRONE:
        fFill(0,2,0,6,7,7,A); fFill(1,0,0,5,1,7,B); fFill(1,2,1,5,2,7,B); fFill(1,3,2,5,3,7,B); fFill(1,4,3,5,4,7,B);
        fFill(1,2,0,1,4,2,B); fFill(5,2,0,5,4,2,B); fFill(1,5,2,1,5,3,B); fFill(5,5,2,5,5,3,B); fFill(0,5,3,0,5,8,B); fFill(6,5,3,6,5,8,B); fFill(1,5,8,5,5,8,B);
        fFill(0,6,3,0,6,8,F); fFill(6,6,3,6,6,8,F); fFill(1,6,8,5,6,8,F); fPut(1,6,3,F); fPut(5,6,3,F);
        fPut(3,5,5,FK_SPAWNER);
        for (int i = 1; i <= 5; i++) for (int j = 0; j <= 7; j++) fDown(i,-1,j);
        break;
    case P_CENTR: case P_CSTALK: {
        fFill(0,3,0,12,13,12,B,A); fFill(1,3,1,11,4,11,B);
        fFill(5,5,0,7,8,0,A); fFill(5,5,12,7,8,12,A);
        for (int k = 0; k < 4; k++) { const int px = (k & 1) ? 11 : 1, pz = (k & 2) ? 11 : 1; fFill(px,5,pz,px,12,pz,B); }
        for (int s = 0; s < 4; s++) { const int px = (s & 1) ? 12 : 0; fFill(px,6,3,px,8,4,F); fFill(px,6,8,px,8,9,F); }
        if (p.t == P_CSTALK) { fFill(0,5,5,0,8,7,A); for (int x = 2; x <= 10; x++) for (int z = 2; z <= 10; z++) if (x < 5 || x > 7) { fPut(x,5,z,FK_SOUL); fPut(x,6,z,FK_WART); } }
        else { fFill(4,5,4,8,5,8,B); fFill(5,6,5,7,6,7,B); fFill(1,13,1,11,13,11,B); }
        for (int i = 0; i <= 12; i++) for (int j = 0; j <= 12; j++) fDown(i,2,j);
        break; }
    case P_CORR: case P_CCROSS: case P_CLEFT: case P_CRIGHT: {
        fFill(0,0,0,4,1,4,B); fFill(0,2,0,4,5,4,B,A); fFill(0,6,0,4,6,4,B);
        if (p.t != P_CLEFT && p.t != P_CRIGHT) { fFill(1,2,4,3,5,4,A); fFill(1,2,0,3,5,0,A); } else fFill(1,2,0,3,5,0,A);
        if (p.t == P_CCROSS || p.t == P_CLEFT) fFill(0,2,1,0,5,3,A);
        if (p.t == P_CCROSS || p.t == P_CRIGHT) fFill(4,2,1,4,5,3,A);
        if (p.t == P_CORR) { fFill(0,3,2,0,4,2,F); fFill(4,3,2,4,4,2,F); }
        if (p.chest) fPut(p.t == P_CLEFT ? 3 : 1, 2, 3, FK_CHEST);
        for (int i = 0; i <= 4; i++) for (int j = 0; j <= 4; j++) fDown(i,-1,j);
        break; }
    case P_CSTAIRS: {
        for (int z = 0; z < 10; z++) { const int f = std::max(0, 8 - z);
            fFill(0,0,z,4,f,z,B); fFill(1,f+1,z,3,f+5,z,A); fFill(0,f+1,z,0,f+6,z,B); fFill(4,f+1,z,4,f+6,z,B); fFill(1,f+6,z,3,f+6,z,B);
            if (z >= 1 && z <= 7) for (int x = 1; x <= 3; x++) fStair(x,f,z,0,-1); }
        fFill(1,8,0,3,12,0,A); for (int i = 0; i <= 4; i++) fDown(i,-1,9);
        break; }
    case P_CBAL:
        fFill(0,0,0,8,1,8,B); fFill(0,2,0,8,5,8,B,A); fFill(0,6,0,8,6,8,B);
        fFill(3,2,0,5,5,0,A); fFill(0,2,1,0,5,3,A); fFill(8,2,1,8,5,3,A); fFill(0,2,5,0,5,7,A); fFill(8,2,5,8,5,7,A);
        fFill(2,3,8,6,4,8,F); fFill(2,2,8,2,5,8,B); fFill(6,2,8,6,5,8,B);
        for (int i = 0; i <= 8; i++) for (int j = 0; j <= 8; j++) fDown(i,-1,j);
        break;
    }
}
// ---- escrita no chunk ----
struct FCk { Chunk* c; int y0, y1; };
static int gNBrickId2 = 0;
static void fkPut(void* u, int wx, int wy, int wz, int kind, int meta) {
    FCk* k = (FCk*)u; if (wy < 1 || wy > 120) return; const int x = wx & 15, z = wz & 15; BT& t = k->c->b[idx(x, wy, z)]; if (t == BEDROCK) return; uint8_t& m = k->c->m[idx(x, wy, z)]; m = 0;
    switch (kind) {
        case FK_AIR: t = (BT)AIR; break; case FK_BRICK: t = (BT)gNBrickId; break;
        case FK_FENCE: t = (BT)XB_FENCE; m = (uint8_t)(6 << 4); break;
        case FK_STAIR: t = (BT)XB_STAIR; m = (uint8_t)(meta | (11 << 3)); break;
        case FK_SOUL: t = (BT)gNSoul; break; case FK_WART: t = (BT)XB_CROP; m = (uint8_t)((4 << 3) | (nhash(wx, wy, wz, 55) & 3)); break; case FK_SPAWNER: t = (BT)gSpawnerId; break;
        case FK_CHEST: t = (BT)XB_CHEST; gFortChests.push_back({wx, wy, wz}); break;
    }
}
static int fkGet(void* u, int wx, int wy, int wz) { FCk* k = (FCk*)u; const BT t = k->c->b[idx(wx & 15, wy, wz & 15)]; return (t == AIR || t == LAVA) ? 0 : 1; }
// tipo de bloco da fortaleza numa celula (para conectar cercas entre chunks): -1 nada, senao FK_*
static int gPrKind, gPrY;
static void probePut(void*, int, int y, int, int kind, int) { if (y == gPrY) gPrKind = kind; }
static int fortProbe(int wx, int wy, int wz, const std::vector<FortS*>& fs) {
    gPrKind = -1; gPrY = wy; gFR = {wx, wx, wz, wz, nullptr, probePut, nullptr};
    for (FortS* s : fs) for (const FPiece& p : s->ps) if (wx >= p.x0 && wx <= p.x1 && wz >= p.z0 && wz <= p.z1 && wy >= p.y0 && wy <= p.y1) fPiece(p);
    return gPrKind;
}
static void netherFortress(Chunk* c, int cx, int cz) {
    gNBrickId = gNBrickId ? gNBrickId : (pkFind("block/nether_bricks") ? pkFind("block/nether_bricks") : pkFind("block/nether_brick") ? pkFind("block/nether_brick") : BRICKS);
    gSpawnerId = pkFind("block/mob_spawner") ? pkFind("block/mob_spawner") : pkFind("block/spawner") ? pkFind("block/spawner") : gNBrickId;
    std::vector<FortS*> fs; fortNear(cx * 16, cz * 16, cx * 16 + 15, cz * 16 + 15, fs); if (fs.empty()) return;
    FCk k{c, 0, 0}; gFR = {cx * 16, cx * 16 + 15, cz * 16, cz * 16 + 15, &k, fkPut, fkGet}; bool any = false;
    for (FortS* s : fs) for (const FPiece& p : s->ps) if (p.x1 >= cx * 16 && p.x0 <= cx * 16 + 15 && p.z1 >= cz * 16 && p.z0 <= cz * 16 + 15) { fPiece(p); any = true; }
    if (!any) return;
    for (int y = 1; y < 121; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {   // conexao das cercas (inclui vizinhos em outros chunks)
        if (c->b[idx(x, y, z)] != XB_FENCE) continue; int mk = 0;
        for (int d = 0; d < 4; d++) { const int nx = x + RS_DX[d], nz = z + RS_DZ[d]; bool con;
            if (nx >= 0 && nx < 16 && nz >= 0 && nz < 16) { const BT t = c->b[idx(nx, y, nz)]; con = t == XB_FENCE || t == (BT)gNBrickId; }
            else { const int kd = fortProbe(cx * 16 + nx, y, cz * 16 + nz, fs); con = kd == FK_FENCE || kd == FK_BRICK; }
            if (con) mk |= 1 << d; }
        c->m[idx(x, y, z)] = (uint8_t)(mk | (6 << 4)); }
}
// loot dos baus (1.12.2: tabela NETHER_BRIDGE simplificada); deterministico pela posicao
static void fortChestFlush() {
    for (size_t i = 0; i < gFortChests.size();) {
        const auto p = gFortChests[i]; if (getB(p[0], p[1], p[2]) != XB_CHEST) { if (!getC(fdiv(p[0], CW), fdiv(p[2], CW))) { ++i; continue; } gFortChests.erase(gFortChests.begin() + i); continue; }
        ChestInv* ci = chestAt(p[0], p[1], p[2], true);
        if (ci && ci->empty()) {
            uint32_t h = nhash(p[0], p[1], p[2], 99); auto R = [&](int n) { h = h * 1664525u + 1013904223u; return (int)((h >> 10) % (uint32_t)n); };
            static const int sad = pkFind("item/saddle"), obs = pkFind("block/obsidian"), gsw = pkFind("item/golden_sword"), fns = pkFind("item/flint_and_steel"), ghr = pkFind("item/golden_horse_armor");
            for (int k = 0, n = 2 + R(3); k < n; k++) { const int w = R(100);
                if (w < 5) chestAdd(*ci, IT_DIAMOND, 1); else if (w < 10) chestAdd(*ci, IT_IRON, 1 + R(3)); else if (w < 25) chestAdd(*ci, IT_GOLD, 1 + R(3));
                else if (w < 30 && gsw) chestAdd(*ci, gsw, 1); else if (w < 35 && fns) chestAdd(*ci, fns, 1); else if (w < 45 && sad) chestAdd(*ci, sad, 1);
                else if (w < 53 && ghr) chestAdd(*ci, ghr, 1); else if (w < 55 && obs) chestAdd(*ci, obs, 2 + R(3)); else chestAdd(*ci, IT_GOLD, 1); }
        }
        gFortChests.erase(gFortChests.begin() + i);
    }
}
// posicoes dos spawners (sala do trono) das fortalezas perto de (x,z)
static void fortSpawners(double px, double pz, std::vector<std::array<int, 3>>& out) {
    std::vector<FortS*> fs; fortNear((int)px - 20, (int)pz - 20, (int)px + 20, (int)pz + 20, fs);
    for (FortS* s : fs) for (const FPiece& p : s->ps) if (p.t == P_THRONE) { int wx, wz; fw(p, 3, 5, wx, wz); out.push_back({wx, p.y0 + 5, wz}); }
}
