// ================= física do jogador (port do Minecraft Java 1.8.9) =================
// Tudo roda em ticks fixos de 20 TPS (como o Java) e é interpolado na renderização.
// Convenção deste jogo: yaw 0 olha para -Z; frente = (-sin yaw, -cos yaw), direita = (cos yaw, -sin yaw).
// (struct AABB agora vive em blockshape.inc: e compartilhada com o raycast e as formas dos blocos)

// Caixas de colisao REAIS dos blocos (blockshape.inc) que cruzam r. Comeca uma celula abaixo (floor(y0) - 1) como o World.getCollisionBoxes do Java:
// caixas mais altas que 1 (cerca = 1,5) invadem a celula de cima.
static int gBoatRide = 0;   // id do bote em que o jogador está montado (0 = nenhum); usado pela colisão
static void boatCollect(const AABB& r, std::vector<AABB>& out, int skipId);   // boat.inc: o casco do bote também é sólido (Entity.getCollisionBox)
static void propCollBoxes(const AABB& r, std::vector<AABB>& out);   // propsys.inc (patchProp.2)
static void collBoxes(const AABB& r, std::vector<AABB>& out) {
    out.clear(); AABB tmp[SH_MAXB];
    for (int y = (int)floor(r.y0) - 1; y <= (int)floor(r.y1); y++)
        for (int z = (int)floor(r.z0); z <= (int)floor(r.z1); z++)
            for (int x = (int)floor(r.x0); x <= (int)floor(r.x1); x++) {
                const int id = getB(x, y, z);
                if (!gBlkColl[id]) continue;
                const int n = blockBoxesId(id, x, y, z, false, tmp);
                for (int i = 0; i < n; i++) if (r.intersects(tmp[i])) out.push_back(tmp[i]);
            }
    propCollBoxes(r, out);   // patchProp.2: props (Decocraft)
}
static float gFxMoveMul = 1.f;   // patchPot.1: Velocidade/Lentidao (atualizado em fxTick)
static bool collEmpty(const AABB& r) { std::vector<AABB> l; collBoxes(r, l); return l.empty(); }
// Volume real da água: o topo da célula é y + nível/9 (fonte = 8/9), como getLiquidHeightPercent.
static bool anyLiquid(const AABB& r) {
    for (int y = (int)floor(r.y0); y <= (int)floor(r.y1); y++)
        for (int z = (int)floor(r.z0); z <= (int)floor(r.z1); z++)
            for (int x = (int)floor(r.x0); x <= (int)floor(r.x1); x++) {
                int b = getB(x, y, z);
                if (!isWater(b)) continue;
                double top = y + (isWater(getB(x, y + 1, z)) ? 1.0 : waterLevel(b) / 9.0);
                if (r.x1 > x && r.x0 < x + 1 && r.z1 > z && r.z0 < z + 1 && r.y1 > y && r.y0 < top) return true;
            }
    return false;
}

static bool anyLava(const AABB& r) {
    for (int y = (int)floor(r.y0); y <= (int)floor(r.y1); y++)
        for (int z = (int)floor(r.z0); z <= (int)floor(r.z1); z++)
            for (int x = (int)floor(r.x0); x <= (int)floor(r.x1); x++) {
                const int b = getB(x, y, z);
                if (!isLava(b)) continue;
                const int m = lavaMeta(b);
                const double top = y + (isLava(getB(x, y + 1, z)) ? 1.0 : 1.0 - ((m >= 8 ? 0 : m) + 1) / 9.0);
                if (r.x1 > x && r.x0 < x + 1 && r.z1 > z && r.z0 < z + 1 && r.y1 > y && r.y0 < top) return true;
            }
    return false;
}

struct Phys {
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0;     // posição e posição do tick anterior
    double mx = 0, my = 0, mz = 0;                          // motionX/Y/Z
    bool elytra = false; int elyTicks = 0;   // voo de elitra (flag 7) e ticksElytraFlying
    bool onGround = false, collH = false, collV = false, sneak = false, sprint = false, inWater = false, inLava = false, flying = false, noclip = false;
    float distWalked = 0, prevDistWalked = 0;
};
static const double PW = .3, PH = 1.8;   // largura/2 e altura do hitbox (1.8.9: agachar não muda o hitbox)
static AABB physBox(const Phys& P) { return {P.x - PW, P.y, P.z - PW, P.x + PW, P.y + PH, P.z + PW}; }

