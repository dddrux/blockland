// patchTnt.1: TNT acesa arrastada pela agua/lava; bloco de TNT nao. (base: patch5.64: TNT (acender com fogo/isqueiro, pavio de 80 ticks, explosao forca 4, cadeia, dano) + explosao do creeper preservada. Roda o main.cpp REAL.
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
    gPkByFile["block/tnt"] = T; tntHooksInit();
    Phys P; P.x = 40; P.y = AY; P.z = 40; P.mx = P.my = P.mz = 0; gHP = 20; gDead = false; creative = false; gInvul = 0; netOn = false; TNTS.clear();
    // canal de agua: fonte em x=2, agua escorrendo (meta crescente) em +x, 1 bloco de fundo
    arena(); for (int x = 2; x <= 10; x++) { setB(x, AY, 5, metaBlock(x == 2 ? 0 : std::min(7, x - 2))); }
    TNTS.clear(); tntPrime(4.5, AY + .1, 5.5, 200);
    for (int i = 0; i < 30; i++) tntTick(P);
    CHECK(TNTS.size() == 1 && TNTS[0].x > 4.6);   // andou para +x (corrente)
    // sem agua: continua parada
    arena(); TNTS.clear(); tntPrime(4.5, AY, 5.5, 200); for (int i = 0; i < 30; i++) tntTick(P);
    CHECK(TNTS.size() == 1 && fabs(TNTS[0].x - 4.5) < .3);
    // bloco de TNT na agua nao se mexe (e nao vira entidade)
    arena(); setB(6, AY, 5, metaBlock(1)); setB(6, AY + 1, 5, T); for (int i = 0; i < 30; i++) tntTick(P); CHECK(getB(6, AY + 1, 5) == T);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
