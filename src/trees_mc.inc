// ---------- árvores originais do Minecraft 1.12.2 (v33): WorldGenTrees, BigTree, Birch, Taiga1/2, Savanna, Shrub ----------
// Determinístico: semente por coluna (wx,wz,WORLD_SEED). Sem checagem de obstáculos (copas cruzam chunks via treePut/gSpill).
struct TreeCtx { Chunk* c; JR2 r; };
static inline void tL(TreeCtx& t, int x, int y, int z) { treePut(t.c, x, y, z, (BT)gTreeLeaf, true); }
static inline void tG(TreeCtx& t, int x, int y, int z) { treePut(t.c, x, y, z, (BT)gTreeLog, false); }
// WorldGenTrees: carvalho (min 4), bétula (min 5), selva pequena (min 4 + nextInt(7))
static void mcTreeSmall(TreeCtx& t, int x, int y, int z, int minH, bool jungle) {
    int H = t.r.nextInt(3) + minH; if (jungle) H += t.r.nextInt(7);
    for (int yy = y - 3 + H; yy <= y + H; yy++) {
        const int i4 = yy - (y + H), j1 = 1 - i4 / 2;
        for (int xx = x - j1; xx <= x + j1; xx++) for (int zz = z - j1; zz <= z + j1; zz++) {
            if (abs(xx - x) != j1 || abs(zz - z) != j1 || (t.r.nextInt(2) != 0 && i4 != 0)) tL(t, xx, yy, zz);
        }
    }
    for (int i = 0; i < H; i++) tG(t, x, y + i, z);
}
// WorldGenTaiga2 (abeto "alto de saias")
static void mcSpruce(TreeCtx& t, int x, int y, int z) {
    const int i = t.r.nextInt(4) + 6, j = 1 + t.r.nextInt(2), k = i - j, l = 2 + t.r.nextInt(2);
    int i3 = t.r.nextInt(2), j3 = 1, k3 = 0;
    for (int l3 = 0; l3 <= k; l3++) {
        const int yy = y + i - l3;
        for (int xx = x - i3; xx <= x + i3; xx++) for (int zz = z - i3; zz <= z + i3; zz++)
            if (abs(xx - x) != i3 || abs(zz - z) != i3 || i3 <= 0) tL(t, xx, yy, zz);
        if (i3 >= j3) { i3 = k3; k3 = 1; if (++j3 > l) j3 = l; } else ++i3;
    }
    const int th = i - t.r.nextInt(3);
    for (int q = 0; q < th; q++) tG(t, x, y + q, z);
}
// WorldGenTaiga1 (pinheiro pontudo)
static void mcPine(TreeCtx& t, int x, int y, int z) {
    const int i = t.r.nextInt(5) + 7, j = i - t.r.nextInt(2) - 3, k = i - j, l = 1 + t.r.nextInt(k + 1);
    int rr = 0;
    for (int yy = y + i; yy >= y + j; yy--) {
        for (int xx = x - rr; xx <= x + rr; xx++) for (int zz = z - rr; zz <= z + rr; zz++)
            if (abs(xx - x) != rr || abs(zz - z) != rr || rr <= 0) tL(t, xx, yy, zz);
        if (rr >= 1 && yy == y + j + 1) --rr; else if (rr < l) ++rr;
    }
    for (int q = 0; q < i - 1; q++) tG(t, x, y + q, z);
}
// WorldGenSavannaTree (acácia, tronco inclinado, copa chata)
static void mcAcacia(TreeCtx& t, int x, int y, int z) {
    static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
    const int i = t.r.nextInt(3) + t.r.nextInt(3) + 5, d1 = t.r.nextInt(4);
    int k = i - t.r.nextInt(4) - 1, l = 3 - t.r.nextInt(3), px = x, pz = z, top = 0;
    for (int q = 0; q < i; q++) {
        const int yy = y + q;
        if (q >= k && l > 0) { px += DX[d1]; pz += DZ[d1]; --l; }
        tG(t, px, yy, pz); top = yy;
    }
    for (int a = -3; a <= 3; a++) for (int b = -3; b <= 3; b++) if (abs(a) != 3 || abs(b) != 3) tL(t, px + a, top, pz + b);
    for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) tL(t, px + a, top + 1, pz + b);
    tL(t, px + 2, top + 1, pz); tL(t, px - 2, top + 1, pz); tL(t, px, top + 1, pz + 2); tL(t, px, top + 1, pz - 2);
    const int d2 = t.r.nextInt(4);
    if (d2 != d1) {
        int qx = x, qz = z, t2 = 0, l3 = k - t.r.nextInt(2) - 1, k4 = 1 + t.r.nextInt(3);
        for (int q = l3; q < i && k4 > 0; ++q, --k4) if (q >= 1) { qx += DX[d2]; qz += DZ[d2]; tG(t, qx, y + q, qz); t2 = y + q; }
        if (t2 > 0) {
            for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++) if (abs(a) != 2 || abs(b) != 2) tL(t, qx + a, t2, qz + b);
            for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) tL(t, qx + a, t2 + 1, qz + b);
        }
    }
}
// WorldGenShrub (arbusto da selva)
static void mcShrub(TreeCtx& t, int x, int y, int z) {
    tG(t, x, y, z);
    for (int yy = y; yy <= y + 2; yy++) { const int k = 2 - (yy - y);
        for (int xx = x - k; xx <= x + k; xx++) for (int zz = z - k; zz <= z + k; zz++)
            if (abs(xx - x) != k || abs(zz - z) != k || t.r.nextInt(2) == 0) tL(t, xx, yy, zz); }
}
// WorldGenBigTree (carvalho grande/"fancy"); altura 5+nextInt(12), sem teste de obstrução
struct BNode { int x, y, z, base; };
static void bLine(TreeCtx& t, int x0, int y0, int z0, int x1, int y1, int z1) {
    const int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0, n = std::max(abs(dx), std::max(abs(dy), abs(dz)));
    if (n == 0) { tG(t, x0, y0, z0); return; }
    for (int i = 0; i <= n; i++) tG(t, (int)floorf(x0 + .5f + (float)dx * i / n), (int)floorf(y0 + .5f + (float)dy * i / n), (int)floorf(z0 + .5f + (float)dz * i / n));
}
static void mcBigTree(TreeCtx& t, int x, int y, int z) {
    const int HL = 5 + t.r.nextInt(12), LD = 4; int height = (int)(HL * 0.618); if (height >= HL) height = HL - 1;
    int k = (int)(1.382 + pow(1.0 * HL / 13.0, 2.0)); if (k < 1) k = 1;
    std::vector<BNode> nodes; int j = y + height, l = HL - LD;
    nodes.push_back({x, j, z, y + l});
    for (; l >= 0; --l) {
        float f2; { const float f = HL / 2.0f, f1 = f - l; f2 = f1 == 0 ? f : fabsf(f1) >= f ? 0.f : sqrtf(f * f - f1 * f1); f2 *= 0.5f; }
        if (l < HL * 0.3f) { --j; continue; }
        for (int i = 0; i < k; i++) {
            const double d0 = 1.0 * f2 * (t.r.nextFloat() + 0.328), d1 = t.r.nextFloat() * 2.0 * M_PI;
            const int bx = (int)floor(d0 * sin(d1) + x + 0.5), bz = (int)floor(d0 * cos(d1) + z + 0.5);
            const double d4 = sqrt((double)(x - bx) * (x - bx) + (double)(z - bz) * (z - bz)) * 0.381;
            int i2 = (int)(j - d4); if (i2 > j) i2 = j;
            nodes.push_back({bx, j, bz, i2});
        }
        --j;
    }
    for (auto& n : nodes) for (int dy = 0; dy < LD; dy++) {   // folhagem (leafSize 2.0 nas pontas, 3.0 no meio)
        const float rad = (dy == 0 || dy == LD - 1) ? 2.0f : 3.0f; const int ir = (int)(rad + 0.618f);
        for (int a = -ir; a <= ir; a++) for (int b = -ir; b <= ir; b++)
            if ((fabsf((float)a) + .5f) * (fabsf((float)a) + .5f) + (fabsf((float)b) + .5f) * (fabsf((float)b) + .5f) <= rad * rad) tL(t, n.x + a, n.y + dy, n.z + b);
    }
    for (int i = 0; i < height; i++) tG(t, x, y + i, z);   // tronco
    for (auto& n : nodes) if (n.base - y >= HL * 0.2f) bLine(t, x, n.base, z, n.x, n.y, n.z);   // galhos
}
// espécie: 0 carvalho, 1 abeto, 2 bétula, 3 selva, 4 acácia (troncos/folhas do pack)
static void placeTree(Chunk* c, int x, int h, int z, int wx, int wz, int bio) {
    const float r0 = hash2(wx, wz, 71);
    TreeCtx t{c, JR2((int64_t)wx * 341873128712LL + (int64_t)wz * 132897987541LL + (int64_t)WORLD_SEED)};
    enum { SMALL, BIG, BIRCH, SPRUCE, PINE, ACACIA, JUNGLE, SHRUB } k;
    switch (bio) {
    case B_TAIGA: case B_TUNDRA: k = r0 < .45f ? PINE : SPRUCE; break;
    case B_MOUNTAINS: k = r0 < .25f ? PINE : SPRUCE; break;
    case B_JUNGLE: k = r0 < .1f ? SHRUB : r0 < .4f ? BIG : JUNGLE; break;
    case B_SAVANNA: k = r0 < .8f ? ACACIA : SMALL; break;
    case B_SWAMP: k = r0 < .15f ? BIG : SMALL; break;
    case B_PLAINS: k = r0 < .1f ? BIG : SMALL; break;
    default: k = r0 < .2f ? BIRCH : r0 < .3f ? BIG : SMALL; break;   // floresta
    }
    const int sp = k == BIRCH ? 2 : (k == SPRUCE || k == PINE) ? 1 : (k == JUNGLE || k == SHRUB || (k == BIG && bio == B_JUNGLE)) ? 3 : k == ACACIA ? 4 : 0;
    treeSpecies(sp);
    const int y = h + 1;
    switch (k) {
    case SMALL: mcTreeSmall(t, x, y, z, 4, false); break;
    case BIRCH: mcTreeSmall(t, x, y, z, 5, false); break;
    case JUNGLE: mcTreeSmall(t, x, y, z, 4, true); break;
    case BIG: mcBigTree(t, x, y, z); break;
    case SPRUCE: mcSpruce(t, x, y, z); break;
    case PINE: mcPine(t, x, y, z); break;
    case ACACIA: mcAcacia(t, x, y, z); break;
    default: mcShrub(t, x, y, z); break;
    }
}
