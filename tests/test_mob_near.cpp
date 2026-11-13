// patch5.64: despawn/spawn consideram todos os jogadores (local, P2, remotos nao espectadores). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    Phys P; P.x = 0; P.y = 100; P.z = 0; remotes.clear();
    CHECK(fabs(sqrt(playerNear2(300, 0, P)) - 300) < 1e-6);   // so o local
    { RemotePl r; r.init = true; r.x = 290; r.y = 100; r.z = 0; remotes[3] = r; }
    CHECK(fabs(sqrt(playerNear2(300, 0, P)) - 10) < 1e-6);    // convidado perto do mob: nao some
    remotes[3].gm = 3; CHECK(fabs(sqrt(playerNear2(300, 0, P)) - 300) < 1e-6);   // espectador nao conta
    remotes[3].gm = 0; remotes[3].init = false; CHECK(fabs(sqrt(playerNear2(300, 0, P)) - 300) < 1e-6);   // sem init nao conta
    remotes[3].init = true; remotes[3].deathT = 5; CHECK(fabs(sqrt(playerNear2(300, 0, P)) - 300) < 1e-6);   // morto nao conta
    remotes[3].deathT = -1;
    std::vector<std::array<double, 3>> v; activePlayers(P, v); CHECK(v.size() == 2 && v[1][0] == 290);
    // despawn de animal: longe do host mas perto do convidado => fica; sem convidado => vai embora
    for (int cz = -9; cz <= 9; cz += 3) for (int cx = 17; cx <= 20; cx++) if (!getC(cx, cz)) gen(cx, cz);
    MOBS.clear(); MobEnt a; a.type = M_PIG; a.x = 300; a.y = 100; a.z = 0; a.dead = false; MOBS.push_back(a);
    animalSpawn(P); CHECK(MOBS[0].y > 0);   // convidado a 10 blocos: fica
    remotes.clear(); animalSpawn(P); CHECK(MOBS[0].y < 0);   // sem ninguem perto (300 do host): despawn
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
