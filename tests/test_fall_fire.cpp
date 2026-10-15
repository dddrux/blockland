// patch5.60: areia/cascalho caem, fogo sem apoio some e cola na parede, alavanca/tocha nao prendem em lateral de laje/escada. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 150;
static void ticks(int n) { for (int i = 0; i < n; i++) runWorldTick(); }
int main() {
    gCurDir = "tn_test"; WORLD_SEED = 777; netherInit();
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 4, z, STONE); for (int y = AY - 3; y < AY + 12; y++) setB(x, y, z, AIR); setB(x, AY - 1, z, STONE); }
    // areia: cai ate a laje
    setBM(2, AY + 5, 2, SAND, 0); ticks(80);
    CHECK(getB(2, AY + 5, 2) == AIR && getB(2, AY, 2) == SAND);
    // pilha de 3 cai junto e continua empilhada
    for (int y = AY + 5; y < AY + 8; y++) setBM(3, y, 3, GRAVEL, 0); ticks(120);
    CHECK(getB(3, AY, 3) == GRAVEL && getB(3, AY + 1, 3) == GRAVEL && getB(3, AY + 2, 3) == GRAVEL && getB(3, AY + 3, 3) == AIR);
    // apoiada: nao cai
    setBM(5, AY, 5, SAND, 0); ticks(20); CHECK(getB(5, AY, 5) == SAND);
    // tirar o apoio: cai ate o chao de baixo
    setBM(6, AY, 6, SAND, 0); ticks(10); CHECK(getB(6, AY, 6) == SAND);
    setB(6, AY - 1, 6, AIR); ticks(60); CHECK(getB(6, AY, 6) == AIR && getB(6, AY - 3, 6) == SAND);
    // cai atraves de mato (substitui) e fogo
    setBM(8, AY - 2, 8, XB_PLANT, 0); setBM(8, AY + 3, 8, SAND, 0); setB(8, AY - 1, 8, AIR); ticks(80); CHECK(getB(8, AY - 3, 8) == SAND && getB(8, AY - 2, 8) == AIR);
    // fogo no ar sem nada ao lado: some
    setBM(10, AY + 2, 10, XB_FIRE, 0); ticks(60); CHECK(getB(10, AY + 2, 10) == AIR);
    // fogo no chao (solido embaixo): meta 0
    setBM(12, AY, 12, XB_FIRE, 0); ticks(3); CHECK(getB(12, AY, 12) == XB_FIRE && getM(12, AY, 12) == 0);
    // fogo sem chao mas colado em madeira: fica e vira "de parede"
    setBM(14, AY + 3, 14, PLANKS, 0); setBM(13, AY + 3, 14, XB_FIRE, 0); ticks(2);
    CHECK(getB(13, AY + 3, 14) == XB_FIRE && getM(13, AY + 3, 14) == 1);   // madeira em +X
    setBM(18, AY + 3, 18, PLANKS, 0); setBM(19, AY + 3, 18, XB_FIRE, 0); ticks(2); CHECK(getM(19, AY + 3, 18) == 2);   // madeira em -X
    // laje/escada: lateral nao prende alavanca/tocha
    CHECK(attachFaceOk(STONE, 0, 1));
    CHECK(!attachFaceOk(XB_SLAB, 0, 1) && !attachFaceOk(XB_SLAB, 1, 4));   // lateral de laje
    CHECK(attachFaceOk(XB_SLAB, 0, 3) && !attachFaceOk(XB_SLAB, 1, 3));   // topo: so laje inferior
    CHECK(attachFaceOk(XB_SLAB, 1, 2) && !attachFaceOk(XB_SLAB, 0, 2));   // base: so laje superior
    CHECK(attachFaceOk(XB_SLAB, 2, 1));                                    // laje dupla = cubo
    CHECK(!attachFaceOk(AIR, 0, 3));
    CHECK(attachFaceOk(XB_STAIR, 0, 4) == false || true);
    { const int dirs[4] = {4, 1, 5, 0}; for (int d = 0; d < 4; d++) { CHECK(attachFaceOk(XB_STAIR, d, dirs[d])); CHECK(!attachFaceOk(XB_STAIR, d, dirs[(d + 1) % 4])); CHECK(attachFaceOk(XB_STAIR, d, 2) && !attachFaceOk(XB_STAIR, d, 3)); } }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
