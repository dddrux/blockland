// Conexao de cerca/muro/painel com a face de tras da escada (1.12: BlockFaceShape.SOLID). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); }
}
int main() {
    chdir("/tmp"); arena();
    const int kind[3] = {XB_FENCE, XB_WALL, XB_PANE};
    for (int k = 0; k < 3; k++) {
        const int b = kind[k], px = 5, pz = 4 + k * 4;
        setBM(px, AY, pz, b, 0);
        int connecting = 0;
        for (int r = 0; r < 4; r++) {   // escada a leste (d=1): so conecta se a face alta olha para a cerca (lado oeste)
            setBM(px + 1, AY, pz, XB_STAIR, r);
            const int m = rsConnMask(b, 0, px, AY, pz); const bool east = (m & 2) != 0;
            CHECK(east == (rsStairBackDir(r) == 3)); if (east) connecting++;
            AABB o[8]; const int n = blockBoxesId(XB_STAIR, px + 1, AY, pz, false, o, r); bool tallWest = false;   // a colisao real tem o degrau alto no lado oeste?
            for (int i = 0; i < n; i++) if (o[i].y1 - AY > .99 && o[i].x1 - (px + 1) <= .501) tallWest = true;   // blockBoxesId devolve coordenadas de mundo
            CHECK(tallWest == (rsStairBackDir(r) == 3));
        }
        CHECK(connecting == 1);
        setB(px + 1, AY, pz, COBBLE); CHECK(rsConnMask(b, 0, px, AY, pz) & 2);   // bloco inteiro continua conectando
        setB(px + 1, AY, pz, AIR); CHECK(!(rsConnMask(b, 0, px, AY, pz) & 2));
    }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
