// patchWolf.1: lobo selvagem (M_WOLF): tabela, neutro ate ser irritado, depois corre e morde (dano 2); matilha so alerta lobos. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345; creative = false; gDead = false; gGm = 0;
    CHECK(M_WOLF < M_N && std::string(MD[M_WOLF].nome) == "lobo" && MD[M_WOLF].hp == 8 && MD[M_WOLF].ai == 3);
    CHECK(mobSndFor(M_WOLF) != nullptr && mobSndFor(M_N - 1) != nullptr);
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    int gy = CH - 2; while (gy > 1 && !solid(getB(4, gy, 4))) gy--;
    MOBS.clear(); spawnMob(M_WOLF, 4.5, gy + 1.0, 4.5, 0.f); spawnMob(M_PIGMAN, 6.5, gy + 1.0, 4.5, 0.f); spawnMob(M_WOLF, 8.5, gy + 1.0, 4.5, 0.f);
    CHECK(MOBS.size() == 3 && MOBS[0].panic == 0);
    // irrita o 1o lobo: o outro lobo (mesma matilha) fica irritado, o porco-zumbi nao
    MOBS[0].panic = 600; for (MobEnt& o : MOBS) if (&o != &MOBS[0] && !o.dead && o.type == MOBS[0].type) o.panic = std::max(o.panic, 600);
    CHECK(MOBS[2].panic == 600 && MOBS[1].panic == 0);
    const int hp0 = gHP; MobEnt& e = MOBS[0];
    for (int t = 0; t < 120; t++) tickMob(e, 4.5, gy + 1.0, 7.5);   // jogador a 3 blocos ao sul
    CHECK(e.z - 4.5 > 0.5);   // foi na direcao do jogador
    for (int t = 0; t < 100; t++) tickMob(e, e.x, e.y, e.z + 1.0);
    CHECK(gHP <= hp0 - 2);   // mordeu (dano 2)
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
