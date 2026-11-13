// ---------- MINA ABANDONADA (patchStruct.3): MapGenMineshaft + StructureMineshaftPieces do 1.12.2 ----------
// Mesmo algoritmo: chance por chunk (0.004 e nextInt(80) < max(|cx|,|cz|)), sala inicial em (cx*16+2, cz*16+2), corredores/cruzamentos/escadas
// recursivos (profundidade 8, raio 80), caixas de colisao, deslocamento vertical markAvailableHeight(10). Escrita por chunk (regenera a lista de pecas).
// Simplificacoes: tipo "normal" (sem mesa/badlands); sem carrinho com bau nem checagem de liquido; spawner de aranha-da-caverna so se o pack tiver o bloco.
struct MsPiece { int t, d, f, b[6]; bool rails, spiders; };   // t: 0 sala, 1 corredor, 2 cruzamento, 3 escada | f: 0 N, 1 S, 2 W, 3 E, -1 nenhuma | b: x0,y0,z0,x1,y1,z1
struct MsGen { std::vector<MsPiece> v; JR2 r; int sx, sz; MsGen() : r(0) {} };
static MsGen* gMs; static Chunk* gMsC; static int gMsCx, gMsCz;
static int msBox(const int* a, const int* b) { return !(a[3] < b[0] || a[0] > b[3] || a[5] < b[2] || a[2] > b[5] || a[4] < b[1] || a[1] > b[4]); }
static bool msFree(const int* bx) { for (const auto& p : gMs->v) if (msBox(p.b, bx)) return false; return true; }
static void msGen(int x, int y, int z, int f, int depth);
static void msBuild(size_t id) {
    MsPiece p = gMs->v[id]; JR2& r = gMs->r; const int* b = p.b; const int d = p.d;
    if (p.t == 0) {
        int j = (b[4] - b[1] + 1) - 3 - 1; if (j <= 0) j = 1; const int w = b[3] - b[0] + 1, l = b[5] - b[2] + 1;
        for (int k = 0; k < w; k += 4) { k += r.nextInt(w); if (k + 3 > w) break; msGen(b[0] + k, b[1] + r.nextInt(j) + 1, b[2] - 1, 0, d); }
        for (int k = 0; k < w; k += 4) { k += r.nextInt(w); if (k + 3 > w) break; msGen(b[0] + k, b[1] + r.nextInt(j) + 1, b[5] + 1, 1, d); }
        for (int k = 0; k < l; k += 4) { k += r.nextInt(l); if (k + 3 > l) break; msGen(b[0] - 1, b[1] + r.nextInt(j) + 1, b[2] + k, 2, d); }
        for (int k = 0; k < l; k += 4) { k += r.nextInt(l); if (k + 3 > l) break; msGen(b[3] + 1, b[1] + r.nextInt(j) + 1, b[2] + k, 3, d); }
    } else if (p.t == 1) {
        const int j = r.nextInt(4), f = p.f, y = b[1] - 1 + r.nextInt(3);
        if (f == 0) { if (j <= 1) msGen(b[0], y, b[2] - 1, f, d); else if (j == 2) msGen(b[0] - 1, y, b[2], 2, d); else msGen(b[3] + 1, y, b[2], 3, d); }
        else if (f == 1) { if (j <= 1) msGen(b[0], y, b[5] + 1, f, d); else if (j == 2) msGen(b[0] - 1, y, b[5] - 3, 2, d); else msGen(b[3] + 1, y, b[5] - 3, 3, d); }
        else if (f == 2) { if (j <= 1) msGen(b[0] - 1, y, b[2], f, d); else if (j == 2) msGen(b[0], y, b[2] - 1, 0, d); else msGen(b[0], y, b[5] + 1, 1, d); }
        else { if (j <= 1) msGen(b[3] + 1, y, b[2], f, d); else if (j == 2) msGen(b[3] - 3, y, b[2] - 1, 0, d); else msGen(b[3] - 3, y, b[5] + 1, 1, d); }
        if (d < 8) {
            if (f == 0 || f == 1) { for (int k = b[2] + 3; k + 3 <= b[5]; k += 5) { const int q = r.nextInt(5); if (q == 0) msGen(b[0] - 1, b[1], k, 2, d + 1); else if (q == 1) msGen(b[3] + 1, b[1], k, 3, d + 1); } }
            else { for (int k = b[0] + 3; k + 3 <= b[3]; k += 5) { const int q = r.nextInt(5); if (q == 0) msGen(k, b[1], b[2] - 1, 0, d + 1); else if (q == 1) msGen(k, b[1], b[5] + 1, 1, d + 1); } }
        }
    } else if (p.t == 2) {
        const int f = p.f;
        if (f == 0) { msGen(b[0] + 1, b[1], b[2] - 1, 0, d); msGen(b[0] - 1, b[1], b[2] + 1, 2, d); msGen(b[3] + 1, b[1], b[2] + 1, 3, d); }
        else if (f == 1) { msGen(b[0] + 1, b[1], b[5] + 1, 1, d); msGen(b[0] - 1, b[1], b[2] + 1, 2, d); msGen(b[3] + 1, b[1], b[2] + 1, 3, d); }
        else if (f == 2) { msGen(b[0] + 1, b[1], b[2] - 1, 0, d); msGen(b[0] + 1, b[1], b[5] + 1, 1, d); msGen(b[0] - 1, b[1], b[2] + 1, 2, d); }
        else { msGen(b[0] + 1, b[1], b[2] - 1, 0, d); msGen(b[0] + 1, b[1], b[5] + 1, 1, d); msGen(b[3] + 1, b[1], b[2] + 1, 3, d); }
        if (b[4] - b[1] > 2) { msGen(b[0] + 1, b[1] + 4, b[2] - 1, 0, d); msGen(b[0] - 1, b[1] + 4, b[2] + 1, 2, d); msGen(b[3] + 1, b[1] + 4, b[2] + 1, 3, d); msGen(b[0] + 1, b[1] + 4, b[5] + 1, 1, d); }
    } else {
        const int f = p.f;
        if (f == 0) msGen(b[0], b[1], b[2] - 1, 0, d); else if (f == 1) msGen(b[0], b[1], b[5] + 1, 1, d); else if (f == 2) msGen(b[0] - 1, b[1], b[2], 2, d); else msGen(b[3] + 1, b[1], b[2], 3, d);
    }
}
static void msGen(int x, int y, int z, int f, int depth) {   // getNextMineShaftComponent + createRandomShaftPiece
    if (depth + 1 > 8 || std::abs(x - gMs->sx) > 80 || std::abs(z - gMs->sz) > 80) return;
    JR2& r = gMs->r; const int i = r.nextInt(100); MsPiece p{}; p.d = depth + 1; p.f = f;
    if (i >= 80) {   // cruzamento
        p.t = 2; int h = 2; if (r.nextInt(4) == 0) h += 4;
        int bx[6]; if (f == 0) { bx[0] = x - 1; bx[2] = z - 4; bx[3] = x + 3; bx[5] = z; } else if (f == 1) { bx[0] = x - 1; bx[2] = z; bx[3] = x + 3; bx[5] = z + 4; }
        else if (f == 2) { bx[0] = x - 4; bx[2] = z - 1; bx[3] = x; bx[5] = z + 3; } else { bx[0] = x; bx[2] = z - 1; bx[3] = x + 4; bx[5] = z + 3; }
        bx[1] = y; bx[4] = y + h; if (!msFree(bx)) return; memcpy(p.b, bx, sizeof bx);
    } else if (i >= 70) {   // escada
        p.t = 3; int bx[6]; if (f == 0) { bx[0] = x; bx[2] = z - 8; bx[3] = x + 2; bx[5] = z; } else if (f == 1) { bx[0] = x; bx[2] = z; bx[3] = x + 2; bx[5] = z + 8; }
        else if (f == 2) { bx[0] = x - 8; bx[2] = z; bx[3] = x; bx[5] = z + 2; } else { bx[0] = x; bx[2] = z; bx[3] = x + 8; bx[5] = z + 2; }
        bx[1] = y - 5; bx[4] = y + 2; if (!msFree(bx)) return; memcpy(p.b, bx, sizeof bx);
    } else {   // corredor: 2..4 secoes de 5
        p.t = 1; bool ok = false; int len = 0, bx[6] = {0};
        for (int n = r.nextInt(3) + 2; n > 0; --n) { len = n * 5;
            if (f == 0) { bx[0] = x; bx[2] = z - (len - 1); bx[3] = x + 2; bx[5] = z; } else if (f == 1) { bx[0] = x; bx[2] = z; bx[3] = x + 2; bx[5] = z + len - 1; }
            else if (f == 2) { bx[0] = x - (len - 1); bx[2] = z; bx[3] = x; bx[5] = z + 2; } else { bx[0] = x; bx[2] = z; bx[3] = x + len - 1; bx[5] = z + 2; }
            bx[1] = y; bx[4] = y + 2; if (msFree(bx)) { ok = true; break; } }
        if (!ok) return; memcpy(p.b, bx, sizeof bx); p.rails = r.nextInt(3) == 0; p.spiders = !p.rails && r.nextInt(23) == 0;
    }
    gMs->v.push_back(p); msBuild(gMs->v.size() - 1);
}
// --- escrita: coordenadas locais da peca (x lateral, y, z ao longo) -> mundo, como StructureComponent.getXWithOffset ---
static const MsPiece* gMsP;
static void msW(int lx, int lz, int& wx, int& wz) {
    const int* b = gMsP->b; switch (gMsP->f) {
        case 0: wx = b[0] + lx; wz = b[5] - lz; break; case 1: wx = b[0] + lx; wz = b[2] + lz; break;
        case 2: wx = b[3] - lz; wz = b[2] + lx; break; case 3: wx = b[0] + lz; wz = b[2] + lx; break; default: wx = b[0] + lx; wz = b[2] + lz; }
}
static bool msIn(int wx, int wz) { return wx >= gMsCx * 16 && wx < gMsCx * 16 + 16 && wz >= gMsCz * 16 && wz < gMsCz * 16 + 16; }
static BT msGet(int wx, int y, int wz) { return msIn(wx, wz) && y > 0 && y < CH ? gMsC->b[idx(wx - gMsCx * 16, y, wz - gMsCz * 16)] : (BT)255; }
static void msPutW(int wx, int y, int wz, int id, int meta = 0) {
    if (!msIn(wx, wz) || y < 1 || y > CH - 2) return; const int i = idx(wx - gMsCx * 16, y, wz - gMsCz * 16); if (gMsC->b[i] == BEDROCK) return;
    gMsC->b[i] = (BT)id; gMsC->m[i] = (uint8_t)meta;
}
static void msPut(int lx, int ly, int lz, int id, int meta = 0) { int wx, wz; msW(lx, lz, wx, wz); msPutW(wx, gMsP->b[1] + ly, wz, id, meta); }
static BT msAt(int lx, int ly, int lz) { int wx, wz; msW(lx, lz, wx, wz); return msGet(wx, gMsP->b[1] + ly, wz); }
static bool msRnd(int lx, int ly, int lz, uint32_t s, int pm) { int wx, wz; msW(lx, lz, wx, wz); return msIn(wx, wz) && (int)(nhash(wx, gMsP->b[1] + ly, wz, s) % 10000) < pm; }   // pm em 1/10000
static void msFill(int x0, int y0, int z0, int x1, int y1, int z1, int id) { for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) msPut(x, y, z, id); }
static void msTorch(int lx, int ly, int lz, int sx, int sz, uint32_t s) {   // wall torch (support em (sx,sz) local), 5% (randomlyPlaceBlock 0.05)
    if (!msRnd(lx, ly, lz, s, 500)) return; const int dr = gMsP->f; int dx, dz;
    if (dr == 0) { dx = sx; dz = -sz; } else if (dr == 1) { dx = sx; dz = sz; } else if (dr == 2) { dx = -sz; dz = sx; } else { dx = sz; dz = sx; }
    msPut(lx, ly, lz, XB_TORCH + 1 + (dx < 0 ? 0 : dz < 0 ? 1 : dx > 0 ? 2 : 3));
}
static void msSupport(int x0, int y0, int z, int x1, int y1, uint32_t s) {   // placeSupport
    const int FN = XB_FENCE; msFill(x0, y0, z, x0, y1 - 1, z, FN); msFill(x1, y0, z, x1, y1 - 1, z, FN); msFill(x0, y1, z, x1, y1, z, PLANKS);
    msTorch(x0 + 1, y1, z - 1, 0, 1, s + 1); msTorch(x0 + 1, y1, z + 1, 0, -1, s + 2);
}
static void msWeb(int lx, int ly, int lz, int pm, uint32_t s) { static const int web = pkFind("block/cobweb"); if (web && msAt(lx, ly, lz) == AIR && msRnd(lx, ly, lz, s, pm)) msPut(lx, ly, lz, web); }
static void msPaint(const MsPiece& p) {
    gMsP = &p; const int* b = p.b; const int w = b[3] - b[0], h = b[4] - b[1], l = b[5] - b[2];   // tamanhos - 1
    if (p.t == 0) {   // sala: piso de terra, ar ate 3 acima, domo
        for (int z = b[2]; z <= b[5]; z++) for (int x = b[0]; x <= b[3]; x++) { msPutW(x, b[1], z, DIRT); for (int y = b[1] + 1; y <= std::min(b[1] + 3, b[4]); y++) msPutW(x, y, z, AIR); }
        const double cx = (b[0] + b[3]) * 0.5, cz = (b[2] + b[5]) * 0.5, rx = w * 0.5 + 0.5, rz = l * 0.5 + 0.5, hh = std::max(1, b[4] - (b[1] + 4));
        for (int y = b[1] + 4; y <= b[4]; y++) for (int z = b[2]; z <= b[5]; z++) for (int x = b[0]; x <= b[3]; x++) {
            const double dx = (x - cx) / rx, dz = (z - cz) / rz, dy = (double)(y - (b[1] + 4)) / hh; if (dx * dx + dz * dz + dy * dy <= 1.0) msPutW(x, y, z, AIR); }
    } else if (p.t == 1) {   // corredor
        const int len = l + 1; if (p.f >= 2) { /* eixo x do mundo */ }
        msFill(0, 0, 0, 2, 1, l, AIR);
        if (p.spiders) { for (int z = 0; z <= l; z++) for (int x = 0; x <= 2; x++) msWeb(x, 2, z, 6000, 40 + z); }
        else msFill(0, 2, 0, 2, 2, l, AIR);
        static const int web = pkFind("block/cobweb"); (void)web;
        for (int j = 0; j < len / 5; j++) { const int k = 2 + j * 5; msSupport(0, 0, k, 2, 2, 100 + (uint32_t)j * 8);
            msWeb(0, 2, k - 1, 1000, 1 + j); msWeb(2, 2, k - 1, 1000, 2 + j); msWeb(0, 2, k + 1, 1000, 3 + j); msWeb(2, 2, k + 1, 1000, 4 + j);
            msWeb(0, 2, k - 2, 500, 5 + j); msWeb(2, 2, k - 2, 500, 6 + j); msWeb(0, 2, k + 2, 500, 7 + j); msWeb(2, 2, k + 2, 500, 8 + j);
            if (p.spiders && j == 0 && gSpawnerId) msPut(1, 0, k - 1 + (len > 10 ? 2 : 0), gSpawnerId); }
        for (int x = 0; x <= 2; x++) for (int z = 0; z <= l; z++) if (msAt(x, -1, z) == AIR || isWater(msAt(x, -1, z))) { msPut(x, -1, z, PLANKS); }
        if (p.rails) for (int z = 0; z <= l; z++) { const BT g = msAt(1, -1, z); if (g != AIR && g != 255 && !isWater(g) && msRnd(1, 0, z, 61, 7000)) msPut(1, 0, z, XB_RAIL, (p.f >= 2) ? 1 : 0); }
    } else if (p.t == 2) {   // cruzamento
        const int hh = h; msFill(1, 0, 0, w - 1, hh, l, AIR); msFill(0, 0, 1, w, hh, l - 1, AIR);
        if (hh > 2) { msFill(1, 3, 0, w - 1, 3, l, AIR); msFill(0, 3, 1, w, 3, l - 1, AIR); msFill(1, 2, 1, w - 1, 2, l - 1, PLANKS); }
        for (int x = 1; x <= w - 1; x += w - 2) for (int z = 1; z <= l - 1; z += l - 2) msFill(x, 0, z, x, hh, z, PLANKS);   // quatro pilares
        msFill(1, hh, 1, w - 1, hh, l - 1, PLANKS);
        for (int x = 0; x <= w; x++) for (int z = 0; z <= l; z++) if (msAt(x, -1, z) == AIR) msPut(x, -1, z, PLANKS);
    } else {   // escada: so a rampa escavada (sem blocos de degrau na decida; o chao e a propria pedra)
        msFill(0, 5, 0, 2, 7, 1, AIR); msFill(0, 0, 7, 2, 2, 8, AIR);
        for (int i = 0; i < 5; i++) msFill(0, 5 - i - (i < 4 ? 1 : 0), 2 + i, 2, 7 - i, 2 + i, AIR);
    }
}
static void mineshaftChunk(Chunk* c, int cx, int cz) {
    if (gDim) return;
    for (int sz = cz - 7; sz <= cz + 7; sz++) for (int sx = cx - 7; sx <= cx + 7; sx++) {
        JR2 g(0); g.setSeed((int64_t)WORLD_SEED); const int64_t a = g.nextLong(), bb = g.nextLong();
        g.setSeed(((int64_t)sx * a ^ (int64_t)sz * bb) ^ (int64_t)WORLD_SEED); g.nextInt();   // MapGenBase.generate; Start: rand.nextInt() antes de recreateStructures
        JR2 s(0); s.setSeed(((int64_t)sx * a ^ (int64_t)sz * bb) ^ (int64_t)WORLD_SEED);
        if (!(s.nextDouble() < 0.004 && s.nextInt(80) < std::max(std::abs(sx), std::abs(sz)))) continue;
        MsGen G; G.r.setSeed((int64_t)sx * 341873128712LL + (int64_t)sz * 132897987541LL + (int64_t)WORLD_SEED + 10387313LL); G.sx = sx * 16 + 2; G.sz = sz * 16 + 2; gMs = &G;
        MsPiece room{}; room.t = 0; room.d = 0; room.f = -1; room.b[0] = G.sx; room.b[1] = 50; room.b[2] = G.sz;
        room.b[3] = G.sx + 7 + G.r.nextInt(6); room.b[4] = 54 + G.r.nextInt(6); room.b[5] = G.sz + 7 + G.r.nextInt(6);
        G.v.push_back(room); msBuild(0);
        int top = 0; for (const auto& p : G.v) top = std::max(top, p.b[4]); int lo = 999; for (const auto& p : G.v) lo = std::min(lo, p.b[1]);
        const int i = SEA - 10, hgt = top - lo + 1; int j = hgt; if (j < i) j += G.r.nextInt(i - j); const int dy = j - top;
        gMsC = c; gMsCx = cx; gMsCz = cz;
        for (auto& p : G.v) { if (p.b[3] < cx * 16 - 1 || p.b[0] > cx * 16 + 16 || p.b[5] < cz * 16 - 1 || p.b[2] > cz * 16 + 16) continue; p.b[1] += dy; p.b[4] += dy; }
        for (const auto& p : G.v) { if (p.b[3] < cx * 16 - 1 || p.b[0] > cx * 16 + 16 || p.b[5] < cz * 16 - 1 || p.b[2] > cz * 16 + 16) continue; msPaint(p); }
    }
}
