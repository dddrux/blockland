// ================= Renderer (patchVk.3, fase 1 do Vulkan nativo; ver doc/VULKAN_NATIVO.md) =================
// O jogo NAO deve chamar glPushMatrix/glTranslatef/... direto: usa rd*. Hoje cada rd* repassa para o OpenGL (ou para a camada vkgl no BL_VULKAN);
// o backend Vulkan nativo vai guardar a pilha de matrizes aqui e mandar o resultado ao shader (push constants), sem passar por macros gl*.
// Grupos migrados: pilha de matrizes/projecao (patchVk.3) e estado (patchVk.4). Proximos: texturas, buffers do mundo, imediato (glBegin/glVertex), clear/FBO.
#pragma once
// patchVkRT.1: build unica. BL_VULKAN = suporte a Vulkan COMPILADO; BL_VKON = Vulkan ATIVO neste processo (gVkOn, decidido antes da janela); BL_NOVK = caminho GL "de verdade".
#ifdef BL_VULKAN
#define BL_VKON (gVkOn)
#else
#define BL_VKON false
#endif
#define BL_NOVK (!BL_VKON)
#ifdef BL_GLES_NATIVE
#define BL_VKORGN true
#else
#define BL_VKORGN BL_VKON
#endif
// patchFast.4: pilha de matrizes e viewport RASTREADOS na CPU. glGetFloatv/glGetIntegerv forcam sincronizacao CPU<->GPU (no radeonsi era o
// "pos-sombras -> pre-reflexo" de 20-70 ms). rdGetMatrixf/rdGetViewport leem daqui; a cada 512 leituras comparam com o GL real e, se
// divergir (ex.: codigo mexendo no GL direto), voltam sozinhos ao glGet* (log "[PERF] matriz rastreada divergiu").
static float gTmS[3][32][16]; static int gTmT[3] = {0, 0, 0}, gTmM = 0; static bool gTmInit = false, gTmBad = false, gRdVpSet = false; static GLint gRdVp[4] = {0, 0, 0, 0};
static inline void tmI(float* m) { memset(m, 0, 64); m[0] = m[5] = m[10] = m[15] = 1.f; }
static inline float* tmC() { if (!gTmInit) { gTmInit = true; for (int a = 0; a < 3; a++) tmI(gTmS[a][0]); } return gTmS[gTmM][gTmT[gTmM]]; }
static void tmMul(float* m, const float* b) { float r[16]; for (int c = 0; c < 4; c++) for (int q = 0; q < 4; q++) r[c * 4 + q] = m[q] * b[c * 4] + m[4 + q] * b[c * 4 + 1] + m[8 + q] * b[c * 4 + 2] + m[12 + q] * b[c * 4 + 3]; memcpy(m, r, 64); }
static inline void rdMatrixMode(GLenum m)                      { glMatrixMode(m); gTmM = m == GL_PROJECTION ? 1 : m == 0x1702 ? 2 : 0; }
static inline void rdLoadIdentity()                            { glLoadIdentity(); tmI(tmC()); }
static inline void rdPushMatrix()                              { glPushMatrix(); float* c = tmC(); if (gTmT[gTmM] < 31) { memcpy(gTmS[gTmM][gTmT[gTmM] + 1], c, 64); gTmT[gTmM]++; } }
static inline void rdPopMatrix()                               { glPopMatrix(); tmC(); if (gTmT[gTmM] > 0) gTmT[gTmM]--; }
static inline void rdTranslatef(GLfloat x, GLfloat y, GLfloat z) { glTranslatef(x, y, z); float* m = tmC(); for (int i = 0; i < 4; i++) m[12 + i] += m[i] * x + m[4 + i] * y + m[8 + i] * z; }
static inline void rdRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { glRotatef(a, x, y, z);
    const float l = sqrtf(x * x + y * y + z * z); if (l < 1e-9f) return; x /= l; y /= l; z /= l; const float r = a * 0.017453292519943295f, c = cosf(r), s = sinf(r), k = 1.f - c; float b[16]; tmI(b);
    b[0] = x * x * k + c; b[4] = x * y * k - z * s; b[8] = x * z * k + y * s; b[1] = y * x * k + z * s; b[5] = y * y * k + c; b[9] = y * z * k - x * s; b[2] = x * z * k - y * s; b[6] = y * z * k + x * s; b[10] = z * z * k + c; tmMul(tmC(), b); }
static inline void rdScalef(GLfloat x, GLfloat y, GLfloat z)   { glScalef(x, y, z); float* m = tmC(); for (int i = 0; i < 4; i++) { m[i] *= x; m[4 + i] *= y; m[8 + i] *= z; } }
static inline void rdOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { glOrtho(l, r, b, t, n, f);
    float m[16]; tmI(m); m[0] = (float)(2 / (r - l)); m[5] = (float)(2 / (t - b)); m[10] = (float)(-2 / (f - n)); m[12] = (float)(-(r + l) / (r - l)); m[13] = (float)(-(t + b) / (t - b)); m[14] = (float)(-(f + n) / (f - n)); tmMul(tmC(), m); }
static inline void rdFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { glFrustum(l, r, b, t, n, f);
    float m[16]; memset(m, 0, 64); m[0] = (float)(2 * n / (r - l)); m[5] = (float)(2 * n / (t - b)); m[8] = (float)((r + l) / (r - l)); m[9] = (float)((t + b) / (t - b)); m[10] = (float)(-(f + n) / (f - n)); m[11] = -1.f; m[14] = (float)(-2 * f * n / (f - n)); tmMul(tmC(), m); }
