// ================= sombras: shadow mapping do sol (estilo shaderpack do OptiFine) =================
// A cada ~0,5 s (ou quando a camera anda 20 blocos) a vizinhanca da camera e desenhada UMA vez, so em profundidade, do ponto de vista do sol,
// numa textura de 2048x2048 (FBO). O shader dos chunks (sky_cycle.inc) compara a profundidade de cada pixel com essa textura (PCF por hardware + 5 amostras).
// A GPU faz o desenho e a comparacao; a CPU so monta 3 matrizes por atualizacao. F7 liga/desliga. Sem FBO/shader: avisa e nada muda.
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_TEXTURE_COMPARE_MODE
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_COMPARE_R_TO_TEXTURE 0x884E
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_NONE
#define GL_NONE 0
#endif
typedef void (APIENTRY* FnGenFB)(GLsizei, GLuint*); typedef void (APIENTRY* FnBindFB)(GLenum, GLuint);
typedef void (APIENTRY* FnFBTex)(GLenum, GLenum, GLenum, GLuint, GLint); typedef GLenum (APIENTRY* FnChkFB)(GLenum); typedef void (APIENTRY* FnActTex)(GLenum);
static FnGenFB pGenFB = nullptr; static FnBindFB pBindFB = nullptr; static FnFBTex pFBTex = nullptr; static FnChkFB pChkFB = nullptr; static FnActTex pActTex = nullptr;
// ---- luz do sol/lua (sombras antigas removidas; as novas entram nos proximos lotes) ----
static bool shReady = false;
// ---- sombras do sol por MAPA DE ALTURAS (sem passada extra de geometria) ----
// O sol anda so no plano X-Y: todo raio corre ao longo de X. Para cada linha o "horizonte" S(x) = max_j (H(j) - |j-x|*tan(elevacao)) sai de um maximo
// de sufixo (O(N) por linha). A textura guarda S, o topo da propria coluna e a distancia do bloqueador (penumbra); o shader faz 4 leituras por pixel.
static const int HS_GX = HS_N + 2 * HS_M;
struct HsCol { uint8_t h[CW * CW], h2[CW * CW], ws[CW * CW], wd[CW * CW]; Uint32 t = 0; };   // h = topo opaco; ws = y da superficie da agua + 1 (0 = sem agua); wd = profundidade da agua (blocos)
static std::unordered_map<int64_t, HsCol> gHsCols;
static GLuint gHsTex = 0, gWdTex = 0; static float gHsLx = 9.f, gHsLc = 9.f; static Uint32 gHsLast = 0; static bool gHsInit = false;
static std::vector<uint8_t> gHsH, gHsH2, gHsPix, gWdPix;   // patchSombra.8: gHsH2 = topo SO de solidos (sem folhas), vai no canal B da textura da agua
static void hsScan(int cx, int cz, HsCol& col, Uint32 now) {   // h = y do topo + 1 da coluna que projeta sombra (folhas sim; agua, plantas e vidro nao)
    col.t = now ? now : 1u; Chunk* c = getC(cx, cz); if (!c) { memset(col.h, 0, sizeof col.h); memset(col.h2, 0, sizeof col.h2); memset(col.ws, 0, sizeof col.ws); memset(col.wd, 0, sizeof col.wd); return; }
    const int top = chunkTop(c);
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
        int y = top; for (; y >= 0; y--) { const int b = c->b[idx(x, y, z)]; if (b && !isWater(b) && lightOpac(b) > 0) break; }
        col.h[z * CW + x] = (uint8_t)std::min(255, y + 1);
        { int y2 = top; for (; y2 >= 0; y2--) { const int b = c->b[idx(x, y2, z)]; if (b && !isWater(b) && !isLeafB(b) && lightOpac(b) > 0) break; } col.h2[z * CW + x] = (uint8_t)std::min(255, y2 + 1); }
        {   int yw = -1; for (int yy = top; yy >= 0; yy--) { const int b = c->b[idx(x, yy, z)]; if (isWater(b)) { yw = yy; break; } if (b && !isLeafB(b) && lightOpac(b) > 0) break; }
            int dd = 0; if (yw >= 0) for (int yy = yw; yy >= 0 && isWater(c->b[idx(x, yy, z)]); yy--) dd++;
            col.ws[z * CW + x] = (uint8_t)(yw >= 0 ? std::min(255, yw + 1) : 0); col.wd[z * CW + x] = (uint8_t)std::min(255, dd); }
    }
}
// lsx/lcs = componentes X e Y da direcao PARA a luz (sol de dia, lua de noite)
static void hsUpdate(const double* ec, float lsx, float lcs) {
    if (!gHsTex || !pActTex) return;
    const Uint32 now = SDL_GetTicks(); const float px = (float)ec[0], pz = (float)ec[2];
    const bool moved = !gHsInit || fabsf(px - (gHsOx + HS_N * .5f)) > 12.f || fabsf(pz - (gHsOz + HS_N * .5f)) > 12.f;
    if (!moved && fabsf(lsx - gHsLx) + fabsf(lcs - gHsLc) <= .006f && now - gHsLast <= 900u) return;
    if (moved) { gHsOx = floorf((px - HS_N * .5f) / CW) * CW; gHsOz = floorf((pz - HS_N * .5f) / CW) * CW; }
    gHsInit = true; gHsLast = now ? now : 1u; gHsLx = lsx; gHsLc = lcs;
    if (gHsCols.size() > 700) gHsCols.clear();
    const bool all = gHsCols.empty(); int budget = 40;
    const int ocx = (int)floorf(gHsOx / CW), ocz = (int)floorf(gHsOz / CW), pcx = fdiv((int)floorf(px), CW), pcz = fdiv((int)floorf(pz), CW), nc = HS_N / CW, mc = HS_M / CW;
    gHsH.assign((size_t)HS_GX * HS_N, 0); gHsH2.assign((size_t)HS_GX * HS_N, 0); gWdPix.assign((size_t)HS_N * HS_N * 4, 0);
    for (int cz = ocz; cz < ocz + nc; cz++) for (int cx = ocx - mc; cx < ocx + nc + mc; cx++) {
        HsCol& col = gHsCols[key(cx, cz)]; const bool nearC = abs(cx - pcx) <= 2 && abs(cz - pcz) <= 2;
        if (col.t == 0 || ((all || budget > 0) && now - col.t > (nearC ? 800u : 6000u))) { hsScan(cx, cz, col, now); budget--; }
        const int gx0 = (cx - ocx + mc) * CW, gz0 = (cz - ocz) * CW;
        for (int z = 0; z < CW; z++) { memcpy(&gHsH[(size_t)(gz0 + z) * HS_GX + gx0], &col.h[z * CW], CW); memcpy(&gHsH2[(size_t)(gz0 + z) * HS_GX + gx0], &col.h2[z * CW], CW); }
        if (cx >= ocx && cx < ocx + nc) for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) { uint8_t* o = &gWdPix[((size_t)(gz0 + z) * HS_N + (cx - ocx) * CW + x) * 4]; o[0] = col.ws[z * CW + x]; o[1] = col.wd[z * CW + x]; o[3] = 255; }
    }
    const int dirx = lsx >= 0.f ? 1 : -1; const float tanE = std::min(40.f, lcs / std::max(fabsf(lsx), .02f));
    gHsPix.assign((size_t)HS_N * HS_N * 4, 0);
    for (int z = 0; z < HS_N; z++) {
        const uint8_t* row = &gHsH[(size_t)z * HS_GX]; uint8_t* out = &gHsPix[(size_t)z * HS_N * 4];
        float m = -1e9f; int jm = 0;
        auto emit = [&](int gx) {   // m = melhor bloqueador entre os que ja passaram
            if (gx < HS_M || gx >= HS_M + HS_N) return;
            float Sv = dirx > 0 ? m + gx * tanE : m - gx * tanE; const int d = abs(jm - gx);
            if (m < -1e8f || Sv <= 0.f) Sv = 0.f; Sv = std::min(Sv, 255.f);
            const int ip = (int)Sv, fr = (int)((Sv - ip) * 255.f + .5f); uint8_t* o = out + (size_t)(gx - HS_M) * 4;
            o[0] = (uint8_t)ip; o[1] = (uint8_t)std::min(255, fr); o[2] = row[gx]; o[3] = (uint8_t)(Sv > 0.f ? std::min(255, d) : 0);
        };
        if (dirx > 0) { for (int gx = HS_GX - 1; gx >= HS_M; gx--) { emit(gx); const float v = row[gx] - gx * tanE; if (v > m) { m = v; jm = gx; } } }
        else          { for (int gx = 0; gx < HS_M + HS_N; gx++)   { emit(gx); const float v = row[gx] + gx * tanE; if (v > m) { m = v; jm = gx; } } }
    }
    for (int z = 0; z < HS_N; z++) {   // patchSombra.8: mesmo horizonte, mas so com solidos (folhas ficam de fora): a sombra de folha e mais leve que a de pedra
        const uint8_t* row = &gHsH2[(size_t)z * HS_GX]; float m = -1e9f;
        auto emit = [&](int gx) { if (gx < HS_M || gx >= HS_M + HS_N) return; float Sv = dirx > 0 ? m + gx * tanE : m - gx * tanE; if (m < -1e8f || Sv <= 0.f) Sv = 0.f; gWdPix[((size_t)z * HS_N + (gx - HS_M)) * 4 + 2] = (uint8_t)(int)(std::min(Sv, 255.f) + .5f); };
        if (dirx > 0) { for (int gx = HS_GX - 1; gx >= HS_M; gx--) { emit(gx); m = std::max(m, row[gx] - gx * tanE); } }
        else          { for (int gx = 0; gx < HS_M + HS_N; gx++)   { emit(gx); m = std::max(m, row[gx] + gx * tanE); } }
    }
    pActTex(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, gHsTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HS_N, HS_N, 0, GL_RGBA, GL_UNSIGNED_BYTE, gHsPix.data());
    if (gWdTex) { pActTex(GL_TEXTURE0 + 2); glBindTexture(GL_TEXTURE_2D, gWdTex); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HS_N, HS_N, 0, GL_RGBA, GL_UNSIGNED_BYTE, gWdPix.data()); }   // patchWater.7: mapa de profundidade da agua (unidade 2)
    pActTex(GL_TEXTURE0);
}
static bool shadowInit() {   // so confirma que o programa de shader realista compilou e pega os ponteiros de FBO (reflexao da agua)
    if (shReady) return true;
    pGenFB = (FnGenFB)SDL_GL_GetProcAddress("glGenFramebuffers"); pBindFB = (FnBindFB)SDL_GL_GetProcAddress("glBindFramebuffer");
    pFBTex = (FnFBTex)SDL_GL_GetProcAddress("glFramebufferTexture2D"); pChkFB = (FnChkFB)SDL_GL_GetProcAddress("glCheckFramebufferStatus"); pActTex = (FnActTex)SDL_GL_GetProcAddress("glActiveTexture");
    if (!gShadowShader) { logWarn("Efeitos realistas: indisponiveis (shader nao compilou)"); return false; }
    if (!gHsTex && pActTex) {   // patchShadow.3: textura do mapa de alturas (unidade 1)
        pActTex(GL_TEXTURE0 + 1); glGenTextures(1, &gHsTex); glBindTexture(GL_TEXTURE_2D, gHsTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HS_N, HS_N, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        pActTex(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);
    }
    if (!gWdTex && pActTex) {
        pActTex(GL_TEXTURE0 + 2); glGenTextures(1, &gWdTex); glBindTexture(GL_TEXTURE_2D, gWdTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HS_N, HS_N, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        pActTex(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);
    }
    if (!gHsTex) { logWarn("Sombras: mapa de alturas indisponivel (glActiveTexture ausente)"); return false; }
    shReady = true; return true;
}
// direcao e forca da luz do quadro (sol de dia, lua de noite); o shader aplica terminador, tom e nuvens
static void shadowFrame(const double* cam, bool underFluid, const double* ec) {
    (void)cam;
    gShadowDraw = false; gShadowAct = false; gFxOn = gShadowsOn && shReady;
    if (!gShadowsOn || !shReady) return;
    const float th = gSky.angle * 6.2831853f; float cs = cosf(th), sn = sinf(th);   // sol = (-sin, cos, 0): mesma conta do glRotate de drawSkyFull
    gSunDir[0] = -sn; gSunDir[1] = cs; gSunDir[2] = 0.f;
    gSunStr = clampf2((cs - .03f) / .2f, 0.f, 1.f);   // some ao entardecer
    float lsx = -sn, lcs = cs;   // direcao PARA a luz no plano X-Y (sol); a lua inverte abaixo
    {   static const float PF[8] = {1.f, .75f, .5f, .25f, 0.f, .25f, .5f, .75f};   // de noite a LUA e a fonte de luz (forca pela fase)
        const float mk = clampf2((-cs - .03f) / .2f, 0.f, 1.f) * (.10f + .30f * PF[gSky.moonPhase & 7]);
        if (mk > gSunStr) { gSunStr = mk; lsx = -lsx; lcs = -lcs; }
    }
    if (gSunStr < .01f || underFluid) return;
    gShadowDraw = true; gShadowAct = true; hsUpdate(ec, lsx, lcs);
}
template <class F> static void shadowToggle(F& say) {
    if (gPerf) { say("Sombras desativadas no modo desempenho"); return; }
    if (!shadowInit()) { say("Sombras indisponiveis nesta placa/driver (veja logs/latest.log)"); return; }
    gShadowsOn = !gShadowsOn; skySelectProg(gShadowsOn);   // celular: troca entre o shader enxuto e o de sombras
#ifdef __ANDROID__
    gSt["shd.mob"] = gShadowsOn ? 1 : 0; optSave();   // lembra a escolha (options.cfg)
#endif
    static bool autoPack = false;   // F7 liga junto o pacote interno (sol realista, agua com reflexo, god rays, lens flare) se o jogador nao escolheu um
    if (gShadowsOn && !BL_NO_FANCY && gsi("shd.sel", 0) == 0) { gSt["shd.sel"] = 1; autoPack = true; }
    else if (!gShadowsOn && autoPack) { gSt["shd.sel"] = 0; autoPack = false; }
    gShaderPackOn = !BL_NO_FANCY && (gsi("shd.sel", 0) > 0 || gShadowsOn);
#ifdef BL_GLES_NATIVE
    gnFancy = !BL_NO_FANCY && gsi("shd.sel", 0) > 0 && !gShadowsOn;   // sem sombras no backend nativo: so a escolha explicita do pacote liga a agua realista
#endif
    say(gShadowsOn ? "Sombras + sol e agua realistas: ligados" : "Sombras e efeitos realistas: desligados");
    logInfo("Sombras %s", gShadowsOn ? "LIGADAS" : "desligadas");
}
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_CLIP_PLANE0
#define GL_CLIP_PLANE0 0x3000
#endif
// ---- reflexao planar da agua (estilo Half-Life 2): o mundo espelhado no plano da agua e desenhado em meia resolucao num FBO; o shader da agua amostra com a normal da onda ----
static bool rfEnsure(int w, int h) {
    static bool bad = false; if (bad) return false;
    if (!pGenFB) { pGenFB = (FnGenFB)SDL_GL_GetProcAddress("glGenFramebuffers"); pBindFB = (FnBindFB)SDL_GL_GetProcAddress("glBindFramebuffer"); pFBTex = (FnFBTex)SDL_GL_GetProcAddress("glFramebufferTexture2D"); pChkFB = (FnChkFB)SDL_GL_GetProcAddress("glCheckFramebufferStatus"); pActTex = (FnActTex)SDL_GL_GetProcAddress("glActiveTexture"); }
    if (!gShadowShader || !pGenFB || !pBindFB || !pFBTex || !pChkFB || !pActTex) { bad = true; return false; }
    if (gRfFbo && gRfW == w && gRfH == h) return true;
    if (!gRfTex) glGenTextures(1, &gRfTex); if (!gRfDep) glGenTextures(1, &gRfDep);
    glBindTexture(GL_TEXTURE_2D, gRfTex); glTexImage2D(GL_TEXTURE_2D, 0, gShCompat ? GL_RGBA : GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, gRfDep); glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (!gRfFbo) pGenFB(1, &gRfFbo);
    pBindFB(GL_FRAMEBUFFER, gRfFbo); pFBTex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gRfTex, 0); pFBTex(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gRfDep, 0);
    bool ok = pChkFB(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok) {   // GLES: profundidade de 16 bits
        glBindTexture(GL_TEXTURE_2D, gRfDep); glTexImage2D(GL_TEXTURE_2D, 0, 0x81A5, w, h, 0, GL_DEPTH_COMPONENT, 0x1403, nullptr);
        pFBTex(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gRfDep, 0); ok = pChkFB(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    pBindFB(GL_FRAMEBUFFER, 0); glBindTexture(GL_TEXTURE_2D, atlasTex);
    if (!ok) { bad = true; logWarn("Reflexo da agua: framebuffer incompleto; desligado"); return false; }
    gRfW = w; gRfH = h; logInfo("Reflexo da agua: pronto (%dx%d)", w, h); return true;
}
// altura da superficie de agua dominante perto do jogador (amostra 7x7 colunas, a cada 12 quadros)
static bool rfFindWater(double ex, double ey, double ez, float& wy) {
    static int cnt = 0; static bool has = false; static float cy = 0.f;
    if ((cnt++ % 12) != 0) { wy = cy; return has; }
    int ys[49], n = 0; const int bx = (int)floor(ex), bz = (int)floor(ez), by = (int)floor(ey);
    for (int gx = -3; gx <= 3; gx++) for (int gz = -3; gz <= 3; gz++)
        for (int y = by; y > by - 48 && y > 0; y--) {
            const BT b = getB(bx + gx * 5, y, bz + gz * 5);
            if (isWater(b)) { if (!isWater(getB(bx + gx * 5, y + 1, bz + gz * 5))) ys[n++] = y; break; }
            if (b != AIR) break;
        }
    int best = 0, bc = 0; for (int i = 0; i < n; i++) { int c = 0; for (int j = 0; j < n; j++) c += ys[j] == ys[i]; if (c > bc) { bc = c; best = ys[i]; } }
    has = bc >= 2; cy = best + 0.89f; wy = cy; return has;
}
