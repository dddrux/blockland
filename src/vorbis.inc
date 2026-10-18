#include <array>
// vorbis.inc - decodificador Ogg Vorbis I (patchAud.OGG). .ogg -> PCM16 intercalado (1 ou 2 canais). Sem libs externas.
// vbDecodeOgg(bytes, pcm, ch, rate): false se o arquivo for invalido/nao suportado. Chamado por sndGet (audio.inc).
#include <complex>
struct VbBits {
    const uint8_t* d; size_t n, pos = 0; int bit = 0; bool eop = false;
    uint32_t get(int k) { uint32_t r = 0; for (int i = 0; i < k; i++) { if (pos >= n) { eop = true; return r; } r |= (uint32_t)((d[pos] >> bit) & 1) << i; if (++bit == 8) { bit = 0; pos++; } } return r; }
};
static inline int vbIlog(unsigned x) { int r = 0; while (x) { r++; x >>= 1; } return r; }
struct VbBook { int dim = 0, n = 0; std::vector<int> tree; std::vector<float> vq; };   // tree: no i tem filhos tree[2i], tree[2i+1]; folha = -(entrada+1)
struct VbFloor { int nparts = 0, mult = 1, rbits = 0, cls[32] = {0}, dim[16] = {0}, sb[16] = {0}, mb[16] = {0}, sub[16][8]; std::vector<int> X, lo, hi, order; };
struct VbRes { int type = 0, begin = 0, end = 0, psize = 1, ncls = 1, cbook = 0, books[64][8]; };
struct VbMap { int nsub = 1, nc = 0; std::vector<int> mag, ang, mux, fl, rs; };
struct VbMode { int flag = 0, map = 0; };
struct VbDec {
    int ch = 0, rate = 0, bs[2] = {0, 0}; std::vector<VbBook> books; std::vector<VbFloor> floors; std::vector<VbRes> res; std::vector<VbMap> maps; std::vector<VbMode> modes;
    std::vector<float> prev[2]; int prevN = 0, prevFlag = 0;
};
static float vbF32(uint32_t x) { int m = (int)(x & 0x1fffff), e = (int)((x & 0x7fe00000) >> 21); if (x & 0x80000000u) m = -m; return (float)ldexp((double)m, e - 788); }
static bool vbReadBook(VbBits& r, VbBook& b) {
    if (r.get(24) != 0x564342) return false;
    const int dim = (int)r.get(16), n = (int)r.get(24); if (dim < 1 || n < 1) return false; b.dim = dim; b.n = n;
    std::vector<int> len(n, 0); const bool ord = r.get(1);
    if (!ord) { const bool sparse = r.get(1); for (int i = 0; i < n; i++) { if (sparse) { if (r.get(1)) len[i] = (int)r.get(5) + 1; } else len[i] = (int)r.get(5) + 1; } }
    else { int cur = (int)r.get(5) + 1, i = 0; while (i < n) { const int num = (int)r.get(vbIlog(n - i)); if (i + num > n || cur > 32) return false; for (int k = 0; k < num; k++) len[i + k] = cur; i += num; cur++; } }
    uint32_t avail[33] = {0}; bool first = true; b.tree.assign(2, 0); int nodes = 1;
    for (int e = 0; e < n; e++) {
        const int L = len[e]; if (!L) continue; uint32_t res;
        if (first) { res = 0; for (int i = 1; i <= L; i++) avail[i] = 1u << (32 - i); first = false; }
        else { int z = L; while (z > 0 && !avail[z]) z--; if (!z) return false; res = avail[z]; avail[z] = 0; for (int y = L; y > z; --y) avail[y] = res + (1u << (32 - y)); }
        int node = 0;
        for (int k = 0; k < L; k++) {
            const int bit = (res >> (31 - k)) & 1;
            if (k == L - 1) b.tree[node * 2 + bit] = -(e + 1);
            else { if (b.tree[node * 2 + bit] == 0) { b.tree[node * 2 + bit] = nodes++; b.tree.resize(nodes * 2, 0); } node = b.tree[node * 2 + bit]; if (node < 0) return false; }
        }
    }
    const int lt = (int)r.get(4); if (lt > 2) return false;
    if (lt) {
        const float mn = vbF32(r.get(32)), dl = vbF32(r.get(32)); const int vb = (int)r.get(4) + 1; const bool seq = r.get(1); int lv;
        if (lt == 1) { lv = (int)floor(pow((double)n, 1.0 / dim)); while (true) { double p = 1; for (int i = 0; i < dim; i++) p *= (lv + 1); if (p <= n) lv++; else break; } while (lv > 1) { double p = 1; for (int i = 0; i < dim; i++) p *= lv; if (p > n) lv--; else break; } }
        else lv = n * dim;
        std::vector<float> mult(lv); for (int i = 0; i < lv; i++) mult[i] = (float)r.get(vb);
        b.vq.assign((size_t)n * dim, 0.f);
        for (int e = 0; e < n; e++) { float last = 0; int idiv = 1; for (int i = 0; i < dim; i++) { const int mo = lt == 1 ? (e / idiv) % lv : e * dim + i; const float v = mult[mo] * dl + mn + last; if (seq) last = v; if (lt == 1) idiv *= lv; b.vq[(size_t)e * dim + i] = v; } }
    }
    return !r.eop;
}
static int vbDec(const VbBook& b, VbBits& r) { int node = 0; for (;;) { const int v = b.tree[node * 2 + r.get(1)]; if (v < 0) return -v - 1; if (v == 0 || r.eop) return -1; node = v; } }
static bool vbReadSetup(VbBits& r, VbDec& D) {
    int n = (int)r.get(8) + 1; D.books.resize(n); for (int i = 0; i < n; i++) if (!vbReadBook(r, D.books[i])) return false;
    n = (int)r.get(6) + 1; for (int i = 0; i < n; i++) r.get(16);
    n = (int)r.get(6) + 1; D.floors.resize(n);
    for (int i = 0; i < n; i++) {
        if (r.get(16) != 1) return false; VbFloor& F = D.floors[i]; F.nparts = (int)r.get(5); int mx = -1;
        for (int p = 0; p < F.nparts; p++) { F.cls[p] = (int)r.get(4); mx = std::max(mx, F.cls[p]); }
        for (int c = 0; c <= mx; c++) { F.dim[c] = (int)r.get(3) + 1; F.sb[c] = (int)r.get(2); if (F.sb[c]) F.mb[c] = (int)r.get(8); for (int j = 0; j < (1 << F.sb[c]); j++) F.sub[c][j] = (int)r.get(8) - 1; }
        F.mult = (int)r.get(2) + 1; F.rbits = (int)r.get(4); F.X = {0, 1 << F.rbits};
        for (int p = 0; p < F.nparts; p++) for (int j = 0; j < F.dim[F.cls[p]]; j++) F.X.push_back((int)r.get(F.rbits));
        const int m = (int)F.X.size(); F.lo.assign(m, 0); F.hi.assign(m, 1); F.order.resize(m); for (int j = 0; j < m; j++) F.order[j] = j;
        std::sort(F.order.begin(), F.order.end(), [&](int a, int b) { return F.X[a] < F.X[b]; });
        for (int j = 2; j < m; j++) { int lo = 0, hi = 1; for (int k = 0; k < j; k++) { if (F.X[k] < F.X[j] && F.X[k] > F.X[lo]) lo = k; if (F.X[k] > F.X[j] && F.X[k] < F.X[hi]) hi = k; } F.lo[j] = lo; F.hi[j] = hi; }
    }
    n = (int)r.get(6) + 1; D.res.resize(n);
    for (int i = 0; i < n; i++) {
        VbRes& R = D.res[i]; R.type = (int)r.get(16); if (R.type > 2) return false; R.begin = (int)r.get(24); R.end = (int)r.get(24); R.psize = (int)r.get(24) + 1; R.ncls = (int)r.get(6) + 1; R.cbook = (int)r.get(8);
        if (R.cbook >= (int)D.books.size()) return false; int cas[64];
        for (int c = 0; c < R.ncls; c++) { const int lo = (int)r.get(3); const int hi = r.get(1) ? (int)r.get(5) : 0; cas[c] = hi * 8 + lo; }
        for (int c = 0; c < R.ncls; c++) for (int j = 0; j < 8; j++) { R.books[c][j] = (cas[c] >> j & 1) ? (int)r.get(8) : -1; if (R.books[c][j] >= (int)D.books.size()) return false; }
    }
    n = (int)r.get(6) + 1; D.maps.resize(n);
    for (int i = 0; i < n; i++) {
        if (r.get(16) != 0) return false; VbMap& M = D.maps[i]; M.nsub = r.get(1) ? (int)r.get(4) + 1 : 1;
        if (r.get(1)) { M.nc = (int)r.get(8) + 1; for (int j = 0; j < M.nc; j++) { M.mag.push_back((int)r.get(vbIlog(D.ch - 1))); M.ang.push_back((int)r.get(vbIlog(D.ch - 1))); if (M.mag[j] >= D.ch || M.ang[j] >= D.ch || M.mag[j] == M.ang[j]) return false; } }
        if (r.get(2)) return false; M.mux.assign(D.ch, 0); if (M.nsub > 1) for (int c = 0; c < D.ch; c++) { M.mux[c] = (int)r.get(4); if (M.mux[c] >= M.nsub) return false; }
        for (int s = 0; s < M.nsub; s++) { r.get(8); M.fl.push_back((int)r.get(8)); M.rs.push_back((int)r.get(8)); if (M.fl[s] >= (int)D.floors.size() || M.rs[s] >= (int)D.res.size()) return false; }
    }
    n = (int)r.get(6) + 1; D.modes.resize(n);
    for (int i = 0; i < n; i++) { D.modes[i].flag = (int)r.get(1); r.get(16); r.get(16); D.modes[i].map = (int)r.get(8); if (D.modes[i].map >= (int)D.maps.size()) return false; }
    return !r.eop;
}
static void vbFft(std::complex<double>* a, int n) {
    for (int i = 1, j = 0; i < n; i++) { int bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap(a[i], a[j]); }
    for (int len = 2; len <= n; len <<= 1) { const std::complex<double> wl = std::polar(1.0, -6.283185307179586 / len); for (int i = 0; i < n; i += len) { std::complex<double> w(1, 0); for (int j = 0; j < len / 2; j++) { const std::complex<double> u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } } }
}
// IMDCT: M coeficientes -> N = 2M amostras (formula do spec, sem escala; vbImdctNaive so para teste)
static void vbImdct(const float* X, int M, float* y) {
    const int H = M / 2; std::vector<std::complex<double>> t(H); std::vector<double> c(M);
    for (int n = 0; n < H; n++) t[n] = std::complex<double>(X[2 * n], X[M - 1 - 2 * n]) * std::polar(1.0, -3.141592653589793 * (4 * n + 1) / (4.0 * M));
    vbFft(t.data(), H);
    for (int k = 0; k < H; k++) { const std::complex<double> u = t[k] * std::polar(1.0, -3.141592653589793 * k / (double)M); c[2 * k] = u.real(); c[M - 1 - 2 * k] = -u.imag(); }
    for (int i = 0; i < 2 * M; i++) { const int ip = i + M / 2; y[i] = (float)(ip < M ? c[ip] : ip < 2 * M ? -c[2 * M - 1 - ip] : -c[ip - 2 * M]); }
}
static void vbImdctNaive(const float* X, int M, float* y) { for (int i = 0; i < 2 * M; i++) { double s = 0; for (int k = 0; k < M; k++) s += X[k] * cos(3.141592653589793 / M * (i + .5 + M / 2.0) * (k + .5)); y[i] = (float)s; } }
static bool vbFloorDec(VbDec& D, const VbFloor& F, VbBits& r, std::vector<float>& out, int half) {
    if (!r.get(1)) return false;
    static const int RNG[4] = {256, 128, 86, 64}; const int range = RNG[F.mult - 1], m = (int)F.X.size(); std::vector<int> Y(m, 0);
    Y[0] = (int)r.get(vbIlog(range - 1)); Y[1] = (int)r.get(vbIlog(range - 1)); int off = 2;
    for (int p = 0; p < F.nparts; p++) {
        const int c = F.cls[p], cd = F.dim[c], cb = F.sb[c]; int cval = 0;
        if (cb) { cval = vbDec(D.books[F.mb[c]], r); if (cval < 0) return false; }
        for (int j = 0; j < cd; j++) { const int bk = F.sub[c][cval & ((1 << cb) - 1)]; cval >>= cb; if (bk >= 0) { const int v = vbDec(D.books[bk], r); if (v < 0) return false; Y[off + j] = v; } else Y[off + j] = 0; }
        off += cd;
    }
    std::vector<int> fy(m, 0); std::vector<char> s2(m, 0); fy[0] = Y[0]; fy[1] = Y[1]; s2[0] = s2[1] = 1;
    for (int i = 2; i < m; i++) {
        const int lo = F.lo[i], hi = F.hi[i], dy = fy[hi] - fy[lo], adx = F.X[hi] - F.X[lo]; if (adx <= 0) return false;
        const int ady = abs(dy), err = ady * (F.X[i] - F.X[lo]), off2 = err / adx; const int pred = dy < 0 ? fy[lo] - off2 : fy[lo] + off2;
        const int val = Y[i], hr = range - pred, lr = pred, room = std::min(hr, lr) * 2;
        if (val) {
            s2[lo] = s2[hi] = s2[i] = 1;
            if (val >= room) fy[i] = hr > lr ? val - lr + pred : pred - val + hr - 1;
            else fy[i] = (val & 1) ? pred - ((val + 1) >> 1) : pred + (val >> 1);
        } else { s2[i] = 0; fy[i] = pred; }
    }
    std::vector<int> line(half, 0);
    auto rl = [&](int x0, int y0, int x1, int y1) {
        const int dy = y1 - y0, adx = x1 - x0; if (adx <= 0) return; int ady = abs(dy); const int base = dy / adx, sy = dy < 0 ? base - 1 : base + 1; ady -= abs(base) * adx; int x = x0, y = y0, err = 0;
        if (x < half) line[x] = y; x++;
        while (x < x1) { err += ady; if (err >= adx) { err -= adx; y += sy; } else y += base; if (x < half) line[x] = y; x++; }
    };
    int lx = 0, ly = fy[F.order[0]] * F.mult, hx = 0, hy = ly;
    for (int k = 1; k < m; k++) { const int i = F.order[k]; if (s2[i]) { hy = fy[i] * F.mult; hx = F.X[i]; rl(lx, ly, hx, hy); lx = hx; ly = hy; } }
    if (hx < half) rl(hx, hy, half, hy);
    static const std::array<float, 256> tab = []() { std::array<float, 256> t; const double s = log(1.0 / 1.0649863e-07) / 255.0; for (int i = 0; i < 256; i++) t[i] = (float)exp((i - 255) * s); return t; }();   // patchPerf.1: init thread-safe (decodifica na thread da musica)
    out.resize(half); for (int i = 0; i < half; i++) out[i] = tab[std::max(0, std::min(255, line[i]))];
    return true;
}
static void vbResDec(VbDec& D, const VbRes& R, VbBits& r, std::vector<float*>& vs, std::vector<char>& dnd, int vsize) {
    const VbBook& cb = D.books[R.cbook]; const int cw = cb.dim, nv = (int)vs.size(); const int lb = std::min(R.begin, vsize), le = std::min(R.end, vsize), ntr = le - lb; if (ntr <= 0) return;
    const int ptr = ntr / R.psize; std::vector<std::vector<int>> cls(nv, std::vector<int>(ptr + cw, 0));
    for (int pass = 0; pass < 8; pass++) {
        for (int pc = 0; pc < ptr;) {
            if (pass == 0) for (int j = 0; j < nv; j++) if (!dnd[j]) { int t = vbDec(cb, r); if (t < 0) return; for (int i = cw - 1; i >= 0; i--) { cls[j][pc + i] = t % R.ncls; t /= R.ncls; } }
            for (int i = 0; i < cw && pc < ptr; i++, pc++) for (int j = 0; j < nv; j++) if (!dnd[j]) {
                const int bk = R.books[cls[j][pc]][pass]; if (bk < 0) continue; const VbBook& b = D.books[bk]; if (b.vq.empty()) return; float* o = vs[j] + lb + pc * R.psize;
                if (R.type == 0) { const int step = R.psize / b.dim; for (int k = 0; k < step; k++) { const int e = vbDec(b, r); if (e < 0) return; for (int d = 0; d < b.dim; d++) o[k + d * step] += b.vq[(size_t)e * b.dim + d]; } }
                else for (int k = 0; k < R.psize; k += b.dim) { const int e = vbDec(b, r); if (e < 0) return; for (int d = 0; d < b.dim && k + d < R.psize; d++) o[k + d] += b.vq[(size_t)e * b.dim + d]; }
            }
        }
    }
}
static float vbSlope(int i, int n) { const double s = sin(3.141592653589793 * (i + .5) / (2.0 * n)); return (float)sin(1.5707963267948966 * s * s); }
static float gVbScale = 1.f;   // escala do IMDCT (calibrada nos testes contra o ffmpeg)
static bool vbPacket(VbDec& D, const uint8_t* p, size_t len, std::vector<int16_t>& pcm) {
    VbBits r{p, len}; if (r.get(1)) return true;   // nao e pacote de audio
    const int nm = (int)D.modes.size(); const int mi = (int)r.get(vbIlog(nm - 1)); if (mi >= nm) return false;
    const VbMode& md = D.modes[mi]; const int flag = md.flag, n = D.bs[flag], half = n / 2; int pf = 0, nf = 0; if (flag) { pf = (int)r.get(1); nf = (int)r.get(1); }
    const VbMap& M = D.maps[md.map]; const int ch = D.ch;
    std::vector<std::vector<float>> fl(ch), sp(ch, std::vector<float>(half, 0.f)); std::vector<char> nz(ch, 0);
    for (int c = 0; c < ch; c++) { const int s = M.mux[c]; nz[c] = vbFloorDec(D, D.floors[M.fl[s]], r, fl[c], half); if (!nz[c]) fl[c].assign(half, 0.f); }
    for (int i = 0; i < M.nc; i++) if (nz[M.mag[i]] || nz[M.ang[i]]) nz[M.mag[i]] = nz[M.ang[i]] = 1;
    for (int s = 0; s < M.nsub; s++) {
        const VbRes& R = D.res[M.rs[s]]; std::vector<int> chs; for (int c = 0; c < ch; c++) if (M.mux[c] == s) chs.push_back(c); if (chs.empty()) continue;
        std::vector<char> dnd; for (int c : chs) dnd.push_back(!nz[c]);
        if (R.type == 2) {
            bool any = false; for (char d : dnd) if (!d) any = true; if (!any) continue; const int nc = (int)chs.size(); std::vector<float> buf((size_t)half * nc, 0.f); std::vector<float*> vs{buf.data()}; std::vector<char> d1{0};
            vbResDec(D, R, r, vs, d1, half * nc); for (int k = 0; k < nc; k++) for (int i = 0; i < half; i++) sp[chs[k]][i] = buf[(size_t)i * nc + k];
        } else { std::vector<float*> vs; for (int c : chs) vs.push_back(sp[c].data()); vbResDec(D, R, r, vs, dnd, half); }
    }
    for (int i = M.nc - 1; i >= 0; i--) {
        std::vector<float>& m = sp[M.mag[i]]; std::vector<float>& a = sp[M.ang[i]];
        for (int k = 0; k < half; k++) { const float mm = m[k], aa = a[k]; if (mm > 0) { if (aa > 0) { m[k] = mm; a[k] = mm - aa; } else { a[k] = mm; m[k] = mm + aa; } } else { if (aa > 0) { m[k] = mm; a[k] = mm + aa; } else { a[k] = mm; m[k] = mm - aa; } } }
    }
    int ln = half, ls = 0, rn = half, rs = half; if (flag && !D.prevFlag) { ln = D.bs[0] / 2; ls = n / 4 - D.bs[0] / 4; } if (flag && !nf) { rn = D.bs[0] / 2; rs = n * 3 / 4 - D.bs[0] / 4; }
    std::vector<float> w(n, 0.f); for (int i = 0; i < n; i++) w[i] = i < ls ? 0.f : i < ls + ln ? vbSlope(i - ls, ln) : i < rs ? 1.f : i < rs + rn ? vbSlope(rn - 1 - (i - rs), rn) : 0.f;
    std::vector<std::vector<float>> y(ch, std::vector<float>(n));
    for (int c = 0; c < ch; c++) { for (int k = 0; k < half; k++) sp[c][k] *= fl[c][k]; vbImdct(sp[c].data(), half, y[c].data()); for (int i = 0; i < n; i++) y[c][i] *= w[i] * gVbScale; }
    if (D.prevN > 0) {
        const int pn = D.prevN, cnt = pn / 4 + n / 4;
        for (int k = 0; k < cnt; k++) for (int c = 0; c < ch; c++) { float s = 0; const int pi = pn / 2 + k, ci = n / 4 - pn / 4 + k; if (pi < pn) s += D.prev[c][pi]; if (ci >= 0 && ci < n) s += y[c][ci]; const int v = (int)lrintf(s * 32768.f); pcm.push_back((int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v)); }
    }
    for (int c = 0; c < ch; c++) D.prev[c] = y[c]; D.prevN = n; D.prevFlag = flag; return true;
}
static bool vbDecodeOgg(const std::vector<uint8_t>& f, std::vector<int16_t>& pcm, int& chOut, int& rateOut) {
    std::vector<std::vector<uint8_t>> pk; std::vector<uint8_t> cur; size_t o = 0;
    while (o + 27 <= f.size() && !memcmp(&f[o], "OggS", 4)) {
        const int ns = f[o + 26]; if (o + 27 + ns > f.size()) break; size_t dp = o + 27 + ns;
        for (int i = 0; i < ns; i++) { const int sz = f[o + 27 + i]; if (dp + sz > f.size()) return false; cur.insert(cur.end(), f.begin() + dp, f.begin() + dp + sz); dp += sz; if (sz < 255) { pk.push_back(std::move(cur)); cur.clear(); } }
        o = dp;
    }
    if (pk.size() < 4) return false;
    VbDec D; { const std::vector<uint8_t>& h = pk[0]; if (h.size() < 30 || h[0] != 1 || memcmp(&h[1], "vorbis", 6)) return false;
        D.ch = h[11]; D.rate = (int)(h[12] | (h[13] << 8) | (h[14] << 16) | ((uint32_t)h[15] << 24)); D.bs[0] = 1 << (h[28] & 15); D.bs[1] = 1 << (h[28] >> 4); if (D.ch < 1 || D.ch > 2 || D.bs[0] < 64 || D.bs[1] < D.bs[0]) return false; }
    { const std::vector<uint8_t>& s = pk[2]; if (s.size() < 8 || s[0] != 5 || memcmp(&s[1], "vorbis", 6)) return false; VbBits r{s.data() + 7, s.size() - 7}; if (!vbReadSetup(r, D)) return false; }
    pcm.clear(); for (size_t i = 3; i < pk.size(); i++) if (!pk[i].empty() && !vbPacket(D, pk[i].data(), pk[i].size(), pcm)) break;
    chOut = D.ch; rateOut = D.rate; return !pcm.empty();
}
