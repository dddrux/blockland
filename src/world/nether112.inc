// ---- Nether 1.12.2 (patchNether.3): ChunkGeneratorHell + MapGenCavesHell + decoracao, usando o ruido vanilla de terrain112.inc ----
static int64_t jNextLong(JRng& r) { return ((int64_t)r.next(32) << 32) + r.next(32); }
static float jNextFloat(JRng& r) { return r.next(24) / (float)(1 << 24); }
struct NetherNoise { int64_t seed = 0; bool ok = false; OctNoise lp1, lp2, pn1, sg, ex, sc, dp; };
static NetherNoise gNN;
static void nnEnsure() {
    if (gNN.ok && gNN.seed == (int64_t)WORLD_SEED) return;
    JRng r((int64_t)WORLD_SEED); gNN.lp1.init(r, 16); gNN.lp2.init(r, 16); gNN.pn1.init(r, 8); gNN.sg.init(r, 4); gNN.ex.init(r, 4); gNN.sc.init(r, 10); gNN.dp.init(r, 16);
    gNN.seed = (int64_t)WORLD_SEED; gNN.ok = true;
}
static void nnHeights(double* buf, int x, int z) {   // getHeights: grade 5x17x5
    static double pnr[425], ar[425], br[425], sc[25], dp[25]; double adj[17];
    const double d0 = 684.412, d1 = 2053.236;
    gNN.sc.gen(sc, x, 0, z, 5, 1, 5, 1.0, 0.0, 1.0); gNN.dp.gen(dp, x, 0, z, 5, 1, 5, 100.0, 0.0, 100.0);
    gNN.pn1.gen(pnr, x, 0, z, 5, 17, 5, d0 / 80.0, d1 / 60.0, d0 / 80.0); gNN.lp1.gen(ar, x, 0, z, 5, 17, 5, d0, d1, d0); gNN.lp2.gen(br, x, 0, z, 5, 17, 5, d0, d1, d0);
    for (int i = 0; i < 17; i++) { adj[i] = cos(i * M_PI * 6.0 / 17.0) * 2.0; double d2 = i; if (i > 8) d2 = 16 - i; if (d2 < 4.0) { d2 = 4.0 - d2; adj[i] -= d2 * d2 * d2 * 10.0; } }
    int l = 0;
    for (int i = 0; i < 5; i++) for (int k = 0; k < 5; k++) for (int j = 0; j < 17; j++) {
        const double d5 = ar[l] / 512.0, d6 = br[l] / 512.0, d7 = (pnr[l] / 10.0 + 1.0) / 2.0; double d8 = d7 < 0 ? d5 : d7 > 1 ? d6 : d5 + (d6 - d5) * d7;
        d8 -= adj[j];
        if (j > 13) { const double d9 = (j - 13) / 3.0; d8 = d8 * (1.0 - d9) + -10.0 * d9; }
        buf[l++] = d8;
    }
}
// MapGenCavesHell
static void nCaveTunnel(int64_t seed, int cx, int cz, Chunk* c, double x, double y, double z, float width, float yaw, float pitch, int start, int end, double hm) {
    JRng r(seed); const double d0 = cx * 16 + 8, d1 = cz * 16 + 8; float f = 0, f1 = 0;
    if (end <= 0) { const int i = 112; end = i - r.nextInt(i / 4); }
    bool f2b = false; if (start == -1) { start = end / 2; f2b = true; }
    const int j = r.nextInt(end / 2) + end / 4;
    for (const bool fl = r.nextInt(6) == 0; start < end; ++start) {
        const double d2 = 1.5 + sinf(start * (float)M_PI / end) * width, d3 = d2 * hm; const float c2 = cosf(pitch), s2 = sinf(pitch);
        x += cosf(yaw) * c2; y += s2; z += sinf(yaw) * c2;
        pitch *= fl ? 0.92f : 0.7f; pitch += f1 * 0.1f; yaw += f * 0.1f; f1 *= 0.9f; f *= 0.75f;
        f1 += (jNextFloat(r) - jNextFloat(r)) * jNextFloat(r) * 2.f; f += (jNextFloat(r) - jNextFloat(r)) * jNextFloat(r) * 4.f;
        if (!f2b && start == j && width > 1.f) {
            nCaveTunnel(jNextLong(r), cx, cz, c, x, y, z, jNextFloat(r) * .5f + .5f, yaw - (float)M_PI / 2, pitch / 3.f, start, end, 1.0);
            nCaveTunnel(jNextLong(r), cx, cz, c, x, y, z, jNextFloat(r) * .5f + .5f, yaw + (float)M_PI / 2, pitch / 3.f, start, end, 1.0); return; }
        if (f2b || r.nextInt(4) != 0) {
            const double d4 = x - d0, d5 = z - d1, d6 = end - start, d7 = width + 2.f + 16.f;
            if (d4 * d4 + d5 * d5 - d6 * d6 > d7 * d7) return;
            if (x >= d0 - 16 - d2 * 2 && z >= d1 - 16 - d2 * 2 && x <= d0 + 16 + d2 * 2 && z <= d1 + 16 + d2 * 2) {
                int k2 = (int)floor(x - d2) - cx * 16 - 1, l2 = (int)floor(x + d2) - cx * 16 + 1, i1 = (int)floor(y - d3) - 1, j1 = (int)floor(y + d3) + 1, k1 = (int)floor(z - d2) - cz * 16 - 1, l1 = (int)floor(z + d2) - cz * 16 + 1;
                k2 = std::max(k2, 0); l2 = std::min(l2, 16); i1 = std::max(i1, 1); j1 = std::min(j1, 120); k1 = std::max(k1, 0); l1 = std::min(l1, 16);
                for (int jx = k2; jx < l2; jx++) { const double d8 = ((jx + cx * 16) + .5 - x) / d2;
                    for (int iz = k1; iz < l1; iz++) { const double d9 = ((iz + cz * 16) + .5 - z) / d2;
                        for (int ky = j1; ky > i1; ky--) { const double d10 = ((ky - 1) + .5 - y) / d3;
                            if (d10 > -.7 && d8 * d8 + d10 * d10 + d9 * d9 < 1.0) { BT& t = c->b[idx(jx, ky, iz)]; if (t == (BT)gNetherrackId || t == (BT)gNSoul || t == GRAVEL) t = (ky - 1 < 10) ? (BT)LAVA : (BT)AIR; } } } }
                if (f2b) break;
            }
        }
    }
}
static void nCaves(Chunk* c, int cx, int cz) {
    JRng r((int64_t)WORLD_SEED); const int64_t a = jNextLong(r), b = jNextLong(r);
    for (int l = cx - 8; l <= cx + 8; l++) for (int m = cz - 8; m <= cz + 8; m++) {
        r.setSeed(((int64_t)l * a) ^ ((int64_t)m * b) ^ (int64_t)WORLD_SEED);
        int n = r.nextInt(r.nextInt(r.nextInt(10) + 1) + 1); if (r.nextInt(5) != 0) n = 0;
        for (int j = 0; j < n; j++) {
            const double x = l * 16 + r.nextInt(16), y = r.nextInt(128), z = m * 16 + r.nextInt(16); int k = 1;
            if (r.nextInt(4) == 0) { nCaveTunnel(jNextLong(r), cx, cz, c, x, y, z, 1.f + jNextFloat(r) * 6.f, 0.f, 0.f, -1, -1, 0.5); k += r.nextInt(4); }
            for (int q = 0; q < k; q++) { const float yaw = jNextFloat(r) * (float)M_PI * 2.f, pit = (jNextFloat(r) - .5f) * 2.f / 8.f, w = (jNextFloat(r) * 2.f + jNextFloat(r)) * 2.f;
                nCaveTunnel(jNextLong(r), cx, cz, c, x, y, z, w, yaw, pit, 0, 0, 0.5); }
        }
    }
}
static void netherBase(Chunk* c, int cx, int cz) {   // terreno + superficie + cavernas (sem estruturas)
    nnEnsure(); const int rack = gNetherrackId, soul = gNSoul;
    static double buf[425]; nnHeights(buf, cx * 4, cz * 4);
    for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {
        const int gx = x >> 2, gz = z >> 2; const double fx = (x & 3) / 4.0, fz = (z & 3) / 4.0;
        for (int y = 0; y < 128; y++) {
            const int gy = y >> 3; const double fy = (y & 7) / 8.0; double v[2][2][2];
            for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) for (int d = 0; d < 2; d++) v[a][b][d] = buf[((gx + a) * 5 + gz + b) * 17 + std::min(gy + d, 16)];
            auto L = [](double p, double q, double t) { return p + (q - p) * t; };
            const double d = L(L(L(v[0][0][0], v[0][0][1], fy), L(v[1][0][0], v[1][0][1], fy), fx), L(L(v[0][1][0], v[0][1][1], fy), L(v[1][1][0], v[1][1][1], fy), fx), fz);
            c->b[idx(x, y, z)] = d > 0.0 ? (BT)rack : y < 32 ? (BT)LAVA : (BT)AIR;
        }
    }
    JRng r((int64_t)cx * 341873128712LL + (int64_t)cz * 132897987541LL);
    static double gb[256], sb[256], db[256]; const double s = 0.03125;
    gNN.sg.gen(gb, cx * 16, cz * 16, 0, 16, 16, 1, s, s, 1.0); gNN.sg.gen(sb, cx * 16, 109, cz * 16, 16, 1, 16, s, 1.0, s); gNN.ex.gen(db, cx * 16, cz * 16, 0, 16, 16, 1, s * 2, s * 2, s * 2);
    for (int j = 0; j < 16; j++) for (int k = 0; k < 16; k++) {
        const bool fg = gb[j + k * 16] + r.nextDouble() * .2 > 0, fs = sb[j + k * 16] + r.nextDouble() * .2 > 0; const int i1 = (int)(db[j + k * 16] / 3.0 + 3.0 + r.nextDouble() * .25);
        int j1 = -1; BT s1 = (BT)rack, s2 = (BT)rack;
        for (int y = 127; y >= 0; y--) {
            BT& t = c->b[idx(k, y, j)];
            if (y < 127 - r.nextInt(5) && y > r.nextInt(5)) {
                if (t == AIR) j1 = -1;
                else if (t == (BT)rack) {
                    if (j1 == -1) {
                        if (i1 <= 0) { s1 = AIR; s2 = (BT)rack; }
                        else if (y >= 60 && y <= 65) { s1 = (BT)rack; s2 = (BT)rack; if (fg) { s1 = GRAVEL; s2 = (BT)rack; } if (fs) { s1 = (BT)soul; s2 = (BT)soul; } }
                        if (y < 64 && s1 == AIR) s1 = (BT)LAVA;
                        j1 = i1; t = y >= 63 ? s1 : s2;
                    } else if (j1 > 0) { --j1; t = s2; }
                }
            } else t = BEDROCK;
        }
    }
    nCaves(c, cx, cz);
}
// cache de terreno-base dos chunks vizinhos (so para a decoracao atravessar a borda do chunk)
static std::map<std::pair<int, int>, Chunk*> gNbBase; static std::deque<std::pair<int, int>> gNbOrder; static int64_t gNbSeed = 0;
static Chunk* nbBase(int cx, int cz) {
    if (gNbSeed != (int64_t)WORLD_SEED) { for (auto& kv : gNbBase) delete kv.second; gNbBase.clear(); gNbOrder.clear(); gNbSeed = (int64_t)WORLD_SEED; }
    auto it = gNbBase.find({cx, cz}); if (it != gNbBase.end()) return it->second;
    Chunk* c = new Chunk(); memset(c->b, 0, sizeof c->b); memset(c->m, 0, sizeof c->m); netherBase(c, cx, cz);
    gNbBase[{cx, cz}] = c; gNbOrder.push_back({cx, cz});
    if (gNbOrder.size() > 40) { auto k = gNbOrder.front(); gNbOrder.pop_front(); auto f = gNbBase.find(k); if (f != gNbBase.end()) { delete f->second; gNbBase.erase(f); } }
    return c;
}
// decoracao como o populate do vanilla: cada chunk-fonte S espalha elementos em [S*16+8, S*16+24); aqui o chunk alvo aplica os 4 fontes que o tocam
static void netherDecor(Chunk* c, int cx, int cz) {
    const int rack = gNetherrackId, quartz = pkFind("block/nether_quartz_ore"), magma = pkFind("block/magma");
    std::unordered_map<int64_t, BT> ov;   // alteracoes feitas por estes elementos (inclusive fora do chunk alvo)
    auto K = [](int x, int y, int z) { return ((int64_t)(x + 100000) << 40) ^ ((int64_t)(z + 100000) << 16) ^ (int64_t)y; };
    auto get = [&](int x, int y, int z) -> BT {
        if (y < 0 || y > 127) return (BT)BEDROCK; auto o = ov.find(K(x, y, z)); if (o != ov.end()) return o->second;
        const int ccx = fdiv(x, 16), ccz = fdiv(z, 16); const Chunk* q = (ccx == cx && ccz == cz) ? c : nbBase(ccx, ccz); return q->b[idx(x - ccx * 16, y, z - ccz * 16)]; };
    auto put = [&](int x, int y, int z, BT id) { ov[K(x, y, z)] = id; if (x >= cx * 16 && x < cx * 16 + 16 && z >= cz * 16 && z < cz * 16 + 16 && y >= 1 && y < 127) c->b[idx(x - cx * 16, y, z - cz * 16)] = id; };
    static const int D6[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int sx = cx - 1; sx <= cx; sx++) for (int sz = cz - 1; sz <= cz; sz++) {
        JRng r((int64_t)sx * 341873128712LL + (int64_t)sz * 132897987541LL ^ (int64_t)WORLD_SEED ^ 0x6E65L);
        const int ox = sx * 16 + 8, oz = sz * 16 + 8;
        for (int i = 0; i < 8; i++) {   // WorldGenHellLava (nascentes na rocha)
            const int x = ox + r.nextInt(16), y = r.nextInt(120) + 4, z = oz + r.nextInt(16);
            if (get(x, y + 1, z) != (BT)rack) continue; const BT b0 = get(x, y, z); if (b0 != AIR && b0 != (BT)rack) continue;
            int nr = 0, na = 0; for (int d : {0, 1, 4, 5, 3}) { const BT t = get(x + D6[d][0], y + D6[d][1], z + D6[d][2]); if (t == (BT)rack) nr++; else if (t == AIR) na++; }
            if ((nr == 4 && na == 1) || nr == 5) put(x, y, z, (BT)LAVA);
        }
        for (int i = 0, n = r.nextInt(r.nextInt(10) + 1) + 1; i < n; i++) {   // WorldGenFire
            const int X = ox + r.nextInt(16), Y = r.nextInt(120) + 4, Z = oz + r.nextInt(16);
            for (int k = 0; k < 64; k++) { const int x = X + r.nextInt(8) - r.nextInt(8), y = Y + r.nextInt(4) - r.nextInt(4), z = Z + r.nextInt(8) - r.nextInt(8);
                if (get(x, y, z) == AIR && get(x, y - 1, z) == (BT)rack) put(x, y, z, (BT)XB_FIRE); } }
        for (int pass = 0; pass < 2; pass++) {   // WorldGenGlowStone1 / 2
            const int n = pass == 0 ? r.nextInt(r.nextInt(10) + 1) : 10;
            for (int i = 0; i < n; i++) {
                const int X = ox + r.nextInt(16), Y = r.nextInt(pass ? 128 : 120) + 4, Z = oz + r.nextInt(16);
                if (get(X, Y, Z) != AIR || get(X, Y + 1, Z) != (BT)rack) continue;
                put(X, Y, Z, GLOWSTONE);
                for (int k = 0; k < 1500; k++) {
                    const int x = X + r.nextInt(8) - r.nextInt(8), y = Y - r.nextInt(12), z = Z + r.nextInt(8) - r.nextInt(8);
                    if (get(x, y, z) != AIR) continue; int cnt = 0; for (auto& d : D6) if (get(x + d[0], y + d[1], z + d[2]) == GLOWSTONE) cnt++;
                    if (cnt == 1) put(x, y, z, GLOWSTONE);
                }
            }
        }
        auto vein = [&](int id, int size, int x0, int y0, int z0, int spread) {
            if (!id) return; for (int k = 0; k < size; k++) { const int x = x0 + r.nextInt(spread * 2 + 1) - spread, y = y0 + r.nextInt(spread + 1) - spread / 2, z = z0 + r.nextInt(spread * 2 + 1) - spread;
                if (get(x, y, z) == (BT)rack) put(x, y, z, (BT)id); } };
        for (int i = 0; i < 16; i++) { const int x = sx * 16 + r.nextInt(16), y = r.nextInt(108) + 10, z = sz * 16 + r.nextInt(16); vein(quartz, 14, x, y, z, 2); }
        for (int i = 0; i < 4; i++) { const int x = sx * 16 + r.nextInt(16), y = 31 + r.nextInt(3), z = sz * 16 + r.nextInt(16); vein(magma, 33, x, y, z, 3); }
        for (int i = 0; i < 16; i++) {   // WorldGenHellLava (dentro da rocha: so se cercada de rocha)
            const int x = sx * 16 + r.nextInt(16), y = r.nextInt(108) + 10, z = sz * 16 + r.nextInt(16);
            if (get(x, y + 1, z) != (BT)rack || get(x, y, z) != (BT)rack) continue; int nr = 0; for (int d : {0, 1, 4, 5, 3}) if (get(x + D6[d][0], y + D6[d][1], z + D6[d][2]) == (BT)rack) nr++;
            if (nr == 5) put(x, y, z, (BT)LAVA);
        }
    }
}
