// Fuzz de malha: todo id registrado x metas variadas passa por build() sem estourar (mundos antigos trazem metas que o codigo atual nao espera).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int maxId = PK0 + (int)PK.size() + 40; int builds = 0;
    const int metas[] = {0, 1, 2, 3, 4, 5, 7, 8, 12, 15, 16, 31, 32, 63, 64, 127, 128, 200, 255};
    for (int m : metas) for (int base = 1; base < maxId; base += 64) {
        for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) for (int y = 190; y < 230; y++) setBM(x, y, z, AIR, 0);
        for (int i = 0; i < 64 && base + i < maxId; i++) { const int x = (i % 8) * 2, z = (i / 8) * 2; setBM(x, 200, z, base + i, m); setBM(x, 199, z, STONE, 0); }
        Chunk* c = getC(0, 0); if (c) { build(0, 0, c); build(0, 0, c, true); ++builds; }
    }
    printf("test_meshfuzz: %d malhas sem estouro (ids ate %d)\n", builds, maxId); return 0;
}
