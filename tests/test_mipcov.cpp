// Mipmap com cobertura de alfa preservada: mato fino nao some, folhas nao engordam.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static float covOf(const uint8_t* m, int w, int tx, int ty, int ts) { int n = 0; for (int y = 0; y < ts; y++) for (int x = 0; x < ts; x++) if (m[((size_t)(ty * ts + y) * w + tx * ts + x) * 4 + 3] >= MIP_REF) n++; return n / (float)(ts * ts); }
int main() {
    const int w = 32, h = 32;   // 2x2 tiles de 16
    std::vector<uint8_t> a((size_t)w * h * 4, 0);
    // tile (0,0): mato fino (5 texels opacos isolados); tile (1,0): folha (60% opaco em padrao xadrez irregular); tile (0,1): bloco solido
    for (int i = 0; i < 5; i++) { uint8_t* p = &a[((size_t)(i * 3) * w + (i * 3 + 1)) * 4]; p[0] = 0; p[1] = 200; p[2] = 0; p[3] = 255; }
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if ((x * 7 + y * 13) % 10 < 6) { uint8_t* p = &a[((size_t)y * w + 16 + x) * 4]; p[0] = 0; p[1] = 150; p[2] = 0; p[3] = 255; }
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { uint8_t* p = &a[((size_t)(16 + y) * w + x) * 4]; p[0] = p[1] = p[2] = 90; p[3] = 255; }
    std::vector<float> cov(4); tileCoverage0(a.data(), w, cov);
    CHECK(cov[2] < 0.f);   // solido: sem furos, nao mexe
    const float c0 = cov[0], c1 = cov[1]; CHECK(c0 > .01f && c0 < .05f && c1 > .5f && c1 < .7f);
    std::vector<uint8_t> cur = a; int cw = w, ch = h;
    for (int l = 1; l <= 3; l++) {
        std::vector<uint8_t> b((size_t)(cw / 2) * (ch / 2) * 4); mipDown(cur.data(), cw, ch, b.data()); cw /= 2; ch /= 2;
        const float rawMat = covOf(b.data(), cw, 0, 0, 16 >> l), rawLeaf = covOf(b.data(), cw, 1, 0, 16 >> l);
        std::vector<uint8_t> c = b; mipCoverage(c.data(), cw, ch, 16 >> l, cov);
        const float mat = covOf(c.data(), cw, 0, 0, 16 >> l), leaf = covOf(c.data(), cw, 1, 0, 16 >> l), sol = covOf(c.data(), cw, 0, 1, 16 >> l);
        printf("  nivel %d: mato cru %.2f -> %.2f (alvo %.2f) | folha cru %.2f -> %.2f (alvo %.2f)\n", l, rawMat, mat, c0, rawLeaf, leaf, c1);
        CHECK(fabsf(leaf - c1) <= fabsf(rawLeaf - c1) + 1e-4f); CHECK(sol == 1.f);
        CHECK(mat > 0.f);   // o mato nao some em nenhum nivel
        if (l == 1) CHECK(fabsf(mat - c0) <= .12f); else CHECK(mat <= .35f);   // texels iguais empatam no corte: pode passar um pouco acima do alvo, mas nunca some
        cur.swap(b);
    }
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
