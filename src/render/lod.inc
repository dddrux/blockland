// patchLOD.1: LOD de terreno distante (incluido por world.inc antes de build()).
// Chunk longe vira um campo de alturas por celula (passo 2 = 2x2 colunas, passo 4 = 4x4), com cor media do tile (atlas) x tingimento de bioma,
// topos fundidos por greedy meshing (retangulos de mesma altura/cor/luz) e paredes fundidas ao longo da borda. Sem subsolo, sem UV de textura:
// todos os vertices apontam para 1 tile branco do atlas (UV constante = sem gradiente = sempre o mip 0 = cor pura). Agua continua na malha normal.
// Niveis (gLodR2/gLodR4, em chunks, definidos no main_loop a partir da nevoa): 0 normal | 2 | 4. Histerese de 1 chunk. BLOCKLAND_NOLOD=1 desliga.
static bool gLodOn = false;   /* patchLOD.5 */ static int gLodR2 = 1 << 20, gLodR4 = 1 << 20;
static uint64_t gLodVerts = 0, gLodChunks = 0;   // estatisticas (ultimas malhas LOD feitas)

// patchLOD.4: luz do chunk LOD sem propagacao: acima do topo opaco da coluna = ceu 15; dentro/abaixo = 0 (escuro, como cavernas; o spawn/sol dos mobs continua coerente)
static void lodLight(Chunk* c, int topY) {
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
        int hs = -1; for (int y = std::min(topY, CH - 1); y >= 0; y--) { const int b = c->b[idx(x, y, z)]; if (b && lightOpac(b) >= 15) { hs = y; break; } }
        for (int y = 0; y < CH; y++) c->lc[idx(x, y, z)] = y > hs ? (uint8_t)0xF0 : (uint8_t)0;
    }
}
static inline int lodWant(int d2, int cur) {
    if (!gLodOn || gDim || gLodR2 >= 4096) return 0;
    const int a2 = cur >= 2 ? gLodR2 - 1 : gLodR2, a4 = cur >= 4 ? gLodR4 - 1 : gLodR4;
    if (gLodR4 < 4096 && d2 >= a4 * a4) return 4;
    return d2 >= a2 * a2 ? 2 : 0;
}
static inline bool lodSolidB(int b) { return b && !isWater(b) && !isXB(b) && (isLeafB(b) || isLava(b) || lightOpac(b) >= 15); }
static const float* lodTileAvg(int tile) {
    const int nt = ATL * ATLR; if (gLodAvg.empty()) gLodAvg.assign((size_t)nt * 3, -1.f);
    if (tile < 0 || tile >= nt) tile = 0;
    float* a = &gLodAvg[(size_t)tile * 3];
    if (a[0] < 0.f) {
        double r = 0, g = 0, b = 0, w = 0;
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const uint8_t* p = &atlas[((size_t)((tile / ATL) * 16 + y) * ATW + (tile % ATL) * 16 + x) * 4]; const double al = p[3]; r += p[0] * al; g += p[1] * al; b += p[2] * al; w += al; }
        if (w < 1) a[0] = a[1] = a[2] = .5f; else { a[0] = (float)(r / w / 255.0); a[1] = (float)(g / w / 255.0); a[2] = (float)(b / w / 255.0); }
    }
    return a;
}
// topo solido da coluna local (lx,lz) de c: altura (-1 = vazio) e bloco
static int lodColTop(Chunk* c, int topY, int lx, int lz, int* blk = nullptr) {
    for (int y = std::min(topY, CH - 1); y >= 0; y--) { const int b = c->b[idx(lx, y, lz)]; if (lodSolidB(b) && !isLeafB(b) && !isLogB(b)) { if (blk) *blk = b; return y; } }   // patchVis.4: copas/troncos viravam colunas palidas altissimas na distancia (passo 2/4 funde a arvore em pilar); LOD so do terreno
    return -1;
}
// patchTerra.2: base do corpo solido que sustenta o topo h da coluna. 0 = chao normal (solido ate o fundo, ou vao < 6 blocos); >0 = terreno flutuante (ilha): y da face de baixo
static int lodColBase(Chunk* c, int h, int lx, int lz) {
    int y = h; while (y > 0 && lodSolidB(c->b[idx(lx, y - 1, lz)])) y--;
    if (y <= 0) return 0;
    for (int k = 1; k <= 6 && y - k >= 0; k++) if (lodSolidB(c->b[idx(lx, y - k, lz)])) return 0;
    return y;
}
// cor (tile medio x tingimento de bioma) do bloco b na coluna (x,z) do chunk
static void lodBlockColor(int b, int x, int z, bool side, const float (*tg)[3], const float (*tl)[3], float* o) {
    const float* a = lodTileAvg(blkTile(b, (side && b != GRASS) ? 1 : 0)); o[0] = a[0]; o[1] = a[1]; o[2] = a[2];
    const int lk = leafKind(b);
    if (b == GRASS || lk == 1) {
        const float (*T)[3] = b == GRASS ? tg : tl; const int g0 = z * (CW + 1) + x;
        for (int q = 0; q < 3; q++) o[q] *= (T[g0][q] + T[g0 + 1][q] + T[g0 + CW + 1][q] + T[g0 + CW + 2][q]) * .25f;
    } else if (lk >= 2) { static const float FIX[2][3] = {{.380f, .600f, .380f}, {.502f, .655f, .333f}}; for (int q = 0; q < 3; q++) o[q] *= FIX[lk == 3][q]; }
}

