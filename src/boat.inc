// ================= bote: EntityBoat + ModelBoat + RenderBoat do Minecraft 1.12.2 =================
// Item = os itens "item/<madeira>_boat" que já existem no criativo (aba Transporte, ids do pack). Clique direito com o item: o bote nasce na água
// (raio de 5 blocos, só em fonte de água ou bloco sólido, como o ItemBoat). Clique direito no bote: monta. Shift: desmonta. Clique esquerdo: bate.
// A física/entrada/dano é a tradução direta de EntityBoat (1.12.2). Para não errar sinais, a SIMULAÇÃO usa o yaw do Minecraft (0 = sul/+Z, cresce no
// sentido horário); só o desenho converte: yaw_nosso = 180 - yaw_mc.
// Modelo: ModelBoat (caixas, pivôs e UVs do 1.12.2; textura 128x64 entity/boat/<madeira>.png). Mapeamento Java -> nosso: x = -x, y = -y, z = z
// (igual ao scale(-1,-1,1) do RenderBoat); ângulos X e Y trocam de sinal, Z não.
#include <deque>
enum { BOAT_OAK, BOAT_SPRUCE, BOAT_BIRCH, BOAT_JUNGLE, BOAT_ACACIA, BOAT_DARK_OAK, BOAT_N };
static const char* BOAT_NAME[BOAT_N] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
enum { BS_IN_WATER, BS_UNDER_WATER, BS_UNDER_FLOWING, BS_ON_LAND, BS_IN_AIR, BS_NONE };
static const double BOAT_W = 1.375, BOAT_H = .5625;
struct BNState { double x, y, z; float yaw, damage; uint8_t hit, flags, status; int8_t fwd, rider; };   // estado de rede do bote (NP_BOAT)
struct BoatEnt {
    int id = 0, type = 0; bool dead = false;
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0, mx = 0, my = 0, mz = 0;
    float yaw = 0, pyaw = 0;   // yaw do Minecraft (contínuo, sem normalizar)
    int status = BS_NONE, prevStatus = BS_NONE, timeSinceHit = 0, fwdDir = 1;
    float damage = 0, momentum = 0, outOfControl = 0, deltaRot = 0, glide = 0, fallDist = 0, paddlePos[2] = {0, 0}, paddleAmp[2] = {0, 0};   // paddleAmp 0..1: o remo acelera e desacelera suave (sem parar de repente)
    double waterLevel = 0, lastYd = 0;
    bool paddle[2] = {false, false};
    // --- multiplayer: autoridade = quem monta (rider = id de rede, -1 = ninguem -> o host simula). Os demais so recebem estados (fila) e interpolam.
    int rider = -1, sentT = 0, age = 0; std::deque<BNState> q;
    // --- animacao visual (so desenho, nunca entra na fisica): balanço por onda, inclinacao em curva/aceleracao; calculada igual em todos os clientes
    float roll = 0, pRoll = 0, pitchV = 0, pPitch = 0, wet = 0, lastFwd = 0, phase = 0;
};
static std::vector<BoatEnt> BOATS; static int gBoatNext = 0;   // gBoatRide (physics.inc) = id do bote em que o jogador está (0 = nenhum)
static AABB boatBox(const BoatEnt& b) { const double h = BOAT_W * .5; return {b.x - h, b.y, b.z - h, b.x + h, b.y + BOAT_H, b.z + h}; }
static void boatCollect(const AABB& r, std::vector<AABB>& out, int skipId) {   // EntityBoat.getCollisionBox: jogador, animais e outros botes batem no casco e podem ficar em cima
    for (const BoatEnt& e : BOATS) { if (e.dead || e.id == skipId) continue; const AABB a = boatBox(e); if (r.intersects(a)) out.push_back(a); }
}
static BoatEnt* boatById(int id) { if (id) for (BoatEnt& b : BOATS) if (b.id == id) return &b; return nullptr; }
static void boatNetSend(const BoatEnt& b); static void boatNetDel(int id); static void boatNetHit(int id, float dmg, bool creative); static bool boatNetAlive(int id);   // net.inc
static bool boatAuth(const BoatEnt& b) { return !netOn || (b.rider >= 0 ? b.rider : 0) == netSelf; }   // quem simula este bote (sem rede: sempre nos)
static const BoatEnt* boatOfRider(int netId) { if (netId >= 0) for (const BoatEnt& b : BOATS) if (!b.dead && b.rider == netId) return &b; return nullptr; }
static float boatYawO(const BoatEnt& b) { float a = fmodf(180.f - b.yaw, 360.f); if (a > 180.f) a -= 360.f; if (a < -180.f) a += 360.f; return a; }   // yaw no sistema do jogo

static float boatYawLerp(const BoatEnt& b, float pt) {   // yaw do bote (sistema do jogo) interpolado entre ticks, p/ a camera
    float a = fmodf(180.f - (b.pyaw + (b.yaw - b.pyaw) * pt), 360.f); if (a > 180.f) a -= 360.f; if (a < -180.f) a += 360.f; return a;
}
static float gBoatCamLast = 0.f; static int gBoatCamId = 0;   // camera do bote: ultimo yaw do bote visto / id do bote em que a camera esta presa

