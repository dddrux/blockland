// patch5.68: contorno do fogo (placa fina) + multiplicador de cor da agua por bioma. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    gCurDir = "tn_test"; WORLD_SEED = 777; netherInit();
    AABB t[SH_MAXB];
    SECTION("fogo no chao: placa de 1/16 embaixo");
    { int n = blockBoxesId(XB_FIRE, 0, 100, 0, true, t, 0); CHECK(n == 1); if (n == 1) { CHECK(fabs(t[0].y1 - t[0].y0 - .0625) < 1e-9); CHECK(fabs(t[0].x1 - t[0].x0 - 1) < 1e-9); } }
    SECTION("fogo de parede: uma placa por lado, 1/16 de espessura");
    { int n = blockBoxesId(XB_FIRE, 0, 100, 0, true, t, 1); CHECK(n == 1); if (n == 1) { CHECK(fabs(t[0].x0 - (1 - .0625)) < 1e-9); CHECK(fabs(t[0].y1 - t[0].y0 - 1) < 1e-9); }
      n = blockBoxesId(XB_FIRE, 0, 100, 0, true, t, 1 | 8); CHECK(n == 2); }
    SECTION("fogo continua sem colisao");
    CHECK(blockBoxesId(XB_FIRE, 0, 100, 0, false, t, 0) == 0);
    SECTION("agua por bioma");
    { float w[3]; biomeWaterTint(100.f, 100.f, w); for (int k = 0; k < 3; k++) CHECK(w[k] > .5f && w[k] <= 1.f); }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
