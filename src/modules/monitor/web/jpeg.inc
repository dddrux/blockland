// ---------- JPEG: decodificador baseline (quadros de outros jogadores / imagens de paginas) + codificador (envia a tela aos outros jogadores) ----------
// Incluido por web/web.inc DENTRO de namespace mod_monitor (so desktop). Sem dependencias externas.
// ======== JPEG baseline (huffman sequencial, 8 bits, 1 ou 3 componentes, qualquer subamostragem, restart) ========
struct JHuff { int cnt[17] = {}, minc[17] = {}, vptr[17] = {}; uint8_t sym[256] = {}; bool ok = false; };
struct JComp { int id = 0, h = 1, v = 1, tq = 0, td = 0, ta = 0, pw = 0, ph = 0, pred = 0; std::vector<uint8_t> pl; };
static const uint8_t JZZ[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
struct JRd {
    const uint8_t* p; const uint8_t* e; uint32_t buf = 0; int n = 0; bool mk = false;
    int bit() { if (!n) { if (mk || p >= e) { mk = true; return 0; } const uint8_t c = *p++; if (c == 0xFF) { if (p < e && *p == 0) p++; else { p--; mk = true; return 0; } } buf = c; n = 8; } n--; return (buf >> n) & 1; }
    int bits(int k) { int v = 0; while (k--) v = (v << 1) | bit(); return v; }
    int ext(int s) { const int v = bits(s); return v < (1 << (s - 1)) ? v - (1 << s) + 1 : v; }
    int dec(const JHuff& h) { int c = 0; for (int l = 1; l <= 16; l++) { c = (c << 1) | bit(); const int d = c - h.minc[l]; if (d >= 0 && d < h.cnt[l]) return h.sym[h.vptr[l] + d]; } return -1; }
    void restart() { n = 0; mk = false; while (p + 1 < e && !(p[0] == 0xFF && p[1] >= 0xD0 && p[1] <= 0xD7)) p++; if (p + 1 < e) p += 2; }
};
static void jIdct(const int* in, uint8_t* o, int stride) {
    static float C[8][8]; static bool init = false;
    if (!init) { for (int u = 0; u < 8; u++) for (int x = 0; x < 8; x++) C[u][x] = (u ? 1.f : 0.70710678f) * cosf((2 * x + 1) * u * 3.14159265f / 16.f) * .5f; init = true; }
    float t[64];
    for (int v = 0; v < 8; v++) for (int x = 0; x < 8; x++) { float s = 0; for (int u = 0; u < 8; u++) s += C[u][x] * in[v * 8 + u]; t[v * 8 + x] = s; }
    for (int x = 0; x < 8; x++) for (int y = 0; y < 8; y++) { float s = 0; for (int v = 0; v < 8; v++) s += C[v][y] * t[v * 8 + x]; const int r = (int)(s + 128.5f); o[y * stride + x] = (uint8_t)(r < 0 ? 0 : r > 255 ? 255 : r); }
}
static bool jpegDecode(const uint8_t* d, size_t n, std::vector<uint32_t>& out, int& W, int& H) {   // out: RGBA (R no byte baixo)
    if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return false;
    int qt[4][64] = {}; JHuff dc[4], ac[4]; JComp cs[3]; int nc = 0, ri = 0; W = H = 0; size_t i = 2; const uint8_t* ent = nullptr;
    while (i + 4 <= n && !ent) {
        if (d[i] != 0xFF) { i++; continue; } const int m = d[i + 1]; if (m == 0xFF) { i++; continue; }
        if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { i += 2; continue; } if (m == 0xD9) break;
        const size_t L = ((size_t)d[i + 2] << 8) | d[i + 3]; if (L < 2 || i + 2 + L > n) return false; const uint8_t* s = d + i + 4; const uint8_t* se = d + i + 2 + L;
        if (m == 0xDB) { while (s < se) { const int pq = s[0] >> 4, t = s[0] & 3; s++; for (int k = 0; k < 64; k++) { if (s + (pq ? 2 : 1) > se) return false; qt[t][JZZ[k]] = pq ? (s[0] << 8 | s[1]) : s[0]; s += pq ? 2 : 1; } } }
        else if (m == 0xC4) { while (s + 17 <= se) { JHuff& h = (s[0] >> 4) ? ac[s[0] & 3] : dc[s[0] & 3]; int tot = 0; for (int l = 1; l <= 16; l++) { h.cnt[l] = s[l]; tot += s[l]; } s += 17; if (tot > 256 || s + tot > se) return false;
                memcpy(h.sym, s, tot); s += tot; int code = 0, k = 0; for (int l = 1; l <= 16; l++) { h.vptr[l] = k; h.minc[l] = code; code += h.cnt[l]; k += h.cnt[l]; code <<= 1; } h.ok = true; } }
        else if (m == 0xC0 || m == 0xC1) { if (se - s < 6 || s[0] != 8) return false; H = s[1] << 8 | s[2]; W = s[3] << 8 | s[4]; nc = s[5]; if ((nc != 1 && nc != 3) || se - s < 6 + 3 * nc) return false;
            for (int c = 0; c < nc; c++) { cs[c].id = s[6 + 3 * c]; cs[c].h = s[7 + 3 * c] >> 4; cs[c].v = s[7 + 3 * c] & 15; cs[c].tq = s[8 + 3 * c] & 3; if (cs[c].h < 1 || cs[c].h > 4 || cs[c].v < 1 || cs[c].v > 4) return false; } }
        else if (m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) return false;   // progressivo / aritmetico / sem perdas: nao suportado
        else if (m == 0xDD) { if (se - s >= 2) ri = s[0] << 8 | s[1]; }
        else if (m == 0xDA) { if (!nc || se - s < 1 + 2 * nc || s[0] != nc) return false;
            for (int k = 0; k < nc; k++) { int f = -1; for (int c = 0; c < nc; c++) if (cs[c].id == s[1 + 2 * k]) f = c; if (f < 0) return false; cs[f].td = s[2 + 2 * k] >> 4 & 3; cs[f].ta = s[2 + 2 * k] & 3; }
            ent = se; }
        i += 2 + L;
    }
    if (!ent || W <= 0 || H <= 0 || W > 4096 || H > 4096) return false;
    int Hm = 1, Vm = 1; for (int c = 0; c < nc; c++) { Hm = std::max(Hm, cs[c].h); Vm = std::max(Vm, cs[c].v); }
    const int mx = (W + 8 * Hm - 1) / (8 * Hm), my = (H + 8 * Vm - 1) / (8 * Vm);
    for (int c = 0; c < nc; c++) { if (!dc[cs[c].td].ok || !ac[cs[c].ta].ok) return false; cs[c].pw = mx * cs[c].h * 8; cs[c].ph = my * cs[c].v * 8; cs[c].pl.assign((size_t)cs[c].pw * cs[c].ph, 0); }
    JRd r; r.p = ent; r.e = d + n; int blk[64], cnt = 0;
    for (int yy = 0; yy < my; yy++) for (int xx = 0; xx < mx; xx++, cnt++) {
        if (ri && cnt && cnt % ri == 0) { r.restart(); for (int c = 0; c < nc; c++) cs[c].pred = 0; }
        for (int c = 0; c < nc; c++) { JComp& C = cs[c];
            for (int by = 0; by < C.v; by++) for (int bx = 0; bx < C.h; bx++) {
                memset(blk, 0, sizeof blk); const int t = r.dec(dc[C.td]); if (t < 0 || t > 11) return false; C.pred += t ? r.ext(t) : 0; blk[0] = C.pred * qt[C.tq][0];
                for (int k = 1; k < 64;) { const int rs = r.dec(ac[C.ta]); if (rs < 0) return false; const int ss = rs & 15, run = rs >> 4;
                    if (!ss) { if (run == 15) { k += 16; continue; } break; } k += run; if (k > 63) break; blk[JZZ[k]] = r.ext(ss) * qt[C.tq][JZZ[k]]; k++; }
                jIdct(blk, &C.pl[(size_t)((yy * C.v + by) * 8) * C.pw + (xx * C.h + bx) * 8], C.pw); } }
    }
    out.resize((size_t)W * H);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        auto smp = [&](const JComp& c) { return (int)c.pl[(size_t)(y * c.v / Vm) * c.pw + (x * c.h / Hm)]; };
        int R, G, B; if (nc == 1) R = G = B = smp(cs[0]); else { const int Y = smp(cs[0]), cb = smp(cs[1]) - 128, cr = smp(cs[2]) - 128;
            R = Y + (int)(1.402f * cr); G = Y - (int)(.344136f * cb + .714136f * cr); B = Y + (int)(1.772f * cb); }
        R = R < 0 ? 0 : R > 255 ? 255 : R; G = G < 0 ? 0 : G > 255 ? 255 : G; B = B < 0 ? 0 : B > 255 ? 255 : B;
        out[(size_t)y * W + x] = 0xFF000000u | ((uint32_t)B << 16) | ((uint32_t)G << 8) | (uint32_t)R;
    }
    return true;
}


