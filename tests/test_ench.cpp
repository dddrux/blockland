// test_ench: Respiracao (ar cai 1 em N+1 ticks) e velocidade de mineracao na agua/ar com Afinidade Aquatica (patchEnch.3b/3c). Rodar: tools/t.sh ench
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    for (Slot& x : armor) x = Slot();
    SECTION("T1 respiracao");
    srand(1); int air = 300; for (int i = 0; i < 1000; i++) air = enchRespAir(air); CHECK(air == 300 - 1000);   // sem capacete: cai 1 por tick
    armor[0].id = IT_A0 + 4; armor[0].n = 1; enchAdd(armor[0], 5, 3, true);
    srand(1); air = 0; for (int i = 0; i < 4000; i++) air = enchRespAir(air); CHECK(air < 0 && air > -1400 && air < -600);   // nivel 3: ~1 em 4 => ~-1000 em 4000 ticks
    SECTION("T2 mineracao");
    for (Slot& x : armor) x = Slot();
    CHECK(enchMineMul(false, true) == 1.f);
    CHECK(fabsf(enchMineMul(true, true) - .2f) < 1e-6f && fabsf(enchMineMul(false, false) - .2f) < 1e-6f && fabsf(enchMineMul(true, false) - .04f) < 1e-6f);
    armor[0].id = IT_A0 + 4; armor[0].n = 1; enchAdd(armor[0], 6, 1, true);
    CHECK(enchMineMul(true, true) == 1.f && fabsf(enchMineMul(true, false) - .2f) < 1e-6f);   // afinidade aquatica anula so a agua
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
