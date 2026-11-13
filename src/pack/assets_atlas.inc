// ---------- atlas em memoria (ATW x 16*ATLR) ----------
// Nao ha mais texturas procedurais/placeholder: todo tile vem do resource pack (asset_manager.inc), de mods ou e a textura "ausente".
static uint8_t clampb(float v) { return (uint8_t)std::max(0.f, std::min(255.f, v)); }
static uint8_t atlas[ATW * 16 * ATLR * 4];
static std::vector<float> gLodAvg;   // patchLOD.1: cor media por tile (lazy; lod.inc)
static inline int lodWhiteTile() { return (ATLR - 1) * ATL + (ATL - 1); }   // patchLOD.1: ultimo tile do atlas (linhas extras, livre) = branco puro p/ malha LOD
static void apx(int t, int x, int y, int r, int g, int b) {
    if (x < 0 || y < 0 || x > 15 || y > 15) return;
    int i = ((t / ATL * 16 + y) * ATW + (t % ATL * 16 + x)) * 4;
    atlas[i] = r; atlas[i + 1] = g; atlas[i + 2] = b; atlas[i + 3] = 255;
}
// textura "ausente" (xadrez magenta/preto, como o Minecraft): usada quando o pack nao tem o arquivo
static void fillMissing(int tile) { for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const bool m = ((x >> 3) ^ (y >> 3)) & 1; apx(tile, x, y, m ? 0 : 248, 0, m ? 0 : 248); } }