// Entity.moveEntity do 1.8.9: ordem Y, X, Z + step de 0.6 + borda ao agachar. (um unico passo; use moveEntity, que varre em sub-passos)
static void moveEntityOnce(Phys& P, double mx, double my, double mz) {
    if (P.noclip) { P.x += mx; P.y += my; P.z += mz; P.onGround = P.collH = P.collV = false; return; }   // espectador: sem colisão
    const double stepHeight = .6;
    double d3 = P.x, d5 = P.z;
    double x = mx, y = my, z = mz;
    AABB bb = physBox(P);
    const bool edge = P.onGround && P.sneak;
    double d6 = x, d7 = y, d8 = z;
    if (edge) {
        const double s = .05;
        while (x != 0 && collEmpty(bb.offset(x, -1, 0))) { if (x < s && x >= -s) x = 0; else if (x > 0) x -= s; else x += s; d6 = x; }
        while (z != 0 && collEmpty(bb.offset(0, -1, z))) { if (z < s && z >= -s) z = 0; else if (z > 0) z -= s; else z += s; d8 = z; }
        while (x != 0 && z != 0 && collEmpty(bb.offset(x, -1, z))) {
            if (x < s && x >= -s) x = 0; else if (x > 0) x -= s; else x += s;
            d6 = x;
            if (z < s && z >= -s) z = 0; else if (z > 0) z -= s; else z += s;
            d8 = z;
        }
    }
    std::vector<AABB> list;
    collBoxes(bb.addCoord(x, y, z), list); boatCollect(bb.addCoord(x, y, z), list, gBoatRide);
    const AABB bb0 = bb;
    for (auto& b : list) y = b.calcY(bb, y);
    bb = bb.offset(0, y, 0);
    const bool flag1 = P.onGround || (d7 != y && d7 < 0);
    for (auto& b : list) x = b.calcX(bb, x);
    bb = bb.offset(x, 0, 0);
    for (auto& b : list) z = b.calcZ(bb, z);
    bb = bb.offset(0, 0, z);

    if (flag1 && (d6 != x || d8 != z)) {
        const double d11 = x, d12 = y, d13 = z;
        const AABB boxNoStep = bb;
        double sy = stepHeight;
        std::vector<AABB> l1;
        collBoxes(bb0.addCoord(d6, sy, d8), l1); boatCollect(bb0.addCoord(d6, sy, d8), l1, gBoatRide);
        // variante A: sobe, depois avança X e Z
        AABB a = bb0, a5 = a.addCoord(d6, 0, d8);
        double dy = sy; for (auto& b : l1) dy = b.calcY(a5, dy);
        a = a.offset(0, dy, 0);
        double ax = d6; for (auto& b : l1) ax = b.calcX(a, ax);
        a = a.offset(ax, 0, 0);
        double az = d8; for (auto& b : l1) az = b.calcZ(a, az);
        a = a.offset(0, 0, az);
        // variante B: sobe sem considerar o deslocamento horizontal
        AABB c = bb0;
        double dy2 = sy; for (auto& b : l1) dy2 = b.calcY(c, dy2);
        c = c.offset(0, dy2, 0);
        double cx = d6; for (auto& b : l1) cx = b.calcX(c, cx);
        c = c.offset(cx, 0, 0);
        double cz = d8; for (auto& b : l1) cz = b.calcZ(c, cz);
        c = c.offset(0, 0, cz);
        if (ax * ax + az * az > cx * cx + cz * cz) { x = ax; z = az; y = -dy; bb = a; }
        else { x = cx; z = cz; y = -dy2; bb = c; }
        for (auto& b : l1) y = b.calcY(bb, y);
        bb = bb.offset(0, y, 0);
        if (d11 * d11 + d13 * d13 >= x * x + z * z) { x = d11; y = d12; z = d13; bb = boxNoStep; }
    }

    P.x = (bb.x0 + bb.x1) / 2; P.y = bb.y0; P.z = (bb.z0 + bb.z1) / 2;
    P.collH = d6 != x || d8 != z;
    P.collV = d7 != y;
    P.onGround = P.collV && d7 < 0;
    if (d6 != x) P.mx = 0;
    if (d8 != z) P.mz = 0;
    if (d7 != y) P.my = 0;
    if (!edge) { double dx = P.x - d3, dz = P.z - d5; P.distWalked += (float)(sqrt(dx * dx + dz * dz) * .6); }
}

