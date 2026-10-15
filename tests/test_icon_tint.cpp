// patch5.75: cor de inventario da vinha/vitoria-regia por mapeamento XB (nao depende do nome do arquivo). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    float c[3] = {1, 1, 1};
    CHECK(!iconTintRGB(5000, c));
    gXbItem[5000] = XB_VINE; CHECK(iconTintRGB(5000, c) && c[1] > c[0] && c[2] < .2f);
    gXbItem[5001] = XB_LILY; CHECK(iconTintRGB(5001, c) && c[1] > c[2]);
    gXbItem[5002] = XB_PIST; CHECK(!iconTintRGB(5002, c));
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
