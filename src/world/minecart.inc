// ================= trilhos + carrinhos de mina (BlockRailBase + EntityMinecart do Minecraft 1.12.2) =================
// Trilhos: 4 ids XB (XB_RAIL + tipo: 0 comum, 1 energizado, 2 detector, 3 ativador). Metadado: bits 0-3 = forma (EnumRailDirection 0..9), bit 4 = ligado.
// Formas: 0 N-S, 1 L-O, 2..5 subida p/ L,O,N,S, 6..9 curvas SL,SO,NO,NL. Carrinhos: entidades CARTS (0 comum, 1 bau, 2 fornalha, 3 TNT, 4 funil).
static const char* RAIL_FILE[4] = {"rail", "powered_rail", "detector_rail", "activator_rail"};
static const int RDX[4] = {0, 1, 0, -1}, RDZ[4] = {-1, 0, 1, 0};   // lados: 0 N(-Z), 1 L(+X), 2 S(+Z), 3 O(-X)
static const int RCON[10][2] = {{0, 2}, {1, 3}, {3, 1}, {1, 3}, {2, 0}, {0, 2}, {2, 1}, {2, 3}, {0, 3}, {0, 1}};   // lados ligados; nas subidas o 2o e o lado ALTO
static const int RMAT[10][2][3] = {{{0, 0, -1}, {0, 0, 1}}, {{-1, 0, 0}, {1, 0, 0}}, {{-1, -1, 0}, {1, 0, 0}}, {{-1, 0, 0}, {1, -1, 0}}, {{0, 0, -1}, {0, -1, 1}},
    {{0, -1, -1}, {0, 0, 1}}, {{0, 0, 1}, {1, 0, 0}}, {{0, 0, 1}, {-1, 0, 0}}, {{0, 0, -1}, {-1, 0, 0}}, {{0, 0, -1}, {1, 0, 0}}};   // BlockRailBase/EntityMinecart.MATRIX
static inline bool railAt(int x, int y, int z) { return xbIsRail(getB(x, y, z)); }
static inline int railShape(int x, int y, int z) { return std::min(getM(x, y, z) & 15, 9); }
static const int RNONE = -999999;

