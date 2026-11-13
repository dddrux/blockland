// patch5.78: afogamento de mobs. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -4; z < 4; z++) for (int x = -4; x < 4; x++) { setB(x, AY - 2, z, STONE); for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, AIR); }
    MOBS.clear(); MobEnt a; a.type = M_PIG; a.x = .5; a.y = AY - 1; a.z = .5; a.hp = MD[M_PIG].hp > 0 ? MD[M_PIG].hp : 10; a.dead = 0; MOBS.push_back(a);
    MobEnt& m = MOBS[0]; const int hp0 = m.hp;
    for (int y = AY - 1; y <= AY + 2; y++) setB(0, y, 0, WATER);   // cabeca na agua
    for (int t = 0; t < 299; t++) mobAirTick(m);
    CHECK(m.air == 1 && m.hp == hp0);
    for (int t = 0; t < 21; t++) mobAirTick(m);
    CHECK(m.hp == hp0 - 2 && m.air == 0);
    m.resist = 0; for (int t = 0; t < 20; t++) mobAirTick(m);
    CHECK(m.hp == hp0 - 4);
    // fora da agua o ar volta
    for (int y = AY - 1; y <= AY + 2; y++) setB(0, y, 0, AIR);
    mobAirTick(m); CHECK(m.air == 300);
    // so os pes na agua (agua rasa): nao se afoga
    m.type = M_ZOMBIE; setB(0, AY - 1, 0, WATER); m.air = 50; mobAirTick(m); CHECK(m.air == 300);
    // mob morto nao leva dano
    m.dead = 5; const int hp1 = m.hp; for (int y = AY - 1; y <= AY + 2; y++) setB(0, y, 0, WATER); m.air = -19; mobAirTick(m); CHECK(m.hp == hp1);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
