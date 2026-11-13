// patchView.1: dump dos icones 2D (tocha, tocha de redstone, estandartes) do pack REAL para /tmp/icons_view.raw (+ checagens). Sem pack => SKIP.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int findItem(int blk, int var) { for (int id : gCreative[1]) if (isPk(id) && xbItemBlock(id) == blk && (var < 0 || gXbVar[id] == var)) return id; return 0; }
struct Box { int n = 0, x0 = 99, y0 = 99, x1 = -1, y1 = -1, part = 0; };
static Box bbox(int t) { Box b; for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { int i = ((t / ATL * 16 + y) * ATW + (t % ATL * 16 + x)) * 4; int a = atlas[i + 3]; if (a) { b.n++; b.x0 = std::min(b.x0, x); b.x1 = std::max(b.x1, x); b.y0 = std::min(b.y0, y); b.y1 = std::max(b.y1, y); if (a < 255) b.part++; } } return b; }
int main() {
    if (chdir("..") != 0) return 99;
    if (!fs::exists("assets/resourcepacks/Default-Java-1.12.2.zip") && !fs::exists("assets/resourcepacks/Ashen_16x.zip")) { printf("SKIP: resource pack ausente\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg(); pkApplyItemData(); rsNames(); ptbrApply();
    std::vector<int> tiles; std::vector<std::string> nm;
    int tr = gXbDrop[XB_TORCH - XB0], rt = gXbDrop[XB_RTORCH - XB0];
    CHECK(tr != 0); CHECK(rt != 0);
    for (int it : {tr, rt}) if (it) { const int t = iconSpriteTile(it); tiles.push_back(t); nm.push_back(isPk(it) ? PK[it - PK0].name : std::string("item") + std::to_string(it));
        Box b = bbox(t); printf("  %s tile=%d px=%d caixa=%d,%d-%d,%d parcial=%d\n", nm.back().c_str(), t, b.n, b.x0, b.y0, b.x1, b.y1, b.part);
        CHECK(b.n >= 8 && b.n < 120);               // tocha fina, nao um quadrado cheio
        CHECK(b.y1 - b.y0 >= 9);                    // alta
        CHECK(b.x1 - b.x0 <= 6); }                  // estreita (2D, sem perspectiva 3D)
    for (int c = 0; c < 16; c++) { const int it = findItem(XB_BANNER, c); CHECK(it != 0); if (!it) continue; const int t = iconSpriteTile(it);
        CHECK(t == entSprite(1, c)); Box b = bbox(t);
        CHECK(b.x1 - b.x0 + 1 == 8); CHECK(b.y1 - b.y0 + 1 == 16); CHECK(b.n == 8 * 16); CHECK(b.part == 0);   // 8 de largura x (1 barra + 15 pano), tudo opaco
        if (c == 0 || c == 14) { tiles.push_back(t); nm.push_back(PK[it - PK0].name); } }
    // mao/chao: tocha nao usa o ramo 3D
    CHECK(torchHeldXb(tr) == 0);
    FILE* f = fopen("/tmp/icons_view.raw", "wb");
    for (int t : tiles) for (int y = 0; y < 16; y++) fwrite(&atlas[(((size_t)(t / ATL * 16 + y)) * ATW + t % ATL * 16) * 4], 1, 64, f);
    fclose(f); printf("tiles=%zu\n", tiles.size());
    printf("%d checks, %d falhas\n", checks, fails); return fails != 0;
}
