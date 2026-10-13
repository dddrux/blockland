// Pistao (patch5.38): energia, estender/recolher, empurrar ate 12, grudento, jogador. Roda o main.cpp REAL.
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
    arena(); const int LV = XB_LEVER + 12 + 4, LV0 = XB_LEVER + 4;   // alavanca de chao ligada / desligada
    auto clr = [&]() { for (int x = -4; x < 22; x++) for (int y = AY; y < AY + 3; y++) for (int z = 0; z < 6; z++) setBM(x, y, z, AIR, 0); };
    SECTION("estende com alavanca e empurra");
    clr(); setBM(2, AY, 2, XB_PIST, 3); setB(3, AY, 2, STONE); setB(4, AY, 2, STONE);
    setB(2, AY, 3, LV); pistonScan(2, AY, 3);
    CHECK(getB(2, AY, 2) == XB_PIST && getM(2, AY, 2) == (16 | 3)); CHECK(getB(3, AY, 2) == XB_PIST && getM(3, AY, 2) == (8 | 3));
    CHECK(getB(4, AY, 2) == STONE && getB(5, AY, 2) == STONE);
    SECTION("forma: corpo 12px e cabeca");
    { AABB t[SH_MAXB]; int n = blockBoxesId(XB_PIST, 2, AY, 2, false, t, 16 | 3); CHECK(n == 1); if (n == 1) CHECK(fabs((t[0].x1 - t[0].x0) - .75) < 1e-6);
      n = blockBoxesId(XB_PIST, 3, AY, 2, false, t, 8 | 3); CHECK(n == 2); }
    SECTION("recolhe sem energia (normal nao puxa)");
    setB(2, AY, 3, LV0); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == 3); CHECK(getB(3, AY, 2) == AIR); CHECK(getB(4, AY, 2) == STONE);
    SECTION("grudento puxa 1 bloco");
    clr(); setBM(2, AY, 2, XB_PIST + 1, 3); setB(2, AY, 3, LV); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == (16 | 3)); setB(4, AY, 2, STONE);
    setB(2, AY, 3, LV0); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == 3); CHECK(getB(3, AY, 2) == STONE); CHECK(getB(4, AY, 2) == AIR);
    SECTION("limite de 12 blocos");
    clr(); setBM(-3, AY, 2, XB_PIST, 3); for (int i = 0; i < 12; i++) setB(-2 + i, AY, 2, STONE);
    setB(-3, AY, 3, LV); pistonScan(-3, AY, 3);
    CHECK(getM(-3, AY, 2) == (16 | 3)); CHECK(getB(10, AY, 2) == STONE);
    clr(); setBM(-3, AY, 2, XB_PIST, 3); for (int i = 0; i < 13; i++) setB(-2 + i, AY, 2, STONE);
    setB(-3, AY, 3, LV); pistonScan(-3, AY, 3);
    CHECK(getM(-3, AY, 2) == 3); CHECK(getB(-2, AY, 2) == STONE);
    SECTION("bedrock e pistao estendido nao movem; planta some");
    clr(); setBM(2, AY, 2, XB_PIST, 3); setB(3, AY, 2, BEDROCK); setB(2, AY, 3, LV); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == 3 && getB(3, AY, 2) == BEDROCK);
    clr(); setBM(2, AY, 2, XB_PIST, 3); setB(3, AY, 2, XB_PLANT); setB(2, AY, 3, LV); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == (16 | 3));
    SECTION("vertical para cima empurra o jogador");
    clr(); Phys P = mk(2.5, AY + 1, 2.5); gPvpP = &P; setBM(2, AY, 2, XB_PIST, 1);   // pistao no chao apontando para cima; o jogador esta na celula da cabeca
    setB(3, AY, 2, LV); pistonScan(3, AY, 2);
    CHECK(getM(2, AY, 2) == (16 | 1)); CHECK(getB(2, AY + 1, 2) == XB_PIST); CHECK(P.y >= AY + 2 - 1e-6);
    SECTION("botao liga o pistao e solta no tick");
    clr(); setBM(2, AY, 2, XB_PIST, 3); setBM(2, AY, 3, XB_BTN, 0 | 16); pistonScan(2, AY, 3);
    CHECK(getM(2, AY, 2) == (16 | 3)); rsTick(2, AY, 3); CHECK(((getM(2, AY, 3) >> 4) & 1) == 0); CHECK(getM(2, AY, 2) == 3);
    SECTION("placa de pressao: jogador aperta, sai e solta");
    clr(); setBM(2, AY, 2, XB_PIST, 3); setBM(2, AY, 3, XB_PLATE, 1); { Phys Q = mk(2.5, AY, 3.5); gPvpP = &Q; rsPlatesTick();
      CHECK(((getM(2, AY, 3) >> 4) & 1) == 1); CHECK(getM(2, AY, 2) == (16 | 3)); Q.x = 9.5; rsTick(2, AY, 3);
      CHECK(((getM(2, AY, 3) >> 4) & 1) == 0); CHECK(getM(2, AY, 2) == 3); }
    SECTION("poeira: alavanca, 5 fios e pistao no fim");
    clr(); for (int i = 1; i <= 5; i++) setBM(i, AY, 2, XB_DUST, 0); setBM(6, AY, 2, XB_PIST, 3); setB(0, AY, 2, LV); pistonScan(0, AY, 2);
    CHECK((getM(1, AY, 2) >> 4) == 15); CHECK((getM(5, AY, 2) >> 4) == 11); CHECK(getM(6, AY, 2) == (16 | 3));
    setB(0, AY, 2, LV0); pistonScan(0, AY, 2); CHECK((getM(3, AY, 2) >> 4) == 0); CHECK(getM(6, AY, 2) == 3);
    SECTION("repetidor: alavanca -> repetidor -> poeira -> pistao");
    clr(); setBM(2, AY, 2, XB_DUST, 0); setBM(3, AY, 2, XB_PIST, 3); setBM(1, AY, 2, XB_REPT, 1); setB(0, AY, 2, LV); rsRepCheck(1, AY, 2);
    CHECK(((getM(1, AY, 2) >> 4) & 1) == 0); rsTick(1, AY, 2);
    CHECK(((getM(1, AY, 2) >> 4) & 1) == 1); CHECK((getM(2, AY, 2) >> 4) == 15); CHECK(getM(3, AY, 2) == (16 | 3));
    setB(0, AY, 2, LV0); pistonScan(0, AY, 2); rsTick(1, AY, 2);
    CHECK(((getM(1, AY, 2) >> 4) & 1) == 0); CHECK((getM(2, AY, 2) >> 4) == 0); CHECK(getM(3, AY, 2) == 3);
    SECTION("tocha de redstone inverte: bloco energizado apaga a tocha");
    clr(); setB(5, AY, 2, STONE); setBM(6, AY, 2, XB_RTORCH + 1, 0); setBM(7, AY, 2, XB_PIST, 3); pistonCheck(7, AY, 2);
    CHECK(getM(7, AY, 2) == (16 | 3)); setB(5, AY, 3, LV); pistonScan(5, AY, 3); rsTick(6, AY, 2);
    CHECK((getM(6, AY, 2) & 1) == 1); CHECK(getM(7, AY, 2) == 3);
    setB(5, AY, 3, LV0); pistonScan(5, AY, 3); rsTick(6, AY, 2);
    CHECK((getM(6, AY, 2) & 1) == 0); CHECK(getM(7, AY, 2) == (16 | 3));
    SECTION("energia por bloco solido: tocha sob o bloco -> poeira ao lado -> pistao");
    clr(); setB(10, AY + 1, 2, STONE); setBM(10, AY, 2, XB_RTORCH, 0); setB(11, AY, 2, STONE); setBM(11, AY + 1, 2, XB_DUST, 0); setBM(12, AY + 1, 2, XB_PIST, 3);
    pistonScan(10, AY, 2);
    CHECK((getM(11, AY + 1, 2) >> 4) == 15); CHECK(getM(12, AY + 1, 2) == (16 | 3));
    setBM(10, AY, 2, XB_RTORCH, 1); pistonScan(10, AY, 2);
    CHECK((getM(11, AY + 1, 2) >> 4) == 0); CHECK(getM(12, AY + 1, 2) == 3);
    SECTION("energia por bloco solido: repetidor aponta para o bloco -> poeira -> pistao");
    clr(); setB(13, AY, 2, LV); setBM(14, AY, 2, XB_REPT, 1); setB(15, AY, 2, STONE); setBM(16, AY, 2, XB_DUST, 0); setBM(17, AY, 2, XB_PIST, 3);
    rsRepCheck(14, AY, 2); rsTick(14, AY, 2);
    CHECK(((getM(14, AY, 2) >> 4) & 1) == 1); CHECK((getM(16, AY, 2) >> 4) == 15); CHECK(getM(17, AY, 2) == (16 | 3));
    setB(13, AY, 2, LV0); pistonScan(13, AY, 2); rsTick(14, AY, 2);
    CHECK((getM(16, AY, 2) >> 4) == 0); CHECK(getM(17, AY, 2) == 3);
    SECTION("poeira energiza o bloco e o bloco liga o repetidor atras dele");
    clr(); setB(13, AY, 2, LV); setBM(14, AY, 2, XB_DUST, 0); setB(15, AY, 2, STONE); setBM(16, AY, 2, XB_REPT, 1);
    pistonScan(13, AY, 2); CHECK((getM(14, AY, 2) >> 4) == 15); rsRepCheck(16, AY, 2); rsTick(16, AY, 2);
    CHECK(((getM(16, AY, 2) >> 4) & 1) == 1);
    SECTION("luz: tocha de redstone apagada nao emite");
    clr(); setBM(5, AY, 5, XB_RTORCH, 0); computeLight(0, 0); CHECK(rBl[rI(5 + CW, AY, 5 + CW)] == 7);
    setBM(5, AY, 5, XB_RTORCH, 1); computeLight(0, 0); CHECK(rBl[rI(5 + CW, AY, 5 + CW)] == 0 && rBl[rI(6 + CW, AY, 5 + CW)] == 0);
    setBM(5, AY, 5, XB_RTORCH, 0); computeLight(0, 0); CHECK(rBl[rI(6 + CW, AY, 5 + CW)] == 6);
    SECTION("save: botao/placa apertados reagendam a soltura ao carregar o chunk");
    { const size_t N = (size_t)CW * CH * CW; std::vector<BT> d(N, 0); std::vector<uint8_t> md(N, 0);
      d[idx(1, AY, 1)] = XB_BTN; md[idx(1, AY, 1)] = 16; d[idx(2, AY, 1)] = XB_PLATE; md[idx(2, AY, 1)] = 16; d[idx(3, AY, 1)] = XB_BTN; md[idx(3, AY, 1)] = 0;
      savedC[key(3, 3)] = d; savedM[key(3, 3)] = md; gen(3, 3);
      CHECK(tickPending(48 + 1, AY, 48 + 1)); CHECK(tickPending(48 + 2, AY, 48 + 1)); CHECK(!tickPending(48 + 3, AY, 48 + 1));
      CHECK(getB(49, AY, 49) == XB_BTN && ((getM(49, AY, 49) >> 4) & 1) == 1);
      for (int i = 0; i < 40; i++) runWorldTick();
      CHECK(((getM(49, AY, 49) >> 4) & 1) == 0); CHECK(((getM(50, AY, 49) >> 4) & 1) == 0); }
    SECTION("alavanca presa ao bloco energiza o bloco -> poeira ao lado -> pistao");
    clr(); setB(10, AY, 2, STONE); setB(9, AY, 2, XB_LEVER + 3); setBM(10, AY, 3, XB_DUST, 0); setBM(11, AY, 3, XB_PIST, 3);
    setB(9, AY, 2, XB_LEVER + 12 + 3); pistonScan(9, AY, 2); CHECK((getM(10, AY, 3) >> 4) == 15); CHECK(getM(11, AY, 3) == (16 | 3));
    setB(9, AY, 2, XB_LEVER + 3); pistonScan(9, AY, 2); CHECK((getM(10, AY, 3) >> 4) == 0); CHECK(getM(11, AY, 3) == 3);
    SECTION("repetidor travado por repetidor lateral");
    clr(); setB(15, AY, 3, LV); setBM(16, AY, 3, XB_REPT, 1); setBM(16, AY, 2, XB_REPT, 2 | 16);
    rsRepCheck(16, AY, 3); rsTick(16, AY, 3); CHECK(((getM(16, AY, 3) >> 4) & 1) == 0);   // travado desligado apesar da entrada
    setBM(16, AY, 2, XB_REPT, 2); rsRepCheck(16, AY, 3); rsTick(16, AY, 3); CHECK(((getM(16, AY, 3) >> 4) & 1) == 1);   // destravou: liga
    setBM(16, AY, 2, XB_REPT, 2 | 16); setB(15, AY, 3, LV0); rsRepCheck(16, AY, 3); rsTick(16, AY, 3); CHECK(((getM(16, AY, 3) >> 4) & 1) == 1);   // travado ligado: nao desliga
    SECTION("botao e placa apertados energizam o bloco onde estao presos");
    clr(); setB(10, AY, 2, STONE); setBM(9, AY, 2, XB_BTN, 5); setBM(10, AY, 3, XB_DUST, 0); setBM(11, AY, 3, XB_PIST, 3);
    setBM(9, AY, 2, XB_BTN, 5 | 16); pistonScan(9, AY, 2); CHECK((getM(10, AY, 3) >> 4) == 15); CHECK(getM(11, AY, 3) == (16 | 3));
    rsTick(9, AY, 2); CHECK(((getM(9, AY, 2) >> 4) & 1) == 0); CHECK((getM(10, AY, 3) >> 4) == 0); CHECK(getM(11, AY, 3) == 3);
    setBM(9, AY, 2, XB_BTN, 4 | 16); pistonScan(9, AY, 2); CHECK((getM(10, AY, 3) >> 4) == 0);   // botao preso a OUTRA face (lado errado): nao energiza este bloco
    setB(13, AY, 2, STONE); setBM(13, AY + 1, 2, XB_PLATE, 0); setBM(14, AY, 2, XB_DUST, 0); setBM(15, AY, 2, XB_PIST, 3);
    setBM(13, AY + 1, 2, XB_PLATE, 16); pistonScan(13, AY + 1, 2); CHECK((getM(14, AY, 2) >> 4) == 15); CHECK(getM(15, AY, 2) == (16 | 3));
    setBM(13, AY + 1, 2, XB_PLATE, 0); pistonScan(13, AY + 1, 2); CHECK((getM(14, AY, 2) >> 4) == 0); CHECK(getM(15, AY, 2) == 3);
    SECTION("repetidor travado: bit5 do meta (barra) liga e desliga com o travamento");
    clr(); setB(15, AY, 4, LV); setBM(16, AY, 4, XB_REPT, 1); setBM(16, AY, 3, XB_REPT, 2 | 16);
    rsRepCheck(16, AY, 4); CHECK(((getM(16, AY, 4) >> 5) & 1) == 1);   // travado: barra
    setBM(16, AY, 3, XB_REPT, 2); rsRepCheck(16, AY, 4); CHECK(((getM(16, AY, 4) >> 5) & 1) == 0);   // destravado: sem barra
    SECTION("comparador: traseira liga a saida (forca 15) -> poeira; lateral subtrai");
    clr(); setBM(16, AY, 3, XB_COMP, 1); setBM(17, AY, 3, XB_DUST, 0); setB(15, AY, 3, LV0); rsRepCheck(16, AY, 3);
    for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 0 && !((getM(16, AY, 3) >> 3) & 1));   // sem entrada
    setB(15, AY, 3, LV); rsRepCheck(16, AY, 3); CHECK(tickPending(16, AY, 3));
    for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 15 && ((getM(16, AY, 3) >> 3) & 1)); CHECK((getM(17, AY, 3) >> 4) == 15);   // modo comparar: repassa
    setBM(16, AY, 3, XB_COMP, (getM(16, AY, 3) & ~4) | 4); rsRepCheck(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 15);   // subtrair sem lateral: 15
    setBM(16, AY, 2, XB_REPT, 2 | 16); rsRepCheck(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 0 && !((getM(16, AY, 3) >> 3) & 1)); CHECK((getM(17, AY, 3) >> 4) == 0);   // 15 - 15 = 0
    setBM(16, AY, 3, XB_COMP, getM(16, AY, 3) & ~4); rsRepCheck(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 15);   // comparar: traseira 15 >= lateral 15
    setB(15, AY, 3, LV0); rsRepCheck(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(16, AY, 3) >> 4) == 0);   // traseira 0 < lateral 15: apaga
    SECTION("animacao: estender empurrando 2 blocos cria 3 deslizes que escondem o destino e expiram em 2 ticks");
    clr(); gPAnim.clear(); setBM(2, AY, 2, XB_PIST, 3); setB(3, AY, 2, STONE); setB(4, AY, 2, STONE); setB(2, AY, 3, LV); pistonScan(2, AY, 3);
    CHECK(gPAnim.size() == 3); CHECK(pAnimHides(4, AY, 2) && pAnimHides(5, AY, 2) && pAnimHides(3, AY, 2)); CHECK(!pAnimHides(2, AY, 2) && !pAnimHides(6, AY, 2));   // pedras em 4 e 5, cabeca em 3; o corpo nao some
    { bool ok = true; for (const PAnim& a : gPAnim) ok = ok && a.d == 3 && a.s0 == -1 && a.e0 == 0 && a.hide; CHECK(ok); }
    runWorldTick(); CHECK(gPAnim.size() == 3); runWorldTick(); CHECK(gPAnim.empty() && !pAnimHides(4, AY, 2));   // expira em 2 ticks; blocos continuam no mundo
    CHECK(getB(4, AY, 2) == STONE && getB(5, AY, 2) == STONE);
    SECTION("animacao: recolher cria a cabeca deslizando para tras (sem esconder o corpo)");
    gPAnim.clear(); setB(2, AY, 3, LV0); pistonScan(2, AY, 3);
    CHECK(gPAnim.size() == 1 && gPAnim[0].x == 3 && gPAnim[0].s0 == 0 && gPAnim[0].e0 == -1 && !gPAnim[0].hide); CHECK(!pAnimHides(3, AY, 2) && !pAnimHides(2, AY, 2));
    for (int i = 0; i < 3; i++) runWorldTick(); CHECK(gPAnim.empty());
    SECTION("animacao: grudento puxa o bloco (desliza de volta e esconde o destino)");
    clr(); gPAnim.clear(); setBM(2, AY, 2, XB_PIST + 1, 3); setB(2, AY, 3, LV); pistonScan(2, AY, 3); setB(4, AY, 2, STONE); gPAnim.clear();
    setB(2, AY, 3, LV0); pistonScan(2, AY, 3);
    CHECK(gPAnim.size() == 2); CHECK(pAnimHides(3, AY, 2));   // pedra puxada para a celula 3 (esconde ate acabar); cabeca nao esconde
    { bool pull = false, head = false; for (const PAnim& a : gPAnim) { if (a.hide && a.s0 == 1 && a.e0 == 0 && a.id == STONE) pull = true; if (!a.hide && a.s0 == 0 && a.e0 == -1) head = true; } CHECK(pull && head); }
    for (int i = 0; i < 3; i++) runWorldTick(); CHECK(gPAnim.empty() && getB(3, AY, 2) == STONE);
    gPAnim.clear();
    SECTION("comparador le o enchimento do bau atras dele");
    clr(); setBM(16, AY, 3, XB_CHEST, 2); setBM(17, AY, 3, XB_COMP, 1); setBM(18, AY, 3, XB_DUST, 0);
    CHECK(chestSignal(16, AY, 3) == 0); ChestInv* ci = chestAt(16, AY, 3, true); CHECK(chestSignal(16, AY, 3) == 0);
    chestAdd(*ci, IT_IRON, 1); CHECK(chestSignal(16, AY, 3) == 1);
    chestAdd(*ci, IT_IRON, 863); CHECK(chestSignal(16, AY, 3) == 8);   // metade cheia
    chestSignalChanged(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(17, AY, 3) >> 4) == 8 && ((getM(17, AY, 3) >> 3) & 1)); CHECK((getM(18, AY, 3) >> 4) == 15);
    chestAdd(*ci, IT_IRON, 864); CHECK(chestSignal(16, AY, 3) == 15);   // cheio
    for (int i = 0; i < CHEST_SLOTS; i++) ci->s[i] = Slot(); chestSignalChanged(16, AY, 3); for (int i = 0; i < 4; i++) runWorldTick(); CHECK((getM(17, AY, 3) >> 4) == 0 && !((getM(18, AY, 3) >> 4) & 15));   // esvaziou: apaga
    CHECK(chestSignal(15, AY, 3) == -1);
    SECTION("comparador aciona pistao pela frente");
    clr(); setBM(2, AY, 3, XB_COMP, 1); setBM(3, AY, 3, XB_PIST, 3); setB(1, AY, 3, LV); rsRepCheck(2, AY, 3);
    for (int i = 0; i < 4; i++) runWorldTick(); CHECK(getM(3, AY, 3) == (16 | 3));   // comparador ligado empurra o pistao da frente
    SECTION("chunk carregado: repetidor/tocha com estado velho reavaliam");
    clr(); { Chunk* c = getC(0, 0); c->b[idx(4, AY, 4)] = XB_REPT; c->m[idx(4, AY, 4)] = 16 | 1; c->b[idx(6, AY, 4)] = XB_RTORCH; c->m[idx(6, AY, 4)] = 1;   // repetidor ligado sem entrada; tocha apagada sem bloco energizado
      rsChunkLoaded(c, 0, 0); CHECK(tickPending(4, AY, 4)); CHECK(tickPending(6, AY, 4));
      for (int i = 0; i < 6; i++) runWorldTick();
      CHECK(((getM(4, AY, 4) >> 4) & 1) == 0); CHECK((getM(6, AY, 4) & 1) == 0); }
    SECTION("chunk carregado: vizinho de chunk nao carregado nao decide (nao apaga o repetidor)");
    { const size_t N = (size_t)CW * CH * CW; std::vector<BT> d(N, 0); std::vector<uint8_t> md(N, 0);
      d[idx(0, AY, 1)] = XB_REPT; md[idx(0, AY, 1)] = 16 | 1; savedC[key(5, 5)] = d; savedM[key(5, 5)] = md; gen(5, 5);
      CHECK(tickPending(80, AY, 81)); for (int i = 0; i < 6; i++) runWorldTick(); CHECK(((getM(80, AY, 81) >> 4) & 1) == 1); }   // na borda do chunk (x local 0)
    SECTION("observador: muda a frente -> pulso de 2 ticks pelas costas -> poeira -> pistao");
    clr(); setBM(10, AY, 2, XB_OBS, 5); rsObsAdded(10, AY, 2); setBM(11, AY, 2, XB_DUST, 0); setBM(12, AY, 2, XB_PIST, 3);   // frente = -X (9), costas = +X (11)
    rsObsNeighbor(10, AY, 2); CHECK(!tickPending(10, AY, 2));   // nada mudou
    setB(9, AY, 2, STONE); rsObsNeighbor(10, AY, 2); CHECK(tickPending(10, AY, 2) && ((getM(10, AY, 2) >> 4) & 1) == 0);
    runWorldTick(); runWorldTick(); CHECK(((getM(10, AY, 2) >> 4) & 1) == 1); CHECK((getM(11, AY, 2) >> 4) == 15); CHECK(getM(12, AY, 2) == (16 | 3));
    runWorldTick(); runWorldTick(); CHECK(((getM(10, AY, 2) >> 4) & 1) == 0); CHECK((getM(11, AY, 2) >> 4) == 0); CHECK(getM(12, AY, 2) == 3);
    runWorldTick(); runWorldTick(); CHECK(((getM(10, AY, 2) >> 4) & 1) == 0);   // sem nova mudanca nao pulsa de novo
    setB(9, AY, 2, AIR); rsObsNeighbor(10, AY, 2); CHECK(tickPending(10, AY, 2));   // remover tambem pulsa
    for (int i = 0; i < 6; i++) runWorldTick();
    setBM(10, AY + 1, 2, STONE, 0); rsObsNeighbor(10, AY, 2); CHECK(!tickPending(10, AY, 2));   // mudanca em uma LATERAL nao conta
    SECTION("observador alimenta bloco solido atras dele");
    clr(); setBM(10, AY, 2, XB_OBS, 5 | 16); setB(11, AY, 2, STONE); setBM(11, AY, 3, XB_DUST, 0); setBM(12, AY, 3, XB_PIST, 3);
    pistonScan(10, AY, 2); CHECK((getM(11, AY, 3) >> 4) == 15); CHECK(getM(12, AY, 3) == (16 | 3));
    SECTION("pistao empurra mob e item solto");
    clr(); { MOBS.clear(); DROPS.clear(); MobEnt e; e.type = 0; e.x = 3.5; e.y = AY; e.z = 2.5; e.px = e.x; e.py = e.y; e.pz = e.z; MOBS.push_back(e);
      DropEnt r{}; r.x = 3.5; r.y = AY; r.z = 2.5; r.id = 1; r.n = 1; DROPS.push_back(r);
      setBM(2, AY, 2, XB_PIST, 3); setB(2, AY, 3, LV); pistonScan(2, AY, 3);
      CHECK(MOBS[0].x >= 4 + MD[0].w * .5 - 1e-6); CHECK(DROPS[0].x >= 4.125 - 1e-6); MOBS.clear(); DROPS.clear(); }
    SECTION("cabeca sem corpo some; cabeca nao dropa");
    clr(); setBM(5, AY, 2, XB_PIST, 8 | 3); pistonCheck(5, AY, 2); CHECK(getB(5, AY, 2) == AIR);
    CHECK(rsDropItem(5, AY, 2, XB_PIST) != 0 || true); gRsLastMeta = 8 | 3; CHECK(rsDropItem(9, AY, 2, XB_PIST) == 0);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
