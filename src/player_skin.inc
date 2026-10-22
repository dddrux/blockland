// ================= skin + modelo do personagem =================
static uint8_t skin[64 * 64 * 4];
static bool slim = false, slimAuto = true;
static GLuint skinTex = 0, atlasTex = 0;
static void waterAnimTick() {   // patchOpt.2: troca o frame da agua (10 fps) no atlas: nivel 0 e mipmaps do tile
    if (gWaterNF < 2 || !atlasTex) return;
    static int last = -1; const int fr = (int)((SDL_GetTicks() / 100) % (Uint32)gWaterNF); if (fr == last) return; last = fr;
    const uint8_t* src = &gWaterFrames[(size_t)fr * 1024]; std::vector<int> done;
    glBindTexture(GL_TEXTURE_2D, atlasTex);
    for (int f = 0; f < 3; f++) {
        const int t = TILES[WATER][f]; if (t < 0 || std::find(done.begin(), done.end(), t) != done.end()) continue; done.push_back(t);
        putTile16(t, src); uint8_t a[1024], b[256]; memcpy(a, src, 1024); int s = 16;
        for (int l = 0; l <= ATLAS_MIPS; l++) {
            glTexSubImage2D(GL_TEXTURE_2D, l, (t % ATL) * s, (t / ATL) * s, s, s, GL_RGBA, GL_UNSIGNED_BYTE, a);
            if (l == ATLAS_MIPS) break;
            const int h = s / 2; for (int y = 0; y < h; y++) for (int x = 0; x < h; x++) for (int c = 0; c < 4; c++) b[(y * h + x) * 4 + c] = (a[((2 * y) * s + 2 * x) * 4 + c] + a[((2 * y) * s + 2 * x + 1) * 4 + c] + a[((2 * y + 1) * s + 2 * x) * 4 + c] + a[((2 * y + 1) * s + 2 * x + 1) * 4 + c]) / 4;
            memcpy(a, b, (size_t)h * h * 4); s = h;
        }
    }
}
static std::vector<V> pHead, pHat, pBody, pJacket, pRArm, pRSleeve, pLArm, pLSleeve, pRLeg, pRPants, pLLeg, pLPants;

