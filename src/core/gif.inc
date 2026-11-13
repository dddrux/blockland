// ================= patchCapa.2: decodificador GIF (87a/89a) =================
// LZW, paleta global/local, entrelacado, transparencia e disposal 0-3. Cada quadro sai COMPOSTO (RGBA, tamanho da tela do GIF).
struct GifImg { int w = 0, h = 0; std::vector<std::vector<uint8_t>> fr; std::vector<int> delay; };   // delay em ms
static bool gifLzw(const std::vector<uint8_t>& d, int minCode, std::vector<uint8_t>& out, size_t need) {
    if (minCode < 2 || minCode > 8) return false;
    const int clear = 1 << minCode, eoi = clear + 1;
    std::vector<uint16_t> prefix(4096, 0); std::vector<uint8_t> suffix(4096, 0), stack(4100);
    for (int i = 0; i < clear; i++) suffix[i] = (uint8_t)i;
    int codeSize = minCode + 1, next = eoi + 1, prev = -1, nb = 0; uint32_t bits = 0; size_t p = 0; uint8_t first = 0;
    out.clear(); out.reserve(need);
    while (out.size() < need) {
        while (nb < codeSize) { if (p >= d.size()) return !out.empty(); bits |= (uint32_t)d[p++] << nb; nb += 8; }
        const int code = (int)(bits & ((1u << codeSize) - 1)); bits >>= codeSize; nb -= codeSize;
        if (code == clear) { codeSize = minCode + 1; next = eoi + 1; prev = -1; continue; }
        if (code == eoi) break;
        if (prev < 0) { if (code >= clear) return false; out.push_back(suffix[code]); first = suffix[code]; prev = code; continue; }
        int cur = code, sp = 0;
        if (code >= next) { if (code > next) return false; stack[sp++] = first; cur = prev; }
        while (cur >= clear) { if (sp >= 4096) return false; stack[sp++] = suffix[cur]; cur = prefix[cur]; }
        first = (uint8_t)cur; stack[sp++] = first;
        while (sp) out.push_back(stack[--sp]);
        if (next < 4096) { prefix[next] = (uint16_t)prev; suffix[next] = first; next++; if (next == (1 << codeSize) && codeSize < 12) codeSize++; }
        prev = code;
    }
    return !out.empty();
}
static bool gifDecode(const std::vector<uint8_t>& d, GifImg& g, int maxFrames = 200, size_t maxBytes = 96u << 20) {
    g = GifImg();
    if (d.size() < 14 || memcmp(d.data(), "GIF8", 4) || (d[4] != '7' && d[4] != '9') || d[5] != 'a') return false;
    auto le = [&](size_t p) { return (int)d[p] | ((int)d[p + 1] << 8); };
    g.w = le(6); g.h = le(8); const int fl = d[10]; size_t p = 13;
    if (g.w < 1 || g.h < 1 || g.w > 4096 || g.h > 4096) return false;
    std::vector<uint8_t> gct; if (fl & 0x80) { const size_t n = (size_t)3 * (2u << (fl & 7)); if (p + n > d.size()) return false; gct.assign(d.begin() + p, d.begin() + p + n); p += n; }
    const size_t fb = (size_t)g.w * g.h * 4; std::vector<uint8_t> canvas(fb, 0), saved;
    int gDelay = 100, gDisp = 0, gTrans = -1;
    while (p < d.size()) {
        const uint8_t b = d[p++];
        if (b == 0x3B) break;
        if (b == 0x21) {   // extensao
            if (p >= d.size()) break; const uint8_t lab = d[p++];
            if (lab == 0xF9 && p + 5 < d.size() && d[p] >= 4) { const int pk = d[p + 1]; gDisp = (pk >> 2) & 7; gDelay = le(p + 2) * 10; gTrans = (pk & 1) ? d[p + 4] : -1; }
            while (p < d.size() && d[p]) p += (size_t)d[p] + 1; p++; continue;
        }
        if (b != 0x2C) return !g.fr.empty();
        if (p + 9 > d.size()) break;
        const int ix = le(p), iy = le(p + 2), iw = le(p + 4), ih = le(p + 6), pk = d[p + 8]; p += 9;
        std::vector<uint8_t> lct; const std::vector<uint8_t>* pal = &gct;
        if (pk & 0x80) { const size_t n = (size_t)3 * (2u << (pk & 7)); if (p + n > d.size()) break; lct.assign(d.begin() + p, d.begin() + p + n); p += n; pal = &lct; }
        if (p >= d.size()) break; const int minCode = d[p++];
        std::vector<uint8_t> comp; while (p < d.size() && d[p]) { const size_t n = d[p]; if (p + 1 + n > d.size()) return !g.fr.empty(); comp.insert(comp.end(), d.begin() + p + 1, d.begin() + p + 1 + n); p += n + 1; } p++;
        if (iw < 1 || ih < 1 || ix + iw > g.w || iy + ih > g.h) { if (!g.fr.empty()) break; return false; }
        std::vector<uint8_t> idx; if (!gifLzw(comp, minCode, idx, (size_t)iw * ih)) { if (!g.fr.empty()) break; return false; }
        idx.resize((size_t)iw * ih, 0);
        if (gDisp == 3) saved = canvas;
        std::vector<int> rowMap(ih); { int r = 0; if (pk & 0x40) { static const int st[4] = {0, 4, 2, 1}, sp2[4] = {8, 8, 4, 2}; for (int ps = 0; ps < 4; ps++) for (int y = st[ps]; y < ih; y += sp2[ps]) rowMap[r++] = y; } else for (int y = 0; y < ih; y++) rowMap[y] = y; }
        for (int r = 0; r < ih; r++) for (int x = 0; x < iw; x++) {
            const int c = idx[(size_t)r * iw + x]; if (c == gTrans) continue; if ((size_t)c * 3 + 2 >= pal->size()) continue;
            uint8_t* o = &canvas[((size_t)(iy + rowMap[r]) * g.w + ix + x) * 4]; o[0] = (*pal)[c * 3]; o[1] = (*pal)[c * 3 + 1]; o[2] = (*pal)[c * 3 + 2]; o[3] = 255;
        }
        if ((int)g.fr.size() >= maxFrames || (g.fr.size() + 1) * fb > maxBytes) break;
        g.fr.push_back(canvas); g.delay.push_back(gDelay <= 10 ? 100 : gDelay);   // 0/1 cs = 100 ms (como os navegadores)
        if (gDisp == 2) { for (int y = 0; y < ih; y++) memset(&canvas[((size_t)(iy + y) * g.w + ix) * 4], 0, (size_t)iw * 4); }
        else if (gDisp == 3 && saved.size() == fb) canvas = saved;
        gDisp = 0; gTrans = -1; gDelay = 100;
    }
    return !g.fr.empty();
}
static bool gifLoadFile(const char* path, GifImg& g) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    std::vector<uint8_t> d; uint8_t buf[8192]; size_t n; while ((n = fread(buf, 1, sizeof buf, f)) > 0) { d.insert(d.end(), buf, buf + n); if (d.size() > (48u << 20)) break; }
    fclose(f); return gifDecode(d, g);
}
