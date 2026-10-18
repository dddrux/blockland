// patchEnt.1 / patchDeco.1: sinalizador, planta de coro, colocacao bloqueada por entidade em queda. Sem pack => SKIP.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0;
#define CHECK(c) do { if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    if (chdir("..") != 0) return 99;
    if (!fs::exists("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("SKIP\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg();
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 12; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    int hh[3] = {5, AY - 1, 5}, pp[3] = {5, AY, 5};
    CHECK(rsPlace(XB_DECOR, 2, hh, pp, 0.f, 0, 0, 0)); CHECK(getB(5, AY, 5) == XB_DECOR && (getM(5, AY, 5) & 3) == 2);
    CHECK(rsQuads(XB_DECOR, 2).size() == 18);   // 3 caixas
    { AABB t[SH_MAXB]; CHECK(blockBoxesId(XB_DECOR, 5, AY, 5, true, t, 2) == 1); }
    int h2[3] = {8, AY - 1, 8}, p2[3] = {8, AY, 8}, p3[3] = {8, AY + 1, 8};
    CHECK(rsPlace(XB_DECOR, 3, h2, p2, 0.f, 0, 0, 0)); CHECK(rsPlace(XB_DECOR, 3, p2, p3, 0.f, 0, 0, 0));
    CHECK(((getM(8, AY, 8) >> 5) & 1) == 1);   // arm +Y (bit 2+3)
    CHECK(((getM(8, AY + 1, 8) >> 4) & 1) == 1);   // arm -Y (bit 2+2)
    CHECK(rsQuads(XB_DECOR, getM(8, AY, 8)).size() >= 10);
    { AABB t[SH_MAXB]; CHECK(blockBoxesId(XB_DECOR, 8, AY, 8, true, t, getM(8, AY, 8)) == 2); }
    // bloco em queda impede colocar bloco dentro
    gFallEnts.push_back({12.5, AY + .5, 12.5, 12.5, AY + .5, 12.5, 0., SAND, 0});
    PlaceCtx c{12, AY - 1, 12, 3, .5, 1., .5, 0.f, 0.f, false, 0, 100, 0};
    CHECK(!placeBlock(STONE, c)); gFallEnts.clear(); CHECK(placeBlock(STONE, c));
    TNTS.push_back({14.5, AY, 14.5, 14.5, AY, 14.5, 0, 0, 0, 50}); PlaceCtx d{14, AY - 1, 14, 3, .5, 1., .5, 0.f, 0.f, false, 0, 100, 0};
    CHECK(!placeBlock(STONE, d));
    printf("%s (%d falhas)\n", fails ? "FALHOU" : "OK", fails); return fails ? 1 : 0;
}
