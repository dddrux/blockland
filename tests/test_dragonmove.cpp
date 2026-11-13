// patchFix.27: movimento suave do Ender Dragon
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gDim = 2; gGm = 1; creative = true; gDragonKilled = false; Phys P; P.x = 200; P.y = 70; P.z = 200; P.noclip = false;
    DragonEnt& d = gDragon; d = DragonEnt(); d.on = true; d.x = 0; d.z = 70; d.y = 85;
    double maxAcc = 0, maxYaw = 0, rmin = 1e9, rmax = 0, ymin = 1e9, ymax = -1e9; bool nan = false;
    for (int t = 0; t < 4000; t++) {
        const double ovx = d.vx, ovy = d.vy, ovz = d.vz; const float oy = d.yaw; dragonTick(P);
        if (t > 300) { maxAcc = std::max(maxAcc, sqrt((d.vx - ovx) * (d.vx - ovx) + (d.vy - ovy) * (d.vy - ovy) + (d.vz - ovz) * (d.vz - ovz)));
            float df = d.yaw - oy; while (df > 180.f) df -= 360.f; while (df < -180.f) df += 360.f; maxYaw = std::max(maxYaw, (double)fabsf(df));
            const double r = sqrt(d.x * d.x + d.z * d.z); rmin = std::min(rmin, r); rmax = std::max(rmax, r); ymin = std::min(ymin, d.y); ymax = std::max(ymax, d.y); }
        if (d.x != d.x || d.y != d.y) nan = true; d.phase = 0;   // so orbita (sem alvo valido)
    }
    printf("acc %.3f yaw %.2f r %.1f-%.1f y %.1f-%.1f\n", maxAcc, maxYaw, rmin, rmax, ymin, ymax);
    CHECK(!nan && maxAcc <= .0501 && maxYaw <= 4.001 && rmin > 40. && rmax < 72. && ymin > 66. && ymax < 95.);
    d = DragonEnt(); d.on = true; d.x = 0; d.z = 70; d.y = 85; d.phase = 3; bool landed = false;   // aproximacao: pousa
    for (int t = 0; t < 700 && !landed; t++) { dragonTick(P); if (d.phase == 4) landed = true; }
    CHECK(landed);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
