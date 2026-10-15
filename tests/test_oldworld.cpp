#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main(int argc, char** argv) {
    if (chdir("..") != 0) return 99;
    const char* w = argc > 1 ? argv[1] : "Novo Mundo";
    applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg(); pkApplyItemData(); rsNames(); ptbrApply();
    printf("PK=%zu gFurnId=%d\n", PK.size(), gFurnId);
    loadSavedChunks(w); printf("saved chunks=%zu\n", savedC.size());
    gCurDir = w; netherOnWorldLoad(w); boatsLoad(w); cartsLoad(w); chestsLoad(w); furnLoad(w); printf("loaded aux\n");
    printf("loadPlayer=%d\n", (int)loadPlayer()); mobsLoadFile(w); printf("mobs ok\n");
    int n = 0; for (auto& kv : savedC) { int cx = (int)(kv.first >> 32), cz = (int)(int32_t)kv.first; (void)cx; (void)cz; if (++n > 300) break; }
    for (int cz = -8; cz <= 8; cz++) for (int cx = -8; cx <= 8; cx++) if (!getC(cx, cz)) gen(cx, cz);
    printf("gen ok\n");
    if (gFurnId) { setBM(3, 120, 3, gFurnId, 2 | 4); setBM(4, 120, 3, gFurnId, 1); }
    int nb = 0; for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) if (Chunk* c = getC(cx, cz)) { build(cx, cz, c); nb++; }
    printf("build ok %d\n", nb);
    double sx, sy, sz; safeSpawn(0, 0, sx, sy, sz); printf("spawn ok %.0f %.0f %.0f\n", sx, sy, sz);
    for (int t = 0; t < 400; t++) runWorldTick(); printf("ticks ok\n"); return 0;
}
