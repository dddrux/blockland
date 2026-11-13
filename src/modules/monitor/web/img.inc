// ---------- PIE Web: imagens (PNG/JPEG/data:) e base64 ----------
namespace pw {
struct Img { int state = 0; int w = 0, h = 0; std::vector<uint32_t> px; };   // state: 0 pedir, 1 carregando, 2 pronto, 3 falhou; px = RGBA (R no byte baixo)
static std::string b64decode(const std::string& s) {
    static int8_t T[256]; static bool init = false; if (!init) { memset(T, -1, sizeof T); const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; for (int i = 0; i < 64; i++) T[(uint8_t)A[i]] = (int8_t)i; T[(uint8_t)'-'] = 62; T[(uint8_t)'_'] = 63; init = true; }
    std::string o; o.reserve(s.size() * 3 / 4); uint32_t v = 0; int n = 0; for (unsigned char c : s) { if (T[c] < 0) continue; v = (v << 6) | (uint32_t)T[c]; n += 6; if (n >= 8) { n -= 8; o += (char)((v >> n) & 0xFF); } } return o;
}
// GIF: primeiro quadro (LZW, entrelacado, transparencia)
static bool gifDecode(const std::string& d, std::vector<uint32_t>& out, int& W, int& H) {
    const uint8_t* p = (const uint8_t*)d.data(); const size_t n = d.size(); if (n < 14 || memcmp(p, "GIF8", 4) != 0) return false; W = p[6] | (p[7] << 8); H = p[8] | (p[9] << 8); if (W <= 0 || H <= 0 || (size_t)W * H > 16000000) return false;
    size_t i = 13; std::vector<uint32_t> gpal; auto readPal = [&](int cnt, std::vector<uint32_t>& pal) { if (i + (size_t)cnt * 3 > n) return false; pal.resize((size_t)cnt); for (int k = 0; k < cnt; k++, i += 3) pal[k] = 0xFF000000u | (p[i + 2] << 16) | (p[i + 1] << 8) | p[i]; return true; };
    if (p[10] & 0x80) { if (!readPal(2 << (p[10] & 7), gpal)) return false; } int transp = -1; out.assign((size_t)W * H, 0);
    while (i < n) { const uint8_t b = p[i++];
        if (b == 0x21) { if (i >= n) return false; const uint8_t lab = p[i++]; if (lab == 0xF9 && i + 5 < n && p[i] >= 4) { if (p[i + 1] & 1) transp = p[i + 4]; } while (i < n && p[i]) i += (size_t)p[i] + 1; i++; }
        else if (b == 0x2C) { if (i + 9 > n) return false; const int ix = p[i] | (p[i + 1] << 8), iy = p[i + 2] | (p[i + 3] << 8), iw = p[i + 4] | (p[i + 5] << 8), ih = p[i + 6] | (p[i + 7] << 8); const uint8_t fl = p[i + 8]; i += 9; std::vector<uint32_t> lp; const std::vector<uint32_t>* pal = &gpal; if (fl & 0x80) { if (!readPal(2 << (fl & 7), lp)) return false; pal = &lp; }
            if (i >= n || iw <= 0 || ih <= 0) return false; const int minc = p[i++]; if (minc < 1 || minc > 11) return false; std::string data; while (i < n && p[i]) { const size_t l = p[i]; if (i + 1 + l > n) break; data.append((const char*)p + i + 1, l); i += l + 1; }
            std::vector<uint8_t> idx; idx.reserve((size_t)iw * ih); const int clear = 1 << minc, eoi = clear + 1; std::vector<uint16_t> prefix(4096); std::vector<uint8_t> suffix(4096), stack(4097); int csz = minc + 1, next = eoi + 1, prev = -1; uint32_t acc = 0; int nb = 0; size_t dp = 0; uint8_t first = 0;
            while (idx.size() < (size_t)iw * ih) { while (nb < csz && dp < data.size()) { acc |= (uint32_t)(uint8_t)data[dp++] << nb; nb += 8; } if (nb < csz) break; int code = acc & ((1 << csz) - 1); acc >>= csz; nb -= csz;
                if (code == clear) { csz = minc + 1; next = eoi + 1; prev = -1; continue; } if (code == eoi) break; int sp = 0; int cur = code;
                if (prev < 0) { if (code >= clear) break; idx.push_back((uint8_t)code); first = (uint8_t)code; prev = code; continue; }
                if (code >= next) { if (code > next) break; stack[sp++] = first; cur = prev; } while (cur >= clear) { if (sp >= 4096) return false; stack[sp++] = suffix[cur]; cur = prefix[cur]; } first = (uint8_t)cur; stack[sp++] = first; while (sp) idx.push_back(stack[--sp]);
                if (next < 4096) { prefix[next] = (uint16_t)prev; suffix[next] = first; next++; if (next == (1 << csz) && csz < 12) csz++; } prev = code; }
            const bool il = fl & 0x40; size_t k = 0; int row = 0, pass = 0; for (int r = 0; r < ih && k < idx.size(); r++) { int y; if (!il) y = r; else { static const int st[4] = {0, 4, 2, 1}, sp2[4] = {8, 8, 4, 2}; y = row; row += sp2[pass]; if (row >= ih && pass < 3) { pass++; row = st[pass]; } }
                for (int x = 0; x < iw && k < idx.size(); x++, k++) { const int px = ix + x, py = iy + y; if (px >= W || py >= H || py < 0) continue; const int c = idx[k]; if (c == transp || c >= (int)pal->size()) continue; const uint32_t q = (*pal)[c]; out[(size_t)py * W + px] = q; } }
            if (il) { /* linha inicial do passe 0 e 0 */ } return true; }
        else break; }
    return false;
}
static bool decodeImageBytes(const std::string& d, Img& im) {
    im.px.clear(); im.state = 3; if (d.size() < 8) return false; std::vector<uint32_t> px; int w = 0, h = 0;
    if ((uint8_t)d[0] == 0x89 && d[1] == 'P') { std::vector<uint8_t> src(d.begin(), d.end()), rgba; if (!loadPNGMem(src, rgba, w, h) || w <= 0 || h <= 0 || (size_t)w * h > 16000000) return false; px.resize((size_t)w * h); memcpy(px.data(), rgba.data(), (size_t)w * h * 4); }
    else if ((uint8_t)d[0] == 0xFF && (uint8_t)d[1] == 0xD8) { if (!jpegDecode((const uint8_t*)d.data(), d.size(), px, w, h)) return false; }
    else if (d.compare(0, 3, "GIF") == 0) { if (!gifDecode(d, px, w, h)) return false; }
    else return false;
    if ((size_t)w * h > 2000000) { const int f = (int)ceil(sqrt((double)w * h / 2000000.0)); const int nw = w / f, nh = h / f; if (nw < 1 || nh < 1) return false; std::vector<uint32_t> o((size_t)nw * nh); for (int y = 0; y < nh; y++) for (int x = 0; x < nw; x++) o[(size_t)y * nw + x] = px[(size_t)y * f * w + (size_t)x * f]; px.swap(o); w = nw; h = nh; }
    im.px.swap(px); im.w = w; im.h = h; im.state = 2; return true;
}
static void decodeDataImg(const std::string& url, Img& im) {
    const size_t c = url.find(','); im.state = 3; if (c == std::string::npos) return; const std::string head = url.substr(0, c); if (head.find("base64") == std::string::npos) return; decodeImageBytes(b64decode(url.substr(c + 1)), im);
}
}   // namespace pw
