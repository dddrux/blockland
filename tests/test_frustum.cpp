// Frustum: NUNCA pode cortar uma caixa que aparece na tela (falso positivo = chunk sumindo), e deve cortar a maior parte do que esta fora.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static float rnd() { static uint32_t s = 12345; s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.f; }
int main() {
    long long falseCull = 0, boxesVisible = 0, culled = 0, boxes = 0;
    for (int trial = 0; trial < 400; trial++) {
        const float fov = 50.f + rnd() * 60.f, asp = 1.f + rnd() * .9f, yaw = rnd() * 6.2832f, pit = (rnd() - .5f) * 2.4f;
        const float n = .05f, t = n * tanf(fov * .5f * .0174533f), r = t * asp, f = 700.f;
        float P[16] = {0}; P[0] = n / r; P[5] = n / t; P[10] = -(f + n) / (f - n); P[11] = -1.f; P[14] = -2.f * f * n / (f - n);
        const float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pit), sp = sinf(pit), cam[3] = {rnd() * 400 - 200, 40 + rnd() * 100, rnd() * 400 - 200};
        // modelview = Rx(pit) * Ry(yaw) * T(-cam), coluna-maior
        float Ry[16] = {cy, 0, -sy, 0, 0, 1, 0, 0, sy, 0, cy, 0, 0, 0, 0, 1}, Rx[16] = {1, 0, 0, 0, 0, cp, sp, 0, 0, -sp, cp, 0, 0, 0, 0, 1}, T[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -cam[0], -cam[1], -cam[2], 1}, A[16], M[16];
        auto mul = [](const float* a, const float* b, float* o) { for (int c = 0; c < 4; c++) for (int rr = 0; rr < 4; rr++) { float s = 0; for (int k = 0; k < 4; k++) s += a[k * 4 + rr] * b[c * 4 + k]; o[c * 4 + rr] = s; } };
        mul(Ry, T, A); mul(Rx, A, M);
        Frustum F; frustumFrom(F, P, M);
        auto inside = [&](float x, float y, float z) { float v[4] = {x, y, z, 1}, e[4] = {0}, c[4] = {0};
            for (int rr = 0; rr < 4; rr++) for (int k = 0; k < 4; k++) e[rr] += M[k * 4 + rr] * v[k];
            for (int rr = 0; rr < 4; rr++) for (int k = 0; k < 4; k++) c[rr] += P[k * 4 + rr] * e[k];
            return c[3] > 0 && fabsf(c[0]) <= c[3] && fabsf(c[1]) <= c[3] && c[2] >= -c[3] && c[2] <= c[3]; };
        for (int b = 0; b < 300; b++) {   // caixas do tamanho de um chunk, perto e longe
            const float x0 = cam[0] + (rnd() - .5f) * 700, z0 = cam[2] + (rnd() - .5f) * 700, y0 = 0.f, x1 = x0 + 16, z1 = z0 + 16, y1 = 256.f;
            bool vis = false; for (int s = 0; s < 120 && !vis; s++) vis = inside(x0 + rnd() * 16, y0 + rnd() * 256, z0 + rnd() * 16);
            const bool out = frustumOutside(F, x0, y0, z0, x1, y1, z1, 0.f);
            boxes++; if (vis) { boxesVisible++; if (out) falseCull++; } if (out) culled++;
        }
    }
    printf("  caixas %lld | visiveis %lld | FALSOS cortes %lld | cortadas %.0f%%\n", boxes, boxesVisible, falseCull, 100.0 * culled / boxes);
    CHECK(falseCull == 0); CHECK(culled > boxes * 45 / 100);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
