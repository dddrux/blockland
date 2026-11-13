// ---------- POCOES (patchPot.1): beber = aplica os efeitos da tabela POT_T; garrafa vazia volta ao inventario ----------
static bool potDrinkable(int id) { return potIs(id) && potForm(id) == 0; }
static void potDrinkNow(Slot& s) {
    const PotType* T = potType(s.id); if (!T) return;
    sndEvtUi("drink");
    if (T->eff[0]) { const int f = fxIdx(T->eff); if (f >= 0) fxGive(f, T->dur, T->amp); }
    if (!creative) { const int bt = potIngs().bottle; if (--s.n <= 0) { s = Slot(); if (bt) { s.id = bt; s.n = 1; } } }
}
// ---- arremessaveis (patchPot.2): EggEnt kind 2 (splash) / 3 (persistente); impacto aplica efeitos em jogador local e mobs; persistente deixa uma nuvem ----
static bool potThrowable(int id) { return potIs(id) && potForm(id) >= 1; }
static int potThrowInfo(int id, int& pot, int& kind) { auto it = gPotOf.find(id); if (it == gPotOf.end() || it->second.first < 1) return 0; pot = it->second.second; kind = it->second.first == 1 ? 2 : 3; return 1; }
static int potItemOf(int form, int pot) { return pot >= 0 && pot < POT_N && form >= 0 && form < 3 ? gPotId[form][pot] : 0; }
struct PotCloud { double x, y, z; float r; int age, pot, tick; };
static std::vector<PotCloud> gClouds;
static bool potUndead(int t) { return t == M_ZOMBIE || t == M_SKELETON || t == M_HUSK || t == M_PIGMAN || t == M_WSKEL || t == M_STRAY || t == M_ZVILL; }
// aplica a pocao `pot` a uma entidade a `dist` do impacto; scale 1 = total (acerto direto); lingering divide duracao por 4 (1.12.2)
static void potApplyPlayer(const PotType& T, double scale, bool linger) {
    const int f = fxIdx(T.eff); if (f < 0) return;
    if (f == FX_HEAL || f == FX_HARM) { const int a = (int)(scale * (f == FX_HEAL ? 4 << T.amp : 6 << T.amp) * (linger ? .5 : 1.) + .5); if (a <= 0) return; if (f == FX_HEAL) gHP = std::min(20, gHP + a); else survDamage(a, "foi morto por magia", true); return; }
    const int d = (int)(scale * T.dur * (linger ? .25 : 1.) + .5); if (d > 20) fxGive(f, d, T.amp);
}
static void potApplyMob(MobEnt& m, const PotType& T, double scale, bool linger, double ix, double iz) {
    const int f = fxIdx(T.eff); if (f < 0 || m.dead) return;
    const bool und = potUndead(m.type);
    if (f == FX_HEAL || f == FX_HARM) {
        const bool harm = (f == FX_HARM) != und; const int a = (int)(scale * (f == FX_HEAL ? 4 << T.amp : 6 << T.amp) * (linger ? .5 : 1.) + .5); if (a <= 0) return;
        if (harm) mobHurt(m, a, (float)ix, (float)iz, false); else m.hp = std::min(MD[m.type].hp, m.hp + a); return;
    }
    const int d = (int)(scale * T.dur * (linger ? .25 : 1.) + .5); if (d <= 20) return;
    if (f == FX_POISON && !und) { m.potPoison = std::max(m.potPoison, d); m.potPoisonAmp = T.amp; }
    else if (f == FX_REGEN && !und) m.potRegen = std::max(m.potRegen, d);
    else if (f == FX_SLOW) { m.potSlow = std::max(m.potSlow, d); m.potSlowAmp = T.amp; }
    else if (f == FX_SPEED) { m.potSpeed = std::max(m.potSpeed, d); m.potSpeedAmp = T.amp; }
}
static void potBurst(double x, double y, double z, int color, int n, float spread) {
    if (parts.size() > 1400) return; const float c[3] = {((color >> 16) & 255) / 255.f, ((color >> 8) & 255) / 255.f, (color & 255) / 255.f}, br = lightNear((int)floor(x), (int)floor(y), (int)floor(z));
    for (int i = 0; i < n; i++) addPart((float)x, (float)y + .1f, (float)z, (frand() - .5f) * spread, frand() * spread * .8f, (frand() - .5f) * spread, itemTile(IT_EGG), c, br, .6f, 1.f);
}
static void potLandFx(const EggEnt& e, double x, double y, double z) {
    if (e.pot < 0 || e.pot >= POT_N) return; const PotType& T = POT_T[e.pot];
    sndEvtAt("block.break.glass", x, y, z, 1.f); potBurst(x, y, z, T.color, 40, .5f);
    if (e.kind == 3) { if (gClouds.size() < 16) gClouds.push_back({x, y, z, 3.f, 0, e.pot, 0}); return; }
    if (!T.eff[0]) {
        if (!strcmp(T.key, "water")) {   // agua arremessada: apaga quem queima na caixa 4x2x4 e fere blaze (1.12.2: damageEntityWaterSensitive)
            if (gPvpP && fabs(gPvpP->x - x) < 4 && fabs(gPvpP->z - z) < 4 && fabs(gPvpP->y - y) < 3) gFire = 0;
            if (e.owner == -1) for (MobEnt& m : MOBS) if (!m.dead && fabs(m.x - x) < 4 && fabs(m.z - z) < 4 && fabs(m.y - y) < 3) { m.fire = 0; if (m.type == M_BLAZE) mobHurt(m, 1, (float)x, (float)z, false); }
        }
        return;
    }
    const double R = 4.0;   // caixa de 4x2x4 em volta do impacto; escala = 1 - sqrt(dist)/4
    if (gPvpP && !gDead) { const double dx = gPvpP->x - x, dy = gPvpP->y + .9 - y, dz = gPvpP->z - z, d2 = dx * dx + dy * dy + dz * dz; if (fabs(dx) < R && fabs(dy) < 2.9 && fabs(dz) < R && d2 < R * R) potApplyPlayer(T, 1. - sqrt(d2) / R, false); }
    if (e.owner != -1) return;   // mobs: so o dono do arremesso aplica (host autoritativo)
    for (MobEnt& m : MOBS) { if (m.dead) continue; const double dx = m.x - x, dy = m.y + mobH(m) * .5 - y, dz = m.z - z, d2 = dx * dx + dy * dy + dz * dz; if (d2 < R * R) potApplyMob(m, T, 1. - sqrt(d2) / R, false, x, z); }
}
static void potCloudsTick(Phys& P) {
    (void)P; static int mobT = 0;
    for (MobEnt& m : MOBS) { if (m.dead) continue;
        if (m.potPoison > 0) { --m.potPoison; if (m.potPoison % (25 >> std::min(m.potPoisonAmp, 4)) == 0 && m.hp > 1) mobHurt(m, 1, (float)m.x, (float)m.z, false); }
        if (m.potSlow > 0) --m.potSlow; if (m.potSpeed > 0) --m.potSpeed;
        if (m.potRegen > 0) { --m.potRegen; if (m.potRegen % 50 == 0) m.hp = std::min(MD[m.type].hp, m.hp + 1); } }
    (void)mobT;
    for (size_t i = 0; i < gClouds.size();) {
        PotCloud& c = gClouds[i]; ++c.age; ++c.tick; const PotType& T = POT_T[c.pot];
        c.r -= 3.f / 600.f;   // encolhe com o tempo (duracao 600 ticks)
        if (c.age % 2 == 0) potBurst(c.x + (frand() - .5) * c.r * 2, c.y, c.z + (frand() - .5) * c.r * 2, T.color, 1, .1f);
        if (c.age >= 10 && c.tick >= 10) {   // reaplica a cada 10 ticks a quem estiver no raio; cada rodada gasta .5 de raio
            c.tick = 0; bool hit = false;
            if (T.eff[0]) {
                if (gPvpP && !gDead) { const double dx = gPvpP->x - c.x, dz = gPvpP->z - c.z; if (dx * dx + dz * dz <= c.r * c.r && fabs(gPvpP->y - c.y) < 3) { potApplyPlayer(T, 1., true); hit = true; } }
                for (MobEnt& m : MOBS) { if (m.dead) continue; const double dx = m.x - c.x, dz = m.z - c.z; if (dx * dx + dz * dz <= c.r * c.r && fabs(m.y - c.y) < 3) { potApplyMob(m, T, 1., true, c.x, c.z); hit = true; } }
            }
            if (hit) c.r -= .5f;
        }
        if (c.r <= .5f || c.age >= 600) { gClouds[i] = gClouds.back(); gClouds.pop_back(); } else ++i;
    }
}