static inline void rdLoadMatrixf(const GLfloat* m)             { glLoadMatrixf(m); memcpy(tmC(), m, 64); }
static void tmBad(const char* what) { if (!gTmBad) { gTmBad = true; printf("[WARN] [PERF] %s rastreado divergiu do GL: voltando ao glGet (sem o ganho de patchFast.4)\n", what); } }
static void rdGetMatrixf(GLenum pn, float* out) {   // pn = GL_MODELVIEW_MATRIX ou GL_PROJECTION_MATRIX
    static unsigned cnt = 0; const int w = pn == 0x0BA7 ? 1 : 0; tmC();
    if (gTmBad || !BL_NOVK) { glGetFloatv(pn, out); return; }
    memcpy(out, gTmS[w][gTmT[w]], 64);
    if ((cnt++ & 511u) == 0) { float g[16]; glGetFloatv(pn, g); for (int i = 0; i < 16; i++) if (fabsf(g[i] - out[i]) > 2e-3f * (1.f + fabsf(g[i]))) { tmBad("matriz"); memcpy(out, g, 64); break; } }
}
static void rdGetViewport(GLint* vp) {
    static unsigned cnt = 0;
    if (gTmBad || !gRdVpSet || !BL_NOVK) { glGetIntegerv(GL_VIEWPORT, vp); return; }
    memcpy(vp, gRdVp, sizeof gRdVp);
    if ((cnt++ & 511u) == 0) { GLint g[4]; glGetIntegerv(GL_VIEWPORT, g); for (int i = 0; i < 4; i++) if (g[i] != vp[i]) { tmBad("viewport"); memcpy(vp, g, sizeof g); break; } }
}
// ---- patchVk.4: estado (enable/disable, blend, alpha, depth, colorMask, neblina, cull, scissor, viewport) ----
// Hoje repassa ao GL/vkgl; o backend Vulkan nativo vai guardar este estado e escolher o pipeline (cache por combinacao).
// patchFast.5: glIsEnabled/glGetFloatv(FOG) tambem sincronizam com a GPU: estado rastreado aqui (mesma auto-verificacao do patchFast.4)
static int8_t gRdCap[6] = {-1, -1, -1, -1, -1, -1}; static float gRdFogSE[2] = {0.f, 0.f}; static bool gRdFogK[2] = {false, false};
static inline int rdCapIx(GLenum c) { switch (c) { case 0x0BE2: return 0; case 0x0B60: return 1; case 0x0BC0: return 2; case 0x0B44: return 3; case 0x0B71: return 4; case 0x0B50: return 5; } return -1; }
static inline void rdEnable(GLenum c)                          { glEnable(c); const int i = rdCapIx(c); if (i >= 0) gRdCap[i] = 1; }
static inline void rdDisable(GLenum c)                         { glDisable(c); const int i = rdCapIx(c); if (i >= 0) gRdCap[i] = 0; }
static GLboolean rdIsEnabled(GLenum c) {
    static unsigned cnt = 0; const int i = rdCapIx(c);
    if (i < 0 || gRdCap[i] < 0 || gTmBad || !BL_NOVK) return glIsEnabled(c);
    if ((cnt++ & 1023u) == 0 && (glIsEnabled(c) ? 1 : 0) != gRdCap[i]) { tmBad("estado (enable)"); return glIsEnabled(c); }
    return gRdCap[i] ? GL_TRUE : GL_FALSE;
}
static inline void rdBlendFunc(GLenum s, GLenum d)             { glBlendFunc(s, d); }
static inline void rdAlphaFunc(GLenum f, GLfloat r)             { glAlphaFunc(f, r); }
static inline void rdDepthMask(GLboolean m)                    { glDepthMask(m); }
static inline void rdDepthFunc(GLenum f)                       { glDepthFunc(f); }
static inline void rdColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { glColorMask(r, g, b, a); }
static inline void rdFogf(GLenum p, GLfloat v)                 { glFogf(p, v); if (p == 0x0B63) { gRdFogSE[0] = v; gRdFogK[0] = true; } else if (p == 0x0B64) { gRdFogSE[1] = v; gRdFogK[1] = true; } }
static void rdGetFogSE(float& st, float& en) { if (gRdFogK[0] && gRdFogK[1] && !gTmBad && BL_NOVK) { st = gRdFogSE[0]; en = gRdFogSE[1]; } else { glGetFloatv(GL_FOG_START, &st); glGetFloatv(GL_FOG_END, &en); } }
static inline void rdFogi(GLenum p, GLint v)                   { glFogi(p, v); }
static inline void rdFogfv(GLenum p, const GLfloat* v)         { glFogfv(p, v); }
static inline void rdCullFace(GLenum m)                        { glCullFace(m); }
static inline void rdFrontFace(GLenum m)                       { glFrontFace(m); }
static inline void rdScissor(GLint x, GLint y, GLsizei w, GLsizei h)    { glScissor(x, y, w, h); }
static inline void rdViewport(GLint x, GLint y, GLsizei w, GLsizei h)   { glViewport(x, y, w, h); gRdVp[0] = x; gRdVp[1] = y; gRdVp[2] = w; gRdVp[3] = h; gRdVpSet = true; }
