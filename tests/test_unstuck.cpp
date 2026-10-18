// patchTrap.2: alcapao/porta em cima do jogador nao empurra (base: patch5.64: TNT (acender com fogo/isqueiro, pavio de 80 ticks, explosao forca 4, cadeia, dano) + explosao do creeper preservada. Roda o main.cpp REAL.
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
static void block(int r, int cx, int cy, int cz) { for (int y = cy - r; y <= cy + r; y++) for (int z = cz - r; z <= cz + r; z++) for (int x = cx - r; x <= cx + r; x++) if (y >= AY) setB(x, y, z, STONE); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Phys P; P.noclip = false; P.sneak = false; P.flying = false;
    arena();
    // alcapao (meia altura, em Y de AY) em cima do jogador: nao empurra
    setBM(5, AY, 5, XB_TRAP, 1 << 4); P.x = 5.5; P.y = AY; P.z = 5.5; const double x0 = P.x, y0 = P.y, z0 = P.z;
    CHECK(!unstuckPlayer(P) && P.x == x0 && P.y == y0 && P.z == z0);
    // aberto (parede fina): idem; porta fechada e aberta: idem
    setBM(5, AY, 5, XB_TRAP, (1 << 4) | 8); CHECK(!unstuckPlayer(P) && P.y == y0 && P.x == x0);
    setBM(5, AY, 5, XB_DOOR, 0); setBM(5, AY + 1, 5, XB_DOOR, 0); CHECK(!unstuckPlayer(P) && P.x == x0 && P.y == y0);
    setBM(5, AY, 5, XB_DOOR, 4); CHECK(!unstuckPlayer(P) && P.x == x0);
    // bloco cheio (pedra) ainda empurra
    setB(5, AY, 5, STONE); setB(5, AY + 1, 5, STONE); CHECK(unstuckPlayer(P) && (P.x != x0 || P.y != y0 || P.z != z0));
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
