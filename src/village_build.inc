// ---------- conteudo das pecas da vila: port de StructureVillagePieces (1.12.2, v40) ----------
// Mesmas coordenadas locais do Java (getXWithOffset/getZWithOffset por coordBaseMode, rotacao/espelho de direcoes), mesmas ordens de fillWithBlocks/setBlockState.
// Escadas, lajes, colheitas e terra arada sao blocos reais (XB_STAIR/XB_SLAB/XB_CROP/XB_FARM). Sem aldeoes (nao existem no jogo).
static void vilChestLoot(int x, int y, int z, int64_t seed);   // chest.inc
static void vilQueueVillagers(double x, double y, double z);   // mobs.inc: aldeoes iniciais ao redor do poco
struct Bk { int id, m; };
static inline int vbPk(const char* f, int fb) { const int i = pkFind(f); return i ? i : fb; }
struct VB {
    Chunk* c; int cx, cz, mode, minX, minY, minZ, maxX, maxZ, st; JR2 r; std::vector<std::array<int, 3>> cl;
    int X(int x, int z) const { return mode == 3 ? maxX - z : mode == 1 ? minX + z : minX + x; }
    int Z(int x, int z) const { return mode == 0 ? maxZ - z : mode == 2 ? minZ + z : minZ + x; }
    int dirT(int d) const { const int m = (d & 1) ? d : d ^ 2; return mode == 0 ? d : mode == 2 ? m : mode == 3 ? (m + 1) & 3 : (d + 1) & 3; }   // N0 L1 S2 O3
    Bk air() const { return {0, 0}; }
    Bk cobR() const { return {COBBLE, 0}; }
    Bk cobB() const { return st == 1 ? Bk{SANDSTONE, 0} : st == 2 ? Bk{vbPk("block/acacia_log", LOG), 0} : Bk{COBBLE, 0}; }
    Bk plkB() const { return st == 1 ? Bk{vbPk("block/smooth_sandstone", SANDSTONE), 0} : st == 2 ? Bk{vbPk("block/acacia_planks", PLANKS), 0} : st == 3 ? Bk{vbPk("block/spruce_planks", PLANKS), 0} : Bk{PLANKS, 0}; }
    Bk logB() const { return st == 1 ? Bk{SANDSTONE, 0} : st == 2 ? Bk{vbPk("block/acacia_log", LOG), 0} : st == 3 ? Bk{vbPk("block/spruce_log", LOG), 0} : Bk{LOG, 0}; }
    Bk fenB() const { return {XB_FENCE, (st == 2 ? 4 : st == 3 ? 1 : 0) << 4}; }
    Bk SO(int jd) const { return {XB_STAIR, dirT(jd) | ((st == 1 ? 7 : st == 2 ? 5 : st == 3 ? 2 : 0) << 3)}; }   // escada de carvalho (facing do Java)
    Bk SS(int jd) const { return {XB_STAIR, dirT(jd) | ((st == 1 ? 7 : 1) << 3)}; }   // escada de pedra
    Bk pane(int v) const { return {XB_PANE, v << 4}; }
    void set(Bk b, int lx, int ly, int lz) {
        const int wx = X(lx, lz), wz = Z(lx, lz), y = minY + ly, bx = wx - cx * 16, bz = wz - cz * 16;
        if (bx < 0 || bx > 15 || bz < 0 || bz > 15 || y < 1 || y > 250) return;
        c->b[idx(bx, y, bz)] = (BT)b.id; c->m[idx(bx, y, bz)] = (uint8_t)b.m;
        if (b.id == XB_FENCE || b.id == XB_PANE) cl.push_back({bx, y, bz});
    }
    int get(int lx, int ly, int lz, bool& ok) const {
        const int bx = X(lx, lz) - cx * 16, bz = Z(lx, lz) - cz * 16, y = minY + ly; ok = bx >= 0 && bx < 16 && bz >= 0 && bz < 16 && y >= 1 && y <= 250;
        return ok ? (int)c->b[idx(bx, y, bz)] : 0;
    }
    void fill(int x1, int y1, int z1, int x2, int y2, int z2, Bk e, Bk in) {
        for (int y = y1; y <= y2; y++) for (int x = x1; x <= x2; x++) for (int z = z1; z <= z2; z++)
            set((x == x1 || x == x2 || y == y1 || y == y2 || z == z1 || z == z2) ? e : in, x, y, z);
    }
    void fill(int x1, int y1, int z1, int x2, int y2, int z2, Bk b) { fill(x1, y1, z1, x2, y2, z2, b, b); }
    void clearUp(int lx, int ly, int lz) {   // clearCurrentPositionBlocksUpwards
        const int bx = X(lx, lz) - cx * 16, bz = Z(lx, lz) - cz * 16; if (bx < 0 || bx > 15 || bz < 0 || bz > 15) return;
        for (int y = std::max(1, minY + ly); y < 250 && c->b[idx(bx, y, bz)] != AIR; y++) { c->b[idx(bx, y, bz)] = AIR; c->m[idx(bx, y, bz)] = 0; }
    }
    void repl(Bk b, int lx, int ly, int lz) {   // replaceAirAndLiquidDownwards
        const int bx = X(lx, lz) - cx * 16, bz = Z(lx, lz) - cz * 16; if (bx < 0 || bx > 15 || bz < 0 || bz > 15) return;
        for (int y = minY + ly; y > 1 && y <= 250; y--) { const BT cur = c->b[idx(bx, y, bz)]; if (cur != AIR && !isWater(cur) && !isLava(cur)) break; c->b[idx(bx, y, bz)] = (BT)b.id; c->m[idx(bx, y, bz)] = (uint8_t)b.m; }
    }
    void door(int lx, int ly, int lz) {
        const int v = st == 2 ? 5 : st == 3 ? 2 : 1, m = ((dirT(0) + 2) & 3) | (v << 5);
        set({XB_DOOR, m}, lx, ly, lz); set({XB_DOOR, m | 8}, lx, ly + 1, lz);
    }
    void torch(int facing, int lx, int ly, int lz) { set({XB_TORCH + 1 + ((dirT(facing) + 3) & 3), 0}, lx, ly, lz); }
    void doorStep(int lx, Bk stairs) {   // degrau na frente da porta se o ar encosta no chao
        bool o1, o2; const int a = get(lx, 0, -1, o1), b = get(lx, -1, -1, o2); if (!(o1 && o2 && a == AIR && b != AIR)) return;
        set(stairs, lx, 0, -1);
        static const int gp = pkFind("block/dirt_path") ? pkFind("block/dirt_path") : pkFind("block/grass_path");
        if (gp && b == gp) set({GRASS, 0}, lx, -1, -1);
    }
    void panes(const int* p, int n) { for (int i = 0; i < n; i++) set(pane(1), p[i * 3], p[i * 3 + 1], p[i * 3 + 2]); }
    void finish() {   // conexoes de cerca/painel (so vizinhos dentro do chunk)
        static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0};
        for (auto& q : cl) {
            const int b = c->b[idx(q[0], q[1], q[2])], old = c->m[idx(q[0], q[1], q[2])]; int m = 0;
            for (int d = 0; d < 4; d++) { const int nx = q[0] + DX[d], nz = q[2] + DZ[d]; if (nx < 0 || nx > 15 || nz < 0 || nz > 15) continue;
                const int nb = c->b[idx(nx, q[1], nz)]; if (nb == b || (nb > 0 && nb < NB && nb != WATER && nb != LEAVES) || (nb >= PK0 && isPkBlk(nb))) m |= 1 << d; }
            c->m[idx(q[0], q[1], q[2])] = (uint8_t)((old & 0xF0) | m);
        }
    }
};
static void vilBuild(Chunk* c, int cx, int cz, const VPiece& p, int bio) {
    VB v{c, cx, cz, p.f, p.x0, p.g, p.z0, p.x0 + p.w - 1, p.z0 + p.d - 1, bio == B_DESERT ? 1 : bio == B_SAVANNA ? 2 : bio == B_TAIGA ? 3 : 0,
         JR2((int64_t)p.x0 * 341873128712LL + (int64_t)p.z0 * 132897987541LL + (int64_t)WORLD_SEED + 77)};
    auto pane = [&](int q) { return v.pane(q); };
    const Bk A = v.air(), cb = v.cobB(), pk = v.plkB(), lg = v.logB(), fe = v.fenB(), cr = v.cobR(), plate{XB_PLATE, 0}, dslab{STONE, 0};
    const Bk craft{vbPk("block/crafting_table", PLANKS), 0}, furn{vbPk("block/furnace", COBBLE), (int)v.dirT(0)}, farm{XB_FARM, 0}, slab{XB_SLAB, 10 << 2};
    auto S = [&](Bk b, int x, int y, int z) { v.set(b, x, y, z); };
    auto F = [&](int a, int b, int c2, int d, int e, int f, Bk k) { v.fill(a, b, c2, d, e, f, k); };
    switch (p.t) {
    case 0: {   // Well (caixa 6x15x6: y local 0 = avg-11)
        vilQueueVillagers(p.x0 + 3.0, p.g + 1.0, p.z0 + 3.0);
        v.fill(1, 0, 1, 4, 12, 4, cb, Bk{WATER, 0}); S(A, 2, 12, 2); S(A, 3, 12, 2); S(A, 2, 12, 3); S(A, 3, 12, 3);
        for (int y = 13; y <= 14; y++) { S(fe, 1, y, 1); S(fe, 4, y, 1); S(fe, 1, y, 4); S(fe, 4, y, 4); }
        F(1, 15, 1, 4, 15, 4, cb);
        for (int i = 0; i <= 5; i++) for (int j = 0; j <= 5; j++) if (j == 0 || j == 5 || i == 0 || i == 5) { S(cb, j, 11, i); v.clearUp(j, 12, i); }
        break; }
    case 3: {   // Church 5x12x9
        F(1, 1, 1, 3, 3, 7, A); F(1, 5, 1, 3, 9, 3, A); F(1, 0, 0, 3, 0, 8, cr); F(1, 1, 0, 3, 10, 0, cr); F(0, 1, 1, 0, 10, 3, cr); F(4, 1, 1, 4, 10, 3, cr);
        F(0, 0, 4, 0, 4, 7, cr); F(4, 0, 4, 4, 4, 7, cr); F(1, 1, 8, 3, 4, 8, cr); F(1, 5, 4, 3, 10, 4, cr); F(1, 5, 5, 3, 5, 7, cr); F(0, 9, 0, 4, 9, 4, cr); F(0, 4, 0, 4, 4, 4, cr);
        static const int cbs[][3] = {{0, 11, 2}, {4, 11, 2}, {2, 11, 0}, {2, 11, 4}, {1, 1, 6}, {1, 1, 7}, {2, 1, 7}, {3, 1, 6}, {3, 1, 7}};
        for (auto& q : cbs) S(cr, q[0], q[1], q[2]);
        static const int sts[][4] = {{1, 1, 5, 0}, {2, 1, 6, 0}, {3, 1, 5, 0}, {1, 2, 7, 3}, {3, 2, 7, 1}};
        for (auto& q : sts) S(v.SS(q[3]), q[0], q[1], q[2]);
        static const int gl[] = {0, 2, 2, 0, 3, 2, 4, 2, 2, 4, 3, 2, 0, 6, 2, 0, 7, 2, 4, 6, 2, 4, 7, 2, 2, 6, 0, 2, 7, 0, 2, 6, 4, 2, 7, 4, 0, 3, 6, 4, 3, 6, 2, 3, 8};
        v.panes(gl, 15);
        v.torch(2, 2, 4, 7); v.torch(1, 1, 4, 6); v.torch(3, 3, 4, 6); v.torch(0, 2, 4, 5);
        for (int i = 1; i <= 9; i++) S(Bk{XB_LADDER, v.dirT(3)}, 3, i, 3);
        S(A, 2, 1, 0); S(A, 2, 2, 0); v.door(2, 1, 0); v.doorStep(2, v.SS(0));
        for (int k = 0; k < 9; k++) for (int j = 0; j < 5; j++) { v.clearUp(j, 12, k); v.repl(cb, j, -1, k); }
        break; }
    case 7: case 8: {   // Field1 13x4x9 / Field2 7x4x9 (sem colheitas)
        const bool f1 = p.t == 7; const int W = f1 ? 12 : 6;
        F(0, 1, 0, W, 4, 8, A); F(1, 0, 1, 2, 0, 7, farm); F(4, 0, 1, 5, 0, 7, farm);
        if (f1) { F(7, 0, 1, 8, 0, 7, farm); F(10, 0, 1, 11, 0, 7, farm); }
        F(0, 0, 0, 0, 0, 8, lg); F(6, 0, 0, 6, 0, 8, lg); if (f1) F(12, 0, 0, 12, 0, 8, lg);
        F(1, 0, 0, W - 1, 0, 0, lg); F(1, 0, 8, W - 1, 0, 8, lg);
        F(3, 0, 1, 3, 0, 7, Bk{WATER, 0}); if (f1) F(9, 0, 1, 9, 0, 7, Bk{WATER, 0});
        { const int nc = f1 ? 4 : 2; int ty[4]; for (int q = 0; q < nc; q++) { const int r = v.r.nextInt(10); ty[q] = r < 2 ? 1 : r < 4 ? 2 : r == 4 ? 3 : 0; }   // getRandomCropType
          for (int i = 1; i <= 7; i++) for (int q = 0; q < nc; q++) for (int w = 0; w < 2; w++) { const int mx = ty[q] == 0 ? 7 : 3, lo = mx / 3, age = lo + v.r.nextInt(mx - lo + 1);
              S(Bk{XB_CROP, (ty[q] << 3) | (ty[q] == 0 ? age : age * 7 / 3)}, (q * 3 + 1) + w + (f1 ? 0 : q * 0), 1, i); } }
        for (int j = 0; j < 9; j++) for (int k = 0; k <= W; k++) { v.clearUp(k, 4, j); v.repl(Bk{DIRT, 0}, k, -1, j); }
        break; }
    case 6: {   // Hall 9x7x11
        F(1, 1, 1, 7, 4, 4, A); F(2, 1, 6, 8, 4, 10, A); F(2, 0, 6, 8, 0, 10, Bk{DIRT, 0}); S(cb, 6, 0, 6);
        F(2, 1, 6, 2, 1, 10, fe); F(8, 1, 6, 8, 1, 10, fe); F(3, 1, 10, 7, 1, 10, fe); F(1, 0, 1, 7, 0, 4, pk);
        F(0, 0, 0, 0, 3, 5, cb); F(8, 0, 0, 8, 3, 5, cb); F(1, 0, 0, 7, 1, 0, cb); F(1, 0, 5, 7, 1, 5, cb); F(1, 2, 0, 7, 3, 0, pk); F(1, 2, 5, 7, 3, 5, pk);
        F(0, 4, 1, 8, 4, 1, pk); F(0, 4, 4, 8, 4, 4, pk); F(0, 5, 2, 8, 5, 3, pk);
        S(pk, 0, 4, 2); S(pk, 0, 4, 3); S(pk, 8, 4, 2); S(pk, 8, 4, 3);
        for (int i = -1; i <= 2; i++) for (int j = 0; j <= 8; j++) { S(v.SO(0), j, 4 + i, i); S(v.SO(2), j, 4 + i, 5 - i); }
        S(lg, 0, 2, 1); S(lg, 0, 2, 4); S(lg, 8, 2, 1); S(lg, 8, 2, 4);
        static const int gl[] = {0, 2, 2, 0, 2, 3, 8, 2, 2, 8, 2, 3, 2, 2, 5, 3, 2, 5, 5, 2, 0, 6, 2, 5};
        v.panes(gl, 8);
        S(fe, 2, 1, 3); S(plate, 2, 2, 3); S(pk, 1, 1, 4); S(v.SO(0), 2, 1, 4); S(v.SO(3), 1, 1, 3);
        F(5, 0, 1, 7, 0, 3, dslab); S(dslab, 6, 1, 1); S(dslab, 6, 1, 2);
        S(A, 2, 1, 0); S(A, 2, 2, 0); v.torch(0, 2, 3, 1); v.door(2, 1, 0); v.doorStep(2, v.SO(0));
        S(A, 6, 1, 5); S(A, 6, 2, 5); v.torch(2, 6, 3, 4); v.door(6, 1, 5);
        for (int k = 0; k < 5; k++) for (int l = 0; l < 9; l++) { v.clearUp(l, 7, k); v.repl(cb, l, -1, k); }
        break; }
    case 4: {   // House1 9x9x6 (biblioteca)
        F(1, 1, 1, 7, 5, 4, A); F(0, 0, 0, 8, 0, 5, cb); F(0, 5, 0, 8, 5, 5, cb); F(0, 6, 1, 8, 6, 4, cb); F(0, 7, 2, 8, 7, 3, cb);
        for (int i = -1; i <= 2; i++) for (int j = 0; j <= 8; j++) { S(v.SO(0), j, 6 + i, i); S(v.SO(2), j, 6 + i, 5 - i); }
        F(0, 1, 0, 0, 1, 5, cb); F(1, 1, 5, 8, 1, 5, cb); F(8, 1, 0, 8, 1, 4, cb); F(2, 1, 0, 7, 1, 0, cb);
        F(0, 2, 0, 0, 4, 0, cb); F(0, 2, 5, 0, 4, 5, cb); F(8, 2, 5, 8, 4, 5, cb); F(8, 2, 0, 8, 4, 0, cb);
        F(0, 2, 1, 0, 4, 4, pk); F(1, 2, 5, 7, 4, 5, pk); F(8, 2, 1, 8, 4, 4, pk); F(1, 2, 0, 7, 4, 0, pk);
        static const int gl[] = {4, 2, 0, 5, 2, 0, 6, 2, 0, 4, 3, 0, 5, 3, 0, 6, 3, 0, 0, 2, 2, 0, 2, 3, 0, 3, 2, 0, 3, 3, 8, 2, 2, 8, 2, 3, 8, 3, 2, 8, 3, 3, 2, 2, 5, 3, 2, 5, 5, 2, 5, 6, 2, 5};
        v.panes(gl, 18);
        F(1, 4, 1, 7, 4, 1, pk); F(1, 4, 4, 7, 4, 4, pk); F(1, 3, 4, 7, 3, 4, Bk{BOOKSHELF, 0});
        S(pk, 7, 1, 4); S(v.SO(1), 7, 1, 3); S(v.SO(0), 6, 1, 4); S(v.SO(0), 5, 1, 4); S(v.SO(0), 4, 1, 4); S(v.SO(0), 3, 1, 4);
        S(fe, 6, 1, 3); S(plate, 6, 2, 3); S(fe, 4, 1, 3); S(plate, 4, 2, 3); S(craft, 7, 1, 1);
        S(A, 1, 1, 0); S(A, 1, 2, 0); v.door(1, 1, 0); v.doorStep(1, v.SS(0));
        for (int l = 0; l < 6; l++) for (int k = 0; k < 9; k++) { v.clearUp(k, 9, l); v.repl(cb, k, -1, l); }
        break; }
    case 9: {   // House2 10x6x7 (ferreiro)
        F(0, 1, 0, 9, 4, 6, A); F(0, 0, 0, 9, 0, 6, cr); F(0, 4, 0, 9, 4, 6, cr); F(0, 5, 0, 9, 5, 6, slab); F(1, 5, 1, 8, 5, 5, A);
        F(1, 1, 0, 2, 3, 0, pk); F(0, 1, 0, 0, 4, 0, lg); F(3, 1, 0, 3, 4, 0, lg); F(0, 1, 6, 0, 4, 6, lg);
        S(pk, 3, 3, 1); F(3, 1, 2, 3, 3, 2, pk); F(4, 1, 3, 5, 3, 3, pk); F(0, 1, 1, 0, 3, 5, pk); F(1, 1, 6, 5, 3, 6, pk);
        F(5, 1, 0, 5, 3, 0, fe); F(9, 1, 0, 9, 3, 0, fe); F(6, 1, 4, 9, 4, 6, cr);
        S(Bk{LAVA, 0}, 7, 1, 5); S(Bk{LAVA, 0}, 8, 1, 5); S(pane(0), 9, 2, 5); S(pane(0), 9, 2, 4); F(7, 2, 4, 8, 2, 5, A);
        S(cr, 6, 1, 3); S(furn, 6, 2, 3); S(furn, 6, 3, 3); S(dslab, 8, 1, 1);
        S(pane(1), 0, 2, 2); S(pane(1), 0, 2, 4); S(pane(1), 2, 2, 6); S(pane(1), 4, 2, 6);
        S(fe, 2, 1, 4); S(plate, 2, 2, 4); S(pk, 1, 1, 5); S(v.SO(0), 2, 1, 5); S(v.SO(3), 1, 1, 4);
        { bool ok; v.get(5, 1, 5, ok); if (ok) { S(Bk{XB_CHEST, v.dirT(2)}, 5, 1, 5); vilChestLoot(v.X(5, 5), v.minY + 1, v.Z(5, 5), v.r.nextLong()); } }
        for (int i = 6; i <= 8; i++) { bool o1, o2; const int a = v.get(i, 0, -1, o1), b = v.get(i, -1, -1, o2); if (o1 && o2 && a == AIR && b != AIR) S(v.SS(0), i, 0, -1); }
        for (int k = 0; k < 7; k++) for (int j = 0; j < 10; j++) { v.clearUp(j, 6, k); v.repl(cb, j, -1, k); }
        break; }
    case 10: {   // House3 9x7x12
        F(1, 1, 1, 7, 4, 4, A); F(2, 1, 6, 8, 4, 10, A); F(2, 0, 5, 8, 0, 10, pk); F(1, 0, 1, 7, 0, 4, pk);
        F(0, 0, 0, 0, 3, 5, cb); F(8, 0, 0, 8, 3, 10, cb); F(1, 0, 0, 7, 2, 0, cb); F(1, 0, 5, 2, 1, 5, cb); F(2, 0, 6, 2, 3, 10, cb); F(3, 0, 10, 7, 3, 10, cb);
        F(1, 2, 0, 7, 3, 0, pk); F(1, 2, 5, 2, 3, 5, pk); F(0, 4, 1, 8, 4, 1, pk); F(0, 4, 4, 3, 4, 4, pk); F(0, 5, 2, 8, 5, 3, pk);
        S(pk, 0, 4, 2); S(pk, 0, 4, 3); S(pk, 8, 4, 2); S(pk, 8, 4, 3); S(pk, 8, 4, 4);
        for (int i = -1; i <= 2; i++) for (int j = 0; j <= 8; j++) { S(v.SO(0), j, 4 + i, i); if ((i > -1 || j <= 1) && (i > 0 || j <= 3) && (i > 1 || j <= 4 || j >= 6)) S(v.SO(2), j, 4 + i, 5 - i); }
        F(3, 4, 5, 3, 4, 10, pk); F(7, 4, 2, 7, 4, 10, pk); F(4, 5, 4, 4, 5, 10, pk); F(6, 5, 4, 6, 5, 10, pk); F(5, 6, 3, 5, 6, 10, pk);
        for (int k = 4; k >= 1; k--) { S(pk, k, 2 + k, 7 - k); for (int k1 = 8 - k; k1 <= 10; k1++) S(v.SO(1), k, 2 + k, k1); }
        S(pk, 6, 6, 3); S(pk, 7, 5, 4); S(v.SO(3), 6, 6, 4);
        for (int l = 6; l <= 8; l++) for (int l1 = 5; l1 <= 10; l1++) S(v.SO(3), l, 12 - l, l1);
        S(lg, 0, 2, 1); S(lg, 0, 2, 4); S(lg, 4, 2, 0); S(lg, 6, 2, 0); S(lg, 8, 2, 1); S(lg, 8, 2, 4); S(pk, 8, 2, 5); S(lg, 8, 2, 6); S(lg, 8, 2, 9); S(lg, 2, 2, 6); S(lg, 2, 2, 9);
        S(lg, 4, 4, 10); S(lg, 6, 4, 10); S(pk, 5, 5, 10);
        static const int gl[] = {0, 2, 2, 0, 2, 3, 5, 2, 0, 8, 2, 2, 8, 2, 3, 8, 2, 7, 8, 2, 8, 2, 2, 7, 2, 2, 8, 5, 4, 10};
        v.panes(gl, 10);
        S(A, 2, 1, 0); S(A, 2, 2, 0); v.torch(0, 2, 3, 1); v.door(2, 1, 0); F(1, 0, -1, 3, 2, -1, A); v.doorStep(2, v.SO(0));
        for (int i1 = 0; i1 < 5; i1++) for (int i2 = 0; i2 < 9; i2++) { v.clearUp(i2, 7, i1); v.repl(cb, i2, -1, i1); }
        for (int j1 = 5; j1 < 11; j1++) for (int j2 = 2; j2 < 9; j2++) { v.clearUp(j2, 7, j1); v.repl(cb, j2, -1, j1); }
        break; }
    case 2: {   // House4Garden 5x6x5
        const bool roof = v.r.nextInt(2) == 0;
        F(0, 0, 0, 4, 0, 4, cb); F(0, 4, 0, 4, 4, 4, lg); F(1, 4, 1, 3, 4, 3, pk);
        for (int y = 1; y <= 3; y++) { S(cb, 0, y, 0); S(cb, 4, y, 0); S(cb, 0, y, 4); S(cb, 4, y, 4); }
        F(0, 1, 1, 0, 3, 3, pk); F(4, 1, 1, 4, 3, 3, pk); F(1, 1, 4, 3, 3, 4, pk);
        S(pane(1), 0, 2, 2); S(pane(1), 2, 2, 4); S(pane(1), 4, 2, 2);
        S(pk, 1, 1, 0); S(pk, 1, 2, 0); S(pk, 1, 3, 0); S(pk, 2, 3, 0); S(pk, 3, 3, 0); S(pk, 3, 2, 0); S(pk, 3, 1, 0);
        { bool o1, o2; const int a = v.get(2, 0, -1, o1), b = v.get(2, -1, -1, o2); if (o1 && o2 && a == AIR && b != AIR) S(v.SS(0), 2, 0, -1); }
        F(1, 1, 1, 3, 3, 3, A);
        if (roof) { for (int i = 0; i <= 4; i++) { S(fe, i, 5, 0); S(fe, i, 5, 4); } for (int i = 1; i <= 3; i++) { S(fe, 4, 5, i); S(fe, 0, 5, i); }
            for (int y = 1; y <= 4; y++) S(Bk{XB_LADDER, v.dirT(2)}, 3, y, 3); }
        v.torch(0, 2, 3, 1);
        for (int j = 0; j < 5; j++) for (int i = 0; i < 5; i++) { v.clearUp(i, 6, j); v.repl(cb, i, -1, j); }
        break; }
    case 5: {   // WoodHut 4x6x5
        const bool tall = v.r.nextInt(2) == 0; const int tp = v.r.nextInt(3);
        F(1, 1, 1, 3, 5, 4, A); F(0, 0, 0, 3, 0, 4, cb); F(1, 0, 1, 2, 0, 3, Bk{DIRT, 0});
        if (tall) F(1, 4, 1, 2, 4, 3, lg); else F(1, 5, 1, 2, 5, 3, lg);
        static const int lgs[][3] = {{1, 4, 0}, {2, 4, 0}, {1, 4, 4}, {2, 4, 4}, {0, 4, 1}, {0, 4, 2}, {0, 4, 3}, {3, 4, 1}, {3, 4, 2}, {3, 4, 3}};
        for (auto& q : lgs) S(lg, q[0], q[1], q[2]);
        F(0, 1, 0, 0, 3, 0, lg); F(3, 1, 0, 3, 3, 0, lg); F(0, 1, 4, 0, 3, 4, lg); F(3, 1, 4, 3, 3, 4, lg);
        F(0, 1, 1, 0, 3, 3, pk); F(3, 1, 1, 3, 3, 3, pk); F(1, 1, 0, 2, 3, 0, pk); F(1, 1, 4, 2, 3, 4, pk);
        S(pane(1), 0, 2, 2); S(pane(1), 3, 2, 2);
        if (tp > 0) { S(fe, tp, 1, 3); S(plate, tp, 2, 3); }
        S(A, 1, 1, 0); S(A, 1, 2, 0); v.door(1, 1, 0); v.doorStep(1, v.SS(0));
        for (int i = 0; i < 5; i++) for (int j = 0; j < 4; j++) { v.clearUp(j, 6, i); v.repl(cb, j, -1, i); }
        break; }
    case 11: {   // Torch (poste)
        F(0, 0, 0, 2, 3, 1, A); S(fe, 1, 0, 0); S(fe, 1, 1, 0); S(fe, 1, 2, 0); S(Bk{WOOL, 0}, 1, 3, 0);
        v.torch(1, 2, 3, 0); v.torch(0, 1, 3, 1); v.torch(3, 0, 3, 0); v.torch(2, 1, 3, -1);
        break; }
    default: break;
    }
    v.finish();
}