static void railInit() {   // chamado no fim de xbInit: modelos do pack (blockstates rail/powered_rail/...) e drops
    for (auto& r : gRailQ) for (auto& v : r) v.clear();
    static const int MY[10] = {0, 90, 90, 270, 0, 180, 0, 90, 180, 270}, MT[10] = {0, 0, 1, 1, 1, 1, 2, 2, 2, 2};   // y do blockstate; modelo 0 reto, 1 subida, 2 curva
    for (int k = 0; k < 4; k++) {
        for (int pw = 0; pw < (k == 0 ? 1 : 2); pw++) for (int s = 0; s < (k == 0 ? 10 : 6); s++) {
            const std::string n = std::string(RAIL_FILE[k]) + (pw ? "_on" : "");
            XModel m; if (xbLoadModel("block/" + (MT[s] == 0 ? n : MT[s] == 1 ? n + "_raised_ne" : std::string("rail_corner")), m)) xbBake(m, 0, MY[s], false, gRailQ[k][s + pw * 10]);
        }
        gXM[XB_RAIL + k - XB0] = gRailQ[k][0];
        { int dr = 0; for (size_t i = 0; i < PK.size() && !dr; i++) if (pkCanon(PK[i].file) == std::string("block/") + RAIL_FILE[k]) dr = PK0 + (int)i; gXbDrop[XB_RAIL + k - XB0] = dr; }
    }
}
static int railKindOfItem(int id) {
    if (id < PK0 || (size_t)(id - PK0) >= PK.size()) return -1;
    const std::string f = pkCanon(PK[id - PK0].file); for (int k = 0; k < 4; k++) if (f == std::string("block/") + RAIL_FILE[k]) return k; return -1;
}
// ---- ligacoes entre trilhos ----
static int railFind(int x, int y, int z, int d, int base) {   // trilho do lado d: no nivel base, depois +1, depois -1 (Rail.findRailAt)
    for (int o : {0, 1, -1}) if (railAt(x + RDX[d], y + base + o, z + RDZ[d])) return y + base + o; return RNONE;
}
static int railLinks(int x, int y, int z, int out[2][3]) {   // trilhos a que ESTE esta ligado pela forma atual
    const int sh = railShape(x, y, z); int n = 0;
    for (int i = 0; i < 2; i++) { const int d = RCON[sh][i], fy = railFind(x, y, z, d, (sh >= 2 && sh <= 5 && i == 1) ? 1 : 0); if (fy != RNONE) { out[n][0] = x + RDX[d]; out[n][1] = fy; out[n][2] = z + RDZ[d]; n++; } }
    return n;
}
static int railPick(int kind, const bool* m, const bool* up) {   // Rail.place: m[d] = ligado ao lado d; up[d] = vizinho um bloco acima
    const bool N = m[0], E = m[1], S = m[2], W = m[3]; int sh = -1;
    if ((N || S) && !E && !W) sh = 0;
    if ((E || W) && !N && !S) sh = 1;
    if (kind == 0) { if (S && E && !N && !W) sh = 6; if (S && W && !N && !E) sh = 7; if (N && W && !S && !E) sh = 8; if (N && E && !S && !W) sh = 9; }
    if (sh < 0) sh = (E && W && !(N && S)) ? 1 : 0;   // 0 ou 3+ ligacoes: reto
    if (sh == 0) { if (up[0]) sh = 4; if (up[2]) sh = 5; }
    if (sh == 1) { if (up[1]) sh = 2; if (up[3]) sh = 3; }
    return sh;
}
// ---- energia (alavanca ligada / tocha de redstone ao lado, propagando por ate 8 trilhos do mesmo tipo) ----
static bool rsStrongBlock(int x, int y, int z);   // redstone.inc
static bool rsRepOutInto(int rx, int ry, int rz, int tx, int ty, int tz);   // redstone.inc (repetidor E observador ligados)
static bool railSrcNear(int x, int y, int z) {
    static const int O[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const auto& o : O) if (rsStrongBlock(x + o[0], y + o[1], z + o[2])) return true;   // bloco solido energizado (tocha sob ele, repetidor, alavanca presa)
    for (const auto& o : O) if (rsRepOutInto(x + o[0], y + o[1], z + o[2], x, y, z)) return true;   // repetidor ligado apontando para o trilho / observador ligado com o trilho atras
    for (int yy = y; yy >= y - 1; yy--) for (const auto& o : O) {
        const int px = x + o[0], py = yy + o[1], pz = z + o[2]; if (px == x && py == y && pz == z) continue;
        const int b = getB(px, py, pz); if ((b >= XB_LEVER + 12 && b < XB_LEVER + 24) || (b >= XB_RTORCH && b < XB_RTORCH + 5 && !(getM(px, py, pz) & 1)) || (b == XB_DUST && ((getM(px, py, pz) >> 4) & 15) > 0) || ((b == XB_BTN || b == XB_PLATE) && ((getM(px, py, pz) >> 4) & 1))) return true;   // tocha so se acesa (meta bit0 = apagada); poeira com nivel > 0; botao/placa apertados
    }
    return false;
}
static bool railPowRec(int x, int y, int z, int kind, int depth, int fx, int fy, int fz) {
    if (railSrcNear(x, y, z)) return true; if (depth >= 8) return false;
    int l[2][3]; const int n = railLinks(x, y, z, l);
    for (int i = 0; i < n; i++) { if (l[i][0] == fx && l[i][1] == fy && l[i][2] == fz) continue; if (getB(l[i][0], l[i][1], l[i][2]) == XB_RAIL + kind && railPowRec(l[i][0], l[i][1], l[i][2], kind, depth + 1, x, y, z)) return true; }
    return false;
}
static void railNeighbor(int x, int y, int z, int b) {   // hook de vizinhanca (blockupdate.inc): atualiza o bit "ligado" dos trilhos energizado/ativador
    const int kind = b - XB_RAIL; if (kind != 1 && kind != 3) return;
    const int m = getM(x, y, z), np = railPowRec(x, y, z, kind, 0, RNONE, 0, 0) ? 1 : 0;
    if (np != ((m >> 4) & 1)) { setM(x, y, z, (m & 15) | (np << 4)); netSendBlock(x, y, z, b, (m & 15) | (np << 4)); }
}
static void railPlaceAt(int x, int y, int z, int kind) {   // coloca e liga aos vizinhos (BlockRailBase.Rail.place + neighborChanged)
    bool m[4] = {false}, up[4] = {false}; int nb[4][3]; bool has[4] = {false};
    for (int d = 0; d < 4; d++) {
        const int fy = railFind(x, y, z, d, 0); if (fy == RNONE) continue;
        const int qx = x + RDX[d], qz = z + RDZ[d]; int l[2][3]; const int n = railLinks(qx, fy, qz, l);
        if (n >= 2) continue;   // vizinho ja esta ocupado dos dois lados
        m[d] = true; up[d] = fy > y; has[d] = true; nb[d][0] = qx; nb[d][1] = fy; nb[d][2] = qz;
    }
    const int sh = railPick(kind, m, up); setBM(x, y, z, XB_RAIL + kind, sh); netSendBlock(x, y, z, XB_RAIL + kind, sh);
    for (int d = 0; d < 4; d++) if (has[d]) {   // o vizinho tambem vira para ligar em nos
        const int qx = nb[d][0], qy = nb[d][1], qz = nb[d][2], qk = getB(qx, qy, qz) - XB_RAIL; bool qm[4] = {false}, qu[4] = {false};
        int l[2][3]; const int n = railLinks(qx, qy, qz, l);
        for (int i = 0; i < n; i++) for (int e = 0; e < 4; e++) if (l[i][0] == qx + RDX[e] && l[i][2] == qz + RDZ[e]) { qm[e] = true; qu[e] = l[i][1] > qy; }
        const int back = (d + 2) & 3; qm[back] = true; qu[back] = y > qy;
        const int ns = railPick(qk, qm, qu), om = getM(qx, qy, qz);
        if (ns != (om & 15)) { setBM(qx, qy, qz, XB_RAIL + qk, ns | (om & 16)); netSendBlock(qx, qy, qz, XB_RAIL + qk, ns | (om & 16)); }
    }
    if (kind == 1 || kind == 3) railNeighbor(x, y, z, XB_RAIL + kind);
}
static bool railPlace(int kind, const int* pp) {   // patchRail.1: como o ItemBlock do MC, o trilho substitui mato/flor/liquido (antes so aceitava AIR: nao colocava no chao com grama alta)
    const int tb = getB(pp[0], pp[1], pp[2]); if (pp[1] < 1 || pp[1] >= CH - 1 || !getC(fdiv(pp[0], CW), fdiv(pp[2], CW))) return false;
    if (!(tb == AIR || isLiquid(tb) || xbReplaceable(tb)) || !solid(getB(pp[0], pp[1] - 1, pp[2]))) return false;
    railPlaceAt(pp[0], pp[1], pp[2], kind); return true;
}

