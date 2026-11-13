// patchCapa.3: as 5 capas padrao (assets/capes/*.png, layout 64x32 do MC) carregam, ficam opacas e entram na lista; tooltips de Configuracoes existem
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    chdir("..");
    capeScan();
    static const char* const N[5] = {"Creeper.png", "Diamante.png", "Ender.png", "Por do sol.png", "Realeza.png"};
    for (const char* n : N) {
        CHECK(std::find(gCapeFiles.begin(), gCapeFiles.end(), std::string(n)) != gCapeFiles.end());
        CapeAnim a; CHECK(capeFromFile(std::string("assets/capes/") + n, a)); CHECK(a.fr.size() == 1);
        if (a.fr.empty()) continue;
        int opaque = 0; for (int y = 2; y < 34; y++) for (int x = 2; x < 22; x++) if (a.fr[0][((size_t)y * CAPE_W + x) * 4 + 3] == 255) opaque++;
        CHECK(opaque == 20 * 32);
        std::vector<uint8_t> enc; capePack(a, enc); CapeAnim b; CHECK(capeUnpack(enc.data(), enc.size(), b));
    }
    CHECK(capeCount() >= 1 + CAPE_NBI + 5);
    CHECK(!cfgTip("fov").empty() && !cfgTip("capesel").empty() && cfgTip("nada.xyz").empty());
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
