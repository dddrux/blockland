// patchOcelot.1: jaguatirica (M_OCELOT): tabela, spawn e fuga do jogador (< 7 blocos). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345; creative = false; gDead = false;
    CHECK(M_OCELOT < M_N && std::string(MD[M_OCELOT].nome) == "jaguatirica" && MD[M_OCELOT].hp == 10);
    CHECK(mobSndFor(M_OCELOT) != nullptr && mobSndFor(M_N - 1) != nullptr);
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    int gy = CH - 2; while (gy > 1 && !solid(getB(4, gy, 4))) gy--;
    MOBS.clear(); spawnMob(M_OCELOT, 4.5, gy + 1.0, 4.5, 0.f); MobEnt& e = MOBS[0]; CHECK(e.hp == 10);
    const double d0 = 3.0;   // jogador a 3 blocos a oeste (x = 1.5)
    for (int t = 0; t < 60; t++) tickMob(e, 4.5 - d0, gy + 1.0, 4.5);
    CHECK(e.x - 4.5 > 1.0);   // fugiu para leste (oposto ao jogador)
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
