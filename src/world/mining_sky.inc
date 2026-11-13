// ---------- mineração (tempo de quebra do 1.8.9), rachaduras, partículas e nuvens ----------
// Tempo = dureza * (1.5 se pode coletar, senão 5) / velocidade da ferramenta (só se for a ferramenta certa).
static float blockHardness(int b) {
    if (isXB(b)) return xbHard(b);
    if (b >= PK0 && isPk(b)) return gPkHard[b - PK0];
    switch (b) {
        case GRASS: case CLAY: case GRAVEL: return .6f;
        case DIRT: case SAND: return .5f;
        case STONE: case STONEBRICK: case BOOKSHELF: return 1.5f;
        case COBBLE: case LOG: case PLANKS: case BRICKS: case MOSSY: return 2.f;
        case LEAVES: case SNOWBLK: return .2f;
        case GLOWSTONE: return .3f;
        case CTABLE: return 2.5f;
        case WOOL: case SANDSTONE: return .8f;
        case COALORE: case IRONORE: case GOLDORE: case DIAMONDORE: case REDSTONEORE: case LAPISORE: case GOLDBLK: return 3.f;
        case IRONBLK: case DIAMBLK: case COALBLK: return 5.f;
        case OBSIDIAN: return 50.f;
        default: return 1.5f;
    }
}
// ferramenta ideal do bloco: 1 picareta, 2 machado, 3 pá, 0 qualquer
static int blockTool(int b) {
    if (isXB(b)) return xbTool(b);
    if (b >= PK0 && isPk(b)) return gPkTool[b - PK0];
    switch (b) {
        case STONE: case COBBLE: case COALORE: case DIAMONDORE: case GOLDORE: case IRONORE: case GOLDBLK: case IRONBLK: case DIAMBLK: case COALBLK:
        case SANDSTONE: case STONEBRICK: case MOSSY: case OBSIDIAN: case BRICKS: case REDSTONEORE: case LAPISORE: return 1;
        case LOG: case PLANKS: case BOOKSHELF: case CTABLE: return 2;
        case GRASS: case DIRT: case SAND: case GRAVEL: case CLAY: case SNOWBLK: return 3;
        default: return 0;
    }
}
// nível mínimo da picareta para coletar (0 madeira ... 3 diamante); -1 = não exige picareta
static int blockLevel(int b) {
    if (isXB(b)) return xbLevel(b);
    if (b >= PK0 && isPk(b)) return gPkLevel[b - PK0];
    switch (b) {
        case IRONORE: case LAPISORE: case IRONBLK: return 1;
        case DIAMONDORE: case GOLDORE: case REDSTONEORE: case GOLDBLK: case DIAMBLK: return 2;
        case OBSIDIAN: return 3;
        default: return blockTool(b) == 1 ? 0 : -1;
    }
}
// tipo (1/2/3) e material (0 madeira, 1 pedra, 2 ferro, 3 ouro, 4 diamante) da ferramenta
static bool toolKind(int id, int& kind, int& tier) {
    if (id == IT_WPICK) { kind = 1; tier = 0; return true; }
    if (id == IT_SPICK) { kind = 1; tier = 1; return true; }
    if (id >= IT_IPICK && id <= IT_DPICK) { kind = 1; tier = id - IT_IPICK + 2; return true; }
    if (id >= IT_WAXE && id <= IT_DAXE) { kind = 2; tier = id - IT_WAXE; return true; }
    if (id >= IT_WSHOVEL && id <= IT_DSHOVEL) { kind = 3; tier = id - IT_WSHOVEL; return true; }
    return false;
}
static float breakTime(int b, int held, bool& harvest) {
    static const float SPD[5] = {2.f, 4.f, 6.f, 12.f, 8.f};
    static const int LVL[5] = {0, 1, 2, 0, 3};
    int tk = 0, tr = 0; toolKind(held, tk, tr);
    const int need = blockLevel(b), bt = blockTool(b);
    harvest = need < 0 || (tk == 1 && LVL[tr] >= need);
    const float speed = enchDigSpeedMul(held, (bt && tk == bt) ? SPD[tr] : 1.f);   // patchEnch.3: eficiencia
    return blockHardness(b) * (harvest ? 1.5f : 5.f) / speed;
}

