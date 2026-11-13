// ---------- árvores originais do Minecraft 1.12.2 (v33): WorldGenTrees, BigTree, Birch, Taiga1/2, Savanna, Shrub ----------
// Determinístico: semente por coluna (wx,wz,WORLD_SEED). Sem checagem de obstáculos (copas cruzam chunks via treePut/gSpill).
struct TreeCtx { Chunk* c; JR2 r; };
static inline void tL(TreeCtx& t, int x, int y, int z) { treePut(t.c, x, y, z, (BT)gTreeLeaf, true); }
static inline void tG(TreeCtx& t, int x, int y, int z) { treePut(t.c, x, y, z, (BT)gTreeLog, false); }
// patchJungle.1: vinhas + arvore gigante da selva (WorldGenMegaJungle / WorldGenHugeTrees do 1.12.2). Bits da vinha: 0=apoio +z, 1=apoio -x, 2=apoio -z, 3=apoio +x
static inline BT tBlk(TreeCtx& t, int x, int y, int z) {   // le o bloco (chunk vizinho se ja existir; senao ar)
    if (y < 0 || y >= CH) return AIR;
    if (x >= 0 && x < CW && z >= 0 && z < CW) return t.c->b[idx(x, y, z)];
    const int ox = fdiv(x, CW), oz = fdiv(z, CW); Chunk* n = getC(gTcx + ox, gTcz + oz); return n ? n->b[idx(x - ox * CW, y, z - oz * CW)] : (BT)AIR;
}
static inline void tVine(TreeCtx& t, int x, int y, int z, int bit) { treePut(t.c, x, y, z, (BT)XB_VINE, true, 1 << bit); }
static inline void tVineRnd(TreeCtx& t, int x, int y, int z, int bit) { if (t.r.nextInt(3) > 0 && tBlk(t, x, y, z) == AIR) tVine(t, x, y, z, bit); }   // placeVine
static void tHangVine(TreeCtx& t, int x, int y, int z, int bit) {   // addHangingVine: ate 4 vinhas para baixo
    tVine(t, x, y, z, bit); int n = 4; y--;
    while (tBlk(t, x, y, z) == AIR && n > 0) { tVine(t, x, y, z, bit); y--; n--; }
}
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
    if (jungle) {   // vinhas no tronco (2/3 por lado) e penduradas nas folhas (1/4)
        for (int yy = y; yy < y + H; yy++) if (yy > y) {
            if (t.r.nextInt(3) > 0 && tBlk(t, x - 1, yy, z) == AIR) tVine(t, x - 1, yy, z, 3);
            if (t.r.nextInt(3) > 0 && tBlk(t, x + 1, yy, z) == AIR) tVine(t, x + 1, yy, z, 1);
            if (t.r.nextInt(3) > 0 && tBlk(t, x, yy, z - 1) == AIR) tVine(t, x, yy, z - 1, 0);
            if (t.r.nextInt(3) > 0 && tBlk(t, x, yy, z + 1) == AIR) tVine(t, x, yy, z + 1, 2);
        }
        for (int yy = y - 3 + H; yy <= y + H; yy++) {
            const int k3 = 2 - (yy - (y + H)) / 2;
            for (int xx = x - k3; xx <= x + k3; xx++) for (int zz = z - k3; zz <= z + k3; zz++) if (isLeafB(tBlk(t, xx, yy, zz))) {
                if (t.r.nextInt(4) == 0 && tBlk(t, xx - 1, yy, zz) == AIR) tHangVine(t, xx - 1, yy, zz, 3);
                if (t.r.nextInt(4) == 0 && tBlk(t, xx + 1, yy, zz) == AIR) tHangVine(t, xx + 1, yy, zz, 1);
                if (t.r.nextInt(4) == 0 && tBlk(t, xx, yy, zz - 1) == AIR) tHangVine(t, xx, yy, zz - 1, 0);
                if (t.r.nextInt(4) == 0 && tBlk(t, xx, yy, zz + 1) == AIR) tHangVine(t, xx, yy, zz + 1, 2);
            }
        }
    }
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
static void megaLayer(TreeCtx& t, int x, int y, int z, int w, bool strict) {   // growLeavesLayer / growLeavesLayerStrict (copa 2x2)
    const int r2 = w * w;
    for (int j = -w; j <= w + (strict ? 1 : 0); j++) for (int k = -w; k <= w + (strict ? 1 : 0); k++) {
        const int l = j - 1, i1 = k - 1;
        const bool ok = strict ? (j * j + k * k <= r2 || l * l + i1 * i1 <= r2 || j * j + i1 * i1 <= r2 || l * l + k * k <= r2) : (j * j + k * k <= r2);
        if (ok) tL(t, x + j, y, z + k);
    }
}
// WorldGenMegaJungle (baseHeight 10, extraRandomHeight 20): tronco 2x2, copa larga, galhos e vinhas
static void mcMegaJungle(TreeCtx& t, int x, int y, int z) {
    const int H = t.r.nextInt(3) + 10 + t.r.nextInt(20);
    for (int j = -2; j <= 0; j++) megaLayer(t, x, y + H + j, z, 2 + 1 - j, true);   // createCrown(pos.up(H), 2)
    for (int j = y + H - 2 - t.r.nextInt(4); j > y + H / 2; j -= 2 + t.r.nextInt(4)) {
        const float f = t.r.nextFloat() * (float)(3.14159265358979323846 * 2.0);
        int k = x + (int)(0.5f + mhCos(f) * 4.0f), l = z + (int)(0.5f + mhSin(f) * 4.0f);
        for (int i1 = 0; i1 < 5; i1++) {
            k = x + (int)(1.5f + mhCos(f) * (float)i1); l = z + (int)(1.5f + mhSin(f) * (float)i1);
            const int by = j - 3 + i1 / 2; if (tBlk(t, k, by, l) == AIR || isLeafB(tBlk(t, k, by, l))) tG(t, k, by, l);
        }
        const int j2 = 1 + t.r.nextInt(2);
        for (int l1 = j - j2; l1 <= j; l1++) megaLayer(t, k, l1, l, 1 - (l1 - j), false);
    }
    auto airLeaf = [&](int xx, int yy, int zz) { const BT b = tBlk(t, xx, yy, zz); return b == AIR || isLeafB(b); };
    for (int xo = 0; xo < 2; xo++) for (int zo = 0; zo < 2; zo++)   // base: sem tronco flutuando em terreno desnivelado
        for (int yy = y - 1; yy >= y - 8 && (xo || zo) && tBlk(t, x + xo, yy, z + zo) == AIR; yy--) tG(t, x + xo, yy, z + zo);
    for (int j3 = 0; j3 < H; j3++) {
        const int yy = y + j3;
        if (airLeaf(x, yy, z)) { tG(t, x, yy, z); if (j3 > 0) { tVineRnd(t, x - 1, yy, z, 3); tVineRnd(t, x, yy, z - 1, 0); } }
        if (j3 < H - 1) {
            if (airLeaf(x + 1, yy, z)) { tG(t, x + 1, yy, z); if (j3 > 0) { tVineRnd(t, x + 2, yy, z, 1); tVineRnd(t, x + 1, yy, z - 1, 0); } }
            if (airLeaf(x + 1, yy, z + 1)) { tG(t, x + 1, yy, z + 1); if (j3 > 0) { tVineRnd(t, x + 2, yy, z + 1, 1); tVineRnd(t, x + 1, yy, z + 2, 2); } }
            if (airLeaf(x, yy, z + 1)) { tG(t, x, yy, z + 1); if (j3 > 0) { tVineRnd(t, x - 1, yy, z + 1, 3); tVineRnd(t, x, yy, z + 2, 2); } }
        }
    }
}
// espécie: 0 carvalho, 1 abeto, 2 bétula, 3 selva, 4 acácia (troncos/folhas do pack)
// patchGen.1: carvalho-escuro da floresta densa (WorldGenCanopyTree, aproximado): tronco 2x2 e copa larga e achatada
static void mcDarkOak(TreeCtx& t, int x, int y, int z) {
    const int H = 5 + t.r.nextInt(2) + t.r.nextInt(3);
    for (int yy = y + H - 2; yy <= y + H + 1; yy++) {
        const int R = yy <= y + H ? 3 : 2;
        for (int xx = x - R; xx <= x + 1 + R; xx++) for (int zz = z - R; zz <= z + 1 + R; zz++) {
            const int dx = xx < x ? x - xx : xx > x + 1 ? xx - (x + 1) : 0, dz = zz < z ? z - zz : zz > z + 1 ? zz - (z + 1) : 0;
            if (dx == R && dz == R && (yy > y + H || t.r.nextInt(2) == 0)) continue;   // cantos cortados
            tL(t, xx, yy, zz);
        }
    }
    for (int i = 0; i < H; i++) for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) tG(t, x + a, y + i, z + b);
}
// abeto gigante da taiga gigante (WorldGenMegaPineTree, aproximado): tronco 2x2 alto, copa conica em camadas
static void mcMegaSpruce(TreeCtx& t, int x, int y, int z) {
    const int H = t.r.nextInt(3) + 13 + t.r.nextInt(15); const int crown = t.r.nextInt(5) + (t.r.nextInt(2) ? 13 : 3), cH = std::min(crown, H - 2);
    for (int i = 0; i <= cH; i++) { const int R = (int)floorf((float)i / (float)std::max(1, cH) * 3.5f) + 1 + ((i & 1) && i > 0 ? 1 : 0); megaLayer(t, x, y + H - i, z, std::min(R, 5), true); }
    tL(t, x, y + H + 1, z); tL(t, x + 1, y + H + 1, z); tL(t, x, y + H + 1, z + 1); tL(t, x + 1, y + H + 1, z + 1);
    for (int i = 0; i < H; i++) for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) tG(t, x + a, y + i, z + b);
}
static void placeTree(Chunk* c, int x, int h, int z, int wx, int wz, int bio) {
    const float r0 = hash2(wx, wz, 71);
    TreeCtx t{c, JR2((int64_t)wx * 341873128712LL + (int64_t)wz * 132897987541LL + (int64_t)WORLD_SEED)};
    enum { SMALL, BIG, BIRCH, SPRUCE, PINE, ACACIA, JUNGLE, SHRUB, MEGA, DARKOAK, MEGASPRUCE } k;
    const int vid0 = biomeRawAt((float)wx, (float)wz), vb = vid0 >= 128 ? vid0 - 128 : vid0;   // id vanilla 1.12 do bioma (patchGen.1)
    if (vb == 27 || vb == 28) k = BIRCH;   // floresta de betulas: so betula
    else if (vb == 29) k = r0 < .67f ? DARKOAK : r0 < .77f ? BIG : SMALL;   // floresta densa: 2/3 carvalho-escuro
    else if (vb == 32 || vb == 33) k = r0 < .33f ? MEGASPRUCE : r0 < .66f ? PINE : SPRUCE;   // taiga gigante
    else switch (bio) {
    case B_TAIGA: case B_TUNDRA: k = r0 < .45f ? PINE : SPRUCE; break;
    case B_MOUNTAINS: k = r0 < .25f ? PINE : SPRUCE; break;
    case B_JUNGLE: k = t.r.nextInt(10) == 0 ? BIG : t.r.nextInt(2) == 0 ? SHRUB : t.r.nextInt(3) == 0 ? MEGA : JUNGLE; break;   // BiomeJungle: 1/10 grande (carvalho), 1/2 arbusto, 1/3 gigante, resto selva pequena
    case B_SAVANNA: k = r0 < .8f ? ACACIA : SMALL; break;
    case B_SWAMP: k = r0 < .15f ? BIG : SMALL; break;
    case B_PLAINS: k = r0 < .1f ? BIG : SMALL; break;
    default: k = r0 < .2f ? BIRCH : r0 < .3f ? BIG : SMALL; break;   // floresta
    }
    const int sp = k == DARKOAK ? 5 : k == MEGASPRUCE ? 1 : k == BIRCH ? 2 : (k == SPRUCE || k == PINE) ? 1 : (k == JUNGLE || k == SHRUB || k == MEGA) ? 3 : k == ACACIA ? 4 : 0;
    treeSpecies(sp);
    if (k == SHRUB && bio == B_JUNGLE) { treeSpecies(0); const int ol = gTreeLeaf; treeSpecies(3); gTreeLeaf = ol; }   // WorldGenShrub(selva): tronco de selva + folha de carvalho
    const int y = h + 1;
    switch (k) {
    case SMALL: mcTreeSmall(t, x, y, z, 4, false); break;
    case BIRCH: mcTreeSmall(t, x, y, z, 5, false); break;
    case JUNGLE: mcTreeSmall(t, x, y, z, 4, true); break;
    case BIG: mcBigTree(t, x, y, z); break;
    case MEGA: mcMegaJungle(t, x, y, z); break;
    case SPRUCE: mcSpruce(t, x, y, z); break;
    case PINE: mcPine(t, x, y, z); break;
    case ACACIA: mcAcacia(t, x, y, z); break;
    case DARKOAK: mcDarkOak(t, x, y, z); break;
    case MEGASPRUCE: mcMegaSpruce(t, x, y, z); break;
    default: mcShrub(t, x, y, z); break;
    }
}
