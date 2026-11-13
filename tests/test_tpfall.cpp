// patchPend.10: /tp zera a distancia de queda (CommandTP 1.12.2). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    Phys P; P.x = P.px = 0; P.y = P.py = 200; P.z = P.pz = 0; P.my = -3.0;
    gFall = 40.f;
    tpSelf(P, 10.0, 70.0, 10.0);
    CHECK(gFall == 0.f); CHECK(P.x == 10.0 && P.y == 70.0 && P.z == 10.0); CHECK(P.my == 0.0 && P.mx == 0.0 && P.mz == 0.0); CHECK(gTpHold == 1);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