// ---- rachaduras (10 estágios, geradas por código: raios a partir do centro) ----
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
static GLuint crackTex = 0;
static void initCrack() {
    static uint8_t px[160][16][4];
    memset(px, 0, sizeof px);
    // preferencia: as 10 texturas reais do resource pack (block/destroy_stage_0..9); so cai no gerado por codigo se o pack nao as tiver
    int fromPack = 0;
    for (int s = 0; s < 10; s++) { char nm[48]; snprintf(nm, sizeof nm, "block/destroy_stage_%d.png", s); uint8_t t[1024]; if (packTileRel(nm, t)) { memcpy(px[s * 16], t, 1024); fromPack++; } }
    if (fromPack < 10) { memset(px, 0, sizeof px);
    for (int s = 0; s < 10; s++)
        for (int r = 0; r < 12; r++) {
            const float a = hash2(r, 7, 91) * 6.2832f;
            const float sx = r < 6 ? 8.f : 3.f + hash2(r, 3, 5) * 10.f, sy = r < 6 ? 8.f : 3.f + hash2(r, 4, 5) * 10.f;
            const float len = r < 6 ? 2.f + s * 1.1f : std::max(0.f, (s - 3) * 1.2f);
            for (float t = 0; t <= len; t += .5f) {
                const int x = (int)(sx + cosf(a) * t), y = (int)(sy + sinf(a) * t);
                if (x < 0 || y < 0 || x > 15 || y > 15) continue;
                uint8_t* q = px[s * 16 + y][x]; q[0] = q[1] = q[2] = 64; q[3] = 255;   // patchBreak.1: cinza escuro opaco (o blend multiplicativo escurece o bloco, como no vanilla)
            }
        }
    }
    glGenTextures(1, &crackTex);
    glBindTexture(GL_TEXTURE_2D, crackTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 160, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
}
static void drawCrack(int x, int y, int z, int st) {
    if (!crackTex) initCrack();
    const float e = .0008f, a = -e, b = 1 + e, v0 = st * .1f, v1 = v0 + .1f;
    const float c[8][3] = {{a,a,a},{b,a,a},{b,a,b},{a,a,b},{a,b,a},{b,b,a},{b,b,b},{a,b,b}};
    static const int F[6][4] = {{0,1,2,3},{4,5,6,7},{0,3,7,4},{1,2,6,5},{0,1,5,4},{3,2,6,7}};
    static const float U[4][2] = {{0,1},{1,1},{1,0},{0,0}};
    glBindTexture(GL_TEXTURE_2D, crackTex);
    // patchBreak.1: MESMO metodo do vanilla (RenderGlobal.drawBlockDamageTexture): blend (DST_COLOR, SRC_COLOR) = 2*tex*fundo, cor (1,1,1,.5),
    // alpha test > 0.1 (descarta os texels transparentes) e polygon offset -3. Antes: SRC_ALPHA, que deixava as linhas do destroy_stage CLARAS (linha branca).
    rdEnable(GL_BLEND); rdBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f);
    rdDepthMask(GL_FALSE); rdEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-1.f, -1.f);   /* patchBreak.2: -3 puxava a rachadura pra frente do bloco (VIS) */
    glColor4f(1, 1, 1, .5f);
    glBegin(GL_QUADS);
    for (auto& f : F) for (int k = 0; k < 4; k++) { glTexCoord2f(U[k][0], v0 + (v1 - v0) * U[k][1]); glVertex3f(x + c[f[k]][0], y + c[f[k]][1], z + c[f[k]][2]); }
    glEnd();
    rdDisable(GL_POLYGON_OFFSET_FILL); rdDepthMask(GL_TRUE); rdDisable(GL_ALPHA_TEST); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDisable(GL_BLEND); glColor4f(1, 1, 1, 1);
}