// Varredura (swept AABB): movimento maior que 1 bloco/tick (queda longa, elytra, knockback, corrida com lag) e dividido em sub-passos
// de no maximo 1 bloco; um eixo que bateu em algo fica travado nos sub-passos seguintes. Movimentos normais (<= 1) sao identicos ao 1.8.9.
static void moveEntity(Phys& P, double mx, double my, double mz) {
    const double big = std::max(fabs(mx), std::max(fabs(my), fabs(mz)));
    if (P.noclip || !(big > 1.0)) { moveEntityOnce(P, mx, my, mz); return; }
    const int n = std::min(32, (int)ceil(big));
    double sx = mx / n, sy = my / n, sz = mz / n;
    bool cH = false, cV = false;
    for (int i = 0; i < n; i++) {
        const double bx = P.x, by = P.y, bz = P.z;
        moveEntityOnce(P, sx, sy, sz);
        if (P.collH) cH = true;
        if (P.collV) cV = true;
        if (fabs(P.x - bx) < fabs(sx) - 1e-9) sx = 0;
        if (fabs(P.z - bz) < fabs(sz) - 1e-9) sz = 0;
        if (P.collV && fabs(P.y - by) < fabs(sy) - 1e-9) sy = 0;
        if (sx == 0 && sy == 0 && sz == 0) break;
    }
    P.collH = cH; P.collV = cV; P.onGround = cV && my < 0;
}

static Uint32 gNoLiftUntil = 0;   // patchPhys.1: bloco caindo pousou perto: ate o jogador ficar livre (ou 60 s) so empurra para os lados (vanilla sufoca no lugar, nao cospe para cima)
// Jogador sobreposto a um bloco solido (tp, respawn, chunk que carregou depois, sair de veiculo/espectador, bloco colocado em cima):
// empurra para a posicao livre mais proxima (primeiro lados/cima ate 2 blocos; senao sobe na coluna ate ter espaco), como o pushOutOfBlocks.
// Nao faz nada se o chunk do jogador ainda nao existe (getB devolve ar la) nem no espectador. Retorna true se mexeu.
static bool unstuckPlayer(Phys& P) {
    if (P.noclip) return false;
    const bool soft = gNoLiftUntil && SDL_GetTicks() < gNoLiftUntil;
    if (!getC(fdiv((int)floor(P.x), CW), fdiv((int)floor(P.z), CW))) return false;
    const double e = 1e-4;   // encostar nao e sobrepor
    auto free = [&](double dx, double dy, double dz) { const AABB b = physBox(P).offset(dx, dy, dz); return collEmpty(AABB{b.x0 + e, b.y0 + e, b.z0 + e, b.x1 - e, b.y1 - e, b.z1 - e}); };
    if (free(0, 0, 0)) { gNoLiftUntil = 0; return false; }
    {   // patchTrap.2 + patchFix.24 (BUG-062): so age se um cubo cheio invade a caixa ENCOLHIDA em .01 (roçar borda por erro numerico nao empurra) (antes qualquer sobreposicao > 1e-4 teleportava ate 1 bloco de lado e 2 para cima)
        const AABB b = physBox(P); const double m = .01, my = .01; std::vector<AABB> l; collBoxes(AABB{b.x0 + m, b.y0 + my, b.z0 + m, b.x1 - m, b.y1 - my, b.z1 - m}, l); bool full = false;
        for (const AABB& q : l) if (q.x1 - q.x0 > .999 && q.y1 - q.y0 > .999 && q.z1 - q.z0 > .999) full = true;
        if (!full) return false;
    }
    double bx = 0, by = 0, bz = 0, best = 1e18; bool found = false, deep = false;
    for (int iy = 0; iy <= 20; iy++) for (int iz = -10; iz <= 10; iz++) for (int ix = -10; ix <= 10; ix++) {
        const double dx = ix * .1, dy = iy * .1, dz = iz * .1, c = dx * dx + dz * dz + dy * dy * 1.01;   // empate: prefere os lados
        if (soft && iy > 0) continue;   // so lados
        if (c >= best || !free(dx, dy, dz)) continue;
        best = c; bx = dx; by = dy; bz = dz; found = true;
    }
    if (!found && soft) return false;   // sem saida lateral: sufoca no lugar
    if (!found) {   // enterrado fundo: sobe a coluna ate achar vao (nunca passa do teto do mundo)
        for (int dy = 1; dy < CH; dy++) if (free(0, dy, 0)) { by = dy; found = true; deep = true; break; }
        if (!found) return false;
    }
    if (soft) { const double L = sqrt(bx * bx + bz * bz); if (L > .05) { bx *= .05 / L; bz *= .05 / L; } }
    else if (found && !deep) { const double L = sqrt(bx * bx + by * by + bz * bz); if (L > .1) { bx *= .1 / L; by *= .1 / L; bz *= .1 / L; } }   // patchFix.24: passo de no maximo .1 por chamada (como o pushOutOfBlocks), sem teleporte; so o "enterrado fundo" (sem saida em 1 bloco/2 acima) continua instantaneo   // passinho lateral por quadro, como o pushOutOfBlocks
    P.x += bx; P.y += by; P.z += bz;
    if (by > 0 && P.my < 0) P.my = 0;
    if (fabs(bx) + fabs(by) + fabs(bz) > 1.0) { P.px = P.x; P.py = P.y; P.pz = P.z; }
    return true;
}

