// ---------- terreno no estilo Minecraft Java 1.12.2 (ChunkGeneratorOverworld) ----------
// java.util.Random + ruído Perlin melhorado em oitavas + mistura de biomas 5x5 + densidade 3D interpolada (4x8x4)
// + camada de superfície (terra/areia/pedra) + bedrock. Nível do mar = 63, altura = 256.
struct JRng {   // réplica exata de java.util.Random
    uint64_t s = 0;
    explicit JRng(int64_t v = 0) { setSeed(v); }
    void setSeed(int64_t v) { s = ((uint64_t)v ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1); }
    int next(int bits) { s = (s * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1); return (int)(int32_t)(uint32_t)(s >> (48 - bits)); }
    int nextInt() { return next(32); }
    int nextInt(int bound) {
        if ((bound & -bound) == bound) return (int)(((int64_t)bound * (int64_t)next(31)) >> 31);
        int bits, val;
        do { bits = next(31); val = bits % bound; } while ((int32_t)((uint32_t)bits - (uint32_t)val + (uint32_t)(bound - 1)) < 0);
        return val;
    }
    double nextDouble() { return (double)(((int64_t)next(26) << 27) + next(27)) * (1.0 / (double)(1LL << 53)); }
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O3")   // patchPerf.3: ruido quente sempre em O3 (o build padrao e -O1)
#endif
struct PerlinImp {   // NoiseGeneratorImproved
    double xo, yo, zo; int p[512];
    void init(JRng& r) {
        xo = r.nextDouble() * 256.0; yo = r.nextDouble() * 256.0; zo = r.nextDouble() * 256.0;
        for (int i = 0; i < 256; i++) p[i] = i;
        for (int i = 0; i < 256; i++) { int j = r.nextInt(256 - i) + i; int k = p[i]; p[i] = p[j]; p[j] = k; p[i + 256] = p[i]; }
    }
    static double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
    static double lerp(double t, double a, double b) { return a + t * (b - a); }
    static double grad(int h, double x, double y, double z) {   // patchPerf.3: tabela sem ramos (resultado identico ao switch original)
        static const signed char G[16][3] = {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1},{1,1,0},{0,-1,1},{-1,1,0},{0,-1,-1}};
        const signed char* g = G[h & 15]; return g[0] * x + g[1] * y + g[2] * z;
    }
    static double grad2(int h, double x, double z) {
        int i = h & 15; double a = (double)(i & 8) / 8.0, b = i < 4 ? 0.0 : (i != 12 && i != 14 ? z : x);
        return ((i & 1) == 0 ? a : -a) + ((i & 2) == 0 ? b : -b);
    }
    void populate(double* out, double xOff, double yOff, double zOff, int xs, int ys, int zs, double xSc, double ySc, double zSc, double noiseScale) const {
        int n = 0; const double inv = 1.0 / noiseScale;
        if (ys == 1) {
            for (int a = 0; a < xs; a++) {
                double dx = xOff + (double)a * xSc + xo; int xi = (int)dx; if (dx < (double)xi) xi--;
                int X = xi & 255; dx -= (double)xi; double u = fade(dx);
                for (int c = 0; c < zs; c++) {
                    double dz = zOff + (double)c * zSc + zo; int zi = (int)dz; if (dz < (double)zi) zi--;
                    int Z = zi & 255; dz -= (double)zi; double w = fade(dz);
                    int l = p[X] + 0, j = p[l] + Z, k = p[X + 1] + 0, l1 = p[k] + Z;
                    double d7 = lerp(u, grad2(p[j], dx, dz), grad(p[l1], dx - 1.0, 0.0, dz));
                    double d8 = lerp(u, grad(p[j + 1], dx, 0.0, dz - 1.0), grad(p[l1 + 1], dx - 1.0, 0.0, dz - 1.0));
                    out[n++] += lerp(w, d7, d8) * inv;
                }
            }
            return;
        }
        if (ys <= 64) {   // patchPerf.3: floor/fade do eixo Y calculados 1x por camada (antes: a cada amostra x,z); mesmas contas => mesmo resultado
            double dyA[64], vA[64]; int YA[64];
            for (int b = 0; b < ys; b++) { double dy = yOff + (double)b * ySc + yo; int yi = (int)dy; if (dy < (double)yi) yi--; YA[b] = yi & 255; dyA[b] = dy - (double)yi; vA[b] = fade(dyA[b]); }
            int prevY = -1; double d1 = 0, d2 = 0, d3 = 0, d4 = 0;
            for (int a = 0; a < xs; a++) {
                double dx = xOff + (double)a * xSc + xo; int xi = (int)dx; if (dx < (double)xi) xi--;
                int X = xi & 255; dx -= (double)xi; double u = fade(dx);
                for (int c = 0; c < zs; c++) {
                    double dz = zOff + (double)c * zSc + zo; int zi = (int)dz; if (dz < (double)zi) zi--;
                    int Z = zi & 255; dz -= (double)zi; double w = fade(dz);
                    for (int b = 0; b < ys; b++) {
                        const int Y = YA[b]; const double dy = dyA[b], v = vA[b];
                        if (b == 0 || Y != prevY) {
                            prevY = Y;
                            int j = p[X] + Y, k = p[j] + Z, l = p[j + 1] + Z, i1 = p[X + 1] + Y, j1 = p[i1] + Z, k1 = p[i1 + 1] + Z;
                            d1 = lerp(u, grad(p[k], dx, dy, dz), grad(p[j1], dx - 1.0, dy, dz));
                            d2 = lerp(u, grad(p[l], dx, dy - 1.0, dz), grad(p[k1], dx - 1.0, dy - 1.0, dz));
                            d3 = lerp(u, grad(p[k + 1], dx, dy, dz - 1.0), grad(p[j1 + 1], dx - 1.0, dy, dz - 1.0));
                            d4 = lerp(u, grad(p[l + 1], dx, dy - 1.0, dz - 1.0), grad(p[k1 + 1], dx - 1.0, dy - 1.0, dz - 1.0));
                        }
                        out[n++] += lerp(w, lerp(v, d1, d2), lerp(v, d3, d4)) * inv;
                    }
                }
            }
            return;
        }
        int prevY = -1; double d1 = 0, d2 = 0, d3 = 0, d4 = 0;
        for (int a = 0; a < xs; a++) {
            double dx = xOff + (double)a * xSc + xo; int xi = (int)dx; if (dx < (double)xi) xi--;
            int X = xi & 255; dx -= (double)xi; double u = fade(dx);
            for (int c = 0; c < zs; c++) {
                double dz = zOff + (double)c * zSc + zo; int zi = (int)dz; if (dz < (double)zi) zi--;
                int Z = zi & 255; dz -= (double)zi; double w = fade(dz);
                for (int b = 0; b < ys; b++) {
                    double dy = yOff + (double)b * ySc + yo; int yi = (int)dy; if (dy < (double)yi) yi--;
                    int Y = yi & 255; dy -= (double)yi; double v = fade(dy);
                    if (b == 0 || Y != prevY) {
                        prevY = Y;
                        int j = p[X] + Y, k = p[j] + Z, l = p[j + 1] + Z, i1 = p[X + 1] + Y, j1 = p[i1] + Z, k1 = p[i1 + 1] + Z;
                        d1 = lerp(u, grad(p[k], dx, dy, dz), grad(p[j1], dx - 1.0, dy, dz));
                        d2 = lerp(u, grad(p[l], dx, dy - 1.0, dz), grad(p[k1], dx - 1.0, dy - 1.0, dz));
                        d3 = lerp(u, grad(p[k + 1], dx, dy, dz - 1.0), grad(p[j1 + 1], dx - 1.0, dy, dz - 1.0));
                        d4 = lerp(u, grad(p[l + 1], dx, dy - 1.0, dz - 1.0), grad(p[k1 + 1], dx - 1.0, dy - 1.0, dz - 1.0));
                    }
                    out[n++] += lerp(w, lerp(v, d1, d2), lerp(v, d3, d4)) * inv;
                }
            }
        }
    }
};
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
struct OctNoise {   // NoiseGeneratorOctaves
    std::vector<PerlinImp> g;
    void init(JRng& r, int oct) { g.resize(oct); for (auto& o : g) o.init(r); }
    void gen(double* out, int xo, int yo, int zo, int xs, int ys, int zs, double xSc, double ySc, double zSc) const {
        for (int i = 0; i < xs * ys * zs; i++) out[i] = 0.0;
        double d3 = 1.0;
        for (size_t j = 0; j < g.size(); j++) {
            double d0 = (double)xo * d3 * xSc, d1 = (double)yo * d3 * ySc, d2 = (double)zo * d3 * zSc;
            int64_t k = (int64_t)floor(d0), l = (int64_t)floor(d2);
            d0 -= (double)k; d2 -= (double)l; k %= 16777216LL; l %= 16777216LL; d0 += (double)k; d2 += (double)l;
            g[j].populate(out, d0, d1, d2, xs, ys, zs, xSc * d3, ySc * d3, zSc * d3, d3);
            d3 /= 2.0;
        }
    }
};
struct SimplexImp {   // NoiseGeneratorSimplex (2D)
    double xo, yo, zo; int p[512];
    void init(JRng& r) {
        xo = r.nextDouble() * 256.0; yo = r.nextDouble() * 256.0; zo = r.nextDouble() * 256.0;
        for (int i = 0; i < 256; i++) p[i] = i;
        for (int l = 0; l < 256; l++) { int j = r.nextInt(256 - l) + l; int k = p[l]; p[l] = p[j]; p[j] = k; p[l + 256] = p[l]; }
    }
    double value(double x, double y) const {
        static const int G[12][3] = {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}};
        const double S3 = sqrt(3.0), F2 = 0.5 * (S3 - 1.0), G2 = (3.0 - S3) / 6.0;
        double s = (x + y) * F2; int i = (int)floor(x + s), j = (int)floor(y + s);
        double t = (double)(i + j) * G2, x0 = x - ((double)i - t), y0 = y - ((double)j - t);
        int i1 = x0 > y0 ? 1 : 0, j1 = x0 > y0 ? 0 : 1;
        double x1 = x0 - i1 + G2, y1 = y0 - j1 + G2, x2 = x0 - 1.0 + 2.0 * G2, y2 = y0 - 1.0 + 2.0 * G2;
        int ii = i & 255, jj = j & 255;
        int g0 = p[ii + p[jj]] % 12, g1 = p[ii + i1 + p[jj + j1]] % 12, g2 = p[ii + 1 + p[jj + 1]] % 12;
        auto contrib = [](double tt, const int* g, double a, double b) { if (tt < 0.0) return 0.0; tt *= tt; return tt * tt * (g[0] * a + g[1] * b); };
        return 70.0 * (contrib(0.5 - x0 * x0 - y0 * y0, G[g0], x0, y0) + contrib(0.5 - x1 * x1 - y1 * y1, G[g1], x1, y1) + contrib(0.5 - x2 * x2 - y2 * y2, G[g2], x2, y2));
    }
    void add(double* out, double xOff, double zOff, int xs, int zs, double xSc, double zSc, double amp) const {
        int n = 0;
#ifdef SURF_X_MAJOR   // layout antigo (x externo); so para comparar
        for (int a = 0; a < xs; a++) { double dx = (xOff + (double)a) * xSc + xo; for (int c = 0; c < zs; c++) out[n++] += value(dx, (zOff + (double)c) * zSc + yo) * amp; }
#else   // NoiseGeneratorSimplex.add do 1.12.2: z externo, x interno => indice = z*xs + x (casa com depthBuffer[j + i*16])
        for (int c = 0; c < zs; c++) { const double dy = (zOff + (double)c) * zSc + yo; for (int a = 0; a < xs; a++) out[n++] += value((xOff + (double)a) * xSc + xo, dy) * amp; }
#endif
    }
};
struct SurfNoise {   // NoiseGeneratorPerlin (4 níveis) - espessura da camada de terra
    SimplexImp lv[4];
    void init(JRng& r) { for (auto& s : lv) s.init(r); }
    void region(double* out, double xOff, double zOff, int xs, int zs, double xSc, double zSc) const {
        for (int i = 0; i < xs * zs; i++) out[i] = 0.0;
        xSc /= 1.5; zSc /= 1.5; double d0 = 1.0, d1 = 1.0;
        for (int j = 0; j < 4; j++) { lv[j].add(out, xOff, zOff, xs, zs, xSc * d1, zSc * d1, 0.55 / d0); d1 *= 1.0; d0 *= 0.5; }
    }
};

