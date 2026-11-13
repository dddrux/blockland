#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
int main() {
    std::vector<int64_t> S = {1, -1, 2, 7, 42, 123456789LL, -123456789LL, 0x5EED1234LL, 9223372036854775807LL, (int64_t)(-9223372036854775807LL - 1), 4294967296LL, -4294967296LL, 2147483647LL, -2147483648LL, 1000000007LL};
    for (int i = 0; i < 40; i++) S.push_back((int64_t)((uint64_t)i * 0x9E3779B97F4A7C15ULL + 12345));
    int bad = 0;
    for (int64_t s : S) {
        resetWorldData(); WORLD_SEED = s; gDim = 0;
        for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) gen(dx, dz);
        double x = 0, y = 0, z = 0; safeSpawn(0, 0, x, y, z);
        const int bx = (int)floor(x), by = (int)floor(y), bz = (int)floor(z);
        const bool ok = y > 1 && y < CH - 2 && !solid(getB(bx, by, bz)) && !solid(getB(bx, by + 1, bz)) && solid(getB(bx, by - 1, bz)) && !isLiquid(getB(bx, by, bz)) && !isLiquid(getB(bx, by + 1, bz)) && by - 1 > SEA - 3;
        if (!ok) { bad++; printf("FALHOU seed %lld: spawn %d,%d,%d bloco=%d abaixo=%d\n", (long long)s, bx, by, bz, getB(bx, by, bz), getB(bx, by - 1, bz)); }
    }
    printf("seeds testadas %zu, falhas %d\n", S.size(), bad); return bad != 0;
}