static void lodMesh(Chunk* c, int cx, int cz, int topY, const float (*tg)[3], const float (*tl)[3], int step) {
    const int N = CW / step, wx0 = cx * CW, wz0 = cz * CW;
    static const float SHT = 1.f, SHX = .8f, SHZ = .65f;
    struct Cell { int h; int blk; int base; int bblk; uint64_t key; uint8_t q[3]; uint8_t lc; bool emit; };
    Cell cell[16][16];   // [j = z][i = x]
    auto qc = [](float v) { const int i = (int)lroundf(std::min(1.f, std::max(0.f, v)) * 15.f); return (uint8_t)i; };
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        Cell& ce = cell[j][i]; ce.h = -1; ce.blk = 0; ce.base = 0; ce.bblk = 0; ce.key = 0; ce.emit = false; ce.lc = 0xF0;
        int n = 0, sh = 0, sk = 0, bl = 0; float col[3] = {0, 0, 0}; bool em = false; int bestH = -1, mb = 1 << 20, bb = 0;
        for (int dz = 0; dz < step; dz++) for (int dx = 0; dx < step; dx++) {
            const int x = i * step + dx, z = j * step + dz; int b = 0; const int h = lodColTop(c, topY, x, z, &b); if (h < 0) continue;
            float cc[3]; lodBlockColor(b, x, z, false, tg, tl, cc); const uint8_t l = h + 1 < CH ? c->lc[idx(x, h + 1, z)] : (uint8_t)0xF0;
            { const int bs = lodColBase(c, h, x, z); if (bs < mb) { mb = bs; bb = bs > 0 ? (int)c->b[idx(x, bs, z)] : 0; } } n++; sh += h; sk += l >> 4; bl += l & 15; for (int q = 0; q < 3; q++) col[q] += cc[q]; if (lightEmit(b) > 0) em = true; if (h > bestH) { bestH = h; ce.blk = b; }
        }
        if (!n) continue;
        ce.base = mb >= (1 << 20) ? 0 : mb; ce.bblk = ce.base > 0 ? bb : 0; ce.h = (sh + n / 2) / n; ce.emit = em; ce.lc = (uint8_t)((((sk + n / 2) / n) << 4) | ((bl + n / 2) / n));
        for (int k = 0; k < 3; k++) ce.q[k] = qc(col[k] / n);
        ce.key = (uint32_t)(ce.h + 1) | ((uint32_t)ce.q[0] << 9) | ((uint32_t)ce.q[1] << 13) | ((uint32_t)ce.q[2] << 17) | ((uint32_t)ce.lc << 21) | (em ? 1u << 29 : 0u); ce.key |= ((uint64_t)ce.base << 32) | ((uint64_t)(ce.bblk & 0xFFFFF) << 40);
    }
    std::vector<V> bk[16];
    const float wu = ((lodWhiteTile() % ATL) + .5f) / (float)ATL, wv = ((lodWhiteTile() / ATL) + .5f) / (float)ATLR;
    auto fin = [&](const float* rgb, int lcb, bool emit, float sh, float* o) {   // cor final + alfa (luz empacotada, modo shader)
        const int sk = lcb >> 4, bl = lcb & 15;
        if (emit) { o[0] = rgb[0]; o[1] = rgb[1]; o[2] = rgb[2]; o[3] = gShaderOK ? packLight(0.f, 15.75f) : 1.f; return; }
        if (gShaderOK) { o[0] = rgb[0] * sh; o[1] = rgb[1] * sh; o[2] = rgb[2] * sh; o[3] = packLight((float)sk, (float)bl); }
        else { const float L = std::max(ltab((float)sk), bl > 0 ? ltabB((float)bl) : 0.f) * sh; o[0] = rgb[0] * L; o[1] = rgb[1] * L; o[2] = rgb[2] * L; o[3] = 1.f; }
    };
    auto pushQ = [&](int sc, const float p[4][3], const float* o) { for (int k = 0; k < 4; k++) bk[sc].push_back({p[k][0], p[k][1], p[k][2], wu, wv, o[0], o[1], o[2], o[3]}); };
    const float EPS = .002f;
    auto top = [&](float x0, float z0, float x1, float z1, int h, const float* rgb, int lcb, bool em) {   // plano y = h+1; ordem = a do mesher (f=2 invertida: culling de costas)
        float o[4]; fin(rgb, lcb, em, SHT, o); const float y = (float)(h + 1); x0 -= EPS; z0 -= EPS; x1 += EPS; z1 += EPS;
        const float p[4][3] = {{x0, y, z0}, {x0, y, z1}, {x1, y, z1}, {x1, y, z0}}; pushQ(std::min(15, h >> 4), p, o);
    };
    auto bottom = [&](float x0, float z0, float x1, float z1, int by, int blkB, int gx, int gz, int lcb) {   // patchTerra.2: face de baixo do terreno flutuante (rocha, mais escura)
        float rgb[3] = {.45f, .45f, .45f}; if (blkB > 0) lodBlockColor(blkB, gx, gz, true, tg, tl, rgb);
        float o[4]; fin(rgb, lcb, false, .5f, o); const float y = (float)by; x0 -= EPS; z0 -= EPS; x1 += EPS; z1 += EPS;
        const float p[4][3] = {{x0, y, z0}, {x1, y, z0}, {x1, y, z1}, {x0, y, z1}}; pushQ(std::min(15, by >> 4), p, o);
    };
    auto wall = [&](int f, float plane, float a0, float a1, int ya, int yb, const float* rgb, int lcb, bool em) {   // f: 0 +X, 1 -X, 4 +Z, 5 -Z; linhas de bloco [ya,yb] (inclusive)
        float o[4]; fin(rgb, lcb, em, f < 2 ? SHX : SHZ, o); a0 -= EPS; a1 += EPS;
        for (int sc = ya >> 4; sc <= std::min(15, yb >> 4); sc++) {
            const float y0 = (float)std::max(ya, sc * 16), y1 = (float)(std::min(yb, sc * 16 + 15) + 1);
            float p[4][3];
            if (f == 0) { const float t[4][3] = {{plane, y0, a1}, {plane, y0, a0}, {plane, y1, a0}, {plane, y1, a1}}; memcpy(p, t, sizeof p); }
            else if (f == 1) { const float t[4][3] = {{plane, y0, a0}, {plane, y0, a1}, {plane, y1, a1}, {plane, y1, a0}}; memcpy(p, t, sizeof p); }
            else if (f == 4) { const float t[4][3] = {{a0, y0, plane}, {a1, y0, plane}, {a1, y1, plane}, {a0, y1, plane}}; memcpy(p, t, sizeof p); }
            else { const float t[4][3] = {{a1, y0, plane}, {a0, y0, plane}, {a0, y1, plane}, {a1, y1, plane}}; memcpy(p, t, sizeof p); }
            pushQ(sc, p, o);
        }
    };
    // ---- topos: greedy ----
    bool used[16][16]; memset(used, 0, sizeof used);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        const Cell& ce = cell[j][i]; if (ce.h < 0 || used[j][i]) continue;
        int w = 1; while (i + w < N && !used[j][i + w] && cell[j][i + w].h >= 0 && cell[j][i + w].key == ce.key) w++;
        int hh = 1;
        for (; j + hh < N; hh++) { bool ok = true; for (int k = 0; k < w; k++) { const Cell& o = cell[j + hh][i + k]; if (used[j + hh][i + k] || o.h < 0 || o.key != ce.key) { ok = false; break; } } if (!ok) break; }
        for (int b = 0; b < hh; b++) for (int a = 0; a < w; a++) used[j + b][i + a] = true;
        const float rgb[3] = {ce.q[0] / 15.f, ce.q[1] / 15.f, ce.q[2] / 15.f};
        top((float)(wx0 + i * step), (float)(wz0 + j * step), (float)(wx0 + (i + w) * step), (float)(wz0 + (j + hh) * step), ce.h, rgb, ce.lc, ce.emit);
        if (ce.base > 0) bottom((float)(wx0 + i * step), (float)(wz0 + j * step), (float)(wx0 + (i + w) * step), (float)(wz0 + (j + hh) * step), ce.base, ce.bblk, i * step, j * step, ce.lc);
    }
    // ---- paredes: 4 direcoes, fundidas ao longo da borda ----
    struct Seg { int ya, yb; uint32_t ck; float rgb[3]; uint8_t lc; bool em; };
    struct Run { Seg s; int p0, p1; };
    static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1}, FF[4] = {0, 1, 4, 5};
    Chunk* nbC[4]; int nbTop[4];
    for (int d = 0; d < 4; d++) { nbC[d] = getC(cx + DX[d], cz + DZ[d]); nbTop[d] = nbC[d] ? chunkTop(nbC[d]) : 0; }
    for (int d = 0; d < 4; d++) for (int L = 0; L < N; L++) {
        std::vector<Run> open, nxt; std::vector<Seg> segs;
        auto flush = [&](const Run& r) {
            const float plane = d < 2 ? (float)(wx0 + (L + (DX[d] > 0 ? 1 : 0)) * step) : (float)(wz0 + (L + (DZ[d] > 0 ? 1 : 0)) * step);
            const float a0 = d < 2 ? (float)(wz0 + r.p0 * step) : (float)(wx0 + r.p0 * step), a1 = d < 2 ? (float)(wz0 + r.p1 * step) : (float)(wx0 + r.p1 * step);
            wall(FF[d], plane, a0, a1, r.s.ya, r.s.yb, r.s.rgb, r.s.lc, r.s.em);
        };
        for (int P = 0; P < N; P++) {
            const int i = d < 2 ? L : P, j = d < 2 ? P : L; const Cell& ce = cell[j][i]; segs.clear();
            if (ce.h >= 0) {
                const int ni = i + DX[d], nj = j + DZ[d]; int nh = -2, nb = 0; const bool edge = !(ni >= 0 && nj >= 0 && ni < N && nj < N);
                if (ni >= 0 && nj >= 0 && ni < N && nj < N) { nh = cell[nj][ni].h; nb = cell[nj][ni].base; }
                else if (nbC[d]) {   // vizinho em outro chunk: media das colunas da borda adjacentes a esta celula
                    int s = 0, n = 0, mbn = 1 << 20;
                    for (int k = 0; k < step; k++) { const int lx = d < 2 ? (DX[d] > 0 ? 0 : CW - 1) : i * step + k, lz = d < 2 ? j * step + k : (DZ[d] > 0 ? 0 : CW - 1); const int h = lodColTop(nbC[d], nbTop[d], lx, lz); if (h >= 0) { s += h; n++; const int bq = lodColBase(nbC[d], h, lx, lz); if (bq < mbn) mbn = bq; } }
                    nh = n ? (s + n / 2) / n : -2; nb = (n && mbn < (1 << 20)) ? mbn : 0;
                }
                if (nh >= -1 && (nh < ce.h || edge || (ce.base > 0 && nb > ce.base))) {   // so a parede voltada para o lado mais baixo; na borda do chunk ha saia de 2 blocos (fecha frestas entre LODs de passos diferentes)
                    const int lo = ce.base > 0 ? ce.base - 1 : (edge ? std::max(-1, nh < ce.h ? nh - 2 : ce.h - 2) : nh);
                    const int rx = d < 2 ? (DX[d] > 0 ? i * step + step - 1 : i * step) : i * step + step / 2, rz = d < 2 ? j * step + step / 2 : (DZ[d] > 0 ? j * step + step - 1 : j * step);
                    int curB = -1, runTop = ce.h; float rgbC[3] = {0, 0, 0}; bool emC = false;
                    auto close = [&](int lowRow) { if (curB < 0) return; Seg s; s.ya = lowRow; s.yb = runTop; memcpy(s.rgb, rgbC, sizeof rgbC); s.em = emC; s.lc = ce.lc;
                        s.ck = ((uint32_t)qc(rgbC[0]) << 4 | qc(rgbC[1])) << 4 | qc(rgbC[2]); for (int q = 0; q < 3; q++) s.rgb[q] = qc(rgbC[q]) / 15.f; segs.push_back(s); };
                    for (int r = ce.h; r > lo; r--) {
                        if (ce.base > 0 && nh >= r && nb <= r) { close(r + 1); curB = -1; continue; }   // patchTerra.2: trecho encostado no solido do vizinho
                        int b = c->b[idx(rx, r, rz)]; if (!lodSolidB(b)) b = ce.blk;
                        if (b != curB) { close(r + 1); curB = b; runTop = r; lodBlockColor(b, rx, rz, true, tg, tl, rgbC); emC = lightEmit(b) > 0; }
                    }
                    close(lo + 1);
                }
            }
            nxt.clear(); std::vector<char> hit(open.size(), 0);
            for (const Seg& s : segs) {
                bool m = false;
                for (size_t k = 0; k < open.size() && !m; k++) if (!hit[k] && open[k].p1 == P && open[k].s.ya == s.ya && open[k].s.yb == s.yb && open[k].s.ck == s.ck && open[k].s.lc == s.lc && open[k].s.em == s.em) { hit[k] = 1; Run r = open[k]; r.p1 = P + 1; nxt.push_back(r); m = true; }
                if (!m) nxt.push_back({s, P, P + 1});
            }
            for (size_t k = 0; k < open.size(); k++) if (!hit[k]) flush(open[k]);
            open.swap(nxt);
        }
        for (const Run& r : open) flush(r);
    }
    // ---- junta por secao vertical de 16 (culling por secao do desenho) ----
    for (int sc = 0; sc < 16; sc++) { c->sOff[sc] = (uint32_t)c->mesh.size(); c->mesh.insert(c->mesh.end(), bk[sc].begin(), bk[sc].end()); }
    c->sOff[16] = (uint32_t)c->mesh.size(); gLodVerts += c->mesh.size(); gLodChunks++;
}

