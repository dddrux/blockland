// ================= BUG-048: mapa (item/map) =================
// Com o mapa na mao aparece um painel 128x128 (1 bloco por pixel) centrado em quem segura: cor do topo de cada coluna (textura do bloco x tinta do bioma),
// relevo por sombreamento (coluna mais alta que a do norte = mais clara), agua azul por profundidade. Atualiza a cada ~1 s. So local (nao vai pela rede).
static GLuint gMapTex = 0; static int gMapT = 0, gMapCx = 0, gMapCz = 0;
static int mapItemId() { static size_t n = (size_t)-1; static int id = 0; if (n != PK.size()) { n = PK.size(); id = 0; for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == "item/map") { id = PK0 + (int)i; break; } } return id; }
static void mapTileRGB(int tile, float* o) {
    static std::unordered_map<int, std::array<float, 3>> C; auto it = C.find(tile); if (it == C.end()) {
        float r = 0, g = 0, b = 0, a = 0; const int tx = tile % ATL, ty = tile / ATL;
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const uint8_t* p = atlas + (((size_t)(ty * 16 + y)) * ATW + (tx * 16 + x)) * 4; r += p[0] * p[3]; g += p[1] * p[3]; b += p[2] * p[3]; a += p[3]; }
        std::array<float, 3> v = {128, 128, 128}; if (a > 0) v = {r / a, g / a, b / a}; it = C.emplace(tile, v).first; }
    o[0] = it->second[0]; o[1] = it->second[1]; o[2] = it->second[2];
}
static int mapTop(int x, int z, int y0) {   // y do bloco de topo (-1 = chunk ausente / vazio)
    if (!getC(fdiv(x, CW), fdiv(z, CW))) return -2;
    for (int y = std::min(CH - 1, y0); y >= 0; y--) { const int b = getB(x, y, z); if (b != AIR && b != XB_FIRE && !(isXB(b) && xbIsPlant(b) && false)) return y; }
    return -1;
}
static void mapRefresh(const Phys& P) {
    const int cx = (int)floor(P.x), cz = (int)floor(P.z), y0 = gDim == 1 ? std::min(120, (int)P.y + 6) : std::min(CH - 1, (int)P.y + 40);
    static uint8_t px[128 * 128 * 4]; static int hy[128 + 1][128];
    for (int j = -1; j < 128; j++) for (int i = 0; i < 128; i++) { const int wx = cx - 64 + i, wz = cz - 64 + j, y = mapTop(wx, wz, y0); if (j >= 0 || true) hy[j + 1][i] = y; }
    for (int j = 0; j < 128; j++) for (int i = 0; i < 128; i++) {
        const int wx = cx - 64 + i, wz = cz - 64 + j, y = hy[j + 1][i]; uint8_t* q = &px[((size_t)j * 128 + i) * 4]; q[3] = 255;
        if (y == -2) { q[0] = q[1] = q[2] = 0; q[3] = 0; continue; }
        if (y < 0) { q[0] = q[1] = q[2] = 20; continue; }
        const int b = getB(wx, y, wz); float c[3];
        if (isWater(b)) { int d = 0; while (y - d > 0 && isWater(getB(wx, y - d, wz)) && d < 12) d++; const float k = 1.f - d * .045f; c[0] = 50 * k; c[1] = 100 * k; c[2] = 220 * k; }
        else { int tile; float t[3]; partLook(b, true, (float)wx, (float)wz, tile, t); mapTileRGB(tile, c); for (int k = 0; k < 3; k++) c[k] *= t[k];
            const int yn = hy[j][i]; const float sh = yn >= 0 ? (y > yn ? 1.12f : y < yn ? .82f : 1.f) : 1.f; for (int k = 0; k < 3; k++) c[k] *= sh; }
        for (int k = 0; k < 3; k++) q[k] = (uint8_t)std::max(0.f, std::min(255.f, c[k]));
    }
    if (!gMapTex) glGenTextures(1, &gMapTex);
    glBindTexture(GL_TEXTURE_2D, gMapTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    gMapCx = cx; gMapCz = cz;
}
static void mapDraw(int w, int h, const Phys& P, int held, float yawDeg) {
    if (!held || held != mapItemId() || gDead) { gMapT = 0; return; }
    if (gMapT-- <= 0 || !gMapTex) { gMapT = 20; mapRefresh(P); }
    const float S = std::max(128.f, std::min(h * .55f, 320.f)), x0 = w - S - 12.f, y0 = h * .5f - S * .5f;
    rdViewport(gVpX, gVpY, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_CULL_FACE); rdDisable(GL_FOG); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdEnable(GL_TEXTURE_2D);
    rdDisable(GL_TEXTURE_2D); glColor4f(.35f, .25f, .12f, 1.f); glBegin(GL_QUADS); glVertex2f(x0 - 6, y0 - 6); glVertex2f(x0 + S + 6, y0 - 6); glVertex2f(x0 + S + 6, y0 + S + 6); glVertex2f(x0 - 6, y0 + S + 6); glEnd();
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, gMapTex); glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(x0, y0); glTexCoord2f(1, 0); glVertex2f(x0 + S, y0); glTexCoord2f(1, 1); glVertex2f(x0 + S, y0 + S); glTexCoord2f(0, 1); glVertex2f(x0, y0 + S); glEnd();
    const float k = S / 128.f, mx = x0 + (float)(P.x - (gMapCx - 64)) * k, mz = y0 + (float)(P.z - (gMapCz - 64)) * k, r = std::max(3.f, k * 2.f);   // marcador do jogador (seta = quadrado + ponta para onde olha)
    rdDisable(GL_TEXTURE_2D); glColor4f(1, 1, 1, 1); glBegin(GL_QUADS); glVertex2f(mx - r, mz - r); glVertex2f(mx + r, mz - r); glVertex2f(mx + r, mz + r); glVertex2f(mx - r, mz + r); glEnd();
    const float yr = yawDeg * .0174533f, fx = mx - sinf(yr) * r * 2.2f, fz = mz - cosf(yr) * r * 2.2f; glColor4f(.9f, .1f, .1f, 1.f); glBegin(GL_QUADS); glVertex2f(fx - r * .6f, fz - r * .6f); glVertex2f(fx + r * .6f, fz - r * .6f); glVertex2f(fx + r * .6f, fz + r * .6f); glVertex2f(fx - r * .6f, fz + r * .6f); glEnd();
    glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
}
