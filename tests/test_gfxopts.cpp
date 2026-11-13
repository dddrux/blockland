// patchCfg.7: opcoes de video novas (VSync v9, MSAA v10, Nitidez v11, Mostrar FPS v12): persistem em options.cfg (aa=, sharp=, opt.*) e gfxLoadOpts le aa/sharp; tooltips existem
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    mkdir("/tmp/gfxo", 0755); chdir("/tmp/gfxo"); remove("options.cfg");
    gSt["v9"] = 2; gSt["v10"] = 2; gSt["v11"] = 3; gSt["v12"] = 1; optSave();
    std::string cfg; { FILE* f = fopen("options.cfg", "rb"); char b[4096]; size_t n = f ? fread(b, 1, sizeof b - 1, f) : 0; b[n] = 0; cfg = b; if (f) fclose(f); }
    CHECK(cfg.find("aa=4\n") != std::string::npos); CHECK(cfg.find("sharp=-1.50\n") != std::string::npos);
    CHECK(cfg.find("opt.v9=2\n") != std::string::npos); CHECK(cfg.find("opt.v12=1\n") != std::string::npos);
    gSt.clear(); optLoad();
    CHECK(gsi("v9", 0) == 2); CHECK(gsi("v10", -1) == 2); CHECK(gsi("v11", -1) == 3); CHECK(gsi("v12", 0) == 1);
    gAA = 0; gSharp = 0.f; gfxLoadOpts(); CHECK(gAA == 4); CHECK(gSharp == -1.5f);
    // aa=8 / sharp=0 escritos a mao viram indices certos
    { FILE* f = fopen("options.cfg", "wb"); fputs("aa=8\nsharp=0.00\n", f); fclose(f); } gSt.clear(); optLoad();
    CHECK(gsi("v10", -1) == 3); CHECK(gsi("v11", -1) == 0);
    for (const char* id : {"v9", "v10", "v11", "v12"}) CHECK(!cfgTip(id).empty());
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
