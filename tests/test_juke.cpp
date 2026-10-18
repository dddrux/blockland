// patchJuke.1: toca-discos (base: patch5.64: TNT (acender com fogo/isqueiro, pavio de 80 ticks, explosao forca 4, cadeia, dano) + explosao do creeper preservada. Roda o main.cpp REAL.
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
    PkItem a, b; a.file = "block/jukebox_side"; b.file = "item/music_disc_cat"; const int base = (int)PK.size(); PK.push_back(a); PK.push_back(b);
    gPkByFile["block/jukebox_side"] = PK0 + base; gPkByFile["item/music_disc_cat"] = PK0 + base + 1; jukeInit();
    const int J = PK0 + base, D = PK0 + base + 1; CHECK(gJukeId == J && jukeDiscOf(D) == 2 && jukeItemOf(2) == D && jukeDiscOf(J) == 0);
    arena(); setB(5, AY, 5, J); Slot s; s.id = D; s.n = 2; gSndOn = false;
    CHECK(jukeUse(5, AY, 5, s, false) == 2 && (getM(5, AY, 5) & 15) == 2 && s.n == 1);   // insere, consome 1
    size_t d0 = DROPS.size(); CHECK(jukeUse(5, AY, 5, s, false) == 1 && (getM(5, AY, 5) & 15) == 0 && DROPS.size() == d0 + 1);   // ejeta: disco cai
    Slot vz; CHECK(jukeUse(5, AY, 5, vz, false) == 0);   // mao vazia: nada
    setB(6, AY, 5, STONE); CHECK(jukeUse(6, AY, 5, s, false) == 0);   // outro bloco
    Slot c; c.id = D; c.n = 1; CHECK(jukeUse(5, AY, 5, c, true) == 2 && c.n == 1);   // criativo: nao consome
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
