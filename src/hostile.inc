// ---------- Mobs hostis: zumbi, esqueleto e creeper (overworld). Porco-zumbi/Nether ficam em mobs.inc/nether.inc ----------
// Reaproveitam MobEnt/MOBS (mobs.inc): o tickMob() cuida da fisica e do andar (alvo em tx/tz); aqui fica a decisao (hostileAI), o
// ataque (hostileTick), a explosao do creeper e o spawn natural (escuro; porco-zumbi so sobre netherrack no Nether).
static int gMobVictim = -1;   // id de rede do jogador REMOTO que o mob em simulacao esta atacando (-1 = jogador local)
static bool mobCanHit() { return gMobVictim >= 0 || (!gDead && !creative); }   // remoto: o host so o escolhe como alvo se for atacavel (mobRemoteTick)
static bool mobDmg(int dmg, const char* why, double kx, double kz) {   // dano de mob: local = survDamage; remoto = NP_MOBDMG (so o alvo age)
    if (gMobVictim < 0) return survDamage(dmg, why);
    NBuf b = netMk(NP_MOBDMG); b.put<uint8_t>((uint8_t)gMobVictim); b.put<int16_t>((int16_t)dmg); b.put<float>((float)kx); b.put<float>((float)kz);
    const int l = (int)std::min<size_t>(strlen(why), 60); b.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)why[i]);
    netBroadcast(b); return true;
}
static bool mobLOS(double ox, double oy, double oz, double tx, double ty, double tz) {   // linha de visao sem bloco solido no meio
    const double dx = tx - ox, dy = ty - oy, dz = tz - oz, l = sqrt(dx * dx + dy * dy + dz * dz); if (l < 1e-3) return true;
    const float o[3] = {(float)ox, (float)oy, (float)oz}, d[3] = {(float)(dx / l), (float)(dy / l), (float)(dz / l)};
    const float t = raySolidDistance(o, d, (float)l); return t < 0.f || t > (float)l - .3f;
}
static MobEnt* mobRev(MobEnt& e) {   // alvo de revide (mob que me feriu): so zumbi/esqueleto; solta se morreu ou ficou longe
    if (e.revT <= 0 || !e.revId || (e.type != M_ZOMBIE && e.type != M_SKELETON)) return nullptr;
    for (MobEnt& o : MOBS) if (o.uid == e.revId && !o.dead && &o != &e) { if (fabs(o.x - e.x) > 32.0 || fabs(o.z - e.z) > 32.0 || fabs(o.y - e.y) > 14.0) break; return &o; }
    e.revT = 0; e.revId = 0; return nullptr;
}
static void hostileAI(MobEnt& e, double ppx, double ppy, double ppz) {
    bool rev = false; if (MobEnt* r = mobRev(e)) { ppx = r->x; ppy = r->y; ppz = r->z; rev = true; }   // revide tem prioridade sobre o jogador (HurtByTarget > NearestAttackable)
    const double dx = ppx - e.x, dz = ppz - e.z, dy = ppy - e.y, dist = sqrt(dx * dx + dz * dz);
    const bool tgt = rev || (mobCanHit() && fabs(dy) < 14.0);   // criativo/espectador nao e alvo
    const float want = (float)(atan2(-dx, -dz) * 57.29578);
    bool chase = false;
    switch (e.type) {
        case M_ZOMBIE: chase = tgt && dist < 35.0; break;                      // EntityZombie: segue a 40 blocos
        case M_CREEPER: {                                                       // EntityAICreeperSwell: pavio a < 3 blocos, mantem ate 7
            const bool swell = tgt && (e.fuse > 0 ? dist < 7.0 : dist < 3.0);
            if (swell) { e.fuse++; e.yaw = mturn(e.yaw, want, 30.f); } else if (e.fuse > 0) e.fuse--;
            chase = tgt && dist < 16.0 && !swell; break; }
        case M_SKELETON:
            if (tgt && dist < 16.0) {
                const bool los = mobLOS(e.x, e.y + 1.5, e.z, ppx, ppy + 1.2, ppz);
                if (!los || dist > 11.0) chase = true;                          // aproxima ate ter linha de visao
                else e.yaw = mturn(e.yaw, want, 30.f);                          // para e mira
            }
            break;
    }
    if (chase) { e.tx = ppx; e.tz = ppz; e.hasT = true; e.chase = true; }
    else if (e.chase) { e.hasT = false; e.chase = false; }
}
// resistencia a explosao (Block.getExplosionResistance 1.8.9 = dureza; pedras 6; obsidiana/bedrock/liquidos bloqueiam)
static float blastRes(int b) {
    if (b == BEDROCK || b == OBSIDIAN || b == XB_PORTAL) return 1000.f;
    if (isLiquid(b)) return 20.f;
    if (b == STONE || b == COBBLE) return 6.f;
    const float h = blockHardness(b); return h < 0.f ? 1000.f : h;
}
static void creeperBoom(MobEnt& e, Phys& P) {   // Explosion 1.8.9, forca 3: 1352 raios; dano = (i^2+i)/2*48+1 com i = 1 - dist/6
    const double cx = e.x, cy = e.y + .6, cz = e.z; const float pw = 3.f;
    std::vector<std::array<int, 3>> hits;
    for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) for (int k = 0; k < 16; k++) {
        if (i != 0 && i != 15 && j != 0 && j != 15 && k != 0 && k != 15) continue;
        double rx = i / 15.0 * 2 - 1, ry = j / 15.0 * 2 - 1, rz = k / 15.0 * 2 - 1; const double l = sqrt(rx * rx + ry * ry + rz * rz); rx /= l; ry /= l; rz /= l;
        double x = cx, y = cy, z = cz;
        for (float f = pw * (.7f + frand() * .6f); f > 0.f; f -= .225f) {
            const int bx = (int)floor(x), by = (int)floor(y), bz = (int)floor(z);
            if (by < 1 || by >= CH - 1) break;
            const int b = getB(bx, by, bz);
            if (b != AIR) { f -= (blastRes(b) + .3f) * .3f; if (f > 0.f) hits.push_back({bx, by, bz}); }
            x += rx * .3; y += ry * .3; z += rz * .3;
        }
    }
    std::sort(hits.begin(), hits.end()); hits.erase(std::unique(hits.begin(), hits.end()), hits.end());
    for (auto& h : hits) {
        const int b = getB(h[0], h[1], h[2]); if (b == AIR) continue;
        if (rand() % 3 == 0) { const int id = dropOf(b); if (id > 0) spawnDrop(h[0] + .5, h[1] + .5, h[2] + .5, frand() * .2 - .1, .2, frand() * .2 - .1, id, 1, 10); }
        setB(h[0], h[1], h[2], AIR); netSendBlock(h[0], h[1], h[2], AIR);
    }
    auto dmgAt = [&](double ex, double ey, double ez, int& dmg, double& imp) { const double dd = sqrt((ex - cx) * (ex - cx) + (ey - cy) * (ey - cy) + (ez - cz) * (ez - cz)); imp = 1.0 - dd / 6.0; dmg = imp > 0 ? (int)((imp * imp + imp) / 2.0 * 48.0 + 1.0) : 0; return dd; };
    { int dmg; double imp; const double dd = dmgAt(P.x, P.y, P.z, dmg, imp);
      if (dmg > 0 && mobCanHit()) { mobDmg(dmg, "explodido por um creeper", P.x - cx, P.z - cz); const double l = std::max(.1, dd); P.mx += (P.x - cx) / l * imp; P.my += (P.y + 1.0 - cy) / l * imp; P.mz += (P.z - cz) / l * imp; } }
    if (split2Targetable() && gSplitCtx != 2) { const Phys& q = *split2PhysP(); int dmg; double imp; const double dd = dmgAt(q.x, q.y, q.z, dmg, imp);   // jogador 2 tambem leva a explosao
        if (dmg > 0) { const double l = std::max(.1, dd); split2Damage(dmg, "explodido por um creeper", false, (float)((q.x - cx) / l * imp), (float)((q.z - cz) / l * imp)); } }
    for (MobEnt& m : MOBS) { if (&m == &e || m.dead) continue; int dmg; double imp; dmgAt(m.x, m.y, m.z, dmg, imp); if (dmg > 0) mobHurt(m, dmg, (float)(m.x - cx), (float)(m.z - cz), true); }
    for (int i = 0; i < 40; i++) {   // fumaca
        Puff p; p.x = p.px = (float)(cx + (frand() * 2.f - 1.f) * 2.5f); p.y = p.py = (float)(cy + (frand() * 2.f - 1.f) * 1.5f); p.z = p.pz = (float)(cz + (frand() * 2.f - 1.f) * 2.5f);
        p.vx = (frand() - .5f) * .12f; p.vy = frand() * .08f; p.vz = (frand() - .5f) * .12f; p.s = frand() * 4.f + 4.f; p.c = frand() * .3f + .5f; p.max = 24 + rand() % 16; p.age = 0; PUFFS.push_back(p);
    }
    e.hp = 0; e.dead = 20;   // sem drops: some no proximo tick
}
static void hostileTick(MobEnt& e, Phys& P) {
    if (e.dead || MD[e.type].ai < 4) return;
    const MobDef& d = MD[e.type];
    { const double pdx = P.x - e.x, pdz = P.z - e.z, ph = sqrt(pdx * pdx + pdz * pdz); if (ph > 96.0 || (ph > 40.0 && rand() % 900 == 0)) { e.y = -100; return; } }   // despawn: longe do jogador
    if (e.revT > 0) e.revT--;
    e.paim = e.aim;
    MobEnt* rv = mobRev(e);   // alvo de revide (outro mob) ou nullptr = jogador
    const double TX = rv ? rv->x : P.x, TY = rv ? rv->y : P.y, TZ = rv ? rv->z : P.z;
    const double dx = TX - e.x, dz = TZ - e.z, dy = TY - e.y, hd = sqrt(dx * dx + dz * dz);
    if (gDim == 0 && (e.type == M_ZOMBIE || e.type == M_SKELETON) && gSky.sunB > .85f && !e.inWater) {   // queimam ao sol (1 de dano por segundo)
        const int wx = (int)floor(e.x), wz = (int)floor(e.z), cx = fdiv(wx, CW), cz = fdiv(wz, CW), wy = (int)floor(e.y + d.h * .9); Chunk* c = getC(cx, cz);
        if (c && c->lc && wy >= 0 && wy < CH && (c->lc[idx(wx - cx * CW, wy, wz - cz * CW)] >> 4) >= 15) e.fire = std::max(e.fire, 160);   // sol acende (8 s); o dano por segundo vem de mobFireTick
    }
    if (e.atk > 0) e.atk--;
    if (e.type == M_ZOMBIE && e.atk == 0 && rv && hd < 1.4 && dy > -1.2 && dy < 1.9) {   // revide no mob que me feriu (3 de dano, 1 s)
        gMobAtkUid = mobUid(e); mobHurt(*rv, 3, (float)dx, (float)dz, true); gMobAtkUid = 0; e.atk = 20;
    } else if (e.type == M_ZOMBIE && e.atk == 0 && !rv && mobCanHit() && hd < 1.4 && dy > -1.2 && dy < 1.9) {   // corpo a corpo (a cada 1 s)
        if (mobDmg(3, "morto por um zumbi", dx, dz)) { const double l = std::max(.01, hd); P.mx += dx / l * .4; P.mz += dz / l * .4; P.my = std::max(P.my, .36); }
        e.atk = 20;
    }
    if (e.type == M_SKELETON) {   // arco: 1 flecha a cada 1-3 s, com linha de visao; mira no mob que o feriu (revide) ou no jogador
        bool aimed = false;
        if ((rv || mobCanHit()) && hd < 15.0 && hd > 1.0) {
            const double ox = e.x, oy = e.y + 1.5, oz = e.z, ty = TY + (rv ? MD[rv->type].h * .7 : 1.1);
            const float wantY = (float)(atan2(-dx, -dz) * 57.29578), faceD = fabsf(mwrap(wantY - e.yaw));   // so atira com o corpo virado (<= 50 graus) para o alvo
            if (faceD <= 50.f && mobLOS(ox, oy, oz, TX, ty, TZ)) {
                aimed = true;
                if (e.atk == 0) {
                    double vx = dx, vy = (ty - oy) + hd * .2, vz = dz; const double l = std::max(1e-6, sqrt(vx * vx + vy * vy + vz * vz));
                    auto G = []() { return (frand() + frand() + frand() + frand() - 2.f) * .13f * .075f; };
                    vx = vx / l + G(); vy = vy / l + G(); vz = vz / l + G();
                    arrowSpawn(ox, oy, oz, vx * 1.6, vy * 1.6, vz * 1.6, false, -2, false, mobUid(e));
                    e.atk = 20 + rand() % 40;
                }
            }
        }
        e.aim += ((aimed ? 1.f : 0.f) - e.aim) * .35f;   // animacao: relaxado <-> mirando (lida em drawMob)
    }
    if (e.type == M_CREEPER && e.fuse >= 30) creeperBoom(e, P);
}
// Spawn natural de animais (Java: grama, luz > 8, grupos de 2 a 4). Aqui: a cada 5 s, tenta 1 grupo a 24-48 blocos do jogador, com teto de animais por perto.
static void animalSpawn(Phys& P) {
    if (gDim != 0 || gDead || (netOn && !netHost)) return;   // so overworld; convidado nao gera (os mobs ainda nao sincronizam, senao ficaria duplicado)
    static int t = 0;
    int n = 0;
    for (MobEnt& e : MOBS) {
        if (e.dead || !(e.type == M_PIG || e.type == M_CHICKEN || e.type == M_COW || e.type == M_SHEEP)) continue;
        const double dx = e.x - P.x, dz = e.z - P.z;
        if (!e.rideBoat && dx * dx + dz * dz > 128.0 * 128.0) { e.y = -100; continue; }   // despawn: animal muito longe do jogador (a 128 blocos ou mais)
        if (fabs(dx) < 80 && fabs(dz) < 80) n++;
    }
    if (++t < (n < 4 ? 20 : 100)) return; t = 0;   // poucos animais por perto (mundo recem-gerado): tenta a cada 1 s em vez de 5 s => spawn inicial rapido
    if (n >= 12) return;
    for (int tries = 0; tries < 8; tries++) {
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 24.0;
        const int x = (int)floor(P.x + cos(ang) * r), z = (int)floor(P.z + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        int y = -1; for (int yy = CH - 3; yy > 2; yy--) if (getB(x, yy - 1, z) == GRASS && !solid(getB(x, yy, z)) && !solid(getB(x, yy + 1, z)) && !isLiquid(getB(x, yy, z))) { y = yy; break; }
        if (y < 0) continue;
        const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4;
        if (sk * std::max(0.f, (gSky.sunB - .2f) / .8f) < 9.f && sk < 15) continue;   // precisa de ceu aberto/claro (grama coberta ou noite escura nao gera)
        static const int KIND[4] = {M_COW, M_SHEEP, M_PIG, M_CHICKEN};
        const int type = KIND[rand() % 4], cnt = 2 + rand() % 3;
        for (int i = 0; i < cnt; i++) {
            const int px = x + rand() % 5 - 2, pz = z + rand() % 5 - 2; int py = -1;
            for (int yy = std::min(CH - 3, y + 3); yy >= std::max(3, y - 3); yy--) if (getB(px, yy - 1, pz) == GRASS && !solid(getB(px, yy, pz)) && !solid(getB(px, yy + 1, pz)) && !isLiquid(getB(px, yy, pz))) { py = yy; break; }
            if (py >= 0 && getC(fdiv(px, CW), fdiv(pz, CW))) spawnMob(type, px + .5, py, pz + .5, frand() * 360.f);
        }
        return;
    }
}

static void hostileSpawn(Phys& P) {   // a cada 2 s, ate 14 hostis; escuro (luz <= 7), 24-40 blocos do jogador
    if (gDim != 0) return;   // so no overworld (porco-zumbi do Nether: nether.inc)
    static int t = 0; if (++t < 40) return; t = 0;
    int n = 0; for (const MobEnt& e : MOBS) if (MD[e.type].ai >= 4 && !e.dead) n++;
    if (n >= 14 || gDead) return;
    for (int tries = 0; tries < 6; tries++) {
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 16.0;
        const int x = (int)floor(P.x + cos(ang) * r), z = (int)floor(P.z + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        int y = -1; for (int yy = std::min(CH - 4, (int)P.y + rand() % 17 - 8), k = 0; yy > 2 && k < 24; yy--, k++) if (solid(getB(x, yy - 1, z)) && !isLiquid(getB(x, yy - 1, z)) && !solid(getB(x, yy, z)) && !solid(getB(x, yy + 1, z)) && !isLiquid(getB(x, yy, z))) { y = yy; break; }
        if (y < 0) continue;
        const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4, bl = v & 15;
        const float effSky = gDim ? 0.f : sk * std::max(0.f, (gSky.sunB - .2f) / .8f);
        if (std::max((float)bl, effSky) > 7.f) continue;
        int type;
        { const int q = rand() % 10; type = q < 4 ? M_ZOMBIE : q < 7 ? M_SKELETON : M_CREEPER; }
        spawnMob(type, x + .5, y, z + .5, frand() * 360.f); return;
    }
}