// ---- mipmaps do atlas (como o Minecraft: 4 niveis) ----
// Sem mipmap, GL_NEAREST de longe amostra 1 texel de uma textura 16x16 com furos de alpha: a cada movimento da camera o texel escolhido muda
// e folhas/mato/flores "piscam" (aliasing de minificacao). Cada nivel e a media 2x2 do anterior, com a COR ponderada pelo alfa (furos transparentes
// nao escurecem as folhas). 16 px por tile => 4 niveis cabem exatamente dentro do proprio tile (1x1 no nivel 4): nao ha vazamento entre tiles vizinhos.
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
static const int ATLAS_MIPS = 4;
static void mipDown(const uint8_t* s, int w, int h, uint8_t* o) {   // (w x h) -> (w/2 x h/2)
    for (int y = 0; y < h / 2; y++) for (int x = 0; x < w / 2; x++) {
        float r = 0, g = 0, b = 0, a = 0;
        for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) { const uint8_t* p = s + ((size_t)(2 * y + j) * w + 2 * x + i) * 4; r += p[0] * p[3]; g += p[1] * p[3]; b += p[2] * p[3]; a += p[3]; }
        uint8_t* q = o + ((size_t)y * (w / 2) + x) * 4;
        if (a > 0) { q[0] = (uint8_t)(r / a + .5f); q[1] = (uint8_t)(g / a + .5f); q[2] = (uint8_t)(b / a + .5f); } else { q[0] = q[1] = q[2] = 0; }
        q[3] = (uint8_t)(a / 4.f + .5f);
    }
}
// Cobertura do alfa: fracao de texels que passam no alpha test (0,1 no mundo). A media 2x2 faz isso crescer (folhas "engordam") ou, em mato fino, cair abaixo
// do corte (some). Por tile e por nivel, escala o alfa (busca binaria) para manter a mesma fracao do nivel 0. So em tiles com furos; o nivel 1x1 fica como esta.
static const int MIP_REF = 26;   // 0,1 * 255
static void tileCoverage0(const uint8_t* at, int w, std::vector<float>& cov) {   // nivel 0: cobertura de cada tile 16x16 (-1 = sem furos: nao mexe)
    const int tw = w / 16, th = (int)(cov.size() / tw);
    for (int ty = 0; ty < th; ty++) for (int tx = 0; tx < tw; tx++) {
        int pass = 0, holes = 0;
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const int al = at[((size_t)(ty * 16 + y) * w + tx * 16 + x) * 4 + 3]; if (al >= MIP_REF) pass++; if (al < MIP_REF) holes++; }
        cov[(size_t)ty * tw + tx] = holes ? pass / 256.f : -1.f;
    }
}
static void mipCoverage(uint8_t* m, int w, int h, int ts, const std::vector<float>& cov) {
    if (ts < 2) return; const int tw = w / ts, th = h / ts;
    for (int ty = 0; ty < th; ty++) for (int tx = 0; tx < tw; tx++) {
        const float tg = cov[(size_t)ty * tw + tx]; if (tg < 0.f) continue;
        const int need = tg > 0.f ? std::max(1, (int)(tg * ts * ts + .5f)) : 0;   // mato fino: ao menos 1 texel sobrevive (antes arredondava para 0 e sumia)
        float lo = .15f, hi = 6.f;
        auto count = [&](float s) { int n = 0; for (int y = 0; y < ts; y++) for (int x = 0; x < ts; x++) if (m[((size_t)(ty * ts + y) * w + tx * ts + x) * 4 + 3] * s >= MIP_REF) n++; return n; };
        for (int it = 0; it < 14; it++) { const float mid = (lo + hi) * .5f; if (count(mid) >= need) hi = mid; else lo = mid; }
        for (int y = 0; y < ts; y++) for (int x = 0; x < ts; x++) { uint8_t& al = m[((size_t)(ty * ts + y) * w + tx * ts + x) * 4 + 3]; al = (uint8_t)std::min(255.f, al * hi + .5f); }
    }
}
#ifndef GL_MAX_TEXTURE_SIZE
#define GL_MAX_TEXTURE_SIZE 0x0D33
#endif
#ifdef BL_RPI
// patchRpiPerf.3: tiles com algum texel nao opaco ("recortados": folha, mato, overlay da grama...). Os quads deles vao para o 2o trecho da malha (shader COM discard); o resto usa o shader SEM discard (early-Z no VC4).
static std::vector<uint8_t> gTileCut;
static void buildTileCut() {
    const int nt = ATL * ATLR; gTileCut.assign((size_t)nt, 0);
    for (int t = 0; t < nt; t++) { const int tx = t % ATL, ty = t / ATL; bool cut = false;
        for (int y = 0; y < 16 && !cut; y++) { const uint8_t* r = &atlas[(((size_t)(ty * 16 + y)) * ATW + tx * 16) * 4]; for (int x = 0; x < 16; x++) if (r[x * 4 + 3] < 255) { cut = true; break; } }
        gTileCut[(size_t)t] = cut ? 1 : 0; }
}
static inline bool tileIsCut(int t) { return gTileCut.empty() || t < 0 || t >= (int)gTileCut.size() || gTileCut[(size_t)t]; }   // sem tabela: tudo "recortado" (comportamento antigo)
#endif
// patchTree.1: texels transparentes tem RGB preto no PNG; a mistura trilinear entre niveis de mip puxava esse preto para os texels vizinhos (pontinhos pretos nas folhas).
// "Sangra" a cor dos texels opacos para dentro dos transparentes (dentro de cada tile, so RGB; o alfa nao muda).
static void bleedRGB(uint8_t* m, int w, int h, int ts) {
    if (ts < 2) return; const int tw = w / ts, th = h / ts; std::vector<uint8_t> done((size_t)ts * ts);
    for (int ty = 0; ty < th; ty++) for (int tx = 0; tx < tw; tx++) {
        bool any = false, hole = false;
        for (int y = 0; y < ts; y++) for (int x = 0; x < ts; x++) { const bool o = m[((size_t)(ty * ts + y) * w + tx * ts + x) * 4 + 3] > 0; done[(size_t)y * ts + x] = o; any |= o; hole |= !o; }
        if (!any || !hole) continue;
        for (int pass = 0; pass < ts; pass++) {
            std::vector<uint8_t> nd = done; bool ch = false;
            for (int y = 0; y < ts; y++) for (int x = 0; x < ts; x++) {
                if (done[(size_t)y * ts + x]) continue; int r = 0, g = 0, b = 0, n = 0;
                for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) { const int xx = x + dx, yy = y + dy; if (xx < 0 || yy < 0 || xx >= ts || yy >= ts || !done[(size_t)yy * ts + xx]) continue;
                    const uint8_t* p = m + ((size_t)(ty * ts + yy) * w + tx * ts + xx) * 4; r += p[0]; g += p[1]; b += p[2]; n++; }
                if (!n) continue; uint8_t* q = m + ((size_t)(ty * ts + y) * w + tx * ts + x) * 4; q[0] = (uint8_t)(r / n); q[1] = (uint8_t)(g / n); q[2] = (uint8_t)(b / n); nd[(size_t)y * ts + x] = 1; ch = true; }
            done.swap(nd); if (!ch) break;
        }
    }
}
static void uploadAtlasMips() {   // com o atlasTex ja ligado
#ifdef BL_RPI
    buildTileCut();
#endif
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
#if defined(BL_RPI) || defined(BL_XP)
        GL_NEAREST_MIPMAP_NEAREST   // patchRpiOpt.4: 1 amostra por texel no VC4 (em vez de 2)
#else
        GL_NEAREST_MIPMAP_LINEAR
#endif
        ); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ATLAS_MIPS);
