// ceu (domo) e panorama do menu (movido de main_loop.inc em patchOrg.4)
static void drawSkyDome(const float* top, const float* hor) {
    static const float el[7] = {-90.f, 0.f, 8.f, 20.f, 38.f, 60.f, 90.f};
    rdDisable(GL_TEXTURE_2D); rdDisable(GL_FOG); rdDisable(GL_DEPTH_TEST); rdDisable(GL_ALPHA_TEST); rdDepthMask(GL_FALSE);
    const int N = gPerf ? 12 : 24; const float R = 100.f;   // modo desempenho: domo com metade dos segmentos; R tem que ser < plano far (200), senão o domo é cortado e vira uma bola
    glBegin(GL_QUADS);
    for (int b = 0; b < 6; b++) for (int j = 0; j < N; j++) {
        const float a0 = j * 6.2831853f / N, a1 = (j + 1) * 6.2831853f / N;
        const float ea[4] = {el[b], el[b], el[b + 1], el[b + 1]}, aa[4] = {a0, a1, a1, a0};
        for (int k = 0; k < 4; k++) {
            const float e = ea[k] * .0174533f, f = ea[k] <= 0.f ? 0.f : powf(ea[k] / 90.f, .55f);
            glColor3f(hor[0] + (top[0] - hor[0]) * f, hor[1] + (top[1] - hor[1]) * f, hor[2] + (top[2] - hor[2]) * f);
            glVertex3f(R * cosf(e) * sinf(aa[k]), R * sinf(e), -R * cosf(e) * cosf(aa[k]));
        }
    }
    glEnd();
    rdDepthMask(GL_TRUE); rdEnable(GL_TEXTURE_2D);
}
// panorama do titulo: mundo gerado (seed fixa) visto de cima, girando devagar
#ifdef __ANDROID__
#define PANO_RAD 4   // celular: panorama do menu com 81 chunks (em vez de 289) = abre o menu bem mais rapido e usa menos RAM/GPU
#else
#ifdef BL_RPI
static void blSwap(SDL_Window* w) {   // patchRpiOpt.7: copia o quadro 400x250 e estica para a janela (bilinear, mantem proporcao)
    if (gLowOn) {
        static GLuint tex = 0; const int rw = gLowRW, rh = gLowRH;
        const float s = std::min(rw / (float)gBW, rh / (float)gBH); const int dw = (int)(gBW * s), dh = (int)(gBH * s), dx = (rw - dw) / 2, dy = (rh - dh) / 2;
        skyShaderOff();
        { typedef void (*FnAT)(GLenum); static FnAT at = (FnAT)SDL_GL_GetProcAddress("glActiveTexture"); if (at) at(0x84C0); }   // GL_TEXTURE0
        if (!tex) { glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); glTexImage2D(GL_TEXTURE_2D, 0, 0x1907, 512, 512, 0, 0x1907, GL_UNSIGNED_BYTE, nullptr); }
        glBindTexture(GL_TEXTURE_2D, tex); glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, blVW(), blVH());
        rdDisable(GL_DEPTH_TEST); rdDisable(GL_BLEND); rdDisable(GL_ALPHA_TEST); rdDisable(GL_FOG); rdDisable(GL_LIGHTING); rdDisable(GL_CULL_FACE); rdDisable(GL_SCISSOR_TEST); rdEnable(GL_TEXTURE_2D); rdDepthMask(GL_TRUE);
        rdViewport(0, 0, rw, rh); glClearColor(0.f, 0.f, 0.f, 1.f); glClear(GL_COLOR_BUFFER_BIT);
        rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdLoadIdentity(); rdOrtho(0, rw, 0, rh, -1, 1); rdMatrixMode(GL_MODELVIEW); rdPushMatrix(); rdLoadIdentity();
        const float u1 = (blVW() - .5f) / 512.f, v1 = (blVH() - .5f) / 512.f, u0 = .5f / 512.f, v0 = .5f / 512.f; glColor3f(1.f, 1.f, 1.f);
        glBegin(GL_QUADS); glTexCoord2f(u0, v0); glVertex2f((float)dx, (float)dy); glTexCoord2f(u1, v0); glVertex2f((float)(dx + dw), (float)dy); glTexCoord2f(u1, v1); glVertex2f((float)(dx + dw), (float)(dy + dh)); glTexCoord2f(u0, v1); glVertex2f((float)dx, (float)(dy + dh)); glEnd();
        rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW); rdPopMatrix();
    }
    (SDL_GL_SwapWindow)(w); vkBootOk();
}
#endif
#ifdef BL_RPI
#define PANO_RAD 2   // Raspberry: panorama minimo (quase sem custo)
#else
#define PANO_RAD 4   /* patchFast.2: era 8 (289 chunks gerados na abertura); 81 como no celular */
#endif
#endif
static bool panoOn = false; static float panoT = 0.f;