struct T112 {
    bool ready = false; int64_t seed = 0;
    OctNoise minL, maxL, mainN, scaleN, depthN; SurfNoise surf;
    void init(int64_t s) {
        seed = s; JRng r(s);   // ordem do construtor do ChunkGeneratorOverworld
        minL.init(r, 16); maxL.init(r, 16); mainN.init(r, 8); surf.init(r); scaleN.init(r, 10); depthN.init(r, 16);
        ready = true;
    }
};
static T112 g112;

// ---- camadas de bioma: porte completo do GenLayer do 1.12.2 (ilhas, neve, bordas, rios, colinas, praias, mistura de rio, VoronoiZoom) ----
// ids internos = ids de bioma do Minecraft (0 oceano, 1 planície, 2 deserto, 3 montanhas, 4 floresta, 5 taiga, 6 pântano, 7 rio ... 129+ = mutações)
enum { GK_ISLAND, GK_FUZZY, GK_ZOOM, GK_ADD, GK_RMO, GK_SNOW, GK_EDGE, GK_MUSH, GK_DEEP, GK_RIVINIT, GK_BIOME, GK_BEDGE, GK_HILLS, GK_RIVER, GK_SMOOTH, GK_RARE, GK_SHORE, GK_MIX, GK_VORO };
static const float V_TEMP[40] = {.5f,.8f,2.f,.2f,.7f,.25f,.8f,.5f,0,0,0.f,0.f,0.f,0.f,.9f,.9f,.8f,2.f,.7f,.25f,.2f,.95f,.95f,.95f,.5f,.2f,.05f,.6f,.6f,.7f,-.5f,-.5f,.3f,.3f,.2f,1.2f,1.f,2.f,2.f,2.f};
static const float V_BV[40][2] = {{-1,.1f},{.125f,.05f},{.125f,.05f},{1,.5f},{.1f,.2f},{.2f,.2f},{-.2f,.1f},{-.5f,0},{0,0},{0,0},{-1,.1f},{-.5f,0},{.125f,.05f},{.45f,.3f},{.2f,1},{-1,.1f},{0,.025f},{.45f,.3f},{.45f,.3f},{.45f,.3f},{.8f,.3f},{.1f,.2f},{.45f,.3f},{.1f,.2f},{-1.8f,.1f},{.1f,.8f},{0,.025f},{.1f,.2f},{.45f,.3f},{.1f,.2f},{.2f,.2f},{.45f,.3f},{.2f,.2f},{.45f,.3f},{1,.5f},{.125f,.05f},{1.5f,.025f},{.1f,.2f},{1.5f,.025f},{1.5f,.025f}};
static void vbBV(int id, float& base, float& var) {
    if (id < 40) { base = V_BV[id][0]; var = V_BV[id][1]; return; }
    switch (id) {   // mutações (BiomeProperties do 1.12.2)
    case 129: base = .125f; var = .05f; return; case 130: base = .225f; var = .25f; return; case 132: base = .1f; var = .4f; return;
    case 133: base = .3f; var = .4f; return; case 134: base = -.1f; var = .3f; return; case 140: base = .425f; var = .45f; return;
    case 163: base = .3625f; var = 1.225f; return; case 164: base = 1.05f; var = 1.2125f; return; case 165: base = .1f; var = .2f; return;
    case 166: case 167: base = .45f; var = .3f; return; case 160: case 161: base = .2f; var = .2f; return;
    default: if (id - 128 < 40) vbBV(id - 128, base, var); else { base = .1f; var = .2f; } if (id == 131 || id == 162) { base = .2f; var = .3f; } else if (id != 156 && id != 128 + 3) { base += .1f; var += .2f; } return;
    }
}
static inline bool vbOcean(int v) { return v == 0 || v == 24 || v == 10; }
static inline bool vbMesaP(int v) { return v == 38 || v == 39; }
static inline bool vbEq(int a, int b) { return a == b || (vbMesaP(a) && vbMesaP(b)); }
static inline int vbCat(int v) { const float t = v < 40 ? V_TEMP[v] : V_TEMP[v - 128 < 40 ? v - 128 : 1]; return t < .2f ? 0 : t < 1.f ? 1 : 2; }
static inline bool vbNeigh(int a, int b) { return vbEq(a, b) || vbCat(a) == vbCat(b); }
static inline bool vbSnowy(int v) { return v == 10 || v == 11 || v == 12 || v == 13 || v == 26 || v == 30 || v == 31 || v == 140 || v == 158; }
static inline bool vbJungleOk(int v) { return v == 23 || v == 21 || v == 22 || v == 4 || v == 5 || vbOcean(v); }
static inline bool vbHasMut(int v) { static const int M[] = {1, 2, 3, 4, 5, 6, 12, 21, 23, 27, 28, 29, 30, 32, 33, 34, 35, 36, 37, 38, 39}; for (int m : M) if (m == v) return true; return false; }
static int vbGame(int v) {   // bioma do Minecraft -> bioma do jogo (cores, vegetação, mobs)
    if (v >= 128) v -= 128;
    switch (v) {
    case 0: case 10: case 24: return B_OCEAN;
    case 16: case 25: case 26: return B_BEACH;
    case 2: case 17: case 37: case 38: case 39: return B_DESERT;
    case 3: case 20: case 34: return B_MOUNTAINS;
    case 4: case 18: case 27: case 28: case 29: return B_FOREST;
    case 5: case 19: case 32: case 33: return B_TAIGA;
    case 30: case 31: case 11: case 12: case 13: return B_TUNDRA;
    case 6: return B_SWAMP;
    case 21: case 22: case 23: return B_JUNGLE;
    case 35: case 36: return B_SAVANNA;
    default: return B_PLAINS;   // planície, rio, cogumelo
    }
}
static inline int64_t glStep(int64_t s, int64_t add) { const uint64_t u = (uint64_t)s; return (int64_t)(u * (u * 6364136223846793005ULL + 1442695040888963407ULL) + (uint64_t)add); }
struct GLayer {
    int kind = 0, p1 = -1, p2 = -1, param = 0; int64_t base = 0, ws = 0, cs = 0;
    void initBase(int64_t b) { base = b; for (int i = 0; i < 3; i++) base = glStep(base, b); }
    void initWorld(int64_t s) { ws = s; for (int i = 0; i < 3; i++) ws = glStep(ws, base); }
    void initChunk(int64_t x, int64_t z) { cs = ws; cs = glStep(cs, x); cs = glStep(cs, z); cs = glStep(cs, x); cs = glStep(cs, z); }
    int nextInt(int m) { int i = (int)((cs >> 24) % m); if (i < 0) i += m; cs = glStep(cs, ws); return i; }
    int pick2(int a, int b) { return nextInt(2) == 0 ? a : b; }
    int pick4(int a, int b, int c, int d) { const int i = nextInt(4); return i == 0 ? a : i == 1 ? b : i == 2 ? c : d; }
};
static int glMode(GLayer& L, int a, int b, int c, int d) {   // GenLayerZoom.selectModeOrRandom
    if (b == c && c == d) return b; if (a == b && a == c) return a; if (a == b && a == d) return a; if (a == c && a == d) return a;
    if (a == b && c != d) return a; if (a == c && b != d) return a; if (a == d && b != c) return a;
    if (b == c && a != d) return b; if (b == d && a != c) return b; if (c == d && a != b) return c;
    return L.pick4(a, b, c, d);
}
struct GLStack {
    std::vector<GLayer> L; int64_t seed = INT64_MIN; int mix = 0, vor = 0;
    int add(int k, int64_t bs, int p1 = -1, int p2 = -1, int param = 0) { GLayer l; l.kind = k; l.p1 = p1; l.p2 = p2; l.param = param; l.initBase(bs); L.push_back(l); return (int)L.size() - 1; }
    void build(int64_t s) {   // GenLayer.initializeAllBiomeGenerators (1.12.2, tamanho de bioma 4)
        L.clear(); seed = s;
        int a = add(GK_ISLAND, 1); a = add(GK_FUZZY, 2000, a); a = add(GK_ADD, 1, a); a = add(GK_ZOOM, 2001, a); a = add(GK_ADD, 2, a); a = add(GK_ADD, 50, a); a = add(GK_ADD, 70, a);
        a = add(GK_RMO, 2, a); a = add(GK_SNOW, 2, a); a = add(GK_ADD, 3, a);
        a = add(GK_EDGE, 2, a, -1, 0); a = add(GK_EDGE, 2, a, -1, 1); a = add(GK_EDGE, 3, a, -1, 2);
        a = add(GK_ZOOM, 2002, a); a = add(GK_ZOOM, 2003, a); a = add(GK_ADD, 4, a); a = add(GK_MUSH, 5, a); a = add(GK_DEEP, 4, a);
        const int ri = add(GK_RIVINIT, 100, a);
        int bi = add(GK_BIOME, 200, a); bi = add(GK_ZOOM, 1000, bi); bi = add(GK_ZOOM, 1001, bi); bi = add(GK_BEDGE, 1000, bi);
        int r2 = add(GK_ZOOM, 1000, ri); r2 = add(GK_ZOOM, 1001, r2);
        const int hills = add(GK_HILLS, 1000, bi, r2);
        int r5 = ri; for (int j = 0; j < 2; j++) r5 = add(GK_ZOOM, 1000 + j, r5); for (int j = 0; j < 4; j++) r5 = add(GK_ZOOM, 1000 + j, r5);
        const int sm = add(GK_SMOOTH, 1000, add(GK_RIVER, 1, r5));
        int x = add(GK_RARE, 1001, hills);
        for (int j = 0; j < 4; j++) { x = add(GK_ZOOM, 1000 + j, x); if (j == 0) x = add(GK_ADD, 3, x); if (j == 1) x = add(GK_SHORE, 1000, x); }
        x = add(GK_SMOOTH, 1000, x); mix = add(GK_MIX, 100, x, sm); vor = add(GK_VORO, 10, mix);
        for (auto& l : L) l.initWorld(s);
    }
    std::vector<int> get(int li, int x, int z, int w, int h) {
        GLayer& Y = L[li]; std::vector<int> out((size_t)w * h);
        switch (Y.kind) {
        case GK_ISLAND:
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) { Y.initChunk(x + i, z + j); out[i + j * w] = Y.nextInt(10) == 0 ? 1 : 0; }
            if (x > -w && x <= 0 && z > -h && z <= 0) out[-x + -z * w] = 1;
            break;
        case GK_FUZZY: case GK_ZOOM: {
            const int px = x >> 1, pz = z >> 1, pw = (w >> 1) + 3, ph = (h >> 1) + 3; const std::vector<int> p = get(Y.p1, px, pz, pw, ph);
            const int tw = pw << 1; std::vector<int> t((size_t)tw * (ph * 2));
            for (int j = 0; j < ph - 1; j++) {
                int k = (j << 1) * tw, a = p[j * pw], c = p[(j + 1) * pw];
                for (int i = 0; i < pw - 1; i++) {
                    Y.initChunk((int64_t)(i + px) << 1, (int64_t)(j + pz) << 1);
                    const int b = p[i + 1 + j * pw], d = p[i + 1 + (j + 1) * pw];
                    t[k] = a; t[k + tw] = Y.pick2(a, c); k++;
                    t[k] = Y.pick2(a, b); t[k + tw] = Y.kind == GK_FUZZY ? Y.pick4(a, b, c, d) : glMode(Y, a, b, c, d); k++;
                    a = b; c = d;
                }
            }
            for (int j = 0; j < h; j++) memcpy(&out[(size_t)j * w], &t[(size_t)(j + (z & 1)) * tw + (x & 1)], (size_t)w * sizeof(int));
        } break;
        case GK_VORO: {   // GenLayerVoronoiZoom
            x -= 2; z -= 2; const int px = x >> 2, pz = z >> 2, pw = (w >> 2) + 2, ph = (h >> 2) + 2; const std::vector<int> p = get(Y.p1, px, pz, pw, ph);
            const int i1 = (pw - 1) << 2, j1 = (ph - 1) << 2; std::vector<int> t((size_t)i1 * j1);
            for (int k1 = 0; k1 < ph - 1; k1++) {
                int i2 = p[k1 * pw], j2 = p[(k1 + 1) * pw];
                for (int l1 = 0; l1 < pw - 1; l1++) {
                    double dd[8]; const int cx[4] = {0, 1, 0, 1}, cz[4] = {0, 0, 1, 1};
                    for (int q = 0; q < 4; q++) {
                        Y.initChunk((int64_t)(l1 + px + cx[q]) << 2, (int64_t)(k1 + pz + cz[q]) << 2);
                        dd[q * 2] = ((double)Y.nextInt(1024) / 1024.0 - .5) * 3.6 + (cx[q] ? 4.0 : 0.0);
                        dd[q * 2 + 1] = ((double)Y.nextInt(1024) / 1024.0 - .5) * 3.6 + (cz[q] ? 4.0 : 0.0);
                    }
                    const int k2 = p[l1 + 1 + k1 * pw], l2 = p[l1 + 1 + (k1 + 1) * pw];
                    for (int i3 = 0; i3 < 4; i3++) {
                        int j3 = ((k1 << 2) + i3) * i1 + (l1 << 2);
                        for (int k3 = 0; k3 < 4; k3++) {
                            // vanilla: d1 = x-ruído de 0, d2 = z-ruído de 0 (k3 usa d1, i3 usa d2)
                            double d[4]; for (int q = 0; q < 4; q++) d[q] = ((double)i3 - dd[q * 2 + 1]) * ((double)i3 - dd[q * 2 + 1]) + ((double)k3 - dd[q * 2]) * ((double)k3 - dd[q * 2]);
                            int v;
                            if (d[0] < d[1] && d[0] < d[2] && d[0] < d[3]) v = i2; else if (d[1] < d[0] && d[1] < d[2] && d[1] < d[3]) v = k2; else if (d[2] < d[0] && d[2] < d[1] && d[2] < d[3]) v = j2; else v = l2;
                            t[j3++] = v;
                        }
                    }
                    i2 = k2; j2 = l2;
                }
            }
            for (int j = 0; j < h; j++) memcpy(&out[(size_t)j * w], &t[(size_t)(j + (z & 3)) * i1 + (x & 3)], (size_t)w * sizeof(int));
        } break;
        case GK_MIX: {
            const std::vector<int> b = get(Y.p1, x, z, w, h), r = get(Y.p2, x, z, w, h);
            for (int i = 0; i < w * h; i++) {
                if (vbOcean(b[i])) out[i] = b[i];
                else if (r[i] == 7) out[i] = b[i] == 12 ? 11 : (b[i] == 14 || b[i] == 15) ? 15 : 7;
                else out[i] = b[i];
            }
        } break;
        case GK_HILLS: {
            const int pw = w + 2; const std::vector<int> p = get(Y.p1, x - 1, z - 1, pw, h + 2), rv = get(Y.p2, x - 1, z - 1, pw, h + 2);
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
                Y.initChunk(i + x, j + z);
                const int c = i + 1 + (j + 1) * pw, k = p[c], l = rv[c]; const bool flag = (l - 2) % 29 == 0;
                int o;
                if (k != 0 && l >= 2 && (l - 2) % 29 == 1 && k < 128) o = vbHasMut(k) ? k + 128 : k;
                else if (Y.nextInt(3) != 0 && !flag) o = k;
                else {
                    int h1 = k;
                    switch (k) {
                    case 2: h1 = 17; break; case 4: h1 = 18; break; case 27: h1 = 28; break; case 29: h1 = 1; break; case 5: h1 = 19; break; case 32: h1 = 33; break;
                    case 30: h1 = 31; break; case 12: h1 = 13; break; case 21: h1 = 22; break; case 3: h1 = 34; break; case 35: h1 = 36; break;
                    case 1: h1 = Y.nextInt(3) == 0 ? 18 : 4; break;
                    default:
                        if (vbMesaP(k)) h1 = 37;
                        else if (k == 24 && Y.nextInt(3) == 0) h1 = Y.nextInt(2) == 0 ? 1 : 4;
                    }
                    if (flag && h1 != k) h1 = vbHasMut(h1) ? h1 + 128 : k;
                    if (h1 == k) o = k;
                    else {
                        int n = 0; for (int q : {p[i + 1 + j * pw], p[i + 2 + (j + 1) * pw], p[i + (j + 1) * pw], p[i + 1 + (j + 2) * pw]}) if (vbEq(q, k)) n++;
                        o = n >= 3 ? h1 : k;
                    }
                }
                out[i + j * w] = o;
            }
        } break;
        default: {   // camadas de vizinhança 3x3 com um único pai
            const int pw = w + 2; const std::vector<int> p = get(Y.p1, x - 1, z - 1, pw, h + 2);
            for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
                const int a = p[i + j * pw], b = p[i + 2 + j * pw], c = p[i + (j + 2) * pw], d = p[i + 2 + (j + 2) * pw];   // diagonais
                const int n = p[i + 1 + j * pw], e = p[i + 2 + (j + 1) * pw], wv = p[i + (j + 1) * pw], so = p[i + 1 + (j + 2) * pw], m = p[i + 1 + (j + 1) * pw];
                Y.initChunk(i + x, j + z); int o = m;
                switch (Y.kind) {
                case GK_ADD:
                    if (m == 0 && (a || b || c || d)) {
                        int cnt = 1, pk = 1;
                        if (a && Y.nextInt(cnt++) == 0) pk = a; if (b && Y.nextInt(cnt++) == 0) pk = b; if (c && Y.nextInt(cnt++) == 0) pk = c; if (d && Y.nextInt(cnt++) == 0) pk = d;
                        o = Y.nextInt(3) == 0 ? pk : (pk == 4 ? 4 : 0);
                    } else if (m > 0 && (!a || !b || !c || !d)) { if (Y.nextInt(5) == 0) o = m == 4 ? 4 : 0; }
                    break;
                case GK_RMO: if (!m && !n && !e && !wv && !so && Y.nextInt(2) == 0) o = 1; break;
                case GK_SNOW: if (m) { int r = Y.nextInt(6); o = r == 0 ? 4 : r <= 1 ? 3 : 1; } break;
                case GK_EDGE:
                    if (Y.param == 0) { if (m == 1 && (n == 3 || n == 4 || e == 3 || e == 4 || wv == 3 || wv == 4 || so == 3 || so == 4)) o = 2; }
                    else if (Y.param == 1) { if (m == 4 && (n == 1 || n == 2 || e == 1 || e == 2 || wv == 1 || wv == 2 || so == 1 || so == 2)) o = 3; }
                    else if (m != 0 && Y.nextInt(13) == 0) o = m | (((1 + Y.nextInt(15)) << 8) & 3840);
                    break;
                case GK_MUSH: if (m == 0 && !a && !b && !c && !d && Y.nextInt(100) == 0) o = 14; break;
                case GK_DEEP: { int k = (n == 0) + (e == 0) + (wv == 0) + (so == 0); if (m == 0 && k > 3) o = 24; } break;
                case GK_RIVINIT: o = m > 0 ? Y.nextInt(299999) + 2 : 0; break;
                case GK_BIOME: {
                    static const int HOT[6] = {2, 2, 2, 35, 35, 1}, WARM[6] = {4, 29, 3, 1, 27, 6}, COLD[4] = {4, 3, 5, 1}, ICE[4] = {12, 12, 30, 12};
                    const int sp = (m & 3840) >> 8, k = m & ~3840;
                    if (vbOcean(k) || k == 14) o = k;
                    else if (k == 1) o = sp > 0 ? (Y.nextInt(3) == 0 ? 39 : 38) : HOT[Y.nextInt(6)];
                    else if (k == 2) o = sp > 0 ? 21 : WARM[Y.nextInt(6)];
                    else if (k == 3) o = sp > 0 ? 32 : COLD[Y.nextInt(4)];
                    else if (k == 4) o = ICE[Y.nextInt(4)];
                    else o = 14;
                } break;
                case GK_BEDGE: {
                    auto strict = [&](int old, int nw) { if (!vbEq(m, old)) return false; o = (vbEq(n, old) && vbEq(e, old) && vbEq(wv, old) && vbEq(so, old)) ? m : nw; return true; };
                    if (vbEq(m, 3)) { o = (vbNeigh(n, 3) && vbNeigh(e, 3) && vbNeigh(wv, 3) && vbNeigh(so, 3)) ? m : 20; }
                    else if (strict(38, 37) || strict(39, 37) || strict(32, 5)) {}
                    else if (m == 2) { if (n == 12 || e == 12 || wv == 12 || so == 12) o = 34; }
                    else if (m == 6) {
                        if (n == 2 || e == 2 || wv == 2 || so == 2 || n == 30 || e == 30 || wv == 30 || so == 30 || n == 12 || e == 12 || wv == 12 || so == 12) o = 1;
                        else if (n == 21 || e == 21 || wv == 21 || so == 21 || n == 22 || e == 22 || wv == 22 || so == 22) o = 23;
                    }
                } break;
                case GK_RIVER: {
                    auto rf = [](int v) { return v >= 2 ? 2 + (v & 1) : v; }; const int k1 = rf(m);
                    o = (k1 == rf(wv) && k1 == rf(n) && k1 == rf(e) && k1 == rf(so)) ? -1 : 7;
                } break;
                case GK_SMOOTH:
                    if (wv == e && n == so) o = Y.nextInt(2) == 0 ? wv : n; else { if (wv == e) o = wv; if (n == so) o = n; }
                    break;
                case GK_RARE: if (Y.nextInt(57) == 0 && m == 1) o = 129; break;
                case GK_SHORE: {
                    const bool anyO = vbOcean(n) || vbOcean(e) || vbOcean(wv) || vbOcean(so);
                    if (m == 14) o = anyO ? 15 : m;
                    else if (m == 21 || m == 22 || m == 23 || m == 149 || m == 151) {
                        if (vbJungleOk(n) && vbJungleOk(e) && vbJungleOk(wv) && vbJungleOk(so)) o = anyO ? 16 : m; else o = 23;
                    } else if (m != 3 && m != 34 && m != 20) {
                        if (vbSnowy(m)) o = anyO ? 26 : m;
                        else if (m != 37 && m != 38) { if (m != 0 && m != 24 && m != 7 && m != 6) o = anyO ? 16 : m; }
                        else if (!anyO) { const bool mz = (n == 37 || vbMesaP(n)) && (e == 37 || vbMesaP(e)) && (wv == 37 || vbMesaP(wv)) && (so == 37 || vbMesaP(so)); o = mz ? m : 2; }
                    } else o = anyO ? 25 : m;
                } break;
                }
                out[i + j * w] = o;
            }
        } }
        return out;
    }
};
static GLStack gGL;
static void gGLEnsure() { if (gGL.seed != (int64_t)WORLD_SEED) { gGL.build((int64_t)WORLD_SEED); } }
static std::unordered_map<int64_t, std::array<uint8_t, 256>> gBioTile;   // cache: bioma do jogo por bloco, em tiles 16x16
static int64_t gBioTileSeed = INT64_MIN;
static int biomeAt(float wx, float wz) {
    gGLEnsure(); if (gBioTileSeed != WORLD_SEED) { gBioTileSeed = WORLD_SEED; gBioTile.clear(); }
    const int ix = (int)floorf(wx), iz = (int)floorf(wz), tx = ix >> 4, tz = iz >> 4; const int64_t k = ((int64_t)tx << 32) ^ (int64_t)(uint32_t)tz;
    auto it = gBioTile.find(k);
    if (it == gBioTile.end()) { const std::vector<int> r = gGL.get(gGL.vor, tx * 16, tz * 16, 16, 16); std::array<uint8_t, 256> a; for (int i = 0; i < 256; i++) a[i] = (uint8_t)vbGame(r[i]); it = gBioTile.emplace(k, a).first; }
    return it->second[(ix & 15) + (iz & 15) * 16];
}
static std::unordered_map<int64_t, std::array<uint8_t, 256>> gBioRaw;   // patchBio.1: id vanilla (1.12) por bloco, para a cor de grama/folha exata de cada bioma
static int64_t gBioRawSeed = INT64_MIN;   // patchPerf.1: era int -> seed grande (64 bits) nunca batia e o cache de bioma era limpo A CADA chamada (~66 us cada, 900 ms por chunk)
static int biomeRawAt(float wx, float wz) {
    gGLEnsure(); if (gBioRawSeed != (int64_t)WORLD_SEED) { gBioRawSeed = (int64_t)WORLD_SEED; gBioRaw.clear(); }
    const int ix = (int)floorf(wx), iz = (int)floorf(wz), tx = ix >> 4, tz = iz >> 4; const int64_t k = ((int64_t)tx << 32) ^ (int64_t)(uint32_t)tz;
    auto it = gBioRaw.find(k);
    if (it == gBioRaw.end()) { const std::vector<int> r = gGL.get(gGL.vor, tx * 16, tz * 16, 16, 16); std::array<uint8_t, 256> a; for (int i = 0; i < 256; i++) a[i] = (uint8_t)r[i]; it = gBioRaw.emplace(k, a).first; }
    return it->second[(ix & 15) + (iz & 15) * 16];
}
// temperatura / chuva do Biome 1.12 (BiomeGenBase); cor = colormap[t][chuva*t]
static void vbClimate(int id, float* t, float* r) {
    int v = id; if (v >= 128 && v != 129 && v != 130 && v != 131 && v != 132 && v != 133 && v != 134 && v != 140 && v != 149 && v != 151 && v != 155 && v != 156 && v != 157 && v != 158 && v != 160 && v != 161 && v != 162 && v != 163 && v != 164 && v != 165 && v != 166 && v != 167) v = 1;
    switch (v) {
    case 2: case 17: case 130: case 37: case 38: case 39: case 165: case 166: case 167: case 8: *t = 2.f; *r = 0.f; break;
    case 35: *t = 1.2f; *r = 0.f; break; case 163: *t = 1.1f; *r = 0.f; break; case 36: case 164: *t = 1.f; *r = 0.f; break;
    case 1: case 16: case 129: *t = .8f; *r = .4f; break;
    case 4: case 18: case 29: case 132: case 157: *t = .7f; *r = .8f; break;
    case 27: case 28: case 155: case 156: *t = .6f; *r = .6f; break;
    case 5: case 19: case 133: case 160: case 161: *t = .25f; *r = .8f; break;
    case 32: case 33: *t = .3f; *r = .8f; break;
    case 30: case 31: case 158: *t = -.5f; *r = .4f; break;
    case 6: case 134: *t = .8f; *r = .9f; break;
    case 21: case 22: case 149: *t = .95f; *r = .9f; break; case 23: case 151: *t = .95f; *r = .8f; break;
    case 3: case 20: case 34: case 131: case 162: case 25: *t = .2f; *r = .3f; break; case 26: *t = .05f; *r = .3f; break;
    case 10: case 11: case 12: case 13: case 140: *t = 0.f; *r = .5f; break;
    case 14: case 15: *t = .9f; *r = 1.f; break;
    default: *t = .5f; *r = .5f; }
}
static bool gPanoTint = false;   // panorama do titulo: cor neutra de planicie (grama/folha/agua) independente do bioma da seed
// patchBio.2: mistura de bioma suave. Cor de grama/folha por bloco -> desfoque gaussiano separavel (raio 8, sigma 4) calculado UMA vez por chunk (grade 17x17 de cantos, em cache); chao e plantas leem a mesma grade (bilinear), entao nao ha mais borda dura nem diferenca entre chao e planta.
struct BTGrid { float g[17 * 17][3], l[17 * 17][3]; };
static std::unordered_map<int64_t, BTGrid> gBTCache; static int64_t gBTSeed = INT64_MIN;
static void biomeSampleCol(int ix, int iz, float* a, float* c) {   // cor de grama (a) e folha (c) do bioma exato do bloco
    const int id = biomeRawAt((float)ix, (float)iz); float t, r; vbClimate(id, &t, &r);
    const float tt = clampf2(t, 0.f, 1.f), th = clampf2(r * tt, 0.f, 1.f);
    cmSample(cmGrass, tt, th, a); cmSample(cmFoliage, tt, th, c);
    const int bv = id >= 128 ? id - 128 : id;
    if (bv == 37 || bv == 38 || bv == 39) { a[0] = .565f; a[1] = .506f; a[2] = .302f; c[0] = .62f; c[1] = .506f; c[2] = .302f; }   // mesa: grama 90814D, folha 9E814D
    else if (bv == 6) { a[0] = .42f; a[1] = .47f; a[2] = .25f; c[0] = .42f; c[1] = .44f; c[2] = .22f; }   // pantano: tons escuros/oliva
    if (gDim == 3) bloodGrassCol(ix, iz, a, c);   // patchBlood.1: Dimensao Rubra: grama e folhas cor de sangue
}
static const BTGrid& biomeGrid(int tx, int tz) {
    if (gBTSeed != (int64_t)WORLD_SEED || gBTCache.size() > 6000) { gBTSeed = (int64_t)WORLD_SEED; gBTCache.clear(); }
    const int64_t k = ((int64_t)tx << 32) ^ (int64_t)(uint32_t)tz ^ (gDim == 3 ? (int64_t)1 << 61 : 0); auto it = gBTCache.find(k);   // patchBlood.1: Rubra tem cores proprias (chave separada) if (it != gBTCache.end()) return it->second;
    enum { R = 2, N = 16 + 2 * R };   // patchBio.3: Java 1.12 = media 3x3 de blocos (BiomeColorHelper); no vertice = media dos 4 blocos vizinhos => nucleo 1/6,1/3,1/3,1/6 (antes gaussiana raio 8)
    static float sg[N * N][3], sl[N * N][3], hg[N * 17][3], hl[N * 17][3]; static float W[2 * R]; static bool wi = false;
    if (!wi) { wi = true; float sum = 0; static const float K4[4] = {1.f / 6, 1.f / 3, 1.f / 3, 1.f / 6}; for (int i = 0; i < 2 * R; i++) { W[i] = K4[i]; sum += W[i]; } (void)sum; }
    const int x0 = tx * 16 - R, z0 = tz * 16 - R;
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) biomeSampleCol(x0 + i, z0 + j, sg[j * N + i], sl[j * N + i]);
    for (int j = 0; j < N; j++) for (int gx = 0; gx <= 16; gx++) { float ag[3] = {0, 0, 0}, al[3] = {0, 0, 0}; for (int d = 0; d < 2 * R; d++) for (int q = 0; q < 3; q++) { ag[q] += sg[j * N + gx + d][q] * W[d]; al[q] += sl[j * N + gx + d][q] * W[d]; } for (int q = 0; q < 3; q++) { hg[j * 17 + gx][q] = ag[q]; hl[j * 17 + gx][q] = al[q]; } }
    static const float RG[3] = {.57f, .74f, .35f}, RL[3] = {.47f, .65f, .19f};   // patchWx.3: planicie como referencia; o desvio de cada bioma/clima e ampliado 1.5x (mantido)
    BTGrid G;
    for (int gz = 0; gz <= 16; gz++) for (int gx = 0; gx <= 16; gx++) { float ag[3] = {0, 0, 0}, al[3] = {0, 0, 0}; for (int d = 0; d < 2 * R; d++) for (int q = 0; q < 3; q++) { ag[q] += hg[(gz + d) * 17 + gx][q] * W[d]; al[q] += hl[(gz + d) * 17 + gx][q] * W[d]; }
        for (int q = 0; q < 3; q++) { G.g[gz * 17 + gx][q] = clampf2(ag[q], 0.f, 1.f); G.l[gz * 17 + gx][q] = clampf2(al[q], 0.f, 1.f);   /* patchBio.3: cor exata do Java, sem ampliar o desvio */ } }
    return gBTCache.emplace(k, G).first->second;
}
static void biomeTints(float wx, float wz, float* g, float* l) {   // cor no ponto (wx,wz): bilinear na grade do chunk (cantos inteiros = valor exato)
    if (gPanoTint) { static const float PG[3] = {.57f, .74f, .35f}, PL[3] = {.47f, .65f, .19f}; for (int k = 0; k < 3; k++) { g[k] = PG[k]; l[k] = PL[k]; } return; }
    const int tx = (int)floorf(wx / 16.f), tz = (int)floorf(wz / 16.f); const float lx = wx - tx * 16.f, lz = wz - tz * 16.f;
    const int i0 = std::max(0, std::min(15, (int)lx)), j0 = std::max(0, std::min(15, (int)lz)); const float fx = std::max(0.f, std::min(1.f, lx - i0)), fz = std::max(0.f, std::min(1.f, lz - j0));
    const BTGrid& G = biomeGrid(tx, tz);
    for (int q = 0; q < 3; q++) {
        g[q] = (G.g[j0 * 17 + i0][q] * (1 - fx) + G.g[j0 * 17 + i0 + 1][q] * fx) * (1 - fz) + (G.g[(j0 + 1) * 17 + i0][q] * (1 - fx) + G.g[(j0 + 1) * 17 + i0 + 1][q] * fx) * fz;
        l[q] = (G.l[j0 * 17 + i0][q] * (1 - fx) + G.l[j0 * 17 + i0 + 1][q] * fx) * (1 - fz) + (G.l[(j0 + 1) * 17 + i0][q] * (1 - fx) + G.l[(j0 + 1) * 17 + i0 + 1][q] * fx) * fz;
    }
}
static void biomeTintsChunk(int cx, int cz, float (*tg)[3], float (*tl)[3]) {   // grade 17x17 de cantos do chunk (mesmo indice gz*17+gx do build)
    if (gPanoTint) { for (int i = 0; i < 17 * 17; i++) biomeTints(0.f, 0.f, tg[i], tl[i]); return; }
    const BTGrid& G = biomeGrid(cx, cz); for (int i = 0; i < 17 * 17; i++) for (int q = 0; q < 3; q++) { tg[i][q] = G.g[i][q]; tl[i][q] = G.l[i][q]; }
}

