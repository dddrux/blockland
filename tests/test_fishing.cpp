// Vara de pesca (fishing.inc): lancar, flutuar na agua, mordida, recolher, cancelar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -12; z < 12; z++) for (int x = -12; x < 12; x++) for (int y = 196; y < 216; y++) setB(x, y, z, y < 199 ? STONE : y == 199 ? WATER : AIR);
    Phys P; P.x = .5; P.y = 200.0; P.z = .5; P.sneak = false; gDead = false; gGm = 0; creative = false;
    gRodId = 12345; Slot rod; rod.id = gRodId; rod.n = 1;
    rodCast(P, 0.f, -20.f);   // pitch negativo = olhando para baixo
    CHECK(gBobOn);
    for (int i = 0; i < 80; i++) rodStep(P);
    CHECK(gBob.water && !gBob.ground);
    CHECK(fabs(gBob.y - 199.88) < .3);                 // flutua na superficie
    gBob.wait = 1; for (int i = 0; i < 3; i++) rodStep(P);
    CHECK(gBob.bite > 0);                              // mordeu
    rodReel(rod, P, true); CHECK(!gBobOn);             // recolheu
    CHECK(rod.d == 1);                                 // 1 de durabilidade
    rodCast(P, 0.f, -80.f); for (int i = 0; i < 400; i++) rodStep(P);   // para o ceu: cai
    rodReel(rod, P, false); CHECK(!gBobOn);
    printf(fails ? "FALHAS: %d/%d\n" : "OK: %d checks\n", fails ? fails : checks, checks);
    return fails ? 1 : 0;
}
