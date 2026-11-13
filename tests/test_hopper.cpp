// patchHop.2: funil embutido (redstone.inc case 10): so texturas opacas, tudo dentro do bloco, bico para baixo/lado, malha do chunk sem quads fora da celula. Roda o main.cpp REAL (texturas falsas).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    PK.resize(3); const char* F[3] = {"block/hopper_outside", "block/hopper_top", "block/hopper_inside"}; for (int i = 0; i < 3; i++) { PK[i].file = F[i]; PK[i].name = F[i]; PK[i].tile = 10 + i; gPkByFile[F[i]] = PK0 + i; }
    for (int m : {0, 2, 3, 4, 5}) {
        std::vector<XQuad> q; rsBuild(XB_HOPPER, m, q); CHECK(q.size() >= 30);
        bool alpha = false, out = false; float ymax = 0; for (auto& a : q) { if (a.tile == 11) alpha = true; for (int k = 0; k < 4; k++) { ymax = std::max(ymax, a.p[k][1]); for (int i = 0; i < 3; i++) if (a.p[k][i] < -1e-4f || a.p[k][i] > 1.0001f) out = true; } }
        CHECK(!alpha); CHECK(!out); CHECK(ymax > .999f && ymax < 1.0001f);   // aro termina em y=1 e nada passa de 1 (aro flutuante)
    }
    const int AY = 200; for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = 0; z < 8; z++) for (int x = 0; x < 8; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    setBM(2, AY, 2, XB_HOPPER, 0); Chunk* c = getC(0, 0); build(0, 0, c);
    float ymax = 0; for (const V& v : c->mesh) if (v.x >= 2 && v.x <= 3 && v.z >= 2 && v.z <= 3 && v.y > AY - .5f) ymax = std::max(ymax, v.y - AY);
    CHECK(ymax <= 1.0001f);
    printf("test_hopper: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