// Entity.handleWaterMovement: caixa encolhida 0.4 em Y e 0.001 nos lados.
static bool inWaterNow(const Phys& P) {
    AABB b = physBox(P);
    b = {b.x0 + .001, b.y0 + .4, b.z0 + .001, b.x1 - .001, b.y1 - .4, b.z1 - .001};
    return anyLiquid(b);
}

// Entity.handleLavaMovement: caixa encolhida 0.1 nos lados e 0.4 em Y.
static bool inLavaNow(const Phys& P) {
    AABB b = physBox(P);
    return anyLava(AABB{b.x0 + .1, b.y0 + .4, b.z0 + .1, b.x1 - .1, b.y1 - .4, b.z1 - .1});
}

// BlockLiquid.getFlowVector: aponta para onde a agua esta mais baixa.
static int effDecay(int x, int y, int z) { int m = liqMeta(getB(x, y, z)); return m >= 8 ? 0 : m; }
static void flowVector(int x, int y, int z, double& fx, double& fz) {
    fx = fz = 0;
    static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
    const int i = effDecay(x, y, z);
    for (int d = 0; d < 4; d++) {
        int j = effDecay(x + DX[d], y, z + DZ[d]), k;
        if (j < 0) {
            if (!solid(getB(x + DX[d], y, z + DZ[d]))) {
                j = effDecay(x + DX[d], y - 1, z + DZ[d]);
                if (j >= 0) { k = j - (i - 8); fx += DX[d] * k; fz += DZ[d] * k; }
            }
        } else { k = j - i; fx += DX[d] * k; fz += DZ[d] * k; }
    }
}
// BlockLiquid.getFlow completo (1.12.2): vetor horizontal; agua caindo ao lado de bloco cheio ganha corrente para baixo (normalizado + (0,-6,0)); resultado sempre normalizado.
static void flowFull(int x, int y, int z, double& fx, double& fy, double& fz) {
    flowVector(x, y, z, fx, fz); fy = 0;
    if (liqMeta(getB(x, y, z)) >= 8) {
        static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1}; bool down = false;
        auto cube = [&](int b) { return !isWater(b) && (unsigned)b < 65536u && gBlkColl[b] == SHAPE_CUBE; };   // causesDownwardCurrent (aprox.: cubo cheio; gelo nao excluido, nao ha id de gelo)
        for (int d = 0; d < 4 && !down; d++) down = cube(getB(x + DX[d], y, z + DZ[d])) || cube(getB(x + DX[d], y + 1, z + DZ[d]));
        if (down) { const double l = sqrt(fx * fx + fz * fz); if (l > 0) { fx /= l; fz /= l; } fy = -6; }
    }
    const double l = sqrt(fx * fx + fy * fy + fz * fz); if (l > 0) { fx /= l; fy /= l; fz /= l; }
}
static void waterPush(Phys& P) {   // Entity.handleWaterMovement / World.handleMaterialAcceleration
    AABB b = physBox(P);
    b = {b.x0 + .001, b.y0 + .4, b.z0 + .001, b.x1 - .001, b.y1 - .4, b.z1 - .001};
    double ax = 0, ay = 0, az = 0;
    for (int y = (int)floor(b.y0); y < (int)floor(b.y1 + 1); y++)
        for (int z = (int)floor(b.z0); z < (int)floor(b.z1 + 1); z++)
            for (int x = (int)floor(b.x0); x < (int)floor(b.x1 + 1); x++) {
                const int bl = getB(x, y, z);
                if (!isWater(bl)) continue;
                const double top = y + 1 - ((liqMeta(bl) >= 8 ? 0 : liqMeta(bl)) + 1) / 9.0;
                if (b.y1 < top) continue;
                double fx, fy, fz; flowFull(x, y, z, fx, fy, fz);
                ax += fx; ay += fy; az += fz;
            }
    const double len = sqrt(ax * ax + ay * ay + az * az);
    if (len > 0) { P.mx += ax / len * .014; P.my += ay / len * .014; P.mz += az / len * .014; }
}

