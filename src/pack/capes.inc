// ================= patchCapa.1/.2: capas (como no Minecraft) =================
// Textura de capa = layout 64x32 do MC guardado em 128x64 (cada pixel do MC = 2x2). Face de fora = (2,2) 20x32, de dentro = (24,2).
// Fontes: capas prontas (geradas no codigo, algumas animadas), PNG/GIF em assets/capes/ (GIF = animada). Imagem comum (foto/arte)
// vira a face de fora; PNG/GIF no layout do MC (64x32 ou multiplo) e usado como esta. Rede: ver NP_CAPE em net.inc.
static const int CAPE_W = 128, CAPE_H = 64, CAPE_FB = CAPE_W * CAPE_H * 4, CAPE_MAXF = 16, CAPE_NBI = 8;
struct CapeAnim {
    std::vector<std::vector<uint8_t>> fr;   // quadros RGBA 128x64
    std::vector<uint16_t> ms;               // duracao de cada quadro (ms)
    std::vector<GLuint> tex; uint32_t total = 0;
    void freeGL() { for (GLuint t : tex) if (t) glDeleteTextures(1, &t); tex.assign(fr.size(), 0); }
    void clear() { freeGL(); fr.clear(); ms.clear(); tex.clear(); total = 0; }
    void finish() { total = 0; for (uint16_t m : ms) total += m; if (!total) total = 1; tex.assign(fr.size(), 0); }
    int frameAt(uint32_t t) const { if (fr.size() < 2) return 0; uint32_t r = t % total; for (size_t i = 0; i < ms.size(); i++) { if (r < ms[i]) return (int)i; r -= ms[i]; } return 0; }
    GLuint texAt(uint32_t t) {
        if (fr.empty()) return 0; const int i = frameAt(t); if (tex.size() != fr.size()) tex.assign(fr.size(), 0);
        if (!tex[i]) { glGenTextures(1, &tex[i]); glBindTexture(GL_TEXTURE_2D, tex[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAPE_W, CAPE_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, fr[i].data()); }
        return tex[i];
    }
};
// ---- montagem do layout a partir de 2 faces 20x32 (RGBA) + cor da borda ----
static void capeFill(std::vector<uint8_t>& t, int x, int y, int w, int h, const uint8_t* c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) { uint8_t* p = &t[((size_t)(y + j) * CAPE_W + x + i) * 4]; p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; p[3] = 255; } }
static void capeLayout(std::vector<uint8_t>& t, const std::vector<uint8_t>& outer, const std::vector<uint8_t>& inner, const uint8_t* edge) {
    t.assign(CAPE_FB, 0);
    capeFill(t, 2, 0, 20, 2, edge); capeFill(t, 22, 0, 20, 2, edge); capeFill(t, 0, 2, 2, 32, edge); capeFill(t, 22, 2, 2, 32, edge);
    for (int j = 0; j < 32; j++) for (int i = 0; i < 20; i++) {
        memcpy(&t[((size_t)(2 + j) * CAPE_W + 2 + i) * 4], &outer[((size_t)j * 20 + i) * 4], 4);
        memcpy(&t[((size_t)(2 + j) * CAPE_W + 24 + i) * 4], &inner[((size_t)j * 20 + i) * 4], 4);
    }
}
static void capeEdgeInner(const std::vector<uint8_t>& outer, std::vector<uint8_t>& inner, uint8_t* edge) {   // dentro = fora escurecido; borda = media escurecida
    inner.assign(20 * 32 * 4, 255); long s[3] = {0, 0, 0};
    for (int i = 0; i < 20 * 32; i++) for (int c = 0; c < 3; c++) { inner[i * 4 + c] = (uint8_t)(outer[i * 4 + c] * 55 / 100); s[c] += outer[i * 4 + c]; }
    for (int c = 0; c < 3; c++) edge[c] = (uint8_t)(s[c] / (20 * 32) * 75 / 100);
}
// ---- capas prontas ----
struct CapeImg { std::vector<uint8_t> p; CapeImg() : p(20 * 32 * 4, 255) {}
    void px(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= 20 || y >= 32) return; uint8_t* q = &p[((size_t)y * 20 + x) * 4]; q[0] = (uint8_t)r; q[1] = (uint8_t)g; q[2] = (uint8_t)b; q[3] = 255; }
    void fill(int r, int g, int b) { for (int y = 0; y < 32; y++) for (int x = 0; x < 20; x++) px(x, y, r, g, b); }
    void border(int x0, int y0, int x1, int y1, int r, int g, int b) { for (int x = x0; x <= x1; x++) { px(x, y0, r, g, b); px(x, y1, r, g, b); } for (int y = y0; y <= y1; y++) { px(x0, y, r, g, b); px(x1, y, r, g, b); } }
    // pinta onde cov(x,y) (coordenadas continuas do pixel) cobre >= 50% (4x4 amostras)
    template <class F> void shape(F inside, int r, int g, int b) { for (int y = 0; y < 32; y++) for (int x = 0; x < 20; x++) { int n = 0; for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) if (inside(x + (i + .5f) / 4.f, y + (j + .5f) / 4.f)) n++; if (n >= 8) px(x, y, r, g, b); } }
};
static float capeSeg(float px, float py, float ax, float ay, float bx, float by) {   // distancia do ponto ao segmento
    const float vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay, t = std::max(0.f, std::min(1.f, (wx * vx + wy * vy) / (vx * vx + vy * vy)));
    const float dx = px - (ax + t * vx), dy = py - (ay + t * vy); return sqrtf(dx * dx + dy * dy);
}
static void capeHammerSickle(CapeImg& im, int r, int g, int b) {   // estrela de pixels + foice (arco em U com cabo) e martelo cruzados (desenho conferido a 20x32)
    static const char* const STAR[7] = {"...X...", "..XXX..", "XXXXXXX", ".XXXXX.", "..XXX..", ".XX.XX.", ".X...X."};
    for (int j = 0; j < 7; j++) for (int i = 0; i < 7; i++) if (STAR[j][i] == 'X') im.px(6 + i, 3 + j, r, g, b);
    typedef std::pair<float, float> Pt; std::vector<Pt> arc;
    for (int k = 0; k <= 24; k++) { const float a = (200.f + (-30.f - 200.f) * k / 24.f) * .01745329f; arc.push_back(Pt(10.5f + 6.6f * cosf(a), 20.5f + 6.6f * sinf(a))); }
    const std::vector<Pt> handle = {Pt(4.3f, 19.2f), Pt(2.6f, 12.6f)}, ham = {Pt(15.5f, 30.f), Pt(7.5f, 15.5f)}, head = {Pt(3.9f, 17.6f), Pt(11.1f, 13.4f)};
    auto near = [](float x, float y, const std::vector<Pt>& v, float th) { for (size_t i = 0; i + 1 < v.size(); i++) if (capeSeg(x, y, v[i].first, v[i].second, v[i + 1].first, v[i + 1].second) < th) return true; return false; };
    for (int y = 11; y < 31; y++) for (int x = 2; x < 18; x++) { const float px = x + .5f, py = y + .5f;
        if (near(px, py, arc, .9f) || near(px, py, handle, .9f) || near(px, py, ham, .75f) || near(px, py, head, 1.35f)) im.px(x, y, r, g, b); }
}
static void capeBuiltin(int id, CapeAnim& a) {
    a.clear(); uint8_t edge[3]; std::vector<uint8_t> t, in, ou; auto push = [&](int ms) { a.fr.push_back(t); a.ms.push_back((uint16_t)ms); };
    auto simple = [&](const CapeImg& im) { ou = im.p; capeEdgeInner(ou, in, edge); capeLayout(t, ou, in, edge); push(1000); };
    if (id == 1) { CapeImg im; im.fill(190, 22, 28); im.border(0, 0, 19, 31, 130, 10, 16); im.border(1, 1, 18, 30, 215, 60, 60); simple(im); }
    else if (id == 2) {   // comunismo: campo vermelho, galao dourado, estrela + foice e martelo
        CapeImg im; im.fill(205, 16, 16); im.border(0, 0, 19, 31, 150, 8, 8); im.border(1, 1, 18, 30, 255, 205, 0);
        capeHammerSickle(im, 255, 215, 0); simple(im); }
    else if (id == 3) {   // Brasil: verde, losango amarelo, esfera azul com faixa branca
        CapeImg im; im.fill(0, 156, 59);
        im.shape([](float x, float y) { return fabsf(x - 10.f) / 8.6f + fabsf(y - 16.f) / 13.f <= 1.f; }, 255, 223, 0);
        im.shape([](float x, float y) { const float dx = x - 10.f, dy = y - 16.f; return dx * dx + dy * dy <= 30.f; }, 0, 39, 118);
        im.shape([](float x, float y) { const float dx = x - 10.f, dy = y - 16.f; return dx * dx + dy * dy <= 30.f && fabsf(y - (14.8f + .045f * (x - 10.f) * (x - 10.f))) < .8f; }, 255, 255, 255); simple(im); }
    else if (id == 4) { CapeImg im; im.fill(30, 64, 176); im.border(0, 0, 19, 31, 18, 40, 120); for (int x = 2; x < 18; x++) im.px(x, 3, 240, 200, 60); simple(im); }
    else if (id == 5) { CapeImg im; im.fill(36, 142, 52); im.border(0, 0, 19, 31, 20, 90, 32); simple(im); }
    else if (id == 6) { CapeImg im; im.fill(26, 26, 32); im.border(0, 0, 19, 31, 10, 10, 14); im.border(1, 1, 18, 30, 80, 40, 130); simple(im); }
    else if (id == 7) {   // arco-iris animado: faixas que descem
        for (int f = 0; f < 8; f++) { CapeImg im; for (int y = 0; y < 32; y++) { const float h = fmodf((y * 11.25f + f * 45.f), 360.f) / 60.f; const int s = (int)h; const float fr = h - s; const int v = 255, q = (int)(255 * (1 - fr)), tt = (int)(255 * fr);
                int r, g, b; switch (s % 6) { case 0: r = v; g = tt; b = 0; break; case 1: r = q; g = v; b = 0; break; case 2: r = 0; g = v; b = tt; break; case 3: r = 0; g = q; b = v; break; case 4: r = tt; g = 0; b = v; break; default: r = v; g = 0; b = q; }
                for (int x = 0; x < 20; x++) im.px(x, y, r, g, b); }
            ou = im.p; capeEdgeInner(ou, in, edge); capeLayout(t, ou, in, edge); push(110); } }
    else if (id == 8) {   // chamas: sobe com cintilacao (ruido por hash)
        for (int f = 0; f < 6; f++) { CapeImg im; for (int y = 0; y < 32; y++) for (int x = 0; x < 20; x++) {
                const unsigned hsh = (unsigned)(x * 73856093u ^ (y + f * 5) * 19349663u ^ 0x9E3779B9u) >> 7 & 31u; const float k = (y + (float)hsh * .35f - f * 1.3f) / 34.f, c = std::max(0.f, std::min(1.f, k));
                im.px(x, y, (int)(255 - 70 * c), (int)(235 - 200 * c), (int)(60 - 55 * c)); }
            ou = im.p; capeEdgeInner(ou, in, edge); capeLayout(t, ou, in, edge); push(120); } }
    a.finish();
}
static const char* capeBuiltinName(int id) { static const char* const N[CAPE_NBI + 1] = {"", "Vermelha", "Comunismo", "Brasil", "Azul real", "Verde", "Preta", "Arco-íris (anim.)", "Chamas (anim.)"}; return id >= 1 && id <= CAPE_NBI ? N[id] : ""; }
// ---- imagem -> capa ----
static void capeArea(const uint8_t* s, int sw, int sh, float x0, float y0, float x1, float y1, uint8_t* o) {   // media por area (pre-multiplicada pelo alfa)
    double a = 0, r = 0, g = 0, b = 0, wt = 0;
    for (int y = std::max(0, (int)floorf(y0)); y < std::min(sh, (int)ceilf(y1)); y++) for (int x = std::max(0, (int)floorf(x0)); x < std::min(sw, (int)ceilf(x1)); x++) {
        const double w = (double)(std::min((float)x + 1, x1) - std::max((float)x, x0)) * (std::min((float)y + 1, y1) - std::max((float)y, y0)); if (w <= 0) continue;
        const uint8_t* p = &s[((size_t)y * sw + x) * 4]; a += w * p[3]; r += w * p[0] * p[3]; g += w * p[1] * p[3]; b += w * p[2] * p[3]; wt += w; }
    if (wt <= 0 || a <= 0) { o[0] = o[1] = o[2] = 0; o[3] = 0; return; }
    o[0] = (uint8_t)(r / a); o[1] = (uint8_t)(g / a); o[2] = (uint8_t)(b / a); o[3] = (uint8_t)(a / wt);
}
static bool capeIsLayout(int w, int h) { return h * 2 == w && w >= 64 && w <= 2048 && w % 64 == 0; }
static void capeFromRGBA(const uint8_t* s, int sw, int sh, std::vector<uint8_t>& t) {   // 1 quadro
    if (capeIsLayout(sw, sh)) { t.assign(CAPE_FB, 0);
        for (int y = 0; y < CAPE_H; y++) for (int x = 0; x < CAPE_W; x++) capeArea(s, sw, sh, (float)x * sw / CAPE_W, (float)y * sh / CAPE_H, (float)(x + 1) * sw / CAPE_W, (float)(y + 1) * sh / CAPE_H, &t[((size_t)y * CAPE_W + x) * 4]);
        return; }
    float cw = (float)sw, ch = (float)sh; if (cw * 32.f > ch * 20.f) cw = ch * 20.f / 32.f; else ch = cw * 32.f / 20.f;   // recorte central na proporcao da face (10x16)
    const float ox = (sw - cw) / 2, oy = (sh - ch) / 2; std::vector<uint8_t> ou(20 * 32 * 4), in; uint8_t edge[3];
    for (int y = 0; y < 32; y++) for (int x = 0; x < 20; x++) { uint8_t* o = &ou[((size_t)y * 20 + x) * 4]; capeArea(s, sw, sh, ox + x * cw / 20, oy + y * ch / 32, ox + (x + 1) * cw / 20, oy + (y + 1) * ch / 32, o);
        const int al = o[3]; for (int c = 0; c < 3; c++) o[c] = (uint8_t)((o[c] * al + 40 * (255 - al)) / 255); o[3] = 255; }   // transparencia vira cinza escuro
    capeEdgeInner(ou, in, edge); capeLayout(t, ou, in, edge);
}
static bool capeFromFile(const std::string& path, CapeAnim& a) {
    a.clear(); std::string ext = path.size() > 4 ? path.substr(path.size() - 4) : ""; for (char& c : ext) c = (char)tolower((unsigned char)c);
    if (ext == ".gif") {
        GifImg g; if (!gifLoadFile(path.c_str(), g)) return false;
        uint32_t T = 0; for (int d : g.delay) T += (uint32_t)d; const int n = std::min<int>(CAPE_MAXF, (int)g.fr.size());
        if (n == 1) { std::vector<uint8_t> t; capeFromRGBA(g.fr[0].data(), g.w, g.h, t); a.fr.push_back(t); a.ms.push_back(1000); }
        else for (int k = 0; k < n; k++) {   // reamostra no tempo: n quadros igualmente espacados (no maximo 16)
            const uint32_t at = (uint32_t)((uint64_t)T * (2 * k + 1) / (2 * n)); uint32_t acc = 0; size_t fi = 0; for (; fi + 1 < g.fr.size(); fi++) { if (at < acc + (uint32_t)g.delay[fi]) break; acc += (uint32_t)g.delay[fi]; }
            std::vector<uint8_t> t; capeFromRGBA(g.fr[fi].data(), g.w, g.h, t); a.fr.push_back(t); a.ms.push_back((uint16_t)std::max<uint32_t>(40, std::min<uint32_t>(5000, T / n))); }
    } else {
        std::vector<uint8_t> px; int w = 0, h = 0; if (!loadPNG(path.c_str(), px, w, h) || w < 2 || h < 2) return false;
        std::vector<uint8_t> t; capeFromRGBA(px.data(), w, h, t); a.fr.push_back(t); a.ms.push_back(1000);
    }
    a.finish(); return !a.fr.empty();
}
// ---- compactacao para a rede: RGB332 + PackBits (16 quadros no maximo = ~130 KB no pior caso, bem menos em arte chapada) ----
static void capeRle(const std::vector<uint8_t>& in, std::vector<uint8_t>& out) {
    out.clear(); size_t i = 0;
    while (i < in.size()) {
        size_t run = 1; while (i + run < in.size() && in[i + run] == in[i] && run < 129) run++;
        if (run >= 2) { out.push_back((uint8_t)(128 + run - 2)); out.push_back(in[i]); i += run; continue; }
        size_t j = i; while (j < in.size() && j - i < 128 && !(j + 1 < in.size() && in[j] == in[j + 1])) j++;
        if (j == i) j = i + 1; out.push_back((uint8_t)(j - i - 1)); out.insert(out.end(), in.begin() + i, in.begin() + j); i = j;
    }
}
static bool capeUnRle(const uint8_t* d, size_t n, std::vector<uint8_t>& out, size_t maxOut) {
    out.clear(); size_t i = 0;
    while (i < n) { const int c = d[i++];
        if (c < 128) { const size_t k = (size_t)c + 1; if (i + k > n || out.size() + k > maxOut) return false; out.insert(out.end(), d + i, d + i + k); i += k; }
        else { const size_t k = (size_t)c - 128 + 2; if (i >= n || out.size() + k > maxOut) return false; out.insert(out.end(), k, d[i++]); } }
    return true;
}
static void capePack(const CapeAnim& a, std::vector<uint8_t>& enc) {
    std::vector<uint8_t> raw; const int nf = std::min<int>(CAPE_MAXF, (int)a.fr.size()); raw.push_back((uint8_t)nf);
    for (int i = 0; i < nf; i++) { raw.push_back((uint8_t)(a.ms[i] & 255)); raw.push_back((uint8_t)(a.ms[i] >> 8)); }
    for (int i = 0; i < nf; i++) for (int k = 0; k < CAPE_W * CAPE_H; k++) { const uint8_t* p = &a.fr[i][(size_t)k * 4]; raw.push_back((uint8_t)(((p[0] * 7 + 127) / 255) << 5 | ((p[1] * 7 + 127) / 255) << 2 | ((p[2] * 3 + 127) / 255))); }
    capeRle(raw, enc);
}
static bool capeUnpack(const uint8_t* d, size_t n, CapeAnim& a) {
    std::vector<uint8_t> raw; if (!capeUnRle(d, n, raw, (size_t)1 + CAPE_MAXF * 2 + (size_t)CAPE_MAXF * CAPE_W * CAPE_H) || raw.empty()) return false;
    const int nf = raw[0]; if (nf < 1 || nf > CAPE_MAXF || raw.size() != (size_t)1 + nf * 2 + (size_t)nf * CAPE_W * CAPE_H) return false;
    a.clear(); for (int i = 0; i < nf; i++) { a.ms.push_back((uint16_t)std::max(30, std::min(5000, raw[1 + i * 2] | raw[2 + i * 2] << 8))); }
    const uint8_t* q = &raw[1 + nf * 2];
    for (int i = 0; i < nf; i++) { std::vector<uint8_t> t((size_t)CAPE_FB); for (int k = 0; k < CAPE_W * CAPE_H; k++) { const int v = q[(size_t)i * CAPE_W * CAPE_H + k]; t[k * 4] = (uint8_t)((v >> 5) * 255 / 7); t[k * 4 + 1] = (uint8_t)(((v >> 2) & 7) * 255 / 7); t[k * 4 + 2] = (uint8_t)((v & 3) * 255 / 3); t[k * 4 + 3] = 255; } a.fr.push_back(std::move(t)); }
    a.finish(); return true;
}
// ---- selecao local: 0 nenhuma | 1..8 prontas | 9.. arquivos de assets/capes (PNG/GIF) ----
static std::vector<std::string> gCapeFiles; static CapeAnim gCapeMine; static int gCapeSelCur = 0; static bool gCapeNetDirty = false; static uint8_t gCapeGen = 0;
static std::vector<uint8_t> gCapeMyEnc;   // forma de rede da capa propria (so para arquivos)
static void capeScan() {
    gCapeFiles.clear(); std::error_code ec;
    for (auto& de : std::filesystem::directory_iterator("assets/capes", ec)) { if (!de.is_regular_file(ec)) continue; std::string n = de.path().filename().string(), e = de.path().extension().string(); for (char& c : e) c = (char)tolower((unsigned char)c);
        if (e == ".png" || e == ".gif") gCapeFiles.push_back(n); }
    std::sort(gCapeFiles.begin(), gCapeFiles.end());
}
static int capeCount() { return 1 + CAPE_NBI + (int)gCapeFiles.size(); }
static std::string capeLabel(int i) {
    if (i <= 0) return "Nenhuma"; if (i <= CAPE_NBI) return capeBuiltinName(i);
    std::string n = gCapeFiles[(size_t)(i - 1 - CAPE_NBI)]; if (n.size() > 4) n.resize(n.size() - 4); if (n.size() > 14) n.resize(14); return n;
}
static std::string capeOptStr() { std::string s; for (int i = 0; i < capeCount(); i++) { if (i) s += '|'; std::string l = capeLabel(i); for (char& c : l) if (c == '|') c = '_'; s += l; } return s; }
static int capeHash(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char c : s) { h ^= c; h *= 16777619u; } return (int)(h & 0x7FFFFFFF); }
static bool capeSelectIdx(int sel) {   // aplica a escolha (carrega a imagem, marca para a rede); false = arquivo invalido (volta para nenhuma)
    gCapeMine.clear(); gCapeMyEnc.clear(); gCapeSelCur = 0; bool ok = true;
    if (sel >= 1 && sel <= CAPE_NBI) { capeBuiltin(sel, gCapeMine); gCapeSelCur = sel; }
    else if (sel > CAPE_NBI && sel - 1 - CAPE_NBI < (int)gCapeFiles.size()) {
        if (capeFromFile("assets/capes/" + gCapeFiles[(size_t)(sel - 1 - CAPE_NBI)], gCapeMine)) { gCapeSelCur = sel; capePack(gCapeMine, gCapeMyEnc); } else ok = false; }
    gCapeGen++; gCapeNetDirty = true; return ok;
}
