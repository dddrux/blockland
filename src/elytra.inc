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
// antes do movimento do tick: inicio do voo, vetor de olhar e fallDistance
static void elytraBefore(Phys& P, bool jumpKey, float yawDeg, float pitchDeg) {
    static bool prevJump = false; const bool press = jumpKey && !prevJump; prevJump = jumpKey;
    const double yr = yawDeg * .017453292519943295, pr = pitchDeg * .017453292519943295;
    gElyLook[0] = -sin(yr) * cos(pr); gElyLook[1] = sin(pr); gElyLook[2] = -cos(yr) * cos(pr); gElyPitchRad = (float)-pr;   // Java: pitch positivo = para baixo
    if (gDead || gGm == 3) { P.elytra = false; P.elyTicks = 0; gRockets.clear(); return; }
    if (press && !P.onGround && P.my < 0 && !P.elytra && !P.flying && !P.inWater && !P.inLava && !gBoatRide && !gCartRide && elytraUsable(armor[1])) { P.elytra = true; P.elyTicks = 0; }
    if (P.elytra && P.my > -.5 && !P.inWater && !P.inLava) gFall = 1.f;
}
// depois do movimento do tick: dano de parede, updateElytra (desgaste/fim do voo) e foguetes
static void elytraAfter(Phys& P) {
    if (gElyWallDmg > 0.f) { const float d = gElyWallDmg; gElyWallDmg = 0.f; survDamage((int)ceilf(d), "Sofreu energia cinética", true); }
    if (P.elytra) {
        if (!P.onGround && !gBoatRide && !gCartRide && gGm != 3 && elytraUsable(armor[1])) { if (!creative && (P.elyTicks + 1) % 20 == 0) armor[1].d++; P.elyTicks++; }
        else { P.elytra = false; P.elyTicks = 0; }
    }
    for (size_t i = gRockets.size(); i-- > 0;) {   // EntityFireworkRocket.onUpdate (boostedEntity)
        RocketBoost& r = gRockets[i];
        if (!P.elytra) { gRockets.erase(gRockets.begin() + i); continue; }
        P.mx += gElyLook[0] * .1 + (gElyLook[0] * 1.5 - P.mx) * .5; P.my += gElyLook[1] * .1 + (gElyLook[1] * 1.5 - P.my) * .5; P.mz += gElyLook[2] * .1 + (gElyLook[2] * 1.5 - P.mz) * .5;
        if (++r.t > r.max) gRockets.erase(gRockets.begin() + i);
    }
}
