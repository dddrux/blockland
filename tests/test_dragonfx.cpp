// patchFix.26: fonte do End (portal so ativo apos matar o dragao) + morte do dragao
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int y = 0; y < CH; y++) for (int z = -6; z <= 6; z++) for (int x = -6; x <= 6; x++) setB(x, y, z, y < 60 ? STONE : AIR);
    bool built = true; int t0 = dragonTop(built); CHECK(!built && t0 == 59);
    CHECK(dragonEggSpawn(true, false));
    built = false; int t1 = dragonTop(built); CHECK(built && t1 == 59);
    CHECK(getB(0, 60, 0) == BEDROCK && getB(0, 62, 0) == BEDROCK && getB(3, 60, 0) != XB_PORTAL && getB(1, 60, 0) == AIR);   // moldura sem portal
    CHECK(dragonEggSpawn(true, false) && dragonTop(built) == 59);   // idempotente
    CHECK(dragonEggSpawn(true, true));
    CHECK(getB(1, 60, 0) == XB_PORTAL && (getM(1, 60, 0) & 2) && getB(0, 60, 1) == XB_PORTAL);   // ativo
    built = false; CHECK(dragonTop(built) == 59 && built);   // ovo no topo nao desloca a base
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
