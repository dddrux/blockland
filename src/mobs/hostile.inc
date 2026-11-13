// ---------- Mobs hostis: zumbi, esqueleto e creeper (overworld). Porco-zumbi/Nether ficam em mobs.inc/nether.inc ----------
// Reaproveitam MobEnt/MOBS (mobs.inc): o tickMob() cuida da fisica e do andar (alvo em tx/tz); aqui fica a decisao (hostileAI), o
// ataque (hostileTick), a explosao do creeper e o spawn natural (escuro; porco-zumbi so sobre netherrack no Nether).
static int gMobVictim = -1;   // id de rede do jogador REMOTO que o mob em simulacao esta atacando (-1 = jogador local)
static bool mobCanHit() { return gMobVictim >= 0 || (!gDead && !creative); }   // remoto: o host so o escolhe como alvo se for atacavel (mobRemoteTick)
static bool mobDmg(int dmg, const char* why, double kx, double kz) {   // dano de mob: local = survDamage; remoto = NP_MOBDMG (so o alvo age)
    dmg = diffDmg(dmg); if (dmg <= 0) return false;   // patchDif.1: dificuldade (Facil: min(d/2+1,d); Dificil: d*3/2)
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
    if (e.revT <= 0 || !e.revId || !(e.type == M_ZOMBIE || isArch(e.type) || e.type == M_ZVILL || e.type == M_WSKEL)) return nullptr;
    for (MobEnt& o : MOBS) if (o.uid == e.revId && !o.dead && &o != &e) { if (fabs(o.x - e.x) > 32.0 || fabs(o.z - e.z) > 32.0 || fabs(o.y - e.y) > 14.0) break; return &o; }
    e.revT = 0; e.revId = 0; return nullptr;
}
// ---- patchEnd.12: Enderman (EntityEnderman 1.12.2): neutro; fica hostil quando o jogador o encara (cabeca), teleporta ao ser acertado de longe / na agua / encarado de perto ----
static bool endermanTpTo(MobEnt& e, double nx, double ny, double nz) {   // desce ate o chao a partir de ny; exige 3 blocos livres e chunk carregado
    const int x = (int)floor(nx), z = (int)floor(nz); if (!getC(fdiv(x, CW), fdiv(z, CW))) return false;
    for (int y = std::min(CH - 6, (int)floor(ny) + 8); y > 3 && y > (int)floor(ny) - 32; y--) {
        if (!solid(getB(x, y - 1, z)) || isLiquid(getB(x, y - 1, z))) continue;
        if (solid(getB(x, y, z)) || solid(getB(x, y + 1, z)) || solid(getB(x, y + 2, z)) || isLiquid(getB(x, y, z)) || isLiquid(getB(x, y + 1, z))) return false;
        e.x = e.px = x + .5; e.y = e.py = y; e.z = e.pz = z + .5; e.mx = e.my = e.mz = 0; e.hasT = false; e.fall = 0.f; e.endTpCd = 6; return true;
    }
    return false;
}
static bool endermanTeleport(MobEnt& e) { for (int t = 0; t < 64; t++) if (endermanTpTo(e, e.x + (frand() - .5) * 64., e.y + (double)(rand() % 64 - 32), e.z + (frand() - .5) * 64.)) return true; return false; }
static void endermanTick(MobEnt& e, Phys& P, bool rv, double dx, double dz, double dy, double hd) {
    if (e.endTpCd > 0) e.endTpCd--;
    if (e.endHp <= 0) e.endHp = e.hp;
    if (e.hp < e.endHp && e.hp > 0) { if (!rv && mobCanHit()) { if (hd > 5.0) { e.hp = e.endHp; endermanTeleport(e); } e.endAgg = 1; } }   // projetil: desvia (sem dano) teleportando   // acertado: fica hostil; flecha/projetil (de longe) faz ele "desviar" teleportando
    e.endHp = e.hp;
    if ((e.inWater || e.wetT > 0) && (gWorldTime % 20) == 0) { gMobAtkUid = 0; mobHurt(e, 1, 0.f, 0.f, false); if (e.hp > 0 && rand() % 10 != 0) endermanTeleport(e); e.endHp = e.hp; }   // agua machuca (1/s) e faz teleportar
    if (e.endAgg == 0 && !(netOn && !netHost) && !e.dead) {   // pega (1/20) e solta (1/2000) blocos: grama, terra, areia, cascalho
        if (!e.endBlk && rand() % 20 == 0) { const int x = (int)floor(e.x) + rand() % 5 - 2, y = (int)floor(e.y) + rand() % 3, z = (int)floor(e.z) + rand() % 5 - 2; const int b = getB(x, y, z);
            if (getC(fdiv(x, CW), fdiv(z, CW)) && (b == GRASS || b == DIRT || b == SAND || b == GRAVEL)) { e.endBlk = b; setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); } }
        else if (e.endBlk && rand() % 2000 == 0) { const int x = (int)floor(e.x) + rand() % 3 - 1, y = (int)floor(e.y) + rand() % 2, z = (int)floor(e.z) + rand() % 3 - 1;
            if (getC(fdiv(x, CW), fdiv(z, CW)) && getB(x, y, z) == AIR && solid(getB(x, y - 1, z))) { setBM(x, y, z, e.endBlk, 0); netSendBlock(x, y, z, e.endBlk, 0); e.endBlk = 0; } }
    }
    if (e.dead || e.hp <= 0) return;
    const bool tgt = !rv && mobCanHit();
    if (!tgt) { e.endAgg = 0; e.endStare = 0; return; }
    const double ex = P.x, ey = P.y + 1.62, ez = P.z, tx = e.x, ty = e.y + 2.55, tz = e.z, vx = tx - ex, vy = ty - ey, vz = tz - ez, dist = std::max(.01, sqrt(vx * vx + vy * vy + vz * vz));
    const float cp = cosf(gPlayerPitch * .0174533f); const double lx = -sinf(gPlayerYaw * .0174533f) * cp, ly = sinf(gPlayerPitch * .0174533f), lz = -cosf(gPlayerYaw * .0174533f) * cp;
    const bool staring = dist < 64.0 && (lx * vx + ly * vy + lz * vz) / dist > 1.0 - .025 / dist && mobLOS(ex, ey, ez, tx, ty, tz);   // EntityEnderman.shouldAttackPlayer
    if (e.endAgg == 0) {
        if (staring) { e.hasT = false; e.yaw = mturn(e.yaw, (float)(atan2(-(P.x - e.x), -(P.z - e.z)) * 57.29578), 30.f); if (++e.endStare >= 5) { e.endAgg = 1; e.endStare = 0; } } else e.endStare = 0;
        return;
    }
    if (staring && dist < 4.0 && e.endTpCd == 0) { endermanTeleport(e); e.endTpCd = 10; }   // encarado de perto: pisca para longe
    else if (!staring && dist > 16.0 && (gWorldTime % 30) == 0) endermanTpTo(e, e.x + (P.x - e.x) * (16.0 / dist) + (frand() - .5) * 8., P.y, e.z + (P.z - e.z) * (16.0 / dist) + (frand() - .5) * 8.);   // longe demais: teleporta 16 blocos em direcao ao alvo
    if (e.atk == 0 && hd < 2.0 && dy > -1.5 && dy < 2.5) {   // corpo a corpo: 7 de dano (Normal), 1 s
        if (mobDmg(7, "morto por um Enderman", dx, dz)) { const double l = std::max(.01, hd); P.mx += dx / l * .4; P.mz += dz / l * .4; P.my = std::max(P.my, .36); }
        e.atk = 20;
    }
}
// patchMob.6: esqueleto de dia procura sombra (EntityAIFleeSun/RestrictSun): luz do ceu < 15 = coberto (arvore, teto, caverna)
static bool skyExposed(double x, double y, double z) {
    const int wx = (int)floor(x), wz = (int)floor(z), wy = (int)floor(y + 1.0), cx = fdiv(wx, CW), cz = fdiv(wz, CW); Chunk* c = getC(cx, cz);
    return c && c->lc && wy >= 0 && wy < CH && (c->lc[idx(wx - cx * CW, wy, wz - cz * CW)] >> 4) >= 15;
}
static bool skelFleeSun(MobEnt& e) {
    const bool rainy = gRain >= .2f || gThunder >= .2f;
    if (gDim != 0 || rainy || gSky.sunB <= .85f || e.inWater || false) { e.sunFlee = false; return false; }
    if (!skyExposed(e.x, e.y, e.z)) { e.sunFlee = false; return false; }   /* ja na sombra: fica (e atira) */
    if (e.sunFlee && e.hasT && (gWorldTime % 20) != 0) return true;
    for (int tr = 0; tr < 24; tr++) {   /* amostra pontos num raio de 12 e pega um coberto, com chao e sem parede */
        const double tx = e.x + (rand() % 25 - 12), tz = e.z + (rand() % 25 - 12);
        int ty = (int)floor(e.y) + 2, fy = -1;
        for (int yy = ty; yy >= std::max(1, ty - 5); yy--) if (solid(getB((int)floor(tx), yy - 1, (int)floor(tz))) && !solid(getB((int)floor(tx), yy, (int)floor(tz)))) { fy = yy; break; }
        if (fy < 0 || isLiquid(getB((int)floor(tx), fy, (int)floor(tz))) || skyExposed(tx, fy, tz)) continue;
        e.tx = tx; e.tz = tz; e.hasT = true; e.sunFlee = true; return true;
    }
    e.sunFlee = false; return false;   /* sem sombra por perto: segue a IA normal */
}
static void hostileAI(MobEnt& e, double ppx, double ppy, double ppz) {
    if (isArch(e.type) && skelFleeSun(e)) { e.chase = true; return; }   /* foge do sol antes de atacar (prioridade da IA vanilla) */
    bool rev = false; if (MobEnt* r = mobRev(e)) { ppx = r->x; ppy = r->y; ppz = r->z; rev = true; }   // revide tem prioridade sobre o jogador (HurtByTarget > NearestAttackable)
    const double dx = ppx - e.x, dz = ppz - e.z, dy = ppy - e.y, dist = sqrt(dx * dx + dz * dz);
    const bool tgt = rev || (mobCanHit() && fabs(dy) < 14.0);   // criativo/espectador nao e alvo
    const float want = (float)(atan2(-dx, -dz) * 57.29578);
    bool chase = false;
    switch (e.type) {
        case M_ZOMBIE: case M_HUSK: case M_ZVILL: case M_WSKEL: case M_VEX: chase = tgt && dist < 35.0; break;
        case M_WITHER:   // patchMobNew.6: wither: persegue ate 14 blocos e dispara caveiras (aprox.: 6 de dano direto a cada 1,5 s, ate 30 blocos com linha de visao)
            if (tgt && dist < 48.0) { if (dist > 14.0) chase = true; else e.yaw = mturn(e.yaw, want, 30.f);
                if (!rev && dist < 30.0 && e.atk == 0 && mobLOS(e.x, e.y + 3.0, e.z, ppx, ppy + 1.2, ppz)) { e.atk = 30; mobDmg(6, "morto por Wither", dx, dz); } }
            break;
        case M_WITCH: case M_ILLUS:   // patchMobNew.5: bruxa arremessa pocao (aprox.: 3 de dano direto a cada 3 s, ate 12 blocos com linha de visao)
            if (tgt && dist < 16.0) { if (dist > 9.0) chase = true; else e.yaw = mturn(e.yaw, want, 30.f);
                if (!rev && dist < 12.0 && e.atk == 0 && mobLOS(e.x, e.y + 1.5, e.z, ppx, ppy + 1.2, ppz)) { e.atk = 60; mobDmg(3, "morto por uma bruxa", dx, dz); } }
            break;
        case M_VINDIC: chase = tgt && dist < 35.0; break;   // patchMobNew.4: vindicador (machado, 13 de dano)
        case M_EVOKER:   // patchMobNew.4: evocador: mantem ~10 blocos e invoca 3 vexes a cada 17 s
            if (tgt && dist < 24.0) { if (dist > 10.0) chase = true; else e.yaw = mturn(e.yaw, want, 30.f);
                if (!rev && dist < 16.0 && e.atk == 0 && mobLOS(e.x, e.y + 1.5, e.z, ppx, ppy + 1.2, ppz)) { e.atk = 340; gVexQ.push_back({e.x, e.y, e.z}); } }
            break;
        case M_SLIME: case M_MAGMA: chase = tgt && dist < 16.0; break;   // patchSlime.2
        case M_CSPIDER: case M_SILVER: case M_ENDERMITE: chase = tgt && dist < 16.0; break;
        case M_ENDERMAN: chase = tgt && e.endAgg > 0 && dist < 64.0; break;   // so persegue depois de provocado (patchEnd.12)   // aranha da caverna: sempre hostil
        case M_SPIDER: chase = tgt && dist < 16.0 && (rev || gSky.sunB <= .5f || e.hurt > 0); break;   // neutra de dia (so ataca se ferida), hostil a noite                      // EntityZombie: segue a 40 blocos
        case M_CREEPER: {                                                       // EntityAICreeperSwell: pavio a < 3 blocos, mantem ate 7
            const bool swell = tgt && (e.fuse > 0 ? dist < 7.0 : dist < 3.0);
            if (swell) { e.fuse++; e.yaw = mturn(e.yaw, want, 30.f); } else if (e.fuse > 0) e.fuse--;
            chase = tgt && dist < 16.0 && !swell; break; }
        case M_BLAZE:                                                           // paira a ~8 blocos e atira bolas de fogo
            if (tgt && dist < 40.0) { if (dist > 8.0) chase = true; else e.yaw = mturn(e.yaw, want, 30.f); }
            break;
        case M_SKELETON: case M_STRAY:
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
struct TntEnt { double x, y, z, px, py, pz, vx, vy, vz; int fuse; };   // (movido de falling.inc no patch5.71: a explosao precisa empurrar as TNTs acesas)
static std::vector<TntEnt> TNTS; static const int TNT_MAX_BOOM = 6;
static int gTntId = 0; static void tntPrime(double x, double y, double z, int fuse);   // falling.inc (patch5.64)
static void propExplode(double cx, double cy, double cz, float pw);   // propsys.inc (patchMec.5b)
static void explosionAt(double cx, double cy, double cz, float pw, Phys& P, const char* cause, MobEnt* self) {   // Explosion 1.8.9: 1352 raios; dano = (i^2+i)/2*8*(2*pw)+1 com i = 1 - dist/(2*pw); pw 3 = creeper, 4 = TNT
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
    netBatchBegin();   // patch5.74: blocos da explosao vao em lote
    for (auto& h : hits) {
        const int b = getB(h[0], h[1], h[2]); if (b == AIR) continue;
        if (gTntId && b == gTntId) { setB(h[0], h[1], h[2], AIR); netSendBlock(h[0], h[1], h[2], AIR); tntPrime(h[0] + .5, h[1], h[2] + .5, 10 + rand() % 20); continue; }   // TNT na explosao: acende em cadeia
        if (!isLiquid(b) && b != XB_FIRE && b != XB_PORTAL && rand() % std::max(1, (int)pw) == 0) blockDrops(h[0], h[1], h[2], b);   // patchTnt.1: tabela real de drops (antes dropOf devolvia o id do bloco: lava/portal/variantes viravam item estranho); chance 1/forca como o MC
        setB(h[0], h[1], h[2], AIR); netSendBlock(h[0], h[1], h[2], AIR);
    }
    netBatchEnd();
    if (!(netOn && !netHost)) propExplode(cx, cy, cz, pw);   // patchMec.5b: props de mod com resistencia
    const double RR = pw * 2.0; auto dmgAt = [&](double ex, double ey, double ez, int& dmg, double& imp) { const double dd = sqrt((ex - cx) * (ex - cx) + (ey - cy) * (ey - cy) + (ez - cz) * (ez - cz)); imp = 1.0 - dd / RR; dmg = imp > 0 ? (int)((imp * imp + imp) / 2.0 * 8.0 * RR + 1.0) : 0; return dd; };
    { int dmg; double imp; const double dd = dmgAt(P.x, P.y, P.z, dmg, imp);
      if (dmg > 0 && mobCanHit()) { mobDmg(dmg, cause, P.x - cx, P.z - cz); const double l = std::max(.1, dd); P.mx += (P.x - cx) / l * imp; P.my += (P.y + 1.0 - cy) / l * imp; P.mz += (P.z - cz) / l * imp; } }
    if (gSplitCtx != 2) for (int xk = 0; xk < split2N(); xk++) if (split2TargetableK(xk)) { const Phys& q = *split2PhysK(xk); int dmg; double imp; const double dd = dmgAt(q.x, q.y, q.z, dmg, imp);   // jogadores do split tambem levam a explosao
        if (dmg > 0) { const double l = std::max(.1, dd); split2DamageK(xk, dmg, cause, false, (float)((q.x - cx) / l * imp), (float)((q.z - cz) / l * imp)); } }
    if (netOn && netHost) for (const auto& kv : remotes) {   // convidados na mesma dimensao levam dano/recuo (NP_BOOM; a vitima aplica)
        const RemotePl& r = kv.second; if (!r.init || r.dim != gDim || r.gm == 3 || r.deathT >= 0) continue;
        int dmg; double imp; const double dd = dmgAt(r.x, r.y, r.z, dmg, imp); if (dmg <= 0) continue; const double l = std::max(.1, dd);
        NBuf b = netMk(NP_BOOM); b.put<uint8_t>((uint8_t)kv.first); b.put<int16_t>((int16_t)dmg); b.put<uint8_t>(pw > 3.5f ? 1 : 0);
        b.put<float>((float)((r.x - cx) / l * imp)); b.put<float>((float)((r.y + 1.0 - cy) / l * imp)); b.put<float>((float)((r.z - cz) / l * imp)); netBroadcast(b); }
    for (TntEnt& e : TNTS) { const double dd = sqrt((e.x - cx) * (e.x - cx) + (e.y - cy) * (e.y - cy) + (e.z - cz) * (e.z - cz)), imp = 1.0 - dd / RR;   // patch5.71: TNT acesa vizinha leva recuo (Entity.motion += dir * impacto), como no Java
        if (imp > 0 && dd > .01) { e.vx += (e.x - cx) / dd * imp; e.vy += (e.y - cy) / dd * imp; e.vz += (e.z - cz) / dd * imp; } }
    for (MobEnt& m : MOBS) { if ((self && &m == self) || m.dead) continue; int dmg; double imp; dmgAt(m.x, m.y, m.z, dmg, imp); if (dmg > 0) mobHurt(m, dmg, (float)(m.x - cx), (float)(m.z - cz), true); }
    HBOOMS.push_back({(float)cx, (float)cy, (float)cz, 0});
    sndEvtAt("explode", cx, cy, cz);   // patchAud.OGG: som da explosao (TNT, creeper, carrinho); convidado toca no NP_EXPL
    if (netOn && netHost) { NBuf xb = netMk(NP_EXPL); xb.put<uint8_t>((uint8_t)(gDim & 255)); xb.put<float>((float)cx); xb.put<float>((float)cy); xb.put<float>((float)cz); netBroadcast(xb); }   // patchTnt.2: explosao grande (MC usa hugeexplosion quando forca >= 2)
    for (int i = 0; i < 40; i++) {   // fumaca
        Puff p; p.x = p.px = (float)(cx + (frand() * 2.f - 1.f) * 2.5f); p.y = p.py = (float)(cy + (frand() * 2.f - 1.f) * 1.5f); p.z = p.pz = (float)(cz + (frand() * 2.f - 1.f) * 2.5f);
        p.vx = (frand() - .5f) * .12f; p.vy = frand() * .08f; p.vz = (frand() - .5f) * .12f; p.s = frand() * 4.f + 4.f; p.c = frand() * .3f + .5f; p.max = 24 + rand() % 16; p.age = 0; PUFFS.push_back(p);
    }
}
static void creeperBoom(MobEnt& e, Phys& P) { explosionAt(e.x, e.y + .6, e.z, 3.f, P, "explodido por um creeper", &e); e.hp = 0; e.dead = 20; }   // sem drops: some no proximo tick
// patch5.64: jogadores "ativos" (local, P2 do split e remotos vivos/nao espectadores). Antes o despawn e o spawn olhavam so o jogador local: no multiplayer os mobs
// perto de um convidado sumiam (longe do host) e nada nascia perto dele.
static std::vector<Phys*> pistonAll();   // splitscreen.inc
static void activePlayers(const Phys& P, std::vector<std::array<double, 3>>& out) {
    out.clear(); out.push_back({P.x, P.y, P.z});
    for (Phys* p2 : pistonAll()) if (p2 && p2 != &P) out.push_back({p2->x, p2->y, p2->z});
    for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (r.init && r.gm != 3 && r.deathT < 0) out.push_back({r.x, r.y, r.z}); }
}
static double playerNear2(double x, double z, const Phys& P) {   // distancia horizontal ao quadrado ate o jogador mais proximo
    std::vector<std::array<double, 3>> v; activePlayers(P, v); double b = 1e30;
    for (auto& q : v) b = std::min(b, (q[0] - x) * (q[0] - x) + (q[2] - z) * (q[2] - z));
    return b;
}
struct Fball { double x, y, z, px, py, pz, vx, vy, vz; int age; };   // bola de fogo pequena do blaze (patchNether.2)
static std::vector<Fball> FBALLS;
// patchNet.2: o host avisa cada bola de fogo (NP_FBALL); o convidado a simula e leva o dano/fogo sozinho (fireballsTick roda nos dois lados)
static void fbNetSend(const Fball& f) { if (!netOn || !netHost) return; NBuf b = netMk(NP_FBALL); b.put<uint8_t>((uint8_t)(gDim & 255)); b.put<float>((float)f.x); b.put<float>((float)f.y); b.put<float>((float)f.z); b.put<float>((float)f.vx); b.put<float>((float)f.vy); b.put<float>((float)f.vz); netBroadcast(b); }
static void fbNetRecv(NBuf& b, int sender) {
    const int dm = b.get<uint8_t>(); const double x = b.get<float>(), y = b.get<float>(), z = b.get<float>(), vx = b.get<float>(), vy = b.get<float>(), vz = b.get<float>();
    if (netHost || sender != 0 || dm != (gDim & 255) || FBALLS.size() > 64) return; FBALLS.push_back({x, y, z, x, y, z, vx, vy, vz, 0});
}
static void fireballsTick(Phys& P) {
    for (size_t i = 0; i < FBALLS.size();) {
        Fball& f = FBALLS[i]; f.px = f.x; f.py = f.y; f.pz = f.z; f.x += f.vx; f.y += f.vy; f.z += f.vz; f.age++;
        bool die = f.age > 200 || solid(getB((int)floor(f.x), (int)floor(f.y), (int)floor(f.z)));
        if (!die && !gDead && mobCanHit() && fabs(f.x - P.x) < .5 && fabs(f.z - P.z) < .5 && f.y > P.y - .2 && f.y < P.y + 1.9) { if (mobDmg(5, "queimado por uma bola de fogo", f.vx, f.vz)) gFire = std::max(gFire, 100); die = true; }
        if (die) { FBALLS[i] = FBALLS.back(); FBALLS.pop_back(); } else ++i;
    }
}
static void fireballsDraw(float pt) {   // sprite da carga de fogo sempre de frente para a camera
    static const int fid = pkFind("item/fire_charge"); if (FBALLS.empty() || !fid) return; glColor4f(1.f, 1.f, 1.f, 1.f);
    for (const Fball& f : FBALLS) {
        const double ix = f.px + (f.x - f.px) * pt, iy = f.py + (f.y - f.py) * pt, iz = f.pz + (f.z - f.pz) * pt;
        rdPushMatrix(); rdTranslatef((float)ix, (float)iy, (float)iz); rdRotatef(gBbYaw, 0, 1, 0); rdRotatef(gBbPitch, 1, 0, 0); rdScalef(.45f, .45f, .45f); rdTranslatef(-.5f, -.5f, -.5f); drawHeldRaw(fid); rdPopMatrix();
    }
}
static bool wxCold(double x, double z);   // weather.inc
static void hostileTick(MobEnt& e, Phys& P) {
    if (e.dead || MD[e.type].ai < 4) return;
    const MobDef& d = MD[e.type];
    { const double ph = sqrt(playerNear2(e.x, e.z, P)); if (e.type != M_WITHER && (ph > 96.0 || (ph > 40.0 && rand() % 900 == 0))) { e.y = -100; return; } }   // despawn: longe do jogador
    if (e.revT > 0) e.revT--;
    if (e.life > 0 && --e.life == 0) { e.dead = 1; return; }   // patchMobNew.4: vex invocado expira
    e.paim = e.aim;
    MobEnt* rv = mobRev(e);   // alvo de revide (outro mob) ou nullptr = jogador
    const double TX = rv ? rv->x : P.x, TY = rv ? rv->y : P.y, TZ = rv ? rv->z : P.z;
    const double dx = TX - e.x, dz = TZ - e.z, dy = TY - e.y, hd = sqrt(dx * dx + dz * dz);
    const bool rainy = gDim == 0 && (gRain >= .2f || gThunder >= .2f) && !wxCold(e.x, e.z);   // chuva: o ceu escurece (brilho < 0,5) e a agua da chuva apaga o fogo (Entity.isWet)
    if (rainy && e.fire > 0) { const int wx = (int)floor(e.x), wz = (int)floor(e.z), cx = fdiv(wx, CW), cz = fdiv(wz, CW), wy = (int)floor(e.y + d.h * .9); Chunk* c = getC(cx, cz);
        if (c && c->lc && wy >= 0 && wy < CH && (c->lc[idx(wx - cx * CW, wy, wz - cz * CW)] >> 4) >= 15) e.fire = 0; }
    if (gDim == 0 && !rainy && (e.type == M_ZOMBIE || e.type == M_ZVILL || isArch(e.type)) && gSky.sunB > .85f && !e.inWater && e.wetT <= 0) {   // patchMob.9: pulando na agua (sai por 1 tick) nao acende ao sol   // queimam ao sol (1 de dano por segundo)
        const int wx = (int)floor(e.x), wz = (int)floor(e.z), cx = fdiv(wx, CW), cz = fdiv(wz, CW), wy = (int)floor(e.y + d.h * .9); Chunk* c = getC(cx, cz);
        if (c && c->lc && wy >= 0 && wy < CH && (c->lc[idx(wx - cx * CW, wy, wz - cz * CW)] >> 4) >= 15) e.fire = std::max(e.fire, 160);   // sol acende (8 s); o dano por segundo vem de mobFireTick
    }
    if (e.atk > 0) e.atk--;
    if ((e.type == M_SPIDER || e.type == M_CSPIDER || e.type == M_SILVER || e.type == M_ENDERMITE) && e.atk == 0 && e.chase && !rv && mobCanHit() && hd < 1.8 && dy > -1.2 && dy < 1.9) {   // mordida: 2 de dano a cada 1 s
        if (mobDmg(e.type == M_SILVER ? 1 : 2, e.type == M_SILVER ? "morto por uma traca" : e.type == M_ENDERMITE ? "morto por um endermite" : "morto por uma aranha", dx, dz)) { const double l = std::max(.01, hd); P.mx += dx / l * .3; P.mz += dz / l * .3; }
        e.atk = 20;
    }
    if (isMelee(e.type) && e.atk == 0 && rv && hd < 1.4 && dy > -1.2 && dy < 1.9) {   // revide no mob que me feriu (3 de dano, 1 s)
        gMobAtkUid = mobUid(e); mobHurt(*rv, e.type == M_VINDIC ? 13 : e.type == M_VEX ? 4 : 3, (float)dx, (float)dz, true); gMobAtkUid = 0; e.atk = 20;
    } else if (isMelee(e.type) && e.atk == 0 && !rv && mobCanHit() && hd < 1.4 && dy > -1.2 && dy < 1.9) {   // corpo a corpo (a cada 1 s)
        if (mobDmg(e.type == M_VINDIC ? 13 : e.type == M_VEX ? 4 : 3, e.type == M_VINDIC ? "morto por um vindicador" : e.type == M_VEX ? "morto por um vex" : "morto por um zumbi", dx, dz)) { const double l = std::max(.01, hd); P.mx += dx / l * .4; P.mz += dz / l * .4; P.my = std::max(P.my, .36); }
        e.atk = 20;
    }
    if (isSlimy(e.type) && e.ssz > 1 && e.atk == 0 && !rv && mobCanHit() && hd < .6 * e.ssz + .3 && dy > -1.8 && dy < .51 * e.ssz + .2) {   // patchSlime.2: so o slime medio/grande machuca (dano = tamanho, a cada 1 s)
        if (mobDmg(e.ssz, "morto por um Slime", dx, dz)) { const double l = std::max(.01, hd); P.mx += dx / l * .4; P.mz += dz / l * .4; P.my = std::max(P.my, .36); }
        e.atk = 20;
    }
    if (isArch(e.type)) {   // arco: 1 flecha a cada 1-3 s, com linha de visao; mira no mob que o feriu (revide) ou no jogador
        bool aimed = false;
        if ((rv || mobCanHit()) && hd < 15.0 && hd > 1.0) {
            const double ox = e.x, oy = e.y + 1.5, oz = e.z, ty = TY + (rv ? mobH(*rv) * .7 : 1.1);
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
    if (e.type == M_BLAZE && e.atk == 0 && (rv || mobCanHit()) && hd < 24.0 && hd > 2.0) {   // rajada de 3 bolas (6 ticks entre elas) e pausa de 5 s
        const double ox = e.x, oy = e.y + 1.1, oz = e.z, ty = TY + (rv ? mobH(*rv) * .7 : 1.0);
        if (mobLOS(ox, oy, oz, TX, ty, TZ)) {
            double vx = dx, vy = ty - oy, vz = dz; const double l = std::max(1e-6, sqrt(vx * vx + vy * vy + vz * vz)); const double j = .02 * sqrt(hd);
            FBALLS.push_back({ox, oy, oz, ox, oy, oz, vx / l * .5 + (frand() - .5) * j, vy / l * .5 + (frand() - .5) * j, vz / l * .5 + (frand() - .5) * j, 0});
            fbNetSend(FBALLS.back());
            if (++e.fuse >= 3) { e.fuse = 0; e.atk = 100; } else e.atk = 6;
        }
    }
    if (e.type == M_ENDERMAN) endermanTick(e, P, rv != nullptr, dx, dz, dy, hd);
    if (e.type == M_CREEPER && e.fuse >= 30) creeperBoom(e, P);
}
// Spawn natural de animais (Java: grama, luz > 8, grupos de 2 a 4). Aqui: a cada 5 s, tenta 1 grupo a 24-48 blocos do jogador, com teto de animais por perto.
static bool rsConnects(int nb);   // redstone.inc (botao, placa, fio, alavanca, tocha...: "fornecem energia" => o MC nao deixa spawnar nessa celula)
// patchFix.5 (BUG-025): WorldEntitySpawner.canCreatureTypeSpawnAtLocation: chao = face de CIMA cheia (laje de baixo nao vale); celula e a de cima sem colisao, sem liquido e sem redstone
static bool spawnFloorFull(int x, int y, int z) {
    const int b = getB(x, y, z); if (!solid(b) || isLiquid(b)) return false;
    AABB bx[SH_MAXB]; const int n = blockBoxes(x, y, z, false, bx); if (n == 0) return true;   // sem forma propria = cubo cheio
    for (int i = 0; i < n; i++) if (bx[i].y1 >= y + 1 - 1e-4 && bx[i].x1 - bx[i].x0 >= .99 && bx[i].z1 - bx[i].z0 >= .99) return true;
    return false;
}
static bool spawnCellFree(int x, int y, int z) {
    const int b = getB(x, y, z); if (solid(b) || isLiquid(b) || rsConnects(b)) return false;
    AABB bx[SH_MAXB]; return blockBoxes(x, y, z, false, bx) == 0;
}
// patchFix.5 (BUG-021): com 1 jogador, nao nasce nada no cone de visao (+-60 graus) a frente dele: bichos nao "aparecem do nada" na sua cara
static bool spawnInFront(double px, double pz, double x, double z, int nPl) {
    if (nPl != 1) return false; const double dx = x - px, dz = z - pz, d = sqrt(dx * dx + dz * dz); if (d < 1.0) return false;
    const double ry = gBbYaw * .01745329252; return (dx * -sin(ry) + dz * -cos(ry)) / d > .5;
}
static void animalSpawn(Phys& P) {
    if (gDim != 0 || gDead || (netOn && !netHost)) return;   // so overworld; convidado nao gera (os mobs ainda nao sincronizam, senao ficaria duplicado)
    static int t = 0;
    int n = 0;
    for (MobEnt& e : MOBS) {
        if (e.dead || !(e.type == M_PIG || e.type == M_CHICKEN || e.type == M_COW || e.type == M_MOOSH || e.type == M_SHEEP || isRabbit(e.type) || e.type == M_OCELOT || e.type == M_WOLF)) continue;
        const double d2 = playerNear2(e.x, e.z, P);
        if (!e.rideBoat && d2 > 128.0 * 128.0) { e.y = -100; continue; }   // despawn: animal a 128 blocos ou mais de TODOS os jogadores
        if (d2 < 80.0 * 80.0) n++;
    }
    if (++t < (n < 4 ? 20 : 400)) return; t = 0;   // patchFix.5 (BUG-021): 20 s entre tentativas quando ja ha animais (antes 5 s)   // poucos animais por perto (mundo recem-gerado): tenta a cada 1 s em vez de 5 s => spawn inicial rapido
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    
    if (gPerf && n >= 6 * (int)pls.size()) return;   // patchRpiOpt.6: menos animais no Pi
    if (n >= 8 * (int)pls.size()) return;   // patchFix.5: cap 8 por jogador (antes 12)
    for (int tries = 0; tries < 8; tries++) {
        const auto& ctr = pls[rand() % pls.size()];   // tenta ao redor de um jogador sorteado (convidados tambem)
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 24.0;
        const int x = (int)floor(ctr[0] + cos(ang) * r), z = (int)floor(ctr[2] + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        if (spawnInFront(ctr[0], ctr[2], x + .5, z + .5, (int)pls.size())) continue;
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        int y = -1; for (int yy = CH - 3; yy > 2; yy--) if (getB(x, yy - 1, z) == GRASS && !solid(getB(x, yy, z)) && !solid(getB(x, yy + 1, z)) && !isLiquid(getB(x, yy, z))) { y = yy; break; }
        if (y < 0) continue;
        const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4;
        if (sk * std::max(0.f, (gSky.sunB - .2f) / .8f) < 9.f && sk < 15) continue;   // precisa de ceu aberto/claro (grama coberta ou noite escura nao gera)
        static const int KIND[5] = {M_COW, M_SHEEP, M_PIG, M_CHICKEN, M_RABBIT};   // patchRabbit.1: coelho entra no sorteio (grupos de 2-3)
        int type = KIND[rand() % 5]; if (biomeAt(x + .5f, z + .5f) == B_JUNGLE && rand() % 3 == 0) type = M_OCELOT;   // patchOcelot.1: jaguatirica so na selva
        { const int bw = biomeAt(x + .5f, z + .5f); if ((bw == B_TAIGA || bw == B_FOREST) && rand() % 5 == 0) type = M_WOLF; }   // patchWolf.1: lobos na floresta/taiga (matilha)
        if (type == M_RABBIT) { const int bq = biomeAt(x + .5f, z + .5f); if (bq == B_DESERT) type = M_RGOLD; else if (bq == B_TUNDRA) type = M_RWHITE; else if (rand() % 12 == 0) type = rand() % 2 ? M_RBLACK : M_RSALT; }   // variante por bioma (deserto dourado, tundra branco); raro preto/sal e pimenta
        const int cnt = type == M_WOLF ? 2 + rand() % 3 : type == M_OCELOT ? 1 + rand() % 2 : isRabbit(type) ? 2 + rand() % 2 : 2 + rand() % 3;
        for (int i = 0; i < cnt; i++) {
            const int px = x + rand() % 5 - 2, pz = z + rand() % 5 - 2; int py = -1;
            for (int yy = std::min(CH - 3, y + 3); yy >= std::max(3, y - 3); yy--) if (getB(px, yy - 1, pz) == GRASS && !solid(getB(px, yy, pz)) && !solid(getB(px, yy + 1, pz)) && !isLiquid(getB(px, yy, pz))) { py = yy; break; }
            if (py >= 0 && spawnCellFree(px, py, pz) && getC(fdiv(px, CW), fdiv(pz, CW))) spawnMob(type, px + .5, py, pz + .5, frand() * 360.f, rand() % 10 == 0);   /* patchBaby.1: 10% filhote */
        }
        return;
    }
}

// patchSlime.2: slime chunk (EntitySlime/ChunkGenerator 1.12.2: Random(seed + cx*cx*0x4c1906 + cx*0x5ac0db + cz*cz*0x4307a7 + cz*0x5f24f ^ 0x3ad8025f).nextInt(10) == 0)
static bool isSlimeChunk(int cx, int cz) {
    const int32_t a = (int32_t)((uint32_t)cx * (uint32_t)cx * 0x4c1906u), b = (int32_t)((uint32_t)cx * 0x5ac0dbu), d = (int32_t)((uint32_t)cz * 0x5f24fu);
    const int64_t c = (int64_t)(int32_t)((uint32_t)cz * (uint32_t)cz) * 0x4307a7LL;
    JRng r(((int64_t)WORLD_SEED + a + b + c + d) ^ 0x3ad8025fLL); return r.nextInt(10) == 0;
}
static void slimeSpawn(Phys& P) {   // pantano (y 51-69) e slime chunks (y < 40), no escuro; ate 6 por jogador; 1 tentativa a cada ~5 s
    static int t = 0; if (++t < 100) return; t = 0;
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    int n = 0; for (const MobEnt& e : MOBS) if (e.type == M_SLIME && !e.dead) n++;
    if (n >= 6 * (int)pls.size() || (gDead && pls.size() == 1)) return;
    for (int tries = 0; tries < 10; tries++) {
        const auto& ctr = pls[rand() % pls.size()];
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 16.0;
        const int x = (int)floor(ctr[0] + cos(ang) * r), z = (int)floor(ctr[2] + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        const bool swamp = biomeAt((float)x + .5f, (float)z + .5f) == B_SWAMP, sc = isSlimeChunk(cx, cz);
        if (!swamp && !sc) continue;
        int y = -1; for (int yy = std::min(CH - 4, (int)ctr[1] + rand() % 17 - 8), k = 0; yy > 2 && k < 40; yy--, k++) if (solid(getB(x, yy - 1, z)) && !isLiquid(getB(x, yy - 1, z)) && !solid(getB(x, yy, z)) && !solid(getB(x, yy + 1, z)) && !isLiquid(getB(x, yy, z))) { y = yy; break; }
        if (y < 0 || !((sc && y < 40) || (swamp && y > 50 && y < 70))) continue;
        const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4, bl = v & 15;
        const float effSky = sk * std::max(0.f, (gSky.sunB - .2f) / .8f);
        if (std::max((float)bl, effSky) > 7.f) continue;
        if (!sc && rand() % 2) continue;   // pantano: so metade das tentativas (no MC depende da fase da lua)
        spawnMob(M_SLIME, x + .5, y, z + .5, frand() * 360.f); return;
    }
}
static void wildSpawn(Phys& P) {   // patchMobNew.2: morcego (cavernas escuras, y < 63) e urso polar (tundra); ~5 s entre tentativas; despawn a 128+ blocos
    if (gDim != 0 || gDead || (netOn && !netHost)) return;
    static int t = 0; if (++t < 100) return; t = 0;
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    int nb = 0, np = 0, nh = 0;
    for (MobEnt& e : MOBS) { if (e.dead || (e.type != M_BAT && e.type != M_POLAR && !isHorse(e.type) && !isLlama(e.type))) continue; if (playerNear2(e.x, e.z, P) > 128.0 * 128.0) { e.y = -100; continue; } (e.type == M_BAT ? nb : e.type == M_POLAR ? np : nh)++; }   // patchMobNew.2: cavalos/lhamas entram na contagem e no despawn longe
    for (int tries = 0; tries < 8; tries++) {
        const auto& ctr = pls[rand() % pls.size()];
        const double ang = frand() * 6.2831853, r = 14.0 + frand() * 18.0;
        const int x = (int)floor(ctr[0] + cos(ang) * r), z = (int)floor(ctr[2] + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        if (nb < 5 * (int)pls.size() && ctr[1] < 62.0 && rand() % 2) {   // morcego: ar livre de 2 celulas, sem ceu, luz de bloco <= 4
            const int y = std::max(4, std::min(61, (int)ctr[1] + rand() % 13 - 6));
            if (!spawnCellFree(x, y, z) || !spawnCellFree(x, y + 1, z)) continue;
            const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4, bl = v & 15;
            if (sk > 2 || bl > 4) continue;
            const int k = std::min(1 + rand() % 2, 5 * (int)pls.size() - nb);   // patchFix.Bat: respeita o teto (antes passava de 5)
            for (int i = 0; i < k; i++) spawnMob(M_BAT, x + .5 + i * .6, y, z + .5, frand() * 360.f); return;
        }
        if (np < 3 * (int)pls.size() && biomeAt(x + .5f, z + .5f) == B_TUNDRA && rand() % 6 == 0) {   // urso polar: superficie da tundra
            int y = -1; for (int yy = CH - 4; yy > 40; yy--) if (spawnFloorFull(x, yy - 1, z) && spawnCellFree(x, yy, z) && spawnCellFree(x, yy + 1, z)) { y = yy; break; }
            if (y < 0) continue;
            const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)]; if ((v >> 4) < 12) continue;   // so a ceu aberto
            const int k = 1 + rand() % 2; for (int i = 0; i < k; i++) spawnMob(M_POLAR, x + .5 + i * 1.2, y, z + .5, frand() * 360.f, i > 0 && rand() % 3 == 0); return;   // filhote (1/3) acompanha
        }
        {   const int bq = biomeAt(x + .5f, z + .5f);   // patchMobNew.2: cavalo/burro em planicie e savana, lhama em montanhas (grupos de 2-3, ate 4 por jogador)
            const bool hz = bq == B_PLAINS || bq == B_SAVANNA, lz = bq == B_MOUNTAINS;
            if ((hz || lz) && nh < 4 * (int)pls.size() && rand() % 8 == 0) {
                int y = -1; for (int yy = CH - 4; yy > 40; yy--) if (spawnFloorFull(x, yy - 1, z) && spawnCellFree(x, yy, z) && spawnCellFree(x, yy + 1, z)) { y = yy; break; }
                if (y < 0) continue;
                const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)]; if ((v >> 4) < 12) continue;
                const int base = lz ? M_LLAMA + rand() % 4 : rand() % 6 == 0 ? M_DONKEY : M_HORSE + rand() % 4, k = 2 + rand() % 2;
                for (int i = 0; i < k; i++) spawnMob(base, x + .5 + i * 1.5, y, z + .5, frand() * 360.f); return;
            }
        }
    }
}
static void hostileSpawn(Phys& P) {   // a cada 2 s, ate 14 hostis; escuro (luz <= 7), 24-40 blocos do jogador
    if (gDim != 0 && gDim != 3) return;   // patchBlood.1: tambem na Rubra (sem luz do ceu: monstros em qualquer lugar sem tocha)   // so no overworld (porco-zumbi do Nether: nether.inc)
    if (gDifficulty == 0) { for (MobEnt& e : MOBS) if (!e.dead && e.y > -90 && MD[e.type].ai >= 4) e.y = -100;   /* patchDif.1: Pacifico remove hostis (mesmo idioma do despawn) e nao gera novos */ wildSpawn(P); return; }
    if (gDim == 0) slimeSpawn(P);   // patchSlime.2
    wildSpawn(P);   // patchMobNew.2
    static int t = 0; if (++t < 40) return; t = 0;
    int n = 0; for (const MobEnt& e : MOBS) if (MD[e.type].ai >= 4 && !e.dead) n++;
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    
    if (gPerf && n >= 5 * (int)pls.size()) return;   // patchRpiOpt.6: menos hostis no Pi
    if (n >= 14 * (int)pls.size() || (gDead && pls.size() == 1)) return;
    for (int tries = 0; tries < 6; tries++) {
        const auto& ctr = pls[rand() % pls.size()];   // ao redor de um jogador sorteado
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 16.0;
        const int x = (int)floor(ctr[0] + cos(ang) * r), z = (int)floor(ctr[2] + sin(ang) * r), cx = fdiv(x, CW), cz = fdiv(z, CW);
        if (spawnInFront(ctr[0], ctr[2], x + .5, z + .5, (int)pls.size())) continue;
        Chunk* c = getC(cx, cz); if (!c || !c->lc) continue;
        int y = -1; for (int yy = std::min(CH - 4, (int)ctr[1] + rand() % 17 - 8), k = 0; yy > 2 && k < 24; yy--, k++) if (spawnFloorFull(x, yy - 1, z) && spawnCellFree(x, yy, z) && spawnCellFree(x, yy + 1, z)) { y = yy; break; }
        if (y < 0) continue;
        const int v = c->lc[idx(x - cx * CW, y, z - cz * CW)], sk = v >> 4, bl = v & 15;
        const float effSky = gDim ? 0.f : sk * std::max(0.f, (gSky.sunB - .2f) / .8f);
        if (std::max((float)bl, effSky) > 7.f) continue;
        int type;
        { const int q = rand() % 10; type = q < 3 ? M_ZOMBIE : q < 7 ? M_SKELETON : M_CREEPER; if (rand() % 24 == 0) type = M_ENDERMAN;
          if (type == M_ZOMBIE && rand() % 20 == 0) type = M_ZVILL;   // patchMob112.1: 5% dos zumbis = zumbi-aldeao
          if (type == M_SKELETON && biomeAt(x + .5f, z + .5f) == B_TUNDRA) type = M_STRAY; }   // vagante: esqueleto em bioma gelado   // patchEnd.12: enderman raro no overworld
        spawnMob(type, x + .5, y, z + .5, frand() * 360.f); return;
    }
}

// patchEnd.12: spawn natural no End (EnderMan, grupos de 1 a 4 sobre blocos solidos, ate 6 por jogador); so o host/solo
static void endSpawn(Phys& P) {
    if (gDim != 2 || gDead || (netOn && !netHost)) return;
    static int t = 0; if (++t < 60) return; t = 0;
    int n = 0; for (const MobEnt& e : MOBS) if (e.type == M_ENDERMAN && !e.dead) n++;
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    if (n >= 6 * (int)pls.size()) return;
    for (int tries = 0; tries < 8; tries++) {
        const auto& ctr = pls[rand() % pls.size()];
        const double ang = frand() * 6.2831853, r = 24.0 + frand() * 24.0;
        const int x = (int)floor(ctr[0] + cos(ang) * r), z = (int)floor(ctr[2] + sin(ang) * r);
        if (!getC(fdiv(x, CW), fdiv(z, CW))) continue;
        int y = -1; for (int yy = std::min(CH - 6, (int)ctr[1] + 24); yy > 3 && yy > (int)ctr[1] - 40; yy--) if (solid(getB(x, yy - 1, z)) && !isLiquid(getB(x, yy - 1, z)) && !solid(getB(x, yy, z)) && !solid(getB(x, yy + 1, z)) && !solid(getB(x, yy + 2, z))) { y = yy; break; }
        if (y < 0) continue;
        const int cnt = 1 + rand() % 4;
        for (int i = 0; i < cnt; i++) {
            const int px = x + rand() % 5 - 2, pz = z + rand() % 5 - 2; if (!getC(fdiv(px, CW), fdiv(pz, CW))) continue;
            if (solid(getB(px, y - 1, pz)) && !solid(getB(px, y, pz)) && !solid(getB(px, y + 1, pz)) && !solid(getB(px, y + 2, pz))) spawnMob(M_ENDERMAN, px + .5, y, pz + .5, frand() * 360.f);
        }
        return;
    }
}
