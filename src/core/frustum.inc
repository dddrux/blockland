// ================= frustum: teste exato de visibilidade por chunk (OPCIONAL, desligado por padrao) =================
// Usa as matrizes REAIS do OpenGL no quadro atual (projecao * modelview), entao nunca ha atraso nem "chunk sumindo":
// so deixa de desenhar o que esta inteiramente fora da tela, com margem generosa. Ligue com: BLOCKLAND_CULL=1 ./blockland
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
struct Frustum { float pl[5][4]; };
static void frustumFrom(Frustum& F, const float* P, const float* M) {   // matrizes em coluna-maior (formato do glGetFloatv)
    float C[16];
    for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0.f; for (int k = 0; k < 4; k++) s += P[k * 4 + r] * M[c * 4 + k]; C[c * 4 + r] = s; }
    auto row = [&](int i, float* o) { for (int c = 0; c < 4; c++) o[c] = C[c * 4 + i]; };
    float R0[4], R1[4], R2[4], R3[4]; row(0, R0); row(1, R1); row(2, R2); row(3, R3);
    const float* A[5] = {R0, R0, R1, R1, R2}; const float sg[5] = {1.f, -1.f, 1.f, -1.f, 1.f};   // esquerda, direita, baixo, cima, perto
    for (int i = 0; i < 5; i++) {
        float* p = F.pl[i]; for (int c = 0; c < 4; c++) p[c] = R3[c] + sg[i] * A[i][c];
        const float l = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]); if (l > 1e-8f) for (int c = 0; c < 4; c++) p[c] /= l;
    }
}
static Frustum gFrs; static bool gFrsOn = false;   // patchRpiCull.1: frustum do quadro atual (preenchido no main_loop); desligado fora do desenho do mundo
// entidade (centro x,y,z; raio r) inteiramente fora da tela? Sem frustum valido => nunca descarta
static bool frustumOutside(const Frustum& F, float x0, float y0, float z0, float x1, float y1, float z1, float margin);
static inline bool entOutView(double x, double y, double z, float r) { return gFrsOn && frustumOutside(gFrs, (float)x - r, (float)y - r, (float)z - r, (float)x + r, (float)y + r, (float)z + r, 0.f); }
static inline bool frustumOutside(const Frustum& F, float x0, float y0, float z0, float x1, float y1, float z1, float margin) {
    for (int i = 0; i < 5; i++) {
        const float* p = F.pl[i];
        const float px = p[0] >= 0.f ? x1 : x0, py = p[1] >= 0.f ? y1 : y0, pz = p[2] >= 0.f ? z1 : z0;
        if (p[0] * px + p[1] * py + p[2] * pz + p[3] < -margin) return true;
    }
    return false;
}
