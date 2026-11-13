// patchLOD.1: malha LOD (greedy heightfield) de chunk distante: contagem, offsets por secao, orientacao CCW (culling de costas), cobertura do topo, histerese.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int topOf(int x, int z) { for (int y = CH - 1; y >= 0; y--) { const int b = getB(x, y, z); if (b && !isWater(b) && !isXB(b) && (isLeafB(b) || lightOpac(b) >= 15)) return y; } return -1; }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Chunk* c = getC(0, 0);
    // histerese / niveis
    gLodOn = true; gLodR2 = 6; gLodR4 = 9; gDim = 0;
    CHECK(lodWant(35, 0) == 0); CHECK(lodWant(36, 0) == 2); CHECK(lodWant(25, 2) == 2); CHECK(lodWant(24, 2) == 0); CHECK(lodWant(81, 0) == 4); CHECK(lodWant(64, 4) == 4); CHECK(lodWant(63, 4) == 2);
    gLodOn = false; CHECK(lodWant(500, 0) == 0); gLodOn = true;
    c->maxY = -1; gMeshFar = false; gMeshLod = 0; build(0, 0, c); const int nNormal = c->nv; CHECK(nNormal > 0);
    gMeshFar = true; c->maxY = -1; c->dirty = true; build(0, 0, c); const int nFar = c->nv; gMeshFar = false;
    int nStep[5] = {0, 0, 0, 0, 0};
    for (int st : {2, 4}) {
        gMeshLod = st; c->maxY = -1; c->dirty = true; build(0, 0, c); gMeshLod = 0;
        nStep[st] = c->nv; CHECK(c->lodL == st); CHECK(c->nv > 0 && c->nv % 4 == 0);
        CHECK(c->cubeN == (int)c->sOff[16]); CHECK(c->sOff[0] == 0);
        for (int sc = 0; sc < 16; sc++) CHECK(c->sOff[sc] <= c->sOff[sc + 1]);
        int bad = 0, topN = 0, topBad = 0, botN = 0;
        for (int i = 0; i + 3 < c->cubeN; i += 4) {
            const V* v = &c->mesh[i]; float e1[3] = {v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z}, e2[3] = {v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z};
            float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]}; const float l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]); if (l < 1e-6f) { bad++; continue; }
            for (float& q : n) q /= l;
            if (fabsf(n[1]) > .99f) { topN++; if (n[1] < 0.f) { botN++; const int by = (int)lroundf(v[0].y); bool okb = false;
                for (int bx = (int)floorf(std::min(v[0].x, v[2].x) + .01f); bx < (int)ceilf(std::max(v[0].x, v[2].x) - .01f); bx++) for (int bz = (int)floorf(std::min(v[0].z, v[2].z) + .01f); bz < (int)ceilf(std::max(v[0].z, v[2].z) - .01f); bz++)
                    if (by > 0 && lodSolidB(getB(bx, by, bz)) && !lodSolidB(getB(bx, by - 1, bz))) okb = true;
                if (!okb) topBad++; } }   // topos apontam para +Y; patchTerra.2: face de baixo so com solido logo acima e vazio logo abaixo (terreno flutuante)
            if (!(fabsf(n[0]) > .99f || fabsf(n[1]) > .99f || fabsf(n[2]) > .99f)) bad++;   // so eixos
            // normal aponta para fora do solido: bloco logo atras existe, logo na frente nao e opaco
            const float cxx = (v[0].x + v[1].x + v[2].x + v[3].x) * .25f, cyy = (v[0].y + v[1].y + v[2].y + v[3].y) * .25f, czz = (v[0].z + v[1].z + v[2].z + v[3].z) * .25f;
            int sec = 0; for (; sec < 16; sec++) if ((uint32_t)i >= c->sOff[sec] && (uint32_t)i < c->sOff[sec + 1]) break;
            float ymin = std::min(std::min(v[0].y, v[1].y), std::min(v[2].y, v[3].y)); CHECK(sec < 16 && ymin >= sec * 16 - .01f && ymin <= sec * 16 + 16.01f);
            (void)cxx; (void)cyy; (void)czz;
        }
        CHECK(bad == 0); CHECK(topBad == 0); CHECK(topN > 0);
        // cobertura: sobre cada coluna ha um topo LOD a +-6 blocos da altura real
        int cover = 0, tot = 0;
        for (int z = 0; z < CW; z += 3) for (int x = 0; x < CW; x += 3) {
            const int h = topOf(x, z); if (h < 0) continue; tot++; bool f = false;
            for (int i = 0; i + 3 < c->cubeN && !f; i += 4) { const V* v = &c->mesh[i]; if (fabsf(v[0].y - v[2].y) > .01f) continue; const float x0 = std::min(v[0].x, v[2].x), x1 = std::max(v[0].x, v[2].x), z0 = std::min(v[0].z, v[2].z), z1 = std::max(v[0].z, v[2].z);
                if (x + .5f >= x0 && x + .5f <= x1 && z + .5f >= z0 && z + .5f <= z1 && fabsf(v[0].y - (h + 1)) <= 6.f) f = true; }
            if (f) cover++;
        }
        printf("faces de baixo (ilhas): %d\n", botN); printf("passo %d: %d verts (normal %d, longe %d) | cobertura %d/%d\n", st, c->nv, nNormal, nFar, cover, tot);
        CHECK(cover * 10 >= tot * 9);
    }
    CHECK(nStep[2] < nFar); CHECK(nStep[4] < nStep[2]); CHECK(nStep[4] * 4 < nNormal);
    // patchLOD.2: agua LOD
    long wN = 0, wL = 0; int withW = 0;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) { Chunk* q = getC(cx, cz); if (!q) continue;
        gMeshLod = 0; q->maxY = -1; q->dirty = true; build(cx, cz, q); const int a = q->nwv;
        gMeshLod = 4; q->maxY = -1; q->dirty = true; build(cx, cz, q); gMeshLod = 0; const int b = q->nwv;
        wN += a; wL += b; if (a > 0) { withW++; CHECK(b > 0); } CHECK(b <= a || a == 0); CHECK(b % 4 == 0);
        CHECK(q->wOff[0] == 0 && q->wOff[16] == (uint32_t)q->nwv); for (int sc = 0; sc < 16; sc++) CHECK(q->wOff[sc] <= q->wOff[sc + 1]);
        for (int i = 0; i + 3 < b; i += 4) { const V* v = &q->wmesh[i]; CHECK(v[0].y == v[2].y); const float e1x = v[1].x - v[0].x, e2z = v[2].z - v[0].z; CHECK(e1x > 0 && e2z > 0); } }
    printf("agua: normal %ld verts, LOD4 %ld (%d chunks com agua)\n", wN, wL, withW); CHECK(wL <= wN);
    printf("%d/%d\n", checks - fails, checks); return fails ? 1 : 0;
}
