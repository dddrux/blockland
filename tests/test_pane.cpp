// Painel de vidro: contorno = 1 caixa (uniao), colisao = poste + bracos; face de cima some com painel em cima.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    AABB o[8]; const int m = (1 << 4) | 10;   // vidro, bracos leste+oeste (mascara 2|8)
    int n = rsShape(0, 0, 0, XB_PANE, m, true, o); CHECK(n == 1); CHECK(o[0].x0 == 0 && o[0].x1 == 1 && o[0].z0 > .43 && o[0].z1 < .57 && o[0].y1 == 1);
    n = rsShape(0, 0, 0, XB_PANE, m, false, o); CHECK(n == 3);   // colisao continua com poste + 2 bracos
    n = rsShape(0, 0, 0, XB_PANE, (1 << 4) | 3, true, o); CHECK(n == 1 && o[0].x1 == 1 && o[0].z0 == 0 && o[0].x0 > .43);   // L: uniao dos bracos (norte+leste)
    n = rsShape(0, 0, 0, XB_FENCE, 5 | (0 << 4), true, o); CHECK(n == 1);
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
