// patchBlk.2: caminho de terra 15/16, caldeirao e bolo (XB_DECOR). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = 8; z < 16; z++) for (int x = 0; x < 8; x++) for (int y = AY - 1; y < AY + 4; y++) setBM(x, y, z, y == AY - 1 ? STONE : AIR, 0);
    SECTION("caminho: 15/16 de altura (colisao e selecao)");
    { AABB t[SH_MAXB]; setBM(1, AY, 9, XB_FARM, 1);
      for (bool sel : {false, true}) { int n = blockBoxesId(XB_FARM, 1, AY, 9, sel, t, 1); CHECK(n == 1); if (n == 1) CHECK(fabs(t[0].y1 - (AY + .9375)) < 1e-9); } }
    SECTION("colocar o caminho guarda a variante");
    { const int hh[3] = {2, AY - 1, 9}, pp[3] = {2, AY, 9}; setBM(2, AY, 9, AIR, 0); rsPlace(XB_FARM, 1, hh, pp, 0.f, 2.5, AY, 8.0); CHECK(getB(2, AY, 9) == XB_FARM && getM(2, AY, 9) == 1); }
    SECTION("caldeirao: colisao 5 caixas, selecao cubo");
    { AABB t[SH_MAXB]; setBM(3, AY, 9, XB_DECOR, 0); CHECK(blockBoxesId(XB_DECOR, 3, AY, 9, false, t, 0) == 5); CHECK(blockBoxesId(XB_DECOR, 3, AY, 9, true, t, 0) == 1);
      CHECK(rsSupported(XB_DECOR, 3, AY, 9)); }
    SECTION("bolo: forma, suporte, drop e comer 7 pedacos");
    { AABB t[SH_MAXB]; setBM(4, AY, 9, XB_DECOR, 1); CHECK(blockBoxesId(XB_DECOR, 4, AY, 9, false, t, 1) == 1); CHECK(fabs(t[0].y1 - (AY + .5)) < 1e-9 && fabs(t[0].x0 - (4 + .0625)) < 1e-9);
      CHECK(blockBoxesId(XB_DECOR, 4, AY, 9, false, t, 1 | (2 << 2)) == 1 && fabs(t[0].x0 - (4 + 5 / 16.0)) < 1e-9);
      CHECK(rsSupported(XB_DECOR, 4, AY, 9)); { const bool o = gNoBlockHooks; gNoBlockHooks = true; setBM(4, AY - 1, 9, AIR, 0); CHECK(!rsSupported(XB_DECOR, 4, AY, 9)); setBM(4, AY - 1, 9, STONE, 0); gNoBlockHooks = o; }
      CHECK(rsDropItemM(XB_DECOR, 1) == 0);
      creative = false; gDead = false; gFood = 0;
      for (int i = 0; i < 6; i++) { CHECK(rsUse(4, AY, 9)); CHECK(getB(4, AY, 9) == XB_DECOR && getM(4, AY, 9) == (1 | ((i + 1) << 2))); }
      CHECK(gFood == 12); CHECK(rsUse(4, AY, 9)); CHECK(getB(4, AY, 9) == AIR); CHECK(gFood == 14); }
    if (access("assets/resourcepacks/Default-Java-1.12.2.zip", R_OK) == 0) {
        SECTION("texturas do pack 1.12 resolvem (caminho, caldeirao, bolo)");
        gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip"); applyMcPack(); std::map<std::string, std::string> none;
        for (const char* n : {"block/grass_path_top", "block/grass_path_side", "block/cauldron_side", "block/cauldron_inner", "block/cauldron_top", "block/cauldron_bottom", "block/cake_top", "block/cake_side", "block/cake_inner", "block/cake_bottom"}) { const bool ok = xbTexTile(n, none) >= 0; if (!ok) printf("  MISS %s\n", n); CHECK(ok); }
        CHECK(rsQuads(XB_DECOR, 0).size() > 20); CHECK(rsQuads(XB_DECOR, 1).size() == 6); CHECK(rsQuads(XB_DECOR, 1 | (2 << 2)).size() == 6); CHECK(rsQuads(XB_FARM, 1).size() >= 5 && rsQuads(XB_FARM, 1).size() <= 6);
    }
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
