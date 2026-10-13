// ---- ovo arremessavel (EntityEgg 1.12.2), etapa 1: so local (sem rede/split/Android/som) ----
// Fisica de EntityThrowable: velocidade 1.5, imprecisao 1.0, gravidade .03, arrasto .99 (.8 na agua). Cooldown de 10 ticks entre arremessos.
struct EggEnt { double x, y, z, px, py, pz, vx, vy, vz; int age = 0, owner = -1; };   // owner -1 = eu; outro = jogador remoto (so visual: sem dano, sem chocar)
static std::vector<EggEnt> EGGS;
static int gEggCd = 0;   // ticks restantes ate poder arremessar de novo
static bool eggBlockUsable(int x, int y, int z) { const int b = getB(x, y, z); return b == CTABLE || b == XB_CHEST || xbIsLever(b) || isRs(b); }   // clique no bloco interativo usa o bloco, nao arremessa
static void eggBreakFx(double x, double y, double z) {   // 8 particulas do item (Entity.handleStatusUpdate 3)
    if (parts.size() > 1500) return; const float c[3] = {1.f, 1.f, 1.f}, br = lightNear((int)floor(x), (int)floor(y), (int)floor(z));
    for (int i = 0; i < 8; i++) addPart((float)x, (float)y, (float)z, (frand() - .5f) * .2f, (frand() - .5f) * .2f, (frand() - .5f) * .2f, itemTile(IT_EGG), c, br, 1.f, 1.f);
}
static void eggHitMob(MobEnt& m, double dx, double dz) {   // dano 0: so empurra e pisca (mobHurt ignora dmg <= 0)
    const double l = sqrt(dx * dx + dz * dz); m.hurt = 10;
    if (l > 1e-6) { m.mx += dx / l * .4; m.mz += dz / l * .4; } m.my = std::min(.4, m.my / 2 + .4);
}
static int gEggHatchCd = 0;   // host: intervalo minimo entre pedidos de choco de convidados (anti-spam)
static void eggHatchSpawn(double x, double y, double z, int n) {   // TODO item 8: deveriam ser filhotes (escala .5), ainda nao existe
    for (int i = 0; i < n; i++) spawnMob(M_CHICKEN, x, y, z, frand() * 360.f);
}
static void eggHatch(double x, double y, double z) {   // 1/8 de chance; desses, 1/32 sao 4. Convidado: mobs sao do host, entao so PEDE (NP_EGGHATCH)
    if (rand() % 8 != 0) return;
    const int n = rand() % 32 == 0 ? 4 : 1;
    if (netOn && !netHost) { NBuf b = netMk(NP_EGGHATCH); b.put<float>((float)x); b.put<float>((float)y); b.put<float>((float)z); b.put<uint8_t>((uint8_t)n); netBroadcast(b); return; }
    eggHatchSpawn(x, y, z, n);
}
static void eggHatchRecv(NBuf& b, int sender) {   // host: convidado pediu galinha(s) do ovo dele
    (void)sender; const double x = b.get<float>(), y = b.get<float>(), z = b.get<float>(); const int n = b.get<uint8_t>();
    if (!netHost || n < 1 || n > 4 || gEggHatchCd > 0 || !(fabs(x) < 3e7 && fabs(z) < 3e7 && y > -64 && y < 400)) return;   // forjado / fora do mundo / spam
    gEggHatchCd = 2; eggHatchSpawn(x, y, z, n);
}
static bool eggThrow(const Phys& P, float yaw, float pitch, Slot& s, int& cd = gEggCd) {   // false = em cooldown / sem item; cd = cooldown do dono (P2 do split tem o seu)
    if (cd > 0 || s.id != IT_EGG || s.n <= 0) return false;
    const float cy = cosf(yaw * .0174533f), sy = sinf(yaw * .0174533f), cp = cosf(pitch * .0174533f), sp = sinf(pitch * .0174533f);
    double d[3] = {-sy * cp, sp, -cy * cp};
    for (int a = 0; a < 3; a++) d[a] += arrowGauss() * .0075;
    const double l = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), v = 1.5;
    EggEnt e; e.x = e.px = P.x - cy * .16; e.y = e.py = P.y + (P.sneak ? 1.54 : 1.62) - .1; e.z = e.pz = P.z - sy * .16;
    e.vx = d[0] / l * v; e.vy = d[1] / l * v; e.vz = d[2] / l * v; EGGS.push_back(e);
    if (netOn) { NBuf b = netMk(NP_EGG); b.put<float>((float)e.x); b.put<float>((float)e.y); b.put<float>((float)e.z); b.put<float>((float)e.vx); b.put<float>((float)e.vy); b.put<float>((float)e.vz); netBroadcast(b); }
    cd = 10; if (!creative && --s.n <= 0) s = Slot(); return true;
}
static void eggNetRecv(NBuf& b, int sender) {   // ovo de outro jogador: simulado aqui so para ver
    EggEnt e; e.x = e.px = b.get<float>(); e.y = e.py = b.get<float>(); e.z = e.pz = b.get<float>(); e.vx = b.get<float>(); e.vy = b.get<float>(); e.vz = b.get<float>(); e.owner = sender;
    if (EGGS.size() < 64) EGGS.push_back(e);
}
static bool eggStep(EggEnt& e, const Phys& P) {   // false = remover
    e.px = e.x; e.py = e.y; e.pz = e.z; ++e.age;
    const double sp = sqrt(e.vx * e.vx + e.vy * e.vy + e.vz * e.vz);
    if (sp > 1e-6) {
        const double o[3] = {e.x, e.y, e.z}, d[3] = {e.vx / sp, e.vy / sp, e.vz / sp};
        const float of[3] = {(float)o[0], (float)o[1], (float)o[2]}, df[3] = {(float)d[0], (float)d[1], (float)d[2]};
        const float tbf = raySolidDistance(of, df, (float)sp); double bt = tbf >= 0.f ? tbf : sp; int idx = -1;
        for (int i = 0; e.owner == -1 && i < (int)MOBS.size(); i++) {
            const MobEnt& m = MOBS[i]; if (m.dead) continue;
            const double hw = MD[m.type].w * .5 + .3, lo[3] = {m.x - hw, m.y - .3, m.z - hw}, hi[3] = {m.x + hw, m.y + MD[m.type].h + .3, m.z + hw};
            const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; idx = i; }
        }
        if (idx >= 0) { eggHitMob(MOBS[idx], d[0], d[2]); eggBreakFx(e.x + d[0] * bt, e.y + d[1] * bt, e.z + d[2] * bt); eggHatch(e.x + d[0] * bt, e.y + d[1] * bt, e.z + d[2] * bt); return false; }
        if (tbf >= 0.f) { const double hx = e.x + d[0] * (tbf - .05), hy = e.y + d[1] * (tbf - .05), hz = e.z + d[2] * (tbf - .05); eggBreakFx(hx, hy, hz); if (e.owner == -1) eggHatch(hx, hy, hz); return false; }
        e.x += e.vx; e.y += e.vy; e.z += e.vz;
    }
    const double f = isWater(getB((int)floor(e.x), (int)floor(e.y), (int)floor(e.z))) ? .8 : .99;
    e.vx *= f; e.vy *= f; e.vz *= f; e.vy -= .03;
    return !(e.y < -64.0 || e.age > 1200);
}
static void eggsTick(const Phys& P) {
    if (gEggCd > 0) --gEggCd; if (gEggHatchCd > 0) --gEggHatchCd;
    for (size_t i = 0; i < EGGS.size();) { if (eggStep(EGGS[i], P)) ++i; else { EGGS[i] = EGGS.back(); EGGS.pop_back(); } }
}
static void eggsDraw(float pt) {   // sprite do item sempre de frente para a camera
    if (EGGS.empty()) return; glColor4f(1.f, 1.f, 1.f, 1.f);
    for (const EggEnt& e : EGGS) {
        const double ix = e.px + (e.x - e.px) * pt, iy = e.py + (e.y - e.py) * pt, iz = e.pz + (e.z - e.pz) * pt;
        entLightAt(ix, iy, iz, gEntDim);
        glPushMatrix(); glTranslatef((float)ix, (float)iy, (float)iz); glRotatef(-gYawNow, 0, 1, 0); glScalef(.35f, .35f, .35f); glTranslatef(-.5f, -.5f, -.5f);
        drawHeldRaw(IT_EGG); glPopMatrix();
    }
    entDimReset();
}
