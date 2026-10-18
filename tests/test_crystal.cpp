// patchEnd.5-8: cristais do End (aparecem sobre o fogo dos pilares, curam, morrem por golpe/flecha/fogo apagado)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    gDim = 2; netHost = true; netOn = false;
    for (int cz = -4; cz <= 4; cz++) for (int cx = -4; cx <= 4; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Phys P; memset(&P, 0, sizeof P);
    // pilar falso 0 em (42,100,0): bedrock + fogo
    const double a0 = 2.0 * (-3.14159265358979); const int px = (int)floor(42.0 * cos(a0)), pz = (int)floor(42.0 * sin(a0));
    setB(px, 100, pz, BEDROCK); setB(px, 101, pz, XB_FIRE);
    for (int t = 0; t < 25; t++) crystalsTick(P);
    CHECK(gCrystInit[0] && gCryst[0].alive);
    CHECK(crystalsAlive(px, 101, pz, 5.0) == 1 && crystalsAlive(500, 101, 500, 5.0) == 0);
    // flecha na horizontal acerta o cristal (esfera r=1 em y+.8)
    const double o[3] = {px - 10.0, gCryst[0].y + .8, pz + .0}, d[3] = {1, 0, 0};
    CHECK(!arrowHitEnd(o, d, 5.0, 6));          // curta demais
    CHECK(arrowHitEnd(o, d, 20.0, 6) && !gCryst[0].alive);
    CHECK(getB(px, 101, pz) == AIR);            // fogo apagado
    for (int t = 0; t < 25; t++) crystalsTick(P);
    CHECK(!gCryst[0].alive);                    // nao renasce
    // convidado: fogo apagado por outro jogador mata o cristal local
    setB(px, 101, pz, XB_FIRE); gCryst[0].alive = true; gCrystInit[0] = true; setB(px, 101, pz, AIR); crystalsTick(P);
    CHECK(!gCryst[0].alive);
    // golpe do jogador
    setB(px, 101, pz, XB_FIRE); gCryst[0].alive = true;
    const float eye[3] = {(float)px - 3.f, (float)gCryst[0].y + .8f, (float)pz}, dir[3] = {1, 0, 0};
    CHECK(crystalHit(eye, dir, 3.f, 4) && !gCryst[0].alive);
    // grade no caminho bloqueia o golpe
    setB(px - 2, (int)floor(gCryst[0].y + .8), pz, XB_PANE); gCryst[0].alive = true;
    const float eye2[3] = {(float)px - 4.f, (float)gCryst[0].y + .8f, (float)pz + .5f}, dir2[3] = {1, 0, 0};
    const float eyeB[3] = {(float)px - 4.f, (float)gCryst[0].y + .8f, (float)pz + .5f};
    CHECK(!crystalHit(eyeB, dir2, 5.f, 4) && gCryst[0].alive); (void)eye2;
    setB(px - 2, (int)floor(gCryst[0].y + .8), pz, AIR);
    // saindo do End zera
    gDim = 0; crystalsTick(P); CHECK(!gCrystInit[0]);
    printf("test_crystal: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
