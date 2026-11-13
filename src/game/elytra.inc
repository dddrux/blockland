// ================= elitra + foguetes de artificio (Minecraft 1.12.2) =================
// Inicio: apertar ESPACO no ar, caindo (motionY < 0), sem voo criativo, com elitra utilizavel no peitoral (EntityPlayerSP.onLivingUpdate).
// Fisica: EntityLivingBase.travel (physics.inc). Desgaste: 1 de dano a cada 20 ticks voando (sobrevivencia); inutilizavel com dano >= 431 (de 432).
// Queda: fallDistance = 1 enquanto motionY > -0.5 (nao ha dano planando devagar). Parede: dano "energia cinetica" = (perda de velocidade horizontal)*10 - 3.
// Foguete (ItemFirework.onItemRightClick): com a elitra aberta empurra para o olhar por 10 + rand(6) + rand(7) ticks (voo 1).
struct RocketBoost { int t, max; };
static std::vector<RocketBoost> gRockets;
static int rocketItemId() { static size_t n = (size_t)-1; static int id = 0; if (n != PK.size()) { n = PK.size(); id = 0; for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == "item/firework_rocket") { id = PK0 + (int)i; break; } } return id; }
static bool rocketUse(const Phys& P, Slot& s) {
    if (!s.id || s.id != rocketItemId() || !P.elytra || gDead) return false;
    gRockets.push_back({0, 10 + rand() % 6 + rand() % 7});
    if (!creative && --s.n <= 0) s = Slot();
    return true;
}
// BUG-014: foguete em bloco (EntityFireworkRocket 1.12.2): sobe (vy += .04, vxz *= 1.15), vida 10 + rand(6) + rand(7) ticks, explode em faiscas coloridas.
// Sem estrela nao fere. Local (sem rede: outros jogadores nao veem). Rastro de faiscas a cada tick.
struct FwEnt { double x, y, z, px, py, pz, vx, vy, vz; int age, life; };
static std::vector<FwEnt> gFw;
static void fwBlast(const FwEnt& f) {
    sndEvtAt("firework.blast", f.x, f.y, f.z, 1.f);
    if (parts.size() > 1300) return;
    static const float COL[6][3] = {{1.f, .2f, .2f}, {1.f, .8f, .1f}, {.3f, .6f, 1.f}, {.4f, 1.f, .4f}, {1.f, .4f, 1.f}, {1.f, 1.f, 1.f}};
    const float* c = COL[rand() % 6]; const float* c2 = COL[rand() % 6];
    const float br = lightNear((int)floor(f.x), (int)floor(f.y), (int)floor(f.z)); const int tl = itemTile(rocketItemId());
    for (int i = 0; i < 70; i++) {
        const float a = frand() * 6.2831853f, b = (frand() - .5f) * 3.1415926f, ca = cosf(b);   // esfera
        addPart((float)f.x, (float)f.y, (float)f.z, cosf(a) * ca, sinf(b), sinf(a) * ca, tl, (i & 1) ? c : c2, std::max(br, .8f), .7f, 3.f);
    }
}
static bool fwLaunch(const int* pp, Slot& s) {   // clique direito com foguete num bloco (sem elitra)
    if (!s.id || s.id != rocketItemId() || gDead || gGm == 3 || gFw.size() > 24) return false;
    FwEnt f; f.x = f.px = pp[0] + .5; f.y = f.py = pp[1] + .05; f.z = f.pz = pp[2] + .5;
    f.vx = (frand() - .5) * .004; f.vz = (frand() - .5) * .004; f.vy = .05; f.age = 0; f.life = 10 + rand() % 6 + rand() % 7;
    gFw.push_back(f); sndEvtAt("firework.launch", f.x, f.y, f.z, 1.f);
    if (!creative && --s.n <= 0) s = Slot();
    return true;
}
static void fwTick() {
    for (size_t i = 0; i < gFw.size();) {
        FwEnt& f = gFw[i]; f.px = f.x; f.py = f.y; f.pz = f.z;
        f.vx *= 1.15; f.vz *= 1.15; f.vy += .04;
        if (!solid(getB((int)floor(f.x + f.vx), (int)floor(f.y + f.vy), (int)floor(f.z + f.vz)))) { f.x += f.vx; f.y += f.vy; f.z += f.vz; } else f.age = f.life;   // bateu: explode
        if (!gPerf && gOptParticles && parts.size() < 1400) { const float c[3] = {1.f, .75f, .3f}; addPart((float)f.x, (float)f.y, (float)f.z, 0.f, -.2f, 0.f, itemTile(rocketItemId()), c, 1.f, .35f, .3f); }
        if (++f.age > f.life) { fwBlast(f); gFw[i] = gFw.back(); gFw.pop_back(); } else ++i;
    }
}
static void fwDraw(float pt) {   // sprite do item de frente para a camera (como o ovo), inclinado no sentido do movimento nao: simples
    if (gFw.empty()) return; glColor4f(1.f, 1.f, 1.f, 1.f);
    for (const FwEnt& f : gFw) {
        const double ix = f.px + (f.x - f.px) * pt, iy = f.py + (f.y - f.py) * pt, iz = f.pz + (f.z - f.pz) * pt;
        entLightAt(ix, iy, iz, gEntDim);
        rdPushMatrix(); rdTranslatef((float)ix, (float)iy, (float)iz); rdRotatef(gBbYaw, 0, 1, 0); rdRotatef(gBbPitch, 1, 0, 0); rdScalef(.5f, .5f, .5f); rdTranslatef(-.5f, -.5f, -.5f);
        drawHeldRaw(rocketItemId()); rdPopMatrix();
    }
    entDimReset();
}
// antes do movimento do tick: inicio do voo, vetor de olhar e fallDistance
static void elytraBefore(Phys& P, bool jumpKey, float yawDeg, float pitchDeg) {
    static bool prevJump = false; const bool press = jumpKey && !prevJump; prevJump = jumpKey;
    const double yr = yawDeg * .017453292519943295, pr = pitchDeg * .017453292519943295;
    gElyLook[0] = -sin(yr) * cos(pr); gElyLook[1] = sin(pr); gElyLook[2] = -cos(yr) * cos(pr); gElyPitchRad = (float)-pr;   // Java: pitch positivo = para baixo
    if (gDead || gGm == 3) { P.elytra = false; P.elyTicks = 0; gRockets.clear(); return; }
    if (press && !P.onGround && P.my < 0 && !P.elytra && !P.flying && !P.inWater && !P.inLava && !gBoatRide && !gCartRide && !gVehRide && elytraUsable(armor[1])) { P.elytra = true; P.elyTicks = 0; }
    if (P.elytra && P.my > -.5 && !P.inWater && !P.inLava) gFall = 1.f;
}
// depois do movimento do tick: dano de parede, updateElytra (desgaste/fim do voo) e foguetes
static void elytraAfter(Phys& P) {
    if (gElyWallDmg > 0.f) { const float d = gElyWallDmg; gElyWallDmg = 0.f; survDamage((int)ceilf(d), "Sofreu energia cinética", true); }
    if (P.elytra) {
        if (!P.onGround && !P.flying && !gBoatRide && !gCartRide && !gVehRide && gGm != 3 && elytraUsable(armor[1])) { if (!creative && (P.elyTicks + 1) % 20 == 0) armor[1].d++; P.elyTicks++; }
        else { P.elytra = false; P.elyTicks = 0; }
    }
    for (size_t i = gRockets.size(); i-- > 0;) {   // EntityFireworkRocket.onUpdate (boostedEntity)
        RocketBoost& r = gRockets[i];
        if (!P.elytra) { gRockets.erase(gRockets.begin() + i); continue; }
        P.mx += gElyLook[0] * .1 + (gElyLook[0] * 1.5 - P.mx) * .5; P.my += gElyLook[1] * .1 + (gElyLook[1] * 1.5 - P.my) * .5; P.mz += gElyLook[2] * .1 + (gElyLook[2] * 1.5 - P.mz) * .5;
        if (++r.t > r.max) gRockets.erase(gRockets.begin() + i);
    }
}
