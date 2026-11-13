// endportal.inc - PORTAL DO END como o do 1.12.2 (patchEndPortal.1): ceu de galaxia.
// O TileEntityEndPortalRenderer do MC desenha 16 camadas com blend aditivo: a primeira usa entity/end_sky... (fundo roxo/azul escuro) e as outras
// usam entity/end_portal.png (estrelas brancas finas), cada uma com escala/rotacao/deslocamento proprios e uma cor teal/azul diferente; o olho
// ve profundidade (paralaxe). Aqui: uma laje de fundo + 12 camadas aditivas empilhadas entre 0.45 e 0.75 de altura (o topo da caixa e 12/16),
// com texturas do PACK (fallback procedural se faltarem), UV em coordenadas de MUNDO (continuo entre as 9 celulas, sem repetir por bloco).
// So GL 1.x imediato (glBegin/glTexCoord/glVertex + blend ONE,ONE): roda igual em GL, GLES (gl4es) e Vulkan (vkgl).
static std::unordered_map<int64_t, std::array<int, 3>> gEndCells;   // celulas do portal (preenchido pelo mesher: emitXB); validadas ao desenhar
static GLuint gEpStars = 0, gEpSky = 0;
static void endPortalNote(int x, int y, int z) { gEndCells[wkey(x, y, z)] = {x, y, z}; }
static void epMakeTex(GLuint& id, const char* file, int w0, bool sky) {
    std::vector<uint8_t> px; int w = 0, h = 0;
    if (!packRGBA(file, px, w, h) || w < 4 || h < 4) {   // sem o pack: gera algo parecido (estrelas finas / ruido roxo escuro)
        w = h = w0; px.assign((size_t)w * h * 4, 0);
        uint32_t s = sky ? 0xE17D5u : 0xC0FFEEu; auto rnd = [&]() { s = s * 1664525u + 1013904223u; return (s >> 8) & 0xFFFFu; };
        for (int i = 0; i < w * h; i++) {
            uint8_t* o = &px[(size_t)i * 4]; const int r = (int)rnd();
            if (sky) { const int n = r & 31; o[0] = (uint8_t)(20 + n); o[1] = (uint8_t)(14 + n / 2); o[2] = (uint8_t)(36 + n); }
            else { const int v = (r % 100) < 2 ? 140 + (r & 115) : 0; o[0] = o[1] = o[2] = (uint8_t)v; }
            o[3] = 255;
        }
    }
    glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}
static void epQuad(int x, int z, float y, float s, float ca, float sa, float tx, float tz) {   // laje 1x1 em (x,z); UV = rotacao(escala * mundo) + deslocamento
    static const int C[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    for (int k = 0; k < 4; k++) {
        const float wx = (float)(x + C[k][0]) * s, wz = (float)(z + C[k][1]) * s;
        glTexCoord2f(wx * ca - wz * sa + tx, wx * sa + wz * ca + tz); glVertex3f((float)(x + C[k][0]), y, (float)(z + C[k][1]));
    }
}
static void drawEndPortals(float, double px, double pz) {
    if (gEndCells.empty()) return;
    if (!gEpStars) { epMakeTex(gEpStars, "entity/end_portal.png", 256, false); epMakeTex(gEpSky, "environment/end_sky.png", 128, true); }
    std::vector<std::array<int, 3>> cells;
    for (auto it = gEndCells.begin(); it != gEndCells.end();) {
        const auto& c = it->second;
        if (getB(c[0], c[1], c[2]) != XB_PORTAL || !(getM(c[0], c[1], c[2]) & 2)) { it = gEndCells.erase(it); continue; }   // removido (ou chunk descarregado: o mesher anota de novo)
        const double dx = c[0] + .5 - px, dz = c[2] + .5 - pz; if (dx * dx + dz * dz < 96.0 * 96.0) cells.push_back(c);
        ++it;
    }
    if (cells.empty()) return;
    const float t = (float)SDL_GetTicks() * .001f;
    rdEnable(GL_TEXTURE_2D); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdDisable(GL_BLEND); rdDepthMask(GL_TRUE); rdDepthFunc(GL_LEQUAL);
    // fundo: laje opaca com o end_sky.png escurecido (roxo/azul quase preto), deriva bem devagar
    glBindTexture(GL_TEXTURE_2D, gEpSky); glColor4f(.13f, .15f, .20f, 1.f);   // o MC multiplica a camada 0 por 2/18 = 0.11
    glBegin(GL_QUADS); for (const auto& c : cells) epQuad(c[0], c[2], c[1] + .45f, .10f, 1.f, 0.f, t * .0016f, t * .0010f); glEnd();
    // camadas de estrelas (aditivas, sem escrever profundidade). Cor por camada: R .1-.6, G .4-.9, B .5-1 (como o renderer do MC), peso 2/(18-i)
    rdDepthMask(GL_FALSE); rdEnable(GL_BLEND); rdBlendFunc(GL_ONE, GL_ONE); glBindTexture(GL_TEXTURE_2D, gEpStars);
    uint32_t rs = 31100u; auto rf = [&]() { rs = rs * 1103515245u + 12345u; return (float)((rs >> 8) & 0xFFFFu) / 65535.f; };
    const int NL = 12;
    for (int i = 1; i <= NL; i++) {
        const float cr = rf() * .5f + .1f, cg = rf() * .5f + .4f, cb = rf() * .5f + .5f, w = 5.f / (18.f - (float)i);
        const float s = .16f + .022f * (float)i, ang = (float)i * .55f, ca = cosf(ang), sa = sinf(ang);   // camadas mais altas = estrelas maiores (mais perto)
        const float dir = (i & 1) ? 1.f : -1.f, tx = dir * t * (.0009f + .0004f * (float)i) + (float)i * .137f, tz = t * (.0007f + .0003f * (float)(i % 5)) - (float)i * .291f;
        glColor4f(cr * w, cg * w, cb * w, 1.f);
        glBegin(GL_QUADS); for (const auto& c : cells) epQuad(c[0], c[2], c[1] + .45f + .30f * (float)(i - 1) / (float)(NL - 1), s, ca, sa, tx, tz); glEnd();
    }
    rdDisable(GL_BLEND); rdDepthMask(GL_TRUE); rdDepthFunc(GL_LEQUAL); glColor4f(1.f, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, atlasTex);
}
