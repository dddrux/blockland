// patchEndGate.1: gateways do End (headless: so o mundo, sem janela). Build: tests/run_endgate.sh
#define main game_main
#include "../src/main.cpp"
#undef main
static int nF = 0, nT = 0;
#define CHK(c) do { nT++; if (!(c)) { nF++; printf("FALHA %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
int main() {
    // posicoes: 20 distintas no anel r~96; ordem = permutacao
    { bool seen[GATE_N] = {}; int cnt = 0; std::set<std::pair<int,int>> ps;
      for (int k = 0; k < GATE_N; k++) { const int i = gateOrder(k); CHK(i >= 0 && i < GATE_N); if (!seen[i]) cnt++; seen[i] = true; int x, z; gatePos(i, x, z); ps.insert({x, z}); const double r = sqrt((double)(x * x + z * z)); CHK(r > 94.5 && r < 97.6); }
      CHK(cnt == GATE_N); CHK((int)ps.size() == GATE_N); int x0, z0; gatePos(0, x0, z0); CHK(x0 == 96 && z0 == 0); }
    dimSwitch(2); CHK(gDim == 2);
    for (int k = 0; k < GATE_N; k++) { int x, z; gatePos(k, x, z); if (!getC(fdiv(x, CW), fdiv(z, CW))) gen(fdiv(x, CW), fdiv(z, CW)); }
    // 1 gateway por morte; 21a nao cria
    std::set<std::pair<int,int>> made;
    for (int n = 1; n <= GATE_N; n++) { CHK(endGateSpawn()); int c = 0; for (int i = 0; i < GATE_N; i++) { int x, z; gatePos(i, x, z); if (gateIsCell(getB(x, GATE_Y, z), getM(x, GATE_Y, z))) c++; } CHK(c == n); }
    CHK(!endGateSpawn());
    { int x, z; gatePos(5, x, z); CHK(getB(x, GATE_Y + 1, z) == XB_PORTAL && getM(x, GATE_Y + 1, z) == 10); CHK(getB(x, GATE_Y - 1, z) == BEDROCK && getB(x, GATE_Y - 2, z) == BEDROCK); CHK(getB(x, GATE_Y + 2, z) == AIR); }
    // celula de portal do End normal (meta 2) NAO e gateway; a de gateway (10) nao e portal do Nether
    CHK(!gateIsCell(XB_PORTAL, 2)); CHK(gateIsCell(XB_PORTAL, 10)); CHK(!gateIsCell(XB_PORTAL, 0)); CHK(!gateIsCell(STONE, 10));
    // viagem de ida: netherTick com o jogador dentro do gateway 0 (96,75,0)
    Phys P; memset(&P, 0, sizeof P); creative = false;
    P.x = P.px = 96.5; P.y = P.py = 75.0; P.z = P.pz = 0.5;
    netherTick(P);
    { const double d = sqrt(P.x * P.x + P.z * P.z); CHK(d > 980 && d < 1100); CHK(P.mx == 0 && P.my == 0); CHK(fabs(P.z) < 120.0); }
    { const int lx = (int)floor(P.x), lz = (int)floor(P.z), ly = (int)floor(P.y);
      CHK(getB(lx, ly, lz) == AIR && getB(lx, ly + 1, lz) == AIR); CHK(getB(lx, ly - 1, lz) != AIR);   // pe no chao, sem preso
      int found = 0; for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) for (int y = ly - 4; y <= ly + 8; y++) if (gateIsCell(getB(lx + dx, y, lz + dz), getM(lx + dx, y, lz + dz))) found++;
      CHK(found == 2); }
    // nao repete ate sair (cooldown): ainda parado na celula nova nao volta
    const double ox = P.x; netherTick(P); CHK(P.x == ox);
    // volta: de dentro do gateway externo para o anel
    { int gx = -1, gz = -1, gy = -1; const int lx = (int)floor(P.x), lz = (int)floor(P.z), ly = (int)floor(P.y);
      for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) for (int y = ly - 4; y <= ly + 8; y++) if (gateIsCell(getB(lx + dx, y, lz + dz), getM(lx + dx, y, lz + dz))) { gx = lx + dx; gz = lz + dz; gy = y; }
      CHK(gy >= 0); P.x = P.px = gx + .5; P.y = P.py = gy; P.z = P.pz = gz + .5; netherTick(P);   // cooldown: saiu da celula antes? forca reset
      const double d0 = sqrt(P.x * P.x + P.z * P.z); if (d0 > 300) { P.x = P.px = ox + 100; netherTick(P); P.x = P.px = gx + .5; P.z = P.pz = gz + .5; P.y = P.py = gy; netherTick(P); }
      const double d = sqrt(P.x * P.x + P.z * P.z); CHK(d > 60 && d < 100); CHK(P.y > 60 && P.y < 100); }
    // jogador fora de gateway/portal nao viaja
    { Phys Q; memset(&Q, 0, sizeof Q); Q.x = Q.px = 3.5; Q.y = Q.py = 90.0; Q.z = Q.pz = 3.5; netherTick(Q); CHK(Q.x == 3.5 && Q.z == 3.5); }
    printf("test_endgate: %d verificacoes, %d falhas\n", nT, nF); return nF != 0;
}