static void sp(int x, int y, int r, int g, int b, int a = 255) { uint8_t* p = &skin[(y * 64 + x) * 4]; p[0] = r; p[1] = g; p[2] = b; p[3] = a; }
static void srect(int x, int y, int w, int h, int r, int g, int b) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) sp(x + i, y + j, r, g, b); }
static void defaultSkin() {  // padrao: cinza com rostinho feliz, layout 64x64 do Minecraft
    memset(skin, 0, sizeof skin);
    srect(0, 0, 32, 16, 150, 150, 150); srect(8, 8, 8, 8, 196, 196, 196); srect(16, 16, 24, 16, 150, 150, 150); srect(20, 20, 8, 12, 110, 110, 110);
    srect(40, 16, 16, 16, 150, 150, 150); srect(32, 48, 16, 16, 150, 150, 150); srect(0, 16, 16, 16, 110, 110, 110); srect(16, 48, 16, 16, 110, 110, 110);
    const int e[][2] = {{10,11},{10,12},{13,11},{13,12},{9,13},{14,13},{10,14},{11,14},{12,14},{13,14}}; for (auto& q : e) srect(q[0], q[1], 1, 1, 20, 20, 20);
}
static void copyArea(int x, int y, int dx, int dy, int w, int h) {  // cópia espelhada (conversão de skin 64x32)
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) memcpy(&skin[((y + dy + j) * 64 + x + dx + w - 1 - i) * 4], &skin[((y + j) * 64 + x + i) * 4], 4);
}
static bool areaClear(int x, int y, int w, int h) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) if (skin[((y + j) * 64 + x + i) * 4 + 3]) return false; return true; }
static void quadV(std::vector<V>& o, const float p[4][3], const float uv[4][2], float s) {
    for (int k = 0; k < 4; k++) o.push_back({p[k][0], p[k][1], p[k][2], uv[k][0] / 64.f, uv[k][1] / 64.f, s, s, s, 1.f});
}
// caixa no layout de UV do Minecraft. Frente = -Z, direita do personagem = +X, topo = +Y.
static void boxM(std::vector<V>& o, float x0, float x1, float y0, float y1, float z0, float z1, float u, float v, float w, float h, float d, float g) {
    x0 -= g; y0 -= g; z0 -= g; x1 += g; y1 += g; z1 += g;
    auto F = [&](float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2, float d0, float d1, float d2,
                 float U, float Vv, float uw, float vh, float s) {
        float p[4][3] = {{a0,a1,a2},{b0,b1,b2},{c0,c1,c2},{d0,d1,d2}};
        float t[4][2] = {{U,Vv},{U+uw,Vv},{U+uw,Vv+vh},{U,Vv+vh}};
        quadV(o, p, t, s);
    };
    F(x1,y1,z0, x0,y1,z0, x0,y0,z0, x1,y0,z0, u+d, v+d, w, h, .85f);          // frente
    F(x0,y1,z1, x1,y1,z1, x1,y0,z1, x0,y0,z1, u+d+w+d, v+d, w, h, .85f);     // costas
    F(x1,y1,z1, x1,y1,z0, x1,y0,z0, x1,y0,z1, u, v+d, d, h, .7f);            // direita (+X)
    F(x0,y1,z0, x0,y1,z1, x0,y0,z1, x0,y0,z0, u+d+w, v+d, d, h, .7f);        // esquerda (-X)
    F(x1,y1,z1, x0,y1,z1, x0,y1,z0, x1,y1,z0, u+d, v, w, d, 1.f);            // topo
    F(x1,y0,z1, x0,y0,z1, x0,y0,z0, x1,y0,z0, u+d+w, v, w, d, .55f);         // base
}
static void buildModel() {
    for (auto* m : {&pHead,&pHat,&pBody,&pJacket,&pRArm,&pRSleeve,&pLArm,&pLSleeve,&pRLeg,&pRPants,&pLLeg,&pLPants}) m->clear();
    float aw = slim ? 3 : 4;
    boxM(pHead, -4,4, 0,8, -4,4, 0,0, 8,8,8, 0);        boxM(pHat, -4,4, 0,8, -4,4, 32,0, 8,8,8, .5f);
    boxM(pBody, -4,4, 12,24, -2,2, 16,16, 8,12,4, 0);   boxM(pJacket, -4,4, 12,24, -2,2, 16,32, 8,12,4, .25f);
    boxM(pRArm, -1,aw-1, -10,2, -2,2, 40,16, aw,12,4, 0); boxM(pRSleeve, -1,aw-1, -10,2, -2,2, 40,32, aw,12,4, .25f);
    boxM(pLArm, -aw+1,1, -10,2, -2,2, 32,48, aw,12,4, 0); boxM(pLSleeve, -aw+1,1, -10,2, -2,2, 48,48, aw,12,4, .25f);
    boxM(pRLeg, -2,2, -12,0, -2,2, 0,16, 4,12,4, 0);    boxM(pRPants, -2,2, -12,0, -2,2, 0,32, 4,12,4, .25f);
    boxM(pLLeg, -2,2, -12,0, -2,2, 16,48, 4,12,4, 0);   boxM(pLPants, -2,2, -12,0, -2,2, 0,48, 4,12,4, .25f);
}
static void uploadSkin() {
    if (!skinTex) glGenTextures(1, &skinTex);
    glBindTexture(GL_TEXTURE_2D, skinTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, skin);
}
static bool loadSkinPNG(const char* path, std::vector<uint8_t>& px, int& w, int& h) {   // PNG de skin; alta resolucao (128x128, 256x256, 128x64...) e reduzida para 64 por media de caixa
    if (!loadPNG(path, px, w, h)) return false;
    if (w > 64 && w % 64 == 0 && (h == w || h * 2 == w)) {
        const int f = w / 64, nh = h / f; std::vector<uint8_t> o((size_t)64 * nh * 4);
        for (int y = 0; y < nh; y++) for (int x = 0; x < 64; x++) { int sum[4] = {0, 0, 0, 0}; for (int j = 0; j < f; j++) for (int i = 0; i < f; i++) { const uint8_t* q = &px[((size_t)(y * f + j) * w + x * f + i) * 4]; const int al = q[3]; sum[0] += q[0] * al; sum[1] += q[1] * al; sum[2] += q[2] * al; sum[3] += al; }
            uint8_t* d = &o[((size_t)y * 64 + x) * 4]; if (sum[3]) { d[0] = (uint8_t)(sum[0] / sum[3]); d[1] = (uint8_t)(sum[1] / sum[3]); d[2] = (uint8_t)(sum[2] / sum[3]); } else d[0] = d[1] = d[2] = 0; d[3] = (uint8_t)(sum[3] / (f * f)); }
        px.swap(o); w = 64; h = nh;
    }
    return w == 64 && (h == 64 || h == 32);
}
static bool loadSkin(const char* path) {
    std::vector<uint8_t> px; int w, h;
    if (!loadSkinPNG(path, px, w, h)) { fprintf(stderr, "skin invalida (precisa ser PNG 64x64, 64x32 ou multiplo, ex. 128x128): %s\n", path); return false; }
    memset(skin, 0, sizeof skin); memcpy(skin, px.data(), (size_t)w * h * 4);
    if (h == 32) {
        bool hatOpaque = true;
        for (int j = 0; j < 16 && hatOpaque; j++) for (int i = 32; i < 64; i++) if (skin[(j * 64 + i) * 4 + 3] < 255) { hatOpaque = false; break; }
        if (hatOpaque) for (int j = 0; j < 16; j++) for (int i = 32; i < 64; i++) skin[(j * 64 + i) * 4 + 3] = 0;
        int c[][6] = {{4,16,16,32,4,4},{8,16,16,32,4,4},{0,20,24,32,4,12},{4,20,16,32,4,12},{8,20,8,32,4,12},{12,20,16,32,4,12},
                      {44,16,-8,32,4,4},{48,16,-8,32,4,4},{40,20,0,32,4,12},{44,20,-8,32,4,12},{48,20,-16,32,4,12},{52,20,-8,32,4,12}};
        for (auto& a : c) copyArea(a[0], a[1], a[2], a[3], a[4], a[5]);
        slimAuto = false; slim = false;
    } else {
        slim = areaClear(50, 16, 2, 4) && areaClear(54, 20, 2, 12) && areaClear(42, 48, 2, 4) && areaClear(46, 52, 2, 12);
    }
    for (int j = 0; j < 16; j++) for (int i = 0; i < 32; i++) skin[(j * 64 + i) * 4 + 3] = 255;   // base opaca
    for (int j = 16; j < 32; j++) for (int i = 0; i < 64; i++) skin[(j * 64 + i) * 4 + 3] = 255;
    for (int j = 48; j < 64; j++) for (int i = 16; i < 48; i++) skin[(j * 64 + i) * 4 + 3] = 255;
    uploadSkin(); buildModel();
    printf("Skin carregada: %s (%s)\n", path, slim ? "bracos finos / Alex" : "bracos normais / Steve");
    return true;
}

