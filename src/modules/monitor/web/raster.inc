// ---------- PIE Web: rasterizador por software (retangulos, texto 5x7, imagens) ----------
namespace pw {
struct Surf { int w = 0, h = 0; std::vector<uint32_t> px; };
static inline uint32_t packRGBA(uint32_t argb, uint32_t a) { return (a << 24) | (((argb >> 0) & 255) << 16) | (((argb >> 8) & 255) << 8) | ((argb >> 16) & 255); }
static inline void blendPx(uint32_t& d, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {   // d: RGBA (R baixo); origem r,g,b + alfa
    if (a >= 255) { d = 0xFF000000u | (b << 16) | (g << 8) | r; return; } if (!a) return; const uint32_t dr = d & 255, dg = (d >> 8) & 255, db = (d >> 16) & 255, ia = 255 - a;
    d = 0xFF000000u | (((b * a + db * ia) / 255) << 16) | (((g * a + dg * ia) / 255) << 8) | ((r * a + dr * ia) / 255);
}
static void fillRect(Surf& s, float fx0, float fy0, float fx1, float fy1, uint32_t argb) {
    const uint32_t a = argb >> 24; if (!a) return; int x0 = (int)floorf(fx0 + .5f), y0 = (int)floorf(fy0 + .5f), x1 = (int)floorf(fx1 + .5f), y1 = (int)floorf(fy1 + .5f); if (x1 <= x0 && fx1 > fx0) x1 = x0 + 1; if (y1 <= y0 && fy1 > fy0) y1 = y0 + 1;
    x0 = std::max(x0, 0); y0 = std::max(y0, 0); x1 = std::min(x1, s.w); y1 = std::min(y1, s.h); const uint32_t r = (argb >> 16) & 255, g = (argb >> 8) & 255, b = argb & 255;
    for (int y = y0; y < y1; y++) { uint32_t* row = &s.px[(size_t)y * s.w]; if (a >= 255) { const uint32_t v = 0xFF000000u | (b << 16) | (g << 8) | r; for (int x = x0; x < x1; x++) row[x] = v; } else for (int x = x0; x < x1; x++) blendPx(row[x], r, g, b, a); }
}
static void drawGlyph(Surf& s, int cp, float ox, float oy, int sc, uint32_t col, bool italic) {
    int ch = cp; char kind = 0; if (cp < 32 || cp > 126) { ch = '?'; for (const AccM& a : ACC) if (a.cp == cp) { ch = a.base; kind = a.kind; } } const uint8_t* g = F57[ch - 32]; const bool up = ch >= 'A' && ch <= 'Z';
    auto R = [&](int cx, int cy, int w, int h) { float sh = italic ? (float)((6 - cy - h / 2) * sc) / 4.f : 0.f; if (sh < -sc) sh = (float)-sc; fillRect(s, ox + cx * sc + sh, oy + cy * sc, ox + (cx + w) * sc + sh, oy + (cy + h) * sc, col); };
    for (int c = 0; c < 5; c++) { int bits = g[c]; if (kind && ch == 'i') bits &= ~3; for (int r = 0; r < 8;) { if ((bits >> r) & 1) { int r2 = r; while (r2 < 8 && ((bits >> r2) & 1)) r2++; R(c, r, 1, r2 - r); r = r2; } else r++; } }
    const int t = up ? -2 : 0; switch (kind) { case 'a': R(3, t, 1, 1); R(2, t + 1, 1, 1); break; case 'g': R(1, t, 1, 1); R(2, t + 1, 1, 1); break; case 'c': R(2, t, 1, 1); R(1, t + 1, 1, 1); R(3, t + 1, 1, 1); break; case 't': R(1, t + 1, 1, 1); R(2, t, 1, 1); R(3, t + 1, 1, 1); R(4, t, 1, 1); break; case 'z': R(2, 7, 1, 1); R(1, 8, 1, 1); break; default: break; }
}
static float drawText(Surf& s, float x, float y, const std::string& t, int sc, uint32_t col, bool bold, bool italic) {
    float cx = x; for (const char* p = t.c_str(); *p;) { const int cp = u8next(p); int f, a; glyphMetrics(cp, f, a); if (cp != ' ' && cx - f * sc < s.w + 40 && cx + 8 * sc > -40) { drawGlyph(s, cp, cx - f * sc, y, sc, col, italic); if (bold) drawGlyph(s, cp, cx - f * sc + std::max(1, sc / 2), y, sc, col, italic); } cx += (float)(a * sc); } return cx - x;
}
static void drawImage(Surf& s, const Img& im, float dx, float dy, float dw, float dh) {
    if (im.w <= 0 || dw < 1 || dh < 1) return; const int x0 = std::max(0, (int)floorf(dx + .5f)), y0 = std::max(0, (int)floorf(dy + .5f)), x1 = std::min(s.w, (int)floorf(dx + dw + .5f)), y1 = std::min(s.h, (int)floorf(dy + dh + .5f)); const float fx = (float)im.w / dw, fy = (float)im.h / dh;
    const int bx = std::max(1, std::min(4, (int)fx)), by = std::max(1, std::min(4, (int)fy));
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) { float sx = ((float)x + .5f - dx) * fx, sy = ((float)y + .5f - dy) * fy; unsigned r = 0, g = 0, b = 0, a = 0; int n = 0;
        for (int j = 0; j < by; j++) for (int i = 0; i < bx; i++) { int px = (int)(sx + ((float)i - (bx - 1) * .5f) * fx / bx), py = (int)(sy + ((float)j - (by - 1) * .5f) * fy / by); px = std::max(0, std::min(im.w - 1, px)); py = std::max(0, std::min(im.h - 1, py)); const uint32_t p = im.px[(size_t)py * im.w + px]; const unsigned pa = p >> 24; r += (p & 255) * pa; g += ((p >> 8) & 255) * pa; b += ((p >> 16) & 255) * pa; a += pa; n++; }
        if (!a) continue; blendPx(s.px[(size_t)y * s.w + x], r / a, g / a, b / a, a / n); }
}
static void renderPage(const Eng& e, float scrollY, Surf& s) {
    const uint32_t c = e.canvas; s.px.assign((size_t)s.w * s.h, 0xFF000000u | (((c) & 255) << 16) | (((c >> 8) & 255) << 8) | ((c >> 16) & 255));
    for (const DOp& o : e.ops) { const float y = o.y - scrollY; if (o.t == 0) { if (y > s.h || y + o.h < 0) continue; fillRect(s, o.x, y, o.x + o.w, y + o.h, o.col); }
        else if (o.t == 1) { if (y > s.h || y + 10 * o.scale < -2 * o.scale) continue; drawText(s, o.x, y, o.s, o.scale, o.col | 0xFF000000u, o.bold, o.italic); if (o.strike) fillRect(s, o.x, y + 4 * o.scale, o.x + Eng::tw(o.s, o.scale), y + 4 * o.scale + std::max(1, o.scale / 2), o.col | 0xFF000000u); if (o.ul) fillRect(s, o.x, y + 8 * o.scale, o.x + Eng::tw(o.s, o.scale), y + 8 * o.scale + std::max(1, o.scale / 2), o.col | 0xFF000000u); }
        else if (o.t == 2) { if (y > s.h || y + o.h < 0 || o.img < 0 || o.img >= (int)e.imgs.size()) continue; drawImage(s, e.imgs[o.img], o.x, y, o.w, o.h); } }
}
}   // namespace pw