// patch5.68: multiplicador de cor da agua por bioma (como o waterColorMultiplier do 1.12: pantano esverdeado, frio azul mais escuro); media 3x3 como a grama
static void biomeWaterTint(float wx, float wz, float* w) {
    if (gPanoTint) { w[0] = w[1] = w[2] = 1.f; return; }
    static const float WT[NBIOME][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {.9f, 1, 1}, {.88f, 1, .68f}, {.8f, .92f, 1}, {.68f, .76f, 1}, {1, 1, 1}};
    float t[3] = {0, 0, 0}; const int ix = (int)floorf(wx), iz = (int)floorf(wz);
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) { const int b = biomeAt((float)(ix + dx * 3), (float)(iz + dz * 3)); float ct, ch; climateAt((float)(ix + dx * 3), (float)(iz + dz * 3), &ct, &ch); const float vr = (ch - .5f) * .14f, vt = (ct - .5f) * .08f; t[0] += WT[b][0] * (1.f - vt * .6f); t[1] += WT[b][1] * (1.f + vr * .5f - vt * .2f); t[2] += WT[b][2] * (1.f - vr * .8f + vt * .2f); }   // patchWx.2: agua levemente mais turva/verde em clima umido e quente
    for (int k = 0; k < 3; k++) w[k] = t[k] / 9.f;
}

