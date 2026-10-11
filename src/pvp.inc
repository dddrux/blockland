// ================= PvP + morte de jogador (multiplayer) =================
// Autoridade: quem ataca detecta o golpe (alcance 3, 6 no criativo) e manda NP_PVP (alvo, dano, direcao, sprint); o alvo aplica dano,
// invulnerabilidade (10 ticks) e knockback como no 1.8.9. Ao morrer o jogador solta o inventario (como no vanilla), avisa todos com
// NP_DEATH (mensagem no chat + animacao de queda de 20 ticks no corpo dele) e, ao renascer, NP_DEATH com flag 0.
static Phys* gPvpP = nullptr;   // jogador local (definido no loop principal)
static std::string gPvpKiller;  // nome de quem deu o ultimo golpe de jogador (para a mensagem de morte)

static bool pvpAttack(const float* eye, const float* dir, int held, bool crit, bool sprint, float reach) {
    if (!netOn) return false;
    int best = -1; float bt = reach;
    for (auto& kv : remotes) {
        const RemotePl& r = kv.second; if (!r.init || r.deathT >= 0 || r.gm == 3 || kv.first == netSelf) continue;
        const float hw = .3f + .1f, h = r.sneak ? 1.65f : 1.8f, lo[3] = {(float)r.x - hw, (float)r.y - .1f, (float)r.z - hw}, hi[3] = {(float)r.x + hw, (float)r.y + h + .1f, (float)r.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = kv.first; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;   // bloco na frente
    int dmg = mobWeaponDmg(held); if (crit) dmg = (int)(dmg * 1.5f);
    NBuf b = netMk(NP_PVP); b.put<uint8_t>((uint8_t)best); b.put<int16_t>((int16_t)dmg); b.put<float>(dir[0]); b.put<float>(dir[2]); b.put<float>(sprint ? 1.f : 0.f);
    netBroadcast(b);
    return true;
}
static void pvpReceive(int from, int dmg, float kx, float kz, float kb) {   // fui acertado por outro jogador
    if (!gPvpP || creative || gDead || dmg <= 0) return;
    const bool arrow = kb < 0.f; if (arrow) kb = 0.f;   // flecha: kb < 0
    gPvpKiller = netLabel(from);
    if (!survDamage(dmg, ((arrow ? "Foi alvejado por " : "Foi morto por ") + gPvpKiller).c_str())) return;   // bloqueado pela invulnerabilidade = sem dano e sem recuo
    Phys& P = *gPvpP; float bx = kx, bz = kz;   // (kx,kz) = olhar do atacante; o recuo base vai da posicao dele para a minha (1.8.9)
    { auto it = remotes.find(from); if (it != remotes.end() && it->second.init) { const float dx = (float)(P.x - it->second.x), dz = (float)(P.z - it->second.z); if (dx * dx + dz * dz > 1e-6f) { bx = dx; bz = dz; } } }
    gHurtAtYaw = atan2f(-bz, -bx) * 57.29578f - (180.f - gYawNow);   // 1.8.9 attackedAtYaw (atacante - eu) relativo ao meu yaw; usado na inclinacao da camera
    const float l = sqrtf(bx * bx + bz * bz), ll = sqrtf(kx * kx + kz * kz);   // EntityLivingBase.knockBack
    P.mx /= 2; P.mz /= 2; P.my = std::min(.4, P.my / 2 + .4);
    if (l > 1e-4f) { P.mx += bx / l * .4; P.mz += bz / l * .4; }
    if (kb > 0.f && ll > 1e-4f) { P.mx += kx / ll * kb * .5; P.mz += kz / ll * kb * .5; P.my += .1; }
}
static void deathDropAll() {   // morte em sobrevivencia: solta inventario, armadura e o item preso ao mouse (sem keepInventory)
    if (!gPvpP) return; const Phys& P = *gPvpP;
    auto drop = [&](Slot& s) { if (s.id && s.n > 0) spawnDrop(P.x + (frand() - .5) * .6, P.y + .5 + frand() * .8, P.z + (frand() - .5) * .6, (frand() - .5) * .3, .2 + frand() * .1, (frand() - .5) * .3, s.id, s.n, 20); s = Slot(); };
    for (Slot& s : inv) drop(s); for (Slot& s : armor) drop(s); drop(cur); for (Slot& s : craftG) drop(s);
}
static void netDeathAnnounce(const std::string& why) {
    if (gSplitCtx == 2) { chatAdd("Jogador 2 morreu: " + why, 0xFFFFFF); return; }   // jogador 2 (split): so o chat local
    std::string msg = netName() + (why.rfind("Foi morto por", 0) == 0 ? " foi morto por " + gPvpKiller : " morreu: " + why);
    chatAdd(msg, 0xFFFFFF);
    if (!netOn) return;
    NBuf b = netMk(NP_DEATH); b.put<uint8_t>(1); const std::string m = msg.substr(0, 120); b.put<uint8_t>((uint8_t)m.size()); for (char c : m) b.put<uint8_t>((uint8_t)c);
    netBroadcast(b);
}
static void netHurtAnnounce() { if (!netOn || gSplitCtx == 2) return; NBuf b = netMk(NP_HURT); b.put<uint8_t>(0); netBroadcast(b); }   // avisa os outros: levei dano -> flash vermelho no meu corpo
static void netRespawnAnnounce() { if (!netOn || gSplitCtx == 2) return; NBuf b = netMk(NP_DEATH); b.put<uint8_t>(0); b.put<uint8_t>(0); netBroadcast(b); }
