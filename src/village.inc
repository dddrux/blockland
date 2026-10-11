// ---------- vilas (v37): LOCALIZAÇÃO idêntica ao MapGenVillage 1.12.2 (espaçamento 32, semente 10387312, biomas); layout simplificado ----------
struct VPiece { int x0, z0, w, d, face, g, kind, H, f, t; };   // f: facing 0N 1L 2S 3O; t: tipo do StructureVillagePieces (ver VPc)   // face: direção da parede da porta 0:+x 1:-x 2:+z 3:-z; kind 0 poço,1 casa,2 estrada,3 biblioteca,4 igreja,5 campo,6 torre/poste
struct Village { int bio; std::vector<VPiece> p; };
static std::unordered_map<int64_t, Village> gVil;
static int vilGround(int wx, int wz) { const int fx = fdiv(wx, CW), fz = fdiv(wz, CW); return floorOf(fx, fz)[(wz - fz * 16) * 16 + (wx - fx * 16)]; }
static inline bool vilBiome(int b) { return b == B_PLAINS || b == B_DESERT || b == B_SAVANNA || b == B_TAIGA; }
static bool vilStart(int rx, int rz, int& sx, int& sz) {   // MapGenStructure.canSpawnStructureAtCoords: região 32x32 chunks
    JR2 r((int64_t)rx * 341873128712LL + (int64_t)rz * 132897987541LL + (int64_t)WORLD_SEED + 10387312LL);
    sx = rx * 32 + r.nextInt(24); sz = rz * 32 + r.nextInt(24);
    return vilBiome(biomeAt((float)(sx * 16 + 8), (float)(sz * 16 + 8)));
}
// ---- StructureVillagePieces: crescimento do layout (peças, tamanhos, pesos, limites e ordem de sorteios do 1.12.2) ----
struct VPc { int x0, z0, x1, z1, f, t, H, len; };   // f: 0 N,1 L(E),2 S,3 O(W) = EnumFacing.Plane.HORIZONTAL; t: 0 poço,1 Path,2 Garden,3 Church,4 House1,5 WoodHut,6 Hall,7 Field1,8 Field2,9 House2,10 House3,11 Torch
struct VPW { int t, wt, lim, sp, W, H, D; };
static int vgInt(JR2& r, int a, int b) { return a >= b ? a : r.nextInt(b - a + 1) + a; }
static void vgBox(int x, int z, int W, int D, int f, int& x0, int& z0, int& x1, int& z1) {   // getComponentToAddBoundingBox (offsets 0)
    if (f == 0) { x0 = x; x1 = x + W - 1; z0 = z - D + 1; z1 = z; } else if (f == 2) { x0 = x; x1 = x + W - 1; z0 = z; z1 = z + D - 1; }
    else if (f == 3) { x0 = x - D + 1; x1 = x; z0 = z; z1 = z + W - 1; } else { x0 = x; x1 = x + D - 1; z0 = z; z1 = z + W - 1; }
}
struct VGen {
    JR2 r; std::vector<VPc> L; std::vector<VPW> W; std::vector<int> roads, houses; int sx0, sz0, last = -1;
    bool free(int x0, int z0, int x1, int z1) { for (auto& p : L) if (!(x1 < p.x0 || x0 > p.x1 || z1 < p.z0 || z0 > p.z1)) return false; return true; }
    bool isNear(int x, int z) { return abs(x - sx0) <= 112 && abs(z - sz0) <= 112; }
    int addPath(int x, int z, int f) {
        if (!isNear(x, z)) return -1;
        for (int i = 7 * vgInt(r, 3, 5); i >= 7; i -= 7) { int a, b, c, d; vgBox(x, z, 3, i, f, a, b, c, d);
            if (free(a, b, c, d)) { if (!vilBiome(biomeAt((float)((a + c) / 2), (float)((b + d) / 2)))) return -1; L.push_back({a, b, c, d, f, 1, 3, std::max(c - a + 1, d - b + 1)}); roads.push_back((int)L.size() - 1); return (int)L.size() - 1; } }
        return -1;
    }
    int addHouse(int x, int z, int f) {
        if (!isNear(x, z)) return -1;
        int tot = 0; bool any = false; for (auto& w : W) { if (w.lim > 0 && w.sp < w.lim) any = true; tot += w.wt; }
        if (!any) return -1;   // updatePieceWeight <= 0: sem peça (nem poste)
        int made = -1;
        if (any) for (int tries = 0; tries < 5 && made < 0; tries++) { int k = r.nextInt(tot);
            for (size_t i = 0; i < W.size(); i++) { k -= W[i].wt; if (k < 0) { if ((W[i].lim != 0 && W[i].sp >= W[i].lim) || ((int)i == last && W.size() > 1)) break;
                int a, b, c, d; vgBox(x, z, W[i].W, W[i].D, f, a, b, c, d);
                if (free(a, b, c, d)) { const int t = W[i].t; if (t == 2) r.nextInt(2); else if (t == 5) { r.nextInt(2); r.nextInt(3); } else if (t == 7) for (int q = 0; q < 4; q++) r.nextInt(10); else if (t == 8) for (int q = 0; q < 2; q++) r.nextInt(10);
                    L.push_back({a, b, c, d, f, t, W[i].H, 0}); made = (int)L.size() - 1; W[i].sp++; last = (int)i; if (W[i].lim != 0 && W[i].sp >= W[i].lim) { W.erase(W.begin() + i); last = -1; } break; } } } }
        if (made < 0) { int a, b, c, d; vgBox(x, z, 3, 2, f, a, b, c, d); if (!free(a, b, c, d)) return -1; L.push_back({a, b, c, d, f, 11, 4, 0}); made = (int)L.size() - 1; }
        houses.push_back(made); return made;
    }
    int nn(const VPc& p, int y, int x) { (void)y; return (p.f == 0 || p.f == 2) ? addHouse(p.x0 - 1, p.z0 + x, 3) : addHouse(p.x0 + x, p.z0 - 1, 0); }
    int pp(const VPc& p, int y, int x) { (void)y; return (p.f == 0 || p.f == 2) ? addHouse(p.x1 + 1, p.z0 + x, 1) : addHouse(p.x0 + x, p.z1 + 1, 2); }
    void buildPath(int idx) {
        const VPc p = L[idx]; bool flag = false;
        for (int i = r.nextInt(5); i < p.len - 8; i += 2 + r.nextInt(5)) { const int c = nn(p, 0, i); if (c >= 0) { i += std::max(L[c].x1 - L[c].x0 + 1, L[c].z1 - L[c].z0 + 1); flag = true; } }
        for (int j = r.nextInt(5); j < p.len - 8; j += 2 + r.nextInt(5)) { const int c = pp(p, 0, j); if (c >= 0) { j += std::max(L[c].x1 - L[c].x0 + 1, L[c].z1 - L[c].z0 + 1); flag = true; } }
        if (flag && r.nextInt(3) > 0) { if (p.f == 0) addPath(p.x0 - 1, p.z0, 3); else if (p.f == 2) addPath(p.x0 - 1, p.z1 - 3, 3); else if (p.f == 3) addPath(p.x0, p.z0 - 1, 0); else addPath(p.x1 - 3, p.z0 - 1, 0); }
        if (flag && r.nextInt(3) > 0) { if (p.f == 0) addPath(p.x1 + 1, p.z0, 1); else if (p.f == 2) addPath(p.x1 + 1, p.z1 - 3, 1); else if (p.f == 3) addPath(p.x0, p.z1 + 1, 2); else addPath(p.x1 - 3, p.z1 + 1, 2); }
    }
};
static std::unordered_map<int64_t, char> gVilOk;   // so o layout (sem terreno): 1 = vila valida (>2 pecas), 0 = nao
static Village gVilDummy;
// layoutOnly = true: so verifica se a vila existe (posicao + bioma + layout), SEM gerar terreno (o /locate usava a versao completa e gerava o terreno de cada vila candidata = travava o jogo, como o locate do MC que so olha as posicoes de inicio)
static const Village* vilGet(int rx, int rz, bool layoutOnly = false) {
    const int64_t k = key(rx, rz); auto it = gVil.find(k); if (it != gVil.end()) return it->second.p.empty() ? nullptr : &it->second;
    if (layoutOnly) { auto o = gVilOk.find(k); if (o != gVilOk.end()) return o->second ? &gVilDummy : nullptr; }
    Village* vp = layoutOnly ? &gVilDummy : &gVil[k]; Village& v = *vp; int sx, sz; if (!vilStart(rx, rz, sx, sz)) { if (layoutOnly) gVilOk[k] = 0; return nullptr; }
    JR2 r((int64_t)WORLD_SEED); const int64_t a = r.nextLong(), b = r.nextLong();   // MapGenBase.generate: setLargeFeatureSeed
    r.setSeed((int64_t)((uint64_t)(int64_t)sx * (uint64_t)a ^ (uint64_t)(int64_t)sz * (uint64_t)b ^ (uint64_t)(int64_t)WORLD_SEED));
    r.nextInt();   // MapGenStructure.recursiveGenerate consome um nextInt antes
    VGen g{r}; g.W = {{2, 4, vgInt(g.r, 2, 4), 0, 5, 6, 5}, {3, 20, vgInt(g.r, 0, 1), 0, 5, 12, 9}, {4, 20, vgInt(g.r, 0, 2), 0, 9, 9, 6}, {5, 3, vgInt(g.r, 2, 5), 0, 4, 6, 5},
        {6, 15, vgInt(g.r, 0, 2), 0, 9, 7, 11}, {7, 3, vgInt(g.r, 1, 4), 0, 13, 4, 9}, {8, 3, vgInt(g.r, 2, 4), 0, 7, 4, 9}, {9, 15, vgInt(g.r, 0, 1), 0, 10, 6, 7}, {10, 8, vgInt(g.r, 0, 3), 0, 9, 7, 12}};
    for (size_t i = g.W.size(); i-- > 0;) if (g.W[i].lim == 0) g.W.erase(g.W.begin() + i);
    const int wf = g.r.nextInt(4), X0 = sx * 16 + 2, Z0 = sz * 16 + 2; g.sx0 = X0; g.sz0 = Z0;   // poço: direção aleatória, caixa 6x6
    g.L.push_back({X0, Z0, X0 + 5, Z0 + 5, wf, 0, 15, 0});
    g.addPath(X0 - 1, Z0 + 1, 3); g.addPath(X0 + 6, Z0 + 1, 1); g.addPath(X0 + 1, Z0 - 1, 0); g.addPath(X0 + 1, Z0 + 6, 2);   // Well.buildComponent: O, L, N, S
    while (!g.roads.empty() || !g.houses.empty()) {
        if (g.roads.empty()) { const int i = g.r.nextInt((int)g.houses.size()); g.houses.erase(g.houses.begin() + i); }   // casas não geram filhos
        else { const int i = g.r.nextInt((int)g.roads.size()); const int id = g.roads[i]; g.roads.erase(g.roads.begin() + i); g.buildPath(id); }
    }
    int nonRoad = 0; for (auto& p : g.L) if (p.t != 1) nonRoad++;
    if (nonRoad <= 2) { if (layoutOnly) gVilOk[k] = 0; return nullptr; }   // hasMoreThanTwoComponents
    if (layoutOnly) { gVilOk[k] = 1; return &gVilDummy; }
    v.bio = biomeAt((float)(sx * 16 + 8), (float)(sz * 16 + 8));
    for (auto& p : g.L) {
        const int w = p.x1 - p.x0 + 1, d = p.z1 - p.z0 + 1, face = p.f == 3 ? 0 : p.f == 1 ? 1 : p.f == 0 ? 2 : 3;   // porta voltada para a estrada (lado oposto ao crescimento)
        const int kind = p.t == 0 ? 0 : p.t == 1 ? 2 : p.t == 4 ? 3 : p.t == 3 ? 4 : (p.t == 7 || p.t == 8) ? 5 : p.t == 11 ? 6 : 1;
        int g = vilGround(p.x0 + w / 2, p.z0 + d / 2);
        if (p.t != 1) { int sum = 0; for (int x = p.x0; x <= p.x1; x++) for (int z = p.z0; z <= p.z1; z++) sum += std::max(vilGround(x, z) + 1, SEA); g = sum / (w * d); if (p.t == 0) g -= 11; }   // getAverageGroundLevel: minY da peca (poco: minY = avg-11)
        v.p.push_back({p.x0, p.z0, w, d, face, g, kind, p.H, p.f, p.t});
    }
    return &v;
}
#include "village_build.inc"
static void placeVillages(Chunk* c, int cx, int cz) {
    auto put = [&](int wx, int y, int wz, BT b) { const int x = wx - cx * 16, z = wz - cz * 16; if (x < 0 || x > 15 || z < 0 || z > 15 || y < 1 || y > 250) return; c->b[idx(x, y, z)] = b; };
    for (int rz = fdiv(cz - 8, 32); rz <= fdiv(cz + 8, 32); rz++) for (int rx = fdiv(cx - 8, 32); rx <= fdiv(cx + 8, 32); rx++) {
        const Village* v = vilGet(rx, rz); if (!v) continue;
        const bool des = v->bio == B_DESERT; const BT wall = des ? (BT)SANDSTONE : (BT)PLANKS, base = des ? (BT)SANDSTONE : (BT)COBBLE, corner = des ? (BT)SANDSTONE : (BT)LOG, path = des ? (BT)SANDSTONE : (BT)GRAVEL;
        const int glass = pkFind("block/glass");
        for (auto& p : v->p) {
            if (p.x0 > cx * 16 + 15 || p.x0 + p.w - 1 < cx * 16 || p.z0 > cz * 16 + 15 || p.z0 + p.d - 1 < cz * 16) continue;
            if (p.t != 1) { vilBuild(c, cx, cz, p, v->bio); continue; }   // pecas portadas do 1.12.2 (village_build.inc)
            if (p.g <= SEA) continue;   // nada na água
            const int g = p.g;
            if (p.kind == 2 || p.kind == 5 || p.kind == 6) {   // estrada (segue o terreno), campo, poste
                for (int x = p.x0; x < p.x0 + p.w; x++) for (int z = p.z0; z < p.z0 + p.d; z++) { const int gg = vilGround(x, z); if (gg <= SEA) continue;
                    const int lx = x - p.x0, lz = z - p.z0;
                    if (p.kind == 2) {   // Path (1.12.2): grama (com ar em cima) -> grass path; liquido -> tabuas; areia/arenito -> cascalho + cobble embaixo; nada acima eh limpo
                        const int bx = x - cx * 16, bz = z - cz * 16; const BT cur = (bx >= 0 && bx < 16 && bz >= 0 && bz < 16) ? c->b[idx(bx, gg, bz)] : (BT)AIR, up = (bx >= 0 && bx < 16 && bz >= 0 && bz < 16) ? c->b[idx(bx, gg + 1, bz)] : (BT)AIR;
                        static const int gpath = pkFind("block/dirt_path") ? pkFind("block/dirt_path") : pkFind("block/grass_path");
                        if (cur == GRASS && up == AIR) put(x, gg, z, gpath ? (BT)gpath : path);
                        else if (isWater(cur)) put(x, gg, z, des ? (BT)SANDSTONE : (BT)PLANKS);
                        else if (cur == SAND || cur == SANDSTONE) { put(x, gg, z, des ? (BT)SANDSTONE : (BT)GRAVEL); put(x, gg - 1, z, des ? (BT)SANDSTONE : (BT)COBBLE); }
                        continue;
                    }
                    else if (p.kind == 5) { const bool edge = lx == 0 || lz == 0 || lx == p.w - 1 || lz == p.d - 1; put(x, gg, z, edge ? corner : (lz == p.d / 2 ? (BT)WATER : (BT)DIRT)); }
                    else { put(x, gg, z, path); if (lx == 1 && lz == 0) { put(x, gg + 1, z, corner); put(x, gg + 2, z, corner); put(x, gg + 3, z, GLOWSTONE); continue; } }
                    for (int y = gg + 1; y < gg + 4; y++) put(x, y, z, AIR); }
                continue; }
            if (p.kind == 0) {   // Well 1.12.2 (caixa 6x15x6, rel y 11 = chao): casca de cobble 1..4 x 0..12, agua dentro, 4 postes (cerca) de 2, teto de tabuas em y15, borda de cascalho em y11
                const BT cb = des ? (BT)SANDSTONE : (BT)COBBLE, pl = des ? (BT)SANDSTONE : (BT)PLANKS, gr = des ? (BT)SANDSTONE : (BT)GRAVEL;
                static const int fence = pkFind("block/oak_fence"); const BT post = fence ? (BT)fence : corner;
                for (int x = 0; x < 6; x++) for (int z = 0; z < 6; z++) { const int wx = p.x0 + x, wz = p.z0 + z; for (int y = g + 1; y < g + 8; y++) put(wx, y, wz, AIR);
                    const int by = g - 11;   // y absoluto de rel 0
                    if (x >= 1 && x <= 4 && z >= 1 && z <= 4) {
                        const bool inner = x >= 2 && x <= 3 && z >= 2 && z <= 3;
                        for (int ry = 0; ry <= 12; ry++) put(wx, by + ry, wz, (inner && ry >= 1 && ry <= 11) ? (BT)WATER : (inner && ry == 12) ? (BT)AIR : cb);
                        if ((x == 1 || x == 4) && (z == 1 || z == 4)) { put(wx, by + 13, wz, post); put(wx, by + 14, wz, post); }
                        put(wx, by + 15, wz, pl);
                    }
                    if (x == 0 || x == 5 || z == 0 || z == 5) { put(wx, by + 11, wz, gr); for (int y = g + 1; y < g + 8; y++) put(wx, y, wz, AIR); }
                }
                continue; }
            const bool xl = p.face >= 2; const int h = std::max(3, p.H - 1 - (xl ? p.d : p.w) / 2);   // face 0:+x 1:-x 2:+z 3:-z
            for (int x = p.x0; x < p.x0 + p.w; x++) for (int z = p.z0; z < p.z0 + p.d; z++) {
                const int lx = x - p.x0, lz = z - p.z0; const bool ex = lx == 0 || lx == p.w - 1, ez = lz == 0 || lz == p.d - 1;
                for (int y = g - 1; y >= g - 6; y--) { const int bx = x - cx * 16, bz = z - cz * 16; if (bx < 0 || bx > 15 || bz < 0 || bz > 15) break; const BT cur = c->b[idx(bx, y, bz)]; if (cur != AIR && !isWater(cur)) break; put(x, y, z, base); }
                put(x, g, z, (ex || ez) ? base : (BT)PLANKS);
                for (int y = g + 1; y <= g + h + 6; y++) put(x, y, z, AIR);
                if (ex || ez) for (int y = g + 1; y <= g + h; y++) {
                    const bool cor = ex && ez, mid = (xl ? ez : ex) ? (xl ? lx : lz) == (xl ? p.w : p.d) / 2 : false;   // porta no meio da parede de frente
                    const bool door = ((p.face == 0 && lx == p.w - 1) || (p.face == 1 && lx == 0) || (p.face == 2 && lz == p.d - 1) || (p.face == 3 && lz == 0)) && ((p.face < 2 ? lz : lx) == (p.face < 2 ? p.d : p.w) / 2) && y <= g + 2;
                    if (door) continue; (void)mid;
                    const bool win = !cor && y == g + 2 && glass && (((p.face < 2) ? ez : ex) || (((p.face < 2) ? lz : lx) % 2 == 0));
                    put(x, y, z, cor ? corner : (win && !((lx + lz) & 1) ? (BT)glass : wall)); }
                const int along = xl ? lz : lx, span = xl ? p.d : p.w, rh = std::min(along, span - 1 - along);   // telhado em duas águas
                put(x, g + h + 1 + rh, z, wall);
                if (p.kind == 3 && !ex && !ez && ((xl ? lx : lz) == 1)) { put(x, g + 1, z, BOOKSHELF); put(x, g + 2, z, BOOKSHELF); }
            }
        }
    }
}
