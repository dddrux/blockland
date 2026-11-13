// endgate.inc — patchEndGate.1: End Gateway (1.12.2). Cada morte do dragao cria 1 dos 20 gateways (anel r=96, y=75, ordem
// embaralhada pela seed); entrar leva a ~1024 blocos na mesma direcao (ilhas externas) e cria la o gateway de volta.
// Bloco = XB_PORTAL meta 10 (bit1 = celula do End como o portal; bit3 = gateway). Sem estado salvo: tudo deriva da posicao.
static const int GATE_N = 20, GATE_R = 96, GATE_Y = 75, GATE_OUT = 1024;
static void gatePos(int i, int& x, int& z) { const double a = 2.0 * (-3.14159265358979323846 + 0.15707963267948966 * i); x = (int)floor(GATE_R * cos(a)); z = (int)floor(GATE_R * sin(a)); }
static int gateOrder(int k) {   // k-esimo gateway a surgir
    const uint32_t sd = ((uint32_t)((uint64_t)WORLD_SEED ^ ((uint64_t)WORLD_SEED >> 32))) ^ 0x47415445u;
    int idx[GATE_N]; for (int i = 0; i < GATE_N; i++) idx[i] = i;
    std::sort(idx, idx + GATE_N, [&](int a, int b) { const uint32_t ha = nhash(a, 5, 9, sd), hb = nhash(b, 5, 9, sd); return ha != hb ? ha < hb : a < b; });
    return idx[((k % GATE_N) + GATE_N) % GATE_N];
}
static inline bool gateIsCell(int b, int m) { return b == XB_PORTAL && (m & 10) == 10; }
static void gatePut(int x, int y, int z, int b, int m) { if (!getC(fdiv(x, CW), fdiv(z, CW))) return; setBM(x, y, z, b, m); netSendBlock(x, y, z, b, m); }
static int gateGround(int x, int z, int from) { for (int y = std::min(from, CH - 3); y > 1; y--) { const int b = getB(x, y, z); if (b != AIR && !isLiquid(b)) return y; } return -1; }
static void gateBuild(int x, int z, int cy) {   // cy = y da celula de baixo; coluna de bedrock ate o chao (max 40), 2 celulas de portal
    for (int y = cy - 1; y >= cy - 2; y--) gatePut(x, y, z, BEDROCK, 0);
    for (int y = cy - 3, n = 0; y > 1 && n < 40 && getB(x, y, z) == AIR; y--, n++) gatePut(x, y, z, BEDROCK, 0);
    gatePut(x, cy, z, XB_PORTAL, 10); gatePut(x, cy + 1, z, XB_PORTAL, 10);
}
static bool endGateSpawn() {   // chamado a cada morte do dragao: cria o proximo que ainda nao existe
    if (gDim != 2) return false;
    for (int k = 0; k < GATE_N; k++) {
        int x, z; gatePos(gateOrder(k), x, z);
        if (!getC(fdiv(x, CW), fdiv(z, CW))) gen(fdiv(x, CW), fdiv(z, CW));
        if (gateIsCell(getB(x, GATE_Y, z), getM(x, GATE_Y, z))) continue;
        gateBuild(x, z, GATE_Y); return true;
    }
    return false;
}
static void gateMove(Phys& P, double x, double y, double z) { P.x = P.px = x; P.y = P.py = y; P.z = P.pz = z; P.mx = P.my = P.mz = 0; P.onGround = false; gFall = 0.f; gKick = true; }
static void endGateTravel(Phys& P) {   // jogador dentro de uma celula de gateway
    if (gDim != 2) return;
    const double len = std::max(1.0, sqrt(P.x * P.x + P.z * P.z)), ux = P.x / len, uz = P.z / len;
    if (len < 300.0) {   // ilha principal -> ilhas externas
        const int tx = (int)floor(ux * GATE_OUT), tz = (int)floor(uz * GATE_OUT);
        for (int dz = -3; dz <= 3; dz++) for (int dx = -3; dx <= 3; dx++) if (!getC(fdiv(tx, CW) + dx, fdiv(tz, CW) + dz)) gen(fdiv(tx, CW) + dx, fdiv(tz, CW) + dz);
        int lx = tx, lz = tz, ly = -1;
        for (int z = tz - 40; z <= tz + 40; z++) for (int x = tx - 40; x <= tx + 40; x++) { const int t = gateGround(x, z, CH - 3); if (t > ly) { ly = t; lx = x; lz = z; } }
        if (ly < 0) {   // sem ilha por perto: plataforma de end stone 7x7 (aproximacao)
            const int es = pkFind("block/end_stone"), st = es ? es : STONE; ly = GATE_Y - 4; lx = tx; lz = tz;
            for (int dz = -3; dz <= 3; dz++) for (int dx = -3; dx <= 3; dx++) gatePut(lx + dx, ly, lz + dz, st, 0);
        }
        for (int dy = 1; dy <= 6; dy++) for (int dz = -3; dz <= 3; dz++) for (int dx = -3; dx <= 3; dx++) if (dx || dz || dy > 4) { if (getB(lx + dx, ly + dy, lz + dz) != AIR && (dx * dx + dz * dz) <= 9 && dy > 1) gatePut(lx + dx, ly + dy, lz + dz, AIR, 0); }
        gateBuild(lx, lz, ly + 3);
        { const int es2 = pkFind("block/end_stone"); gatePut(lx + 2, ly, lz, es2 ? es2 : STONE, 0); }   // chao garantido onde o jogador chega
        { const int es2 = pkFind("block/end_stone"); gatePut(lx + 2, ly, lz, es2 ? es2 : STONE, 0); }   // chao garantido sob o jogador
        gatePut(lx + 2, ly + 1, lz, AIR, 0); gatePut(lx + 2, ly + 2, lz, AIR, 0);
        gateMove(P, lx + 2.5, ly + 1.0, lz + .5);
    } else {   // gateway externo -> volta para o anel (gateway de angulo mais proximo)
        int bi = 0; double bd = -2.0; for (int i = 0; i < GATE_N; i++) { int x, z; gatePos(i, x, z); const double d = (x * ux + z * uz) / GATE_R; if (d > bd) { bd = d; bi = i; } }
        int rx, rz; gatePos(bi, rx, rz); const int ix = (int)(rx * .85), iz = (int)(rz * .85);
        if (!getC(fdiv(ix, CW), fdiv(iz, CW))) gen(fdiv(ix, CW), fdiv(iz, CW));
        const int t = gateGround(ix, iz, GATE_Y - 3);
        if (t < 0) gateMove(P, .5, 66.0, 5.5); else gateMove(P, ix + .5, t + 1.0, iz + .5);
    }
}