// ---- partículas (EntityDiggingFX do 1.8.9): ticks de 20 Hz com interpolação, gravidade 0.04, arrasto 0.98, atrito 0.7 no chão ----
struct Part { float x, y, z, px, py, pz, vx, vy, vz, su, sv, sz, r, g, b, cr, cg, cb; int tile, age, max; bool gnd; };   // patchFix.28: cr/cg/cb = cor base (x.6); r/g/b = base x luz atual
static std::vector<Part> parts; static float partAcc = 0;
static float lightNear(int x, int y, int z) {   // brilho (luz do céu/bloco) nas células em volta do bloco
    static const int N[7][3] = {{0,0,0},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    int bsk = 0, bbl = 0;
    for (auto& n : N) {
        const int wx = x + n[0], wy = y + n[1], wz = z + n[2]; if (wy < 0 || wy >= CH) { bsk = 15; continue; }
        const int cx = fdiv(wx, CW), cz = fdiv(wz, CW); Chunk* c = getC(cx, cz);
        if (!c || !c->lc) { bsk = 15; continue; }
        const int v = c->lc[idx(wx - cx * CW, wy, wz - cz * CW)]; bsk = std::max(bsk, v >> 4); bbl = std::max(bbl, v & 15);
    }
    return std::max(ltabSky((float)bsk), bbl ? ltabB((float)bbl) : 0.f);   // céu com ciclo dia/noite, bloco sem
}
// textura e cor de um pedacinho: grama (camada de cima) e folhas usam a cor do bioma; o resto da grama é terra (como o MC)
static void partLook(int blk, bool top, float wx, float wz, int& tile, float* c) {
    c[0] = c[1] = c[2] = 1.f; tile = blk == GRASS ? TILES[DIRT][1] : isXB(blk) ? xbTile(blk) : blkTile(blk, 1);
    if (xbIsPlant(blk)) { const int pi = xbPIdx(blk); if (pi < 2 || (pi >= 14 && pi < 18)) { float g[3], l[3]; biomeTints(wx, wz, g, l); for (int k = 0; k < 3; k++) c[k] = g[k]; } return; }   // patchPart.1: mato/samambaia tingidos (antes: cinza)
    if ((blk == GRASS && top) || blk == LEAVES) {
        float g[3], l[3]; biomeTints(wx, wz, g, l);
        if (blk == GRASS) tile = TILES[GRASS][0];
        for (int k = 0; k < 3; k++) c[k] = blk == GRASS ? g[k] : l[k];
    }
}
static float partFloor(float br) { if (gDim == 1) return std::max(br, .30f); if (gDim == 2) return std::max(br, .45f); return br; }   // patchLight.4: piso de brilho no Nether/End
static void addPart(float x, float y, float z, float sx, float sy, float sz, int tile, const float* c, float br, float scl, float vel) {
    if (gPerf || !gOptParticles) return;   // patchRpi.5: sem particulas (modo desempenho ou opcao Particulas OFF)
    auto R = []() { return rand() / (float)RAND_MAX; };
    br = partFloor(br);
    Part p; p.x = p.px = x; p.y = p.py = y; p.z = p.pz = z;
    float mx = sx + (R() * 2.f - 1.f) * .4f, my = sy + (R() * 2.f - 1.f) * .4f, mz = sz + (R() * 2.f - 1.f) * .4f;
    const float f = (R() + R() + 1.f) * .15f, l = std::max(1e-4f, sqrtf(mx * mx + my * my + mz * mz));
    p.vx = mx / l * f * .4f * vel; p.vy = my / l * f * .4f * vel; p.vz = mz / l * f * .4f * vel;
    p.max = (int)(4.f / (R() * .9f + .1f)); p.age = 0; p.gnd = false;
    p.su = R() * 3.f / 4.f; p.sv = R() * 3.f / 4.f; p.sz = .1f * (R() * .5f + .5f) * scl; p.tile = tile;
    p.cr = c[0] * .6f; p.cg = c[1] * .6f; p.cb = c[2] * .6f; p.r = p.cr * br; p.g = p.cg * br; p.b = p.cb * br;
    parts.push_back(p);
}
static void spawnBreak(int bx, int by, int bz, int blk) {   // addBlockDestroyEffects: 4x4x4
    if (blk == AIR || isWater(blk) || parts.size() > 1500) return;
    const float br = lightNear(bx, by, bz);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) for (int k = 0; k < 4; k++) {
        const float dx = (i + .5f) / 4.f, dy = (j + .5f) / 4.f, dz = (k + .5f) / 4.f;
        int tile; float c[3]; partLook(blk, j == 3, bx + dx, bz + dz, tile, c);
        addPart(bx + dx, by + dy, bz + dz, dx - .5f, dy - .5f, dz - .5f, tile, c, br, 1.f, 1.f);
    }
}
static void spawnHit(int bx, int by, int bz, int nx, int ny, int nz, int blk) {   // addBlockHitEffects: 1 por tick, na face batida
    if (blk == AIR || isWater(blk) || parts.size() > 1500 || abs(nx) + abs(ny) + abs(nz) != 1) return;
    auto R = []() { return rand() / (float)RAND_MAX; };
    float x = bx + R() * .6f + .2f, y = by + R() * .6f + .2f, z = bz + R() * .6f + .2f;
    if (nx) x = bx + (nx > 0 ? 1.1f : -.1f);
    if (ny) y = by + (ny > 0 ? 1.1f : -.1f);
    if (nz) z = bz + (nz > 0 ? 1.1f : -.1f);
    int tile; float c[3]; partLook(blk, ny > 0, x, z, tile, c);
    addPart(x, y, z, 0.f, 0.f, 0.f, tile, c, lightNear(bx + nx, by + ny, bz + nz), .6f, .2f);
}
static void tickParts() {
    for (size_t i = 0; i < parts.size();) {
        Part& p = parts[i]; p.px = p.x; p.py = p.y; p.pz = p.z;
        if (p.age++ >= p.max) { parts[i] = parts.back(); parts.pop_back(); continue; }
        p.vy -= .04f;
        const float H = .1f;   // meia caixa 0.2
        { const float ny = p.y + p.vy; if (solid(getB((int)floorf(p.x), (int)floorf(ny + (p.vy < 0 ? -H : H)), (int)floorf(p.z)))) { p.gnd = p.vy < 0; p.vy = 0; } else { p.y = ny; p.gnd = false; } }
        { const float nx = p.x + p.vx; if (solid(getB((int)floorf(nx + (p.vx < 0 ? -H : H)), (int)floorf(p.y), (int)floorf(p.z)))) p.vx = 0; else p.x = nx; }
        { const float nz = p.z + p.vz; if (solid(getB((int)floorf(p.x), (int)floorf(p.y), (int)floorf(nz + (p.vz < 0 ? -H : H))))) p.vz = 0; else p.z = nz; }
        p.vx *= .98f; p.vy *= .98f; p.vz *= .98f; if (p.gnd) { p.vx *= .7f; p.vz *= .7f; }
        { const float br = partFloor(lightNear((int)floorf(p.x), (int)floorf(p.y), (int)floorf(p.z))); p.r = p.cr * br; p.g = p.cg * br; p.b = p.cb * br; }   // patchFix.28 (BUG-020): a luz segue a particula (antes: fixa do instante em que nasceu = escura ao sair da sombra e vice-versa); como EntityFX.getBrightnessForRender
        i++;
    }
}
static void updateParts(float dt) {
    partAcc += dt; for (int n = 0; partAcc >= .05f && n < 4; n++) { tickParts(); partAcc -= .05f; }
    if (partAcc > .05f) partAcc = 0.f;
}
static void drawParts(float yawDeg, float pitchDeg) {
    if (gPerf || !gOptParticles) { parts.clear(); return; }   // patchRpi.5
    if (parts.empty()) return;
    const float cy = cosf(yawDeg * .0174533f), sy = sinf(yawDeg * .0174533f), cp = cosf(pitchDeg * .0174533f), sp = sinf(pitchDeg * .0174533f);
    const float rx = cy, rz = -sy, ux = sy * sp, uy = cp, uz = cy * sp, pt = partAcc / .05f;
    static const int S[4][2] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f); glBegin(GL_QUADS);   // alpha test: texels transparentes (plantas) nao viram quadrado cinza
    for (const Part& p : parts) {
        const float x = p.px + (p.x - p.px) * pt, y = p.py + (p.y - p.py) * pt, z = p.pz + (p.z - p.pz) * pt;
        const float u0 = ((p.tile % ATL) + p.su) / (float)ATL, v0 = ((p.tile / ATL) + p.sv) / (float)ATLR, du = .25f / ATL, dv = .25f / ATLR;
        glColor3f(p.r, p.g, p.b);
        for (int k = 0; k < 4; k++) {
            glTexCoord2f(u0 + (S[k][0] > 0 ? du : 0.f), v0 + (S[k][1] > 0 ? 0.f : dv));
            glVertex3f(x + (rx * S[k][0] + ux * S[k][1]) * p.sz, y + uy * S[k][1] * p.sz, z + (rz * S[k][0] + uz * S[k][1]) * p.sz);
        }
    }
    glEnd(); rdDisable(GL_ALPHA_TEST);
}