// terreno do titulo: vale com rio/lago e colinas (parecido com o panorama classico do Minecraft); esculpido sobre o mundo da seed fixa
static unsigned pvH = 0; static float pvPh = 0.f, pvPh2 = 1.f, pvAmp = 16.f, pvF = .04f, pvOx = 0.f, pvOz = 0.f, pvYaw = 0.f, pvTreeD = .011f;   // variacao do panorama: muda a cada abertura do titulo
static float panoHash(int x, int z) { unsigned h = (unsigned)x * 374761393u + (unsigned)z * 668265263u + pvH; h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16; return (h & 0xffff) / 65535.f; }
static float panoNoise(float x, float z) { const int xi = (int)floorf(x), zi = (int)floorf(z); float fx = x - xi, fz = z - zi; fx = fx * fx * (3 - 2 * fx); fz = fz * fz * (3 - 2 * fz);
    const float a = panoHash(xi, zi), b = panoHash(xi + 1, zi), c = panoHash(xi, zi + 1), d = panoHash(xi + 1, zi + 1); return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fz; }
static float panoFbm(float x, float z) { return panoNoise(x, z) * .55f + panoNoise(x * 2.1f + 17.f, z * 2.1f + 5.f) * .30f + panoNoise(x * 4.3f + 3.f, z * 4.3f + 41.f) * .15f; }   // 0..1
static int panoHeight(int x, int z, bool* lake) {   // cenario do titulo: margem gramada no centro (camera), lagos, colinas e montanhas ao redor
    const float dx = x - 8.5f, dz = z - 8.5f, r = sqrtf(dx * dx + dz * dz);
    float k = std::min(1.f, std::max(0.f, (r - 6.f) / 22.f)); k = k * k * (3 - 2 * k);   // 0 no centro, 1 longe
    const float hill = (panoFbm(x * .028f + pvOx, z * .028f + pvOz) - .5f) * 2.f;       // -1..1
    const float far = std::min(1.f, std::max(0.f, (r - 40.f) / 60.f));
    float h = SEA + 2.f + k * (hill * (9.f + pvAmp * .5f) + far * 16.f);
    const float lk = panoNoise(x * .022f + pvOx * .3f + 91.f, z * .022f + pvOz * .3f + 7.f);   // lagos
    if (lk < .42f) h -= (.42f - lk) * 70.f * k;
    h += (panoNoise(x * .2f, z * .2f) - .5f) * 1.6f * k;   // textura fina
    int hi = (int)floorf(h + .5f); hi = std::max(8, std::min(CH - 20, hi)); if (lake) *lake = hi < SEA; return hi;
}
static void panoFill(Chunk* c, int cx, int cz) {
    for (int lz = 0; lz < CW; lz++) for (int lx = 0; lx < CW; lx++) {
        const int x = cx * CW + lx, z = cz * CW + lz; bool lake = false; const int h = panoHeight(x, z, &lake);
        for (int y = 0; y <= h; y++) { int b = STONE; if (y == 0) b = BEDROCK; else if (y == h) b = h > SEA + 0 ? GRASS : (h >= SEA - 1 ? SAND : GRAVEL); else if (y >= h - 3) b = (h > SEA ? DIRT : SAND); c->b[idx(lx, y, lz)] = (BT)b; }
        for (int y = h + 1; y <= SEA; y++) c->b[idx(lx, y, lz)] = (BT)WATER;
        if (h < SEA) c->hasWater = true;
        // arvores: so dentro do chunk (sem cruzar a borda) e longe do centro
        if (h > SEA + 1 && lx >= 3 && lx <= 12 && lz >= 3 && lz <= 12 && hypotf(x - 8.5f, z - 8.5f) > 9.f && panoHash(x * 7 + 3, z * 13 + 5) < pvTreeD * .6f) {
            const int th = 4 + (int)(panoHash(x, z) * 3);
            for (int y = h + 1; y <= h + th; y++) c->b[idx(lx, y, lz)] = (BT)LOG;
            for (int yy = th - 2; yy <= th + 1; yy++) { const int rr = yy >= th ? 1 : 2; for (int ddz = -rr; ddz <= rr; ddz++) for (int ddx = -rr; ddx <= rr; ddx++) { if (rr == 2 && abs(ddx) == 2 && abs(ddz) == 2) continue; if (h + yy < CH && c->b[idx(lx + ddx, h + yy, lz + ddz)] == AIR) c->b[idx(lx + ddx, h + yy, lz + ddz)] = (BT)LEAVES; } }
        }
    }
}
static int panoStage = 0, panoSteps = 0;
static unsigned gPanoSeedBase = 0x5EED2026u;   // semente do panorama: o titulo usa a fixa; a tela de geracao usa outra (terreno diferente)
static void panoBegin() {
    { unsigned sd = gPanoSeedBase; sd ^= sd >> 15;   /* patchMenu.2: semente FIXA = mesmo terreno do titulo sempre */ sd *= 2246822519u; sd ^= sd >> 13; auto fr = [&]() { sd = sd * 1664525u + 1013904223u; return ((sd >> 8) & 0xffff) / 65535.f; };
      pvH = sd; pvPh = fr() * 6.28f; pvPh2 = fr() * 6.28f; pvAmp = 10.f + fr() * 10.f; pvF = .03f + fr() * .025f; pvOx = fr() * 500.f; pvOz = fr() * 500.f; pvTreeD = .006f + fr() * .014f; pvYaw = (fr() < .5f ? 0.f : 180.f) + (fr() - .5f) * 90.f; }   // terreno novo a cada abertura do titulo
    resetWorldData(); WORLD_SEED = pvH | 1u; applyGm(1); gHard = false; RAD = PANO_RAD; gPanoGen = true;   // panorama do menu: raio fixo (8 no PC, 4 no celular), nao importa a distancia configurada
    panoOn = true; panoT = 0.f;
    if (!gPerf) {
    gPanoTint = true; panoStage = 1; panoSteps = 0;   // patchPerf.1: gera/esculpe/remalha em fatias de tempo (antes: tudo de uma vez = tela congelada ao voltar ao titulo)
    } else panoStage = 0;
}
static void panoStep(double budgetMs) {   // uma fatia por quadro; budgetMs < 0 = sem limite (loading)
    if (panoStage == 0) return;
    const int mg = budgetMs < 0 ? 100000 : 6, mm = budgetMs < 0 ? 100000 : 3;
    stream(0, 0, mg, mm, budgetMs); panoSteps++;
    if (gKick && panoSteps < 4000) return;   // ainda ha chunks por gerar/malhar
    gPanoGen = false;   // gerou tudo (fatias seguintes so remalham)
   // esculpe o vale (marca os chunks sujos); proximas fatias remalham
    panoStage = 0; gPanoTint = false;
}
static void panoInit() { panoBegin(); if (panoStage) { while (panoStage) panoStep(-1.0); } }
static bool gInvOpenNow = false;   // espelho de `invOpen` (local do main) para o cursor do controle
static bool gpSlotPoints(int w, int h, std::vector<std::pair<float, float>>& out, float& rad) {   // patchPad.2: slots do inventario/bau/bancada abertos (para o cursor do controle grudar)
    if (!gInvOpenNow || gScreen) return false;
    const InvUI u = invLayout(w, h); for (int id : activeSlots(u)) { int x, y; if (slotUnits(u, id, &x, &y)) out.push_back({u.ox + (x + 8) * u.gs, u.oy + (y + 8) * u.gs}); }
    rad = 9.f * u.gs; return !out.empty();
}
static void drawPanorama(int w, int h, float dt) {
    if (gPerf) {
    panoT += dt; blViewport(w, h); glClearColor(.09f, .09f, .11f, 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); return;   // Raspberry: fundo escuro liso, sem panorama
    }
    panoT += dt;
    static float parX = 0.f, parY = 0.f;   // patchMenu.4: parallax do titulo: a camera acompanha o mouse/dedo (suave, +-3 graus de yaw, +-1.5 de pitch); dt=0 (captura de cache) nao aplica
    if (dt > 0.f && w > 0 && h > 0) { const float tx = std::max(-1.f, std::min(1.f, ((float)gMX / w - .5f) * 2.f)), ty = std::max(-1.f, std::min(1.f, ((float)gMY / h - .5f) * 2.f)), k = std::min(1.f, dt * 4.f); parX += (tx - parX) * k; parY += (ty - parY) * k; }
    const float parYaw = dt > 0.f ? parX * 3.f : 0.f, parPitch = dt > 0.f ? parY * 1.5f : 0.f;
    const float yawD = panoT * 2.f + pvYaw + parYaw;   // patchMenu.1: fundo FIXO (sem rotacao/balanco/nuvens andando)
    int gy = SEA; for (int y = CH - 1; y >= 0; y--) if (solid(getB(8, y, 8))) { gy = y; break; }
    const float cy = (float)SEA + 3.5f;   // camera baixa, perto do chao (como o menu oficial)
    const float pitchD = 5.f + 10.f * sinf(panoT * .05f);   // balanca suave do pitch, como o panorama do Minecraft
    // panorama NAO muda com F7/shaders: congela estado global (restaurado no fim)
    const bool sv_pk = gShaderPackOn, sv_sd = gShadowDraw, sv_rf = gRfOn, sv_fx = gFxOn; const int sv_dim = gDim; const float sv_ang = gSky.angle; float sv_dyn[4][4]; memcpy(sv_dyn, gDyn, sizeof sv_dyn);
    gShaderPackOn = false; gShadowDraw = false; gRfOn = false; gFxOn = false; gDim = 0; gSky.angle = 0.f; memset(gDyn, 0, sizeof gDyn);
    if (gShadowsOn) skySelectProg(false);   // F7 ligado: usa o shader enxuto no fundo (mesmo visual, sem custo de sombras)
    blViewport(w, h);
    const float sky[4] = {.74f, .86f, 1.f, 1.f}, top[3] = {.27f, .50f, .90f};
    glClearColor(sky[0], sky[1], sky[2], 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const float n = .05f, t = n * tanf(50.f * .0174533f), r = t * w / (float)h;
    rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdFrustum(-r, r, -t, t, n, 700);
    rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdRotatef(pitchD + parPitch, 1, 0, 0); rdRotatef(yawD, 0, 1, 0);   // pitch 18 graus para baixo (era 8): olha mais de cima, como o menu oficial
    drawSkyDome(top, sky);
    rdTranslatef(-8.5f, -cy, -8.5f);
    rdEnable(GL_DEPTH_TEST); rdDepthFunc(GL_LEQUAL); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.02f);
    rdEnable(GL_FOG); rdFogi(GL_FOG_MODE, GL_LINEAR); rdFogf(GL_FOG_START, PANO_RAD * 4.75f); rdFogf(GL_FOG_END, PANO_RAD * 13.5f); rdFogfv(GL_FOG_COLOR, sky);   // neblina proporcional ao raio (some a borda do mundo)
    glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D); glColor3f(1, 1, 1);
    glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
    skyShaderOn(0.f, 1.f, 1.f);   // panorama do título: sempre meio-dia
    for (int pass = 0; pass < 2; pass++) {
        if (pass) { rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDepthMask(GL_FALSE); skyShaderOn(1.f, 1.f, 1.f); }
        for (int dz = -RAD; dz <= RAD; dz++) for (int dx = -RAD; dx <= RAD; dx++) {
            Chunk* c = getC(dx, dz); if (!c) continue;
            const int cnt = pass ? c->nwv : c->nv; if (cnt <= 0) continue;
            if (useVBO) {
                if (c->gl || c->glw) uploadChunk(c);
                pBindBuffer(GL_ARRAY_BUFFER, pass ? c->wvbo : c->vbo);
                glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)offsetof(V, x)); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)offsetof(V, u)); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)offsetof(V, r));
            } else { std::vector<V>& m = pass ? c->wmesh : c->mesh; const V* v = m.data(); glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r); }
            glDrawArrays(GL_QUADS, 0, (GLsizei)cnt);
        }
    }
    skyShaderOff();
    rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
    if (!gPerf) {
    drawClouds(8.5, cy, 8.5, panoT);   // nuvens como no menu oficial
    rdDepthMask(GL_TRUE); rdDisable(GL_BLEND);
    }

    rdDisable(GL_FOG);
    if (gShadowsOn) skySelectProg(true);
    gShaderPackOn = sv_pk; gShadowDraw = sv_sd; gRfOn = sv_rf; gFxOn = sv_fx; gDim = sv_dim; gSky.angle = sv_ang; memcpy(gDyn, sv_dyn, sizeof sv_dyn);
}
static bool touchShadowOn() { return gShadowsOn; }   // usado pelo HUD touch (botao de luz acende quando as sombras estao ligadas)
