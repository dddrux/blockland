// patchCrop.2: farinha de osso em colheitas (crops.inc cropUse). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    PK.resize(2); const char* F[2] = {"item/bone_meal", "item/wheat_seeds"}; for (int i = 0; i < 2; i++) { PK[i].file = F[i]; PK[i].name = F[i]; PK[i].tile = 10 + i; gPkByFile[F[i]] = PK0 + i; }
    const int BM = PK0, AY = 200; for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = 0; z < 8; z++) for (int x = 0; x < 8; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    const int hh[3] = {2, AY, 2}, pp[3] = {2, AY + 1, 2};
    for (int ty : {0, 1, 3}) for (int t = 0; t < 40; t++) {   // trigo, cenoura, beterraba; varias sementes aleatorias
        setBM(2, AY, 2, XB_CROP, ty << 3); Slot s; s.id = BM; s.n = 5; s.d = 0;
        const int r = cropUse(s, hh, pp, false); const int m = getM(2, AY, 2);
        CHECK(r == 2); CHECK(((m >> 3) & 3) == ty);   // tipo preservado, item gasto pelo chamador
        const int age = m & 7; if (ty == 3) CHECK(age <= 1); else CHECK(age >= 2 && age <= 5);
    }
    setBM(2, AY, 2, XB_CROP, 7); { Slot s; s.id = BM; s.n = 1; s.d = 0; CHECK(cropUse(s, hh, pp, false) == 0); CHECK((getM(2, AY, 2) & 7) == 7); }   // maduro: nao gasta
    setBM(2, AY, 2, XB_CROP, 6); { Slot s; s.id = BM; s.n = 1; s.d = 0; CHECK(cropUse(s, hh, pp, false) == 2); CHECK((getM(2, AY, 2) & 7) == 7); }   // topa em 7
    printf("test_crop: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
