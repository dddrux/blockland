// Os baldes do pack Ashen_16x precisam virar itens com id (antes ficavam 0: empilhavam 64 e nao funcionavam).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    const char* names[3] = {"bucket", "water_bucket", "lava_bucket"};
    PK.clear(); for (int i = 0; i < 3; i++) { PkItem it; it.file = std::string("item/") + names[i]; it.name = names[i]; it.cube = false; it.tile = PKTILE0 + i; PK.push_back(it); }
    pkBuildProps();
    CHECK(gBktEmpty >= PK0 && gBktWater >= PK0 && gBktLava >= PK0 && gBktEmpty != gBktWater && gBktWater != gBktLava);
    CHECK(maxStack(gBktWater) == 1 && maxStack(gBktLava) == 1 && maxStack(gBktEmpty) == 16 && maxStack(STONE) == 64);
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
