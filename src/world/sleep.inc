// patchSleep.1: DORMIR IGUAL AO MINECRAFT (Java 1.12) para a cama de bloco (XB_BED) e para a cama de prop (Decocraft, propsys.inc).
// - Usar a cama de noite (ou em tempestade) deita o jogador; a tela escurece aos poucos e o dia so chega depois de 100 ticks (5 s) com
//   TODOS os jogadores da dimensao deitados ha pelo menos 100 ticks (WorldServer.areAllPlayersAsleep). So entao a noite passa (hora vira o
//   proximo amanhecer) e o clima limpa (resetRainAndThunder). Quem fica sozinho acordado impede de amanhecer.
// - Agachar/pular ou levar dano acorda; de dia, ou com monstro hostil a menos de 8x5 blocos, nao deita (mensagem como no MC).
// - Rede: cada cliente manda "dormindo" (bit 1 do byte sf) e "dormindo ha 100+ ticks" (bit 2, NET_VER 53) no NP_STATE; TODOS os clientes
//   enxergam o estado dos outros em `remotes`, entao a checagem de "todos prontos" e feita localmente. So o host (ou o solo) mexe na hora
//   e a manda aos convidados por NP_TIME (os convidados acordam quando a hora vira dia).
// - PESADELO: no meio do sono (tick 60) o overworld rola gBloodDreamPct% de chance de arrastar o jogador para a Dimensao Rubra
//   (blood.inc). Dormir la, com todos da dimensao deitados, volta ao overworld. E a UNICA forma de ir para la.
// Incluido depois de nether.inc (usa dimSwitch/gNetherYaw). Estado e declaracoes antecipadas: main.cpp.

static const int SLEEP_FULL_TICKS = 100;   // MC: sleepTimer >= 100 = "totalmente dormindo"
static const int SLEEP_DREAM_TICK = 60;    // momento do sono em que o pesadelo e sorteado (tela ja bem escura)

static bool sleepNightNow() { return (gWorldTime % DAY_TICKS) >= 12500 || gWeather == 2; }   // noite ou tempestade: EntityPlayer.trySleep (!isDaytime)
static bool sleepHostileType(int t) {
    return t == M_ZOMBIE || t == M_SKELETON || t == M_CREEPER || t == M_SPIDER || t == M_HUSK || t == M_CSPIDER || t == M_STRAY || t == M_WSKEL ||
           t == M_ZVILL || t == M_SILVER || t == M_ENDERMITE || t == M_WITCH || t == M_VINDIC || t == M_EVOKER || t == M_ILLUS || t == M_BLAZE || t == M_SLIME || t == M_MAGMA;
}
static bool sleepMonsterNear(double x, double y, double z) {   // 8 blocos na horizontal, 5 na vertical (EntityPlayer.trySleep)
    for (const MobEnt& m : MOBS) { if (m.dead || !sleepHostileType(m.type)) continue; if (fabs(m.x - x) < 8.0 && fabs(m.z - z) < 8.0 && fabs(m.y - y) < 5.0) return true; }
    return false;
}
// jogadores da MINHA dimensao que contam (eu + remotos vivos, nao espectadores, vistos ha menos de 3 s): total e quantos estao deitados
static void sleepCount(int& tot, int& sleeping, int& full) {
    tot = 1; sleeping = gSleepT > 0 ? 1 : 0; full = gSleepT >= SLEEP_FULL_TICKS ? 1 : 0;
    { int t2 = 0, s2 = 0, f2 = 0; sleepExtraCount(t2, s2, f2); tot += t2; sleeping += s2; full += f2; }   // patchSleep.2: jogadores locais do split tambem contam
    if (!netOn) return;
    for (const auto& kv : remotes) {
        const RemotePl& r = kv.second; if (kv.first == netSelf || !r.init || r.dim != gDim || r.gm == 3 || r.deathT >= 0 || SDL_GetTicks() - r.seen >= 3000u) continue;
        tot++; if (r.sleep) { sleeping++; if (r.sleepFull) full++; }
    }
}
static bool sleepAllReady() { int t, s, f; sleepCount(t, s, f); return t > 0 && f == t; }
static bool sleepOthersInOverworld() {   // algum OUTRO jogador acordado no overworld (para nao roubar a noite dele ao voltar do pesadelo)
    if (netOn) for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (kv.first != netSelf && r.init && r.dim == 0 && r.gm != 3 && r.deathT < 0 && SDL_GetTicks() - r.seen < 3000u) return true; }
    return false;
}
static void sleepSkipNight() {   // todos dormiram: amanhece (proximo inicio de dia) e o clima limpa; so host/solo
    gWorldTime = (gWorldTime / DAY_TICKS + 1) * DAY_TICKS;
    if (gWeather) { gWeather = 0; gWxLeft = 12000 + rand() % 168000; }
    if (netOn) netSendTime(false);
}