// ---- nuvens (RenderGlobal.renderClouds do 1.8.9): máscara 256x256 que se repete, 1 texel = célula 12x12, espessura 4,
// deriva de 0,03 bloco/tick (para -X), alpha 0,8, topo 1.0 / base 0.7 / lados 0.9 e 0.8, pré-passe de profundidade ----
static uint8_t cloudMask[256][256];
static float tileNoise(float x, float z, int per, uint32_t sd) {   // value noise periódico (a máscara emenda sem costura)
    const int xi = (int)floorf(x), zi = (int)floorf(z); float fx = x - xi, fz = z - zi; fx = fx * fx * (3 - 2 * fx); fz = fz * fz * (3 - 2 * fz);
    auto h = [&](int a, int b) { return hash2(((a % per) + per) % per, ((b % per) + per) % per, sd); };
    return lerpf(lerpf(h(xi, zi), h(xi + 1, zi), fx), lerpf(h(xi, zi + 1), h(xi + 1, zi + 1), fx), fz);
}
static void initClouds() {
    {   // patchCloud.1: mascara REAL do pack (environment/clouds.png 256x256; alfa>0 = nuvem), como o Minecraft 1.12.2
        std::vector<uint8_t> px; int w = 0, h = 0;
        if (packRGBA("environment/clouds.png", px, w, h) && w >= 8 && h >= 8) {
            int n = 0;
            for (int z = 0; z < 256; z++) for (int x = 0; x < 256; x++) { const int sx = x * w / 256, sz = z * h / 256; const bool on = px[((size_t)sz * w + sx) * 4 + 3] > 10 && px[((size_t)sz * w + sx) * 4] > 10; cloudMask[z][x] = on; n += on; }
            if (n > 256 * 256 / 50) return;   // textura valida
        }
    }
    static float v[256 * 256]; static float srt[256 * 256];
    for (int z = 0; z < 256; z++) for (int x = 0; x < 256; x++) {
        float sum = 0.f, a = 1.f, tot = 0.f;
        for (int o = 0, per = 32; o < 3; o++, per *= 2, a *= .5f) { sum += a * tileNoise(x * per / 256.f, z * per / 256.f, per, 800 + o); tot += a; }
        srt[z * 256 + x] = v[z * 256 + x] = sum / tot;
    }
    std::nth_element(srt, srt + (int)(256 * 256 * .58f), srt + 256 * 256);   // ~42% de cobertura, como o clouds.png
    const float th = srt[(int)(256 * 256 * .58f)];
    for (int z = 0; z < 256; z++) for (int x = 0; x < 256; x++) cloudMask[z][x] = v[z * 256 + x] > th;
}
static void drawClouds(double camX, double camY, double camZ, float t) {
    static bool ready = false; if (!ready) { initClouds(); ready = true; }
    if (!gCloudTex) {   // mascara das nuvens como textura (unidade 7): o shader dos blocos projeta as sombras das nuvens no chao
        static uint8_t lum[256 * 256]; for (int i = 0; i < 256 * 256; i++) lum[i] = cloudMask[i >> 8][i & 255] ? 255 : 0;
        typedef void (APIENTRY* FnAT)(GLenum); static FnAT act = (FnAT)SDL_GL_GetProcAddress("glActiveTexture");
        if (act) {
            act(0x84C7); glGenTextures(1, &gCloudTex); glBindTexture(GL_TEXTURE_2D, gCloudTex); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, lum);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            act(0x84C0);
        }
    }
    const float CS = 12.f, TH = 4.f, Y0 = 128.f + gCloudH * 64.f, off = fmodf(t * .6f, 3072.f);   // 0,03 bloco/tick = 0,6 bloco/s
    gCloudOff = off;
    const int RC = gPerf ? 14 : 40;   // modo desempenho: nuvens 2D, raio curto (168 blocos); normal: 480 blocos de raio
    auto occ = [](int cx, int cz) { return cloudMask[cz & 255][cx & 255] != 0; };
    const int c0x = (int)floorf(((float)camX + off) / CS), c0z = (int)floorf((float)camZ / CS);
    const bool drawBot = camY < Y0 + 5.33, drawTop = camY >= Y0 - 4.67;   // mesmas condições de face do renderClouds (f9 > -5 / f9 <= 5)
    // geometria em display list (coordenadas relativas à célula central); só é refeita ao mudar de célula (a cada 12 blocos)
    // patchShader.4: camera DENTRO da camada (ou colada nela) pintava a tela inteira de uma cor so (nuvem opaca em volta); a opacidade cai perto da camada e volta a .8 a 3+ blocos
    const float cdy = camY < Y0 ? Y0 - (float)camY : (camY > Y0 + TH ? (float)camY - (Y0 + TH) : 0.f);
    const int aq = (int)(std::min(1.f, cdy / 3.f) * 4.f + .5f); const int rq = gAmbJava ? 8 : (int)((1.f - std::min(1.f, gRain)) * 8.f + .5f);   /* patchAmb.2: no Java as nuvens ficam na chuva, so cinzas (getCloudColour) */   /* patchWx.9: chuva esconde as nuvens 3D (some em 8 passos; 0 = nao desenha) */
    if (rq == 0) return;
    const float cAlpha = .8f * (.12f + .88f * aq / 4.f) * (rq / 8.f), cGray = 1.f - (1.f - rq / 8.f) * .3f;
    static GLuint list = 0; static int lx = 0, lz = 0, laq = -1, lrq = -1; static bool lt = false, lb = false;
    if (!list || lx != c0x || lz != c0z || lt != drawTop || lb != drawBot || laq != aq || lrq != rq) {
        if (!list) list = glGenLists(1);
        lx = c0x; lz = c0z; lt = drawTop; lb = drawBot; laq = aq; lrq = rq;
        glNewList(list, GL_COMPILE);
        glBegin(GL_QUADS);
        auto vtx = [&](float x, float y, float z, float sh) { glColor4f(sh * cGray, sh * cGray, sh * cGray, cAlpha); glVertex3f(x, y, z); };
        if (gPerf) {   // modo desempenho: nuvem 2D = UM plano na altura media, sem espessura nem laterais; celulas vizinhas em x viram uma faixa so (1 quad por faixa)
            const float yc = Y0 + TH * .5f;
            for (int dz = -RC; dz <= RC; dz++) {
                int run = 0, rx0 = 0;
                for (int dx = -RC; dx <= RC + 1; dx++) {
                    const bool on = dx <= RC && dx * dx + dz * dz <= RC * RC && occ(c0x + dx, c0z + dz);
                    if (on) { if (!run) rx0 = dx; run++; continue; }
                    if (run) { const float x0 = rx0 * CS, x1 = (rx0 + run) * CS, z0 = dz * CS, z1 = z0 + CS; vtx(x0, yc, z0, 1.f); vtx(x0, yc, z1, 1.f); vtx(x1, yc, z1, 1.f); vtx(x1, yc, z0, 1.f); vtx(x0, yc, z0, 1.f); vtx(x1, yc, z0, 1.f); vtx(x1, yc, z1, 1.f); vtx(x0, yc, z1, 1.f); run = 0; }   // dois lados: visivel de baixo e de cima
                }
            }
        }
        if (!gPerf)
        for (int dz = -RC; dz <= RC; dz++) for (int dx = -RC; dx <= RC; dx++) {
            const int cx = c0x + dx, cz = c0z + dz;
            if (dx * dx + dz * dz > RC * RC || !occ(cx, cz)) continue;
            const float x0 = dx * CS, x1 = x0 + CS, z0 = dz * CS, z1 = z0 + CS, y0 = Y0, y1 = Y0 + TH;
            if (drawTop) { vtx(x0, y1, z0, 1.f); vtx(x1, y1, z0, 1.f); vtx(x1, y1, z1, 1.f); vtx(x0, y1, z1, 1.f); }
            if (drawBot) { vtx(x0, y0, z0, .7f); vtx(x1, y0, z0, .7f); vtx(x1, y0, z1, .7f); vtx(x0, y0, z1, .7f); }
            if (!occ(cx - 1, cz)) { vtx(x0, y0, z0, .9f); vtx(x0, y0, z1, .9f); vtx(x0, y1, z1, .9f); vtx(x0, y1, z0, .9f); }
            if (!occ(cx + 1, cz)) { vtx(x1, y0, z0, .9f); vtx(x1, y0, z1, .9f); vtx(x1, y1, z1, .9f); vtx(x1, y1, z0, .9f); }
            if (!occ(cx, cz - 1)) { vtx(x0, y0, z0, .8f); vtx(x1, y0, z0, .8f); vtx(x1, y1, z0, .8f); vtx(x0, y1, z0, .8f); }
            if (!occ(cx, cz + 1)) { vtx(x0, y0, z1, .8f); vtx(x1, y0, z1, .8f); vtx(x1, y1, z1, .8f); vtx(x0, y1, z1, .8f); }
        }
        glEnd();
        glEndList();
    }
    float fogS = 38.f, fogE = 108.f; rdGetFogSE(fogS, fogE);
    rdFogf(GL_FOG_START, RC * CS * .30f); rdFogf(GL_FOG_END, RC * CS * .93f);   // névoa própria: as nuvens somem suavemente antes da borda (sem "aparecer do nada" ao andar)
    rdDisable(GL_TEXTURE_2D); rdEnable(GL_FOG); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    { const float cb = gSky.cloud, amb[4] = {gAmbJava ? gSky.cloudC[0] : cb, gAmbJava ? gSky.cloudC[1] : cb, gAmbJava ? gSky.cloudC[2] : cb, 1.f};   // cor das nuvens = World.getCloudColor (0.1 à noite .. 1.0 de dia): luz ambiente global multiplica as cores da lista
      rdEnable(GL_LIGHTING); rdEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb); }
    rdPushMatrix(); rdTranslatef((float)c0x * CS - off, 0.f, (float)c0z * CS);
    for (int pass = gPerf ? 1 : 0; pass < 2; pass++) {   // modo desempenho: sem pre-passe de profundidade (plano unico); normal: pre-passe
  // de profundidade (só a superfície mais próxima é pintada) + cor
        if (pass == 0) { rdColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); rdDepthMask(GL_TRUE); }
        else { rdColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); rdDepthMask(GL_FALSE); }
        glCallList(list);
    }
    rdPopMatrix();
    rdDisable(GL_COLOR_MATERIAL); rdDisable(GL_LIGHTING);
    rdColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D);
    rdFogf(GL_FOG_START, fogS); rdFogf(GL_FOG_END, fogE);
}