static int boatTypeOfItem(int id) {
    if (!isPk(id)) return -1;
    const std::string& f = PK[id - PK0].file;
    for (int t = 0; t < BOAT_N; t++) if (f == std::string("item/") + BOAT_NAME[t] + "_boat") return t;
    return -1;
}
static int boatItemId(int t) { for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == std::string("item/") + BOAT_NAME[t] + "_boat") return PK0 + (int)i; return 0; }
static float boatSlip(int b) {   // Block.slipperiness
    if (!isPk(b)) return .6f;
    const std::string& f = PK[b - PK0].file;
    return f == "block/ice" || f == "block/packed_ice" ? .98f : f == "block/blue_ice" ? .989f : .6f;
}
static float boatLiq(int x, int y, int z) {   // BlockLiquid.getLiquidHeight (absoluta)
    if (isWater(getB(x, y + 1, z))) return (float)y + 1.f;
    int m = liqMeta(getB(x, y, z)); if (m >= 8) m = 0;
    return (float)y + 1.f - (m + 1) / 9.f;
}
static int boatUnderwater(const BoatEnt& b) {   // EntityBoat.getUnderwaterStatus (-1 = nenhum)
    const AABB a = boatBox(b); const double d0 = a.y1 + .001; bool flag = false;
    for (int y = (int)floor(a.y1); y < (int)ceil(d0); y++) for (int z = (int)floor(a.z0); z < (int)ceil(a.z1); z++) for (int x = (int)floor(a.x0); x < (int)ceil(a.x1); x++) {
        const int bl = getB(x, y, z);
        if (isWater(bl) && d0 < boatLiq(x, y, z)) { if (liqMeta(bl) != 0) return BS_UNDER_FLOWING; flag = true; }
    }
    return flag ? BS_UNDER_WATER : -1;
}
static bool boatCheckWater(BoatEnt& b) {   // EntityBoat.checkInWater
    const AABB a = boatBox(b); bool f = false; b.waterLevel = -1e300;
    for (int y = (int)floor(a.y0); y < (int)ceil(a.y0 + .001); y++) for (int z = (int)floor(a.z0); z < (int)ceil(a.z1); z++) for (int x = (int)floor(a.x0); x < (int)ceil(a.x1); x++)
        if (isWater(getB(x, y, z))) { const float h = boatLiq(x, y, z); b.waterLevel = std::max(b.waterLevel, (double)h); f |= a.y0 < h; }
    return f;
}
static float boatWaterAbove(const BoatEnt& b) {   // EntityBoat.getWaterLevelAbove
    const AABB a = boatBox(b); const int l = (int)ceil(a.y1 - b.lastYd);
    for (int y = (int)floor(a.y1); y < l; y++) {
        float f = 0.f;
        for (int z = (int)floor(a.z0); z < (int)ceil(a.z1) && f < 1.f; z++) for (int x = (int)floor(a.x0); x < (int)ceil(a.x1); x++)
            if (isWater(getB(x, y, z))) f = std::max(f, boatLiq(x, y, z) - (float)y);
        if (f < 1.f) return (float)y + f;
    }
    return (float)(l + 1);
}
static float boatGlide(const BoatEnt& b) {   // EntityBoat.getBoatGlide: média da derrapagem dos blocos sob o casco (0 = nada embaixo)
    const AABB a = boatBox(b); const int yy = (int)floor(a.y0 - .001); float f = 0.f; int n = 0;
    for (int z = (int)floor(a.z0); z < (int)ceil(a.z1); z++) for (int x = (int)floor(a.x0); x < (int)ceil(a.x1); x++) { const int bl = getB(x, yy, z); if (solid(bl)) { f += boatSlip(bl); n++; } }
    return n ? f / n : 0.f;
}
static void boatPush(BoatEnt& b) {   // Entity.handleWaterMovement: correnteza empurra 0.014
    const AABB a = boatBox(b); double ax = 0, az = 0; const double y0 = a.y0 + .4, y1 = a.y1 - .4;
    for (int y = (int)floor(std::min(y0, y1)); y < (int)floor(std::max(y0, y1) + 1); y++) for (int z = (int)floor(a.z0 + .001); z < (int)floor(a.z1 - .001 + 1); z++) for (int x = (int)floor(a.x0 + .001); x < (int)floor(a.x1 - .001 + 1); x++) {
        const int bl = getB(x, y, z); if (!isWater(bl)) continue;
        const double top = y + 1 - ((liqMeta(bl) >= 8 ? 0 : liqMeta(bl)) + 1) / 9.0;
        if (ceil(y1) < top) continue;
        double fx, fz; flowVector(x, y, z, fx, fz);
        if (liqMeta(bl) >= 8) { const double l = sqrt(fx * fx + fz * fz); if (l > 0) { fx /= l; fz /= l; } }
        ax += fx; az += fz;
    }
    const double len = sqrt(ax * ax + az * az); if (len > 0) { b.mx += ax / len * .014; b.mz += az / len * .014; }
}
static void boatEject(BoatEnt& b) { if (gBoatRide == b.id) gBoatRide = 0; for (MobEnt& m : MOBS) if (m.rideBoat == b.id) { m.rideBoat = 0; m.my = 0; } }   // animais sentados descem junto
static void boatBreak(BoatEnt& b, bool drop) { if (drop) spawnDrop(b.x, b.y + .3, b.z, 0, .15, 0, boatItemId(b.type), 1, 10); boatEject(b); if (netOn && !b.dead) boatNetDel(b.id); b.dead = true; }

