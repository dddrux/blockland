// Biomas (GenLayer 1.12.2 simplificado): so 4 biomas de terra, oceano/praia, deterministico, terreno gerado sem buracos.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345u; int cnt[NBIOME] = {0}, tot = 0;
    for (int z = -4096; z < 4096; z += 32) for (int x = -4096; x < 4096; x += 32) { cnt[biomeAt((float)x, (float)z)]++; tot++; }
    for (int b = 0; b < NBIOME; b++) printf("  %-10s %5.1f%%\n", BIOME_NAME[b], 100.0 * cnt[b] / tot);
    CHECK(cnt[B_PLAINS] && cnt[B_FOREST] && cnt[B_DESERT] && cnt[B_MOUNTAINS] && cnt[B_OCEAN] && cnt[B_BEACH]);
    CHECK(!cnt[B_TAIGA] && !cnt[B_TUNDRA] && !cnt[B_SWAMP] && !cnt[B_JUNGLE] && !cnt[B_SAVANNA]);
    CHECK(cnt[B_OCEAN] < tot * 0.6 && cnt[B_PLAINS] + cnt[B_FOREST] + cnt[B_DESERT] + cnt[B_MOUNTAINS] > tot * 0.35);
    CHECK(biomeAt(0.f, 0.f) != B_OCEAN);   // origem sempre em terra
    const int a = biomeAt(777.f, -333.f); gGL.seed = -1; CHECK(biomeAt(777.f, -333.f) == a);   // mesma seed => mesmo bioma
    WORLD_SEED = 999u; int diff = 0; for (int i = 0; i < 200; i++) if (biomeAt(i * 97.f, i * -61.f) != gBioFrom(GC_PLAINS)) diff++; CHECK(diff > 0);
    // terreno de chunks: altura plausivel e sem camada vazia
    WORLD_SEED = 12345u; g112.ready = false; int hmin = 999, hmax = 0, water = 0, land = 0;
    static BT B[CW * CH * CW]; uint8_t bo[256]; int tp[256];
    for (int cz = -6; cz < 6; cz++) for (int cx = -6; cx < 6; cx++) {
        terrain112(cx, cz, B, bo, tp);
        for (int i = 0; i < 256; i++) { hmin = std::min(hmin, tp[i]); hmax = std::max(hmax, tp[i]); if (tp[i] < SEA) water++; else land++; }
    }
    printf("  alturas %d..%d, agua %d, terra %d\n", hmin, hmax, water, land);
    CHECK(hmin > 5 && hmax < CH - 8 && land > 0);
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
