// Uso: cd tests && g++ -std=c++17 -O1 -Istubs -Wno-narrowing ../tools/handprev.cpp -o /tmp/handprev -Wl,--unresolved-symbols=ignore-all && /tmp/handprev <id_do_item> <swing 0..1> saida.ppm  (ajuste os caminhos /home/claude/proj no arquivo)
// Pre-visualizador SOFTWARE da mao em 1a pessoa: executa o drawHand/drawHeld REAL com um "GL" minimo (pilha de matrizes + rasterizador).
#include <unistd.h>
#define main game_main
#include "/home/claude/proj/src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
struct M4 { double m[16]; };
static M4 ident() { M4 r; for (int i = 0; i < 16; i++) r.m[i] = (i % 5 == 0); return r; }
static M4 mul(const M4& a, const M4& b) { M4 r; for (int c = 0; c < 4; c++) for (int rr = 0; rr < 4; rr++) { double s = 0; for (int k = 0; k < 4; k++) s += a.m[k * 4 + rr] * b.m[c * 4 + k]; r.m[c * 4 + rr] = s; } return r; }
static M4 gM = ident(); static std::vector<M4> gStk;
void glPushMatrix() { gStk.push_back(gM); } void glPopMatrix() { gM = gStk.back(); gStk.pop_back(); }
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) { M4 t = ident(); t.m[12] = x; t.m[13] = y; t.m[14] = z; gM = mul(gM, t); }
void glScalef(GLfloat x, GLfloat y, GLfloat z) { M4 t = ident(); t.m[0] = x; t.m[5] = y; t.m[10] = z; gM = mul(gM, t); }
void glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { double l = sqrt(x*x+y*y+z*z); x /= l; y /= l; z /= l; double c = cos(a * M_PI / 180), s = sin(a * M_PI / 180), C = 1 - c; M4 t = ident();
    t.m[0] = x*x*C+c; t.m[1] = y*x*C+z*s; t.m[2] = z*x*C-y*s; t.m[4] = x*y*C-z*s; t.m[5] = y*y*C+c; t.m[6] = z*y*C+x*s; t.m[8] = x*z*C+y*s; t.m[9] = y*z*C-x*s; t.m[10] = z*z*C+c; gM = mul(gM, t); }
