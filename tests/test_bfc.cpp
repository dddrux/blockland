// patchBFC.1: malha solida = cubos [0,cubeN) com ordem CCW vista de fora (culling de costas) + modelos de 2 faces [cubeN,nv); offsets coerentes.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    PK.resize(1); PK[0].file = "block/iron_bars"; PK[0].name = "block/iron_bars"; PK[0].tile = 10; gPkByFile["block/iron_bars"] = PK0;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Chunk* c = getC(0, 0);
    const int AY = 230; setBM(3, AY, 3, STONE, 0); setBM(8, AY, 8, XB_PANE, 0); setBM(9, AY, 8, GRASS, 0);   // cubo no alto + painel (modelo de 2 faces)
    c->maxY = -1; build(0, 0, c);
    CHECK(c->cubeN > 0 && c->cubeN <= (int)c->mesh.size()); CHECK(c->sOff[16] == (uint32_t)c->cubeN); CHECK(c->xOff[16] == (uint32_t)c->mesh.size()); CHECK(c->xOff[0] == (uint32_t)c->cubeN);
    for (int sc = 0; sc < 16; sc++) CHECK(c->sOff[sc] <= c->sOff[sc + 1] && c->xOff[sc] <= c->xOff[sc + 1]);
    int good = 0, bad = 0, tot = 0;
    for (int i = 0; i + 3 < c->cubeN; i += 4) {
        const V* v = &c->mesh[i]; float e1[3] = {v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z}, e2[3] = {v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z};
        float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]}; const float l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]); if (l < 1e-6f) continue;
        for (float& q : n) q /= l;
        const float cxx = (v[0].x + v[1].x + v[2].x + v[3].x) * .25f, cyy = (v[0].y + v[1].y + v[2].y + v[3].y) * .25f, czz = (v[0].z + v[1].z + v[2].z + v[3].z) * .25f;
        const int bx = (int)floorf(cxx - n[0] * .1f), by = (int)floorf(cyy - n[1] * .1f), bz = (int)floorf(czz - n[2] * .1f);
        const int fx = (int)floorf(cxx + n[0] * .1f), fy = (int)floorf(cyy + n[1] * .1f), fz = (int)floorf(czz + n[2] * .1f);
        tot++; if (getB(bx, by, bz) != AIR && !(getB(fx, fy, fz) && lightOpac(getB(fx, fy, fz)) >= 15)) good++; else bad++;   // atras = bloco; na frente nao pode ser bloco opaco
    }
    printf("cubos: %d quads, bons %d, ruins %d, modelos %d verts\n", tot, good, bad, (int)c->mesh.size() - c->cubeN);
    CHECK(tot > 100); CHECK((int)c->mesh.size() > c->cubeN);   // o painel foi para o 2o trecho; CHECK(bad * 50 <= tot);   // folhas/agua/lava com altura podem destoar (<2%)
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
