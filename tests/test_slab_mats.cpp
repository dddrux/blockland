// patch5.71: facilitador RS_SM de laje/escada (15 materiais, 1.12.2). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    std::string sd, tp, bt;
    CHECK(RS_SM_N == 16 && RS_SM_N <= 16);   // 4 bits de variante
    rsMat(0, sd, tp, bt); CHECK(sd == "block/oak_planks" && tp == sd && bt == sd);
    rsMat(7, sd, tp, bt); CHECK(sd == "block/sandstone" && tp == "block/sandstone_top" && bt == "block/sandstone_bottom");
    rsMat(8, sd, tp, bt); CHECK(sd == "block/stone_bricks");
    rsMat(11, sd, tp, bt); CHECK(sd == "block/nether_bricks");
    rsMat(12, sd, tp, bt); CHECK(sd == "block/quartz_block_side" && tp == "block/quartz_block_top" && bt == "block/quartz_block_bottom");
    rsMat(13, sd, tp, bt); CHECK(sd == "block/red_sandstone" && tp == "block/red_sandstone_top");
    rsMat(14, sd, tp, bt); CHECK(sd == "block/purpur_block");
    rsMat(10, sd, tp, bt); CHECK(sd == "block/stone_slab_side" && tp == "block/stone_slab_top" && bt == tp);   // laje de pedra lisa do 1.12
    rsMat(99, sd, tp, bt); CHECK(sd == "block/oak_planks");   // fora da faixa: ultimo (laje petrificada), sem estourar
    // variante no meta (laje: bits 2..5; escada: bits 3..6)
    CHECK(rsVariant(XB_SLAB, 15 << 2) == 15 && rsVariant(XB_STAIR, 14 << 3) == 14);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
