// patchFix.13 (BUG-043): pulso curto passa pelo repetidor (liga e depois desliga), como no 1.12.2. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static bool on(int x) { return (getM(x, AY, 5) >> 4) & 1; }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = 0; z < 12; z++) for (int x = 0; x < 12; x++) for (int y = AY - 1; y < AY + 4; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    setB(4, AY, 5, XB_REPT); setM(4, AY, 5, 1 | 16);   // S: leste, ligado (entrada de R)
    setB(5, AY, 5, XB_REPT); setM(5, AY, 5, 1);        // R: leste, atraso 1 (2 ticks), desligado
    gNoBlockHooks = false;
    rsRepCheck(5, AY, 5); CHECK(tickPending(5, AY, 5)); CHECK(!on(5));
    gNoBlockHooks = true; setM(4, AY, 5, 1); gNoBlockHooks = false;   // entrada cai antes do atraso
    rsRepCheck(5, AY, 5);
    for (int i = 0; i < 2; i++) runWorldTick();
    CHECK(on(5));   // pulso curto ainda liga
    for (int i = 0; i < 4; i++) runWorldTick();
    CHECK(!on(5));  // e desliga depois do atraso
    // sem pulso: repetidor desligado com entrada desligada nao liga sozinho (ex.: recarga de chunk)
    gTicks.clear(); gTickSet.clear(); scheduleTick(5, AY, 5, 1); runWorldTick(); runWorldTick(); CHECK(!on(5));
    printf("%d checagens, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
