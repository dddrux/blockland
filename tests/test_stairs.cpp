// Escada: cantos (1.12 getStairShape), colisao x malha coerentes, e luz de escada/laje (opacidade 255 + useNeighborBrightness). Roda o main.cpp REAL.
#include <unistd.h>
#include <filesystem>
namespace fs = std::filesystem;
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200, CX = 8, CZ = 8;
static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0};   // N L S O
static int BR[4];   // BR[d] = (m & 3) cuja face alta fica no lado d
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -16; z < 32; z++) for (int x = -16; x < 32; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
}
static void clearAround() { for (int z = CZ - 2; z <= CZ + 2; z++) for (int x = CX - 2; x <= CX + 2; x++) setB(x, AY, z, AIR); }
static int mk(int dir, int half, int mat = 7) { return BR[dir] | (half << 2) | (mat << 3); }
static void put(int x, int z, int dir, int half) { setBM(x, AY, z, XB_STAIR, mk(dir, half)); }
// ponto (x,z locais, y local) esta dentro de alguma das caixas (coords de mundo)?
static bool inside(const AABB* o, int n, double x, double y, double z) { for (int i = 0; i < n; i++) if (x > o[i].x0 - CX + 1e-9 && x < o[i].x1 - CX - 1e-9 && y > o[i].y0 - AY + 1e-9 && y < o[i].y1 - AY - 1e-9 && z > o[i].z0 - CZ + 1e-9 && z < o[i].z1 - CZ - 1e-9) return true; return false; }
// regiao alta esperada, derivada so da definicao (frente f, esquerda = anti-horario): reta = f>0; externo = f>0 & lado>0; interno = f>0 | lado>0
static bool expectHigh(int shape, int f, double rx, double rz) {
    const int ccw = (f + 3) & 3, cw = (f + 1) & 3; const double pf = rx * DX[f] + rz * DZ[f], pl = rx * DX[ccw] + rz * DZ[ccw], pr = rx * DX[cw] + rz * DZ[cw];
    switch (shape) { case 1: return pf > 0 && pl > 0; case 2: return pf > 0 && pr > 0; case 3: return pf > 0 || pl > 0; case 4: return pf > 0 || pr > 0; default: return pf > 0; }
}
static double faceArea(const XQuad& q) { float e1[3], e2[3]; for (int i = 0; i < 3; i++) { e1[i] = q.p[1][i] - q.p[0][i]; e2[i] = q.p[3][i] - q.p[0][i]; }
    const float n0 = e1[1] * e2[2] - e1[2] * e2[1], n1 = e1[2] * e2[0] - e1[0] * e2[2], n2 = e1[0] * e2[1] - e1[1] * e2[0]; return sqrt(n0 * n0 + n1 * n1 + n2 * n2); }
