// Vilas: nada na agua; arvores bloqueadas dentro/perto das pecas.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    int nv = 0, nvw = 0;
    for (int seed : {777, 12345, 99}) { WORLD_SEED = seed; gVil.clear(); gVilOk.clear();
        for (int rz = -4; rz <= 4; rz++) for (int rx = -4; rx <= 4; rx++) { const Village* v = vilGet(rx, rz); if (!v) continue; nv++;
            long wet = 0, tot = 0; for (const VPiece& p : v->p) { if (p.kind == 2) continue; for (int x = p.x0; x < p.x0 + p.w; x++) for (int z = p.z0; z < p.z0 + p.d; z++) { tot++; if (vilGround(x, z) <= SEA) wet++; } }
            if (wet * 4 > tot) nvw++;
            const VPiece& p0 = v->p[0]; CHECK(vilTreeBlocked(p0.x0 + 1, p0.z0 + 1)); CHECK(vilTreeBlocked(p0.x0 - 3, p0.z0 - 3)); } }
    printf("  vilas: %d, molhadas: %d\n", nv, nvw); CHECK(nv > 0); CHECK(nvw == 0);
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
