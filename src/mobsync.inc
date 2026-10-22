// ---------- Mobs na rede (host-autoritativo) ----------
// O HOST simula todos os mobs (IA, fisica, spawn) e manda NP_MOBS (estado) a cada 2 ticks; o CONVIDADO nao simula: so interpola.
//  - golpe/flecha do convidado em mob -> NP_MOBHIT (o host aplica mobHurt e os drops saem pelo NP_DROP de sempre)
//  - mob hostil mira o jogador MAIS PROXIMO entre host, jogador 2 (split) e convidados (por id de rede, nunca ponteiro; revalida todo tick)
//  - dano de mob em convidado -> NP_MOBDMG (so o alvo age); o host gera chunks em volta dos convidados para os mobs de la nao congelarem
//  - mobs so existem na dimensao do host (cada jogador tem sua dimensao); convidado em outra dimensao nao ve os mobs do host.
// Registro por mob (32 bytes): uid i32, tipo u8, flags u8 (1 morto, 2 fogo, 4 perseguindo), x y z yaw f32, hyaw/hpit i16 (graus*100), hp i16, hurt u8, fuse u8, aim u8 (0..255), dead u8.
static double split2Dist2(double x, double z);   // splitscreen.inc
struct MobTgt { double x, y, z; float yaw, hyaw, hpit; Uint32 seen; };
static std::map<int, MobTgt> gMobTgt; static bool gMobGuestInit = false;
static const int MOBSYNC_PER_PKT = 30, MOBSYNC_TIMEOUT_MS = 1500;

