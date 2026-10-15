// patch5.74: lote de edicoes de bloco na rede (explosao) + pick de plantacao. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -12; z < 20; z++) for (int x = -12; x < 20; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 12; y++) setB(x, y, z, AIR); }
    // lote: 200 edicoes -> 3 pacotes (80+80+40), todas registradas em netEdits; fora do lote nada acumula
    netOn = true; netEdits.clear(); gNetBatchQ.clear();
    netBatchBegin(); for (int i = 0; i < 200; i++) netSendBlock(i % 10, AY, i / 10, STONE, 0);
    CHECK(gNetBatchQ.size() == 200 && netEdits.size() == 200);
    const int np = netBatchEnd(); CHECK(np == 3); CHECK(gNetBatchQ.empty() && !gNetBatch);
    netSendBlock(0, AY + 5, 0, STONE, 0); CHECK(gNetBatchQ.empty());
    // explosao: usa lote (fila vazia depois) e registra as edicoes
    netEdits.clear(); for (int z = -4; z < 4; z++) for (int x = -4; x < 4; x++) for (int y = AY; y < AY + 4; y++) setB(x, y, z, DIRT);
    Phys P; P.x = 90; P.y = AY; P.z = 90; P.mx = P.my = P.mz = 0; gHP = 20; gDead = false; creative = false; gInvul = 0;
    explosionAt(0.5, AY + 2, 0.5, 4.f, P, "teste", nullptr);
    CHECK(gNetBatchQ.empty() && !gNetBatch); CHECK(netEdits.size() > 20); CHECK(getB(0, AY + 2, 0) == AIR);
    netOn = false;
    // pick de plantacao: tipo 1 (cenoura) em qualquer idade
    setBM(5, AY, 5, XB_CROP, (1 << 3) | 0); CHECK(pickItemAt(5, AY, 5) == IT_CARROT);
    setBM(5, AY, 5, XB_CROP, (1 << 3) | 3); CHECK(pickItemAt(5, AY, 5) == IT_CARROT);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