// chamada antes de deitar (cama de bloco ou de prop): true = pode deitar; senao ja avisou o motivo
static bool sleepCanStart(double x, double y, double z) {
    if (gSleepT > 0 || gSeated || gDead || gGm == 3) return false;
    if (gDim == 3) return true;   // pesadelo: sempre "noite", sem checar monstros
    if (!gDayCycle) { chatAdd("O tempo está parado: não dá para dormir.", 0xFFFF55); return false; }
    if (!sleepNightNow()) { chatAdd("Você só pode dormir à noite ou durante tempestades.", 0xFFFF55); return false; }
    if (sleepMonsterNear(x, y, z)) { chatAdd("Você não pode dormir agora, há monstros por perto.", 0xFFFF55); return false; }
    return true;
}
static void sleepBegin(int kind) { gSleepT = 1; gSleepKind = kind; gSleepRolled = false; gSleepShown = -1; gHurtWake = false; }

// patchSleep.4: onde o jogador fica ao sair da cama: ao lado direito (olhando para o pe) ou esquerdo, junto do pe e depois junto da cabeceira; se tudo estiver
// obstruido, em cima da cama. o[] = pes do jogador (centro do bloco)
static bool sleepFreeSpot(int x, int y, int z) {
    auto bad = [](int b) { return solid(b) || isLiquid(b) || b == LAVA || b == XB_FIRE; };
    return !bad(getB(x, y, z)) && !bad(getB(x, y + 1, z)) && solid(getB(x, y - 1, z)) && !isLiquid(getB(x, y - 1, z));
}
static void sleepWakeSpot(int fx, int y, int fz, int hx, int hz, double* o) {
    const int dx = hx - fx, dz = hz - fz, rx = dz, rz = -dx;   // direita de quem olha para o pe (frente = -cabeceira): (dz, -dx)
    const int cx[2] = {fx, hx}, cz[2] = {fz, hz};
    for (int side = 0; side < 2; side++) for (int k = 0; k < 2; k++) {
        const int sg = side ? -1 : 1, x = cx[k] + sg * rx, z = cz[k] + sg * rz;
        if (sleepFreeSpot(x, y, z)) { o[0] = x + .5; o[1] = y; o[2] = z + .5; return; }
    }
    o[0] = (fx + hx) / 2.0 + .5; o[1] = y + .5625 + .05; o[2] = (fz + hz) / 2.0 + .5;   // lados obstruidos: para cima (em cima da cama)
}
static bool sleepBedGeom(int x, int y, int z, int& fx, int& fz, int& hx, int& hz) {   // qualquer metade da cama de bloco -> pe e cabeceira
    static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0};
    if (getB(x, y, z) != XB_BED) return false;
    const int m = getM(x, y, z), pf = m & 3; fx = (m & 4) ? x - DX[pf] : x; fz = (m & 4) ? z - DZ[pf] : z; hx = fx + DX[pf]; hz = fz + DZ[pf]; return true;
}

static bool sleepUseBlockBed(int x, int y, int z) {   // botao direito numa cama de bloco (rsUse); sempre "usou"
    static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0};
    const int m = getM(x, y, z), pf = m & 3, fx = (m & 4) ? x - DX[pf] : x, fz = (m & 4) ? z - DZ[pf] : z, hx = fx + DX[pf], hz = fz + DZ[pf];
    if (gS2Use) return sleepExtraUse(fx, y, fz, hx, hz, -(float)pf * 90.f);   // patchSleep.2: jogador extra do split deita no proprio estado
    if (gDim == 0) { gSpX = fx + .5; gSpY = y + .6; gSpZ = fz + .5; spawnSave(); }   // dormir define o ponto de renascimento (na Rubra nao: ela some ao acordar)
    if (!sleepCanStart(fx + .5, y, fz + .5)) return true;
    sleepBegin(2); gSleepBx = fx; gSleepBy = y; gSleepBz = fz; gSleepHx = hx; gSleepHz = hz; gSleepYaw = -(float)pf * 90.f;   // cabeca para a cabeceira (convencao do jogo: yaw 0 = -Z)
    return true;
}

