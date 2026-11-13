// patchRep.1: prioridade do tick do repetidor (1.12.2): -3 saida para outro diodo, -2 ligado, -1 normal. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void put(int x, int id, int m) { gNoBlockHooks = true; setBM(x, AY, 5, id, m); gNoBlockHooks = false; }
static int prioOf(int x) { for (const SchedTick& t : gTicks) if (t.x == x && t.y == AY && t.z == 5) return t.prio; return 99; }
static void reset() { gTicks.clear(); gTickSet.clear(); gTickSeq = 0; for (int x = 2; x <= 8; x++) put(x, AIR, 0); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    // entrada ligada (repetidor ligado atras, mesmo sentido L), saida livre: -1
    reset(); put(4, XB_REPT, 1 | (1 << 4)); put(5, XB_REPT, 1); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -1);
    // saida aponta para repetidor virado de volta (sentido O): -3
    reset(); put(4, XB_REPT, 1 | (1 << 4)); put(5, XB_REPT, 1); put(6, XB_REPT, 3); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -3);
    // saida para repetidor de lado (sentido S): tambem -3 (nao e o mesmo sentido)
    reset(); put(4, XB_REPT, 1 | (1 << 4)); put(5, XB_REPT, 1); put(6, XB_REPT, 2); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -3);
    // saida para repetidor no mesmo sentido (corrente normal): -1
    reset(); put(4, XB_REPT, 1 | (1 << 4)); put(5, XB_REPT, 1); put(6, XB_REPT, 1); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -1);
    // ligado e perdendo a entrada, saida livre: -2
    reset(); put(5, XB_REPT, 1 | (1 << 4)); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -2);
    // ligado, perdendo a entrada e saida para outro diodo: -3 vence
    reset(); put(5, XB_REPT, 1 | (1 << 4)); put(6, XB_REPT, 3); rsRepCheck(5, AY, 5); CHECK(prioOf(5) == -3);
    // sem mudanca de entrada: nada agendado
    reset(); put(5, XB_REPT, 1); rsRepCheck(5, AY, 5); CHECK(gTicks.empty());
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
