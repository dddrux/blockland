// ================= glesnative: OpenGL 1.x -> OpenGL ES NATIVO (Android, sem gl4es) =================
// Ativado so no Android (BL_GLES_NATIVE, definido em android.inc; -DBL_GL4ES volta ao caminho antigo). O PC e o Vulkan NAO usam este arquivo.
// O jogo continua chamando glBegin/glVertex/glTranslatef/glDrawArrays... (macros no fim redirecionam para gln*).
// Otimizacoes: (1) shader em VARIANTES geradas por estado (textura/alpha-test/neblina/luz do mundo/agua): zero branch em runtime;
// (2) luz do ceu/blocos: exp2 (1 SFU) com constantes pre-calculadas na CPU em vez de pow+floor; neblina por vertice (exata p/ linear);
// (3) cache de TODO estado GL (enable/blend/depth/textura/programa/atributos): chamada redundante nao chega ao driver;
// (4) uniformes por GRUPO com versao (so reenvia o que mudou desde o ultimo uso do programa); MVP multiplicado 1x por mudanca;
// (5) modo imediato -> arrays do cliente (sem buffer: evita a copia do buffer inteiro no Mali); display lists viram VBO estatico; quads por 1 IBO uint16 fixo;
// (6) sem VAO/FBO/sombras: igual ao backend Vulkan (o jogo desliga sozinho). glFlush vira no-op (tilers GPU nao gostam).
// Shaders em GLSL ES 1.00 => compativeis com contextos ES 2.0 e 3.x.
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
static void logInfo(const char* fmt, ...); static void logWarn(const char* fmt, ...); static void logError(const char* fmt, ...);

struct GnV { float x, y, z, u, v, r, g, b, a; };   // = struct V do mundo (36 bytes): VBOs dos chunks sao usados SEM copia

// ---- funcoes GLES carregadas em runtime (eglGetProcAddress via SDL): nao depende de libGL/gl4es nem de cabecalhos GLES ----
#define GN_FUNCS(X) \
 X(void, glEnable, (GLenum)) X(void, glDisable, (GLenum)) X(void, glBlendFunc, (GLenum, GLenum)) X(void, glDepthMask, (GLboolean)) X(void, glDepthFunc, (GLenum)) \
 X(void, glColorMask, (GLboolean, GLboolean, GLboolean, GLboolean)) X(void, glPolygonOffset, (GLfloat, GLfloat)) X(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat)) \
 X(void, glClear, (GLbitfield)) X(void, glViewport, (GLint, GLint, GLsizei, GLsizei)) X(void, glScissor, (GLint, GLint, GLsizei, GLsizei)) \
 X(void, glBindTexture, (GLenum, GLuint)) X(void, glGenTextures, (GLsizei, GLuint*)) X(void, glDeleteTextures, (GLsizei, const GLuint*)) \
 X(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
 X(void, glTexSubImage2D, (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*)) X(void, glTexParameteri, (GLenum, GLenum, GLint)) \
 X(void, glActiveTexture, (GLenum)) X(void, glBlendColor, (GLfloat, GLfloat, GLfloat, GLfloat)) X(void, glStencilOp, (GLenum, GLenum, GLenum)) \
 X(void, glStencilFunc, (GLenum, GLint, GLuint)) X(void, glClearStencil, (GLint)) X(void, glCopyTexSubImage2D, (GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) \
 X(void, glPixelStorei, (GLenum, GLint)) X(void, glLineWidth, (GLfloat)) X(const GLubyte*, glGetString, (GLenum)) X(void, glGetIntegerv, (GLenum, GLint*)) \
 X(void, glGenBuffers, (GLsizei, GLuint*)) X(void, glBindBuffer, (GLenum, GLuint)) X(void, glBufferData, (GLenum, ptrdiff_t, const void*, GLenum)) \
 X(void, glBufferSubData, (GLenum, ptrdiff_t, ptrdiff_t, const void*)) X(void, glDeleteBuffers, (GLsizei, const GLuint*)) \
 X(void, glDrawArrays, (GLenum, GLint, GLsizei)) X(void, glDrawElements, (GLenum, GLsizei, GLenum, const void*)) \
 X(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) X(void, glEnableVertexAttribArray, (GLuint)) X(void, glDisableVertexAttribArray, (GLuint)) \
 X(GLuint, glCreateShader, (GLenum)) X(void, glShaderSource, (GLuint, GLsizei, const char* const*, const GLint*)) X(void, glCompileShader, (GLuint)) \
 X(void, glGetShaderiv, (GLuint, GLenum, GLint*)) X(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei*, char*)) X(void, glDeleteShader, (GLuint)) \
 X(GLuint, glCreateProgram, ()) X(void, glAttachShader, (GLuint, GLuint)) X(void, glBindAttribLocation, (GLuint, GLuint, const char*)) X(void, glLinkProgram, (GLuint)) \
 X(void, glGetProgramiv, (GLuint, GLenum, GLint*)) X(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei*, char*)) X(void, glUseProgram, (GLuint)) \
 X(GLint, glGetUniformLocation, (GLuint, const char*)) X(void, glUniform1i, (GLint, GLint)) X(void, glUniform1f, (GLint, GLfloat)) \
 X(void, glUniform4fv, (GLint, GLsizei, const GLfloat*)) X(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat*))
#define GN_DECL(R, N, A) typedef R (*GnPFN_##N) A; static GnPFN_##N r_##N = nullptr;
GN_FUNCS(GN_DECL)
static bool glnLoadProcs() {
    bool ok = true;
#define GN_LOAD(R, N, A) r_##N = (GnPFN_##N)SDL_GL_GetProcAddress(#N); if (!r_##N) { ok = false; logError("GLES nativo: funcao ausente: %s", #N); }
    GN_FUNCS(GN_LOAD)
    return ok;
}
// constantes ES que o cabecalho GL de desktop pode nao ter (so as que NAO sao do jogo; as do jogo usam os nomes GL_*)
enum : unsigned { GNC_ARRAY = 0x8892, GNC_ELEM = 0x8893, GNC_STREAM = 0x88E0, GNC_STATIC = 0x88E4, GNC_VS = 0x8B31, GNC_FS = 0x8B30, GNC_COMPILE_OK = 0x8B81, GNC_LINK_OK = 0x8B82,
                  GNC_RGBA = 0x1908, GNC_RGBA8 = 0x8058, GNC_CLAMP = 0x2900, GNC_CLAMP_EDGE = 0x812F, GNC_TEX_WRAP_S = 0x2802, GNC_TEX_WRAP_T = 0x2803, GNC_USHORT = 0x1403,
                  GNC_TRIANGLES = 0x0004, GNC_TRI_STRIP = 0x0005, GNC_FLOAT = 0x1406, GNC_UBYTE = 0x1401, GNC_TEX0 = 0x84C0 };

