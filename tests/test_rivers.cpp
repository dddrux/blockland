// Rios + seeds: fracao de rio, continuidade entre chunks, determinismo, parseSeed estilo Minecraft.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static BT B[CW * CH * CW];
int main() {
    WORLD_SEED = 12345u; g112.ready = false;
    int rv = 0, tot = 0;
    for (int z = -2048; z < 2048; z += 8) for (int x = -2048; x < 2048; x += 8) { tot++; if (riverFactor(x, z) > .5f) rv++; }
    printf("  rio (centro) %.2f%% da area\n", 100.0 * rv / tot);
    CHECK(rv > tot * 0.01 && rv < tot * 0.12);
    // mapa ASCII 128x64 blocos (passo 4)
    uint8_t bo[256]; int tp[256]; int water = 0, rivWater = 0;
    for (int cz = 0; cz < 8; cz++) { for (int cx = 0; cx < 32; cx++) { terrain112(cx, cz, B, bo, tp); int w = 0; for (int i = 0; i < 256; i++) if (tp[i] < SEA) w++; putchar(w > 128 ? '~' : w > 0 ? ':' : '.'); } putchar('\n'); }
    // continuidade: rio na borda entre chunks vizinhos (altura do topo nunca salta > 40 entre colunas adjacentes sem ser montanha)
    static BT B2[CW * CH * CW]; uint8_t bo2[256]; int tp2[256]; int bad = 0, cmp = 0;
    for (int cz = 0; cz < 6; cz++) for (int cx = 0; cx < 6; cx++) {
        terrain112(cx, cz, B, bo, tp); terrain112(cx + 1, cz, B2, bo2, tp2);
        for (int z = 0; z < 16; z++) { cmp++; if (abs(tp[z * 16 + 15] - tp2[z * 16 + 0]) > 14) bad++; }
    }
    printf("  emendas suspeitas %d/%d\n", bad, cmp); CHECK(bad < cmp / 12);
    // determinismo
    terrain112(3, 3, B, bo, tp); static BT B3[CW * CH * CW]; terrain112(3, 3, B3, bo2, tp2); CHECK(memcmp(B, B3, sizeof B) == 0);
    // seeds diferentes => mundos diferentes
    WORLD_SEED = 54321u; g112.ready = false; terrain112(3, 3, B3, bo2, tp2); CHECK(memcmp(B, B3, sizeof B) != 0);
    // parseSeed
    CHECK(parseSeed("12345") == 12345u); CHECK(parseSeed("  777 ") == 777u); CHECK(parseSeed("abc") == 96354u);   // "abc".hashCode() == 96354
    CHECK(parseSeed("Minecraft") == parseSeed("Minecraft"));
    // gerenciamento de mundos: criar com seed, listar, renomear, excluir
    { std::error_code ec; fs::remove_all("/tmp/bltest", ec); fs::create_directories("/tmp/bltest", ec); if (chdir("/tmp/bltest")) return 1;
      const std::string d1 = createWorld("Meu Mundo", "424242", 1), d2 = createWorld("Meu Mundo", "abc", 0); scanWorlds();
      CHECK(d1 == "Meu Mundo" && d2 == "Meu Mundo (2)" && gWorlds.size() == 2);
      bool f1 = false, f2 = false; for (auto& w : gWorlds) { if (w.dir == d1) { f1 = w.seed == 424242u && w.mode == 1 && w.name == "Meu Mundo"; } if (w.dir == d2) f2 = w.seed == 96354u && w.mode == 0; }
      CHECK(f1 && f2);
      renameWorld(d1, "Ilha Nova"); scanWorlds(); bool rn = false; for (auto& w : gWorlds) if (w.dir == d1 && w.name == "Ilha Nova" && w.seed == 424242u) rn = true; CHECK(rn);
      CHECK(safeDirName("Cria\xc3\xa7\xc3\xa3o/..") == "Cria____o___");
      deleteWorld("../x"); deleteWorld(d1); scanWorlds(); CHECK(gWorlds.size() == 1 && gWorlds[0].dir == d2); }
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
