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
    SECTION("queda rapida nao atravessa o piso");
    for (double v : {-1.5, -3.0, -3.92, -9.0, -20.0}) {
        Phys P = mk(2.5, AY + 12, 2.5); P.my = v;
        for (int t = 0; t < 40; t++) { P.my -= .08; P.my *= .98; moveEntity(P, P.mx, P.my, P.mz); if (P.onGround) break; }
        CHECK(P.y >= AY - 1e-6); CHECK(P.onGround);
        Phys Q = mk(2.5, AY + 30, 2.5);   // um unico movimento enorme
        moveEntity(Q, 0, -40, 0);
        CHECK(fabs(Q.y - AY) < 1e-6); CHECK(Q.collV && Q.onGround);
    }
    SECTION("movimento horizontal rapido nao atravessa parede de 1 bloco");
    for (int y = AY; y < AY + 3; y++) setB(6, y, 2, STONE);
    for (double v : {4.0, 6.0, 8.0, 30.0}) {
        Phys P = mk(2.5, AY, 2.5); moveEntity(P, v, 0, 0);
        CHECK(P.x <= 6 - PW + 1e-6); CHECK(P.collH);
        CHECK(!overlaps(P));
    }
    SECTION("movimento normal (<=1) igual ao passo unico");
    { Phys A = mk(2.5, AY, 2.5), B = A; A.my = B.my = -.5; moveEntity(A, .3, -.5, .2); moveEntityOnce(B, .3, -.5, .2);
      CHECK(A.x == B.x && A.y == B.y && A.z == B.z && A.onGround == B.onGround && A.collH == B.collH); }
    SECTION("corredor de 1 bloco em velocidade alta");
    arena();
    for (int x = 0; x < 20; x++) for (int y = AY; y < AY + 3; y++) { setB(x, y, 1, STONE); setB(x, y, 3, STONE); }
    { Phys P = mk(1.5, AY, 2.5); moveEntity(P, 6, 0, 0); CHECK(fabs(P.x - 7.5) < 1e-6 && fabs(P.z - 2.5) < 1e-6); moveEntity(P, 4, 0, 4); CHECK(!overlaps(P)); CHECK(P.z > 2.5 - .3 - 1e-6 && P.z < 2.5 + .3 + 1e-6); }
    SECTION("quina de bloco: deslizar sem entrar");
    arena();
    for (int y = AY; y < AY + 3; y++) setB(5, y, 5, STONE);
    { Phys P = mk(3.5, AY, 4.0); for (int t = 0; t < 30; t++) { moveEntity(P, .3, 0, .3); CHECK(!overlaps(P)); } }
    SECTION("unstuck: dentro da parede sai para o lado mais proximo");
    arena();
    for (int z = 0; z < 6; z++) for (int y = AY; y < AY + 3; y++) { setB(10, y, z, STONE); }
    { Phys P = mk(10.3, AY, 3.5); CHECK(overlaps(P)); const bool m = unstuckPlayer(P); CHECK(m); for (int i = 0; i < 40 && unstuckPlayer(P); i++) {} CHECK(!overlaps(P)); CHECK(fabs(P.x - 9.7) < .11); CHECK(fabs(P.y - AY) < 1e-6); }
    SECTION("unstuck: nao mexe quando livre, no espectador ou com chunk nao carregado");
    { Phys P = mk(2.5, AY, 2.5); CHECK(!unstuckPlayer(P)); CHECK(P.x == 2.5 && P.y == AY && P.z == 2.5); }
    { Phys P = mk(10.5, AY, 3.5); P.noclip = true; CHECK(!unstuckPlayer(P)); CHECK(P.x == 10.5); }
    { Phys P = mk(10000.5, AY, 10000.5); CHECK(!unstuckPlayer(P)); CHECK(P.x == 10000.5); }
    SECTION("unstuck: enterrado fundo sobe pela coluna");
    arena();
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY; y < AY + 8; y++) setB(x, y, z, STONE);
    { Phys P = mk(5.5, AY + 3, 5.5); CHECK(overlaps(P)); CHECK(unstuckPlayer(P)); CHECK(!overlaps(P)); CHECK(P.y >= AY + 8 - 1e-6 && P.y < AY + 8.2); }
    SECTION("unstuck: encostar no teto nao e sobrepor");
    arena();
    setB(4, AY + 2, 4, STONE);
    { Phys P = mk(4.5, AY, 4.5); CHECK(!overlaps(P)); CHECK(!unstuckPlayer(P)); }
    SECTION("bloco colocado em cima do jogador");
    arena();
    { Phys P = mk(8.5, AY, 8.5); setB(8, AY, 8, STONE); setB(8, AY + 1, 8, STONE); CHECK(unstuckPlayer(P)); for (int i = 0; i < 40 && unstuckPlayer(P); i++) {} CHECK(!overlaps(P)); moveEntity(P, 0, -.1, 0); CHECK(!overlaps(P)); }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
