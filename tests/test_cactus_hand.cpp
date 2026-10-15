// patchTex.4: cacto na mao = laterais 1/16 para dentro (modelo 1.12), topo/base cheios
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip"); applyMcPack();
    CHECK(gCactusId >= PK0);
    const std::vector<V>& m = meshOf(gCactusId);
    CHECK(m.size() == 24);
    int in15 = 0, in1 = 0, full = 0; for (const V& v : m) { if (fabsf(v.x - 15.f / 16) < 1e-4f || fabsf(v.z - 15.f / 16) < 1e-4f) in15++; if (fabsf(v.x - 1.f / 16) < 1e-4f || fabsf(v.z - 1.f / 16) < 1e-4f) in1++; if (v.y > .999f) full++; }
    CHECK(in15 >= 8 && in1 >= 8);   // 4 laterais: 16 vertices recuados (os do topo/base ficam em 0/1)
    CHECK(full >= 4);               // topo cheio
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
