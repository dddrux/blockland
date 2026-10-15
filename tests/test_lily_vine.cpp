// patch5.63: vitoria-regia (so sobre fonte de agua, cai sem agua) e vinha (lados, junta, perde suporte, escala). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120;
static void clearArena() { for (int z = -8; z < 16; z++) for (int x = -8; x < 16; x++) { setB(x, AY - 2, z, STONE); for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, AIR); } }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    lilyVineHooksInit();
    // ---- formas ----
    AABB o[8];
    CHECK(rsShape(0, 0, 0, XB_VINE, 9, true, o) == 2);   // bits 0 e 3: dois lados
    CHECK(rsShape(0, 0, 0, XB_VINE, 16, true, o) == 1 && o[0].y0 > .9);   // so por cima
    CHECK(rsShape(0, 0, 0, XB_LILY, 0, true, o) == 1 && o[0].y1 < .1);
    CHECK(gBlkColl[XB_LILY] != SHAPE_NONE && gBlkColl[XB_VINE] == SHAPE_NONE);   // pisa na vitoria-regia; a vinha nao colide
    CHECK(climbBlk(XB_VINE) && climbBlk(XB_LADDER) && !climbBlk(XB_LILY));
    // ---- vitoria-regia ----
    clearArena(); setB(5, AY - 1, 5, WATER);
    Slot s; s.id = 1200; s.n = 2; gXbItem[1200] = XB_LILY; creative = false;
    const float eye[3] = {5.5f, AY + 1.5f, 5.5f}, down[3] = {0.f, -1.f, 0.f};
    CHECK(lilyUse(s, eye, down)); CHECK(getB(5, AY, 5) == XB_LILY && s.n == 1);   // clicou na agua: poe em cima
    CHECK(lilyUse(s, eye, down) && s.n == 1);   // celula ocupada: nada
    clearArena(); const float eye2[3] = {5.5f, AY - 0.5f, 5.5f}; s.n = 2; CHECK(lilyUse(s, eye2, down) && getB(5, AY - 1, 5) != XB_LILY && s.n == 2);   // chao seco: nao poe
    clearArena(); setB(5, AY - 1, 5, WATER); setB(5, AY, 5, XB_LILY); setB(5, AY - 1, 5, AIR); lilyNeighbor(5, AY, 5, 0, 0, 0);
    CHECK(getB(5, AY, 5) == AIR);   // sem agua embaixo: some
    // ---- vinha ----
    clearArena(); setB(6, AY, 5, STONE);   // bloco de apoio a +X da celula (5,AY,5)
    int hh[3] = {6, AY, 5}, pp[3] = {5, AY, 5};
    CHECK(rsPlace(XB_VINE, 0, hh, pp, 0.f, 0, 0, 0)); CHECK(getB(5, AY, 5) == XB_VINE && getM(5, AY, 5) == 8);   // clique na face -X do bloco a +X: bit 3
    setB(5, AY, 6, STONE); int h2[3] = {5, AY, 6}, p2[3] = {5, AY, 5};
    CHECK(rsPlace(XB_VINE, 0, h2, p2, 0.f, 0, 0, 0) && getM(5, AY, 5) == 9);   // outro lado: junta (bit 0)
    CHECK(!rsPlace(XB_VINE, 0, h2, p2, 0.f, 0, 0, 0));   // mesmo lado de novo: nada
    setB(6, AY, 5, AIR); vineNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == XB_VINE && getM(5, AY, 5) == 1);   // perdeu o apoio +X: fica so o +Z
    setB(5, AY, 6, AIR); vineNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == AIR);   // sem apoio nenhum: some
    // vinha pendurada: a de baixo se segura na de cima
    clearArena(); setB(6, AY + 1, 5, STONE); setBM(5, AY + 1, 5, XB_VINE, 8); setBM(5, AY, 5, XB_VINE, 8);
    vineNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == XB_VINE);   // lado igual da vinha de cima segura
    setB(6, AY + 1, 5, AIR); vineNeighbor(5, AY + 1, 5, 0, 0, 0); CHECK(getB(5, AY + 1, 5) == AIR);   // a de cima cai sem apoio
    // vinha por baixo de bloco (bit 4)
    clearArena(); setB(5, AY + 2, 5, STONE); int h3[3] = {5, AY + 2, 5}, p3[3] = {5, AY + 1, 5};
    CHECK(rsPlace(XB_VINE, 0, h3, p3, 0.f, 0, 0, 0) && getM(5, AY + 1, 5) == 16);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
