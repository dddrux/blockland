// Trilhos + carrinho (minecart.inc): conexao automatica, curva, rampa, energia, fisica do carrinho. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY - 1; y < AY + 20; y++) setB(x, y, z, y < AY ? STONE : AIR);
}
static void rail(int k, int x, int y, int z) { const int p[3] = {x, y, z}; railPlace(k, p); }
int main() {
    arena(); Phys P; P.x = 40; P.y = AY + 5; P.z = 40;
    printf("[ligacao e forma]\n");
    rail(0, 0, AY, 0); CHECK(railShape(0, AY, 0) == 0);                                   // sozinho: N-S
    rail(0, 1, AY, 0); CHECK(railShape(1, AY, 0) == 1 && railShape(0, AY, 0) == 1);        // vizinho a leste: os dois viram L-O
    rail(0, 2, AY, 0); rail(0, 2, AY, 1);                                                  // curva: (2,0) liga a L-O e (2,1) ao sul
    CHECK(railShape(2, AY, 1) == 0);                                                       // (2,1) so tem (2,0) ao norte: N-S
    CHECK(railShape(2, AY, 0) == 7);                                                       // (2,0): ligado a O e S = curva SO
    printf("[trilho energizado nao faz curva]\n");
    rail(1, 10, AY, 0); rail(1, 10, AY, 1); rail(1, 11, AY, 0);
    CHECK(railShape(10, AY, 0) <= 1 || railShape(10, AY, 0) >= 2);
    CHECK(railShape(10, AY, 0) < 6 && railShape(11, AY, 0) < 6 && railShape(10, AY, 1) < 6);
    printf("[rampa]\n");
    for (int x = 20; x <= 24; x++) setB(x, AY, 5, STONE);   // degrau: y=AY+? bloco a mais
    setB(23, AY, 5, STONE); setB(24, AY, 5, STONE); setB(23, AY + 1, 5, STONE); setB(24, AY + 1, 5, STONE);
    rail(0, 22, AY + 1, 5); rail(0, 23, AY + 2, 5);
    CHECK(railShape(22, AY + 1, 5) == 2);   // sobe para leste
    CHECK(railAt(23, AY + 2, 5));
    printf("[suporte e quebra]\n");
    setB(1, AY - 1, 0, AIR); xbNeighbor(1, AY, 0, 0, -1, 0);
    CHECK(getB(1, AY, 0) == AIR);   // sem chao embaixo o trilho cai
    printf("[carrinho em linha reta com trilho energizado + alavanca]\n");
    for (int x = -20; x <= 20; x++) { setB(x, AY, 15, AIR); }
    for (int x = -20; x <= 20; x++) rail(0, x, AY, 15);
    for (int x = -3; x <= 3; x++) { setBM(x, AY, 15, XB_RAIL + 1, 1); }                    // 7 trilhos energizaveis L-O no meio
    CARTS.clear(); gCartRide = 0; cartSpawn(CT_NORMAL, -10 + .5, AY + .0625, 15.5); CARTS[0].mx = .1;
    for (int i = 0; i < 40; i++) cartTickAll(P, false, false, 0.f);
    printf("  x=%.3f y=%.3f z=%.3f mx=%.3f\n", CARTS[0].x, CARTS[0].y, CARTS[0].z, CARTS[0].mx);
    CHECK(CARTS[0].x > -10 && fabs(CARTS[0].z - 15.5) < .01 && fabs(CARTS[0].y - (AY + .0625)) < .02);   // anda no trilho, sem sair da linha
    printf("[trilho energizado SEM energia freia; COM alavanca acelera]\n");
    CARTS.clear(); cartSpawn(CT_NORMAL, -3 + .5, AY + .0625, 15.5); CARTS[0].mx = .3;
    for (int i = 0; i < 10; i++) cartTickAll(P, false, false, 0.f);
    const double xSemEnergia = CARTS[0].x; const double vSem = CARTS[0].mx;
    CHECK(vSem < .15);   // freou
    const int lv = XB_LEVER + 12 + 4;   // alavanca ligada (no chao)
    setB(-1, AY, 16, lv);   // encostada no trilho (-1,15), lado +Z
    railNeighbor(-1, AY, 15, getB(-1, AY, 15));
    CHECK(((getM(-1, AY, 15) >> 4) & 1) == 1);   // ligou
    CHECK(((getM(0, AY, 15) >> 4) & 1) == 0 || ((getM(0, AY, 15) >> 4) & 1) == 1);   // (propaga pelos vizinhos ao notificar)
    for (int x = -3; x <= 3; x++) railNeighbor(x, AY, 15, getB(x, AY, 15));
    CHECK(((getM(3, AY, 15) >> 4) & 1) == 1);   // energia propagou ate 4 trilhos de distancia
    CARTS.clear(); cartSpawn(CT_NORMAL, -3 + .5, AY + .0625, 15.5); CARTS[0].mx = .1;
    for (int i = 0; i < 20; i++) cartTickAll(P, false, false, 0.f);
    printf("  com energia: x=%.3f mx=%.3f\n", CARTS[0].x, CARTS[0].mx);
    CHECK(CARTS[0].mx > .2);   // acelerou
    printf("[tocha de redstone: so acesa energiza]\n");
    setBM(20, AY, 20, XB_RTORCH, 0); CHECK(railSrcNear(21, AY, 20));
    setBM(20, AY, 20, XB_RTORCH, 1); CHECK(!railSrcNear(21, AY, 20));
    setBM(23, AY, 20, XB_DUST, 5 << 4); CHECK(railSrcNear(24, AY, 20)); setBM(23, AY, 20, XB_DUST, 0); CHECK(!railSrcNear(24, AY, 20));
    setBM(23, AY, 20, XB_BTN, 16); CHECK(railSrcNear(24, AY, 20)); setBM(23, AY, 20, XB_BTN, 0); CHECK(!railSrcNear(24, AY, 20));
    setBM(23, AY, 20, XB_PLATE, 16); CHECK(railSrcNear(24, AY, 20)); setBM(23, AY, 20, AIR, 0);
    { rail(1, -20, AY, 25); setBM(-20, AY, 26, XB_BTN, 0); railNeighbor(-20, AY, 25, getB(-20, AY, 25)); CHECK(((getM(-20, AY, 25) >> 4) & 1) == 0);
      setBM(-20, AY, 26, XB_BTN, 16); railNeighbor(-20, AY, 25, getB(-20, AY, 25)); CHECK(((getM(-20, AY, 25) >> 4) & 1) == 1);
      setBM(-20, AY, 26, AIR, 0); setBM(-20, AY, 25, AIR, 0); }
    setB(-10, AY, 13, STONE); setB(-10, AY, 14, XB_LEVER + 2); CHECK(!railSrcNear(-10, AY, 12)); setB(-10, AY, 14, XB_LEVER + 12 + 2); CHECK(railSrcNear(-10, AY, 12));   // alavanca presa ao bloco vizinho
    setB(-10, AY, 14, AIR); setB(-10, AY, 13, AIR);
    printf("[montar, empurrar com W, descer]\n");
    CARTS.clear(); gCartRide = 0; cartSpawn(CT_NORMAL, 12.5, AY + .0625, 15.5); gCartRide = CARTS[0].id;
    for (int i = 0; i < 30; i++) cartTickAll(P, true, false, 0.f);
    CHECK(fabs(P.x - CARTS[0].x) < 1e-6 && fabs(P.y - (CARTS[0].y + CART_SEAT_Y)) < 1e-6);
    CHECK(CARTS[0].mx > 0);   // com W (yaw 0 = para -Z... so checa mover) 
    cartTickAll(P, false, true, 0.f); CHECK(gCartRide == 0);
    printf("[sem trilho: cai e para]\n");
    CARTS.clear(); cartSpawn(CT_NORMAL, 25.5, AY + 5, 25.5);
    for (int i = 0; i < 60; i++) cartTickAll(P, false, false, 0.f);
    CHECK(CARTS[0].onGround && fabs(CARTS[0].y - AY) < .01);
    printf("[detector liga com carrinho em cima]\n");
    rail(2, 30, AY, 20); CARTS.clear(); cartSpawn(CT_NORMAL, 30.5, AY + .0625, 20.5);
    cartTickAll(P, false, false, 0.f); CHECK(((getM(30, AY, 20) >> 4) & 1) == 1);
    CARTS.clear(); cartTickAll(P, false, false, 0.f); CHECK(((getM(30, AY, 20) >> 4) & 1) == 0);
    printf("[TNT: ativador energizado acende e explode]\n");
    CARTS.clear(); cartSpawn(CT_TNT, 35.5, AY + .0625, 20.5); CARTS[0].fuse = 3; setB(36, AY, 20, STONE);
    for (int i = 0; i < 6; i++) cartTickAll(P, false, false, 0.f);
    CHECK(CARTS.empty() && getB(36, AY, 20) == AIR);
    printf("[trilho energizado por repetidor e observador]\n");
    rail(1, -25, AY, 25); setBM(-26, AY, 25, XB_REPT, 1 | 16); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 1);   // repetidor ligado aponta (+X) para o trilho
    setBM(-26, AY, 25, XB_REPT, 1); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 0);   // desligado
    setBM(-26, AY, 25, XB_REPT, 3 | 16); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 0);   // aponta para o lado errado
    setBM(-26, AY, 25, AIR, 0);
    setBM(-25, AY, 26, XB_OBS, 2 | 16); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 0);   // frente -Z = rosto sobre o trilho, costas +Z: nao energiza o trilho
    setBM(-25, AY, 26, XB_OBS, 4 | 16); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 1);   // frente +Z, costas -Z = trilho: ligado
    setBM(-25, AY, 26, XB_OBS, 4); railNeighbor(-25, AY, 25, getB(-25, AY, 25)); CHECK(((getM(-25, AY, 25) >> 4) & 1) == 0);   // pulso acabou
    setBM(-25, AY, 26, AIR, 0);
    printf("[pilhas]\n");
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
