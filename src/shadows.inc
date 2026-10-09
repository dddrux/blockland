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
static const float SH_ZR = 200.f;   // meia-profundidade dos mapas (igual em todos); cascatas (tamanhos/alcances) em gCasT/gCasE (sky_cycle.inc)
static const int SH_CHUNKS = 5;     // maximo de chunks desenhados por cascata de terreno
static bool gShadowsOn = false, shReady = false, shFailed = false;

static void shMatMul(const float* A, const float* B, float* C) {   // C = A*B (coluna-maior)
    for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0.f; for (int k = 0; k < 4; k++) s += A[k * 4 + r] * B[c * 4 + k]; C[c * 4 + r] = s; }
}
// Matrizes de um mapa ortografico olhando na direcao do sol, centrado em c, meia-largura E, N texels. LV = vista da luz, P = projecao, BPV = bias*P*LV (mundo -> [0,1]).
static void shMakeMat(float sn, float cs, const float* c, float E, int N, float* LV, float* P, float* BPV) {
    const float f[3] = {sn, -cs, 0.f}, up[3] = {0.f, 0.f, 1.f};   // f = direcao da luz (do sol para o chao), r/u = eixos do mapa
    float r[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]};
    const float rl = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]); for (float& v : r) v /= rl;
    const float u[3] = {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]};
    const float ts = 2.f * E / N;
    float tx = -(r[0] * c[0] + r[1] * c[1] + r[2] * c[2]), ty = -(u[0] * c[0] + u[1] * c[1] + u[2] * c[2]);
    tx = floorf(tx / ts + .5f) * ts; ty = floorf(ty / ts + .5f) * ts;   // alinha a texels: a sombra nao "treme" quando o jogador anda
    const float tz = f[0] * c[0] + f[1] * c[1] + f[2] * c[2];
    const float L[16] = {r[0], u[0], -f[0], 0, r[1], u[1], -f[1], 0, r[2], u[2], -f[2], 0, tx, ty, tz, 1};
    const float Pm[16] = {1.f / E, 0, 0, 0, 0, 1.f / E, 0, 0, 0, 0, -1.f / SH_ZR, 0, 0, 0, 0, 1};
    const float B[16] = {.5f, 0, 0, 0, 0, .5f, 0, 0, 0, 0, .5f, 0, .5f, .5f, .5f, 1};
    float PV[16]; shMatMul(Pm, L, PV); shMatMul(B, PV, BPV);
    for (int k = 0; k < 16; k++) { LV[k] = L[k]; P[k] = Pm[k]; }
}
static bool shMakeDepth(ShCas& c) {   // textura de profundidade com comparacao por hardware + FBO so de profundidade
    glGenTextures(1, &c.tex); glBindTexture(GL_TEXTURE_2D, c.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, c.n, c.n, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    const GLint flt = gShCompat ? GL_NEAREST : GL_LINEAR;   // compat: sem comparacao por hardware, entao sem filtro
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flt); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flt);   // LINEAR + comparacao = PCF 2x2 no hardware
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!gShCompat) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); }
    pGenFB(1, &c.fbo); pBindFB(GL_FRAMEBUFFER, c.fbo); pFBTex(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, c.tex, 0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);   // so profundidade
    bool ok = pChkFB(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok) {   // GLES/gl4es: tenta profundidade de 16 bits
        glBindTexture(GL_TEXTURE_2D, c.tex); glTexImage2D(GL_TEXTURE_2D, 0, 0x81A5 /*GL_DEPTH_COMPONENT16*/, c.n, c.n, 0, GL_DEPTH_COMPONENT, 0x1403 /*GL_UNSIGNED_SHORT*/, nullptr);
        pFBTex(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, c.tex, 0); ok = pChkFB(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        if (ok) logInfo("Sombras: usando profundidade de 16 bits (compatibilidade)");
    }
    pBindFB(GL_FRAMEBUFFER, 0); return ok;
}
static bool shadowInit() {
    if (shReady || shFailed) return shReady;
    pGenFB = (FnGenFB)SDL_GL_GetProcAddress("glGenFramebuffers"); pBindFB = (FnBindFB)SDL_GL_GetProcAddress("glBindFramebuffer");
    pFBTex = (FnFBTex)SDL_GL_GetProcAddress("glFramebufferTexture2D"); pChkFB = (FnChkFB)SDL_GL_GetProcAddress("glCheckFramebufferStatus"); pActTex = (FnActTex)SDL_GL_GetProcAddress("glActiveTexture");
    if (!gShadowShader || !useVBO || !pGenFB || !pBindFB || !pFBTex || !pChkFB || !pActTex) { shFailed = true; logWarn("Sombras: indisponiveis (shader com sombras, VBO ou FBO ausentes)"); return false; }
    if (gShCompat) { for (ShCas& c : gCasT) c.n = 1024; for (ShCas& c : gCasE) c.n = 512; }   // celular: mapas menores (4x menos pixels)
    bool ok = true; for (ShCas& c : gCasT) ok = shMakeDepth(c) && ok; for (ShCas& c : gCasE) ok = shMakeDepth(c) && ok;
    glDrawBuffer(GL_BACK); glReadBuffer(GL_BACK); glBindTexture(GL_TEXTURE_2D, atlasTex);
    if (!ok) { shFailed = true; logWarn("Sombras: framebuffer de profundidade incompleto"); return false; }
    shReady = true; logInfo("Sombras: cascatas de terreno %d/%d/%d (alcance %.0f/%.0f/%.0f) + entidades %d/%d prontas", gCasT[0].n, gCasT[1].n, gCasT[2].n, gCasT[0].e, gCasT[1].e, gCasT[2].e, gCasE[0].n, gCasE[1].n); return true;
}
// estado comum de uma passada so de profundidade (o shader dos chunks fica desligado; folhas/plantas/mobs recortados pelo alpha)
static void shPassBegin(GLuint fbo, int n, const float* P, const float* LV, GLint* vp) {
    glGetIntegerv(GL_VIEWPORT, vp);
    pBindFB(GL_FRAMEBUFFER, fbo); glViewport(0, 0, n, n);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); glDepthMask(GL_TRUE); glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_FOG); glDisable(GL_BLEND); glDisable(GL_STENCIL_TEST); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.5f);
    skyShaderOff(); glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadMatrixf(P); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadMatrixf(LV);
}
static void shPassEnd(const GLint* vp) {
    glDisable(GL_POLYGON_OFFSET_FILL); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); pBindFB(GL_FRAMEBUFFER, 0); glViewport(vp[0], vp[1], vp[2], vp[3]);
}
// Chamado todo quadro de mundo, ANTES de posicionar a camera. Define gShadowDraw (usado so na passada dos solidos); atualiza cada cascata do TERRENO
// quando preciso (a mais proxima mais vezes) e REDESENHA todo quadro as cascatas das ENTIDADES (ec = centro, o jogador): drawEnts(alcance) desenha
// mobs/jogador com a vista da luz carregada. Tudo centrado no jogador.
template <class F> static void shadowFrame(const double* cam, bool underFluid, const double* ec, F&& drawEnts) {
    gShadowDraw = false; gShadowEnt = false; gShadowAct = false; gFxOn = gShadowsOn && shReady;
    if (!gShadowsOn || !shReady) return;
    const float th = gSky.angle * 6.2831853f, cs = cosf(th), sn = sinf(th);   // sol = (-sin, cos, 0): mesma conta do glRotate de drawSkyFull
    gSunDir[0] = -sn; gSunDir[1] = cs; gSunDir[2] = 0.f;
    gSunStr = clampf2((cs - .03f) / .2f, 0.f, 1.f);   // some ao entardecer; de noite nao ha sombra
    if (gSunStr < .01f || underFluid) return;
    gShadowDraw = true; gShadowAct = true;
    static unsigned shFrame = 0; shFrame++; bool shTerDone = false;
    const Uint32 now = SDL_GetTicks(); const float c[3] = {(float)ec[0], (float)ec[1], (float)ec[2]};
    for (int ci = 0; ci < 3; ci++) {   // ---- terreno: so em profundidade, vizinhanca da cascata ----
        ShCas& cc = gCasT[ci];
        const float dx = c[0] - cc.ctr[0], dy = c[1] - cc.ctr[1], dz = c[2] - cc.ctr[2], lim = cc.e * .2f;
        const Uint32 every = ci == 0 ? 300u : ci == 1 ? 900u : 2500u;   // perto atualiza mais; os intervalos diferentes tambem espalham o custo entre quadros
        if (cc.last && now - cc.last < every && dx * dx + dy * dy + dz * dz < lim * lim && fabsf(gSky.angle - cc.ang) < .004f) continue;   // ainda valida
        if (shTerDone) continue;   // no maximo UMA cascata de terreno por quadro: as outras ficam para o proximo
        shTerDone = true;
        cc.last = now ? now : 1u; cc.ang = gSky.angle; cc.ctr[0] = c[0]; cc.ctr[1] = c[1]; cc.ctr[2] = c[2];
        float LV[16], P[16]; shMakeMat(sn, cs, c, cc.e, cc.n, LV, P, cc.mat);
        GLint vp[4]; shPassBegin(cc.fbo, cc.n, P, LV, vp);
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_COLOR_ARRAY);   // sem cor por vertice: o alfa dele guarda a luz
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.5f, 2.f);
        const int R = std::min(SH_CHUNKS, (int)ceilf(cc.e / CW) + 1), pcx = fdiv((int)floorf(c[0]), CW), pcz = fdiv((int)floorf(c[2]), CW);
        for (int zz = -R; zz <= R; zz++) for (int xx = -R; xx <= R; xx++) {
            Chunk* ch = getC(pcx + xx, pcz + zz); if (!ch || ch->nv <= 0) continue;
            if (ch->gl || ch->glw) uploadChunk(ch);
            pBindBuffer(GL_ARRAY_BUFFER, ch->vbo); bindQuadIB((size_t)ch->nv / 4);
            glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)offsetof(V, x)); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)offsetof(V, u));
            quadDrawVBO(0, ch->nv, false);
        }
        pBindBuffer(GL_ARRAY_BUFFER, 0); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
        shPassEnd(vp);
    }
    for (int ei = 0; ei < 2; ei++) {   // ---- entidades: E0 (pe do jogador) todo quadro; E1 (longe) em quadros alternados
        ShCas& cc = gCasE[ei]; if (ei == 1 && (shFrame & 1) && cc.last) continue; cc.last = 1u;
        float LV[16], P[16]; shMakeMat(sn, cs, c, cc.e, cc.n, LV, P, cc.mat);
        GLint vp[4]; shPassBegin(cc.fbo, cc.n, P, LV, vp);
        glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_COLOR_ARRAY); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);   // modelos: modo imediato
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.f, 1.f);
        drawEnts(cc.e);
        shPassEnd(vp); glEnable(GL_TEXTURE_2D);
    }
    gShadowEnt = true;
    for (int i = 0; i < 3; i++) { pActTex(GL_TEXTURE0 + 1 + i); glBindTexture(GL_TEXTURE_2D, gCasT[i].tex); }   // unidades 1-3 = terreno, 4-5 = entidades (o shader le de la)
    for (int i = 0; i < 2; i++) { pActTex(GL_TEXTURE0 + 4 + i); glBindTexture(GL_TEXTURE_2D, gCasE[i].tex); }
    pActTex(GL_TEXTURE0);
}
template <class F> static void shadowToggle(F& say) {
    if (!shadowInit()) { say("Sombras indisponiveis nesta placa/driver (veja logs/latest.log)"); return; }
#ifdef __ANDROID__
    say("Sombras indisponiveis no celular (desempenho)"); return;
#endif
    gShadowsOn = !gShadowsOn; for (ShCas& cc : gCasT) cc.last = 0;
    static bool autoPack = false;   // F7 liga junto o pacote interno (sol realista, agua com reflexo, god rays, lens flare) se o jogador nao escolheu um
    if (gShadowsOn && !BL_NO_FANCY && gsi("shd.sel", 0) == 0) { gSt["shd.sel"] = 1; autoPack = true; }
    else if (!gShadowsOn && autoPack) { gSt["shd.sel"] = 0; autoPack = false; }
    gShaderPackOn = !BL_NO_FANCY && (gsi("shd.sel", 0) > 0 || gShadowsOn);
    say(gShadowsOn ? "Sombras + sol e agua realistas: ligados (F7 desliga)" : "Sombras e efeitos realistas: desligados");
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