static float normalY(const XQuad& q) { float e1[3], e2[3]; for (int i = 0; i < 3; i++) { e1[i] = q.p[1][i] - q.p[0][i]; e2[i] = q.p[3][i] - q.p[0][i]; } return e1[2] * e2[0] - e1[0] * e2[2]; }
int main() {
    if (chdir("..") != 0) return 99;
    const bool havePack = fs::exists("assets/resourcepacks/Ashen_16x.zip");   // a malha precisa das texturas reais; sem o pack so a parte de forma/luz roda
    if (havePack) { applyMcPack(); loadPackItems(); xbInit(); } else printf("AVISO: pack ausente, checagens de malha puladas\n");
    arena();
    for (int d = 0; d < 4; d++) { BR[d] = -1; for (int r = 0; r < 4; r++) if (rsStairBackDir(r) == d) BR[d] = r; CHECK(BR[d] >= 0); }

    // 1) escolha do canto, igual ao 1.12
    for (int h = 0; h < 2; h++) for (int f = 0; f < 4; f++) {
        const int ccw = (f + 3) & 3, cw = (f + 1) & 3, bk = (f + 2) & 3;
        clearAround(); put(CX, CZ, f, h); CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);   // sozinha = reta
        for (int e : {ccw, cw}) {   // vizinha na FRENTE virada para o lado => externo (esq se ela olha para a esquerda)
            clearAround(); put(CX, CZ, f, h); put(CX + DX[f], CZ + DZ[f], e, h);
            CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == (e == ccw ? 1 : 2));
            put(CX + DX[f], CZ + DZ[f], e, 1 - h); CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);   // metade diferente => nao forma canto
            put(CX + DX[f], CZ + DZ[f], e, h); put(CX + DX[(e + 2) & 3], CZ + DZ[(e + 2) & 3], f, h);   // outra escada igual a ela do lado oposto => isDifferentStairs falha => reta
            CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);
            clearAround(); put(CX, CZ, f, h); put(CX + DX[bk], CZ + DZ[bk], e, h);   // vizinha ATRAS virada para o lado => interno
            CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == (e == ccw ? 3 : 4));
            put(CX + DX[bk], CZ + DZ[bk], e, 1 - h); CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);
        }
        clearAround(); put(CX, CZ, f, h); put(CX + DX[f], CZ + DZ[f], f, h); put(CX + DX[bk], CZ + DZ[bk], bk, h);   // mesmo eixo => reta
        CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);
        clearAround(); put(CX, CZ, f, h); setB(CX + DX[f], AY, CZ + DZ[f], COBBLE); CHECK(rsStairShape(CX, AY, CZ, mk(f, h)) == 0);   // bloco inteiro nao faz canto
    }

    // 2) colisao/selecao: forma = a esperada (base inteira + parte alta so na regiao do canto); malha cobre exatamente a mesma area do topo
    const double volExp[5] = {.75, .625, .625, .875, .875};
    for (int shape = 0; shape < 5; shape++) for (int h = 0; h < 2; h++) for (int f = 0; f < 4; f++) {
        const int m = mk(f, h); AABB o[SH_MAXB]; const int n = rsStairBoxes(shape, m, o);
        for (int i = 0; i < n; i++) { o[i].x0 += CX; o[i].x1 += CX; o[i].y0 += AY; o[i].y1 += AY; o[i].z0 += CZ; o[i].z1 += CZ; }
        int bad = 0, solidN = 0; const int G = 16;
        for (int ix = 0; ix < G; ix++) for (int iy = 0; iy < G; iy++) for (int iz = 0; iz < G; iz++) {
            const double x = (ix + .5) / G, y = (iy + .5) / G, z = (iz + .5) / G; const bool baseZone = h ? y > .5 : y < .5;
            const bool want = baseZone || expectHigh(shape, f, x - .5, z - .5); const bool got = inside(o, n, x, y, z); if (want != got) bad++; solidN += got;
        }
        CHECK(bad == 0); CHECK(fabs(solidN / (double)(G * G * G) - volExp[shape]) < 1e-9);
        if (shape == 0) { AABB p[SH_MAXB]; clearAround(); put(CX, CZ, f, h); CHECK(blockBoxesId(XB_STAIR, CX, AY, CZ, false, p, m) == 2); }   // reta = 2 caixas, como antes
        // malha: area das faces "de cima" da parte alta (plano y=1 se baixo; y=0 se de cabeca para baixo) == pegada esperada
        if (!havePack) continue;
        std::vector<XQuad> q; rsStairBake(shape ? shape : 0, m, q); if (!shape) q = rsQuads(XB_STAIR, m);
        double area = 0; float mnx = 9, mxx = -9, mnz = 9, mxz = -9; bool inUnit = true;
        for (const XQuad& k : q) {
            for (int v = 0; v < 4; v++) for (int c = 0; c < 3; c++) if (k.p[v][c] < -1e-4f || k.p[v][c] > 1 + 1e-4f) inUnit = false;
            const float py = k.p[0][1]; bool flat = true; for (int v = 1; v < 4; v++) if (fabsf(k.p[v][1] - py) > 1e-4f) flat = false;
            if (!flat || fabsf(py - (h ? 0.f : 1.f)) > 1e-4f) continue;
            if ((h ? -1 : 1) * normalY(k) <= 0) continue;
            area += faceArea(k); for (int v = 0; v < 4; v++) { mnx = std::min(mnx, k.p[v][0]); mxx = std::max(mxx, k.p[v][0]); mnz = std::min(mnz, k.p[v][2]); mxz = std::max(mxz, k.p[v][2]); }
        }
        const double foot = shape == 0 ? .5 : (shape <= 2 ? .25 : .75);
        CHECK(inUnit); CHECK(fabs(area - foot) < 1e-4);
        int expN = 0; double ex0 = 9, ex1 = -9, ez0 = 9, ez1 = -9;   // bbox da pegada esperada
        for (int ix = 0; ix < 32; ix++) for (int iz = 0; iz < 32; iz++) { const double x = (ix + .5) / 32, z = (iz + .5) / 32; if (expectHigh(shape, f, x - .5, z - .5)) { expN++; ex0 = std::min(ex0, x - .5 / 32); ex1 = std::max(ex1, x + .5 / 32); ez0 = std::min(ez0, z - .5 / 32); ez1 = std::max(ez1, z + .5 / 32); } }
        // a malha usa o mesmo giro da colisao (rotacao do xbBake): bbox da malha == bbox esperado
        CHECK(fabs(mnx - ex0) < 1e-3 && fabs(mxx - ex1) < 1e-3 && fabs(mnz - ez0) < 1e-3 && fabs(mxz - ez1) < 1e-3);
    }

    // 3) a escada ja posta ganha o canto na malha real do mundo (emitXB passa pelas coordenadas)
    clearAround(); put(CX, CZ, 0, 0); put(CX, CZ - 1, 3, 0);   // frente (norte) vira para o oeste => externo esquerdo
    CHECK(rsStairShape(CX, AY, CZ, mk(0, 0)) == 1); if (havePack) CHECK(&rsStairQuadsAt(CX, AY, CZ, mk(0, 0)) != &rsQuads(XB_STAIR, mk(0, 0)));
    setB(CX, AY, CZ - 1, AIR); if (havePack) CHECK(&rsStairQuadsAt(CX, AY, CZ, mk(0, 0)) == &rsQuads(XB_STAIR, mk(0, 0)));

    // 4) luz: escada/laje bloqueiam luz do ceu (opacidade 255 do 1.12) e a propria celula usa a luz dos vizinhos (nao fica preta)
    CHECK(lightOpacL(XB_SLAB) == 15 && lightOpacL(XB_STAIR) == 15 && lightOpac(XB_SLAB) == 0 && lightOpacL(AIR) == 0 && lightOpacL(STONE) == 15);
    for (int z = -16; z < 32; z++) for (int x = -16; x < 32; x++) { setBM(x, AY + 2, z, XB_SLAB, 7 << 2); }   // teto de meia laje cobrindo a regiao de luz do chunk 0
    for (int z = -16; z < 32; z++) for (int x = -16; x < 32; x++) if (x < 2 || x > 12 || z < 2 || z > 12) for (int y = AY; y <= AY + 1; y++) setB(x, y, z, STONE);   // paredes: so entra luz por cima da laje
    Chunk* c = getC(0, 0); build(0, 0, c);
    CHECK((c->lc[idx(CX, AY + 1, CZ)] >> 4) == 0);   // sob o teto: escuro (antes da correcao a laje deixava o ceu passar = 15)
    CHECK((c->lc[idx(CX, AY, CZ)] >> 4) == 0);
    CHECK((c->lc[idx(CX, AY + 2, CZ)] >> 4) == 15);  // a propria laje: luz do vizinho de cima, nao 0
    // escada com tocha ao lado: brilha com a luz do vizinho
    setBM(CX, AY, CZ, XB_STAIR, mk(0, 0)); setB(CX + 1, AY, CZ, XB_TORCH); build(0, 0, c);
    CHECK((c->lc[idx(CX, AY, CZ)] & 15) > 0);
    setB(CX + 1, AY, CZ, AIR); setBM(CX, AY, CZ, XB_STAIR, mk(0, 0));
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
