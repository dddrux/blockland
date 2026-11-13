// ================= veiculos (patchVeic.1): MrCrayfish's Vehicle Mod 1.12.2 convertido (mods/vehicle) =================
// Item "vehicle_<nome>_body" (aba Vehicle do criativo): botao direito no chao/agua = nasce o veiculo. Botao direito nele = monta. Agachar = desce.
// W/S acelera/freia (re), A/D esterça, espaco = sobe (aviao). Botao esquerdo = recolhe (devolve o item). Corpo = modelo do prop; rodas = prop vehicle_wheel.
// Simulacao no yaw do Minecraft (0 = +Z, cresce no sentido horario; frente = (-sin, cos)); desenho: rotate(-yaw). Local (sem rede ainda).
enum { VK_LAND, VK_WATER, VK_AIR };
struct VehDef { const char* key; int kind; float maxSpd, accel, turn, hr, h, seatY, seatZ; int nWheels; float wx, wf, wb, wy, ws; float s = 1.f, cx = 0.f, cz = 0.f, y0 = 0.f; };   // vel. em blocos/tick; turn em graus/tick na vel. maxima
static const VehDef VEHD[] = {
    // key              kind     max   acc    turn  hr   h    seatY seatZ  n  wx   wf    wb    wy   ws
    {"go_kart",         VK_LAND, .50f, .022f, 5.5f, .55f, .6f, .35f, -.05f, 4, .42f, .55f, -.55f, .16f, .8f},
    {"atv",             VK_LAND, .55f, .022f, 4.5f, .6f,  .8f, .55f, -.1f,  4, .5f,  .6f,  -.6f,  .2f,  1.f},
    {"dirt_bike",       VK_LAND, .62f, .026f, 4.8f, .45f, .9f, .6f,  -.1f,  2, 0.f,  .6f,  -.6f,  .25f, 1.f},
    {"mini_bike",       VK_LAND, .40f, .020f, 4.8f, .45f, .8f, .55f, -.1f,  2, 0.f,  .5f,  -.5f,  .2f,  .9f},
    {"moped",           VK_LAND, .45f, .022f, 4.8f, .45f, .9f, .6f,  -.1f,  2, 0.f,  .6f,  -.6f,  .22f, .9f},
    {"lawn_mower",      VK_LAND, .25f, .014f, 5.0f, .55f, .9f, .6f,  -.1f,  4, .42f, .5f,  -.5f,  .16f, .8f},
    {"golf_cart",       VK_LAND, .40f, .018f, 4.2f, .7f,  1.3f, .5f, -.2f,  4, .55f, .85f, -.85f, .2f,  .9f},
    {"tractor",         VK_LAND, .32f, .014f, 3.6f, .8f,  1.6f, .9f, -.3f,  4, .62f, .8f,  -.8f,  .4f,  1.3f},
    {"dune_buggy",      VK_LAND, .60f, .026f, 4.8f, .7f,  1.1f, .45f, -.1f, 4, .6f,  .85f, -.8f,  .22f, 1.f},
    {"off_roader",      VK_LAND, .65f, .026f, 4.4f, .8f,  1.4f, .6f, -.1f,  4, .65f, 1.0f, -1.0f, .3f,  1.1f},
    {"smart_car",       VK_LAND, .55f, .024f, 4.6f, .7f,  1.4f, .5f, -.15f, 4, .55f, .7f,  -.7f,  .2f,  .9f},
    {"mini_bus",        VK_LAND, .45f, .018f, 3.4f, 1.0f, 2.0f, .7f, .6f,   4, .8f,  1.3f, -1.3f, .3f,  1.1f},
    {"bumper_car",      VK_LAND, .28f, .016f, 6.0f, .6f,  .9f, .3f,  0.f,   0, 0.f,  0.f,  0.f,   0.f,  1.f},
    {"shopping_cart",   VK_LAND, .22f, .012f, 5.0f, .45f, 1.0f, .2f, 0.f,   0, 0.f,  0.f,  0.f,   0.f,  1.f},
    {"aluminum_boat",   VK_WATER, .30f, .014f, 3.4f, .8f, .6f, .1f,  -.1f,  0, 0.f,  0.f,  0.f,   0.f,  1.f},
    {"speed_boat",      VK_WATER, .62f, .022f, 3.0f, .8f, .7f, .15f, -.1f,  0, 0.f,  0.f,  0.f,   0.f,  1.f},
    {"jet_ski",         VK_WATER, .58f, .024f, 4.0f, .6f, .7f, .25f, -.1f,  0, 0.f,  0.f,  0.f,   0.f,  1.f},
    {"sports_plane",    VK_AIR,  .85f, .014f, 2.6f, .9f, 1.6f, .55f, 0.f,   0, 0.f,  0.f,  0.f,   0.f,  1.f},
};
static const int VEH_N = (int)(sizeof(VEHD) / sizeof(VEHD[0]));
struct VehEnt {
    int id = 0, def = 0; bool dead = false;
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0, my = 0; float yaw = 0, pyaw = 0, v = 0, steer = 0, pSteer = 0, wheel = 0, pWheel = 0, pitch = 0, pPitch = 0; bool ground = false, inWater = false;
};
static std::vector<VehEnt> VEHS; static int gVehNext = 0, gVehRide = 0;   // gVehRide = id do veiculo em que o jogador esta (0 = nenhum)
static std::vector<int> gVehItem(VEH_N, -2);   // id de item do corpo (cache; -1 = pack nao carregado)
static int vehItemId(int d) {
    if (d < 0 || d >= VEH_N) return 0;
    if (gVehItem[d] > 0) return gVehItem[d];
    const int id = propFromKey(std::string("vehicle/vehicle_") + VEHD[d].key + "_body"); if (id > 0) gVehItem[d] = id; return id > 0 ? id : 0;
}
static int vehDefOfItem(int id) {   // item na mao -> definicao (-1 = nao e corpo de veiculo)
    if (id <= 0) return -1;
    int pk = 0; const PropItem* it = propItem(id, &pk); if (!it || gPropPacks[pk].mod != "vehicle") return -1;
    for (int d = 0; d < VEH_N; d++) if (it->id == std::string("vehicle_") + VEHD[d].key + "_body") return d;
    return -1;
}
static const float VEHLEN[VEH_N] = {2.2f, 1.9f, 1.9f, 1.6f, 1.8f, 1.8f, 2.8f, 2.6f, 2.6f, 3.0f, 2.4f, 3.4f, 1.8f, 1.4f, 3.0f, 3.2f, 2.2f, 3.6f};   // comprimento real alvo (blocos): os modelos de item vem em escalas diferentes
static VehDef VEHM[VEH_N]; static bool VEHMok[VEH_N];
static const VehDef& vehM(int d) {   // geometria derivada do modelo real (bbox) e do comprimento alvo; antes do pack carregar usa a tabela
    if (d < 0 || d >= VEH_N) d = 0;
    if (!VEHMok[d]) {
        VEHM[d] = VEHD[d]; const int id = vehItemId(d); int pk = 0; const PropItem* it = id ? propItem(id, &pk) : nullptr;
        if (!it) return VEHD[d];
        const float* bb = gPropPacks[pk].models[it->model].bb; VehDef& v = VEHM[d];
        const float L = std::max(.2f, bb[5] - bb[2]); v.s = VEHLEN[d] / L; v.cx = (bb[0] + bb[3]) * .5f; v.cz = (bb[2] + bb[5]) * .5f; v.y0 = bb[1];
        const float W = (bb[3] - bb[0]) * v.s, H = (bb[4] - bb[1]) * v.s, len = VEHLEN[d];
        v.hr = std::max(.45f, std::min(1.0f, std::max(W, len * .45f) * .5f)); v.h = std::max(.5f, H);
        const bool bike = v.nWheels == 2; v.seatY = v.kind == VK_LAND ? (bike ? H * .5f : H * .26f) : (v.kind == VK_WATER ? H * .25f : H * .3f); v.seatZ = -len * (bike ? .06f : .03f);
        const float dia = len * .2f; v.ws = dia / .32f; v.wy = dia * .5f; v.wf = len * .3f; v.wb = -len * .3f; v.wx = bike ? 0.f : std::max(W * .5f - .05f, len * .16f);
        VEHMok[d] = true;
    }
    return VEHM[d];
}
static VehEnt* vehById(int id) { if (id) for (VehEnt& e : VEHS) if (e.id == id) return &e; return nullptr; }
static AABB vehBox(const VehEnt& e) { const double h = vehM(e.def).hr; return {e.x - h, e.y, e.z - h, e.x + h, e.y + vehM(e.def).h, e.z + h}; }
static AABB vehBoxAt(const VehEnt& e, double x, double y, double z) { const double h = vehM(e.def).hr; return {x - h, y, z - h, x + h, y + vehM(e.def).h, z + h}; }
static void vehCollect(const AABB& r, std::vector<AABB>& out, int skipId) {   // outros veiculos tambem sao solidos (jogador/mobs andam em cima)
    for (const VehEnt& e : VEHS) { if (e.dead || e.id == skipId) continue; const AABB a = vehBox(e); if (r.intersects(a)) out.push_back(a); }
}
static void vehClip(const AABB& b, const std::vector<AABB>& l, double& mx, double& my, double& mz) {   // Entity.move: Y, X, Z contra as caixas
    AABB c = b;
    for (const AABB& o : l) if (c.x1 > o.x0 && c.x0 < o.x1 && c.z1 > o.z0 && c.z0 < o.z1) { if (my > 0 && c.y1 <= o.y0) my = std::min(my, o.y0 - c.y1); else if (my < 0 && c.y0 >= o.y1) my = std::max(my, o.y1 - c.y0); }
    c.y0 += my; c.y1 += my;
    for (const AABB& o : l) if (c.y1 > o.y0 && c.y0 < o.y1 && c.z1 > o.z0 && c.z0 < o.z1) { if (mx > 0 && c.x1 <= o.x0) mx = std::min(mx, o.x0 - c.x1); else if (mx < 0 && c.x0 >= o.x1) mx = std::max(mx, o.x1 - c.x0); }
    c.x0 += mx; c.x1 += mx;
    for (const AABB& o : l) if (c.y1 > o.y0 && c.y0 < o.y1 && c.x1 > o.x0 && c.x0 < o.x1) { if (mz > 0 && c.z1 <= o.z0) mz = std::min(mz, o.z0 - c.z1); else if (mz < 0 && c.z0 >= o.z1) mz = std::max(mz, o.z1 - c.z0); }
}
static void vehMove(VehEnt& e, double mx, double my, double mz, bool step, bool& hitWall) {
    const double ox = mx, oz = mz, oy = my; std::vector<AABB> l, l2; AABB b = vehBox(e);
    AABB ext{b.x0 + std::min(mx, 0.) - .01, b.y0 + std::min(my, 0.) - .01, b.z0 + std::min(mz, 0.) - .01, b.x1 + std::max(mx, 0.) + .01, b.y1 + std::max(my, 0.) + 1.1, b.z1 + std::max(mz, 0.) + .01};
    collBoxes(ext, l); vehCollect(ext, l, e.id);
    double ax = mx, ay = my, az = mz; vehClip(b, l, ax, ay, az);
    e.ground = e.ground || (oy < 0 && ay != oy);
    if (step && (ax != ox || az != oz) && (e.ground || oy <= 0)) {   // degrau: sobe ate 1,05 bloco (EntityVehicle.stepHeight)
        double sx = ox, sy = 1.05, sz = oz; vehClip(b, l, sx, sy, sz);
        AABB b2 = b; b2.y0 += sy; b2.y1 += sy; double hx = ox, hy = 0, hz = oz; vehClip(b2, l, hx, hy, hz);
        AABB b3 = b2; b3.x0 += hx; b3.x1 += hx; b3.z0 += hz; b3.z1 += hz; double dx = 0, dy = -sy, dz = 0; vehClip(b3, l, dx, dy, dz);
        if (hx * hx + hz * hz > ax * ax + az * az + 1e-6) { ax = hx; az = hz; ay = sy + dy; e.ground = true; }
    }
    hitWall = (fabs(ax - ox) > 1e-4 || fabs(az - oz) > 1e-4);
    e.x += ax; e.y += ay; e.z += az; if (ay != oy && oy > 0) e.my = 0;
    (void)l2;
}
static bool vehSolidBelow(const VehEnt& e) { std::vector<AABB> l; const AABB b = vehBox(e); collBoxes(AABB{b.x0, b.y0 - .06, b.z0, b.x1, b.y0, b.z1}, l); vehCollect(AABB{b.x0, b.y0 - .06, b.z0, b.x1, b.y0, b.z1}, l, e.id); return !l.empty(); }
static void vehTick(VehEnt& e, bool L, bool R, bool F, bool B, bool up, bool riden) {
    const VehDef& d = vehM(e.def);
    e.px = e.x; e.py = e.y; e.pz = e.z; e.pyaw = e.yaw; e.pWheel = e.wheel; e.pSteer = e.steer; e.pPitch = e.pitch;
    const int bx = (int)floor(e.x), bz = (int)floor(e.z);
    const bool wet = isWater(getB(bx, (int)floor(e.y + .15), bz)); e.inWater = wet;
    const bool onSolid = vehSolidBelow(e); e.ground = onSolid;
    float acc = 0.f; const float fr = d.maxSpd > 0 ? fabsf(e.v) / d.maxSpd : 0.f;
    if (riden) { if (F) acc += d.accel * (1.f - .6f * fr); if (B) acc -= (e.v > .01f ? d.accel * 2.5f : d.accel * .7f); }
    const bool powered = d.kind == VK_LAND ? (onSolid && !wet) : d.kind == VK_WATER ? wet : true;   // motor so empurra se tem apoio (agua pro barco, chao pro carro; aviao sempre)
    if (powered) e.v += acc;
    float drag = d.kind == VK_WATER ? (wet ? .985f : .80f) : d.kind == VK_AIR ? (onSolid ? .985f : .997f) : (onSolid ? (acc == 0.f ? .965f : .995f) : .999f);
    if (wet && d.kind == VK_LAND) drag = .90f;
    e.v *= drag; const float lim = acc < 0 ? d.maxSpd * .35f : d.maxSpd; if (e.v > lim) e.v = lim; if (e.v < -d.maxSpd * .35f) e.v = -d.maxSpd * .35f; if (fabsf(e.v) < .002f && acc == 0.f) e.v = 0.f;
    // direcao: so gira com velocidade (carro) ou sempre um pouco (barco); sinal inverte na re
    const float want = riden ? ((R ? 1.f : 0.f) - (L ? 1.f : 0.f)) : 0.f; e.steer += (want - e.steer) * .35f;
    const float rate = d.kind == VK_AIR ? (onSolid ? .7f : .9f) : std::min(1.f, .25f + fabsf(e.v) / (d.maxSpd * .4f));
    if (d.kind != VK_AIR || fabsf(e.v) > .05f) e.yaw += e.steer * d.turn * rate * (e.v < -.001f ? -1.f : 1.f) * (d.kind == VK_AIR ? .6f : 1.f);
    const float yr = e.yaw * (float)M_PI / 180.f; const double fx = -sin(yr), fz = cos(yr);
    // vertical
    double dy = 0;
    if (d.kind == VK_AIR) {
        const bool fly = e.v > d.maxSpd * .55f;
        if (fly) { e.my += (up ? .02 : -.004); if (e.my > .16) e.my = .16; if (!up && e.my < -.06) e.my = -.06; if (e.v < d.maxSpd * .7f && !up) e.my -= .01; }
        else e.my -= .05 * (1.0 - std::min(1.f, e.v / (d.maxSpd * .55f)) * .85);
        e.my *= .96; if (onSolid && e.my < 0) e.my = 0; dy = e.my;
        const float tp = fly ? (up ? -18.f : (e.my < -.02 ? 10.f : 0.f)) : 0.f; e.pitch += (tp - e.pitch) * .15f;
    } else if (d.kind == VK_WATER) {
        if (wet) { const double surf = floor(e.y + .15) + 1.0; const bool above = isWater(getB(bx, (int)floor(e.y + .15) + 1, bz)); const double tgt = above ? surf + 1.0 : surf - .22; e.my += (tgt - e.y) * .06; e.my *= .6; }
        else e.my = (e.my - .04) * .98;
        if (onSolid && e.my < 0) e.my = 0; dy = e.my;
    } else {
        if (wet) { e.my = (e.my - .015) * .8; } else e.my = (e.my - .06) * .98;   // gravidade (carro afunda devagar na agua)
        if (onSolid && e.my < 0) e.my = 0; dy = e.my;
    }
    bool wall = false; vehMove(e, fx * e.v, dy, fz * e.v, d.kind != VK_AIR, wall);
    if (wall) { if (fabsf(e.v) > d.maxSpd * .6f && riden) { /* batida forte: sem dano ao jogador por enquanto */ } e.v *= .55f; }
    if (e.ground && e.my < 0) e.my = 0;
    e.wheel += e.v / .3f * 57.2958f; e.pWheel = e.wheel - e.v / .3f * 57.2958f;
    if (e.y < -20) e.dead = true;   // caiu do mundo
}
static bool vehFree(double x, double y, double z) { std::vector<AABB> l; collBoxes(AABB{x - .3, y, z - .3, x + .3, y + 1.8, z + .3}, l); return l.empty(); }
static void vehEject(VehEnt& e, Phys& P) {
    const VehDef& d = vehM(e.def); const float yr = e.yaw * (float)M_PI / 180.f; const double rx = cos(yr), rz = sin(yr);   // direita do veiculo
    const double offs[6][2] = {{1.f, 0.f}, {-1.f, 0.f}, {0.f, 1.f}, {0.f, -1.f}, {1.4f, 0.f}, {-1.4f, 0.f}};
    for (int k = 0; k < 6; k++) { const double r = d.hr + .5; const double x = e.x + (k < 2 ? rx * offs[k][0] * r : (k < 4 ? -sin(yr) * offs[k][1] * r : rx * offs[k][0] * r)), z = e.z + (k < 2 ? rz * offs[k][0] * r : (k < 4 ? cos(yr) * offs[k][1] * r : rz * offs[k][0] * r)); if (vehFree(x, e.y + .1, z)) { P.x = x; P.y = e.y + .1; P.z = z; gVehRide = 0; P.mx = P.my = P.mz = 0; return; } }
    P.x = e.x; P.y = e.y + d.h + .1; P.z = e.z; gVehRide = 0; P.mx = P.my = P.mz = 0;
}
static bool vehTickAll(Phys& P, bool L, bool R, bool F, bool B, bool up, bool sneak) {   // true = o jogador esta montado (tick.inc zera quedas e gira o corpo)
    for (size_t i = 0; i < VEHS.size(); i++) {
        VehEnt& e = VEHS[i]; const bool r = e.id == gVehRide;
        if (!getC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { e.px = e.x; e.py = e.y; e.pz = e.z; continue; }
        vehTick(e, L && r, R && r, F && r, B && r, up && r, r);
        if (!gVehRide && !e.dead) {   // jogador a pe empurra o veiculo parado
            const AABB pb{P.x - .3, P.y, P.z - .3, P.x + .3, P.y + 1.8, P.z + .3}; const AABB vb = vehBox(e);
            if (pb.intersects(vb) && fabsf(e.v) < .05f) { double dx = e.x - P.x, dz = e.z - P.z; const double m = sqrt(dx * dx + dz * dz); if (m > .01) { bool w; vehMove(e, dx / m * .04, 0, dz / m * .04, true, w); } }
        }
    }
    for (size_t i = VEHS.size(); i-- > 0;) if (VEHS[i].dead) { if (VEHS[i].id == gVehRide) gVehRide = 0; VEHS.erase(VEHS.begin() + i); }
    VehEnt* e = vehById(gVehRide); if (!e) { gVehRide = 0; return false; }
    if (sneak && (vehM(e->def).kind != VK_AIR || e->ground)) { vehEject(*e, P); return false; }
    const VehDef& d = vehM(e->def); const float yr = e->yaw * (float)M_PI / 180.f;
    P.x = e->x + (-sin(yr)) * d.seatZ; P.z = e->z + cos(yr) * d.seatZ; P.y = e->y + d.seatY; P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false;
    return true;
}
static int vehPick(const float* eye, const float* dir, float reach) {
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)VEHS.size(); i++) {
        const VehEnt& e = VEHS[i]; if (e.id == gVehRide) continue; const AABB a = vehBox(e);
        const float lo[3] = {(float)a.x0 - .1f, (float)a.y0 - .1f, (float)a.z0 - .1f}, hi[3] = {(float)a.x1 + .1f, (float)a.y1 + .1f, (float)a.z1 + .1f};
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
static bool vehInteract(const float* eye, const float* dir, float reach) {   // botao direito no veiculo: monta
    const int i = vehPick(eye, dir, reach); if (i < 0) return false; gVehRide = VEHS[i].id; return true;
}
static bool vehHit(const float* eye, const float* dir, bool creative, float reach) {   // botao esquerdo: recolhe e devolve o item
    const int i = vehPick(eye, dir, reach); if (i < 0) return false; VehEnt& e = VEHS[i];
    if (!creative) spawnDrop(e.x, e.y + .4, e.z, 0, .15, 0, vehItemId(e.def), 1, 10);
    e.dead = true; return true;
}
static void vehSpawn(int d, double x, double y, double z, float yawMc) { VehEnt e; e.id = (netSelf << 20) | (++gVehNext & 0xFFFFF); e.def = d; e.x = e.px = x; e.y = e.py = y; e.z = e.pz = z; e.yaw = e.pyaw = yawMc; VEHS.push_back(e); }
static bool vehPlace(int d, const float* eye, const float* dir, float yawO, float reach) {   // item de veiculo: chao ou agua
    int c[3] = {0, 0, 0}; float tHit = 0.f; bool hit = false;
    for (float t = 0.f; t <= reach && !hit; t += .02f) {
        for (int k = 0; k < 3; k++) c[k] = (int)floorf(eye[k] + dir[k] * t);
        const int bl = getB(c[0], c[1], c[2]);
        if ((isWater(bl) && liqMeta(bl) == 0) || solid(bl)) { hit = true; tHit = t; }
    }
    if (!hit) return false;
    float t0 = 0.f; for (int k = 0; k < 3; k++) if (fabsf(dir[k]) > 1e-6f) t0 = std::max(t0, ((dir[k] > 0 ? c[k] : c[k] + 1) - eye[k]) / dir[k]);
    t0 = std::min(t0, tHit);
    const bool w = isWater(getB(c[0], c[1], c[2]));
    const double x = eye[0] + dir[0] * t0, z = eye[2] + dir[2] * t0, y = eye[1] + dir[1] * t0 - (w ? .12 : 0.);
    if (vehM(d).kind == VK_WATER && !w) return false;   // barco so na agua
    const double h = vehM(d).hr; std::vector<AABB> l; collBoxes(AABB{x - h, y + .1, z - h, x + h, y + vehM(d).h, z + h}, l);
    if (!l.empty()) return false;
    vehSpawn(d, x, y, z, 180.f - yawO); return true;
}
// ---- desenho: modelo do prop (corpo e rodas) com a transformacao do veiculo ----
static void vehDrawMesh(int id, const float* lt) {   // mesmas quads do drawProps (faces opacas e vidro com alfa)
    PropMesh* m = propMesh(id); if (!m) return; int pack = 0; propItem(id, &pack);
    int cur = -2; bool open = false;
    for (const PropQuad& q : m->q) {
        GLuint tex = q.page >= 0 ? propPageTex(pack, q.page) : 0; const int key = tex ? (int)tex : -1;
        if (key != cur) { if (open) { glEnd(); open = false; } cur = key; if (tex) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); } else rdDisable(GL_TEXTURE_2D); }
        if (!open) { glBegin(GL_QUADS); open = true; }
        const float g = tex ? 1.f : .55f; glColor4f(lt[0] * q.shade * g, lt[1] * q.shade * g, lt[2] * q.shade * g, q.glass ? m->alpha : 1.f);
        for (int k = 0; k < 4; k++) { if (tex) glTexCoord2f(q.uv[k][0], q.uv[k][1]); glVertex3f(q.p[k][0], q.p[k][1], q.p[k][2]); }
    }
    if (open) glEnd();
}
static void drawVehicles(float pt) {
    if (VEHS.empty()) return;
    static int wheelId = 0; if (!wheelId) { static const char* wn[] = {"standard", "all_terrain", "sports", "racing", "plastic", "off_road", "snow"}; for (const char* n : wn) if ((wheelId = propFromKey(std::string("vehicle/vehicle_wheel_") + n))) break; }
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f);
    for (const VehEnt& e : VEHS) {
        const double x = e.px + (e.x - e.px) * pt, y = e.py + (e.y - e.py) * pt, z = e.pz + (e.z - e.pz) * pt; if (entOutView(x, y, z, 4.f)) continue;
        const VehDef& d = vehM(e.def); const int body = vehItemId(e.def); if (!body) continue;
        float lt[3]; entLightAt(x, y + .5, z, lt);
        float dyaw = e.yaw - e.pyaw; while (dyaw > 180.f) dyaw -= 360.f; while (dyaw < -180.f) dyaw += 360.f;
        rdPushMatrix(); rdTranslatef((float)x, (float)y, (float)z); rdRotatef(-(e.pyaw + dyaw * pt), 0, 1, 0); rdRotatef(e.pPitch + (e.pitch - e.pPitch) * pt, 1, 0, 0);
        rdPushMatrix(); rdScalef(d.s, d.s, d.s); rdTranslatef(-d.cx, -d.y0, -d.cz); vehDrawMesh(body, lt); rdPopMatrix();
        if (wheelId && d.nWheels > 0) {
            const float wr = e.pWheel + (e.wheel - e.pWheel) * pt, st = (e.pSteer + (e.steer - e.pSteer) * pt) * 25.f;
            for (int w = 0; w < d.nWheels; w++) {
                const bool front = d.nWheels == 2 ? (w == 0) : (w < 2); const float sx = d.nWheels == 2 ? 0.f : ((w & 1) ? d.wx : -d.wx), sz = front ? d.wf : d.wb;
                rdPushMatrix(); rdTranslatef(sx, d.wy, sz); if (front) rdRotatef(-st, 0, 1, 0); rdRotatef(-wr, 1, 0, 0); rdScalef(d.ws, d.ws, d.ws); rdTranslatef(0, -.5f, 0);
                vehDrawMesh(wheelId, lt); rdPopMatrix();
            }
        }
        rdPopMatrix();
    }
    rdDisable(GL_ALPHA_TEST); glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, atlasTex);
}
