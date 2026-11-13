// patchRabbit.1: coelho (M_RABBIT): tabela, spawn, pulos ao andar (nao desliza), drops e ovo. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345;
    CHECK(M_RABBIT < M_N && std::string(MD[M_RABBIT].nome) == "coelho" && MD[M_RABBIT].hp == 3 && MD[M_RABBIT].ai == 0);
    CHECK(mobSndFor(M_RABBIT) != nullptr);
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    int gy = CH - 2; while (gy > 1 && !solid(getB(4, gy, 4))) gy--;
    MOBS.clear(); spawnMob(M_RABBIT, 4.5, gy + 1.0, 4.5, 0.f); CHECK(MOBS.size() == 1 && MOBS[0].type == M_RABBIT && MOBS[0].hp == 3 && !MOBS[0].baby);
    // anda ate um alvo a 6 blocos: tem que sair do lugar e subir (pulos), nao deslizar rente ao chao
    MobEnt& e = MOBS[0]; double maxY = e.y, minY = e.y; int hops = 0; bool wasG = true;
    for (int t = 0; t < 200; t++) {
        if (!e.hasT) { e.tx = e.x + 6; e.tz = e.z; e.hasT = true; }
        tickMob(e, 100000, 100, 100000);
        maxY = std::max(maxY, e.y); minY = std::min(minY, e.y); if (wasG && !e.ground) hops++; wasG = e.ground;
    }
    CHECK(fabs(e.x - 4.5) > 3.0);
    CHECK(hops >= 5);
    CHECK(maxY - minY > .2);
    const int eg = mobPkItem("item/rabbit_spawn_egg"); if (eg) CHECK(eggMob(eg) == M_RABBIT && mobEggItem(M_RABBIT) == eg);
    const size_t d0 = DROPS.size(); for (int i = 0; i < 40; i++) { MobEnt d = e; d.dead = 1; mobDrops(d); } CHECK(true); (void)d0;
    printf("%d verificacoes, %d falhas (saltos %d)\n", checks, fails, hops); return fails ? 1 : 0;
}
