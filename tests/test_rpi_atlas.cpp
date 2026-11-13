// patchRpi.3: atlas cabe no limite do VC4 (<=2048, potencia de 2) e todos os tiles do pack/modelos ficam dentro dele
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static bool pot(int v) { return v > 0 && (v & (v - 1)) == 0; }
int main() {
    printf("atlas %dx%d (ATL=%d ATLR=%d)\n", ATW, 16 * ATLR, ATL, ATLR);
#ifdef BL_RPI
    CHECK(ATW <= 2048 && 16 * ATLR <= 2048 && pot(ATW) && pot(16 * ATLR));
#endif
    CHECK(ATL * ATLR0 >= 4608);                       // mesma capacidade de tiles do atlas antigo (32 x 144)
    CHECK(PKTILE0 < ATL * ATLR0 && SPR0 + 32 <= ATL * ATLR);   // sprites de cama/estandarte dentro do atlas
    gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip"); applyMcPack();
    int bad = 0; for (int b = 1; b < 256; b++) for (int f = 0; f < 3; f++) if (TILES[b][f] < 0 || TILES[b][f] >= ATL * ATLR) bad++;
    CHECK(bad == 0);
    for (const auto& e : PK) if (e.tile >= ATL * ATLR) bad++;
    CHECK(bad == 0);
    std::map<std::string, std::string> none; std::string sd, tp, bt; rsMat(7, sd, tp, bt);
    CHECK(xbTexTile(sd, none) == TILES[SANDSTONE][1] && xbTexTile(tp, none) < ATL * ATLR);
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
