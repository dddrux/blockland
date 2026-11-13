// patch5.71: limite de explosoes por tick + recuo em TNT acesa vizinha. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120, T = 1300;
static void arena() { for (int z = -12; z < 20; z++) for (int x = -12; x < 20; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 12; y++) setB(x, y, z, AIR); } }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gPkByFile["block/tnt"] = T; tntHooksInit(); CHECK(gTntId == T);
    Phys P; P.x = 90; P.y = AY; P.z = 90; P.mx = P.my = P.mz = 0; gHP = 20; gDead = false; creative = false; gInvul = 0; netOn = false; TNTS.clear();
    arena();
    // 20 TNTs com pavio 1 espalhadas: no 1o tick so TNT_MAX_BOOM explodem, o resto espera
    for (int i = 0; i < 20; i++) tntPrime(-10 + (i % 5) * 6 + .5, AY + 8, -10 + (i / 5) * 6 + .5, 1);
    const size_t n0 = TNTS.size(); CHECK(n0 == 20);
    tntTick(P); CHECK((int)TNTS.size() >= 20 - TNT_MAX_BOOM); CHECK((int)TNTS.size() <= 20 - 1);   // algumas explodiram, nenhuma sumiu sem explodir (cadeia pode acrescentar)
    int ticks = 0; while (!TNTS.empty() && ticks < 50) { tntTick(P); ticks++; }
    CHECK(TNTS.empty() && ticks >= 2);   // as 20 explodem ao longo de varios ticks
    // recuo: TNT acesa perto de outra explosao ganha velocidade para longe
    TNTS.clear(); arena(); tntPrime(8.5, AY + 1, 8.5, 80); TNTS[0].vx = TNTS[0].vy = TNTS[0].vz = 0;
    explosionAt(6.5, AY + 1, 8.5, 4.f, P, "teste", nullptr);
    CHECK(!TNTS.empty() && TNTS[0].vx > .2);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
