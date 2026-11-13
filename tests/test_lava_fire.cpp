// patch5.61: lava incendeia inflamaveis ao lado; item cai no fogo e queima. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
static int fireCount() { int n = 0; for (int y = AY - 2; y < AY + 6; y++) for (int z = -6; z < 14; z++) for (int x = -6; x < 14; x++) if (getB(x, y, z) == XB_FIRE) n++; return n; }
static void clearArena() { for (int z = -8; z < 16; z++) for (int x = -8; x < 16; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 6; y++) setB(x, y, z, AIR); } }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    clearArena(); setB(4, AY, 4, 11);   // lava (id 11 = LAVA fonte no projeto? ver abaixo)
    // descobre um id de lava valido
    int lava = 0; for (int b = 1; b < 120; b++) if (isLava(b)) { lava = b; break; }
    CHECK(lava != 0);
    clearArena(); setB(4, AY, 4, lava);
    for (int i = 0; i < 1500; i++) lavaIgniteTick();
    CHECK(fireCount() == 0);   // sem inflamavel: nada pega fogo
    setB(6, AY + 1, 4, PLANKS);   // tabua perto (a 2 blocos): o ar entre elas encosta nela
    for (int i = 0; i < 4000 && fireCount() == 0; i++) lavaIgniteTick();
    CHECK(fireCount() >= 1);   // lava acendeu o ar ao lado da madeira
    // item caindo no fogo queima
    clearArena(); setB(9, AY, 9, XB_FIRE); DROPS.clear();
    spawnDrop(9.5, AY + .2, 9.5, 0, 0, 0, STONE, 1, 0);
    CHECK(DROPS.size() == 1); tickDrops(0, 0, 0); CHECK(DROPS.empty());
    clearArena(); DROPS.clear(); spawnDrop(9.5, AY + .2, 9.5, 0, 0, 0, STONE, 1, 0); tickDrops(0, 0, 0); CHECK(DROPS.size() == 1);   // sem fogo: fica
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