// ================= armadura visivel no personagem =================
// Camadas no formato do Minecraft (64x32, mesma disposicao da skin). Vem do resource pack ativo
// (entity/equipment/humanoid[_leggings]/<material>.png ou models/armor/<material>_layer_N.png);
// sem pack, gera uma textura simples na cor do material. Couro recebe a tinta padrao (0xA06540) + overlay.
static GLuint armorTex[4][2];
static inline GLuint armorTexOf(int id, int layer) {   // nativa (couro/ferro/ouro/diamante) ou de mod (propArmorTex; sem textura = ferro)
    if (isArmor(id)) return armorTex[(id - IT_A0) / 4][layer];
    const GLuint t = propArmorTex(id, layer); return t ? t : armorTex[1][layer];
}
static std::vector<V> aHead, aBody1, aBody2, aRArm, aLArm, aRLeg1, aLLeg1, aRLeg2, aLLeg2;   // 1 = camada 1 (infla 1.0), 2 = calcas (infla 0.5)
static void mirrorLegacyArmor(std::vector<uint8_t>& t) {
    static const int c[][6] = {{4,16,16,32,4,4},{8,16,16,32,4,4},{0,20,24,32,4,12},{4,20,16,32,4,12},{8,20,8,32,4,12},{12,20,16,32,4,12},
                               {44,16,-8,32,4,4},{48,16,-8,32,4,4},{40,20,0,32,4,12},{44,20,-8,32,4,12},{48,20,-16,32,4,12},{52,20,-8,32,4,12}};
    for (auto& a : c) for (int j = 0; j < a[5]; j++) for (int i = 0; i < a[4]; i++)
        memcpy(&t[((a[1] + a[3] + j) * 64 + a[0] + a[2] + a[4] - 1 - i) * 4], &t[((a[1] + j) * 64 + a[0] + i) * 4], 4);
}
static bool packArmorImg(const char* mat, int layer, bool overlay, std::vector<uint8_t>& out) {
    char a[160], b[160]; const char* ov = overlay ? "_overlay" : "";
    snprintf(a, sizeof a, "entity/equipment/%s/%s%s.png", layer ? "humanoid_leggings" : "humanoid", mat, ov);
    snprintf(b, sizeof b, "models/armor/%s_layer_%d%s.png", mat, layer + 1, ov);
    std::vector<uint8_t> px; int w, h;
    if (!packRGBA(a, px, w, h) && !packRGBA(b, px, w, h)) return false;
    int th = h * 64 / w; if (w < 16 || (th != 32 && th != 64)) return false;
    out.assign(64 * 64 * 4, 0); scaleRGBA(px, w, h, out.data(), 64, th);
    if (th == 32) mirrorLegacyArmor(out);
    return true;
}
static void procArmor(int layer, const int* c, std::vector<uint8_t>& t) {
    t.assign(64 * 64 * 4, 0);
    auto R = [&](int x, int y, int w, int h) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
        uint8_t* p = &t[((y + j) * 64 + x + i) * 4]; float k = .82f + (((x + i) * 7 + (y + j) * 13) % 5) * .045f - (j == h - 1 ? .2f : 0.f);
        for (int q = 0; q < 3; q++) p[q] = (uint8_t)std::min(255, (int)(c[q] * k)); p[3] = 255; } };
    if (!layer) { R(0, 0, 32, 16); R(16, 16, 24, 16); R(40, 16, 16, 16); R(8, 16, 4, 4); R(0, 27, 16, 5); }   // capacete, peitoral, bracos, botas
    else { R(0, 16, 16, 16); R(16, 28, 24, 4); }                                                              // calcas
    mirrorLegacyArmor(t);
}
static void buildArmorModel() {
    for (auto* m : {&aHead,&aBody1,&aBody2,&aRArm,&aLArm,&aRLeg1,&aLLeg1,&aRLeg2,&aLLeg2}) m->clear();
    boxM(aHead, -4,4, 0,8, -4,4, 0,0, 8,8,8, 1.f);
    boxM(aBody1, -4,4, 12,24, -2,2, 16,16, 8,12,4, 1.f); boxM(aBody2, -4,4, 12,24, -2,2, 16,16, 8,12,4, .5f);
    boxM(aRArm, -1,3, -10,2, -2,2, 40,16, 4,12,4, 1.f);  boxM(aLArm, -3,1, -10,2, -2,2, 32,48, 4,12,4, 1.f);
    boxM(aRLeg1, -2,2, -12,0, -2,2, 0,16, 4,12,4, 1.f);  boxM(aLLeg1, -2,2, -12,0, -2,2, 16,48, 4,12,4, 1.f);
    boxM(aRLeg2, -2,2, -12,0, -2,2, 0,16, 4,12,4, .5f);  boxM(aLLeg2, -2,2, -12,0, -2,2, 16,48, 4,12,4, .5f);
}
static void initArmor() {   // precisa de contexto GL; chamar depois de applyMcPack()
    static const char* MN[4] = {"leather", "iron", "gold", "diamond"};
    static const int FC[4][3] = {{160,101,64},{210,210,210},{245,205,55},{60,220,200}}, LEATHER[3] = {160,101,64};
    int fromPack = 0;
    for (int m = 0; m < 4; m++) for (int l = 0; l < 2; l++) {
        std::vector<uint8_t> t, o;
        if (packArmorImg(MN[m], l, false, t)) {
            fromPack++;
            if (m == 0) {
                for (size_t i = 0; i < t.size(); i += 4) for (int q = 0; q < 3; q++) t[i + q] = (uint8_t)(t[i + q] * LEATHER[q] / 255);
                if (packArmorImg("leather", l, true, o)) for (size_t i = 0; i < t.size(); i += 4) if (o[i + 3]) { float al = o[i + 3] / 255.f;
                    for (int q = 0; q < 3; q++) t[i + q] = (uint8_t)(o[i + q] * al + t[i + q] * (1 - al)); t[i + 3] = std::max(t[i + 3], o[i + 3]); }
            }
        } else procArmor(l, FC[m], t);
        GLuint& tx = armorTex[m][l]; if (!tx) glGenTextures(1, &tx);
        glBindTexture(GL_TEXTURE_2D, tx);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.data());
    }
    buildArmorModel();
    printf("Armadura: %d/8 camadas vieram do resource pack\n", fromPack);
}

