// patchBoat4.2: so o condutor rema (pose); olho acompanha o balanco do casco. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    BoatEnt b; b.paddleAmp[0] = b.paddleAmp[1] = 1.f; b.paddlePos[0] = b.paddlePos[1] = 1.f;
    Pose d, p; boatRowPose(d, b, 1.f); boatRowPose(p, b, 1.f, false);
    CHECK(d.oar && d.rowAmp[0] > .5f); CHECK(!p.oar && p.rowAmp[0] == 0.f && p.rowAmp[1] == 0.f);
    double dx = 9, dy = 9, dz = 9; boatEyeDelta(b, 1.f, dx, dy, dz);
    CHECK(fabs(dx) < 1.0 && fabs(dy) < 1.0 && fabs(dz) < 1.0);
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