// ---- estado do "pipeline fixo" emulado ----
enum { GV_MAT, GV_CUR, GV_FOG, GV_AT, GV_SKY, GV_AMB, GV_N };
struct GnProg { GLuint id = 0; GLint uMVP = -1, uMVZ = -1, uCur = -1, uFog = -1, uFogC = -1, uSky = -1, uAmb = -1, uEnv = -1, uCam = -1, uAref = -1; uint32_t seen[GV_N] = {0}; bool bad = false; };
struct GnAttr { GLuint buf = 0; const void* ptr = nullptr; int size = 4; GLenum type = 0; int stride = 0; bool en = false; };
struct GnPart { GLuint buf = 0; GLsizei n = 0; bool quads = false; GLenum mode = 0; };
static struct { float sky[4] = {0, 0, 1, 1}, env[4] = {0, 1, 0, 0}, cam[4] = {0, 0, 0, 0}; } vkS;   // escrito por skyShaderOn()/skyShaderOff() (sky_cycle.inc)

static struct GnState {
    float cur[4] = {1, 1, 1, 1}, tc[2] = {0, 0}, amb[4] = {0.2f, 0.2f, 0.2f, 1.f}, fogC[4] = {0, 0, 0, 0};
    bool tex2d = false, atest = false, fog = false, light = false;
    GLenum afn = 0, aref_dummy = 0; float aref = 0.f;
    float fogS = 0.f, fogE = 1.f, fogD = 1.f; int fogM = 1;
    int mmode = 0; float mat[3][32][16]; int top[3] = {0, 0, 0};
    GnAttr arr[3];                 // 0 posicao, 1 uv, 2 cor (ponteiros/VBO como o jogo configurou)
    GLuint ab = 0, eb = 0, tex = 0; GLenum unit = GNC_TEX0;
    int8_t cap[6] = {-1, -1, -1, -1, -1, -1};   // blend, depth, cull, stencil, scissor, polyoffset
    GLenum bs = 0, bd = 0, dfn = 0; int8_t dmask = -1; int cmask = -1; float pof = 1e30f, pou = 1e30f;
    float vp[4] = {0, 0, 0, 0};
} gn;
static uint32_t gnVer[GV_N] = {1, 1, 1, 1, 1, 1}; static uint32_t gnMatVer = 1, gnMatDone = 0; static float gnMVP[16], gnMVZ[4];
static decltype(vkS) gnSkyPrev; static GnProg* gnCurProg = nullptr; static GnAttr gnApplied[3]; static bool gnReady = false;
static std::unordered_map<uint32_t, GnProg*> gnProgs;
static bool gnFancy = false;   // agua realista (ondas/fresnel/reflexo): so se o jogador escolher um pacote de shader (shd.sel>0); padrao DESLIGADO = agua simples e barata
static GLuint gnRing = 0, gnQIB = 0; static size_t gnRingOff = 0; static const size_t GN_RING = 8u << 20; static const int GN_QMAX = 65532;   // 16383 quads por desenho (indices de 16 bits)
static std::vector<GnV> gnImm; static GLenum gnImmMode = 0; static int gnListCur = -1; static std::vector<std::vector<GnPart>> gnLists;

// ---- matrizes (coluna-maior, como o GL). Multiplicacao a direita especializada por operacao ----
static void gnId(float* m) { memset(m, 0, 64); m[0] = m[5] = m[10] = m[15] = 1.f; }
static inline float* gnM() { return gn.mat[gn.mmode][gn.top[gn.mmode]]; }
static inline void gnDirtyM() { if (gn.mmode < 2) gnMatVer++; }
static void gnMulR(float* a, const float* b) { float r[16]; for (int c = 0; c < 4; c++) for (int i = 0; i < 4; i++) r[c * 4 + i] = a[i] * b[c * 4] + a[4 + i] * b[c * 4 + 1] + a[8 + i] * b[c * 4 + 2] + a[12 + i] * b[c * 4 + 3]; memcpy(a, r, 64); }
static void glnMatrixMode(GLenum m) { gn.mmode = m == GL_PROJECTION ? 1 : m == GL_TEXTURE ? 2 : 0; }
static void glnLoadIdentity() { gnId(gnM()); gnDirtyM(); }
static void glnLoadMatrixf(const GLfloat* m) { memcpy(gnM(), m, 64); gnDirtyM(); }
static void glnPushMatrix() { if (gn.top[gn.mmode] < 31) { memcpy(gn.mat[gn.mmode][gn.top[gn.mmode] + 1], gnM(), 64); gn.top[gn.mmode]++; } }
static void glnPopMatrix() { if (gn.top[gn.mmode] > 0) { gn.top[gn.mmode]--; gnDirtyM(); } }
static void glnTranslatef(GLfloat x, GLfloat y, GLfloat z) { float* m = gnM(); for (int i = 0; i < 4; i++) m[12 + i] += m[i] * x + m[4 + i] * y + m[8 + i] * z; gnDirtyM(); }
static void glnScalef(GLfloat x, GLfloat y, GLfloat z) { float* m = gnM(); for (int i = 0; i < 4; i++) { m[i] *= x; m[4 + i] *= y; m[8 + i] *= z; } gnDirtyM(); }
static void glnRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) {
    const float l = sqrtf(x * x + y * y + z * z); if (l < 1e-8f) return; x /= l; y /= l; z /= l;
    const float r = a * 0.017453292519943295f, c = cosf(r), s = sinf(r), t = 1.f - c; float* m = gnM();
    const float R[9] = {t * x * x + c, t * x * y + s * z, t * x * z - s * y, t * x * y - s * z, t * y * y + c, t * y * z + s * x, t * x * z + s * y, t * y * z - s * x, t * z * z + c};
    for (int i = 0; i < 4; i++) { const float a0 = m[i], a1 = m[4 + i], a2 = m[8 + i];
        m[i] = a0 * R[0] + a1 * R[1] + a2 * R[2]; m[4 + i] = a0 * R[3] + a1 * R[4] + a2 * R[5]; m[8 + i] = a0 * R[6] + a1 * R[7] + a2 * R[8]; }
    gnDirtyM();
}
static void glnOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    float o[16]; gnId(o); o[0] = (float)(2 / (r - l)); o[5] = (float)(2 / (t - b)); o[10] = (float)(-2 / (f - n)); o[12] = (float)(-(r + l) / (r - l)); o[13] = (float)(-(t + b) / (t - b)); o[14] = (float)(-(f + n) / (f - n));
    gnMulR(gnM(), o); gnDirtyM();
}
static void glnFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    float o[16]; memset(o, 0, 64); o[0] = (float)(2 * n / (r - l)); o[5] = (float)(2 * n / (t - b)); o[8] = (float)((r + l) / (r - l)); o[9] = (float)((t + b) / (t - b)); o[10] = (float)(-(f + n) / (f - n)); o[11] = -1.f; o[14] = (float)(-2 * f * n / (f - n));
    gnMulR(gnM(), o); gnDirtyM();
}
static void gnUpdMVP() {
    if (gnMatDone == gnMatVer) return; gnMatDone = gnMatVer; const float* M = gn.mat[0][gn.top[0]]; const float* P = gn.mat[1][gn.top[1]];
    for (int c = 0; c < 4; c++) for (int i = 0; i < 4; i++) gnMVP[c * 4 + i] = P[i] * M[c * 4] + P[4 + i] * M[c * 4 + 1] + P[8 + i] * M[c * 4 + 2] + P[12 + i] * M[c * 4 + 3];
    gnMVZ[0] = M[2]; gnMVZ[1] = M[6]; gnMVZ[2] = M[10]; gnMVZ[3] = M[14]; gnVer[GV_MAT]++;   // profundidade de olho (neblina)
}

