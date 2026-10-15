// Chisel (assets/chisel/chisel.cfg): registro, tiles deduplicados, faces, luz, aba, colocar/quebrar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (access("assets/chisel/chisel.cfg", R_OK) != 0 && chdir("..") != 0) return 99;
    loadPackItems();   // sem pack: so o Chisel
    CHECK(gChiselItems > 1500 && (int)PK.size() == gChiselItems);
    CHECK(gChiselTiles > 500 && gChiselTiles <= gChiselItems + 100);
    CHECK(pkTileEnd() == PKTILE0 + gChiselTiles);
    int bad = 0, cubes = 0, lit = 0, pil = 0;
    for (int i = 0; i < (int)PK.size(); i++) { const PkItem& e = PK[i];
        if (e.file.rfind("chisel/", 0) != 0) bad++; if (!e.cube) { bad++; continue; }
        for (int k = 0; k < 3; k++) if (e.ft[k] < PKTILE0 || e.ft[k] >= pkTileEnd()) bad++;
        if (isPkBlk(PK0 + i)) cubes++; if (gPkEmit[i] > 0) lit++; if (e.ft[0] != e.ft[1]) pil++; }
    CHECK(bad == 2 && cubes == gChiselItems - 2 && lit > 20 && pil > 100);
    auto it = gPkByFile.find("chisel/marble/bricks-soft"); CHECK(it != gPkByFile.end());
    CHECK(PK[it->second - PK0].name == "Mármore: Tijolos Desgastados");
    CHECK((int)gPkByHash.size() >= gChiselItems);   // sem colisao de hash entre os blocos do Chisel (rede/salvar)
    { std::unordered_set<uint32_t> hs; for (auto& e : PK) hs.insert(netFnvPk(e.file)); CHECK(hs.size() == PK.size()); }
    CHECK(gChiselIron > 0 && gChiselDia == gChiselIron + 1 && !isPkBlk(gChiselIron) && PK[gChiselIron - PK0].stack == 1);
    // cinzel: troca de variante (ordem do grupo), volta ao agachar, desgaste, so em bloco Chisel
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int m0 = gPkByFile["chisel/marble/array"], m1 = gPkByFile["chisel/marble/braid"];
    CHECK(m0 && m1 && m1 == m0 + 1);
    for (int y = 199; y < 205; y++) setB(0, y, 0, y == 199 ? STONE : AIR); setB(0, 200, 0, m0);
    float eye[3] = {.5f, 202.5f, .5f}, dir[3] = {0, -1, 0}; creative = false;
    Slot c; c.id = gChiselIron; c.n = 1; c.d = 0;
    CHECK(chiselUse(c, eye, dir, false) && getB(0, 200, 0) == m1 && c.d == 1);
    CHECK(chiselUse(c, eye, dir, true) && getB(0, 200, 0) == m0 && c.d == 2);
    setB(0, 200, 0, SAND); CHECK(chiselUse(c, eye, dir, false) && getB(0, 200, 0) == SAND && c.d == 2);   // vanilla: nada
    setB(0, 200, 0, m0); c.d = 249; chiselUse(c, eye, dir, false); CHECK(c.id == 0);                       // quebrou
    Slot dd; dd.id = gChiselDia; dd.n = 1; creative = true; chiselUse(dd, eye, dir, false); CHECK(dd.d == 0 && getB(0, 200, 0) == m1 + 1);
    { int tr = 0, gl = 0; for (int i = 0; i < (int)PK.size(); i++) { if (gPkTrans[i]) tr++; if (PK[i].file.rfind("chisel/glass", 0) == 0) { gl++; CHECK(gPkOpac[i] == 0 && gPkTrans[i] > 0); } }
      CHECK(tr > 100 && tr < 300 && gl >= 15); }
    // vanilla no grupo: pedra -> 1a variante chisel; ultima variante -> volta a pedra
    creative = true; setB(0, 200, 0, STONE); Slot v; v.id = gChiselIron; v.n = 1;
    CHECK(chiselUse(v, eye, dir, false)); const int s1 = getB(0, 200, 0); CHECK(isPk(s1) && PK[s1 - PK0].file.rfind("chisel/stone/", 0) == 0);
    CHECK(chiselUse(v, eye, dir, true) && getB(0, 200, 0) == STONE);
    setB(0, 200, 0, SAND); CHECK(chiselUse(v, eye, dir, false) && getB(0, 200, 0) == SAND);   // sem grupo: nada
    printf("%d checks, %d falhas (blocos %d, tiles %d, luminosos %d, topo!=lado %d)\n", checks, fails, gChiselItems, gChiselTiles, lit, pil);
    return fails ? 1 : 0;
}