// moveFlying: yaw no padrão deste jogo. strafe > 0 = direita.
static void moveFlying(Phys& P, float yawDeg, float strafe, float forward, float friction) {
    float f = strafe * strafe + forward * forward;
    if (f < 1e-4f) return;
    f = sqrtf(f); if (f < 1.f) f = 1.f;
    f = friction / f; strafe *= f; forward *= f;
    const double r = yawDeg * .017453292519943295, s = sin(r), c = cos(r);
    P.mx += strafe * c + forward * -s;
    P.mz += strafe * -s + forward * -c;
}

static double gElyLook[3] = {0, 0, -1}; static float gElyPitchRad = 0.f; static float gElyWallDmg = 0.f;   // vetor de olhar, pitch (convencao Java: + = para baixo) e dano acumulado de colisao em voo
// EntityLivingBase.moveEntityWithHeading (sem poções/gelo/lava).
// patchMech.1: atrito do chao (Block.slipperiness: gelo/gelo compacto .98, slime .8, resto .6), teia (x.25/.05/.25) e alma de areia (x.4 andando)
static int mechId(const char* f) { auto it = gPkByFile.find(f); return it != gPkByFile.end() ? it->second : -1; }
static int slimeBlkId() { static int sl = -2; if (sl == -2) sl = mechId("block/slime_block"); return sl; }   // patchSlime.3
static bool slimeUnder(double x, double y, double z) { const int sl = slimeBlkId(); return sl > 0 && getB((int)floor(x), (int)floor(y - .01), (int)floor(z)) == sl; }
static float groundSlipAt(double x, double y, double z);
static float groundSlip(const Phys& P) { return groundSlipAt(P.x, P.y, P.z); }
static float groundSlipAt(double px, double py, double pz) {
    static int ice = -2, pk = -2, fr = -2, sl = -2; if (ice == -2) { ice = mechId("block/ice"); pk = mechId("block/packed_ice"); fr = mechId("block/frosted_ice"); sl = mechId("block/slime_block"); }
    const int b = getB((int)floor(px), (int)floor(py) - 1, (int)floor(pz));
    if (b < 0) return .6f;
    if (b == ice || b == pk || b == fr) return .98f;
    if (b == sl) return .8f;
    return .6f;
}
static bool physInWeb(const Phys& P) {   // patchFallWeb.1: corpo dentro de teia (Entity.isInWeb)
    static int web = -2; if (web == -2) web = mechId("block/cobweb");
    if (web < 0) return false;
    const int x0 = (int)floor(P.x - .3), x1 = (int)floor(P.x + .3), z0 = (int)floor(P.z - .3), z1 = (int)floor(P.z + .3), y0 = (int)floor(P.y + .001), y1 = (int)floor(P.y + 1.79);
    for (int x = x0; x <= x1; x++) for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) if (getB(x, y, z) == web) return true;
    return false;
}
static void mechSlow(Phys& P) {
    static int soul = -2; if (soul == -2) soul = mechId("block/soul_sand");
    if (P.flying || P.noclip) return;
    if (physInWeb(P)) { P.mx *= .25; P.my *= .05; P.mz *= .25; }
    if (soul >= 0 && P.onGround && getB((int)floor(P.x), (int)floor(P.y) - 1, (int)floor(P.z)) == soul) { P.mx *= .4; P.mz *= .4; }
}
static void moveEntityWithHeading(Phys& P, float yawDeg, float strafe, float forward) {
    mechSlow(P);
    if (P.inLava && !P.flying) {   // lava: movimento lento (x0.5), pode-se subir pulando
        const double d0 = P.y;
        moveFlying(P, yawDeg, strafe, forward, .02f);
        moveEntity(P, P.mx, P.my, P.mz);
        P.mx *= .5; P.my *= .5; P.mz *= .5;
        P.my -= .02;
        if (P.collH && collEmpty(physBox(P).offset(P.mx, P.my + .6 - P.y + d0, P.mz)) && !anyLiquid(physBox(P).offset(P.mx, P.my + .6 - P.y + d0, P.mz)) && !anyLava(physBox(P).offset(P.mx, P.my + .6 - P.y + d0, P.mz)))
            P.my = .3;
    } else if (P.inWater && !P.flying) {
        const double d0 = P.y;
        moveFlying(P, yawDeg, strafe, forward, .02f);
        moveEntity(P, P.mx, P.my, P.mz);
        P.mx *= .8; P.my *= .8; P.mz *= .8;
        P.my -= .02;
        if (P.collH && collEmpty(physBox(P).offset(P.mx, P.my + .6 - P.y + d0, P.mz)) && !anyLiquid(physBox(P).offset(P.mx, P.my + .6 - P.y + d0, P.mz)))
            P.my = .3;
    } else if (P.elytra && !P.flying) {   // EntityLivingBase.travel (1.12.2): planar com elitra
        const double lx = gElyLook[0], ly = gElyLook[1], lz = gElyLook[2];
        const float f = gElyPitchRad;
        const double d6 = sqrt(lx * lx + lz * lz), d8 = sqrt(P.mx * P.mx + P.mz * P.mz), d1 = sqrt(lx * lx + ly * ly + lz * lz);
        float f4 = cosf(f); f4 = (float)((double)f4 * (double)f4 * std::min(1.0, d1 / .4));
        P.my += -.08 + (double)f4 * .06;
        if (P.my < 0 && d6 > 0) { const double d2 = P.my * -.1 * (double)f4; P.my += d2; P.mx += lx * d2 / d6; P.mz += lz * d2 / d6; }
        if (f < 0.f && d6 > 0) { const double d9 = d8 * (double)(-sinf(f)) * .04; P.my += d9 * 3.2; P.mx -= lx * d9 / d6; P.mz -= lz * d9 / d6; }
        if (d6 > 0) { P.mx += (lx / d6 * d8 - P.mx) * .1; P.mz += (lz / d6 * d8 - P.mz) * .1; }
        P.mx *= .9900000095367432; P.my *= .9800000190734863; P.mz *= .9900000095367432;
        moveEntity(P, P.mx, P.my, P.mz);
        if (P.collH) { const double d10 = sqrt(P.mx * P.mx + P.mz * P.mz); const float f5 = (float)((d8 - d10) * 10.0 - 3.0); if (f5 > 0.f) gElyWallDmg += f5; }   // bater na parede: energia cinetica
    } else {
        const float slip = groundSlip(P);
        float f4 = .91f;
        if (P.onGround) f4 = slip * .91f;
        const float f = .16277136f / (f4 * f4 * f4);
        const float landMove = (P.sprint ? .13f : .1f) * gFxMoveMul;   // atributo de velocidade 0.1 (+30% correndo)
        const float f5 = P.onGround ? landMove * f : (P.flying ? (P.sprint ? .1f : .05f) : (P.sprint ? .025999999f : .02f));   // voando: flySpeed 0.05 (x2 correndo)
        moveFlying(P, yawDeg, strafe, forward, f5);
        f4 = .91f;
        if (P.onGround) f4 = groundSlip(P) * .91f;
        const double flyY = P.my;
        moveEntity(P, P.mx, P.my, P.mz);
        P.my -= .08;
        P.my *= .9800000190734863;
        if (P.flying) P.my = flyY * .6;   // EntityPlayer: motionY = d3 * 0.6 enquanto voa
        P.mx *= f4; P.mz *= f4;
    }
}