// ---- elitra (ModelElytra do 1.12.2): duas asas 10x20x2 (textura 64x32, offset 22,0; a direita e espelhada), pivo em x=+-5 ----
static std::vector<V> eLWing, eRWing; static GLuint elyTex = 0;
static void initElytra() {   // precisa de contexto GL; depois de applyMcPack()
    eLWing.clear(); eRWing.clear();
    boxM(eLWing, 0, 10, -20, 0, 0, 2, 22, 0, 10, 20, 2, 1.f);    // nosso x = -x do Java: a asa esquerda (pivo em x=-5) estende para o centro (+x)
    boxM(eRWing, -10, 0, -20, 0, 0, 2, 22, 0, 10, 20, 2, 1.f);
    for (V& v : eLWing) v.v *= 2.f; for (V& v : eRWing) v.v *= 2.f;   // boxM normaliza por 64x64; a textura da elitra e 64x32
    for (int q = 0; q < 6; q++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, eRWing[q * 4 + k].u); hi = std::max(hi, eRWing[q * 4 + k].u); }
        for (int k = 0; k < 4; k++) eRWing[q * 4 + k].u = lo + hi - eRWing[q * 4 + k].u; }   // ModelRenderer.mirror: espelha cada face...
    for (int k = 0; k < 4; k++) { std::swap(eRWing[8 + k].u, eRWing[12 + k].u); std::swap(eRWing[8 + k].v, eRWing[12 + k].v); }   // ...e troca as laterais
    std::vector<uint8_t> t; int w = 0, h = 0; bool ok = packRGBA("entity/equipment/wings/elytra.png", t, w, h) || packRGBA("entity/elytra.png", t, w, h);
    if (!ok || w < 1 || h < 1) { w = 64; h = 32; t.assign((size_t)w * h * 4, 0); for (int y = 0; y < 22; y++) for (int x = 22; x < 46; x++) { uint8_t* p = &t[((size_t)y * w + x) * 4]; p[0] = 120; p[1] = 120; p[2] = 130; p[3] = 255; } }
    for (int pass = 0; pass < 3; pass++) { std::vector<uint8_t> o = t;   // pixels transparentes (0,0,0,0) herdam a cor dos vizinhos opacos: sem franja preta no filtro
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { uint8_t* q = &o[((size_t)y * w + x) * 4]; if (q[3] || (t[((size_t)y * w + x) * 4 + 3])) continue;
            int sr = 0, sg = 0, sb = 0, n = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) { const int xx = x + dx, yy = y + dy; if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                const uint8_t* s = &t[((size_t)yy * w + xx) * 4]; if (s[3] || s[0] | s[1] | s[2]) { sr += s[0]; sg += s[1]; sb += s[2]; n++; } }
            if (n) { q[0] = (uint8_t)(sr / n); q[1] = (uint8_t)(sg / n); q[2] = (uint8_t)(sb / n); } }
        t = o; }
    if (!elyTex) glGenTextures(1, &elyTex);
    glBindTexture(GL_TEXTURE_2D, elyTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.data());
    printf("Elitra: textura %s\n", ok ? "do resource pack" : "padrao");
}
