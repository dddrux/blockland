// patch5.68: grama alta (quebrar em cima quebra a de baixo) + cor de bioma na vinha/vitoria-regia. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
}
int main() {
    arena(); applyMcPack(); loadPackItems(); xbInit(); const bool havePack = gPack.ok;
    const int LO = XB_PLANT + 14, UP = XB_PLANT + 15;
    SECTION("colocar baixo e depois cima nao apaga nada");
    setB(2, AY, 2, LO); setB(2, AY + 1, 2, UP);
    CHECK(getB(2, AY, 2) == LO); CHECK(getB(2, AY + 1, 2) == UP);
    SECTION("quebrar a de cima remove a de baixo");
    setB(2, AY + 1, 2, AIR);
    CHECK(getB(2, AY, 2) == AIR);
    SECTION("quebrar a de baixo remove a de cima (ja existia)");
    setB(4, AY, 2, LO); setB(4, AY + 1, 2, UP); setB(4, AY, 2, AIR);
    CHECK(getB(4, AY + 1, 2) == AIR);
    SECTION("planta alta sobre outra: girassol");
    setB(6, AY, 2, XB_PLANT + 18); setB(6, AY + 1, 2, XB_PLANT + 19); setB(6, AY + 1, 2, AIR);
    CHECK(getB(6, AY, 2) == AIR);
    SECTION("cor de bioma na vinha e fixa na vitoria-regia");
    if (!havePack || !gPack.ok) printf("AVISO: pack ausente, checagens de malha puladas\n"); else {
    { const std::vector<XQuad>& v = rsQuads(XB_VINE, 1); CHECK(!v.empty()); bool ok = !v.empty(); for (const XQuad& q : v) if (!(q.tint && q.foli)) ok = false; CHECK(ok); }
    { const std::vector<XQuad>& l = rsQuads(XB_LILY, 0); CHECK(!l.empty()); bool ok = !l.empty(); for (const XQuad& q : l) if (!(q.tc[0] < .2f && q.tc[1] > .4f)) ok = false; CHECK(ok); }
    }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
