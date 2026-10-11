// ---------- modelos com textura de ENTIDADE 64x64 (bau, bau do Ender, cama) ----------
// No resource pack essas texturas ficam em entity/chest/*.png e entity/bed/<cor>.png (nao em block/). Cada uma ocupa um bloco 4x4 de tiles
// do atlas (logo depois dos itens do pack), carregado em loadPackItems -> entLoadAll(). A malha usa UV em pixels da textura de 64x64
// (u/v do XQuad em unidades de tile = px/16, cruzando tiles vizinhos do bloco). Medidas e UVs = ModelChest / ModelBed do 1.12.2.
static std::unordered_map<std::string, int> gEntTile;   // "chest/normal" -> tile do canto superior esquerdo do bloco 4x4 (ausente = pack sem a textura)
static int gEntN = 0;                                    // blocos 4x4 ja alocados
static int entTile(const std::string& name) { auto it = gEntTile.find(name); return it == gEntTile.end() ? -1 : it->second; }
static void entLoadOne(const std::string& name, int firstRow) {
    std::vector<uint8_t> px; int w, h;
    if (!packRGBA("entity/" + name + ".png", px, w, h) || w < 1 || h < 1) return;
    const int row = firstRow + (gEntN / 8) * 4, col = (gEntN % 8) * 4;
    if (row + 4 > ATLR0) return;   // atlas cheio: cai no modelo de reserva (texturas de block/)
    std::vector<uint8_t> f((size_t)64 * 64 * 4); scaleRGBA(px, w, h, f.data(), 64, 64);
    for (int ty = 0; ty < 4; ty++) for (int tx = 0; tx < 4; tx++) {
        uint8_t t[1024];
        for (int y = 0; y < 16; y++) memcpy(t + y * 64, &f[(((size_t)(ty * 16 + y)) * 64 + tx * 16) * 4], 64);
        putTile16((row + ty) * ATL + col + tx, t);
    }
    gEntTile[name] = row * ATL + col; gEntN++;
}
// ---- SPRITES 16x16 (item plano, pixel art) de cama e estandarte, gerados a partir das texturas de entidade do proprio pack ----
// Tiles fixos nas 2 linhas extras do atlas (depois de ATLR0): cama cor c = SPR0 + c, estandarte cor c = SPR0 + 16 + c. Todo pixel da silhueta tem alfa 255 (nada translucido).
static const int SPR0 = ATLR0 * ATL;
static bool gSprOk[2][16];
static int entSprite(int kind, int color) { color &= 15; return gSprOk[kind][color] ? SPR0 + kind * 16 + color : -1; }
static void entBuildSprites() {
    memset(gSprOk, 0, sizeof gSprOk);
    static const char* C[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
    static const int DYE[16][3] = {{249,255,254},{249,128,29},{199,78,189},{58,179,218},{254,216,61},{128,199,31},{243,139,170},{71,79,82},{157,157,151},{22,156,156},{137,50,184},{60,68,170},{131,84,50},{94,124,22},{176,46,38},{29,29,33}};
    std::vector<uint8_t> px; int w, h; std::vector<uint8_t> t((size_t)64 * 64 * 4);
    for (int c = 0; c < 16; c++) {   // CAMA: vista obliqua, cabeceira a esquerda; topo (travesseiro+colcha) = amostras do proprio topo da textura
        if (!packRGBA(std::string("entity/bed/") + C[c] + ".png", px, w, h) || w < 1) continue;
        scaleRGBA(px, w, h, t.data(), 64, 64);
        auto P = [&](int x, int y, uint8_t* o) { const uint8_t* p = &t[((size_t)y * 64 + x) * 4]; o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255; };
        uint8_t o[1024]; memset(o, 0, sizeof o);
        auto put = [&](int x, int y, const uint8_t* rgb) { uint8_t* q = o + (y * 16 + x) * 4; q[0] = rgb[0]; q[1] = rgb[1]; q[2] = rgb[2]; q[3] = 255; };
        uint8_t col[4];
        for (int r = 0; r < 7; r++) for (int x = 0; x < 16; x++) {   // topo: 7 linhas (largura 16 px -> 7), comprimento 32 px -> 16 colunas
            const int z = x * 2 + (x & 1 ? 0 : 1), wx = std::min(15, (int)((r + .5f) * 16.f / 7.f));
            if (z < 16) P(6 + wx, 6 + z, col); else P(6 + wx, 28 + (z - 16), col);
            put(x, 3 + r, col);
        }
        for (int r = 0; r < 3; r++) for (int x = 0; x < 16; x++) { P(28 + x, 8 + r * 4, col); put(x, 10 + r, col); }   // lateral de madeira
        for (int r = 0; r < 2; r++) for (int lx : {0, 1, 2, 13, 14, 15}) { P(50 + (lx % 3), 3 + r, col); put(lx, 13 + r, col); }   // pes
        for (int r = 1; r < 3; r++) for (int x = 0; x < 2; x++) { P(0 + x, 8 + r, col); put(x, r + 1, col); }   // cabeceira (poste de madeira)
        putTile16(SPR0 + c, o); gSprOk[0][c] = true;
    }
    gEntTile.erase("banner/banner_base");
    if (packRGBA("entity/banner/banner_base.png", px, w, h) && w >= 1) {   // ESTANDARTE: pano (u1..21, v1..41) tingido pela cor + barra de madeira
        scaleRGBA(px, w, h, t.data(), 64, 64);
        for (int ty = 0; ty < 4; ty++) for (int tx = 0; tx < 4; tx++) {   // textura 64x64 do modelo 3D: bloco 4x4 de tiles nas linhas ATLR0+1..+4
            uint8_t tt[1024]; for (int y = 0; y < 16; y++) memcpy(tt + y * 64, &t[(((size_t)(ty * 16 + y)) * 64 + tx * 16) * 4], 64);
            putTile16((ATLR0 + 1 + ty) * ATL + tx, tt);
        }
        gEntTile["banner/banner_base"] = (ATLR0 + 1) * ATL;
        for (int c = 0; c < 16; c++) {
            uint8_t o[1024]; memset(o, 0, sizeof o);
            auto put = [&](int x, int y, int r, int g, int b) { uint8_t* q = o + (y * 16 + x) * 4; q[0] = (uint8_t)r; q[1] = (uint8_t)g; q[2] = (uint8_t)b; q[3] = 255; };
            for (int y = 2; y < 14; y++) for (int x = 3; x < 13; x++) {
                const uint8_t* p = &t[((size_t)std::min(40, 1 + (int)((y - 2 + .5f) * 40.f / 12.f)) * 64 + 1 + (int)((x - 3 + .5f) * 20.f / 10.f)) * 4];
                put(x, y, std::min(255, p[0] * DYE[c][0] / 235), std::min(255, p[1] * DYE[c][1] / 235), std::min(255, p[2] * DYE[c][2] / 235));
            }
            for (int x = 2; x < 14; x++) for (int y = 0; y < 2; y++) { const uint8_t* p = &t[((size_t)(43 + y) * 64 + 4 + x * 2) * 4]; put(x, y, p[0], p[1], p[2]); }   // barra (madeira da textura)
            for (int y = 14; y < 16; y++) for (int x : {7, 8}) { const uint8_t* p = &t[((size_t)(8 + y) * 64 + 45 + (x - 7) * 4) * 4]; put(x, y, p[0], p[1], p[2]); }   // haste
            putTile16(SPR0 + 16 + c, o); gSprOk[1][c] = true;
        }
    }
}
static const int SIGN_N = 12;
static const char* SIGN_WOOD[SIGN_N] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak", "mangrove", "cherry", "bamboo", "crimson", "warped", "pale_oak"};
static void entLoadAll() {   // depois de os tiles do pack estarem no atlas (PK.size() final)
    gEntTile.clear(); gEntN = 0; memset(gSprOk, 0, sizeof gSprOk); if (!gPack.ok) return;
    const int firstRow = (PKTILE0 + (int)PK.size() + ATL - 1) / ATL;
    for (const char* n : {"chest/normal", "chest/trapped", "chest/ender", "chest/normal_left", "chest/normal_right", "chest/trapped_left", "chest/trapped_right", "enchantment/enchanting_table_book"}) entLoadOne(n, firstRow);
    static const char* C[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
    for (int i = 0; i < 16; i++) entLoadOne(std::string("bed/") + C[i], firstRow);
    for (int i = 0; i < 16; i++) entLoadOne(std::string("shulker/shulker_") + C[i], firstRow);
    for (int i = 0; i < SIGN_N; i++) entLoadOne(std::string("signs/") + SIGN_WOOD[i], firstRow);
    entBuildSprites();   // 64x32 -> esticada para 64x64 (v dobra)
}

// Uma face de caixa (pixels de bloco 0..16). f: 0 -X, 1 +X, 2 -Y, 3 +Y, 4 -Z, 5 +Z. Regiao de textura (ua,va)-(ub,vb) em pixels.
// rot = true: o eixo horizontal da face corre ao longo de v da regiao (faces compridas da cama).
struct EntBoxCtx { std::vector<XQuad>* out; int tile; float ox, oy, oz; };   // origem (pixels) para deslocar a peca
static void entFace(const EntBoxCtx& c, int f, float x0, float y0, float z0, float x1, float y1, float z1, float ua, float va, float ub, float vb, int rot = 0) {
    x0 += c.ox; x1 += c.ox; y0 += c.oy; y1 += c.oy; z0 += c.oz; z1 += c.oz;
    float BL[3], BR[3], TR[3], TL[3];
    auto set = [](float* p, float x, float y, float z) { p[0] = x; p[1] = y; p[2] = z; };
    switch (f) {
    case 5: set(BL, x0, y0, z1); set(BR, x1, y0, z1); set(TR, x1, y1, z1); set(TL, x0, y1, z1); break;
    case 4: set(BL, x1, y0, z0); set(BR, x0, y0, z0); set(TR, x0, y1, z0); set(TL, x1, y1, z0); break;
    case 1: set(BL, x1, y0, z1); set(BR, x1, y0, z0); set(TR, x1, y1, z0); set(TL, x1, y1, z1); break;
    case 0: set(BL, x0, y0, z0); set(BR, x0, y0, z1); set(TR, x0, y1, z1); set(TL, x0, y1, z0); break;
    case 3: set(BL, x0, y1, z1); set(BR, x1, y1, z1); set(TR, x1, y1, z0); set(TL, x0, y1, z0); break;
    default: set(BL, x0, y0, z0); set(BR, x1, y0, z0); set(TR, x1, y0, z1); set(TL, x0, y0, z1); break;
    }
    const float e = .02f;   // recuo de 0,02 px: nada de vazar o texel vizinho
    ua += e; va += e; ub -= e; vb -= e;
    float U[4], V[4];   // BL, BR, TR, TL
    if (!rot) { U[0] = ua; V[0] = vb; U[1] = ub; V[1] = vb; U[2] = ub; V[2] = va; U[3] = ua; V[3] = va; }
    else if (rot == 2) { U[0] = ua; V[0] = va; U[1] = ua; V[1] = vb; U[2] = ub; V[2] = vb; U[3] = ub; V[3] = va; }   // lado -X da cama: madeira (u pequeno) embaixo, v corre de norte a sul
    else if (rot == 3) { U[0] = ub; V[0] = vb; U[1] = ub; V[1] = va; U[2] = ua; V[2] = va; U[3] = ua; V[3] = vb; }   // lado +X: espelho do anterior (a face e vista de fora, o sul fica a esquerda)
    else      { U[3] = ua; V[3] = va; U[2] = ua; V[2] = vb; U[1] = ub; V[1] = vb; U[0] = ub; V[0] = va; }   // TL=(ua,va) TR=(ua,vb) BR=(ub,vb) BL=(ub,va)
    XQuad q; q.tile = c.tile; q.tint = false; q.shade = true;
    const float* P[4] = {BL, BR, TR, TL};
    for (int k = 0; k < 4; k++) { for (int i = 0; i < 3; i++) q.p[k][i] = P[k][i] / 16.f; q.u[k] = U[k] / 16.f; q.v[k] = V[k] / 16.f; }
    c.out->push_back(q);
}
// gira os quads em torno do centro do bloco (mesma convencao do xbBake: angulo negativo = horario visto de cima)
static void entRotY(std::vector<XQuad>& qs, size_t from, int ay) {
    if (!ay) return;
    for (size_t i = from; i < qs.size(); i++) for (int k = 0; k < 4; k++) {
        float p[3] = {qs[i].p[k][0] - .5f, qs[i].p[k][1] - .5f, qs[i].p[k][2] - .5f}; xRot(p, 1, (float)-ay);
        qs[i].p[k][0] = p[0] + .5f; qs[i].p[k][1] = p[1] + .5f; qs[i].p[k][2] = p[2] + .5f;
    }
}

// Bau (normal 0 / armadilhado 1 / ender 2). Frente do modelo = +Z; dir 0 N,1 L,2 S,3 O = para onde a FRENTE aponta.
// half: -1 simples (14 de largura); 0/1 = metade ESQUERDA/DIREITA de bau duplo (textura *_left/_right do pack 1.15+: 15 de largura, a emenda fica sem face,
// trinco de 1x4x1 na emenda). open: tampa a 90 graus em torno da dobradica (y 9, z 1), como o ModelChest com lidAngle = 1.
static bool entChest(int type, int dir, std::vector<XQuad>& o, int half = -1, bool open = false) {
    static const char* N[3] = {"chest/normal", "chest/trapped", "chest/ender"};
    static const char* H[4] = {"chest/normal_left", "chest/normal_right", "chest/trapped_left", "chest/trapped_right"};
    type = std::max(0, std::min(type, 2)); if (half >= 0 && type > 1) return false;
    const int t = entTile(half < 0 ? N[type] : H[type * 2 + (half & 1)]); if (t < 0) return false;
    const float xa = half == 1 ? 0.f : 1.f, xb = half == 0 ? 16.f : 15.f, w = xb - xa; const bool sW = half != 1, sE = half != 0;
    const size_t s0 = o.size(); EntBoxCtx c{&o, t, 0, 0, 0};
    // corpo (0,19) em y 0..9 (a tampa cobre 9..10: sem faces coplanares)
    if (sW) entFace(c, 0, xa, 0, 1, xa, 9, 15, 0, 34, 14, 43);
    entFace(c, 5, xa, 0, 1, xb, 9, 15, 14, 34, 14 + w, 43);
    if (sE) entFace(c, 1, xb, 0, 1, xb, 9, 15, 14 + w, 34, 28 + w, 43);
    entFace(c, 4, xa, 0, 1, xb, 9, 1, 28 + w, 34, 28 + 2 * w, 43);
    entFace(c, 2, xa, 0, 1, xb, 0, 15, 14 + w, 19, 14 + 2 * w, 33);
    const size_t s1 = o.size();   // tampa (0,0) 5 de altura em y 9..14, e trinco
    if (sW) entFace(c, 0, xa, 9, 1, xa, 14, 15, 0, 14, 14, 19);
    entFace(c, 5, xa, 9, 1, xb, 14, 15, 14, 14, 14 + w, 19);
    if (sE) entFace(c, 1, xb, 9, 1, xb, 14, 15, 14 + w, 14, 28 + w, 19);
    entFace(c, 4, xa, 9, 1, xb, 14, 1, 28 + w, 14, 28 + 2 * w, 19);
    entFace(c, 3, xa, 14, 1, xb, 14, 15, 14, 0, 14 + w, 14);
    if (half < 0) {   // trinco (0,0) 2x4x1 em (7,7,15)
        entFace(c, 5, 7, 7, 15, 9, 11, 16, 1, 1, 3, 5);     entFace(c, 0, 7, 7, 15, 7, 11, 16, 0, 1, 1, 5);
        entFace(c, 1, 9, 7, 15, 9, 11, 16, 3, 1, 4, 5);     entFace(c, 3, 7, 11, 15, 9, 11, 16, 1, 0, 3, 1);
    } else {          // trinco (0,0) 1x4x1 na emenda: x 15 na esquerda, x 0 na direita
        const float lx = half == 0 ? 15.f : 0.f;
        entFace(c, 5, lx, 7, 15, lx + 1, 11, 16, 1, 1, 2, 5); entFace(c, 0, lx, 7, 15, lx, 11, 16, 0, 1, 1, 5);
        entFace(c, 1, lx + 1, 7, 15, lx + 1, 11, 16, 2, 1, 3, 5); entFace(c, 3, lx, 11, 15, lx + 1, 11, 16, 1, 0, 2, 1);
    }
    if (open) for (size_t i = s1; i < o.size(); i++) for (int k = 0; k < 4; k++) {   // tampa (e trinco) giram em torno da dobradica: frente sobe
        float p[3] = {o[i].p[k][0], o[i].p[k][1] - 9.f / 16.f, o[i].p[k][2] - 1.f / 16.f}; xRot(p, 0, -90.f);
        o[i].p[k][1] = p[1] + 9.f / 16.f; o[i].p[k][2] = p[2] + 1.f / 16.f;
    }
    entRotY(o, s0, 180 + 90 * (dir & 3));   // modelo tem a frente em +Z; o jogo usa frente = norte para dir 0
    return true;
}

// Caixa de shulker (ModelShulker 1.12.2, textura 64x64): base 16x8x16 (0,28) e tampa 16x12x16 (0,0) sobrepostas em y 4..8.
// Para nao haver faces coplanares a base so aparece em y 0..4 (metade de baixo da regiao v 44..52); a tampa vai de y 4 a 16.
// Regioes (u0=0): esquerda(0,d) frente(d,d) direita(d+w,d) tras(2d+w,d); topo(d,0) fundo(d+w,0). Mesmo mapeamento do bau: x0=esq, x1=dir, z1=frente, z0=tras.
static bool entShulker(int color, std::vector<XQuad>& o) {
    static const char* C[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
    const int t = entTile(std::string("shulker/shulker_") + C[std::max(0, std::min(color, 15))]); if (t < 0) return false;
    EntBoxCtx c{&o, t, 0, 0, 0};
    // base (y 0..4)
    entFace(c, 0, 0, 0, 0, 0, 4, 16, 0, 48, 16, 52);   entFace(c, 5, 0, 0, 16, 16, 4, 16, 16, 48, 32, 52);
    entFace(c, 1, 16, 0, 0, 16, 4, 16, 32, 48, 48, 52); entFace(c, 4, 0, 0, 0, 16, 4, 0, 48, 48, 64, 52);
    entFace(c, 2, 0, 0, 0, 16, 0, 16, 32, 28, 48, 44);
    // tampa (y 4..16)
    entFace(c, 0, 0, 4, 0, 0, 16, 16, 0, 16, 16, 28);   entFace(c, 5, 0, 4, 16, 16, 16, 16, 16, 16, 32, 28);
    entFace(c, 1, 16, 4, 0, 16, 16, 16, 32, 16, 48, 28); entFace(c, 4, 0, 4, 0, 16, 16, 0, 48, 16, 64, 28);
    entFace(c, 3, 0, 16, 0, 16, 16, 16, 16, 0, 32, 16);
    return true;
}

// Placa (ModelSign do 1.12.2, escala 2/3): tabua 16x8x1,33 e poste 1,33x9,33x1,33. Textura 64x32 esticada para 64x64 => v nativo x2.
// Frente do modelo = +Z; dir 0 N,1 L,2 S,3 O = para onde a FRENTE aponta. Parede: so a tabua, encostada na parede atras da frente.
static bool entSign(int wood, bool wall, int dir, std::vector<XQuad>& o) {
    const int t = entTile(std::string("signs/") + SIGN_WOOD[std::max(0, std::min(wood, SIGN_N - 1))]); if (t < 0) return false;
    const size_t s0 = o.size(); EntBoxCtx c{&o, t, 0, 0, 0};
    const float bx0 = 0, bx1 = 16, bz0 = 7.333f, bz1 = 8.667f; float by0 = 9.333f, by1 = 17.333f;
    float dz = 0, dy = 0; if (wall) { dz = -7; dy = -5; }   // 1.12: translate(0,-0.3125,-0.4375)
    auto V = [](float v) { return v * 2; };
    // tabua: caixa nativa (u0=0,v0=0) w24 h12 d2 -> topo (2,0) baixo (26,0) esq (0,2) frente (2,2) dir (26,2) tras (28,2)
    entFace(c, 5, bx0, by0 + dy, bz1 + dz, bx1, by1 + dy, bz1 + dz, 2, V(2), 26, V(14));
    entFace(c, 4, bx0, by0 + dy, bz0 + dz, bx1, by1 + dy, bz0 + dz, 28, V(2), 52, V(14));
    entFace(c, 0, bx0, by0 + dy, bz0 + dz, bx0, by1 + dy, bz1 + dz, 0, V(2), 2, V(14));
    entFace(c, 1, bx1, by0 + dy, bz0 + dz, bx1, by1 + dy, bz1 + dz, 26, V(2), 28, V(14));
    entFace(c, 3, bx0, by1 + dy, bz0 + dz, bx1, by1 + dy, bz1 + dz, 2, 0, 26, V(2));
    entFace(c, 2, bx0, by0 + dy, bz0 + dz, bx1, by0 + dy, bz1 + dz, 26, 0, 50, V(2));
    if (!wall) {   // poste (0,14) w2 h14 d2: topo (2,14) baixo (4,14) esq (0,16) frente (2,16) dir (4,16) tras (6,16)
        const float x0 = 7.333f, x1 = 8.667f;
        entFace(c, 5, x0, 0, bz1, x1, by0, bz1, 2, V(16), 4, V(30));
        entFace(c, 4, x0, 0, bz0, x1, by0, bz0, 6, V(16), 8, V(30));
        entFace(c, 0, x0, 0, bz0, x0, by0, bz1, 0, V(16), 2, V(30));
        entFace(c, 1, x1, 0, bz0, x1, by0, bz1, 4, V(16), 6, V(30));
        entFace(c, 2, x0, 0, bz0, x1, 0, bz1, 4, V(14), 6, V(16));
    }
    // modelo com a frente em +Z. Parede: dir 0..3 (2 = sul = sem giro). De pe: rotacao 0..15 do Minecraft (22,5 graus, horario; 0 = sul)
    const float ay = wall ? 180.f + 90.f * (dir & 3) : 22.5f * (dir & 15);
    for (size_t i = s0; i < o.size(); i++) for (int k = 0; k < 4; k++) {
        float p[3] = {o[i].p[k][0] - .5f, o[i].p[k][1] - .5f, o[i].p[k][2] - .5f}; xRot(p, 1, -ay);
        o[i].p[k][0] = p[0] + .5f; o[i].p[k][1] = p[1] + .5f; o[i].p[k][2] = p[2] + .5f;
    }
    return true;
}

// Cama: uma metade (cabeca ou pe) na celula; cabeca para o NORTE na pose canonica (dir 0), girada por dir. zoff: deslocamento (px) p/ o icone de 2 pecas.
static bool entBedPiece(int color, bool head, float zoff, std::vector<XQuad>& o) {
    const int t = entTile(std::string("bed/") + RS_COLOR[color & 15]); if (t < 0) return false;
    EntBoxCtx c{&o, t, 0, 0, zoff};
    if (head) {
        entFace(c, 3, 0, 9, 0, 16, 9, 16, 6, 6, 22, 22);            // topo com o travesseiro (v cresce para o sul)
        entFace(c, 4, 0, 3, 0, 16, 9, 0, 6, 0, 22, 6);              // cabeceira
        entFace(c, 0, 0, 3, 0, 0, 9, 16, 0, 6, 6, 22, 2);        // lados compridos
        entFace(c, 1, 16, 3, 0, 16, 9, 16, 22, 6, 28, 22, 3);
        entFace(c, 2, 0, 3, 0, 16, 3, 16, 28, 6, 44, 22);           // baixo
    } else {
        entFace(c, 3, 0, 9, 0, 16, 9, 16, 6, 28, 22, 44);           // topo (colchao)
        entFace(c, 5, 0, 3, 16, 16, 9, 16, 22, 22, 38, 28);         // peseira
        entFace(c, 0, 0, 3, 0, 0, 9, 16, 0, 28, 6, 44, 2);
        entFace(c, 1, 16, 3, 0, 16, 9, 16, 22, 28, 28, 44, 3);
        entFace(c, 2, 0, 3, 0, 16, 3, 16, 28, 28, 44, 44);
    }
    const float lz = head ? 0.f : 13.f;   // pes nas pontas de fora
    for (int lx : {0, 13}) {              // perna 3x3x3 em (50,0): lados (50,3)-(62,6)
        entFace(c, 0, (float)lx, 0, lz, (float)lx, 3, lz + 3, 50, 3, 53, 6); entFace(c, 5, (float)lx, 0, lz + 3, lx + 3.f, 3, lz + 3, 53, 3, 56, 6);
        entFace(c, 1, lx + 3.f, 0, lz, lx + 3.f, 3, lz + 3, 56, 3, 59, 6);   entFace(c, 4, (float)lx, 0, lz, lx + 3.f, 3, lz, 59, 3, 62, 6);
    }
    return true;
}
// ---------- ESTANDARTE (ModelBanner + TileEntityBannerRenderer do 1.12.2) ----------
// Escala 2/3, y e z invertidos; em pe: origem (8,8,8) px; parede: (8,-9,1) (translate 0,-0.3125,-0.4375). Haste 2x42x2 (44,0), barra 20x2x2 (0,42), pano 20x40x1 (0,0) preso em y=-32.
// Frente do pano em +Z (rotacao 0 = sul, horario). Pano tingido pela cor (tc); haste e barra sem tinta. Meta: standing rot|cor<<4, parede dir|cor<<4.
static const std::vector<XQuad>& bnQuads(int b, int meta) {
    static std::vector<XQuad> C[2][256]; static bool B[2][256]; static int lastT = -2;
    static const int DYE[16][3] = {{249,255,254},{249,128,29},{199,78,189},{58,179,218},{254,216,61},{128,199,31},{243,139,170},{71,79,82},{157,157,151},{22,156,156},{137,50,184},{60,68,170},{131,84,50},{94,124,22},{176,46,38},{29,29,33}};
    const int t = entTile("banner/banner_base");
    if (t != lastT) { memset(B, 0, sizeof B); for (auto& r : C) for (auto& v : r) v.clear(); lastT = t; }
    const int k = b == XB_WBANNER ? 1 : 0, m = meta & 255;
    if (B[k][m]) return C[k][m];
    B[k][m] = true; std::vector<XQuad>& o = C[k][m]; o.clear(); if (t < 0) return o;
    const bool wall = k == 1; const float s = 2.f / 3.f, oy = wall ? -9.f : 8.f, oz = wall ? 1.f : 8.f;
    const int col = (m >> 4) & 15; const float tcv[3] = {std::min(1.f, DYE[col][0] / 235.f), std::min(1.f, DYE[col][1] / 235.f), std::min(1.f, DYE[col][2] / 235.f)};
    auto box = [&](float bx, float by, float bz, float w, float h, float d, int u, int v, const float* tc) {
        const float x0 = 8 + bx * s, x1 = 8 + (bx + w) * s, y0 = oy - (by + h) * s, y1 = oy - by * s, z0 = oz - (bz + d) * s, z1 = oz - bz * s;
        const size_t n0 = o.size(); EntBoxCtx c{&o, t, 0, 0, 0};
        entFace(c, 5, x0, y0, z1, x1, y1, z1, u + d, v + d, u + d + w, v + d + h);
        entFace(c, 4, x0, y0, z0, x1, y1, z0, u + d + w + d, v + d, u + d + w + d + w, v + d + h);
        entFace(c, 0, x0, y0, z0, x0, y1, z1, u, v + d, u + d, v + d + h);
        entFace(c, 1, x1, y0, z0, x1, y1, z1, u + d + w, v + d, u + d + w + d, v + d + h);
        entFace(c, 3, x0, y1, z0, x1, y1, z1, u + d, v, u + d + w, v + d);
        entFace(c, 2, x0, y0, z0, x1, y0, z1, u + d + w, v, u + d + w + w, v + d);
        if (tc) for (size_t i = n0; i < o.size(); i++) for (int q = 0; q < 3; q++) o[i].tc[q] = tc[q];
    };
    if (!wall) box(-1, -30, -1, 2, 42, 2, 44, 0, nullptr);   // haste (so em pe)
    box(-10, -32, -1, 20, 2, 2, 0, 42, nullptr);              // barra
    box(-10, -32, -2, 20, 40, 1, 0, 0, tcv);                  // pano
    const float ay = wall ? 180.f + 90.f * (m & 3) : 22.5f * (m & 15);
    for (XQuad& q : o) for (int i = 0; i < 4; i++) { float p[3] = {q.p[i][0] - .5f, q.p[i][1] - .5f, q.p[i][2] - .5f}; xRot(p, 1, -ay); q.p[i][0] = p[0] + .5f; q.p[i][1] = p[1] + .5f; q.p[i][2] = p[2] + .5f; }
    return o;
}
static bool entBed(int m, std::vector<XQuad>& o) {
    const int color = (m >> 3) & 15;
    if (m & 128) {   // icone: as duas metades lado a lado (cabeca ao norte), sem giro
        const size_t i0 = o.size();
        if (!(entBedPiece(color, true, -16.f, o) && entBedPiece(color, false, 0.f, o))) return false;
        const float S = .62f;   // 2 blocos de comprimento -> cabe na caixa do icone
        for (size_t i = i0; i < o.size(); i++) for (int k = 0; k < 4; k++) {
            o[i].p[k][0] = .5f + (o[i].p[k][0] - .5f) * S; o[i].p[k][1] = .5f + (o[i].p[k][1] - .28f) * S; o[i].p[k][2] = .5f + o[i].p[k][2] * S;
        }
        return true;
    }
    const size_t s0 = o.size(); if (!entBedPiece(color, ((m >> 2) & 1) != 0, 0.f, o)) return false;
    entRotY(o, s0, 90 * (m & 3)); return true;
}

// ---------- livro da mesa de encantamento (ModelBook + TileEntityEnchantmentTableRenderer do 1.12.2) ----------
// Textura entity/enchantment/enchanting_table_book.png e 64x32; entLoadOne a estica para 64x64, entao v (px) vale o dobro.
// Pose estatica: livro aberto (abertura 1,25 rad), pose do renderizador: translate(0,5; 0,75+0,1; 0,5), rotate 80 graus em Z.
static const std::vector<XQuad>& entBookQuads() {
    static std::vector<XQuad> Q; static bool built = false; static int lastTile = -2;
    const int t = entTile("enchantment/enchanting_table_book");
    if (built && lastTile == t) return Q;
    built = true; lastTile = t; Q.clear(); if (t < 0) return Q;
    const float PI = 3.14159265f, f5 = 1.25f, fl1 = .35f, fl2 = .65f, sf = sinf(f5);
    struct Part { float bx, by, bz, w, h, d; int u, v; float rx, ry, rz, rot; };
    const Part P[] = {
        {-6, -5, 0, 6, 10, 1, 0, 0, 0, 0, -1, PI + f5},            // capa direita
        {0, -5, 0, 6, 10, 1, 16, 0, 0, 0, 1, -f5},                 // capa esquerda
        {0, -4, -.99f, 5, 8, 1, 0, 10, sf, 0, 0, f5},              // paginas direita
        {0, -4, -.01f, 5, 8, 1, 12, 10, sf, 0, 0, -f5},            // paginas esquerda
        {0, -4, 0, 5, 8, 0, 24, 10, sf, 0, 0, f5 - f5 * 2.f * fl1},   // pagina virando (direita)
        {0, -4, 0, 5, 8, 0, 24, 10, sf, 0, 0, f5 - f5 * 2.f * fl2},   // pagina virando (esquerda)
        {-1, -5, 0, 2, 10, 1, 12, 0, 0, 0, 0, PI / 2.f}};          // lombada
    const float ca = cosf(80.f * PI / 180.f), sa = sinf(80.f * PI / 180.f);
    for (const Part& p : P) {
        const float x1 = p.bx, y1 = p.by, z1 = p.bz, x2 = p.bx + p.w, y2 = p.by + p.h, z2 = p.bz + p.d;
        const float C[8][3] = {{x1, y1, z1}, {x2, y1, z1}, {x2, y2, z1}, {x1, y2, z1}, {x1, y1, z2}, {x2, y1, z2}, {x2, y2, z2}, {x1, y2, z2}};   // A B C D E F G H
        enum { A, B, Cc, Dd, E, F, G, H };
        const int FV[6][4] = {{F, B, Cc, G}, {A, E, H, Dd}, {F, E, A, B}, {Cc, Dd, H, G}, {B, A, Dd, Cc}, {E, F, G, H}};
        const float d = p.d, w = p.w, h = p.h, u = (float)p.u, v = (float)p.v;
        const float UVR[6][4] = {{u + d + w, v + d, u + d + w + d, v + d + h}, {u, v + d, u + d, v + d + h}, {u + d, v, u + d + w, v + d},
                                 {u + d + w, v + d, u + d + w + w, v}, {u + d, v + d, u + d + w, v + d + h}, {u + d + w + d, v + d, u + d + w + d + w, v + d + h}};
        const float cr = cosf(p.rot), sr = sinf(p.rot);
        for (int f = 0; f < 6; f++) {
            if (d == 0.f && f != 4) continue;   // pagina sem espessura: so um plano
            XQuad q; q.tile = t; q.tint = false; q.shade = true;
            const float u1 = UVR[f][0], v1 = UVR[f][1], u2 = UVR[f][2], v2 = UVR[f][3];
            const float QU[4] = {u2, u1, u1, u2}, QV[4] = {v1, v1, v2, v2};
            for (int k = 0; k < 4; k++) {
                const float* c = C[FV[f][k]];
                float x = c[0] * cr + c[2] * sr, z = -c[0] * sr + c[2] * cr, y = c[1];   // gira em Y em torno do ponto de rotacao
                x += p.rx; y += p.ry; z += p.rz;
                const float gx = x * ca - y * sa, gy = x * sa + y * ca;                   // rotate(80, 0, 0, 1)
                q.p[k][0] = .5f + gx / 16.f; q.p[k][1] = .85f + gy / 16.f; q.p[k][2] = .5f + z / 16.f;
                q.u[k] = QU[k] / 16.f; q.v[k] = QV[k] * 2.f / 16.f;
            }
            Q.push_back(q);
        }
    }
    return Q;
}
static void emitEnchBook(Chunk* c, int x, int y, int z, int wx, int wz) {
    const std::vector<XQuad>& qs = entBookQuads(); if (qs.empty()) return;
    const uint8_t Lc = c->lc[idx(x, y, z)]; const int sk = Lc >> 4, bk = Lc & 15;
    const float sfc = ltab((float)sk), bf = bk > 0 ? ltabB((float)bk) : 0.f, pk = packLight((float)sk, (float)bk);
    for (const XQuad& q : qs) {
        const float m = .85f;
        float r, g, bb, a = 1.f;
        if (gShaderOK) { r = g = bb = m; a = pk; } else { r = std::max(sfc, bf) * m; g = std::max(sfc, bf * .93f) * m; bb = std::max(sfc, bf * .78f) * m; }
        const int tx = q.tile % ATL, ty = q.tile / ATL;
        for (int k = 0; k < 4; k++) c->mesh.push_back({wx + q.p[k][0], y + q.p[k][1], wz + q.p[k][2], (tx + q.u[k]) / (float)ATL, (ty + q.v[k]) / (float)ATLR, r, g, bb, a});
    }
}
