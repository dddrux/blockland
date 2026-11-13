// Nametag: regra de visibilidade (agachado/morto/espectador/distancia). A parte GL nao roda headless.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    RemotePl r; r.init = true; gGm = 0;
    CHECK(netTagVisible(r, 3, 0, 4));                                   // perto, em pe: aparece
    CHECK(netTagVisible(r, 64, 0, 0) && !netTagVisible(r, 64.1, 0, 0)); // limite de 64 blocos
    CHECK(!netTagVisible(r, 40, 40, 40));                               // distancia 3D
    r.sneak = true; CHECK(!netTagVisible(r, 1, 0, 1)); r.sneak = false; CHECK(netTagVisible(r, 1, 0, 1));   // some agachado, volta ao levantar
    r.deathT = 0; CHECK(!netTagVisible(r, 1, 0, 1)); r.deathT = -1;
    r.gm = 3; CHECK(!netTagVisible(r, 1, 0, 1)); gGm = 3; CHECK(netTagVisible(r, 1, 0, 1)); gGm = 0; r.gm = 0;   // espectador so aparece para espectador
    r.init = false; CHECK(!netTagVisible(r, 1, 0, 1));
    gNetTagOn = false; netDrawTags(0.f);                                // desligado: nao toca em GL
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
