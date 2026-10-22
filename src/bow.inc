// ================= arco e flecha (como no Minecraft 1.8.9) =================
// Segurar o botão direito com o arco carrega (ticks de 20 Hz): f = (t/20)^2 e t/20 combinados -> f=(f*f+f*2)/3, máx 1; abaixo de 0,1 não atira.
// Flecha: velocidade f*3 (blocos/tick), gravidade 0,05, atrito 0,99 (0,6 na água), dano = ceil(velocidade*2) (+ extra aleatório se f==1, "crítica").
// Gasta 1 flecha (criativo não gasta); flecha de sobrevivência fica cravada no bloco e pode ser recolhida; some após 1200 ticks.
// Multiplayer: quem atira avisa NP_ARROW (os outros só veem a flecha, sem dano) e, ao acertar um jogador, manda NP_PVP (dano) a ele.
struct ArrowEnt { double x, y, z, px, py, pz, vx, vy, vz; double dx = 0, dy = 0, dz = 1; int owner = -1, age = 0, shake = 0, bx = 0, by = 0, bz = 0; bool crit = false, pick = false, stuck = false, cre = false; int shooter = 0; };   // owner -1 = eu; cre = atirada em criativo (so criativo recolhe, sem item)
static std::vector<ArrowEnt> ARROWS; static int gSplitHitK = 0;   // extra do split atingido pela flecha
static double arrowGauss() { double s = 0; for (int i = 0; i < 12; i++) s += frand(); return s - 6.0; }
static void arrowSpawn(double x, double y, double z, double vx, double vy, double vz, bool crit, int owner, bool pick, int shooter = 0) {   // shooter: uid do mob que atirou (0 = jogador)
    ArrowEnt a; a.x = a.px = x; a.y = a.py = y; a.z = a.pz = z; a.vx = vx; a.vy = vy; a.vz = vz; { const double l = sqrt(vx * vx + vy * vy + vz * vz); if (l > 1e-9) { a.dx = vx / l; a.dy = vy / l; a.dz = vz / l; } } a.crit = crit; a.owner = owner; a.pick = pick; a.shooter = shooter;
    if (ARROWS.size() > 200) ARROWS.erase(ARROWS.begin());
    ARROWS.push_back(a);
}
static void arrowNetRecv(NBuf& b, int sender) {
    const int type = b.get<uint8_t>();   // 0 = nova flecha, 1 = a flecha acertou alguem: some nos outros clientes
    const double x = b.get<float>(), y = b.get<float>(), z = b.get<float>(), vx = b.get<float>(), vy = b.get<float>(), vz = b.get<float>(); const int fl = b.get<uint8_t>(); const bool cr = (fl & 1) != 0;   // fl: bit0 critica, bit1 recolhivel, bit2 so criativo
    if (type == 3) {   // a flecha do dono cravou: a copia aqui cola no mesmo ponto/direcao (sem divergir da simulacao local)
        int best = -1; double bd = 100.0;
        for (int i = 0; i < (int)ARROWS.size(); i++) { const ArrowEnt& a = ARROWS[i]; if (a.owner != sender) continue; const double d2 = (a.x - x) * (a.x - x) + (a.y - y) * (a.y - y) + (a.z - z) * (a.z - z); if (d2 < bd) { bd = d2; best = i; } }
        if (best >= 0) { ArrowEnt& a = ARROWS[best]; const double l = sqrt(vx * vx + vy * vy + vz * vz); if (l > 1e-9) { a.dx = vx / l; a.dy = vy / l; a.dz = vz / l; }
            a.x = a.px = x; a.y = a.py = y; a.z = a.pz = z; a.bx = (int)floor(x + a.dx * .07); a.by = (int)floor(y + a.dy * .07); a.bz = (int)floor(z + a.dz * .07);
            a.vx = a.vy = a.vz = 0; a.stuck = true; if (a.shake <= 0) a.shake = 7; a.age = 0; a.crit = false; }
        return;
    }
    if (type == 2) {   // alguem recolheu uma flecha cravada: some a mais proxima aqui (qualquer dono)
        int best = -1; double bd = 4.0;
        for (int i = 0; i < (int)ARROWS.size(); i++) { const ArrowEnt& a = ARROWS[i]; if (!a.stuck) continue; const double d2 = (a.x - x) * (a.x - x) + (a.y - y) * (a.y - y) + (a.z - z) * (a.z - z); if (d2 < bd) { bd = d2; best = i; } }
        if (best >= 0) ARROWS.erase(ARROWS.begin() + best);
        return;
    }
    if (type == 1) {
        int best = -1; double bd = 9.0;
        for (int i = 0; i < (int)ARROWS.size(); i++) { const ArrowEnt& a = ARROWS[i]; if (a.owner != sender || a.stuck) continue; const double d2 = (a.x - x) * (a.x - x) + (a.y - y) * (a.y - y) + (a.z - z) * (a.z - z); if (d2 < bd) { bd = d2; best = i; } }
        if (best >= 0) ARROWS.erase(ARROWS.begin() + best);
        return;
    }
    arrowSpawn(x, y, z, vx, vy, vz, cr, sender, (fl & 2) != 0); ARROWS.back().cre = (fl & 4) != 0;
}
static void arrowNetStuck(const ArrowEnt& a) { if (!netOn || a.owner != -1 || a.shooter) return; NBuf b = netMk(NP_ARROW); b.put<uint8_t>(3); b.put<float>((float)a.x); b.put<float>((float)a.y); b.put<float>((float)a.z); b.put<float>((float)a.dx); b.put<float>((float)a.dy); b.put<float>((float)a.dz); b.put<uint8_t>(0); netBroadcast(b); }
static void arrowNetTake(const ArrowEnt& a) { if (!netOn) return; NBuf b = netMk(NP_ARROW); b.put<uint8_t>(2); b.put<float>((float)a.x); b.put<float>((float)a.y); b.put<float>((float)a.z); b.put<float>(0.f); b.put<float>(0.f); b.put<float>(0.f); b.put<uint8_t>(0); netBroadcast(b); }
static void arrowNetKill(const ArrowEnt& a) { if (!netOn) return; NBuf b = netMk(NP_ARROW); b.put<uint8_t>(1); b.put<float>((float)a.x); b.put<float>((float)a.y); b.put<float>((float)a.z); b.put<float>(0.f); b.put<float>(0.f); b.put<float>(0.f); b.put<uint8_t>(0); netBroadcast(b); }
static bool bowHasArrow() { if (creative) return true; for (const Slot& s : inv) if (s.id == IT_ARROW && s.n > 0) return true; return false; }
static void bowConsume() { if (creative) return; for (Slot& s : inv) if (s.id == IT_ARROW && s.n > 0) { if (--s.n <= 0) s = Slot(); return; } }
static Slot* gBowSlot = nullptr;   // patchDur.1: slot do arco na mao (setado antes de bowTick)
static void bowShoot(const Phys& P, float yaw, float pitch, float f) {
    const float cy = cosf(yaw * .0174533f), sy = sinf(yaw * .0174533f), cp = cosf(pitch * .0174533f), sp = sinf(pitch * .0174533f);
    double d[3] = {-sy * cp, sp, -cy * cp};
    const double hl = sqrt(d[0] * d[0] + d[2] * d[2]), rx = hl > 1e-6 ? -d[2] / hl : 1.0, rz = hl > 1e-6 ? d[0] / hl : 0.0;   // direita do olhar
    const double x = P.x + rx * .16, y = P.y + (P.sneak ? 1.54 : 1.62) - .1, z = P.z + rz * .16;   // EntityArrow: sai ao lado da mão, 0,1 abaixo dos olhos
    for (int a = 0; a < 3; a++) d[a] += arrowGauss() * .0075;   // setThrowableHeading: imprecisão 1.0
    const double l = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), v = f * 3.0;
    const double vx = d[0] / l * v, vy = d[1] / l * v, vz = d[2] / l * v;
    const bool crit = f >= 1.f;
    arrowSpawn(x, y, z, vx, vy, vz, crit, -1, true); ARROWS.back().cre = creative;
    if (netOn) { NBuf b = netMk(NP_ARROW); b.put<uint8_t>(0); b.put<float>((float)x); b.put<float>((float)y); b.put<float>((float)z); b.put<float>((float)vx); b.put<float>((float)vy); b.put<float>((float)vz); b.put<uint8_t>((crit ? 1 : 0) | 2 | (creative ? 4 : 0)); netBroadcast(b); }
    sndEvtAt("bow.shoot", P.x, P.y + 1.4, P.z, .5f + f * .5f);   // patchAud.6: disparo (volume cresce com a forca)
    bowConsume();
    if (gBowSlot && gBowSlot->id == IT_BOW) itemWear(*gBowSlot, 1);
}
static bool touchUse = false, gTouchUseItem = false;   // toque: dedo segurando para usar item (comer / puxar arco); gTouchUseItem = item da mão é usável (touch.inc / main_loop.inc)
static void bowTick(Phys& P, bool can, int heldId, float yaw, float pitch) {   // 1x por tick, depois do movimento
    const bool rmb = gUseOverride >= 0 ? gUseOverride != 0 : ((gpMouseState() & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || (touchUse && gTouchUseItem));
    const bool ok = can && !gDead && gGm != 3 && heldId == IT_BOW;
    if (ok && rmb && bowHasArrow()) { gBowT = gBowT < 0 ? 0 : gBowT + 1; gBowOn = true; P.sprint = false; return; }
    if (ok && !rmb && gBowT >= 0) {   // soltou: atira
        float f = gBowT / 20.f; f = (f * f + f * 2.f) / 3.f; if (f > 1.f) f = 1.f;
        if (f >= .1f && bowHasArrow()) bowShoot(P, yaw, pitch, f);
    }
    gBowT = -1; gBowOn = false;
}
static void eatTick(Phys& P, bool can, Slot& s) {   // 1x por tick: segurar o botao direito com comida = comer em 32 ticks (1,6 s); soltar cancela
    const bool rmb = gUseOverride >= 0 ? gUseOverride != 0 : ((gpMouseState() & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || (touchUse && gTouchUseItem));
    if (can && rmb && !gBowOn && gGm != 3 && survCanEat(s.id)) {
        gEatT = gEatT < 0 ? 0 : gEatT + 1; gEatOn = true; gEatMs = SDL_GetTicks(); P.sprint = false;
        if (gEatT >= 7 && gEatT % 4 == 0 && gEatT < 32) sndEvtAt("eat", P.x, P.y + 1.4, P.z);   // patchAud.2: mordidas
        if (gEatT >= 32) { sndEvtAt("burp", P.x, P.y + 1.4, P.z); survEatNow(s); gEatT = -1; gEatOn = false; }
        return;
    }
    gEatT = -1; gEatOn = false;
}
static int bowPullItem(int id) { return bowPullItemT(id, gBowT); }   // textura de puxar na mao em 1a pessoa (helper em render_items.inc)
static double arrowSegBox(const double* o, const double* d, double len, const double* lo, const double* hi) {   // raio x caixa; devolve t em [0,len] ou -1
    double t0 = 0, t1 = len;
    for (int a = 0; a < 3; a++) {
        if (fabs(d[a]) < 1e-9) { if (o[a] < lo[a] || o[a] > hi[a]) return -1; continue; }
        double ta = (lo[a] - o[a]) / d[a], tb = (hi[a] - o[a]) / d[a]; if (ta > tb) std::swap(ta, tb);
        t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) return -1;
    }
    return t0;
}
static bool arrowTryPick(const ArrowEnt& a, const Phys& P) {   // recolha de flecha cravada por quem esta no contexto atual (P1, ou P2 dentro do S2Ctx)
    if (!(a.pick && a.stuck && a.shake <= 0 && !gDead && gGm != 3 && fabs(P.x - a.x) < 1.0 && fabs(P.z - a.z) < 1.0 && a.y > P.y - .5 && a.y < P.y + 2.3)) return false;
    if (a.cre) { if (!creative) return false; }   // PickupStatus.CREATIVE_ONLY: so criativo, nao entrega item (anima a coleta)
    else if (addItem(IT_ARROW, 1) > 0) return false;   // ALLOWED: recolhe se couber; a flecha voa ate o jogador (3 ticks, igual aos itens)
    PICKS.push_back({a.x, a.y, a.z, IT_ARROW, 0, -1}); arrowNetTake(a); return true;
}
static void arrowsPickTick(const Phys& P) { for (size_t i = 0; i < ARROWS.size(); ++i) if (arrowTryPick(ARROWS[i], P)) { ARROWS[i] = ARROWS.back(); ARROWS.pop_back(); --i; } }   // jogador 2 (o P1 recolhe dentro do arrowStep)
static bool arrowHitEnd(const double* o, const double* d, double bt, int dmg);   // dragon.inc (patchEnd.7)
static bool arrowStep(ArrowEnt& a, Phys& P) {   // EntityArrow.onUpdate; false = remover
    a.px = a.x; a.py = a.y; a.pz = a.z; ++a.age;
    if (a.stuck) {
        if (a.shake > 0) --a.shake;
        if (solid(getB(a.bx, a.by, a.bz))) {
            if (a.age >= 1200) return false;
            if (arrowTryPick(a, P)) return false;
            return true;
        }
        a.stuck = false; a.age = 0; a.vx *= frand() * .2; a.vy *= frand() * .2; a.vz *= frand() * .2;   // o bloco sumiu: solta
    }
    const double sp = sqrt(a.vx * a.vx + a.vy * a.vy + a.vz * a.vz);
    if (sp > 1e-6) {
        const double o[3] = {a.x, a.y, a.z}, d[3] = {a.vx / sp, a.vy / sp, a.vz / sp};
        a.dx = d[0]; a.dy = d[1]; a.dz = d[2];
        const float of[3] = {(float)o[0], (float)o[1], (float)o[2]}, df[3] = {(float)d[0], (float)d[1], (float)d[2]};
        const float tbf = raySolidDistance(of, df, (float)sp); const bool hitB = tbf >= 0.f; const double tb = hitB ? tbf : sp + 1;
        double bt = hitB ? tb : sp; int kind = 0, idx = -1;   // kind 1 = mob, 2 = jogador remoto
        if (a.owner == -1 && a.age > 1) {   // so a flecha que eu atirei causa dano (mobs sao locais; jogadores recebem NP_PVP)
            for (int i = 0; i < (int)MOBS.size(); i++) {
                const MobEnt& e = MOBS[i]; if (e.dead) continue;
                const double hw = mobW(e) * .5 + .3, lo[3] = {e.x - hw, e.y - .3, e.z - hw}, hi[3] = {e.x + hw, e.y + mobH(e) + .3, e.z + hw};
                const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; kind = 1; idx = i; }
            }
            for (auto& kv : remotes) {
                const RemotePl& r = kv.second; if (!r.init || r.dim != gDim || r.deathT >= 0 || r.gm == 3 || kv.first == netSelf) continue;
                const double hw = .6, h = r.sneak ? 1.65 : 1.8, lo[3] = {r.x - hw, r.y - .3, r.z - hw}, hi[3] = {r.x + hw, r.y + h + .3, r.z + hw};
                const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; kind = 2; idx = kv.first; }
            }
        }
        if (a.owner == -2 && a.age > 1) {   // flecha de esqueleto tambem fere os outros mobs (menos quem atirou)
            for (int i = 0; i < (int)MOBS.size(); i++) {
                const MobEnt& e = MOBS[i]; if (e.dead || (a.shooter && e.uid == a.shooter)) continue;
                const double hw = mobW(e) * .5 + .3, lo[3] = {e.x - hw, e.y - .3, e.z - hw}, hi[3] = {e.x + hw, e.y + mobH(e) + .3, e.z + hw};
                const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; kind = 4; idx = i; }
            }
        }
        if (a.owner == -2 && a.age > 1 && !gDead && !creative) {   // flecha de esqueleto: so acerta o jogador local
            const double lo[3] = {P.x - .45, P.y - .3, P.z - .45}, hi[3] = {P.x + .45, P.y + 2.1, P.z + .45};
            const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; kind = 3; }
        }
        for (int xk = 0; xk < split2N(); xk++) if (a.owner == -2 && a.age > 1 && split2TargetableK(xk)) {   // flecha de esqueleto tambem acerta os jogadores do split (patchSplit.1: ate 3 extras)
            const Phys& q = *split2PhysK(xk); gSplitHitK = xk; const double lo[3] = {q.x - .45, q.y - .3, q.z - .45}, hi[3] = {q.x + .45, q.y + 2.1, q.z + .45};
            const double t = arrowSegBox(o, d, bt, lo, hi); if (t >= 0 && t <= bt) { bt = t; kind = 5; }
        }
        if (a.owner == -1 && a.age > 1 && gDim == 2 && !kind) { int ed = (int)ceil(sp * 2.0); if (a.crit) ed += (int)(frand() * (ed / 2 + 2)); if (arrowHitEnd(o, d, bt, ed)) { arrowNetKill(a); return false; } }   // patchEnd.7: flecha acerta cristal/dragao
        if (kind) {
            int dmg = (int)ceil(sp * 2.0); if (a.crit) dmg += (int)(frand() * (dmg / 2 + 2));
            if (kind == 5) { split2DamageK(gSplitHitK, dmg, "baleado por um esqueleto", false, (float)(d[0] * .1), (float)(d[2] * .1)); return false; }
            if (kind == 3) { survDamage(dmg, "baleado por um esqueleto"); P.mx += d[0] * .1; P.mz += d[2] * .1; return false; }
            if (kind == 4) {   // esqueleto -> outro mob: o ferido revida no atirador (gMobAtkUid)
                gMobAtkUid = a.shooter; mobHurt(MOBS[idx], dmg, (float)d[0], (float)d[2], true, 0.f, (float)d[0], (float)d[2]); gMobAtkUid = 0; return false;
            }
            if (kind == 1) {
                const MobEnt& m = MOBS[idx];
                if (mobHurt(MOBS[idx], dmg, (float)(m.x - P.x), (float)(m.z - P.z), true, 0.f, (float)d[0], (float)d[2])) { sndPlay("random/successful_hit", .6f); arrowNetKill(a); return false; }
                a.vx *= -.1; a.vy *= -.1; a.vz *= -.1; a.age = 0;   // dano ignorado (invulnerável): ricocheteia
            } else {
                if (netOn) { NBuf b = netMk(NP_PVP); b.put<uint8_t>((uint8_t)idx); b.put<int16_t>((int16_t)dmg); b.put<float>((float)d[0]); b.put<float>((float)d[2]); b.put<float>(-1.f); b.put<float>((float)o[0]); b.put<float>((float)o[2]); netBroadcast(b); }
                arrowNetKill(a); return false;
            }
        } else if (hitB) {
            a.x += d[0] * (tb - .05); a.y += d[1] * (tb - .05); a.z += d[2] * (tb - .05);   // crava ~0,05 antes do ponto de impacto
            a.bx = (int)floor(o[0] + d[0] * (tb + .02)); a.by = (int)floor(o[1] + d[1] * (tb + .02)); a.bz = (int)floor(o[2] + d[2] * (tb + .02));
            a.vx = a.vy = a.vz = 0; a.stuck = true; a.shake = 7; a.age = 0; a.crit = false; a.px = a.x; a.py = a.y; a.pz = a.z; arrowNetStuck(a); sndEvtAt("bow.hit", a.x, a.y, a.z);
            return true;
        } else { a.x += a.vx; a.y += a.vy; a.z += a.vz; }
    }
    const double f3 = isWater(getB((int)floor(a.x), (int)floor(a.y), (int)floor(a.z))) ? .6 : .99;
    a.vx *= f3; a.vy *= f3; a.vz *= f3; a.vy -= .05;
    return !(a.y < -64.0 || a.age > 1200);
}
static void arrowsTick(Phys& P) { for (size_t i = 0; i < ARROWS.size();) { if (arrowStep(ARROWS[i], P)) ++i; else { ARROWS[i] = ARROWS.back(); ARROWS.pop_back(); } } }
static void arrowsDraw(float pt) {   // sprite da flecha em cruz (2 planos), apontando na direcao do movimento
    if (ARROWS.empty()) return;
    glColor4f(1.f, 1.f, 1.f, 1.f);
    for (const ArrowEnt& a : ARROWS) {
        const double vx = a.dx, vy = a.dy, vz = a.dz, h = sqrt(vx * vx + vz * vz);
        const float th = (float)(atan2(-vz, vx) * 57.29578), ph = (float)(atan2(vy, h) * 57.29578);
        const double ix = a.px + (a.x - a.px) * pt, iy = a.py + (a.y - a.py) * pt, iz = a.pz + (a.z - a.pz) * pt;
        entLightAt(ix, iy, iz, gEntDim);   // luz do local (dia/noite, caverna, tocha): a flecha escurece igual aos outros itens
        for (int k = 0; k < 2; k++) {
            glPushMatrix();
            glTranslatef((float)ix, (float)iy, (float)iz); glRotatef(th, 0, 1, 0); glRotatef(ph, 0, 0, 1);
            glRotatef(k * 90.f, 1, 0, 0); glRotatef(-45.f, 0, 0, 1); glScalef(.65f, .65f, .65f); glTranslatef(-.5f, -.5f, -.5f);
            drawHeldRaw(IT_ARROW);
            glPopMatrix();
        }
    }
    entDimReset();
}
// ---- espectador: clique esquerdo teleporta para o próximo jogador (equivale ao menu de teletransporte do Minecraft) ----
static void spectTeleport(Phys& P) {
    static int last = -1; int first = -1, next = -1;
    for (auto& kv : remotes) {
        const RemotePl& r = kv.second; if (!r.init || r.dim != gDim || r.deathT >= 0 || kv.first == netSelf) continue;
        if (first < 0) first = kv.first; if (next < 0 && kv.first > last) next = kv.first;
    }
    if (next < 0) next = first;
    if (next < 0) { chatAdd("Nenhum jogador para teleportar", 0xFF5555); return; }
    last = next; const RemotePl& r = remotes[next];
    P.x = P.px = r.x; P.y = P.py = r.y; P.z = P.pz = r.z; P.mx = P.my = P.mz = 0;
    chatAdd("Teleportado para " + netLabel(next), 0xFFFFFF);
}
