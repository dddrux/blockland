// patchFix.25: End = luz ambiente neutra para entidades (jogador/mobs/itens/mao)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    float o[3];
    gDim = 2; gSkyScale = .1f; entLightAt(0.5, 70, 0.5, o);
    CHECK(o[0] >= .92f && o[1] >= .92f && o[2] >= .92f && gEntSk == 15.f);
    gDim = 0; gSkyScale = 1.f; entLightAt(0.5, 70, 0.5, o);
    CHECK(o[0] <= 1.f && o[2] <= 1.f);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
