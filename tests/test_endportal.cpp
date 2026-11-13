// patchEndPortal.1: o portal do End sai da malha (desenhado por drawEndPortals) e o mesher registra as celulas
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    WORLD_SEED = 12345u; resetWorldData(); gDim = 0; for (int z = -1; z <= 1; z++) for (int x = -1; x <= 1; x++) gen(x, z);
    Chunk* c = getC(0, 0); CHECK(c != nullptr); if (!c) return 1;
    const int y = 200; gEndCells.clear();
    for (int z = 4; z <= 6; z++) for (int x = 4; x <= 6; x++) { c->b[idx(x, y, z)] = (BT)XB_PORTAL; c->m[idx(x, y, z)] = 2; }
    c->b[idx(9, y, 9)] = (BT)XB_PORTAL; c->m[idx(9, y, 9)] = 0;   // portal do Nether (meta sem bit 2): continua na malha
    c->maxY = -1; build(0, 0, c);
    CHECK(gEndCells.size() == 9);                       // 9 celulas do End registradas; a do Nether nao
    CHECK(gEndCells.count(wkey(5, y, 5)) == 1 && gEndCells.count(wkey(9, y, 9)) == 0);
    size_t withEnd = c->mesh.size() + c->wmesh.size();
    for (int z = 4; z <= 6; z++) for (int x = 4; x <= 6; x++) { c->b[idx(x, y, z)] = AIR; c->m[idx(x, y, z)] = 0; }
    c->maxY = -1; build(0, 0, c);
    CHECK(c->mesh.size() + c->wmesh.size() == withEnd);   // as 9 celulas do End nao somam vertices (so a do Nether)
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