// patchLOD.2: agua distante: 1 quad plano por retangulo fundido (altura do nivel, cor de bioma, luz), UV constante no centro do tile da agua. Sem lados/fundo.
static void lodWater(Chunk* c, int cx, int cz, int topY, int step) {
    const int N = CW / step, wx0 = cx * CW, wz0 = cz * CW;
    struct WC { bool w; uint32_t key; float y; uint8_t lc; uint8_t t[3]; };
    WC cell[16][16];
    auto qc = [](float v) { return (uint8_t)lroundf(std::min(1.f, std::max(0.f, v)) * 15.f); };
    bool any = false;
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        WC& e = cell[j][i]; e.w = false; e.key = 0;
        const int x = i * step + step / 2, z = j * step + step / 2; int wy = -1, b = 0;
        for (int y = std::min(topY, CH - 1); y >= 0; y--) { b = c->b[idx(x, y, z)]; if (b) { if (isWater(b)) wy = y; break; } }
        if (wy < 0) continue;
        const int m = liqMeta(b); const float h = (m == 0 || m >= 8) ? .889f : 1.f - (m + 1) / 9.f;
        float wtn[3] = {1, 1, 1}; if (gDim == 0) biomeWaterTint((float)(wx0 + x) + .5f, (float)(wz0 + z) + .5f, wtn);
        e.w = true; e.y = wy + h; e.lc = c->lc[idx(x, wy, z)]; for (int k = 0; k < 3; k++) e.t[k] = qc(wtn[k]);
        e.key = (uint32_t)(wy * 16 + (int)(h * 15.f)) | ((uint32_t)e.t[0] << 12) | ((uint32_t)e.t[1] << 16) | ((uint32_t)e.t[2] << 20) | ((uint32_t)(e.lc) << 24);
        any = true;
    }
    std::vector<V> bk[16];
    const int wt = TILES[WATER][1]; const float u = ((wt % ATL) + .5f) / (float)ATL, v = ((wt / ATL) + .5f) / (float)ATLR;
    bool used[16][16]; memset(used, 0, sizeof used);
    if (any) for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        const WC& e = cell[j][i]; if (!e.w || used[j][i]) continue;
        int w = 1; while (i + w < N && !used[j][i + w] && cell[j][i + w].w && cell[j][i + w].key == e.key) w++;
        int hh = 1; for (; j + hh < N; hh++) { bool ok = true; for (int k = 0; k < w; k++) if (used[j + hh][i + k] || !cell[j + hh][i + k].w || cell[j + hh][i + k].key != e.key) { ok = false; break; } if (!ok) break; }
        for (int b2 = 0; b2 < hh; b2++) for (int a = 0; a < w; a++) used[j + b2][i + a] = true;
        float r = e.t[0] / 15.f, g = e.t[1] / 15.f, bb = e.t[2] / 15.f, al;
        const int sk = e.lc >> 4, bl = e.lc & 15;
        if (gShaderOK) al = packLight((float)sk, (float)bl); else { const float lw = std::max(ltab((float)std::min(15, sk + 2)), bl ? ltabB((float)bl) : 0.f); r *= lw; g *= lw; bb *= lw; al = .62f; }
        const float x0 = (float)(wx0 + i * step) - .002f, z0 = (float)(wz0 + j * step) - .002f, x1 = (float)(wx0 + (i + w) * step) + .002f, z1 = (float)(wz0 + (j + hh) * step) + .002f;
        const int sc = std::min(15, (int)e.y >> 4);
        bk[sc].push_back({x0, e.y, z0, u, v, r, g, bb, al}); bk[sc].push_back({x1, e.y, z0, u, v, r, g, bb, al});
        bk[sc].push_back({x1, e.y, z1, u, v, r, g, bb, al}); bk[sc].push_back({x0, e.y, z1, u, v, r, g, bb, al});
    }
    c->wmesh.clear();
    for (int sc = 0; sc < 16; sc++) { c->wOff[sc] = (uint32_t)c->wmesh.size(); c->wmesh.insert(c->wmesh.end(), bk[sc].begin(), bk[sc].end()); }
    c->wOff[16] = (uint32_t)c->wmesh.size();
}
