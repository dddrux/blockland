// patchMod.2: alcapao de ferro: nao abre com clique, abre/fecha por redstone; madeira abre com clique e com redstone. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -4; z < 12; z++) for (int x = -4; x < 12; x++) for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    const int LV = XB_LEVER + 12 + 4, LV0 = XB_LEVER + 4;
    printf("[clique]\n");
    setBM(2, AY, 2, XB_TRAP, 0 << 4); rsUse(2, AY, 2); CHECK(((getM(2, AY, 2) >> 3) & 1) == 0);   // ferro: clique nao abre
    setBM(4, AY, 2, XB_TRAP, 1 << 4); rsUse(4, AY, 2); CHECK(((getM(4, AY, 2) >> 3) & 1) == 1);   // madeira: abre
    printf("[redstone]\n");
    setBM(2, AY, 2, XB_TRAP, 0 << 4 | 1); setB(2, AY, 3, LV);   // alavanca ligada ao lado
    rsTrapCheck(2, AY, 2); CHECK(((getM(2, AY, 2) >> 3) & 1) == 1);   // abriu
    CHECK((getM(2, AY, 2) & 3) == 1 && ((getM(2, AY, 2) >> 4) & 7) == 0);   // orientacao e tipo preservados
    setB(2, AY, 3, LV0); rsTrapCheck(2, AY, 2); CHECK(((getM(2, AY, 2) >> 3) & 1) == 0);   // fechou
    printf("[portao/porta/colocar]\n");   // patchMod.9
    setBM(6, AY, 2, XB_GATE, 1 << 3); setB(6, AY, 3, LV); rsOpenCheck(6, AY, 2); CHECK(((getM(6, AY, 2) >> 2) & 1) == 1); CHECK(((getM(6, AY, 2) >> 3) & 7) == 1);   // portao abre, tipo preservado
    setB(6, AY, 3, LV0); rsOpenCheck(6, AY, 2); CHECK(((getM(6, AY, 2) >> 2) & 1) == 0);
    setBM(8, AY, 2, XB_DOOR, 0 << 5); setBM(8, AY + 1, 2, XB_DOOR, 8 | (0 << 5)); setB(8, AY, 3, LV);   // porta de ferro, alavanca na metade de baixo
    rsOpenCheck(8, AY, 2); CHECK(((getM(8, AY, 2) >> 2) & 1) == 1 && ((getM(8, AY + 1, 2) >> 2) & 1) == 1);   // as duas metades abriram
    setB(8, AY, 3, LV0); rsOpenCheck(8, AY + 1, 2); CHECK(((getM(8, AY, 2) >> 2) & 1) == 0 && ((getM(8, AY + 1, 2) >> 2) & 1) == 0);   // fecha partindo da metade de cima
    setB(0, AY, 6, LV); setBM(0, AY, 5, XB_TRAP, 0 << 4 | 2); rsAddedXb(0, AY, 5); CHECK(((getM(0, AY, 5) >> 3) & 1) == 1);   // alcapao colocado ja energizado abre
    printf("[bloco solido energizado]\n");   // patchProp.11: alavanca ligada presa a um bloco; o alcapao colado nesse bloco abre, e fecha ao desligar
    setB(6, AY, 3, STONE); setBM(6, AY, 2, XB_TRAP, 0 << 4); setB(6, AY, 4, XB_LEVER + 12 + 2);   // alavanca (ligada, presa em (6,AY,3)) do lado oposto ao alcapao
    rsTrapCheck(6, AY, 2); CHECK(((getM(6, AY, 2) >> 3) & 1) == 1);
    setB(6, AY, 4, AIR); rsTrapCheck(6, AY, 2); CHECK(((getM(6, AY, 2) >> 3) & 1) == 0);
    setB(6, AY, 4, XB_LEVER + 2); rsTrapCheck(6, AY, 2); CHECK(((getM(6, AY, 2) >> 3) & 1) == 0);   // alavanca desligada: continua fechado
    setB(6, AY, 4, XB_LEVER + 12 + 2); xbNeighbor(6, AY, 3, 6, AY, 4); CHECK(((getM(6, AY, 2) >> 3) & 1) == 1);   // aviso de vizinho no bloco solido reavalia o alcapao
    printf("test_irontrap: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