// ---- shader em variantes. key: bit0 TEX | bits1-4 AF (0 sem teste; 1+func-GL_NEVER) | bits5-6 FOG(0..3) | bits7-8 SKY(0 off,1 mundo,2 agua) | bit9 VCOL | bit10 LIT ----
static const char* kGnVS = R"GLSL(
precision highp float;
attribute vec3 aPos; attribute vec2 aUV; attribute vec4 aCol;
uniform mat4 uMVP; uniform vec4 uMVZ; uniform vec4 uCur; uniform vec4 uFog; uniform vec4 uSky; uniform vec4 uAmb;
varying vec2 vUV; varying vec4 vCol;
#if FOG > 0
varying float vF;
#endif
#if SKY == 2
varying float vD; varying vec3 vW;
#endif
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
#if VCOL
    vec4 c = aCol;
#else
    vec4 c = uCur;
#endif
#if SKY > 0
    float v = floor(c.a * 2097152.0 + 0.5);
    float cc = floor(v * (1.0 / 4096.0)); v -= cc * 4096.0;
    if (cc > 511.5) { cc = 509.0; v = 4096.0; }
    float sk4 = floor(v * (1.0 / 64.0));
    float sk = sk4 * 0.25;
    float bl = (v - sk4 * 64.0) * 0.25;
#if SKY >= 2
    sk = min(15.0, sk + 2.0);
#endif
    float sv = uSky.x * exp2(0.3219281 * sk);               // = 0.94*0.8^(15-sk)*escala (constantes ja em uSky.x)
    float rr = floor(cc * (1.0 / 64.0)); float gg = floor((cc - rr * 64.0) * 0.125);
    vec3 bc = (step(0.01, bl) * (0.06 + 0.033073 * exp2(0.3219281 * bl)) * (1.0 / 7.0)) * vec3(rr, gg, cc - rr * 64.0 - gg * 8.0);
    vec3 L = max(vec3(0.06 + sv * uSky.y, 0.06 + sv * uSky.y, 0.06 + sv), bc);
    L = mix(L, vec3(1.0), step(15.6, bl));
    c = vec4(c.rgb * L, SKYA);
#elif LIT
    c.rgb *= uAmb.rgb;
#endif
    vCol = c;
#if FOG > 0 || SKY == 2
    float d = abs(dot(uMVZ, vec4(aPos, 1.0)));
#endif
#if FOG == 1
    vF = uFog.x + uFog.y * d;
#elif FOG == 2
    vF = exp2(uFog.z * d);
#elif FOG == 3
    float t = uFog.w * d; vF = exp2(-t * t);
#endif
#if SKY == 2
    vD = d; vW = aPos;