// ======== codificador JPEG baseline 4:2:0 (tabelas Huffman otimas por quadro, Anexo K.2 da norma; sem tabelas decoradas) ========
// px: RGBA (R no byte baixo). Devolve false se algo estiver errado. quality 10..95.
struct JEncBits { std::string o; uint32_t acc = 0; int n = 0;
    void put(uint32_t code, int len) { acc = (acc << len) | (code & ((1u << len) - 1)); n += len; while (n >= 8) { const uint8_t b = (uint8_t)(acc >> (n - 8)); o += (char)b; if (b == 0xFF) o += (char)0; n -= 8; } acc &= (1u << n) - 1; }
    void flush() { if (n > 0) put(0x7F, 8 - n); } };
struct JEncTab { uint8_t bits[17] = {}; uint8_t val[256] = {}; int nval = 0; uint16_t code[257] = {}; uint8_t len[257] = {};
    void build(const long* freqIn) {   // freqIn[0..255]; Anexo K.2
        long long freq[257]; int codesize[257], others[257]; for (int i = 0; i < 256; i++) freq[i] = freqIn[i]; freq[256] = 1; for (int i = 0; i < 257; i++) { codesize[i] = 0; others[i] = -1; }
        for (;;) { int c1 = -1, c2 = -1; long long v = 1LL << 60; for (int i = 0; i <= 256; i++) if (freq[i] && freq[i] <= v) { v = freq[i]; c1 = i; }
            v = 1LL << 60; for (int i = 0; i <= 256; i++) if (freq[i] && freq[i] <= v && i != c1) { v = freq[i]; c2 = i; } if (c2 < 0) break;
            freq[c1] += freq[c2]; freq[c2] = 0; codesize[c1]++; while (others[c1] >= 0) { c1 = others[c1]; codesize[c1]++; } others[c1] = c2; codesize[c2]++; while (others[c2] >= 0) { c2 = others[c2]; codesize[c2]++; } }
        int cnt[40] = {}; for (int i = 0; i <= 256; i++) if (codesize[i]) cnt[std::min(codesize[i], 39)]++;
        for (int i = 39; i > 16; i--) while (cnt[i] > 0) { int j = i - 2; while (cnt[j] == 0) j--; cnt[i] -= 2; cnt[i - 1]++; cnt[j + 1] += 2; cnt[j]--; }
        int i = 16; while (i > 0 && cnt[i] == 0) i--; if (i > 0) cnt[i]--;   // remove o simbolo reservado (256)
        for (int l = 1; l <= 16; l++) bits[l] = (uint8_t)cnt[l];
        nval = 0; for (int l = 1; l < 40; l++) for (int s = 0; s < 256; s++) if (codesize[s] == l && nval < 256) val[nval++] = (uint8_t)s;
        int c = 0, k = 0; for (int l = 1; l <= 16; l++) { for (int q = 0; q < bits[l]; q++, k++) { code[val[k]] = (uint16_t)c; len[val[k]] = (uint8_t)l; c++; } c <<= 1; }
    } };
