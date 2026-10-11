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
#ifdef __ANDROID__
// ---- sombras do sol por MAPA DE ALTURAS (celular): sem FBO e sem passada extra de geometria ----
// A CPU guarda a altura do topo de cada coluna (cache por chunk) e, para cada coluna de uma area 128x128 ao redor do jogador, a altura do raio de sol
// que passa por cima dela (maximo de H(coluna a d blocos rumo ao sol) - d*tan(elevacao)). O sol anda no plano x-y, entao os raios vao so em +-x.
// Vira 1 textura RGBA8 (64 KB) refeita ~1x/0,7 s; o shader faz 4 leituras nela por pixel.
static const int HS_N = 128, HS_M = 32, HS_G = HS_N + 2 * HS_M;
struct HsCol { uint8_t h[CW * CW]; Uint32 t = 0; };
static std::unordered_map<int64_t, HsCol> gHsCols;
static GLuint gHsTex = 0; static float gHsAng = -9.f; static Uint32 gHsLast = 0; static bool gHsInit = false;
static std::vector<uint16_t> gHsH; static std::vector<uint8_t> gHsPix;
static void hsScan(int cx, int cz, HsCol& col, Uint32 now) {   // h = y do topo + 1 da coluna que projeta sombra (folhas sim; agua, plantas e vidro nao)
    col.t = now ? now : 1u; Chunk* c = getC(cx, cz); if (!c) { memset(col.h, 0, sizeof col.h); return; }
    const int top = chunkTop(c);
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
        int y = top; for (; y >= 0; y--) { const int b = c->b[idx(x, y, z)]; if (b && !isWater(b) && lightOpac(b) > 0) break; }
        col.h[z * CW + x] = (uint8_t)std::min(255, y + 1);
    }
}
static void hsUpdate(const double* ec, float sn, float cs) {
    pActTex(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, gHsTex); pActTex(GL_TEXTURE0);
    const Uint32 now = SDL_GetTicks(); const float px = (float)ec[0], pz = (float)ec[2];
    const bool moved = !gHsInit || fabsf(px - (gHsOx + HS_N * .5f)) > 10.f || fabsf(pz - (gHsOz + HS_N * .5f)) > 10.f;
    if (!moved && fabsf(gSky.angle - gHsAng) <= .0015f && now - gHsLast <= 700u) return;
    if (moved) { gHsOx = floorf((px - HS_N * .5f) / CW) * CW; gHsOz = floorf((pz - HS_N * .5f) / CW) * CW; }
    gHsInit = true; gHsLast = now ? now : 1u; gHsAng = gSky.angle;
    if (gHsCols.size() > 600) gHsCols.clear();
    const bool all = gHsCols.empty(); int budget = 48;
    const int ocx = (int)gHsOx / CW, ocz = (int)gHsOz / CW, pcx = fdiv((int)floorf(px), CW), pcz = fdiv((int)floorf(pz), CW), nc = HS_N / CW, mc = HS_M / CW;
    gHsH.assign((size_t)HS_G * HS_G, 0);
    for (int cz = ocz - mc; cz < ocz + nc + mc; cz++) for (int cx = ocx - mc; cx < ocx + nc + mc; cx++) {
        HsCol& col = gHsCols[key(cx, cz)]; const bool nearC = abs(cx - pcx) <= 2 && abs(cz - pcz) <= 2;
        if (col.t == 0 || ((all || budget > 0) && now - col.t > (nearC ? 700u : 6000u))) { hsScan(cx, cz, col, now); budget--; }
        const int gx0 = (cx - ocx + mc) * CW, gz0 = (cz - ocz + mc) * CW;
        for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) gHsH[(size_t)(gz0 + z) * HS_G + gx0 + x] = col.h[z * CW + x];
    }
    const float sx = -sn; const int dirx = sx >= 0.f ? 1 : -1; const float tanE = fabsf(sx) < 1e-3f ? 1e3f : cs / fabsf(sx);
    const int D = tanE > 100.f ? 0 : std::min(HS_M, (int)ceilf(36.f / std::max(tanE, .05f)));
    gHsPix.resize((size_t)HS_N * HS_N * 4);
    for (int z = 0; z < HS_N; z++) for (int x = 0; x < HS_N; x++) {
        const uint16_t* row = &gHsH[(size_t)(z + HS_M) * HS_G + x + HS_M]; float S = 0.f;
        for (int d = 1; d <= D; d++) { const float v = (float)row[dirx * d] - d * tanE; if (v > S) S = v; }
        S = std::min(S, 255.f); const int ip = (int)S, fr = (int)((S - ip) * 255.f + .5f); uint8_t* o = &gHsPix[((size_t)z * HS_N + x) * 4];
        o[0] = (uint8_t)ip; o[1] = (uint8_t)std::min(255, fr); o[2] = (uint8_t)std::min(255, (int)row[0]); o[3] = 255;
    }
    pActTex(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, gHsTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HS_N, HS_N, 0, GL_RGBA, GL_UNSIGNED_BYTE, gHsPix.data()); pActTex(GL_TEXTURE0);
}
#endif
static bool shadowInit() {
    if (shReady || shFailed) return shReady;
#ifdef __ANDROID__
    pActTex = (FnActTex)SDL_GL_GetProcAddress("glActiveTexture");
    if (!gShadowShader || !pActTex) { shFailed = true; logWarn("Sombras: indisponiveis (shader de sombras ou glActiveTexture ausentes)"); return false; }
    gNCasT = 0; gNCasE = 0; pActTex(GL_TEXTURE0 + 1); glGenTextures(1, &gHsTex); glBindTexture(GL_TEXTURE_2D, gHsTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    pActTex(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);
    shReady = true; logInfo("Sombras (celular): mapa de alturas %dx%d pronto", HS_N, HS_N); return true;
#endif
    pGenFB = (FnGenFB)SDL_GL_GetProcAddress("glGenFramebuffers"); pBindFB = (FnBindFB)SDL_GL_GetProcAddress("glBindFramebuffer");
    pFBTex = (FnFBTex)SDL_GL_GetProcAddress("glFramebufferTexture2D"); pChkFB = (FnChkFB)SDL_GL_GetProcAddress("glCheckFramebufferStatus"); pActTex = (FnActTex)SDL_GL_GetProcAddress("glActiveTexture");
    if (!gShadowShader || !useVBO || !pGenFB || !pBindFB || !pFBTex || !pChkFB || !pActTex) { shFailed = true; logWarn("Sombras: indisponiveis (shader com sombras, VBO ou FBO ausentes)"); return false; }
#ifdef __ANDROID__
    gNCasT = 0; gNCasE = 0;   // celular nao usa cascatas (FBO); ver mapa de alturas abaixo
#else
    if (gShCompat) { for (ShCas& c : gCasT) c.n = 1024; for (ShCas& c : gCasE) c.n = 512; }   // compat: mapas menores (4x menos pixels)
#endif
    bool ok = true; for (int i = 0; i < gNCasT; i++) ok = shMakeDepth(gCasT[i]) && ok; for (int i = 0; i < gNCasE; i++) ok = shMakeDepth(gCasE[i]) && ok;
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
#ifdef __ANDROID__
    hsUpdate(ec, sn, cs); gShadowDraw = true; gShadowAct = true; gShadowEnt = false; return;   // celular: sem cascatas/FBO
#endif
    gShadowDraw = true; gShadowAct = true;
    static unsigned shFrame = 0; shFrame++; bool shTerDone = false;
    const Uint32 now = SDL_GetTicks(); const float c[3] = {(float)ec[0], (float)ec[1], (float)ec[2]};
    for (int ci = 0; ci < gNCasT; ci++) {   // ---- terreno: so em profundidade, vizinhanca da cascata ----
        ShCas& cc = gCasT[ci];
        const float dx = c[0] - cc.ctr[0], dy = c[1] - cc.ctr[1], dz = c[2] - cc.ctr[2], lim = cc.e * .2f;
        const Uint32 every = (ci == 0 ? 300u : ci == 1 ? 900u : 2500u) * (gNCasT == 1 ? (gShQ >= 2 ? 1u : gShQ == 1 ? 2u : 4u) : 1u);   // perto atualiza mais; os intervalos diferentes tambem espalham o custo entre quadros
        if (cc.last && now - cc.last < every && dx * dx + dy * dy + dz * dz < lim * lim && fabsf(gSky.angle - cc.ang) < .004f) continue;   // ainda valida
        if (shTerDone) continue;   // no maximo UMA cascata de terreno por quadro: as outras ficam para o proximo
        shTerDone = true;
        cc.last = now ? now : 1u; cc.ang = gSky.angle; cc.ctr[0] = c[0]; cc.ctr[1] = c[1]; cc.ctr[2] = c[2];
        float LV[16], P[16]; shMakeMat(sn, cs, c, cc.e, cc.n, LV, P, cc.mat);
        GLint vp[4]; shPassBegin(cc.fbo, cc.n, P, LV, vp);
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_COLOR_ARRAY);   // sem cor por vertice: o alfa dele guarda a luz
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.5f, 2.f);
        const int R = std::min(SH_CHUNKS, (int)ceilf(cc.e / CW) + (gNCasT == 1 ? 2 : 1)), pcx = fdiv((int)floorf(c[0]), CW), pcz = fdiv((int)floorf(c[2]), CW);
        for (int zz = -R; zz <= R; zz++) for (int xx = -R; xx <= R; xx++) {
            {   // corte no espaco da luz: o mapa so enxerga x_l = -cs*(x-cx) - sn*(y-cy) em [-E,E] e z em [cz-E,cz+E]; chunk fora disso nao projeta sombra util
                const float X0 = (float)((pcx + xx) * CW), Z0 = (float)((pcz + zz) * CW), E2 = cc.e + 2.f;
                if (Z0 + CW < c[2] - E2 || Z0 > c[2] + E2) continue;
                const float ylo = std::max(0.f, c[1] - 100.f), yhi = std::min((float)CH, c[1] + 100.f); float mn = 1e9f, mx = -1e9f;
                for (int q = 0; q < 4; q++) { const float xl = -cs * ((q & 1 ? X0 + CW : X0) - c[0]) - sn * ((q & 2 ? yhi : ylo) - c[1]); mn = std::min(mn, xl); mx = std::max(mx, xl); }
                if (mx < -E2 || mn > E2) continue;
            }
            Chunk* ch = getC(pcx + xx, pcz + zz); if (!ch || ch->nv <= 0) continue;
            if (ch->gl || ch->glw) uploadChunk(ch);
            pBindBuffer(GL_ARRAY_BUFFER, ch->vbo); bindQuadIB((size_t)ch->nv / 4);
            glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)offsetof(V, x)); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)offsetof(V, u));
            quadDrawVBO(0, ch->nv, false);
        }
        pBindBuffer(GL_ARRAY_BUFFER, 0); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
        shPassEnd(vp);
    }
    const int nE = gShQ >= 1 ? gNCasE : 0;   // qualidade 0: sem sombra de entidades
    for (int ei = 0; ei < nE; ei++) {   // ---- entidades: E0 (pe do jogador) todo quadro; E1 (longe) / qualidade 1 em quadros alternados
        ShCas& cc = gCasE[ei]; if ((ei == 1 || gShQ == 1) && (shFrame & 1) && cc.last) continue; cc.last = 1u;
        float LV[16], P[16]; shMakeMat(sn, cs, c, cc.e, cc.n, LV, P, cc.mat);
        GLint vp[4]; shPassBegin(cc.fbo, cc.n, P, LV, vp);
        glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_COLOR_ARRAY); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);   // modelos: modo imediato
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.f, 1.f);
        drawEnts(cc.e);
        shPassEnd(vp); glEnable(GL_TEXTURE_2D);
    }
    gShadowEnt = nE > 0;
    for (int i = 0; i < gNCasT; i++) { pActTex(GL_TEXTURE0 + 1 + i); glBindTexture(GL_TEXTURE_2D, gCasT[i].tex); }   // unidades 1-3 = terreno, 4-5 = entidades (o shader le de la)
    for (int i = 0; i < gNCasE; i++) { pActTex(GL_TEXTURE0 + 4 + i); glBindTexture(GL_TEXTURE_2D, gCasE[i].tex); }
    pActTex(GL_TEXTURE0);
}
template <class F> static void shadowToggle(F& say) {
    if (!shadowInit()) { say("Sombras indisponiveis nesta placa/driver (veja logs/latest.log)"); return; }
    gShadowsOn = !gShadowsOn; for (ShCas& cc : gCasT) cc.last = 0; skySelectProg(gShadowsOn);   // celular: troca entre o shader enxuto e o de sombras
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