static void boatMove(BoatEnt& b, double mx, double my, double mz) {   // Entity.move (sem degrau) + EntityBoat.updateFallState
    AABB bb = boatBox(b); double x = mx, y = my, z = mz; const double d6 = x, d7 = y, d8 = z;
    std::vector<AABB> list; collBoxes(bb.addCoord(x, y, z), list); boatCollect(bb.addCoord(x, y, z), list, b.id);   // blocos + outros botes
    for (auto& c : list) y = c.calcY(bb, y);
    bb = bb.offset(0, y, 0);
    for (auto& c : list) x = c.calcX(bb, x);
    bb = bb.offset(x, 0, 0);
    for (auto& c : list) z = c.calcZ(bb, z);
    bb = bb.offset(0, 0, z);
    b.x = (bb.x0 + bb.x1) / 2; b.y = bb.y0; b.z = (bb.z0 + bb.z1) / 2;
    const bool onGround = d7 != y && d7 < 0;
    b.lastYd = b.my;
    if (onGround) {
        if (b.fallDist > 3.f) {
            if (b.status != BS_ON_LAND) b.fallDist = 0.f;
            else { for (int i = 0; i < 3; i++) spawnDrop(b.x, b.y + .3, b.z, 0, .1, 0, PLANKS, 1, 10); spawnDrop(b.x, b.y + .3, b.z, 0, .1, 0, IT_STICK, 2, 10); boatBreak(b, false); }
        }
        b.fallDist = 0.f;
    } else if (!isWater(getB((int)floor(b.x), (int)floor(b.y) - 1, (int)floor(b.z))) && y < 0) b.fallDist -= (float)y;
    if (d6 != x) b.mx = 0; if (d8 != z) b.mz = 0; if (d7 != y) b.my = 0;
}
static void boatPaddleStep(BoatEnt& b) {   // fase do remo avança proporcional à amplitude: ao soltar a tecla ele vai perdendo força até parar
    for (int i = 0; i < 2; i++) {
        const float t = b.paddle[i] ? 1.f : 0.f; b.paddleAmp[i] += (t - b.paddleAmp[i]) * (t > 0.f ? .35f : .1f);
        if (!b.paddle[i] && b.paddleAmp[i] < .03f) { b.paddleAmp[i] = 0.f; b.paddlePos[i] = 0.f; } else b.paddlePos[i] += .39269909f * b.paddleAmp[i];
    }
}
static void boatTick(BoatEnt& b, bool L, bool R, bool F, bool B, bool riden) {   // EntityBoat.onUpdate
    b.prevStatus = b.status;
    const int us = boatUnderwater(b);
    if (us >= 0) { b.waterLevel = boatBox(b).y1; b.status = us; }
    else if (boatCheckWater(b)) b.status = BS_IN_WATER;
    else { const float g = boatGlide(b); if (g > 0.f) { b.glide = g; b.status = BS_ON_LAND; } else b.status = BS_IN_AIR; }
    if (b.status != BS_UNDER_WATER && b.status != BS_UNDER_FLOWING) b.outOfControl = 0.f; else b.outOfControl += 1.f;
    if (riden && b.outOfControl >= 60.f) boatEject(b);
    if (b.timeSinceHit > 0) --b.timeSinceHit;
    if (b.damage > 0.f) b.damage -= 1.f;
    b.px = b.x; b.py = b.y; b.pz = b.z; b.pyaw = b.yaw;
    if (b.y < -64) { boatBreak(b, false); return; }
    boatPush(b);
    if (!riden) b.paddle[0] = b.paddle[1] = false;
    // updateMotion
    double d1 = -.03999999910593033, d2 = 0; b.momentum = .05f;
    if (b.prevStatus == BS_IN_AIR && b.status != BS_IN_AIR && b.status != BS_ON_LAND) {
        b.waterLevel = b.y + BOAT_H; b.y = boatWaterAbove(b) - BOAT_H + .101; b.my = 0; b.lastYd = 0; b.status = BS_IN_WATER;
    } else {
        if (b.status == BS_IN_WATER) { d2 = (b.waterLevel - b.y) / BOAT_H; b.momentum = .9f; }
        else if (b.status == BS_UNDER_FLOWING) { d1 = -7e-4; b.momentum = .9f; }
        else if (b.status == BS_UNDER_WATER) { d2 = .009999999776482582; b.momentum = .45f; }
        else if (b.status == BS_IN_AIR) b.momentum = .9f;
        else if (b.status == BS_ON_LAND) { b.momentum = b.glide; if (riden) b.glide /= 2.f; }
        b.mx *= b.momentum; b.mz *= b.momentum; b.deltaRot *= b.momentum; b.my += d1;
        if (d2 > 0) { b.my += d2 * .06153846016296973; b.my *= .75; }
    }
    if (riden) {   // controlBoat
        float f = 0.f;
        if (L) b.deltaRot -= 1.f;
        if (R) b.deltaRot += 1.f;
        if (R != L && !F && !B) f += .005f;
        b.yaw += b.deltaRot;
        if (F) f += .04f;
        if (B) f -= .005f;
        b.mx += sin(-b.yaw * .017453292) * f; b.mz += cos(b.yaw * .017453292) * f;
        b.paddle[0] = (R && !L) || F; b.paddle[1] = (L && !R) || F;
    }
    boatMove(b, b.mx, b.my, b.mz);
    boatPaddleStep(b);
}