#if !defined(__ANDROID__) && !defined(BL_RPI)
    if (BL_NOVK && gSharp != 0.f) glTexParameterf(GL_TEXTURE_2D, 0x8501 /* GL_TEXTURE_LOD_BIAS */, gSharp);   // patchGfx.2: menos "embacado" no chao/paredes em angulo (o aniso do atlas segue desligado por causa das listras; o bias nao cruza a borda do tile)
#endif
#if !defined(__ANDROID__) && !defined(BL_RPI)
    if (BL_NOVK) { GLfloat mxA = 1.f; glGetFloatv(0x84FF, &mxA); const float an = std::min(mxA, (float)gAniso); if (false && an > 1.f) glTexParameterf(GL_TEXTURE_2D, 0x84FE, an); }   // patchSeam.4: aniso DESLIGADO no atlas (o filtro alongado cruza a borda do tile e puxa cor do vizinho = listras laranja/amarelas)   // patchGfx.1: filtro anisotropico
#endif
    { GLint mx = 0; glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx); if (mx > 0 && (ATW > mx || 16 * ATLR > mx)) logError("Atlas %dx%d maior que o limite da GPU (%d): texturas nao vao aparecer", ATW, 16 * ATLR, (int)mx); else logInfo("Atlas de texturas %dx%d (limite da GPU %d)", ATW, 16 * ATLR, (int)mx); }
    int w = ATW, h = 16 * ATLR;
    { gLodAvg.clear(); const int t = lodWhiteTile(); for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) apx(t, x, y, 255, 255, 255); }   // patchLOD.1
    if (BL_NOVK) { auto sl = gPkByFile.find("block/slime_block"); if (sl != gPkByFile.end() && isPk(sl->second) && PK[sl->second - PK0].tile >= 0) {   // patchSlime.9: alpha 255 no upload final (independe da ordem de montagem do atlas)
        const int tl = PK[sl->second - PK0].tile; uint8_t* t0 = &atlas[(((size_t)(tl / ATL) * 16) * ATW + (size_t)(tl % ATL) * 16) * 4];
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) t0[((size_t)y * ATW + x) * 4 + 3] = 255; } }
    std::vector<uint8_t> a(atlas, atlas + (size_t)w * h * 4), b; bleedRGB(a.data(), w, h, 16);   // patchTree.1
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, a.data());
    std::vector<float> cov((size_t)(w / 16) * (h / 16)); tileCoverage0(atlas, w, cov);
    // cada nivel l usa tiles de (16>>l) texels; a tabela de cobertura (por tile) vale para todos os niveis
    for (int l = 1; l <= ATLAS_MIPS; l++) {
        splashPoll();
        b.assign((size_t)(w / 2) * (h / 2) * 4, 0); mipDown(a.data(), w, h, b.data()); w /= 2; h /= 2;
        std::vector<uint8_t> c = b; mipCoverage(c.data(), w, h, 16 >> l, cov); bleedRGB(c.data(), w, h, 16 >> l);   // alfa com cobertura preservada (folhas/mato nao engordam nem somem ao longe)
        glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c.data()); a.swap(b);
    }
}
