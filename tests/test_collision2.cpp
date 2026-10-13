// Parte 1: colisao do jogador (varredura em sub-passos + sair de dentro de blocos). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;   // piso de pedra em y=199, ar acima
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
}
static Phys mk(double x, double y, double z) { Phys P; P.x = P.px = x; P.y = P.py = y; P.z = P.pz = z; return P; }
static bool overlaps(const Phys& P) { AABB b = physBox(P); return !collEmpty(AABB{b.x0 + 1e-4, b.y0 + 1e-4, b.z0 + 1e-4, b.x1 - 1e-4, b.y1 - 1e-4, b.z1 - 1e-4}); }

int main() {
    arena();
    std::vector<AABB> l;
    SECTION("cerca: caixa de 1,5 de altura e poste de 4/16");
    setB(6, AY, 2, XB_FENCE); collBoxes(AABB{6, AY, 2, 7, AY + 2, 3}, l);
    CHECK(l.size() == 1); if (!l.empty()) { CHECK(fabs(l[0].y1 - (AY + 1.5)) < 1e-9); CHECK(fabs(l[0].x0 - 6.375) < 1e-9 && fabs(l[0].x1 - 6.625) < 1e-9); }
    { Phys P = mk(2.5, AY, 2.5); moveEntity(P, 4, 0, 0); CHECK(P.x <= 6.375 - PW + 1e-6); CHECK(P.collH); }
    { Phys P = mk(6.5, AY + 2.0, 2.5); P.mx = 0; moveEntity(P, 0, -1, 0); CHECK(fabs(P.y - (AY + 1.5)) < 1e-6 && P.onGround); }   // cai em cima da cerca
    SECTION("cerca com ligacao (sul) tem braco");
    setB(6, AY, 2, XB_FENCE); setM(6, AY, 2, 4); collBoxes(AABB{6, AY, 2, 7, AY + 2, 3}, l); CHECK(l.size() == 2);
    SECTION("painel: 2/16 de espessura e 1 de altura");
    setB(6, AY, 2, AIR); setB(8, AY, 2, XB_PANE); collBoxes(AABB{8, AY, 2, 9, AY + 2, 3}, l);
    CHECK(!l.empty()); if (!l.empty()) { CHECK(fabs(l[0].y1 - (AY + 1)) < 1e-9); CHECK(fabs(l[0].x0 - 8.4375) < 1e-9); }
    SECTION("muro: poste de 8/16 e altura 1,5");
    setB(10, AY, 2, XB_WALL); collBoxes(AABB{10, AY, 2, 11, AY + 2, 3}, l);
    CHECK(!l.empty()); if (!l.empty()) CHECK(fabs(l[0].y1 - (AY + 1.5)) < 1e-9);
    SECTION("tocha de redstone e fio nao bloqueiam");
    setB(12, AY, 2, XB_DUST); collBoxes(AABB{12, AY, 2, 13, AY + 1, 3}, l); CHECK(l.empty());
    SECTION("nao sobe degrau de 1 bloco, mas sobe laje (<= 0,5) se existir");
    setB(14, AY, 2, STONE); { Phys P = mk(12.5, AY, 2.5); P.onGround = true; moveEntity(P, 3, 0, 0); CHECK(P.x <= 14 - PW + 1e-6); }
    SECTION("laje: baixo 0,5; cima 0,5..1; dupla cheia; anda por cima sem pular");
    for (int h = 0; h < 3; h++) { setB(16, AY, 2, XB_SLAB); setM(16, AY, 2, h | (1 << 2)); collBoxes(AABB{16, AY, 2, 17, AY + 2, 3}, l);
        CHECK(l.size() == 1); if (!l.empty()) { CHECK(fabs(l[0].y0 - (h == 1 ? AY + .5 : AY)) < 1e-9); CHECK(fabs(l[0].y1 - (h == 0 ? AY + .5 : AY + 1)) < 1e-9); } }
    setB(16, AY, 2, XB_SLAB); setM(16, AY, 2, 0 | (1 << 2)); { Phys P = mk(14.5, AY, 3.5); P.onGround = true; moveEntity(P, 4, 0, 0); CHECK(P.x > 17.0); CHECK(fabs(P.y - (AY + .5)) < 1e-6 || P.y <= AY + .5 + 1e-6); }
    SECTION("escada de baixo: 2 caixas (0..0,5 cheia e 0,5..1 em meia celula)");
    for (int d = 0; d < 4; d++) { setB(20, AY, 2, XB_STAIR); setM(20, AY, 2, d | (1 << 3)); collBoxes(AABB{20, AY, 2, 21, AY + 2, 3}, l);
        CHECK(l.size() == 2); double top = 0, lowFull = 0; for (const AABB& b : l) { top = std::max(top, b.y1 - AY); if (fabs(b.y1 - AY - .5) < 1e-9 && b.x1 - b.x0 > .99 && b.z1 - b.z0 > .99) lowFull = 1; }
        CHECK(fabs(top - 1) < 1e-9); CHECK(lowFull == 1); }
    SECTION("escada de cima: caixa 0,5..1 cheia");
    setB(20, AY, 2, XB_STAIR); setM(20, AY, 2, 0 | (1 << 2) | (1 << 3)); collBoxes(AABB{20, AY, 2, 21, AY + 2, 3}, l);
    { bool full = false; for (const AABB& b : l) if (fabs(b.y0 - AY - .5) < 1e-9 && fabs(b.y1 - AY - 1) < 1e-9 && b.x1 - b.x0 > .99 && b.z1 - b.z0 > .99) full = true; CHECK(full); }
    SECTION("agua corrente: empurra para o lado mais baixo, vetor normalizado (.014)");
    setB(4, AY, 10, WATER); setB(5, AY, 10, 47); setB(6, AY, 10, 46);
    { Phys P = mk(5.5, AY, 10.5); P.mx = P.my = P.mz = 0; waterPush(P); CHECK(fabs(P.mx - .014) < 1e-9 && fabs(P.my) < 1e-12 && fabs(P.mz) < 1e-12); }
    SECTION("agua caindo ao lado de bloco cheio: corrente para baixo");
    for (int y = AY; y < AY + 3; y++) { setB(10, y, 10, WATER_FALL); setB(11, y, 10, STONE); }
    { Phys P = mk(10.5, AY, 10.5); P.mx = P.my = P.mz = 0; waterPush(P); CHECK(P.my < -.0139 && fabs(P.mx) < 1e-6); }
    SECTION("fora da agua nao empurra");
    { Phys P = mk(18.5, AY, 10.5); P.mx = P.my = P.mz = 0; waterPush(P); CHECK(P.mx == 0 && P.my == 0 && P.mz == 0); }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
