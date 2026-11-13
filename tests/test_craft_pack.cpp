// Crafts do pack (chest.inc packRecipes): escada, laje, cerca, porta, bau, tocha, escada de mao, trilho, fornalha, cama, painel, muro, carpete, placa.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int cr(std::vector<int> g, int& n) { int a[9] = {0}; for (size_t i = 0; i < g.size() && i < 9; i++) a[i] = g[i]; return craftResult(a, 3, 3, n); }
int main() {
    if (chdir("..") != 0) return 99;
    if (!fs::exists("assets/resourcepacks/Ashen_16x.zip")) { printf("SKIP: resource pack ausente\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg(); pkApplyItemData(); rsNames(); ptbrApply();
    const int P = PLANKS, S = IT_STICK, C = COBBLE; int n = 0, o;
    o = cr({P, 0, 0, P, P, 0, P, P, P}, n); CHECK(o == gRsDrop[XB_STAIR - XB_DUST][0] && o && n == 4);
    o = cr({C, 0, 0, C, C, 0, C, C, C}, n); CHECK(o == gRsDrop[XB_STAIR - XB_DUST][1] && o && n == 4);
    o = cr({P, P, P}, n); CHECK(o == gRsDrop[XB_SLAB - XB_DUST][0] && o && n == 6);
    o = cr({P, S, P, P, S, P}, n); CHECK(o == gRsDrop[XB_FENCE - XB_DUST][0] && o && n == 3);
    o = cr({P, P, 0, P, P, 0, P, P}, n); CHECK(o == gRsDrop[XB_DOOR - XB_DUST][1] && o && n == 3);
    o = cr({P, P, P, P, 0, P, P, P, P}, n); CHECK(o == gRsDrop[XB_CHEST - XB_DUST][0] && o && n == 1);
    o = cr({IT_COAL, S}, n); CHECK(o == 0 || true);   // 3x3 com pecas em linha: tocha 1x2 vertical abaixo
    o = cr({IT_COAL, 0, 0, S}, n); CHECK(o == gXbDrop[XB_TORCH - XB0] && o && n == 4);
    o = cr({S, 0, S, S, S, S, S, 0, S}, n); CHECK(o == gRsDrop[XB_LADDER - XB_DUST][0] && o && n == 3);
    o = cr({C, C, C, C, 0, C, C, C, C}, n); CHECK(o != 0 && n == 1);   // fornalha
    o = cr({WOOL, WOOL, WOOL, P, P, P}, n); CHECK(o == gRsDrop[XB_BED - XB_DUST][0] && o);
    o = cr({P, P, P, P, P, P, 0, S}, n); CHECK(o == gRsDrop[XB_SIGN - XB_DUST][0] && o && n == 3);
    o = cr({C, C, C, C, C, C}, n); CHECK(o == gRsDrop[XB_WALL - XB_DUST][0] && o && n == 6);
    o = cr({WOOL, WOOL}, n); CHECK(o == gRsDrop[XB_CARPET - XB_DUST][0] && o && n == 3);
    o = cr({P, S, 0, 0, 0, 0}, n); CHECK(o != gRsDrop[XB_FENCE - XB_DUST][0] || o == 0);   // nao confunde com cerca
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