#endif
}
)GLSL";
static const char* kGnFS = R"GLSL(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
uniform sampler2D uTex; uniform vec4 uFogC; uniform vec4 uEnv; uniform vec4 uCam; uniform float uAref;
varying vec2 vUV; varying vec4 vCol;
#if FOG > 0
varying float vF;
#endif
#if SKY == 2
varying float vD; varying vec3 vW;
#endif
void main() {
    vec4 c = vCol;
#if TEX
    c *= texture2D(uTex, vUV);
#endif
#if AF == 1
    discard;
#elif AF == 2
    if (!(c.a < uAref)) discard;
#elif AF == 3
    if (abs(c.a - uAref) >= 1e-4) discard;
#elif AF == 4
    if (!(c.a <= uAref)) discard;
#elif AF == 5
    if (c.a <= uAref) discard;
#elif AF == 6
    if (abs(c.a - uAref) < 1e-4) discard;
#elif AF == 7
    if (c.a < uAref) discard;
#endif
#if SKY == 2   // agua: ondas, fresnel, reflexo do ceu e brilho do sol (sem reflexo planar)
    vec3 sun = uEnv.xyz; float t = uEnv.w;
    vec3 vd = normalize(vW - uCam.xyz);
    float dayK = clamp(clamp(sun.y, -0.2, 1.0) * 4.0 + 0.3, 0.0, 1.0);
    vec2 w = vW.xz;
    vec2 g = vec2(cos(w.x * 1.7 + t * 1.1 + w.y * 0.9), cos(w.y * 2.1 - t * 0.9 + w.x * 0.6)) * 0.07;
    g += vec2(cos(w.x * 3.9 - t * 1.7 + w.y * 2.3), cos(w.y * 4.4 + t * 1.5 - w.x * 1.9)) * 0.04;
    g *= 1.0 - smoothstep(16.0, 56.0, vD);
    vec3 nw = normalize(vec3(-g.x, 1.0, -g.y));
    float q = 1.0 - clamp(dot(-vd, nw), 0.0, 1.0); q *= q; q *= q;
    float fr = 0.05 + 0.95 * q;
    vec3 rf = reflect(vd, nw); rf.y = abs(rf.y);
    vec3 zen = uFogC.rgb * vec3(0.55, 0.75, 1.15);
    vec3 refl = mix(uFogC.rgb, zen, clamp(rf.y * 1.6, 0.0, 1.0));
    float sd2 = max(dot(rf, sun), 0.0);
    float l2 = log2(max(sd2, 1e-6)); float sp = exp2(l2 * 380.0) * 1.4 + exp2(l2 * 36.0) * 0.16; sp = sp / (1.0 + sp);
    float sunV = smoothstep(-0.02, 0.12, sun.y);
    vec3 wb = mix(vec3(0.06, 0.27, 0.36) * (0.3 + 0.7 * dayK), c.rgb * vec3(0.8, 1.1, 1.2), 0.3);
    c.rgb = mix(wb, refl, clamp(fr * 1.3 + 0.08, 0.0, 1.0)) + vec3(1.0, 0.82, 0.58) * sp * sunV * (0.35 + 0.65 * fr);
    c.rgb *= 0.94 + 0.12 * (g.x + g.y) * 8.0;
    c.a = clamp(c.a + fr * 0.6 + sp * 0.3, 0.0, 0.96);
#endif
#if FOG > 0
    c.rgb = mix(uFogC.rgb, c.rgb, clamp(vF, 0.0, 1.0));
#endif
    gl_FragColor = c;
}
)GLSL";
static GLuint gnCompile(GLenum type, const std::string& src) {
    const GLuint s = r_glCreateShader(type); const char* p = src.c_str(); r_glShaderSource(s, 1, &p, nullptr); r_glCompileShader(s);
    GLint ok = 0; r_glGetShaderiv(s, GNC_COMPILE_OK, &ok);
    if (!ok) { char log[1024] = {0}; r_glGetShaderInfoLog(s, sizeof log - 1, nullptr, log); logError("GLES nativo: shader %s falhou: %s", type == GNC_VS ? "vertex" : "fragment", log); r_glDeleteShader(s); return 0; }
    return s;
}
static GnProg* gnProgFor(uint32_t k) {
    auto it = gnProgs.find(k); if (it != gnProgs.end()) return it->second;
    GnProg* P = new GnProg(); gnProgs[k] = P;
    const int af = (k >> 1) & 15, fog = (k >> 5) & 3, sky = (k >> 7) & 3;
    std::string h = "#define TEX " + std::to_string(k & 1) + "\n#define AF " + std::to_string(af) + "\n#define FOG " + std::to_string(fog) + "\n#define SKY " + std::to_string(sky) +
                    "\n#define VCOL " + std::to_string((k >> 9) & 1) + "\n#define LIT " + std::to_string((k >> 10) & 1) + "\n#define SKYA " + (sky >= 2 ? "0.62" : "1.0") + "\n";
    const GLuint vs = gnCompile(GNC_VS, h + kGnVS), fs = gnCompile(GNC_FS, h + kGnFS);
    if (!vs || !fs) { P->bad = true; return P; }
    P->id = r_glCreateProgram(); r_glAttachShader(P->id, vs); r_glAttachShader(P->id, fs);
    r_glBindAttribLocation(P->id, 0, "aPos"); r_glBindAttribLocation(P->id, 1, "aUV"); r_glBindAttribLocation(P->id, 2, "aCol");
    r_glLinkProgram(P->id); r_glDeleteShader(vs); r_glDeleteShader(fs);
    GLint ok = 0; r_glGetProgramiv(P->id, GNC_LINK_OK, &ok);
    if (!ok) { char log[1024] = {0}; r_glGetProgramInfoLog(P->id, sizeof log - 1, nullptr, log); logError("GLES nativo: link falhou (key %u): %s", k, log); P->bad = true; return P; }
    auto L = [&](const char* n) { return r_glGetUniformLocation(P->id, n); };
    P->uMVP = L("uMVP"); P->uMVZ = L("uMVZ"); P->uCur = L("uCur"); P->uFog = L("uFog"); P->uFogC = L("uFogC"); P->uSky = L("uSky"); P->uAmb = L("uAmb"); P->uEnv = L("uEnv"); P->uCam = L("uCam"); P->uAref = L("uAref");
    r_glUseProgram(P->id); const GLint t = L("uTex"); if (t >= 0) r_glUniform1i(t, 0); gnCurProg = P;   // sampler sempre na unidade 0
    logInfo("GLES nativo: variante de shader %u compilada (tex=%u af=%d fog=%d sky=%d [3=agua simples])", k, k & 1, af, fog, sky);
    return P;
}
// ---- cache de ligacao de buffers e atributos ----
static inline void gnBindAB(GLuint b) { if (gn.ab != b) { gn.ab = b; r_glBindBuffer(GNC_ARRAY, b); } }
static inline void gnBindEB(GLuint b) { if (gn.eb != b) { gn.eb = b; r_glBindBuffer(GNC_ELEM, b); } }
static void gnSetAttr(int i, const GnAttr& a, ptrdiff_t first) {
    GnAttr& c = gnApplied[i];
    if (!a.en) { if (c.en) { r_glDisableVertexAttribArray(i); c.en = false; } return; }
    const void* p = (const char*)a.ptr + first * a.stride;
    if (!c.en) { r_glEnableVertexAttribArray(i); c.en = true; c.type = 0; }
    if (c.buf != a.buf || c.ptr != p || c.size != a.size || c.type != a.type || c.stride != a.stride) {
        gnBindAB(a.buf); r_glVertexAttribPointer(i, a.size, a.type, a.type == GNC_UBYTE, a.stride, p); c.buf = a.buf; c.ptr = p; c.size = a.size; c.type = a.type; c.stride = a.stride;
    }
}
// ---- escolhe a variante, liga o programa e reenvia so os grupos de uniformes que mudaram ----
static bool gnPrepare(bool vcol) {
    int af = 0; if (gn.atest && gn.afn != (GLenum)GL_ALWAYS) af = 1 + (int)(gn.afn - (GLenum)GL_NEVER); if (af < 0 || af > 7) af = 0;
    const int sky = vkS.sky[0] > 0.5f ? (vkS.sky[1] > 0.5f ? (gnFancy ? 2 : 3) : 1) : 0, fog = gn.fog ? 1 + gn.fogM : 0;
    const uint32_t key = (gn.tex2d ? 1u : 0u) | ((uint32_t)af << 1) | ((uint32_t)fog << 5) | ((uint32_t)sky << 7) | (vcol ? 1u << 9 : 0u) | ((gn.light && sky == 0) ? 1u << 10 : 0u);
    static uint32_t lastKey = ~0u; static GnProg* lastP = nullptr;
    GnProg* P = (key == lastKey) ? lastP : gnProgFor(key); lastKey = key; lastP = P;
    if (!P || P->bad) return false;
    if (gnCurProg != P) { r_glUseProgram(P->id); gnCurProg = P; }
    gnUpdMVP();
    if (memcmp(&vkS, &gnSkyPrev, sizeof vkS)) { gnSkyPrev = vkS; gnVer[GV_SKY]++; }
    if (P->seen[GV_MAT] != gnVer[GV_MAT]) { P->seen[GV_MAT] = gnVer[GV_MAT]; if (P->uMVP >= 0) r_glUniformMatrix4fv(P->uMVP, 1, 0, gnMVP); if (P->uMVZ >= 0) r_glUniform4fv(P->uMVZ, 1, gnMVZ); }
    if (P->seen[GV_CUR] != gnVer[GV_CUR]) { P->seen[GV_CUR] = gnVer[GV_CUR]; if (P->uCur >= 0) r_glUniform4fv(P->uCur, 1, gn.cur); }
    if (P->seen[GV_AT] != gnVer[GV_AT]) { P->seen[GV_AT] = gnVer[GV_AT]; if (P->uAref >= 0) r_glUniform1f(P->uAref, gn.aref); }
    if (P->seen[GV_AMB] != gnVer[GV_AMB]) { P->seen[GV_AMB] = gnVer[GV_AMB]; if (P->uAmb >= 0) r_glUniform4fv(P->uAmb, 1, gn.amb); }
    if (P->seen[GV_FOG] != gnVer[GV_FOG]) {
        P->seen[GV_FOG] = gnVer[GV_FOG];
        if (P->uFog >= 0) { const float inv = 1.f / std::max(gn.fogE - gn.fogS, 1e-4f); const float f[4] = {gn.fogE * inv, -inv, -gn.fogD * 1.442695f, gn.fogD * 1.2011224f}; r_glUniform4fv(P->uFog, 1, f); }
        if (P->uFogC >= 0) r_glUniform4fv(P->uFogC, 1, gn.fogC);
    }
    if (P->seen[GV_SKY] != gnVer[GV_SKY]) {
        P->seen[GV_SKY] = gnVer[GV_SKY];
        if (P->uSky >= 0) { const float s[4] = {0.94f * vkS.sky[2] * 0.035184372f, vkS.sky[3], 0.f, 0.f}; r_glUniform4fv(P->uSky, 1, s); }   // 0.035184372 = 0.8^15
        if (P->uEnv >= 0) r_glUniform4fv(P->uEnv, 1, vkS.env);
        if (P->uCam >= 0) r_glUniform4fv(P->uCam, 1, vkS.cam);
    }
    return true;
}
static void gnBindQIB() {
    gnBindEB(gnQIB);
}
// desenha n vertices a partir de `first` com os atributos A (3). quads => triangulos pelo IBO fixo, em fatias de 16383 quads
static void gnDraw(GLenum mode, GLint first, GLsizei n, bool quads, const GnAttr* A, bool vcol) {
    if (n <= 0 || !A[0].en || !gnPrepare(vcol)) return;
    if (quads) {
        gnBindQIB();
        for (; n >= 4;) { const int c = std::min(n, GN_QMAX) & ~3; for (int i = 0; i < 3; i++) gnSetAttr(i, A[i], first); r_glDrawElements(GNC_TRIANGLES, c / 4 * 6, GNC_USHORT, nullptr); first += c; n -= c; }
    } else { for (int i = 0; i < 3; i++) gnSetAttr(i, A[i], first); r_glDrawArrays(mode, 0, n); }
}
static void gnImmAttrs(GLuint buf, size_t off, GnAttr* A) {
    A[0] = {buf, (const char*)0 + off, 3, GNC_FLOAT, 36, true}; A[1] = {buf, (const char*)0 + off + 12, 2, GNC_FLOAT, 36, true}; A[2] = {buf, (const char*)0 + off + 20, 4, GNC_FLOAT, 36, true};
}
// ---- modo imediato: glBegin/glEnd -> lote convertido (quads/fan/strip) -> ring buffer (ou VBO estatico se estiver gravando display list) ----
static void gnConvert(GLenum m, const std::vector<GnV>& in, std::vector<GnV>& out, bool& quads, GLenum& dm) {
    quads = false; dm = m; const size_t n = in.size();
    if (m == GL_QUADS) { out = in; out.resize(n & ~(size_t)3); quads = true; }
    else if (m == GL_QUAD_STRIP) { quads = true; for (size_t i = 0; i + 3 < n; i += 2) { out.push_back(in[i]); out.push_back(in[i + 1]); out.push_back(in[i + 3]); out.push_back(in[i + 2]); } }
    else if (m == GL_TRIANGLE_FAN) { dm = GNC_TRIANGLES; for (size_t i = 1; i + 1 < n; i++) { out.push_back(in[0]); out.push_back(in[i]); out.push_back(in[i + 1]); } }
    else out = in;
}
static void glnBegin(GLenum m) { gnImmMode = m; gnImm.clear(); }
static void glnEnd() {
    if (gnImm.empty()) return; static std::vector<GnV> out; out.clear(); bool q; GLenum dm; gnConvert(gnImmMode, gnImm, out, q, dm); gnImm.clear(); if (out.empty()) return;
    const size_t bytes = out.size() * sizeof(GnV);
    if (gnListCur >= 0) {   // gravando display list: vira VBO estatico
        GnPart p; r_glGenBuffers(1, &p.buf); gnBindAB(p.buf); r_glBufferData(GNC_ARRAY, bytes, out.data(), GNC_STATIC); p.n = (GLsizei)out.size(); p.quads = q; p.mode = dm; gnLists[gnListCur].push_back(p); return;
    }
    // lote imediato: ARRAYS DO CLIENTE (sem buffer). Em Mali/Adreno, glBufferSubData num buffer em uso copia o buffer inteiro => segundos por quadro.
    // O driver copia so os bytes deste lote no momento do desenho.
    const char* base = (const char*)out.data(); GnAttr A[3];
    A[0] = {0, base, 3, GNC_FLOAT, 36, true}; A[1] = {0, base + 12, 2, GNC_FLOAT, 36, true}; A[2] = {0, base + 20, 4, GNC_FLOAT, 36, true};
    gnDraw(dm, 0, (GLsizei)out.size(), q, A, true);
}
static void glnVertex3f(GLfloat x, GLfloat y, GLfloat z) { gnImm.push_back({x, y, z, gn.tc[0], gn.tc[1], gn.cur[0], gn.cur[1], gn.cur[2], gn.cur[3]}); }
static void glnVertex2f(GLfloat x, GLfloat y) { glnVertex3f(x, y, 0.f); } static void glnVertex3d(GLdouble x, GLdouble y, GLdouble z) { glnVertex3f((float)x, (float)y, (float)z); }
static void glnTexCoord2f(GLfloat u, GLfloat v) { gn.tc[0] = u; gn.tc[1] = v; }
static void glnColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { if (gn.cur[0] == r && gn.cur[1] == g && gn.cur[2] == b && gn.cur[3] == a) return; gn.cur[0] = r; gn.cur[1] = g; gn.cur[2] = b; gn.cur[3] = a; gnVer[GV_CUR]++; }
static void glnColor3f(GLfloat r, GLfloat g, GLfloat b) { glnColor4f(r, g, b, 1.f); }
// display lists: so a geometria (como no backend Vulkan); o estado vale na hora do glCallList
static GLuint glnGenLists(GLsizei n) { const GLuint f = (GLuint)gnLists.size() + 1; for (int i = 0; i < n; i++) gnLists.emplace_back(); return f; }
static void glnNewList(GLuint id, GLenum) { if (id >= 1 && id <= gnLists.size()) { gnListCur = (int)id - 1; for (auto& p : gnLists[gnListCur]) r_glDeleteBuffers(1, &p.buf); gnLists[gnListCur].clear(); } }
static void glnEndList() { gnListCur = -1; }
static void glnCallList(GLuint id) {
    if (id < 1 || id > gnLists.size()) return;
    for (auto& p : gnLists[id - 1]) { GnAttr A[3]; gnImmAttrs(p.buf, 0, A); gnDraw(p.mode, 0, p.n, p.quads, A, true); }
}
// ---- arrays do cliente (VBO dos chunks sem copia) ----
static void glnEnableClientState(GLenum a) { if (a == GL_VERTEX_ARRAY) gn.arr[0].en = true; else if (a == GL_TEXTURE_COORD_ARRAY) gn.arr[1].en = true; else if (a == GL_COLOR_ARRAY) gn.arr[2].en = true; }
static void glnDisableClientState(GLenum a) { if (a == GL_VERTEX_ARRAY) gn.arr[0].en = false; else if (a == GL_TEXTURE_COORD_ARRAY) gn.arr[1].en = false; else if (a == GL_COLOR_ARRAY) gn.arr[2].en = false; }
static void gnPtr(int i, GLint size, GLenum type, GLsizei stride, const void* p) { GnAttr& a = gn.arr[i]; a.buf = gn.ab; a.ptr = p; a.size = size; a.type = type; a.stride = stride ? stride : size * (type == GL_UNSIGNED_BYTE ? 1 : 4); }
static void glnVertexPointer(GLint s, GLenum t, GLsizei st, const void* p) { gnPtr(0, s, t, st, p); }
static void glnTexCoordPointer(GLint s, GLenum t, GLsizei st, const void* p) { gnPtr(1, s, t, st, p); }
static void glnColorPointer(GLint s, GLenum t, GLsizei st, const void* p) { gnPtr(2, s, t, st, p); }
static void glnDrawArrays(GLenum m, GLint first, GLsizei n) { const bool q = m == GL_QUADS; gnDraw(q ? GNC_TRIANGLES : m, first, n, q, gn.arr, gn.arr[2].en); }
static void glnDrawElements(GLenum m, GLsizei n, GLenum t, const void* idx) {
    if (n <= 0 || !gn.arr[0].en || !gnPrepare(gn.arr[2].en)) return;
    for (int i = 0; i < 3; i++) gnSetAttr(i, gn.arr[i], 0);
    r_glDrawElements(m, n, t, idx);   // o IBO (ELEMENT_ARRAY_BUFFER) e o que o jogo ligou
}
// ---- buffers/texturas: repasse com cache ----
static void glnGenBuffers(GLsizei n, GLuint* b) { r_glGenBuffers(n, b); }
static void glnBindBuffer(GLenum t, GLuint b) { if (t == GNC_ARRAY) gnBindAB(b); else if (t == GNC_ELEM) gnBindEB(b); else r_glBindBuffer(t, b); }
static void glnBufferData(GLenum t, ptrdiff_t sz, const void* d, GLenum u) { r_glBufferData(t, sz, d, u); }
static void glnDeleteBuffers(GLsizei n, const GLuint* b) { for (int i = 0; i < n; i++) { if (b[i] == gn.ab) gn.ab = ~0u; if (b[i] == gn.eb) gn.eb = ~0u; for (auto& a : gnApplied) if (a.buf == b[i]) a.type = 0; } r_glDeleteBuffers(n, b); }
static void glnGenTextures(GLsizei n, GLuint* t) { r_glGenTextures(n, t); }
static void glnBindTexture(GLenum t, GLuint id) { if (gn.unit == GNC_TEX0) { if (gn.tex == id) return; gn.tex = id; } r_glBindTexture(t, id); }
static void glnDeleteTextures(GLsizei n, const GLuint* t) { r_glDeleteTextures(n, t); gn.tex = ~0u; }
static void glnActiveTexture(GLenum u) { gn.unit = u; r_glActiveTexture(u); }
static void glnTexImage2D(GLenum t, GLint l, GLint ifmt, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void* p) { r_glTexImage2D(t, l, (f == (GLenum)GL_RGBA && ty == GNC_UBYTE) ? (GLint)GNC_RGBA : ifmt, w, h, b, f, ty, p); }
static void glnTexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void* p) { r_glTexSubImage2D(t, l, x, y, w, h, f, ty, p); }
static void glnTexParameteri(GLenum t, GLenum p, GLint v) { if ((p == GNC_TEX_WRAP_S || p == GNC_TEX_WRAP_T) && v == (GLint)GNC_CLAMP) v = (GLint)GNC_CLAMP_EDGE; r_glTexParameteri(t, p, v); }
static void glnCopyTexSubImage2D(GLenum t, GLint l, GLint xo, GLint yo, GLint x, GLint y, GLsizei w, GLsizei h) { r_glCopyTexSubImage2D(t, l, xo, yo, x, y, w, h); }
// ---- estado: enable/blend/depth... so chegam ao driver quando mudam ----
static int gnCapIdx(GLenum c) { switch (c) { case GL_BLEND: return 0; case GL_DEPTH_TEST: return 1; case GL_CULL_FACE: return 2; case GL_STENCIL_TEST: return 3; case GL_SCISSOR_TEST: return 4; case GL_POLYGON_OFFSET_FILL: return 5; default: return -1; } }
static void gnSetCap(GLenum c, bool on) {
    if (c == GL_TEXTURE_2D) { gn.tex2d = on; return; } if (c == GL_ALPHA_TEST) { gn.atest = on; return; } if (c == GL_FOG) { gn.fog = on; return; }
    if (c == GL_LIGHTING) { gn.light = on; return; } if (c == GL_COLOR_MATERIAL || c == GL_CLIP_PLANE0) return;   // clip plane: sem equivalente (reflexo planar desligado)
    const int i = gnCapIdx(c); if (i < 0) return; if (gn.cap[i] == (int8_t)on) return; gn.cap[i] = on; if (on) r_glEnable(c); else r_glDisable(c);
}
static void glnEnable(GLenum c) { gnSetCap(c, true); } static void glnDisable(GLenum c) { gnSetCap(c, false); }
static GLboolean glnIsEnabled(GLenum c) { switch (c) { case GL_TEXTURE_2D: return gn.tex2d; case GL_ALPHA_TEST: return gn.atest; case GL_FOG: return gn.fog; case GL_LIGHTING: return gn.light; default: { const int i = gnCapIdx(c); return i >= 0 && gn.cap[i] == 1; } } }
static void glnBlendFunc(GLenum s, GLenum d) { if (gn.bs == s && gn.bd == d) return; gn.bs = s; gn.bd = d; r_glBlendFunc(s, d); }
static void glnDepthMask(GLboolean m) { if (gn.dmask == (int8_t)(m != 0)) return; gn.dmask = m != 0; r_glDepthMask(m); }
static void glnDepthFunc(GLenum f) { if (gn.dfn == f) return; gn.dfn = f; r_glDepthFunc(f); }
static void glnColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { const int m = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0); if (gn.cmask == m) return; gn.cmask = m; r_glColorMask(r, g, b, a); }
static void glnPolygonOffset(GLfloat f, GLfloat u) { if (gn.pof == f && gn.pou == u) return; gn.pof = f; gn.pou = u; r_glPolygonOffset(f, u); }
static void glnAlphaFunc(GLenum f, GLclampf r) { if (gn.afn == f && gn.aref == r) return; gn.afn = f; gn.aref = r; gnVer[GV_AT]++; }
static void glnFogf(GLenum p, GLfloat v) {
    if (p == GL_FOG_START) gn.fogS = v; else if (p == GL_FOG_END) gn.fogE = v; else if (p == GL_FOG_DENSITY) gn.fogD = v;
    else if (p == GL_FOG_MODE) gn.fogM = v == (float)GL_LINEAR ? 0 : v == (float)GL_EXP ? 1 : 2; else return; gnVer[GV_FOG]++;
}
static void glnFogi(GLenum p, GLint v) { glnFogf(p, (float)v); }
static void glnFogfv(GLenum p, const GLfloat* v) { if (p == GL_FOG_COLOR) { memcpy(gn.fogC, v, 16); gnVer[GV_FOG]++; } else glnFogf(p, v[0]); }
static void glnLightModelfv(GLenum p, const GLfloat* v) { if (p == GL_LIGHT_MODEL_AMBIENT && memcmp(gn.amb, v, 16)) { memcpy(gn.amb, v, 16); gnVer[GV_AMB]++; } }
static void glnViewport(GLint x, GLint y, GLsizei w, GLsizei h) { if (gn.vp[0] == x && gn.vp[1] == y && gn.vp[2] == w && gn.vp[3] == h) return; gn.vp[0] = (float)x; gn.vp[1] = (float)y; gn.vp[2] = (float)w; gn.vp[3] = (float)h; r_glViewport(x, y, w, h); }
static void glnClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) { r_glClearColor(r, g, b, a); }
static void glnClear(GLbitfield m) { r_glClear(m); }
static void glnScissor(GLint x, GLint y, GLsizei w, GLsizei h) { r_glScissor(x, y, w, h); }
static void glnStencilOp(GLenum a, GLenum b, GLenum c) { r_glStencilOp(a, b, c); } static void glnStencilFunc(GLenum f, GLint r, GLuint m) { r_glStencilFunc(f, r, m); } static void glnClearStencil(GLint s) { r_glClearStencil(s); }
static void glnBlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) { r_glBlendColor(r, g, b, a); }
static void glnPixelStorei(GLenum p, GLint v) { r_glPixelStorei(p, v); } static void glnLineWidth(GLfloat w) { r_glLineWidth(w); }
static void glnGetFloatv(GLenum p, GLfloat* v) {
    if (p == GL_MODELVIEW_MATRIX) memcpy(v, gn.mat[0][gn.top[0]], 64); else if (p == GL_PROJECTION_MATRIX) memcpy(v, gn.mat[1][gn.top[1]], 64);
    else if (p == GL_FOG_START) v[0] = gn.fogS; else if (p == GL_FOG_END) v[0] = gn.fogE; else v[0] = 0.f;
}
static void glnGetIntegerv(GLenum p, GLint* v) { if (p == GL_VIEWPORT) { for (int i = 0; i < 4; i++) v[i] = (GLint)gn.vp[i]; } else if (p == GL_TEXTURE_BINDING_2D) v[0] = (GLint)gn.tex; else v[0] = 0; }
static const GLubyte* glnGetString(GLenum n) { const GLubyte* s = gnReady ? r_glGetString(n) : nullptr; return s ? s : (const GLubyte*)""; }
static GLenum glnGetError() { return 0; }   // nunca consulta o driver (glGetError sincroniza em alguns drivers)
// sem equivalente no ES (ou sem efeito no jogo): ficam como no-op, igual ao backend Vulkan
static void glnColorMaterial(GLenum, GLenum) {} static void glnReadBuffer(GLenum) {} static void glnDrawBuffer(GLenum) {} static void glnHint(GLenum, GLenum) {}
static void glnClipPlane(GLenum, const GLdouble*) {} static void glnFlush() {}   // glFlush: no-op (em GPUs de tile forcaria o fim do quadro)
static void glnPushAttrib(GLbitfield) {} static void glnPopAttrib() {} static void glnPushClientAttrib(GLbitfield) {} static void glnPopClientAttrib() {}
static void* glnProc(const char* n) {   // o jogo so enxerga estas extensoes; shaders/FBO do jogo ficam indisponiveis (ele desliga sombras/god rays sozinho)
    static const std::pair<const char*, void*> T[] = {{"glGenBuffers", (void*)glnGenBuffers}, {"glBindBuffer", (void*)glnBindBuffer}, {"glBufferData", (void*)glnBufferData}, {"glDeleteBuffers", (void*)glnDeleteBuffers},
                                                      {"glActiveTexture", (void*)glnActiveTexture}, {"glBlendColor", (void*)glnBlendColor}};
    for (const auto& e : T) if (!strcmp(e.first, n)) return e.second; return nullptr;
}
// chamado por androidGLInit() logo depois de SDL_GL_CreateContext
static void glnInit() {
    gnReady = glnLoadProcs(); if (!gnReady) { logError("GLES nativo: contexto sem as funcoes necessarias"); return; }
    for (int m = 0; m < 3; m++) gnId(gn.mat[m][0]);
    gn.ab = ~0u; gnBindAB(0);
    std::vector<uint16_t> idx((size_t)GN_QMAX / 4 * 6); for (size_t q = 0; q < (size_t)GN_QMAX / 4; q++) { const uint16_t b = (uint16_t)(q * 4); uint16_t* p = &idx[q * 6]; p[0] = b; p[1] = b + 1; p[2] = b + 2; p[3] = b; p[4] = b + 2; p[5] = b + 3; }
    r_glGenBuffers(1, &gnQIB); gn.eb = ~0u; gnBindEB(gnQIB); r_glBufferData(GNC_ELEM, (ptrdiff_t)(idx.size() * 2), idx.data(), GNC_STATIC);
    for (auto& a : gnApplied) a = GnAttr(); gn.tex = ~0u;
    logInfo("OpenGL ES nativo: %s | %s | %s", (const char*)r_glGetString(0x1F02), (const char*)r_glGetString(0x1F01), (const char*)r_glGetString(0x1F00));
}

