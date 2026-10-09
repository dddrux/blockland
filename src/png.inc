// ================= PNG (decoder mínimo, sem dependências) =================
struct Bits { const uint8_t *p, *e; uint64_t buf = 0; int cnt = 0; bool err = false;   // buffer de 64 bits (leitura rapida)
    inline void fill() { while (cnt <= 56 && p < e) { buf |= (uint64_t)*p++ << cnt; cnt += 8; } }
    int bits(int n) { if (!n) return 0; if (cnt < n) { fill(); if (cnt < n) { err = true; buf = 0; cnt = 0; return 0; } } int v = (int)(buf & ((1u << n) - 1)); buf >>= n; cnt -= n; return v; }
    int bit() { return bits(1); }
    void alignByte() { const int d = cnt & 7; buf >>= d; cnt -= d; p -= cnt / 8; buf = 0; cnt = 0; } };   // bloco "stored": devolve os bytes ja pre-lidos
struct Huff { short count[16], sym[288]; uint16_t fast[512]; };   // fast: decodificacao por tabela de 9 bits ((len<<9)|simbolo); 0 = codigo longo (caminho lento)
static void hbuild(Huff& h, const short* len, int n) {
    memset(h.count, 0, sizeof h.count); memset(h.fast, 0, sizeof h.fast);
    for (int i = 0; i < n; i++) h.count[len[i]]++;
    short offs[16]; offs[1] = 0;
    for (int i = 1; i < 15; i++) offs[i + 1] = offs[i] + h.count[i];
    for (int i = 0; i < n; i++) if (len[i]) h.sym[offs[len[i]]++] = i;
    int next[17] = {0}, code = 0;
    for (int l = 1; l <= 15; l++) { code = (code + (l > 1 ? h.count[l - 1] : 0)) << 1; next[l] = code; }
    for (int i = 0; i < n; i++) { const int l = len[i]; if (!l) continue; const int c = next[l]++; if (l > 9) continue;
        int r = 0; for (int k = 0; k < l; k++) if (c >> k & 1) r |= 1 << (l - 1 - k);
        for (int k = r; k < 512; k += 1 << l) h.fast[k] = (uint16_t)(l << 9 | i); }
}
static int hdec(Bits& b, const Huff& h) {
    b.fill();
    const uint16_t f = h.fast[b.buf & 511];
    if (f) { const int l = f >> 9; if (l <= b.cnt) { b.buf >>= l; b.cnt -= l; return f & 511; } }
    int code = 0, first = 0, index = 0;
    for (int l = 1; l <= 15; l++) {
        code |= b.bit(); int c = h.count[l];
        if (code - c < first) return h.sym[index + (code - first)];
        index += c; first += c; first <<= 1; code <<= 1;
    }
    return -1;
}
static bool inflateZ(const uint8_t* in, size_t n, std::vector<uint8_t>& out) {
    static const short LB[] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const short LE[] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const short DB[] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const short DE[] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    if (n < 2) return false;
    Bits b{in + 2, in + n}; out.reserve(out.size() + n * 4);
    int last;
    do {
        last = b.bit(); int type = b.bits(2);
        if (type == 0) {
            b.alignByte();
            if (b.p + 4 > b.e) return false;
            int len = b.p[0] | b.p[1] << 8; b.p += 4;
            if (b.p + len > b.e) return false;
            out.insert(out.end(), b.p, b.p + len); b.p += len;
        } else if (type == 1 || type == 2) {
            Huff lh, dh; short l[320];
            if (type == 1) {
                for (int i = 0; i < 144; i++) l[i] = 8;
                for (int i = 144; i < 256; i++) l[i] = 9;
                for (int i = 256; i < 280; i++) l[i] = 7;
                for (int i = 280; i < 288; i++) l[i] = 8;
                hbuild(lh, l, 288);
                for (int i = 0; i < 30; i++) l[i] = 5;
                hbuild(dh, l, 30);
            } else {
                static const int ord[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
                int nl = b.bits(5) + 257, nd = b.bits(5) + 1, nc = b.bits(4) + 4;
                short cl[19] = {0};
                for (int i = 0; i < nc; i++) cl[ord[i]] = b.bits(3);
                Huff ch; hbuild(ch, cl, 19);
                int i = 0;
                while (i < nl + nd) {
                    int s = hdec(b, ch); if (s < 0 || b.err) return false;
                    if (s < 16) l[i++] = s;
                    else {
                        int rep, v = 0;
                        if (s == 16) { if (!i) return false; v = l[i - 1]; rep = 3 + b.bits(2); }
                        else if (s == 17) rep = 3 + b.bits(3); else rep = 11 + b.bits(7);
                        if (i + rep > nl + nd) return false;
                        while (rep--) l[i++] = v;
                    }
                }
                hbuild(lh, l, nl); hbuild(dh, l + nl, nd);
            }
            for (;;) {
                int s = hdec(b, lh); if (s < 0 || b.err) return false;
                if (s < 256) out.push_back((uint8_t)s);
                else if (s == 256) break;
                else {
                    s -= 257; if (s >= 29) return false;
                    int len = LB[s] + b.bits(LE[s]);
                    int ds = hdec(b, dh); if (ds < 0 || ds >= 30) return false;
                    size_t dist = DB[ds] + b.bits(DE[ds]);
                    if (dist > out.size()) return false;
                    { const size_t o = out.size(); out.resize(o + len); uint8_t* d = out.data() + o; const uint8_t* sr = d - dist; for (int k = 0; k < len; k++) d[k] = sr[k]; }
                }
            }
        } else return false;
    } while (!last);
    return true;
}
static uint32_t be32(const uint8_t* p) { return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
// devolve RGBA 8 bits. Suporta não-entrelaçado, tipos 0/2/3/4/6.
static bool loadPNGMem(const std::vector<uint8_t>& d, std::vector<uint8_t>& rgba, int& W, int& H) {
    splashPoll();   // loading: uma textura decodificada = chance de desenhar um quadro da barra
    if (d.size() < 33 || memcmp(d.data(), "\x89PNG\r\n\x1a\n", 8)) return false;
    int depth = 0, ct = 0, inter = 0; std::vector<uint8_t> idat, plte, trns;
    for (size_t p = 8; p + 12 <= d.size();) {
        uint32_t len = be32(&d[p]); const uint8_t* c = &d[p + 4];
        if (p + 12 + len > d.size()) break;
        if (!memcmp(c, "IHDR", 4)) { W = be32(c + 4); H = be32(c + 8); depth = c[12]; ct = c[13]; inter = c[16]; }
        else if (!memcmp(c, "PLTE", 4)) plte.assign(c + 4, c + 4 + len);
        else if (!memcmp(c, "tRNS", 4)) trns.assign(c + 4, c + 4 + len);
        else if (!memcmp(c, "IDAT", 4)) idat.insert(idat.end(), c + 4, c + 4 + len);
        else if (!memcmp(c, "IEND", 4)) break;
        p += 12 + len;
    }
    if (inter || W <= 0 || H <= 0 || W > 4096 || H > 4096) return false;
    int ch = ct == 0 ? 1 : ct == 2 ? 3 : ct == 3 ? 1 : ct == 4 ? 2 : ct == 6 ? 4 : 0;
    if (!ch || (depth != 8 && ct != 3)) return false;
    std::vector<uint8_t> raw;
    if (!inflateZ(idat.data(), idat.size(), raw)) return false;
    int bpp = std::max(1, ch * depth / 8); size_t stride = ((size_t)W * ch * depth + 7) / 8;
    if (raw.size() < (stride + 1) * H) return false;
    std::vector<uint8_t> img(stride * H);
    for (int y = 0; y < H; y++) {
        int ft = raw[y * (stride + 1)];
        const uint8_t* s = &raw[y * (stride + 1) + 1]; uint8_t* o = &img[y * stride]; const uint8_t* u = y ? o - stride : nullptr;
        for (size_t x = 0; x < stride; x++) {
            int a = x >= (size_t)bpp ? o[x - bpp] : 0, b = u ? u[x] : 0, c = (u && x >= (size_t)bpp) ? u[x - bpp] : 0, v = s[x];
            switch (ft) {
                case 1: v += a; break; case 2: v += b; break; case 3: v += (a + b) / 2; break;
                case 4: { int pp = a + b - c, pa = abs(pp - a), pb = abs(pp - b), pc = abs(pp - c);
                          v += (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c); } break;
            }
            o[x] = (uint8_t)v;
        }
    }
    rgba.assign((size_t)W * H * 4, 255);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        const uint8_t* row = &img[y * stride]; uint8_t* o = &rgba[((size_t)y * W + x) * 4];
        if (ct == 3) {
            int v = depth == 8 ? row[x] : (row[x * depth / 8] >> (8 - depth - (x * depth) % 8)) & ((1 << depth) - 1);
            if ((size_t)v * 3 + 2 < plte.size()) { o[0] = plte[v * 3]; o[1] = plte[v * 3 + 1]; o[2] = plte[v * 3 + 2]; }
            o[3] = (size_t)v < trns.size() ? trns[v] : 255;
        } else {
            const uint8_t* p = row + x * ch;
            if (ct == 0) o[0] = o[1] = o[2] = p[0];
            else if (ct == 4) { o[0] = o[1] = o[2] = p[0]; o[3] = p[1]; }
            else { o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; if (ct == 6) o[3] = p[3]; }
        }
    }
    return true;
}
static bool loadPNG(const char* path, std::vector<uint8_t>& rgba, int& W, int& H) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    std::vector<uint8_t> d; uint8_t tmp[4096]; size_t r;
    while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) d.insert(d.end(), tmp, tmp + r);
    fclose(f);
    return loadPNGMem(d, rgba, W, H);
}