// fracao (0..1) do sol tapada pelas nuvens: 5 raios (centro + 4 em volta, ~12 graus) da camera ate o sol atravessando a camada de nuvens (analitico, nao depende do buffer de profundidade)
static float cloudSunOcc(const double* cam, const float* sd) {
    const float CS = 12.f, Y0 = 128.f, TH = 4.f; const float cx = (float)cam[0], cy = (float)cam[1], cz = (float)cam[2];
    if (cy > Y0 + TH) return 0.f;
    static const float OA[5] = {0.f, .2f, -.2f, 0.f, 0.f}, OB[5] = {0.f, 0.f, 0.f, .2f, -.2f}, W[5] = {.4f, .15f, .15f, .15f, .15f};
    float tot = 0.f;
    for (int r = 0; r < 5; r++) {
        float dx = sd[0] + OA[r] * sd[1], dy = sd[1] - OA[r] * sd[0], dz = OB[r]; const float L = sqrtf(dx * dx + dy * dy + dz * dz); dx /= L; dy /= L; dz /= L;
        if (dy < .03f) continue;
        const float t0 = std::max(0.f, (Y0 - cy) / dy), t1 = (Y0 + TH - cy) / dy, hd = t0 * sqrtf(dx * dx + dz * dz);
        if (hd > 440.f) continue;
        int n = 0; for (int i = 0; i < 12; i++) { const float t = t0 + (t1 - t0) * (i + .5f) / 12.f; n += cloudMask[(int)floorf((cz + dz * t) / CS) & 255][(int)floorf((cx + dx * t + gCloudOff) / CS) & 255]; }
        const float fr = n / 12.f, v = clampf2((fr - .1f) / .8f, 0.f, 1.f), fade = 1.f - clampf2((hd - 250.f) / 190.f, 0.f, 1.f);
        tot += W[r] * v * v * (3.f - 2.f * v) * .92f * fade;
    }
    return tot;
}