static bool mobSyncGuestHit(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra, float lx, float lz) {
    if (!netOn || netHost) return false;
    NBuf b = netMk(NP_MOBHIT); b.put<int32_t>(e.uid); b.put<int16_t>((int16_t)dmg); b.put<float>(kx); b.put<float>(kz); b.put<uint8_t>(attacked ? 1 : 0); b.put<float>(kbExtra); b.put<float>(lx); b.put<float>(lz);
    netBroadcast(b); e.hurt = std::max(e.hurt, 10);   // feedback imediato (o estado real vem do host)
    return true;
}
static void mobSyncHitRecv(NBuf& b, int sender) {   // host: golpe de um convidado
    const int uid = b.get<int32_t>(), dmg = b.get<int16_t>(); const float kx = b.get<float>(), kz = b.get<float>(); const bool att = b.get<uint8_t>() != 0; const float kbe = b.get<float>(), lx = b.get<float>(), lz = b.get<float>();
    if (dmg <= 0 || dmg > 40) return;   // valor absurdo = pacote forjado
    for (MobEnt& e : MOBS) if (e.uid == uid && !e.dead) { gMobAtkUid = 0; mobHurt(e, dmg, kx, kz, att, kbe, lx, lz); return; }
}
static void mobDmgRecv(NBuf& b) {   // fui atacado por um mob simulado no host
    const int tid = b.get<uint8_t>(), dmg = b.get<int16_t>(); const float kx = b.get<float>(), kz = b.get<float>();
    const int l = b.get<uint8_t>(); std::string why; for (int i = 0; i < l; i++) why += (char)b.get<uint8_t>();
    if (tid != netSelf || !gPvpP || creative || gDead || dmg <= 0 || dmg > 40) return;
    if (!survDamage(dmg, why.c_str())) return;
    Phys& P = *gPvpP; const float n = sqrtf(kx * kx + kz * kz);
    P.mx /= 2; P.mz /= 2; P.my = std::min(.4, P.my / 2 + .4); if (n > 1e-4f) { P.mx += kx / n * .4; P.mz += kz / n * .4; }
}
static void mobSyncRecv(NBuf& b) {   // convidado: estado dos mobs vindo do host
    if (!netOn || netHost) return;
    const int dim = b.get<uint8_t>() & 3, cnt = b.get<uint8_t>(); const Uint32 now = SDL_GetTicks();
    if (dim != gDim) return;
    if (!gMobGuestInit) { gMobGuestInit = true; MOBS.clear(); gMobTgt.clear(); }   // mobs do mundo local do convidado nao valem: so os do host
    for (int i = 0; i < cnt; i++) {
        const int uid = b.get<int32_t>(), type = b.get<uint8_t>(), fl = b.get<uint8_t>(); MobTgt t; t.x = b.get<float>(); t.y = b.get<float>(); t.z = b.get<float>(); t.yaw = b.get<float>();
        t.hyaw = b.get<int16_t>() / 100.f; t.hpit = b.get<int16_t>() / 100.f; const int hp = b.get<int16_t>(), hurt = b.get<uint8_t>(), fuse = b.get<uint8_t>(), aim = b.get<uint8_t>(), dead = b.get<uint8_t>(); t.seen = now;
        if (uid <= 0 || type < 0 || type >= (int)(sizeof MD / sizeof MD[0])) continue;
        MobEnt* e = nullptr; for (MobEnt& m : MOBS) if (m.uid == uid) { e = &m; break; }
        if (!e) { MobEnt n; n.type = type; n.uid = uid; n.x = n.px = t.x; n.y = n.py = t.y; n.z = n.pz = t.z; n.yaw = n.pyaw = t.yaw; n.hyaw = n.phyaw = t.hyaw; n.hpit = n.phpit = t.hpit; MOBS.push_back(n); e = &MOBS.back(); }
        gMobTgt[uid] = t; e->hp = hp; e->fuse = fuse; e->aim = aim / 255.f; e->chase = (fl & 4) != 0; e->baby = (fl & 8) != 0; e->sheared = (fl & 16) != 0; e->fire = (fl & 2) ? 100 : 0;
        if (hurt > e->hurt) e->hurt = hurt; if ((fl & 1) && !e->dead) e->dead = std::max(1, dead);
    }
}
static bool mobSyncGuestTick() {   // 1x por tick no convidado: interpola, anima e remove mobs que o host parou de mandar
    if (!netOn || netHost) { if (gMobGuestInit) { gMobGuestInit = false; gMobTgt.clear(); } return false; }
    const Uint32 now = SDL_GetTicks();
    for (size_t i = 0; i < MOBS.size();) {
        MobEnt& e = MOBS[i]; auto it = gMobTgt.find(e.uid);
        if (it == gMobTgt.end() || now - it->second.seen > (Uint32)MOBSYNC_TIMEOUT_MS) { if (it != gMobTgt.end()) gMobTgt.erase(it); MOBS.erase(MOBS.begin() + i); continue; }
        const MobTgt& t = it->second;
        e.px = e.x; e.py = e.y; e.pz = e.z; e.pyaw = e.yaw; e.phyaw = e.hyaw; e.phpit = e.hpit; e.plimbAmt = e.limbAmt;
        e.x += (t.x - e.x) * .5; e.y += (t.y - e.y) * .5; e.z += (t.z - e.z) * .5;
        e.yaw += mwrap(t.yaw - e.yaw) * .5f; e.hyaw += (t.hyaw - e.hyaw) * .5f; e.hpit += (t.hpit - e.hpit) * .5f;
        const float mv = (float)sqrt((e.x - e.px) * (e.x - e.px) + (e.z - e.pz) * (e.z - e.pz));   // andar = deslocamento do tick (como o limbSwingAmount do Java)
        e.limbAmt += (std::min(1.f, mv * 4.f) - e.limbAmt) * .4f; e.limb += e.limbAmt; e.ground = true;
        if (e.hurt > 0) e.hurt--;
        if (e.dead && ++e.dead >= 21) { spawnPoof(e); gMobTgt.erase(e.uid); MOBS.erase(MOBS.begin() + i); continue; }
        i++;
    }
    return true;
}
// ---- host ----
static bool mobRemoteAlive(const RemotePl& r) { return r.init && r.dim == gDim && r.deathT < 0 && r.gm != 1 && r.gm != 3 && SDL_GetTicks() - r.seen < 3000u; }   // criativo/espectador/morto/fantasma nao sao alvo
static bool mobRemoteTick(MobEnt& e, Phys& P) {   // roda o mob no contexto do convidado mais proximo, se ele for mais perto que o host e o jogador 2
    if (!netOn || !netHost || remotes.empty() || e.dead || MD[e.type].ai < 4) return false;
    int bid = -1; double bd = 1e18;
    for (auto& kv : remotes) { if (kv.first == netSelf || !mobRemoteAlive(kv.second)) continue; const double d = (kv.second.x - e.x) * (kv.second.x - e.x) + (kv.second.z - e.z) * (kv.second.z - e.z); if (d < bd) { bd = d; bid = kv.first; } }
    if (bid < 0) return false;
    const double d1 = (!gDead && !creative && gGm != 3) ? (P.x - e.x) * (P.x - e.x) + (P.z - e.z) * (P.z - e.z) : 1e18;
    if (bd >= std::min(d1, split2Dist2(e.x, e.z))) return false;
    const RemotePl& r = remotes[bid]; Phys pr; pr.x = pr.px = r.x; pr.y = pr.py = r.y; pr.z = pr.pz = r.z;
    gMobVictim = bid; tickMob(e, r.x, r.y, r.z); hostileTick(e, pr); gMobVictim = -1;
    return true;
}
static void mobSyncHostTick(Phys& P) {
    if (!netOn || !netHost) return;
    static int tk = 0, rr = 0; tk++;
    std::vector<int> ids; for (auto& kv : remotes) if (kv.first != netSelf && mobRemoteAlive(kv.second)) ids.push_back(kv.first);
    if (!ids.empty()) {
        const RemotePl& r = remotes[ids[rr++ % ids.size()]]; const int pcx = fdiv((int)floor(r.x), CW), pcz = fdiv((int)floor(r.z), CW);   // 1 convidado por tick (rodizio)
        bool gen1 = false; for (int ring = 0; ring <= 3 && !gen1; ring++) for (int dz = -ring; dz <= ring && !gen1; dz++) for (int dx = -ring; dx <= ring && !gen1; dx++) { if (std::max(abs(dx), abs(dz)) != ring) continue; if (!getC(pcx + dx, pcz + dz)) { gen(pcx + dx, pcz + dz); gen1 = true; } }   // chunks em volta do convidado (sem eles os mobs la ficam congelados)
        Phys pr; pr.x = pr.px = r.x; pr.y = pr.py = r.y; pr.z = pr.pz = r.z; if (getC(pcx, pcz)) { hostileSpawn(pr); animalSpawn(pr); }   // nasce mob em volta do convidado tambem
    }
    if (tk & 1) return;
    std::vector<const MobEnt*> v; for (const MobEnt& e : MOBS) { if (e.y < -30 || e.dead >= 21) continue; v.push_back(&e); }
    for (size_t i = 0; i < v.size() || i == 0; i += MOBSYNC_PER_PKT) {
        NBuf b = netMk(NP_MOBS); const size_t n = std::min<size_t>(MOBSYNC_PER_PKT, v.size() - std::min(i, v.size())); b.put<uint8_t>((uint8_t)gDim); b.put<uint8_t>((uint8_t)n);
        for (size_t k = 0; k < n; k++) {
            MobEnt& e = *const_cast<MobEnt*>(v[i + k]);
            b.put<int32_t>(mobUid(e)); b.put<uint8_t>((uint8_t)e.type); b.put<uint8_t>((uint8_t)((e.dead ? 1 : 0) | (e.fire > 0 ? 2 : 0) | (e.chase ? 4 : 0) | (e.baby ? 8 : 0) | (e.sheared ? 16 : 0)));
            b.put<float>((float)e.x); b.put<float>((float)e.y); b.put<float>((float)e.z); b.put<float>(e.yaw);
            b.put<int16_t>((int16_t)std::max(-32000.f, std::min(32000.f, e.hyaw * 100.f))); b.put<int16_t>((int16_t)std::max(-32000.f, std::min(32000.f, e.hpit * 100.f)));
            b.put<int16_t>((int16_t)e.hp); b.put<uint8_t>((uint8_t)std::min(e.hurt, 255)); b.put<uint8_t>((uint8_t)std::min(std::max(e.fuse, 0), 255)); b.put<uint8_t>((uint8_t)(std::max(0.f, std::min(1.f, e.aim)) * 255.f)); b.put<uint8_t>((uint8_t)std::min(e.dead, 255));
        }
        netBroadcast(b); if (v.empty()) break;
    }
}