// 1 tick (20/s), chamada por propSleepTick no laco de fisica: true = o jogador esta deitado (fisica pulada)
static bool sleepTick(Phys& P, bool leave) {
    gSleepA = std::max(0.f, std::min(1.f, gSleepA + (gSleepT > 0 ? .01f : -.1f)));   // GuiIngame.renderSleepOverlay: escurece devagar ao longo dos 100 ticks (alfa max .86), clareia em 10 ticks
    if (gSleepT <= 0) { gHurtWake = false; gSleepKind = 0; return false; }
    if (gHurtWake) { gHurtWake = false; leave = true; }   // dano acorda
    double lx = 0, ly = 0, lz = 0, sx = 0, sz = 0; bool ok = false;
    if (gSleepKind == 1) {
        const PropEnt* f = nullptr;
        for (const PropEnt& e : gProps) if (std::fabs(e.x - gSleepPx) < 1e-4 && std::fabs(e.y - gSleepPy) < 1e-4 && std::fabs(e.z - gSleepPz) < 1e-4) { f = &e; break; }
        if (f) { ok = true; lx = sx = (f->box.x0 + f->box.x1) / 2; lz = sz = (f->box.z0 + f->box.z1) / 2; ly = f->box.y1; }
    } else if (gSleepKind == 2) {
        ok = getB(gSleepBx, gSleepBy, gSleepBz) == XB_BED && getB(gSleepHx, gSleepBy, gSleepHz) == XB_BED;   // cama quebrada: acorda
        lx = (gSleepBx + gSleepHx) / 2.0 + .5; lz = (gSleepBz + gSleepHz) / 2.0 + .5; ly = gSleepBy + .5625; sx = gSleepBx + .5; sz = gSleepBz + .5;
    }
    const bool stay = gDim == 3 || (gDayCycle && sleepNightNow());
    if (!ok || leave || !stay) {   // acordou (agachou/pulou, dano, amanheceu, cama sumiu)
        gSleepT = 0; gSleepKind = 0; gSleepShown = -1;
        if (ok) {
            if (gSleepKind == 2) { double o[3]; sleepWakeSpot(gSleepBx, gSleepBy, gSleepBz, gSleepHx, gSleepHz, o); P.x = P.px = o[0]; P.y = P.py = o[1]; P.z = P.pz = o[2]; }   // patchSleep.4
            else { P.x = P.px = sx; P.z = P.pz = sz; P.y = P.py = ly + .05; }
        }
        P.mx = P.my = P.mz = 0; return false;
    }
    P.x = P.px = lx; P.z = P.pz = lz; P.y = P.py = ly; P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false;
    ++gSleepT;
    if (netOn && gSleepT % 10 == 0) {   // "Dormindo: 1/2" so quando muda
        int t, s, f; sleepCount(t, s, f); const int sig = t * 100 + s; if (t > 1 && sig != gSleepShown) { gSleepShown = sig; chatAdd("Dormindo: " + std::to_string(s) + "/" + std::to_string(t) + " jogadores", 0xAAAAAA); }
    }
    if (gSleepT == SLEEP_DREAM_TICK && !gSleepRolled && gDim == 0) {   // pesadelo: so no overworld, 1x por sono; depois de voltar de um, o proximo sono e tranquilo
        gSleepRolled = true;
        if (gBloodSafe) gBloodSafe = false;
        else if ((rand() % 100) < gBloodDreamPct) { gSleepT = 0; gSleepKind = 0; bloodEnter(P); return false; }
    }
    if (gSleepT >= SLEEP_FULL_TICKS && sleepAllReady()) {
        if (gDim == 3) { gSleepT = 0; gSleepKind = 0; gSleepShown = -1; bloodLeave(P); return false; }   // todos da Rubra dormiram: volta para o overworld
        if (!netOn || netHost) sleepSkipNight();   // convidados acordam quando recebem a nova hora (NP_TIME)
    }
    return true;
}