static bool jpegEncode(const uint32_t* px, int W, int H, int quality, std::string& out) {
    if (!px || W < 8 || H < 8 || W > 4096 || H > 4096) return false;
    static const int QL[64] = {16,11,10,16,24,40,51,61, 12,12,14,19,26,58,60,55, 14,13,16,24,40,57,69,56, 14,17,22,29,51,87,80,62, 18,22,37,56,68,109,103,77, 24,35,55,64,81,104,113,92, 49,64,78,87,103,121,120,101, 72,92,95,98,112,100,103,99};
    static const int QC[64] = {17,18,24,47,99,99,99,99, 18,21,26,66,99,99,99,99, 24,26,56,99,99,99,99,99, 47,66,99,99,99,99,99,99, 99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99};
    quality = std::max(10, std::min(95, quality)); const int sc = quality < 50 ? 5000 / quality : 200 - quality * 2; int ql[64], qc[64];   // natural order -> zigzag na saida
    for (int i = 0; i < 64; i++) { ql[i] = std::max(1, std::min(255, (QL[i] * sc + 50) / 100)); qc[i] = std::max(1, std::min(255, (QC[i] * sc + 50) / 100)); }
    static float C[8][8]; static bool init = false; if (!init) { for (int u = 0; u < 8; u++) for (int x = 0; x < 8; x++) C[u][x] = (u ? 1.f : 0.70710678f) * cosf((2 * x + 1) * u * 3.14159265f / 16.f) * .5f; init = true; }
    const int mw = (W + 15) / 16, mh = (H + 15) / 16; const int cw = mw * 8, ch = mh * 8, yw = mw * 16, yh = mh * 16;
    std::vector<float> Y((size_t)yw * yh), Cb((size_t)cw * ch), Cr((size_t)cw * ch);
    for (int y = 0; y < yh; y++) for (int x = 0; x < yw; x++) { const uint32_t p = px[(size_t)std::min(y, H - 1) * W + std::min(x, W - 1)]; const float r = (float)(p & 255), g = (float)((p >> 8) & 255), b = (float)((p >> 16) & 255);
        Y[(size_t)y * yw + x] = .299f * r + .587f * g + .114f * b - 128.f; }
    for (int y = 0; y < ch; y++) for (int x = 0; x < cw; x++) { float r = 0, g = 0, b = 0; for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) { const uint32_t p = px[(size_t)std::min(2 * y + dy, H - 1) * W + std::min(2 * x + dx, W - 1)]; r += (float)(p & 255); g += (float)((p >> 8) & 255); b += (float)((p >> 16) & 255); }
        r *= .25f; g *= .25f; b *= .25f; Cb[(size_t)y * cw + x] = -.168736f * r - .331264f * g + .5f * b; Cr[(size_t)y * cw + x] = .5f * r - .418688f * g - .081312f * b; }
    struct Sym { uint8_t tab; uint8_t sym; uint8_t nb; uint16_t bits; };   // tab: 0 DC lum, 1 AC lum, 2 DC chr, 3 AC chr
    std::vector<Sym> st; st.reserve((size_t)mw * mh * 80); long freq[4][256] = {};
    auto cat = [](int v) { int a = v < 0 ? -v : v, n = 0; while (a) { n++; a >>= 1; } return n; };
    auto block = [&](const std::vector<float>& pl, int stride, int bx, int by, const int* q, int& pred, int tdc) {
        float t[64], d[64]; for (int v = 0; v < 8; v++) for (int x = 0; x < 8; x++) { float s = 0; for (int y = 0; y < 8; y++) s += C[v][y] * pl[(size_t)(by + y) * stride + bx + x]; t[v * 8 + x] = s; }
        for (int v = 0; v < 8; v++) for (int u = 0; u < 8; u++) { float s = 0; for (int x = 0; x < 8; x++) s += C[u][x] * t[v * 8 + x]; d[v * 8 + u] = s; }
        int z[64]; for (int k = 0; k < 64; k++) { const int i = JZZ[k]; z[k] = (int)lrintf(d[i] / (float)q[i]); }
        const int diff = z[0] - pred; pred = z[0]; int n = cat(diff); st.push_back({(uint8_t)tdc, (uint8_t)n, (uint8_t)n, (uint16_t)(diff < 0 ? diff - 1 : diff)}); freq[tdc][n]++;
        int run = 0; for (int k = 1; k < 64; k++) { if (!z[k]) { run++; continue; } while (run > 15) { st.push_back({(uint8_t)(tdc + 1), 0xF0, 0, 0}); freq[tdc + 1][0xF0]++; run -= 16; }
            n = cat(z[k]); const uint8_t s = (uint8_t)((run << 4) | n); st.push_back({(uint8_t)(tdc + 1), s, (uint8_t)n, (uint16_t)(z[k] < 0 ? z[k] - 1 : z[k])}); freq[tdc + 1][s]++; run = 0; }
        if (run) { st.push_back({(uint8_t)(tdc + 1), 0x00, 0, 0}); freq[tdc + 1][0]++; } };
    int pY = 0, pCb = 0, pCr = 0;
    for (int my = 0; my < mh; my++) for (int mx = 0; mx < mw; mx++) {
        block(Y, yw, mx * 16, my * 16, ql, pY, 0); block(Y, yw, mx * 16 + 8, my * 16, ql, pY, 0); block(Y, yw, mx * 16, my * 16 + 8, ql, pY, 0); block(Y, yw, mx * 16 + 8, my * 16 + 8, ql, pY, 0);
        block(Cb, cw, mx * 8, my * 8, qc, pCb, 2); block(Cr, cw, mx * 8, my * 8, qc, pCr, 2); }
    JEncTab T[4]; for (int i = 0; i < 4; i++) T[i].build(freq[i]);
    std::string o; auto w16 = [&](int v) { o += (char)(v >> 8); o += (char)(v & 255); };
    o += "\xFF\xD8"; o += "\xFF\xDB"; w16(2 + 65 * 2); o += (char)0; for (int k = 0; k < 64; k++) o += (char)ql[JZZ[k]]; o += (char)1; for (int k = 0; k < 64; k++) o += (char)qc[JZZ[k]];
    o += "\xFF\xC0"; w16(17); o += (char)8; w16(H); w16(W); o += (char)3; o += (char)1; o += (char)0x22; o += (char)0; o += (char)2; o += (char)0x11; o += (char)1; o += (char)3; o += (char)0x11; o += (char)1;
    static const uint8_t TID[4] = {0x00, 0x10, 0x01, 0x11};
    for (int i = 0; i < 4; i++) { int tot = 0; for (int l = 1; l <= 16; l++) tot += T[i].bits[l]; o += "\xFF\xC4"; w16(2 + 17 + tot); o += (char)TID[i]; for (int l = 1; l <= 16; l++) o += (char)T[i].bits[l]; for (int k = 0; k < tot; k++) o += (char)T[i].val[k]; }
    o += "\xFF\xDA"; w16(12); o += (char)3; o += (char)1; o += (char)0x00; o += (char)2; o += (char)0x11; o += (char)3; o += (char)0x11; o += (char)0; o += (char)63; o += (char)0;
    JEncBits bw; for (const Sym& s : st) { const JEncTab& t = T[s.tab]; if (!t.len[s.sym]) return false; bw.put(t.code[s.sym], t.len[s.sym]); if (s.nb) bw.put(s.bits, s.nb); }
    bw.flush(); o += bw.o; o += "\xFF\xD9"; out.swap(o); return true;
}
