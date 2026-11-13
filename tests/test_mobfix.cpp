// test_mobfix: papagaio/ghast voam, lula/guardiao nao andam em terra e nadam na agua (patchMobFix.1). Rodar: tools/t.sh mobfix
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345; const int AY = 150;
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z < 30; z++) for (int x = -30; x < 30; x++) for (int y = AY - 8; y < AY + 12; y++) setB(x, y, z, y < AY ? STONE : AIR);
    for (int z = 10; z < 22; z++) for (int x = 10; x < 22; x++) for (int y = AY - 5; y < AY; y++) setB(x, y, z, WATER);
    MOBS.clear(); spawnMob(M_PARROT, 0.5, AY + 1.0, 0.5, 0.f); { MobEnt& e = MOBS[0]; double mx = e.y; for (int k = 0; k < 1500; k++) { tickMob(e, 0.0, AY, 25.0); mx = std::max(mx, e.y); } CHECK(mx > AY + 1.8); CHECK(e.hp == MD[M_PARROT].hp); }   // voa e nao morre de queda
    MOBS.clear(); spawnMob(M_SQUID, 0.5, AY + 1.0, 0.5, 0.f); { MobEnt& e = MOBS[0]; for (int k = 0; k < 600; k++) tickMob(e, 0.0, AY, 25.0); CHECK(fabs(e.x - .5) < .5 && fabs(e.z - .5) < .5); }   // em terra fica parada
    for (int t : {M_SQUID, M_GUARD}) { MOBS.clear(); spawnMob(t, 16.5, AY - 2.5, 16.5, 0.f); MobEnt& e = MOBS[0]; int inW = 0; for (int k = 0; k < 1500; k++) { tickMob(e, 0.0, AY, 40.0); inW += e.inWater; } CHECK(inW > 1400); CHECK(e.y < AY); }
    printf("test_mobfix: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