// ================= carrinhos =================
enum { CT_NORMAL, CT_CHEST, CT_FURNACE, CT_TNT, CT_HOPPER, CT_N };
static const char* CART_FILE[CT_N] = {"minecart", "chest_minecart", "furnace_minecart", "tnt_minecart", "hopper_minecart"};
static const double CART_W = .98, CART_H = .7, CART_SEAT_Y = -.20;
struct CartEnt {
    int id = 0, type = 0; bool dead = false, onGround = false, inReverse = false, boom = false;
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0, mx = 0, my = 0, mz = 0, pushX = 0, pushZ = 0;
    float yaw = 0, damage = 0; int rolling = 0, rollDir = 1, fuse = -1, fuel = 0;
};
static std::vector<CartEnt> CARTS; static int gCartNext = 0, gCartRide = 0;
static std::vector<CartEnt> gCartGhost;   // BUG-040: convidado: carrinhos do host so para desenhar (falling.inc: cartNetRecv)
static CartEnt* cartById(int id) { if (id) for (CartEnt& c : CARTS) if (c.id == id) return &c; return nullptr; }
static AABB cartBox(const CartEnt& c) { const double h = CART_W * .5; return {c.x - h, c.y, c.z - h, c.x + h, c.y + CART_H, c.z + h}; }
static int cartTypeOfItem(int id) {
    if (id < PK0 || (size_t)(id - PK0) >= PK.size()) return -1;
    const std::string f = pkCanon(PK[id - PK0].file); for (int t = 0; t < CT_N; t++) if (f == std::string("item/") + CART_FILE[t]) return t; return -1;
}
static int cartItemId(int t) { for (size_t i = 0; i < PK.size(); i++) if (pkCanon(PK[i].file) == std::string("item/") + CART_FILE[t]) return PK0 + (int)i; return 0; }   // patchRail.2: nomes crus do pack 1.12 (rail_normal, minecart_normal)
// EntityMinecart.getPos / getPosOffset: ponto sobre o trilho (off = deslocamento ao longo dele)
static bool railPoint(double x, double y, double z, double off, double* o) {
    const int i = (int)floor(x); int j = (int)floor(y); const int k = (int)floor(z); if (railAt(i, j - 1, k)) j--;
    if (!railAt(i, j, k)) return false;
    const int sh = railShape(i, j, k); const int(*a)[3] = RMAT[sh];
    const double d0 = i + .5 + a[0][0] * .5, d1 = j + .0625 + a[0][1] * .5, d2 = k + .5 + a[0][2] * .5, d3 = i + .5 + a[1][0] * .5, d4 = j + .0625 + a[1][1] * .5, d5 = k + .5 + a[1][2] * .5;
    double d6 = d3 - d0, d7 = (d4 - d1) * 2, d8 = d5 - d2, d9;
    if (off != 0) {
        const double l = sqrt(d6 * d6 + d7 * d7 + d8 * d8); x += d6 / l * off; y += d7 / l * off; z += d8 / l * off;
        if (a[0][1] != 0 && (int)floor(x) - i == a[0][0] && (int)floor(z) - k == a[0][2]) y += a[0][1];
        else if (a[1][1] != 0 && (int)floor(x) - i == a[1][0] && (int)floor(z) - k == a[1][2]) y += a[1][1];
        return railPoint(x, y, z, 0, o);
    }
    if (d6 == 0) d9 = z - k; else if (d8 == 0) d9 = x - i; else d9 = ((x - d0) * d6 + (z - d2) * d8) * 2;
    x = d0 + d6 * d9; y = d1 + d7 * d9; z = d2 + d8 * d9; if (d7 < 0) y += 1; if (d7 > 0) y += .5;
    o[0] = x; o[1] = y; o[2] = z; return true;
}
static void cartMove(CartEnt& c, double mx, double my, double mz) {   // Entity.move (sem degrau)
    AABB bb = cartBox(c); double x = mx, y = my, z = mz; const double d6 = x, d7 = y, d8 = z;
    std::vector<AABB> list; collBoxes(bb.addCoord(x, y, z), list);
    for (auto& q : list) y = q.calcY(bb, y); bb = bb.offset(0, y, 0);
    for (auto& q : list) x = q.calcX(bb, x); bb = bb.offset(x, 0, 0);
    for (auto& q : list) z = q.calcZ(bb, z); bb = bb.offset(0, 0, z);
    c.x = (bb.x0 + bb.x1) / 2; c.y = bb.y0; c.z = (bb.z0 + bb.z1) / 2;
    c.onGround = d7 != y && d7 < 0; if (d6 != x) c.mx = 0; if (d8 != z) c.mz = 0; if (d7 != y) c.my = 0;
}
static void cartDrag(CartEnt& c, bool riden) {   // applyDrag (a fornalha empurra por conta propria)
    if (c.type == CT_FURNACE) {
        double d = c.pushX * c.pushX + c.pushZ * c.pushZ;
        if (d > 1e-4) { d = sqrt(d); c.pushX /= d; c.pushZ /= d; c.mx = c.mx * .8 + c.pushX; c.my = 0; c.mz = c.mz * .8 + c.pushZ; }
        else { c.mx *= .98; c.my = 0; c.mz *= .98; }
    }
    if (riden) { c.mx *= .997; c.my = 0; c.mz *= .997; } else { c.mx *= .96; c.my = 0; c.mz *= .96; }
}
static void cartTrack(CartEnt& c, int bx, int by, int bz, bool riden, bool fwd, float yawO) {   // EntityMinecart.moveAlongTrack
    const int meta = getM(bx, by, bz), sh = std::min(meta & 15, 9), kind = getB(bx, by, bz) - XB_RAIL; const bool pw = (meta >> 4) & 1;
    double v0[3], v1[3]; const bool hv = railPoint(c.x, c.y, c.z, 0, v0);
    c.y = by; bool flag = false, flag1 = false; if (kind == 1) { flag = pw; flag1 = !pw; }
    const double D0 = .0078125;
    if (sh == 2) { c.mx -= D0; c.y += 1; } else if (sh == 3) { c.mx += D0; c.y += 1; } else if (sh == 4) { c.mz += D0; c.y += 1; } else if (sh == 5) { c.mz -= D0; c.y += 1; }
    const int(*a)[3] = RMAT[sh];
    double d1 = a[1][0] - a[0][0], d2 = a[1][2] - a[0][2]; const double d3 = sqrt(d1 * d1 + d2 * d2);
    if (c.mx * d1 + c.mz * d2 < 0) { d1 = -d1; d2 = -d2; }
    const double sp = std::min(2.0, sqrt(c.mx * c.mx + c.mz * c.mz)); c.mx = sp * d1 / d3; c.mz = sp * d2 / d3;
    if (riden && fwd && c.mx * c.mx + c.mz * c.mz < .01) { const float r = yawO * .0174533f; c.mx += -sin(r) * .1; c.mz += -cos(r) * .1; flag1 = false; }   // jogador empurra (W)
    if (c.type == CT_FURNACE) { double d = c.pushX * c.pushX + c.pushZ * c.pushZ; if (d > 1e-4 && c.mx * c.mx + c.mz * c.mz > .001) { if (c.pushX * c.mx + c.pushZ * c.mz < 0) c.pushX = c.pushZ = 0; else { c.pushX = c.mx; c.pushZ = c.mz; } } }
    if (flag1) { if (sqrt(c.mx * c.mx + c.mz * c.mz) < .03) c.mx = c.my = c.mz = 0; else { c.mx *= .5; c.my = 0; c.mz *= .5; } }   // trilho energizado apagado freia
    const double e0 = bx + .5 + a[0][0] * .5, e1 = bz + .5 + a[0][2] * .5, e2 = bx + .5 + a[1][0] * .5, e3 = bz + .5 + a[1][2] * .5; double d10;
    d1 = e2 - e0; d2 = e3 - e1;
    if (d1 == 0) { c.x = bx + .5; d10 = c.z - bz; } else if (d2 == 0) { c.z = bz + .5; d10 = c.x - bx; } else d10 = ((c.x - e0) * d1 + (c.z - e1) * d2) * 2;
    c.x = e0 + d1 * d10; c.z = e1 + d2 * d10;
    double d22 = c.mx, d23 = c.mz; if (riden) { d22 *= .75; d23 *= .75; }
    cartMove(c, std::max(-.4, std::min(.4, d22)), 0, std::max(-.4, std::min(.4, d23)));
    if (a[0][1] != 0 && (int)floor(c.x) - bx == a[0][0] && (int)floor(c.z) - bz == a[0][2]) c.y += a[0][1];
    else if (a[1][1] != 0 && (int)floor(c.x) - bx == a[1][0] && (int)floor(c.z) - bz == a[1][2]) c.y += a[1][1];
    cartDrag(c, riden);
    if (hv && railPoint(c.x, c.y, c.z, 0, v1)) {   // sobe/desce devagar nas rampas
        const double d14 = (v0[1] - v1[1]) * .05, d = sqrt(c.mx * c.mx + c.mz * c.mz); if (d > 0) { c.mx = c.mx / d * (d + d14); c.mz = c.mz / d * (d + d14); } c.y = v1[1];
    }
    const int j = (int)floor(c.x), k = (int)floor(c.z);
    if (j != bx || k != bz) { const double d = sqrt(c.mx * c.mx + c.mz * c.mz); c.mx = d * (j - bx); c.mz = d * (k - bz); }
    if (flag) {   // trilho energizado acelera (0.06) ou arranca encostado em parede
        const double d = sqrt(c.mx * c.mx + c.mz * c.mz);
        if (d > .01) { c.mx += c.mx / d * .06; c.mz += c.mz / d * .06; }
        else if (sh == 1) { if (solid(getB(bx - 1, by, bz))) c.mx = .02; else if (solid(getB(bx + 1, by, bz))) c.mx = -.02; }
        else if (sh == 0) { if (solid(getB(bx, by, bz - 1))) c.mz = .02; else if (solid(getB(bx, by, bz + 1))) c.mz = -.02; }
    }
}
static void cartDerail(CartEnt& c) {   // moveDerailedMinecart
    c.mx = std::max(-.4, std::min(.4, c.mx)); c.mz = std::max(-.4, std::min(.4, c.mz));
    if (c.onGround) { c.mx *= .5; c.my *= .5; c.mz *= .5; }
    cartMove(c, c.mx, c.my, c.mz);
    if (!c.onGround) { c.mx *= .95; c.my *= .95; c.mz *= .95; }
}
static void cartEject(CartEnt& c, Phys& P) { if (gCartRide == c.id) { gCartRide = 0; P.x = c.x; P.y = c.y + .75; P.z = c.z; P.px = P.x; P.py = P.y; P.pz = P.z; P.mx = P.my = P.mz = 0; } }
static void cartKill(CartEnt& c, Phys& P, bool drop) { if (drop) spawnDrop(c.x, c.y + .3, c.z, 0, .15, 0, cartItemId(c.type), 1, 10); cartEject(c, P); c.dead = true; }
static void cartExplode(const CartEnt& c, Phys& P) {   // EntityMinecartTNT.explodeCart (raio 3, sem drops)
    const int cx = (int)floor(c.x), cy = (int)floor(c.y), cz = (int)floor(c.z), R = 3;
    for (int dx = -R; dx <= R; dx++) for (int dy = -R; dy <= R; dy++) for (int dz = -R; dz <= R; dz++) {
        if (dx * dx + dy * dy + dz * dz > R * R + 1) continue; const int b = getB(cx + dx, cy + dy, cz + dz);
        if (b == AIR || b == BEDROCK || isLiquid(b)) continue; setB(cx + dx, cy + dy, cz + dz, AIR); netSendBlock(cx + dx, cy + dy, cz + dz, AIR);
    }
    const double d = sqrt((P.x - c.x) * (P.x - c.x) + (P.y - c.y) * (P.y - c.y) + (P.z - c.z) * (P.z - c.z)); if (d < 6) survDamage((int)(14 * (1 - d / 6)) + 1, "explosao");
}
static void cartTick(CartEnt& c, Phys& P, bool riden, bool fwd, float yawO) {   // EntityMinecart.onUpdate
    c.px = c.x; c.py = c.y; c.pz = c.z; const float oy = c.yaw;
    if (c.rolling > 0) c.rolling--; if (c.damage > 0) c.damage--;
    if (c.y < -64) { cartKill(c, P, false); return; }
    if (c.type == CT_TNT && c.fuse >= 0) { if (c.fuse-- == 0) { c.boom = true; return; } }
    if (c.type == CT_FURNACE) { if (c.fuel > 0) c.fuel--; if (c.fuel <= 0) c.pushX = c.pushZ = 0; }
    c.my -= .04;
    const int i = (int)floor(c.x); int j = (int)floor(c.y); const int k = (int)floor(c.z); if (railAt(i, j - 1, k)) j--;
    if (railAt(i, j, k)) {
        cartTrack(c, i, j, k, riden, fwd, yawO);
        if (getB(i, j, k) == XB_RAIL + 3 && ((getM(i, j, k) >> 4) & 1)) {   // trilho ativador ligado: TNT acende, passageiro desce
            if (c.type == CT_TNT && c.fuse < 0) c.fuse = 80; if (riden) cartEject(c, P);
        }
    } else cartDerail(c);
    const double dx = c.px - c.x, dz = c.pz - c.z;
    if (dx * dx + dz * dz > .001) { c.yaw = (float)(atan2(dz, dx) * 57.29578); if (c.inReverse) c.yaw += 180.f; }
    float d3 = fmodf(c.yaw - oy, 360.f); if (d3 > 180.f) d3 -= 360.f; if (d3 < -180.f) d3 += 360.f;
    if (d3 < -170.f || d3 >= 170.f) { c.yaw += 180.f; c.inReverse = !c.inReverse; }
}
static std::vector<std::array<int, 3>> gDetOn;
static void railDetectTick() {   // trilho detector liga enquanto houver carrinho em cima
    std::vector<std::array<int, 3>> now;
    for (const CartEnt& c : CARTS) { const int i = (int)floor(c.x); int j = (int)floor(c.y); const int k = (int)floor(c.z); if (railAt(i, j - 1, k)) j--; if (getB(i, j, k) == XB_RAIL + 2) now.push_back({i, j, k}); }
    auto setD = [](const std::array<int, 3>& p, int on) { if (getB(p[0], p[1], p[2]) != XB_RAIL + 2) return; const int m = getM(p[0], p[1], p[2]); if (((m >> 4) & 1) != on) { setM(p[0], p[1], p[2], (m & 15) | (on << 4)); netSendBlock(p[0], p[1], p[2], XB_RAIL + 2, (m & 15) | (on << 4)); } };
    for (auto& p : gDetOn) if (std::find(now.begin(), now.end(), p) == now.end()) setD(p, 0);
    for (auto& p : now) setD(p, 1);
    gDetOn = now;
}
static bool cartTickAll(Phys& P, bool F, bool sneak, float yawO) {
    for (size_t i = 0; i < CARTS.size(); i++) {
        CartEnt& c = CARTS[i]; const bool r = c.id == gCartRide;
        if (!r && !getC(fdiv((int)floor(c.x), CW), fdiv((int)floor(c.z), CW))) { c.px = c.x; c.py = c.y; c.pz = c.z; continue; }   // chunk nao carregado: congela
        cartTick(c, P, r, F && r, yawO);
        if (c.boom) { cartExplode(c, P); cartKill(c, P, false); continue; }
        if (!gCartRide && !gBoatRide && !c.dead) {   // Entity.applyEntityCollision: o jogador empurra o carrinho
            const AABB pb{P.x - .3, P.y, P.z - .3, P.x + .3, P.y + 1.8, P.z + .3};
            if (pb.intersects(cartBox(c))) { double dx = P.x - c.x, dz = P.z - c.z; double m = std::max(fabs(dx), fabs(dz)); if (m >= .01) { m = sqrt(m); dx /= m; dz /= m; const double t = std::min(1.0, 1.0 / m); c.mx -= dx * t * .05; c.mz -= dz * t * .05; } }
        }
    }
    for (size_t i = CARTS.size(); i-- > 0;) if (CARTS[i].dead) CARTS.erase(CARTS.begin() + i);
    railDetectTick();
    CartEnt* c = cartById(gCartRide);
    if (!c) gCartRide = 0; else if (sneak) cartEject(*c, P);
    if ((c = cartById(gCartRide))) { P.x = c->x; P.y = c->y + CART_SEAT_Y; P.z = c->z; P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false; return true; }
    return false;
}
// ---- interacao ----
static int cartPick(const float* eye, const float* dir, float reach) {
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)CARTS.size(); i++) {
        const CartEnt& e = CARTS[i]; if (e.id == gCartRide) continue;
        const AABB a = cartBox(e); const float lo[3] = {(float)a.x0 - .1f, (float)a.y0 - .1f, (float)a.z0 - .1f}, hi[3] = {(float)a.x1 + .1f, (float)a.y1 + .1f, (float)a.z1 + .1f};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int k = 0; k < 3 && ok; k++) {
            if (fabsf(dir[k]) < 1e-6f) { if (eye[k] < lo[k] || eye[k] > hi[k]) ok = false; continue; }
            float ta = (lo[k] - eye[k]) / dir[k], tb = (hi[k] - eye[k]) / dir[k]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    return best;
}
static bool cartInteract(const float* eye, const float* dir, Slot& held, bool creative, float reach, const Phys& P) {   // clique direito: montar (comum) / abastecer (fornalha)
    const int i = cartPick(eye, dir, reach); if (i < 0) return false; CartEnt& c = CARTS[i];
    if (c.type == CT_FURNACE && held.id == IT_COAL) { c.fuel += 3600; double dx = c.x - P.x, dz = c.z - P.z; const double l = sqrt(dx * dx + dz * dz); if (l > 1e-6) { c.pushX = dx / l; c.pushZ = dz / l; } if (!creative && --held.n <= 0) held = Slot(); return true; }
    if (c.type == CT_NORMAL) gCartRide = c.id;
    return true;
}
static bool cartHit(const float* eye, const float* dir, int held, bool creative, float reach, Phys& P) {   // EntityMinecart.attackEntityFrom
    const int i = cartPick(eye, dir, reach); if (i < 0) return false; CartEnt& c = CARTS[i];
    c.rollDir = -c.rollDir; c.rolling = 10; c.damage += mobWeaponDmg(held) * 10.f;
    if (creative) cartKill(c, P, false); else if (c.damage > 40.f) cartKill(c, P, true);
    return true;
}
static void cartSpawn(int type, double x, double y, double z) { CartEnt c; c.id = ++gCartNext; c.type = type; c.x = c.px = x; c.y = c.py = y; c.z = c.pz = z; CARTS.push_back(c); }
// 0 = nao e item de trilho/carrinho; 1 = tratado sem efeito; 2 = colocou (consumir 1)
static int railUse(const Slot& s, const int* hh, const int* pp, bool rpt) {
    const int rk = railKindOfItem(s.id), ct = cartTypeOfItem(s.id); if (rk < 0 && ct < 0) return 0; if (rpt) return 1;
    if (rk >= 0) { int t[3] = {pp[0], pp[1], pp[2]}; if (xbReplaceable(getB(hh[0], hh[1], hh[2]))) { t[0] = hh[0]; t[1] = hh[1]; t[2] = hh[2]; }   // mirar no mato: o trilho entra no lugar dele
        return railPlace(rk, t) ? 2 : 1; }
    if (!railAt(hh[0], hh[1], hh[2])) return 1;   // ItemMinecart.onItemUse: so em cima de trilho
    cartSpawn(ct, hh[0] + .5, hh[1] + .0625 + (railShape(hh[0], hh[1], hh[2]) >= 2 && railShape(hh[0], hh[1], hh[2]) <= 5 ? .5 : 0), hh[2] + .5); return 2;
}
// ---------------- salvar / carregar (saves/<mundo>/carts.dat) ----------------
static void cartsSave(const std::string& dir) {
    FILE* f = fopen(("saves/" + dir + "/carts.dat").c_str(), "wb"); if (!f) return; fprintf(f, "CARTS1\n");
    for (const CartEnt& c : CARTS) if (!c.dead) fprintf(f, "c %d %d %.9g %.9g %.9g %.6g %.6g %d %d\n", c.id, c.type, c.x, c.y, c.z, c.mx, c.mz, c.fuel, c.id == gCartRide ? 1 : 0);
    fclose(f);
}
static void cartsLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    CARTS.clear(); gCartRide = 0; gCartNext = 0; gDetOn.clear();
    FILE* f = fopen(("saves/" + dir + "/carts.dat").c_str(), "rb"); if (!f) return; char ln[256];
    while (fgets(ln, sizeof ln, f)) {
        int id, t, fuel, ride; double x, y, z, mx, mz;
        if (ln[0] != 'c' || sscanf(ln, "c %d %d %lf %lf %lf %lf %lf %d %d", &id, &t, &x, &y, &z, &mx, &mz, &fuel, &ride) < 9 || id <= 0 || t < 0 || t >= CT_N) continue;
        CartEnt c; c.id = id; c.type = t; c.x = c.px = x; c.y = c.py = y; c.z = c.pz = z; c.mx = mx; c.mz = mz; c.fuel = fuel; CARTS.push_back(c); gCartNext = std::max(gCartNext, id); if (ride) gCartRide = id;
    }
    fclose(f);
}
// ---------------- modelo e desenho (ModelMinecart 1.12.2, textura 64x32 entity/minecart/minecart.png) ----------------
static std::vector<V> cmPart[6]; static GLuint cartTex = 0; static bool cartReady = false;
static void loadCartModel() {
    cartReady = true; std::vector<uint8_t> px; int w = 0, h = 0;
    if (!packRGBA("entity/minecart/minecart.png", px, w, h) && !packRGBA("entity/minecart.png", px, w, h)) { w = 64; h = 32; px.assign(64 * 32 * 4, 0); for (int i = 0; i < 64 * 32; i++) { px[i * 4] = px[i * 4 + 1] = px[i * 4 + 2] = 120; px[i * 4 + 3] = 255; } }
    if (!cartTex) glGenTextures(1, &cartTex); glBindTexture(GL_TEXTURE_2D, cartTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    for (auto& m : cmPart) m.clear();
    boxM(cmPart[0], -10, 10, -8, 8, -1, 1, 0, 10, 20, 16, 2, 0);   // fundo
    boxM(cmPart[1], -9, 9, -7, 7, -1, 0, 44, 10, 18, 14, 1, 0);    // piso interno
    for (int i = 2; i < 6; i++) boxM(cmPart[i], -8, 8, 1, 9, -1, 1, 0, 0, 16, 8, 2, 0);   // 4 laterais
    for (auto& m : cmPart) for (V& v : m) v.v *= 2.f;   // boxM normaliza v por 64; a textura do carrinho tem 32 de altura
}
static int cartTile(int type) {
    static const char* N[CT_N] = {"", "oak_planks", "furnace_front", "tnt_side", "hopper_outside"};
    if (!N[type][0]) return -1; auto it = gPkByFile.find(std::string("block/") + N[type]); return it != gPkByFile.end() && isPk(it->second) ? PK[it->second - PK0].tile : -1;
}
static void drawCart(const CartEnt& e, float pt) {
    auto L = [&](double a, double b) { return a + (b - a) * pt; };
    const double ix = L(e.px, e.x), iy = L(e.py, e.y), iz = L(e.pz, e.z);
    double p0[3], p1[3], p2[3]; float ox = 0, oy = 0, oz = 0, yw = e.yaw, pitch = 0;
    if (railPoint(ix, iy, iz, 0, p0)) {
        if (!railPoint(ix, iy, iz, .3, p1)) memcpy(p1, p0, sizeof p0); if (!railPoint(ix, iy, iz, -.3, p2)) memcpy(p2, p0, sizeof p0);
        ox = (float)(p0[0] - ix); oy = (float)((p1[1] + p2[1]) / 2 - iy); oz = (float)(p0[2] - iz);
        double vx = p2[0] - p1[0], vy = p2[1] - p1[1], vz = p2[2] - p1[2]; const double l = sqrt(vx * vx + vy * vy + vz * vz);
        if (l > 1e-6) { vx /= l; vy /= l; vz /= l; yw = (float)(atan2(vz, vx) * 57.29578); pitch = (float)(atan(vy) * 73.0); }
    }
    rdPushMatrix(); rdTranslatef((float)ix + ox, (float)iy + oy + .375f, (float)iz + oz);
    rdRotatef(180.f - yw, 0, 1, 0); rdRotatef(-pitch, 0, 0, 1);
    const float f = e.rolling - pt; float f1 = e.damage - pt; if (f1 < 0.f) f1 = 0.f; if (f > 0.f) rdRotatef(sinf(f) * f * f1 / 10.f * e.rollDir, 1, 0, 0);
    const int tile = cartTile(e.type);
    if (tile >= 0) {   // bloco carregado (bau/fornalha/TNT/funil): cubo de 0,75 com a textura do bloco
        glBindTexture(GL_TEXTURE_2D, atlasTex); rdPushMatrix(); rdScalef(.75f, .75f, .75f); rdTranslatef(-.5f, -.125f, -.5f);
        const float u0 = (tile % ATL + .02f) / ATL, u1 = (tile % ATL + .98f) / ATL, v0 = (tile / ATL + .02f) / ATLR, v1 = (tile / ATL + .98f) / ATLR;
        static const float Q[6][4][3] = {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
            {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}, {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}};
        glBegin(GL_QUADS); for (int s = 0; s < 6; s++) { const float sh = s == 4 ? 1.f : s == 5 ? .5f : .8f; glColor4f(sh, sh, sh, 1.f);
            for (int k = 0; k < 4; k++) { glTexCoord2f(k == 0 || k == 3 ? u0 : u1, k < 2 ? v1 : v0); glVertex3f(Q[s][k][0], Q[s][k][1], Q[s][k][2]); } } glEnd();
        glColor4f(1.f, 1.f, 1.f, 1.f); rdPopMatrix();
    }
    glBindTexture(GL_TEXTURE_2D, cartTex);
    part(cmPart[0], 0, -4, 0, -90.f); part(cmPart[1], 0, -4, 0, 90.f);
    part(cmPart[2], 9, -4, 0, 0, 90.f); part(cmPart[3], -9, -4, 0, 0, -90.f); part(cmPart[4], 0, -4, -7, 0, 180.f); part(cmPart[5], 0, -4, 7);
    rdPopMatrix();
}
static void drawCarts(float pt) {
    if (CARTS.empty() && gCartGhost.empty()) return; if (!cartReady) loadCartModel();
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .4f);
    for (const CartEnt& e : CARTS) { if (entOutView(e.x, e.y, e.z, 2.5f)) continue; entLightAt(e.x, e.y + .4, e.z, gEntDim); drawCart(e, pt); }
    for (const CartEnt& e : gCartGhost) { if (entOutView(e.x, e.y, e.z, 2.5f)) continue; entLightAt(e.x, e.y + .4, e.z, gEntDim); drawCart(e, pt); }
    entDimReset(); rdDisable(GL_ALPHA_TEST);
}