// ---------------- animacao visual + sincronizacao (multiplayer) ----------------
static float wrapDeg(float a) { a = fmodf(a, 360.f); if (a > 180.f) a -= 360.f; if (a < -180.f) a += 360.f; return a; }
static void boatVisTick(BoatEnt& b) {   // 1x por tick, igual para bote local e remoto: so olha o deslocamento (nao depende da fisica interna)
    b.age++; b.pRoll = b.roll; b.pPitch = b.pitchV;
    const float yr = wrapDeg(b.yaw - b.pyaw), r = b.yaw * .017453292f;   // graus/tick
    const float fwd = (float)((b.x - b.px) * -sin(r) + (b.z - b.pz) * cos(r));   // velocidade para a frente (blocos/tick)
    const float acc = fwd - b.lastFwd; b.lastFwd = fwd;
    const bool inW = b.status == BS_IN_WATER || b.status == BS_UNDER_WATER || b.status == BS_UNDER_FLOWING;
    b.wet += ((inW ? 1.f : 0.f) - b.wet) * .2f;
    const float rollT = std::max(-6.f, std::min(6.f, yr * 2.4f * std::min(1.f, .35f + fwd * 6.f))) * b.wet;   // casco inclina para fora da curva
    const float pitT = std::max(-4.f, std::min(4.f, acc * 110.f + fwd * 6.f)) * b.wet;   // proa sobe ao acelerar e na velocidade, desce ao frear
    b.roll += (rollT - b.roll) * .25f; b.pitchV += (pitT - b.pitchV) * .2f;
}
static void boatVisOff(const BoatEnt& b, float pt, float& pitch, float& roll, float& bob) {   // graus / blocos, ja interpolados
    const float a = (float)b.age + pt;
    pitch = b.pPitch + (b.pitchV - b.pPitch) * pt + sinf(a * .11f + b.phase * 1.7f) * .8f * b.wet;
    roll = b.pRoll + (b.roll - b.pRoll) * pt + sinf(a * .15f + b.phase) * 1.1f * b.wet;
    bob = (sinf(a * .21f + b.phase) * .016f + sinf(a * .53f + b.phase * 2.f) * .005f) * b.wet;
}
static void boatApplyState(BoatEnt& b, const BNState& s) {
    b.x = s.x; b.y = s.y; b.z = s.z; b.yaw = b.pyaw + wrapDeg(s.yaw - b.pyaw); b.damage = s.damage; b.timeSinceHit = s.hit; b.fwdDir = s.fwd ? s.fwd : 1;
    b.paddle[0] = s.flags & 1; b.paddle[1] = (s.flags >> 1) & 1; b.status = s.status; b.rider = s.rider;
}
static void boatRemoteTick(BoatEnt& b) {   // bote que outro cliente simula: consome 1 estado por tick (fila curta, como os jogadores remotos)
    b.px = b.x; b.py = b.y; b.pz = b.z; b.pyaw = b.yaw; b.prevStatus = b.status;
    while (b.q.size() > 2) b.q.pop_front();
    if (!b.q.empty()) { const BNState s = b.q.front(); b.q.pop_front(); const int keep = b.rider; boatApplyState(b, s); b.rider = keep; }
    else { if (b.timeSinceHit > 0) --b.timeSinceHit; if (b.damage > 0.f) b.damage -= 1.f; }
    boatPaddleStep(b);
    boatVisTick(b);
}
static void boatNetPush(BoatEnt& e) {   // dono do bote: manda o estado todo tick se montado/em movimento; parado so a cada 1 s (quem entra depois aprende os botes por aqui)
    if (!netOn) return;
    const bool mov = e.rider == netSelf || fabs(e.x - e.px) + fabs(e.y - e.py) + fabs(e.z - e.pz) + fabs(e.yaw - e.pyaw) > 1e-4 || e.timeSinceHit > 0 || e.damage > 0.f || e.paddle[0] || e.paddle[1];
    if (mov || ++e.sentT >= 20) { e.sentT = 0; boatNetSend(e); }
}
// estado recebido de 'sender'. Cria o bote se ainda nao existe. Disputa de montaria: menor id de rede vence.
static void boatNetApply(int id, int type, const BNState& s) {
    BoatEnt* b = boatById(id);
    if (!b) {
        if (type < 0 || type >= BOAT_N) return;
        BoatEnt n; n.id = id; n.type = type; n.x = n.px = s.x; n.y = n.py = s.y; n.z = n.pz = s.z; n.yaw = n.pyaw = s.yaw; n.phase = (float)(id & 255) * .37f;
        BOATS.push_back(n); b = &BOATS.back();
    }
    if (gBoatRide == id) {
        if (s.rider < 0 || s.rider == netSelf || s.rider > netSelf) return;   // estado velho/eco, ou perdi o empate para quem tem id menor... (o outro desmonta ao ver o meu)
        gBoatRide = 0;   // perdi a disputa
    }
    if (s.rider >= 0 && s.rider != netSelf && b->rider == netSelf) b->rider = -1;
    if (boatAuth(*b) && s.rider < 0) {   // devolvido ao host: continua dali (sem velocidade conhecida)
        b->x = b->px = s.x; b->y = b->py = s.y; b->z = b->pz = s.z; b->yaw = b->pyaw = s.yaw; b->mx = b->my = b->mz = 0; b->q.clear(); return;
    }
    b->rider = s.rider; b->type = type; b->q.push_back(s); if (b->q.size() > 6) b->q.pop_front();
}
// Assentos (EntityBoat.updatePassenger): sozinho = centro; em dupla o 1o vai a +0,2 e o 2o a -0,6 (animal +0,2). O jogador é sempre o 1o.
static double boatSeat(int count, int idx, bool animal) { double f = 0; if (count > 1) { f = idx == 0 ? .2 : -.6; if (animal) f += .2; } return f; }
static const double BOAT_SEAT_Y = -.30;   // pés do jogador em relação ao bote (MC: -0,45; subi 0,15 para o quadril apoiar no piso e as pernas não atravessarem o casco)
static void boatSeatAll(Phys& P) {   // posiciona os passageiros; chamado depois de mover os botes
    for (BoatEnt& b : BOATS) {
        if (b.dead) continue;
        std::vector<MobEnt*> ms; for (MobEnt& m : MOBS) if (m.rideBoat == b.id && !m.dead) ms.push_back(&m);
        const bool pl = gBoatRide == b.id; const int n = (pl ? 1 : 0) + (int)ms.size(); int idx = 0;
        const float yo = boatYawO(b), r = yo * .0174533f; const double fx = -sin(r), fz = -cos(r);
        if (pl) { const double f = boatSeat(n, idx++, false); P.x = b.x + fx * f; P.y = b.y + BOAT_SEAT_Y; P.z = b.z + fz * f; }
        for (MobEnt* m : ms) {
            const double f = boatSeat(n, idx++, true); m->x = b.x + fx * f; m->y = b.y + .15; m->z = b.z + fz * f;
            m->yaw = yo; m->mx = m->my = m->mz = 0;   // de frente, como o bote
        }
    }
}
static void boatBoard() {   // EntityBoat.applyEntityCollision: animal que encosta no casco (+0,2) senta, se houver lugar (máx. 2) e couber (largura < 1,375)
    for (BoatEnt& b : BOATS) {
        if (b.dead) continue;
        int n = gBoatRide == b.id ? 1 : 0; for (const MobEnt& m : MOBS) if (m.rideBoat == b.id && !m.dead) n++;
        const AABB a = boatBox(b), ex = {a.x0 - .2, a.y0, a.z0 - .2, a.x1 + .2, a.y1, a.z1 + .2};
        for (MobEnt& m : MOBS) {
            if (n >= 2) break;
            if (m.dead || m.rideBoat || MD[m.type].w >= BOAT_W || !ex.intersects(mobBox(m))) continue;
            m.rideBoat = b.id; m.hasT = false; m.panic = 0; n++;
        }
    }
}
// chamado 1x por tick (depois da entrada do jogador). Se o jogador estiver montado, prende o corpo ao assento e devolve true.
static bool boatTickAll(Phys& P, bool L, bool R, bool F, bool B, bool sneak) {
    for (MobEnt& m : MOBS) if (m.rideBoat && !boatById(m.rideBoat)) m.rideBoat = 0;   // bote sumiu (ex.: /bote limpar): animal desce
    for (size_t i = 0; i < BOATS.size(); i++) {
        BoatEnt& e = BOATS[i]; const bool r = e.id == gBoatRide;
        if (r) e.rider = netSelf; else if (e.rider == netSelf) { e.rider = -1; if (netOn) boatNetSend(e); }   // desmontou: devolve a autoridade ao host
        if (netOn && e.rider >= 0 && e.rider != netSelf && !boatNetAlive(e.rider)) e.rider = -1;   // quem montava saiu do jogo
        if (!boatAuth(e)) { boatRemoteTick(e); continue; }
        if (!r && !getC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { e.px = e.x; e.py = e.y; e.pz = e.z; continue; }   // chunk ainda não carregado (ex.: ao entrar no mundo): congela, senão cai no vazio e quebra
        boatTick(e, L && r, R && r, F && r, B && r, r); boatVisTick(e); boatNetPush(e);
    }
    for (size_t i = BOATS.size(); i-- > 0;) if (BOATS[i].dead) BOATS.erase(BOATS.begin() + i);
    boatBoard();
    BoatEnt* b = boatById(gBoatRide);
    if (!b) gBoatRide = 0;
    else if (sneak) {   // desmonta em cima do bote (Entity.dismountRidingEntity)
        P.x = b->x; P.y = b->y + BOAT_H; P.z = b->z; P.mx = P.my = P.mz = 0; P.px = P.x; P.py = P.y; P.pz = P.z; gBoatRide = 0;
    }
    boatSeatAll(P);
    if (!boatById(gBoatRide)) return false;
    P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false;
    return true;
}
// ---------------- salvar / carregar (saves/<mundo>/boats.dat) ----------------
static void boatsSave(const std::string& dir) {
    FILE* f = fopen(("saves/" + dir + "/boats.dat").c_str(), "wb"); if (!f) return;
    fprintf(f, "BOATS1\n");   // (mobs ficam em mobs.dat logo abaixo)
    for (const BoatEnt& b : BOATS) if (!b.dead) fprintf(f, "b %d %d %.9g %.9g %.9g %.9g %.4g %d\n", b.id, b.type, b.x, b.y, b.z, (double)b.yaw, (double)b.damage, b.id == gBoatRide ? 1 : 0);
    fclose(f);
    // mobs (inclusive os sentados nos botes): antes nao eram salvos e sumiam ao sair/entrar no mundo
    FILE* m = fopen(("saves/" + dir + "/mobs.dat").c_str(), "wb"); if (!m) return;
    fprintf(m, "MOBS1\n");
    for (const MobEnt& e : MOBS) if (!e.dead && e.hp > 0) fprintf(m, "m %d %.9g %.9g %.9g %.4g %d %d\n", e.type, e.x, e.y, e.z, (double)e.yaw, e.hp, e.rideBoat);
    fclose(m);
}
static void mobsLoadFile(const std::string& dir) {
    FILE* f = fopen(("saves/" + dir + "/mobs.dat").c_str(), "rb"); if (!f) return;
    char ln[256];
    while (fgets(ln, sizeof ln, f)) {
        int t = 0, hp = 1, ride = 0; double x, y, z, yw;
        if (ln[0] != 'm' || sscanf(ln, "m %d %lf %lf %lf %lf %d %d", &t, &x, &y, &z, &yw, &hp, &ride) < 7 || t < 0 || t >= M_N || hp <= 0) continue;
        spawnMob(t, x, y, z, (float)yw); MOBS.back().hp = std::min(hp, MD[t].hp); MOBS.back().rideBoat = ride; MOBS.back().ground = true;
    }
    fclose(f);
}
static void boatsLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    mobsLoadFile(dir);
    FILE* f = fopen(("saves/" + dir + "/boats.dat").c_str(), "rb"); if (!f) return;
    char ln[256];
    while (fgets(ln, sizeof ln, f)) {
        if (ln[0] != 'b') continue;
        int id = 0, t = 0, ride = 0; double x, y, z, yw, dm;
        if (sscanf(ln, "b %d %d %lf %lf %lf %lf %lf %d", &id, &t, &x, &y, &z, &yw, &dm, &ride) < 7 || id <= 0 || t < 0 || t >= BOAT_N) continue;
        BoatEnt b; b.id = id; b.type = t; b.x = b.px = x; b.y = b.py = y; b.z = b.pz = z; b.yaw = b.pyaw = (float)yw; b.damage = (float)dm;
        b.phase = (float)(id & 255) * .37f; BOATS.push_back(b); gBoatNext = std::max(gBoatNext, id & 0xFFFFF); if (ride) gBoatRide = id;
    }
    fclose(f);
}
static int boatPick(const float* eye, const float* dir, float reach) {   // bote mais próximo na mira (ignora o que o jogador está montando)
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)BOATS.size(); i++) {
        const BoatEnt& e = BOATS[i]; if (e.id == gBoatRide) continue;
        const AABB a = boatBox(e); const float lo[3] = {(float)a.x0 - .1f, (float)a.y0 - .1f, (float)a.z0 - .1f}, hi[3] = {(float)a.x1 + .1f, (float)a.y1 + .1f, (float)a.z1 + .1f};
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
static bool boatInteract(const float* eye, const float* dir, float reach) {   // clique direito no bote: monta (EntityBoat.processInteract)
    const int i = boatPick(eye, dir, reach); if (i < 0) return false;
    BoatEnt& b = BOATS[i];
    if (b.outOfControl < 60.f && (b.rider < 0 || b.rider == netSelf)) { gBoatRide = b.id; b.rider = netSelf; b.q.clear(); b.px = b.x; b.py = b.y; b.pz = b.z; b.pyaw = b.yaw; if (netOn) boatNetSend(b); }   // assume a autoridade; o host para de simular e passa a receber os estados deste cliente
    return true;
}
static void boatApplyHit(BoatEnt& b, float dmg, bool creative) {   // EntityBoat.attackEntityFrom
    b.fwdDir = -b.fwdDir; b.timeSinceHit = 10; b.damage += dmg;
    if (creative) boatBreak(b, false); else if (b.damage > 40.f) boatBreak(b, true);
}
static bool boatHit(const float* eye, const float* dir, int held, bool creative, float reach) {   // EntityBoat.attackEntityFrom
    const int i = boatPick(eye, dir, reach); if (i < 0) return false;
    BoatEnt& b = BOATS[i]; const float dmg = mobWeaponDmg(held) * 10.f;
    if (boatAuth(b)) boatApplyHit(b, dmg, creative); else boatNetHit(b.id, dmg, creative);   // o dono do bote aplica o dano e todos veem o balanço no proximo estado
    return true;
}
static void boatSpawn(int type, double x, double y, double z, float yawMc) {
    BoatEnt b; b.id = (netSelf << 20) | (++gBoatNext & 0xFFFFF); b.phase = (float)(b.id & 255) * .37f; b.type = type; b.x = b.px = x; b.y = b.py = y; b.z = b.pz = z; b.yaw = b.pyaw = yawMc; BOATS.push_back(b);
}
static bool boatPlace(int type, const float* eye, const float* dir, float yawO, float reach) {   // ItemBoat.onItemRightClick
    int c[3] = {0, 0, 0}; float tHit = 0.f; bool hit = false;
    for (float t = 0.f; t <= reach && !hit; t += .02f) {
        for (int k = 0; k < 3; k++) c[k] = (int)floorf(eye[k] + dir[k] * t);
        const int bl = getB(c[0], c[1], c[2]);
        if ((isWater(bl) && liqMeta(bl) == 0) || solid(bl)) { hit = true; tHit = t; }
    }
    if (!hit) return false;
    float t0 = 0.f;   // ponto exato de entrada na célula
    for (int k = 0; k < 3; k++) if (fabsf(dir[k]) > 1e-6f) t0 = std::max(t0, ((dir[k] > 0 ? c[k] : c[k] + 1) - eye[k]) / dir[k]);
    t0 = std::min(t0, tHit);
    const double x = eye[0] + dir[0] * t0, z = eye[2] + dir[2] * t0, y = eye[1] + dir[1] * t0 - (isWater(getB(c[0], c[1], c[2])) ? .12 : 0.);
    std::vector<AABB> l; collBoxes(AABB{x - .5875, y + .1, z - .5875, x + .5875, y + .4625, z + .5875}, l);   // caixa -0.1: não nasce dentro de bloco
    if (!l.empty()) return false;
    boatSpawn(type, x, y, z, 180.f - yawO);
    return true;
}
static void boatSpawnFront(int type, const Phys& P, float yawO) {   // /bote: 3 blocos à frente, na superfície da água ou no chão
    const float r = yawO * .0174533f; const double x = P.x - sin(r) * 3.0, z = P.z - cos(r) * 3.0; double y = P.y;
    for (int by = (int)floor(P.y) + 3; by >= 0; by--) { const int bl = getB((int)floor(x), by, (int)floor(z)); if (isWater(bl)) { y = boatLiq((int)floor(x), by, (int)floor(z)) - .12; break; } if (solid(bl)) { y = by + 1.0; break; } }
    boatSpawn(type, x, y, z, 180.f - yawO);
}

// ---------------- modelo e desenho ----------------
static std::vector<V> bmSide[5], bmPad[2], bmMask;
static GLuint boatTex[BOAT_N]; static bool boatReady = false;
static void loadBoatModel() {   // precisa de contexto GL e do pack aberto; chamado ao desenhar o 1º bote
    boatReady = true;
    static const int FB[BOAT_N][3] = {{160, 130, 80}, {110, 80, 50}, {200, 190, 140}, {150, 110, 80}, {170, 95, 55}, {65, 45, 25}};
    int fromPack = 0;
    for (int t = 0; t < BOAT_N; t++) {
        std::vector<uint8_t> px; int w = 0, h = 0; bool ok = false;
        std::vector<std::string> names = {std::string("entity/boat/") + BOAT_NAME[t] + ".png", std::string("entity/boat/boat_") + BOAT_NAME[t] + ".png"};
        if (t == 0) names.push_back("entity/boat.png");
        for (const std::string& n : names) if (!ok) ok = packRGBA(n, px, w, h) && w >= 64 && h * 2 == w;
        if (!ok) { w = 128; h = 64; px.assign(128 * 64 * 4, 0); for (int i = 0; i < 128 * 64; i++) { px[i * 4] = FB[t][0]; px[i * 4 + 1] = FB[t][1]; px[i * 4 + 2] = FB[t][2]; px[i * 4 + 3] = 255; } } else fromPack++;
        if (!boatTex[t]) glGenTextures(1, &boatTex[t]);
        glBindTexture(GL_TEXTURE_2D, boatTex[t]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    }
    for (auto& m : bmSide) m.clear(); bmPad[0].clear(); bmPad[1].clear(); bmMask.clear();
    // ModelBoat 1.12.2 (Java x,y -> nosso -x,-y): fundo, proa/popa/laterais, remos
    boxM(bmSide[0], -14, 14, -7, 9, -3, 0, 0, 0, 28, 16, 3, 0);
    boxM(bmSide[1], -5, 13, 1, 7, -1, 1, 0, 19, 18, 6, 2, 0);
    boxM(bmSide[2], -8, 8, 1, 7, -1, 1, 0, 27, 16, 6, 2, 0);
    boxM(bmSide[3], -14, 14, 1, 7, -1, 1, 0, 35, 28, 6, 2, 0);
    boxM(bmSide[4], -14, 14, 1, 7, -1, 1, 0, 43, 28, 6, 2, 0);
    boxM(bmMask, -14, 14, -7, 9, -3, 0, 0, 0, 28, 16, 3, 0);
    boxM(bmPad[0], -1, 1, -2, 0, -5, 13, 62, 0, 2, 2, 18, 0);   boxM(bmPad[0], .001f, 1.001f, -3, 3, 8, 15, 62, 0, 1, 6, 7, 0);
    boxM(bmPad[1], -1, 1, -2, 0, -5, 13, 62, 20, 2, 2, 18, 0);  boxM(bmPad[1], -1.001f, -.001f, -3, 3, 8, 15, 62, 20, 1, 6, 7, 0);
    for (auto* m : {&bmSide[0], &bmSide[1], &bmSide[2], &bmSide[3], &bmSide[4], &bmPad[0], &bmPad[1], &bmMask}) for (V& v : *m) v.u *= .5f;   // boxM normaliza por 64; o layout do bote tem 128 de largura
    printf("Barcos: texturas %d/%d do resource pack\n", fromPack, BOAT_N);
}
// Matriz do passageiro: parte do centro do casco (mesmo pivo do desenho do bote), aplica balanço/inclinação e desce ao assento.
// 'seat' (opcional) = posição interpolada do jogador (mantém o deslocamento lateral de quando há animal a bordo). Depois o chamador gira o corpo.
static void boatRiderBegin(const BoatEnt& e, float pt, const double* seat = nullptr) {
    float pi, ro, bo; boatVisOff(e, pt, pi, ro, bo);
    const double bx = e.px + (e.x - e.px) * pt, by = e.py + (e.y - e.py) * pt, bz = e.pz + (e.z - e.pz) * pt;
    const float yg = boatYawLerp(e, pt);
    glTranslatef((float)(seat ? seat[0] : bx), (float)by + .375f + bo, (float)(seat ? seat[2] : bz));
    glRotatef(yg, 0, 1, 0); glRotatef(pi, 1, 0, 0); glRotatef(ro, 0, 0, 1); glRotatef(-yg, 0, 1, 0);
    glTranslatef(0.f, (float)BOAT_SEAT_Y - .375f, 0.f);
}
static void boatRowPose(Pose& q, const BoatEnt& b, float pt) { q.oar = true; for (int i = 0; i < 2; i++) { const int k = 1 - i; q.rowAmp[i] = b.paddleAmp[k]; q.row[i] = b.paddlePos[k] + .3926991f * b.paddleAmp[k] * (pt - 1.f); } }   // fase = a do drawBoat (pt-1), braço e remo sincronizados; paddle[0] = remo esquerdo do modelo, row[0] = braço direito: cruza
static float boatLerp(double a, double b, double t) { return (float)(t < 0 ? a : t > 1 ? b : a + (b - a) * t); }   // MathHelper.clampedLerp
static void drawBoat(const BoatEnt& e, float pt) {
    const float R2D = 57.29578f, PI_ = 3.14159265f;
    auto L = [&](double a, double b) { return (float)(a + (b - a) * pt); };
    glBindTexture(GL_TEXTURE_2D, boatTex[e.type]);
    float vPit, vRol, vBob; boatVisOff(e, pt, vPit, vRol, vBob);   // balanço nas ondas + inclinação (curva/aceleração), só visual
    glPushMatrix(); glTranslatef(L(e.px, e.x), L(e.py, e.y) + .375f + vBob, L(e.pz, e.z));   // RenderBoat.setupTranslation
    glRotatef(180.f - L(e.pyaw, e.yaw), 0, 1, 0);
    const float f = e.timeSinceHit - pt; float f1 = e.damage - pt; if (f1 < 0.f) f1 = 0.f;
    if (f > 0.f) glRotatef(sinf(f) * f * f1 / 10.f * e.fwdDir, 1, 0, 0);   // balanço ao apanhar
    if (vPit != 0.f) glRotatef(vPit, 1, 0, 0);   // eixo X do referencial do casco = lateral (arfagem); Z = longitudinal (rolagem)
    if (vRol != 0.f) glRotatef(vRol, 0, 0, 1);
    glRotatef(-90.f, 0, 1, 0);   // scale(-1,-1,1) + rotate(90) do ModelBoat, já no nosso referencial
    part(bmSide[0], 0, -3, 1, -90.f);
    part(bmSide[1], 15, -4, 4, 0, 90.f); part(bmSide[2], -15, -4, 0, 0, -90.f); part(bmSide[3], 0, -4, -9, 0, 180.f); part(bmSide[4], 0, -4, 9);
    for (int i = 0; i < 2; i++) {   // ModelBoat.renderPaddle
        const float am = std::min(1.f, e.paddleAmp[i]), rt = e.paddlePos[i] + .3926991f * e.paddleAmp[i] * (pt - 1.f);   // am mistura a pose de repouso com a remada
        const float jx0 = boatLerp(-1.0471976, -.2617994, .5), jy0 = boatLerp(-.7853982, .7853982, (sinf(1.f) + 1.f) / 2.f);
        const float jx = jx0 + (boatLerp(-1.0471976, -.2617994, (sinf(-rt) + 1.f) / 2.f) - jx0) * am; float jy = jy0 + (boatLerp(-.7853982, .7853982, (sinf(-rt + 1.f) + 1.f) / 2.f) - jy0) * am;
        if (i == 1) jy = PI_ - jy;
        part(bmPad[i], -3, 5, i ? -9 : 9, -jx * R2D, -jy * R2D, 11.25f);
    }
    // máscara "noWater" via STENCIL (não escreve profundidade: o corpo/pernas de quem está sentado não são cortados); a água é desenhada só onde o stencil != 1
    glEnable(GL_STENCIL_TEST); glStencilFunc(GL_ALWAYS, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); glDepthMask(GL_FALSE); part(bmMask, 0, 3, 1, -90.f); glDepthMask(GL_TRUE); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_STENCIL_TEST);   // "noWater" do ModelBoat: só profundidade; a água (desenhada depois) não entra no casco
    glPopMatrix();
}
// Desenhar ANTES da passada translúcida da água (a placa invisível do casco só esconde a água se for desenhada antes dela).
static void drawBoats(float pt) {
    if (BOATS.empty()) return;
    if (!boatReady) loadBoatModel();
    glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    for (const BoatEnt& e : BOATS) { entLightAt(e.x, e.y + .4, e.z, gEntDim); drawBoat(e, pt); }
    entDimReset(); glDisable(GL_ALPHA_TEST);
}
