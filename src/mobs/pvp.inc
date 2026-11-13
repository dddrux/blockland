static std::string splitNick(int k);   // splitscreen.inc
// ================= PvP + morte de jogador (multiplayer) =================
// Autoridade: quem ataca detecta o golpe (alcance 3, 6 no criativo) e manda NP_PVP (alvo, dano, direcao, sprint); o alvo aplica dano,
// invulnerabilidade (10 ticks) e knockback como no 1.8.9. Ao morrer o jogador solta o inventario (como no vanilla), avisa todos com
// NP_DEATH (mensagem no chat + animacao de queda de 20 ticks no corpo dele) e, ao renascer, NP_DEATH com flag 0.
/* gPvpP agora e declarado em egg.inc (patchEgg.1: o ovo precisa dele) */
static std::string gPvpKiller;  // nome de quem deu o ultimo golpe de jogador (para a mensagem de morte)

static bool split2PvpHit(int k, int dmg, const char* why, float kx, float kz);   // splitscreen.inc: dano + recuo no extra k (false = bloqueado pela invulnerabilidade)
static void pvpApply1(const std::string& killer, bool haveA, double ax, double az, int dmg, float kx, float kz, float kb);
// patchSplit.4: golpe de jogador. atk = -1 (jogador 1) ou k (extra do split). Alvos: J1, extras do split (locais) e jogadores/extras de OUTROS clientes (rede).
// NP_PVP: alvo = id | ((k+1) << 4) quando o alvo e o extra k do cliente id (k+1 = 0: o jogador principal)
static bool pvpAttackK(int atk, const float* eye, const float* dir, int held, bool crit, bool sprint, float reach) {
    int best = -1, kind = -1; float bt = reach;
    auto hit = [&](double x, double y, double z, bool sneak) {
        const float hw = .3f + .1f, h = sneak ? 1.65f : 1.8f, lo[3] = {(float)x - hw, (float)y - .1f, (float)z - hw}, hi[3] = {(float)x + hw, (float)y + h + .1f, (float)z + hw};
        float t0 = 0.f, t1 = bt;
        for (int a = 0; a < 3; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) return -1.f; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) return -1.f;
        }
        return t0;
    };
    if (atk >= 0 && gPvpP && !gDead && !creative && gGm != 3 && gSplitCtx == 0) { const float t = hit(gPvpP->x, gPvpP->y, gPvpP->z, gPvpP->sneak); if (t >= 0.f && t < bt) { bt = t; best = 0; kind = 2; } }   // jogador 1 como alvo
    for (int k = 0; k < split2N(); k++) if (k != atk && split2TargetableK(k)) { const Phys* q = split2PhysK(k); const float t = hit(q->x, q->y, q->z, q->sneak); if (t >= 0.f && t < bt) { bt = t; best = k; kind = 3; } }
    if (netOn) for (int pass = 0; pass < 2; pass++) for (auto& kv : (pass ? remotesX : remotes)) {
        const RemotePl& r = kv.second; if (!r.init || r.dim != gDim || r.deathT >= 0 || r.gm == 3) continue;
        const int rid = pass ? ((kv.first - 1000) >> 2) : kv.first; if (rid == netSelf) continue;
        const float t = hit(r.x, r.y, r.z, r.sneak); if (t >= 0.f && t < bt) { bt = t; best = pass ? (rid | ((((kv.first - 1000) & 3) + 1) << 4)) : rid; kind = pass ? 1 : 0; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;   // bloco na frente
    int dmg = mobWeaponDmg(held); if (crit) dmg = (int)(dmg * 1.5f);
    const std::string me = atk >= 0 ? splitNick(atk) : netName();
    if (kind <= 1) {
        NBuf b = netMk(NP_PVP); b.put<uint8_t>((uint8_t)best); b.put<int16_t>((int16_t)dmg); b.put<float>(dir[0]); b.put<float>(dir[2]); b.put<float>(sprint ? 1.f : 0.f); b.put<float>(eye[0]); b.put<float>(eye[2]);   // patchPvpOrigin.1: posicao de QUEM bate (J1 ou extra do split), nao a do jogador principal
        netBroadcast(b); return true;
    }
    gPvpKiller = me;
    if (kind == 2) { pvpApply1(me, true, eye[0], eye[2], dmg, dir[0], dir[2], sprint ? 1.f : 0.f); return true; }
    { const Phys* q = split2PhysK(best); const double ax = eye[0], az = eye[2]; float bx = (float)(q->x - ax), bz = (float)(q->z - az); const float l = sqrtf(bx * bx + bz * bz), ll = sqrtf(dir[0] * dir[0] + dir[2] * dir[2]);
      float kx = 0.f, kz = 0.f; if (l > 1e-4f) { kx = bx / l * .4f; kz = bz / l * .4f; } if (sprint && ll > 1e-4f) { kx += dir[0] / ll * .5f; kz += dir[2] / ll * .5f; }
      split2PvpHit(best, dmg, ("Foi morto por " + me).c_str(), kx, kz); }
    return true;
}
static bool pvpAttack(const float* eye, const float* dir, int held, bool crit, bool sprint, float reach) { return pvpAttackK(-1, eye, dir, held, crit, sprint, reach); }
static void boomReceive(int dmg, bool tnt, float kx, float ky, float kz) {   // explosao do host (TNT/creeper): dano e recuo (NP_BOOM)
    if (!gPvpP || creative || gDead || dmg <= 0) return;
    if (survDamage(dmg, tnt ? "explodido por TNT" : "explodido por um creeper")) { gPvpP->mx += kx; gPvpP->my += ky; gPvpP->mz += kz; }
}
static void pvpApply1(const std::string& killer, bool haveA, double ax, double az, int dmg, float kx, float kz, float kb) {   // jogador do contexto atual (gPvpP) acertado por outro jogador
    if (!gPvpP || creative || gDead || dmg <= 0) return;
    const bool arrow = kb < 0.f; if (arrow) kb = 0.f;   // flecha: kb < 0
    gPvpKiller = killer;
    if (!survDamage(dmg, ((arrow ? "Foi alvejado por " : "Foi morto por ") + gPvpKiller).c_str())) return;   // bloqueado pela invulnerabilidade = sem dano e sem recuo
    Phys& P = *gPvpP; float bx = kx, bz = kz;   // (kx,kz) = olhar do atacante; o recuo base vai da posicao dele para a minha (1.8.9)
    if (haveA) { const float dx = (float)(P.x - ax), dz = (float)(P.z - az); if (dx * dx + dz * dz > 1e-6f) { bx = dx; bz = dz; } }
    gHurtAtYaw = atan2f(-bz, -bx) * 57.29578f - (180.f - gYawNow);   // 1.8.9 attackedAtYaw (atacante - eu) relativo ao meu yaw; usado na inclinacao da camera
    const float l = sqrtf(bx * bx + bz * bz), ll = sqrtf(kx * kx + kz * kz);   // EntityLivingBase.knockBack
    P.mx /= 2; P.mz /= 2; P.my = std::min(.4, P.my / 2 + .4);
    if (l > 1e-4f) { P.mx += bx / l * .4; P.mz += bz / l * .4; }
    if (kb > 0.f && ll > 1e-4f) { P.mx += kx / ll * kb * .5; P.mz += kz / ll * kb * .5; P.my += .1; }
}
static void pvpReceive(int from, int dmg, float kx, float kz, float kb, float oax, float oaz) {   // fui acertado por outro jogador (rede); (oax,oaz) = origem enviada pelo atacante
    pvpApply1(netLabel(from), true, oax, oaz, dmg, kx, kz, kb);
}
static void split2PvpReceive(int k, int from, int dmg, float kx, float kz, float kb, float oax, float oaz);   // splitscreen.inc: NP_PVP para o extra k deste cliente
static void deathDropAll() {   // morte em sobrevivencia: solta inventario, armadura e o item preso ao mouse (sem keepInventory)
    if (!gPvpP) return; const Phys& P = *gPvpP;
    auto drop = [&](Slot& s) { if (s.id && s.n > 0) spawnDrop(P.x + (frand() - .5) * .6, P.y + .5 + frand() * .8, P.z + (frand() - .5) * .6, (frand() - .5) * .3, .2 + frand() * .1, (frand() - .5) * .3, s.id, s.n, 20); s = Slot(); };
    for (Slot& s : inv) drop(s); for (Slot& s : armor) drop(s); drop(cur); for (Slot& s : craftG) drop(s);
}
static void netDeathAnnounce(const std::string& why) {
    if (gSplitCtx == 2) { chatAdd(splitNick(gS2i) + " morreu: " + why, 0xFFFFFF); return; }   // jogador 2 (split): so o chat local
    std::string msg = netName() + (why.rfind("Foi morto por", 0) == 0 ? " foi morto por " + gPvpKiller : " morreu: " + why);
    chatAdd(msg, 0xFFFFFF);
    if (!netOn) return;
    NBuf b = netMk(NP_DEATH); b.put<uint8_t>(1); const std::string m = msg.substr(0, 120); b.put<uint8_t>((uint8_t)m.size()); for (char c : m) b.put<uint8_t>((uint8_t)c);
    netBroadcast(b);
}
static void netHurtAnnounce() { if (!netOn || gSplitCtx == 2) return; NBuf b = netMk(NP_HURT); b.put<uint8_t>(0); netBroadcast(b); }   // avisa os outros: levei dano -> flash vermelho no meu corpo
static void netRespawnAnnounce() { if (!netOn || gSplitCtx == 2) return; NBuf b = netMk(NP_DEATH); b.put<uint8_t>(0); b.put<uint8_t>(0); netBroadcast(b); }