static int pkFind(const char* f);
struct CellB { int id; float base, var; };
// gera o terreno base do chunk (pedra, água, bedrock, superfície). B = bloco[(y*CW+z)*CW+x]; bioOut[z*16+x]
// patchGenSlice.1: terreno em 2 metades (A = ruido/densidade, B = preenchimento + superficie) para o gerador em etapas; terrain112() continua igual (A+B)
struct Terr112State { double hm[5 * 33 * 5]; std::vector<int> vox; };
static void terrain112A(int cx, int cz, Terr112State& S) {
    if (!g112.ready || g112.seed != (int64_t)WORLD_SEED) g112.init((int64_t)WORLD_SEED);   // refaz o ruído quando a seed muda (antes ficava com a 1ª seed)
    gGLEnsure(); CellB cell[10][10];   // [z][x] em células de 4 blocos, com margem de 2 (camada genBiomes, 1:4)
    { const std::vector<int> cb = gGL.get(gGL.mix, cx * 4 - 2, cz * 4 - 2, 10, 10); for (int j = 0; j < 10; j++) for (int i = 0; i < 10; i++) { CellB& q = cell[j][i]; q.id = cb[i + j * 10]; vbBV(q.id, q.base, q.var); } }
    S.vox = gGL.get(gGL.vor, cx * 16, cz * 16, 16, 16);   // bioma por coluna (VoronoiZoom)
    static float W5[25]; static bool w5 = false;
    if (!w5) { w5 = true; for (int i = -2; i <= 2; i++) for (int j = -2; j <= 2; j++) W5[i + 2 + (j + 2) * 5] = 10.0f / sqrtf((float)(i * i + j * j) + 0.2f); }
    double depthR[25], mainR[5 * 33 * 5], minR[5 * 33 * 5], maxR[5 * 33 * 5]; double* hm = S.hm;
    const int X = cx * 4, Z = cz * 4;
    g112.depthN.gen(depthR, X, 10, Z, 5, 1, 5, 200.0, 1.0, 200.0);
    g112.mainN.gen(mainR, X, 0, Z, 5, 33, 5, 684.412 / 80.0, 684.412 / 160.0, 684.412 / 80.0);
    g112.minL.gen(minR, X, 0, Z, 5, 33, 5, 684.412, 684.412, 684.412);
    g112.maxL.gen(maxR, X, 0, Z, 5, 33, 5, 684.412, 684.412, 684.412);
    int m = 0, di = 0;
    for (int j = 0; j < 5; j++) for (int k = 0; k < 5; k++) {
        float f = 0, f1 = 0, f2 = 0; const CellB& cb = cell[k + 2][j + 2]; const float f3 = cb.base;
        for (int l = -2; l <= 2; l++) for (int i2 = -2; i2 <= 2; i2++) {
            const CellB& b1 = cell[k + i2 + 2][j + l + 2];
            float f5 = 0.f + b1.base * 1.f, f6 = 0.f + b1.var * 1.f;
            float f7 = W5[l + 2 + (i2 + 2) * 5] / (f5 + 2.0f);
            if (b1.base > f3) f7 /= 2.0f;
            f += f6 * f7; f1 += f5 * f7; f2 += f7;
        }
        f /= f2; f1 /= f2; f = f * 0.9f + 0.1f; f1 = (f1 * 4.0f - 1.0f) / 8.0f;
        double d7 = depthR[di++] / 8000.0;
        if (d7 < 0.0) d7 = -d7 * 0.3;
        d7 = d7 * 3.0 - 2.0;
        if (d7 < 0.0) { d7 /= 2.0; if (d7 < -1.0) d7 = -1.0; d7 /= 1.4; d7 /= 2.0; } else { if (d7 > 1.0) d7 = 1.0; d7 /= 8.0; }
        double d8 = (double)f1, d9 = (double)f;
        d8 += d7 * 0.2; d8 = d8 * 8.5 / 8.0;
        const double d0 = 8.5 + d8 * 4.0;
        for (int l1 = 0; l1 < 33; l1++) {
            double d1 = ((double)l1 - d0) * 12.0 * 128.0 / 256.0 / d9;
            if (d1 < 0.0) d1 *= 4.0;
            const double d2 = minR[m] / 512.0, d3 = maxR[m] / 512.0, d4 = (mainR[m] / 10.0 + 1.0) / 2.0;
            const double cl = d4 < 0.0 ? d2 : (d4 > 1.0 ? d3 : d2 + (d3 - d2) * d4);   // clampedLerp
            double d5 = cl - d1;
            if (l1 > 29) { double d6 = (double)((float)(l1 - 29) / 3.0f); d5 = d5 * (1.0 - d6) + -10.0 * d6; }
            hm[m++] = d5;
        }
    }
}
static void terrain112B(int cx, int cz, BT* B, uint8_t* bioOut, int* topOut, const Terr112State& S) {
    const double* hm = S.hm; const std::vector<int>& vox = S.vox;
    // interpolação trilinear: células de 4x8x4 -> pedra onde densidade > 0, água abaixo do nível do mar
    memset(B, 0, (size_t)CW * CH * CW * sizeof(BT));
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) for (int k = 0; k < 32; k++) {
        double d1 = hm[((i + 0) * 5 + j + 0) * 33 + k], d2 = hm[((i + 0) * 5 + j + 1) * 33 + k], d3 = hm[((i + 1) * 5 + j + 0) * 33 + k], d4 = hm[((i + 1) * 5 + j + 1) * 33 + k];
        const double d5 = (hm[((i + 0) * 5 + j + 0) * 33 + k + 1] - d1) * 0.125, d6 = (hm[((i + 0) * 5 + j + 1) * 33 + k + 1] - d2) * 0.125;
        const double d7 = (hm[((i + 1) * 5 + j + 0) * 33 + k + 1] - d3) * 0.125, d8 = (hm[((i + 1) * 5 + j + 1) * 33 + k + 1] - d4) * 0.125;
        for (int l = 0; l < 8; l++) {
            double d10 = d1, d11 = d2; const double d12 = (d3 - d1) * 0.25, d13 = (d4 - d2) * 0.25;
            for (int i1 = 0; i1 < 4; i1++) {
                const double d16 = (d11 - d10) * 0.25; double d15 = d10 - d16;
                for (int j1 = 0; j1 < 4; j1++) {
                    d15 += d16; const int y = k * 8 + l;
                    if (d15 > 0.0) B[(y * CW + (j * 4 + j1)) * CW + (i * 4 + i1)] = STONE;
                    else if (y < SEA) B[(y * CW + (j * 4 + j1)) * CW + (i * 4 + i1)] = WATER;
                }
                d10 += d12; d11 += d13;
            }
            d1 += d5; d2 += d6; d3 += d7; d4 += d8;
        }
    }
    // superfície (Biome.generateBiomeTerrain do 1.12.2): topo/enchimento por bioma do MC, bedrock rugoso, camadas de areia/arenito
    double sd[256]; g112.surf.region(sd, (double)(cx * 16), (double)(cz * 16), 16, 16, 0.0625, 0.0625);
    JRng rnd((int64_t)cx * 341873128712LL + (int64_t)cz * 132897987541LL);
    const BT podzol = (BT)pkFind("block/podzol"), coarse = (BT)pkFind("block/coarse_dirt"), mycel = (BT)pkFind("block/mycelium"), redSand = (BT)pkFind("block/red_sand"), iceB = (BT)pkFind("block/ice");
    BT bands[64]; const BT terra = (BT)pkFind("block/terracotta"); const bool mesaOk = terra != 0 && redSand != 0;
    if (mesaOk) {   // BiomeMesa.generateBands (simplificado: mesma ordem de sorteios, 6 cores)
        static const char* CN[6] = {"orange", "yellow", "brown", "red", "white", "light_gray"}; BT col[6];
        for (int q = 0; q < 6; q++) { col[q] = (BT)pkFind((std::string("block/") + CN[q] + "_terracotta").c_str()); if (!col[q]) col[q] = terra; }
        JRng br((int64_t)WORLD_SEED); for (int q = 0; q < 64; q++) bands[q] = terra;
        for (int q = 0; q < 64;) { q += br.nextInt(5) + 1; if (q < 64) bands[q++] = col[0]; }
        const int th[3] = {3, 3, 2}; const int cc[3] = {1, 2, 3};
        for (int g = 0; g < 3; g++) { const int n = br.nextInt(4) + 2; for (int q = 0; q < n; q++) { const int w = br.nextInt(th[g]) + 1, s = br.nextInt(64); for (int u = 0; s + u < 64 && u < w; u++) bands[s + u] = col[cc[g]]; } }
        const int n = br.nextInt(3) + 3; int s = 0;
        for (int q = 0; q < n; q++) { s += br.nextInt(16) + 4; if (s < 64) { bands[s] = col[4]; if (s > 1 && br.nextInt(2)) bands[s - 1] = col[5]; if (s < 63 && br.nextInt(2)) bands[s + 1] = col[5]; } }
    }
    for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) {
        const int px = j, pz = i;
        const int vid = vox[px + pz * 16], gb = vbGame(vid), vb = vid >= 128 ? vid - 128 : vid; const bool mut = vid >= 128; bioOut[pz * 16 + px] = (uint8_t)gb;
        const double nv = sd[j + i * 16];
        BT topB = GRASS, fillB = DIRT; bool mesa = false;
        switch (vb) {
        case 2: case 17: case 16: case 26: topB = fillB = SAND; break;
        case 25: topB = fillB = STONE; break;
        case 14: case 15: if (mycel) topB = mycel; break;
        case 32: case 33: if (nv > 1.75) { if (coarse) topB = coarse; } else if (nv > -0.95 && podzol) topB = podzol; break;
        case 35: case 36: if (mut) { if (nv > 1.75) topB = fillB = STONE; else if (nv > -0.5 && coarse) topB = coarse; } break;
        case 3: case 20: case 34:
            if ((nv < -1.0 || nv > 2.0) && mut) topB = fillB = GRAVEL; else if (nv > 1.0 && vb != 34) topB = fillB = STONE; break;
        case 37: case 38: case 39: if (mesaOk) { mesa = true; topB = redSand; fillB = terra; } else topB = fillB = SAND; break;
        default: if (gb == B_TUNDRA) topB = SNOWBLK; break;
        }
        BT top = topB, fill = fillB; int run = -1;
        int k = (int)(nv / 3.0 + 3.0 + rnd.nextDouble() * 0.25);
        for (int y = 255; y >= 0; y--) {
            BT& b = B[(y * CW + pz) * CW + px];
            if (y <= rnd.nextInt(5)) { b = BEDROCK; continue; }
            if (b == AIR || isWater(b)) { if (b == AIR) run = -1; continue; }
            if (b != STONE) continue;
            const BT mfill = mesa ? bands[(y + (int)floor(nv * 2.0 + .5) + 64) & 63] : fill;
            if (run == -1) {
                if (k <= 0) { top = AIR; fill = STONE; }
                else if (y >= SEA - 4 && y <= SEA + 1) { top = topB; fill = fillB; }
                if (y < SEA && top == AIR) top = (vb < 40 && V_TEMP[vb] < .15f && iceB) ? iceB : (BT)WATER;
                run = k;
                BT st = (gb == B_MOUNTAINS && y >= 95 && top == GRASS) ? (BT)SNOWBLK : top;
                if (mesa && y >= SEA - 1 && y > SEA + 3 + k) st = mfill;
                if (y >= SEA - 1) b = st; else if (y < SEA - 7 - k) { top = AIR; fill = STONE; b = GRAVEL; } else b = mesa ? mfill : fill;
            } else if (run > 0) {
                run--; b = mesa ? mfill : fill;
                if (run == 0 && fill == SAND && k > 1) { run = rnd.nextInt(4) + std::max(0, y - 63); fill = SANDSTONE; }
            }
        }
    }
    for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {
        int t = 0; for (int y = CH - 1; y > 0; y--) { const uint8_t b = B[(y * CW + z) * CW + x]; if (b != AIR && !isWater(b)) { t = y; break; } }
        topOut[z * 16 + x] = t;
    }
}
static void terrain112(int cx, int cz, BT* B, uint8_t* bioOut, int* topOut) { Terr112State S; terrain112A(cx, cz, S); terrain112B(cx, cz, B, bioOut, topOut, S); }