void glLoadIdentity() { gM = ident(); }
void glMatrixMode(GLenum) {} void glClear(GLbitfield) {} void glEnable(GLenum) {} void glDisable(GLenum) {} void glAlphaFunc(GLenum, GLfloat) {} void glBlendFunc(GLenum, GLenum) {}
void glEnableClientState(GLenum) {} void glDisableClientState(GLenum) {}
static GLuint boundTex = 0; void glBindTexture(GLenum, GLuint t) { boundTex = t; }
static const V* vp = nullptr;
void glVertexPointer(GLint, GLenum, GLsizei, const void* p) { vp = (const V*)((const char*)p - offsetof(V, x)); }
void glTexCoordPointer(GLint, GLenum, GLsizei, const void*) {} void glColorPointer(GLint, GLenum, GLsizei, const void*) {}
static const int SW = 960, SH = 540; static std::vector<float> gZb; static std::vector<uint8_t> gImg;
static void tri(double P[3][3], float U[3][2], float C[3], bool tex) {
    double minx = std::min({P[0][0], P[1][0], P[2][0]}), maxx = std::max({P[0][0], P[1][0], P[2][0]}), miny = std::min({P[0][1], P[1][1], P[2][1]}), maxy = std::max({P[0][1], P[1][1], P[2][1]});
    for (int y = std::max(0, (int)floor(miny)); y <= std::min(SH - 1, (int)ceil(maxy)); y++) for (int x = std::max(0, (int)floor(minx)); x <= std::min(SW - 1, (int)ceil(maxx)); x++) {
        double px = x + .5, py = y + .5, d = (P[1][1]-P[2][1])*(P[0][0]-P[2][0]) + (P[2][0]-P[1][0])*(P[0][1]-P[2][1]); if (fabs(d) < 1e-12) continue;
        double a = ((P[1][1]-P[2][1])*(px-P[2][0]) + (P[2][0]-P[1][0])*(py-P[2][1])) / d, b = ((P[2][1]-P[0][1])*(px-P[2][0]) + (P[0][0]-P[2][0])*(py-P[2][1])) / d, c = 1 - a - b;
        if (a < -1e-6 || b < -1e-6 || c < -1e-6) continue;
        double z = a * P[0][2] + b * P[1][2] + c * P[2][2]; if (z >= gZb[y * SW + x]) continue;
        float r = .62f, g = .46f, bl = .36f;
        if (tex) { double u = a*U[0][0]+b*U[1][0]+c*U[2][0], v = a*U[0][1]+b*U[1][1]+c*U[2][1]; int ax = std::min(ATW - 1, std::max(0, (int)(u * ATW))), ay = std::min(16 * ATLR - 1, std::max(0, (int)(v * 16 * ATLR)));
            const uint8_t* p = &atlas[(ay * ATW + ax) * 4]; if (p[3] < 128) continue; r = p[0] / 255.f; g = p[1] / 255.f; bl = p[2] / 255.f; }
        gZb[y * SW + x] = (float)z; uint8_t* o = &gImg[(y * SW + x) * 3]; o[0] = (uint8_t)(255 * std::min(1.f, r * C[0])); o[1] = (uint8_t)(255 * std::min(1.f, g * C[1])); o[2] = (uint8_t)(255 * std::min(1.f, bl * C[2]));
    }
}
void glDrawArrays(GLenum, GLint, GLsizei n) {
    const double fov = 70 * M_PI / 180, T = tan(fov / 2), Rr = T * SW / SH;
    for (int i = 0; i + 3 < n; i += 4) {
        double P[4][3]; bool ok = true; float U[4][2];
        for (int k = 0; k < 4; k++) { const V& v = vp[i + k]; double x = gM.m[0]*v.x+gM.m[4]*v.y+gM.m[8]*v.z+gM.m[12], y = gM.m[1]*v.x+gM.m[5]*v.y+gM.m[9]*v.z+gM.m[13], z = gM.m[2]*v.x+gM.m[6]*v.y+gM.m[10]*v.z+gM.m[14];
            if (z > -.05) ok = false; P[k][0] = (x / (-z) / Rr * .5 + .5) * SW; P[k][1] = (1 - (y / (-z) / T * .5 + .5)) * SH; P[k][2] = -z; U[k][0] = v.u; U[k][1] = v.v; }
        if (!ok) continue; float C[3] = {vp[i].r, vp[i].g, vp[i].b};
        double Q[3][3]; float UU[3][2];
        for (int t = 0; t < 2; t++) { int id[3] = {0, 1 + t, 2 + t}; for (int k = 0; k < 3; k++) { memcpy(Q[k], P[id[k]], sizeof Q[k]); UU[k][0] = U[id[k]][0]; UU[k][1] = U[id[k]][1]; } tri(Q, UU, C, boundTex == 1); }
    }
}
int main(int argc, char** argv) {
    if (chdir("/home/claude/proj") != 0) return 99;
    applyMcPack(); loadPackItems(); xbInit(); atlasTex = 1; skinTex = 2; buildModel();
    int id = argc > 1 ? atoi(argv[1]) : IT_DAXE; float sw = argc > 2 ? atof(argv[2]) : 0.f; const char* out = argc > 3 ? argv[3] : "/tmp/prev/out.ppm";
    gZb.assign(SW * SH, 1e9f); gImg.assign(SW * SH * 3, 0); for (int i = 0; i < SW * SH; i++) { gImg[i*3] = 90; gImg[i*3+1] = 140; gImg[i*3+2] = 70; }
    gM = ident(); drawHand(id, sw, 1.f);
    FILE* f = fopen(out, "wb"); fprintf(f, "P6\n%d %d\n255\n", SW, SH); fwrite(gImg.data(), 1, gImg.size(), f); fclose(f); return 0;
}