// ---------- redirecionamento: o resto do jogo continua chamando gl* / SDL_GL_* ----------
#undef SDL_GL_GetProcAddress
#define SDL_GL_GetProcAddress glnProc
#define glEnable glnEnable
#define glDisable glnDisable
#define glIsEnabled glnIsEnabled
#define glBlendFunc glnBlendFunc
#define glDepthMask glnDepthMask
#define glDepthFunc glnDepthFunc
#define glColorMask glnColorMask
#define glAlphaFunc glnAlphaFunc
#define glPolygonOffset glnPolygonOffset
#define glFogf glnFogf
#define glFogi glnFogi
#define glFogfv glnFogfv
#define glClearColor glnClearColor
#define glClear glnClear
#define glViewport glnViewport
#define glScissor glnScissor
#define glMatrixMode glnMatrixMode
#define glLoadIdentity glnLoadIdentity
#define glLoadMatrixf glnLoadMatrixf
#define glPushMatrix glnPushMatrix
#define glPopMatrix glnPopMatrix
#define glTranslatef glnTranslatef
#define glScalef glnScalef
#define glRotatef glnRotatef
#define glOrtho glnOrtho
#define glFrustum glnFrustum
#define glGetFloatv glnGetFloatv
#define glGetIntegerv glnGetIntegerv
#define glGetString glnGetString
#define glGetError glnGetError
#define glColor4f glnColor4f
#define glColor3f glnColor3f
#define glTexCoord2f glnTexCoord2f
#define glBegin glnBegin
#define glEnd glnEnd
#define glVertex3f glnVertex3f
#define glVertex2f glnVertex2f
#define glVertex3d glnVertex3d
#define glGenLists glnGenLists
#define glNewList glnNewList
#define glEndList glnEndList
#define glCallList glnCallList
#define glEnableClientState glnEnableClientState
#define glDisableClientState glnDisableClientState
#define glVertexPointer glnVertexPointer
#define glTexCoordPointer glnTexCoordPointer
#define glColorPointer glnColorPointer
#define glDrawArrays glnDrawArrays
#define glDrawElements glnDrawElements
#define glGenBuffers glnGenBuffers
#define glBindBuffer glnBindBuffer
#define glBufferData glnBufferData
#define glDeleteBuffers glnDeleteBuffers
#define glGenTextures glnGenTextures
#define glBindTexture glnBindTexture
#define glDeleteTextures glnDeleteTextures
#define glTexImage2D glnTexImage2D
#define glTexSubImage2D glnTexSubImage2D
#define glTexParameteri glnTexParameteri
#define glActiveTexture glnActiveTexture
#define glBlendColor glnBlendColor
#define glLightModelfv glnLightModelfv
#define glColorMaterial glnColorMaterial
#define glStencilOp glnStencilOp
#define glStencilFunc glnStencilFunc
#define glClearStencil glnClearStencil
#define glReadBuffer glnReadBuffer
#define glDrawBuffer glnDrawBuffer
#define glCopyTexSubImage2D glnCopyTexSubImage2D
#define glPixelStorei glnPixelStorei
#define glHint glnHint
#define glLineWidth glnLineWidth
#define glClipPlane glnClipPlane
#define glFlush glnFlush
#define glPushAttrib glnPushAttrib
#define glPopAttrib glnPopAttrib
#define glPushClientAttrib glnPushClientAttrib
#define glPopClientAttrib glnPopClientAttrib
