// ---------- CERVEJARIA (patchBrew.1): bloco, tela, tempo de preparo, combustivel, salvar, multiplayer ----------
// Bloco = XB_DECOR com meta&7 == 6 (bits 3-5 = quais das 3 garrafas existem, so para o modelo). Conteudo por posicao em gBrews (como o TileEntityBrewingStand):
// s[0..2] garrafas, s[3] ingrediente, s[4] po de blaze (20 cargas); 400 ticks por rodada, 1 carga por rodada. Tela reaproveita a do bau (gBrewUi + gChest).
// Multiplayer: host manda; convidado abre via NP_CHESTREQ/NP_CHEST (formato da fornalha: slots 0..4 itens, slot 5 = progresso 0..255, slot 6 = cargas), sem mudar NET_VER.
struct BrewData { Slot s[5]; int brew = 0, fuel = 0; };
static std::map<ChestKey, BrewData> gBrews;
static int gBrewPos[3] = {0, 0, 0};
static const int BREW_TIME = 400;
static inline bool brewIsBlock(int b, int m) { return b == XB_DECOR && (m & 7) == 6; }
static bool brewHere(int x, int y, int z) { return brewIsBlock(getB(x, y, z), getM(x, y, z)); }
static BrewData* brewAt(int x, int y, int z, bool create = false) {
    if (!brewHere(x, y, z)) return nullptr;
    auto it = gBrews.find(ckey(x, y, z)); if (it != gBrews.end()) return &it->second;
    return create ? &gBrews[ckey(x, y, z)] : nullptr;
}
static bool brewGuest() { return netSock >= 0 && !netHost && netConn; }
static bool brewCanBrew(const BrewData& b) {
    if (b.s[3].id <= 0 || b.s[3].n <= 0) return false;
    for (int i = 0; i < 3; i++) if (b.s[i].id > 0 && b.s[i].n > 0 && potBrew(b.s[i].id, b.s[3].id)) return true;
    return false;
}
static void brewSetMeta(int x, int y, int z, const BrewData& b) {
    int bits = 0; for (int i = 0; i < 3; i++) if (b.s[i].id > 0 && b.s[i].n > 0) bits |= 1 << i;
    const int nm = 6 | (bits << 3), m = getM(x, y, z); if (nm == m) return;
    const int sv = gRsLastMeta; setBM(x, y, z, XB_DECOR, nm); gRsLastMeta = sv; netSendBlock(x, y, z, XB_DECOR, nm);
}
static void brewStep(BrewData& b) {   // TileEntityBrewingStand.update
    if (b.fuel <= 0 && b.s[4].id > 0 && b.s[4].n > 0 && b.s[4].id == potIngs().blaze) { b.fuel = 20; if (--b.s[4].n <= 0) b.s[4] = Slot(); }
    const bool can = brewCanBrew(b);
    if (b.brew > 0) {
        if (!can) { b.brew = 0; return; }
        if (--b.brew == 0) {
            for (int i = 0; i < 3; i++) if (b.s[i].id > 0 && b.s[i].n > 0) { const int r = potBrew(b.s[i].id, b.s[3].id); if (r) b.s[i] = {r, 1, 0}; }
            if (--b.s[3].n <= 0) b.s[3] = Slot();
        }
    } else if (can && b.fuel > 0) { b.fuel--; b.brew = BREW_TIME; }
}
static void brewTick() {   // por runWorldTick (so quem manda no mundo); so em chunks carregados da dimensao atual
    if (gBrews.empty()) return;
    for (auto& kv : gBrews) {
        const int x = std::get<0>(kv.first), yy = std::get<1>(kv.first), z = std::get<2>(kv.first), y = yy & 4095;
        if ((yy >> 12) != gDim || !getC(fdiv(x, CW), fdiv(z, CW)) || !brewHere(x, y, z)) continue;
        BrewData& b = kv.second; if (!b.brew && !brewCanBrew(b) && !(b.fuel <= 0 && b.s[4].id)) continue;
        const int ob = b.brew; brewStep(b); brewSetMeta(x, y, z, b);
        if (b.brew > 0 && b.brew % 40 == 0) sndEvtAt("brewing_stand.brew", x + .5, y + .5, z + .5, .5f);
        if (ob > 0 && b.brew == 0) sndEvtAt("random.potion.brewed", x + .5, y + .5, z + .5, .8f);
    }
}
static void brewOnRemoved(int x, int y, int z) {   // bloco saiu: solta o conteudo (so host/offline)
    auto it = gBrews.find(ckey(x, y, z)); if (it == gBrews.end()) return;
    if (!(netSock >= 0 && !netHost)) for (const Slot& s : it->second.s) if (s.id && s.n > 0)
        spawnDrop(x + .1 + frand() * .8, y + .1 + frand() * .8, z + .1 + frand() * .8, (frand() - .5) * .1, frand() * .1 + .2, (frand() - .5) * .1, s.id, s.n, 10, 0, s.nbt);
    gBrews.erase(it);
}
// ---- tela ----
static Slot* brewSlotRef(int k) {
    if (!gBrewUi || k < 0 || k > 4) return nullptr;
    BrewData* b = brewAt(gBrewPos[0], gBrewPos[1], gBrewPos[2], false); return b ? &b->s[k] : nullptr;
}
static bool brewProgress(float* prog, int* fuel) {
    *prog = 0; *fuel = 0; if (!gBrewUi) return false; BrewData* b = brewAt(gBrewPos[0], gBrewPos[1], gBrewPos[2], false); if (!b) return false;
    *prog = b->brew > 0 ? 1.f - b->brew / (float)BREW_TIME : 0.f; *fuel = b->fuel; return true;
}
static void brewShift(Slot& s) {   // shift-click da mochila: po de blaze -> combustivel, ingrediente -> ingrediente, pocao/garrafa -> primeira garrafa livre
    int k = -1; const PotIng& I = potIngs();
    if (s.id == I.blaze) k = 4; else if (potIsIngredient(s.id)) k = 3; else if (potIs(s.id) || s.id == I.bottle) k = 0; else return;
    for (int t = 0; t < (k == 0 ? 3 : 1); t++) {
        Slot* d = brewSlotRef(k == 0 ? t : k); if (!d) return;
        if (d->id && !sameStack(*d, s)) continue;
        const int put = std::min(s.n, maxStack(s.id) - (d->id ? d->n : 0)); if (put <= 0) continue;
        if (!d->id) { d->id = s.id; d->nbt = s.nbt; d->d = s.d; d->n = 0; } d->n += put; s.n -= put; if (s.n <= 0) s = Slot(); return;
    }
}
// ---- multiplayer ----
static std::map<ChestKey, Uint32> gBrewWatch; static BrewData gBrewSnap; static Uint32 gBrewReq = 0;
static bool brewSlotsEq(const BrewData& a, const BrewData& b) { for (int i = 0; i < 5; i++) if (a.s[i].id != b.s[i].id || a.s[i].n != b.s[i].n || a.s[i].nbt != b.s[i].nbt) return false; return true; }
static void brewNetSend(int x, int y, int z, const BrewData& f, bool progress) {
    if (!netOn) return; ChestInv t; for (int i = 0; i < 5; i++) t.s[i] = f.s[i];
    for (int i = 0; i < 5; i++) if (t.s[i].id > 0 && t.s[i].n > 0) netSendNbt(t.s[i].nbt);
    NBuf b = netMk(NP_CHEST); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z);
    const int pr = progress && f.brew > 0 ? std::max(1, std::min(255, (BREW_TIME - f.brew) * 255 / BREW_TIME)) : 0, fu = progress ? std::min(20, f.fuel) : 0;
    for (int i = 0; i < CHEST_SLOTS; i++) {
        if (i < 5) { const Slot& s = t.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
        else { b.put<int32_t>(i == 5 ? pr : i == 6 ? fu : 0); b.put<uint8_t>(0); }
    }
    chestNbtTailPut(b, t); netBroadcast(b);
}
static void brewNetReq(int x, int y, int z) { if (!netHost) return; BrewData* f = brewAt(x, y, z, true); if (!f) return; gBrewWatch[ckey(x, y, z)] = SDL_GetTicks(); brewNetSend(x, y, z, *f, true); }
static void brewNetRecv(NBuf& b, int x, int y, int z) {
    ChestInv t; int prog[2] = {0, 0};
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>();
        if (i < 5) { const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } } else if (i < 7) prog[i - 5] = std::max(0, std::min(255, (int)w)); }
    chestNbtTailGet(b, t);
    if (netHost) { BrewData* f = brewAt(x, y, z, false); if (!f) return; for (int i = 0; i < 5; i++) f->s[i] = t.s[i]; gBrewWatch[ckey(x, y, z)] = SDL_GetTicks(); return; }
    if (!gBrewUi || gBrewPos[0] != x || gBrewPos[1] != y || gBrewPos[2] != z) return;
    BrewData* f = brewAt(x, y, z, true); if (!f) return;
    for (int i = 0; i < 5; i++) f->s[i] = t.s[i];
    f->brew = prog[0] > 0 ? std::max(1, BREW_TIME - prog[0] * BREW_TIME / 255) : 0; f->fuel = std::min(20, prog[1]);
    gBrewSnap = *f; gChWait = false;
}
static void brewSyncNow() {
    if (!brewGuest() || !gBrewUi || gChWait) return;
    BrewData* f = brewAt(gBrewPos[0], gBrewPos[1], gBrewPos[2], false); if (!f) return;
    if (!brewSlotsEq(*f, gBrewSnap)) { gBrewSnap = *f; brewNetSend(gBrewPos[0], gBrewPos[1], gBrewPos[2], *f, false); }
    const Uint32 now = SDL_GetTicks(); if (now - gBrewReq > 10000) { gBrewReq = now; NBuf r = netMk(NP_CHESTREQ); r.put<int32_t>(gBrewPos[0]); r.put<int32_t>(gBrewPos[1]); r.put<int32_t>(gBrewPos[2]); netBroadcast(r); }
}
static void brewWatchPush() {   // host: a cada 10 ticks, progresso para quem tem a cervejaria aberta (30 s)
    if (!netHost || !netOn || gBrewWatch.empty()) return; const Uint32 now = SDL_GetTicks();
    for (auto it = gBrewWatch.begin(); it != gBrewWatch.end();) {
        if (now - it->second > 30000) { it = gBrewWatch.erase(it); continue; }
        const int x = std::get<0>(it->first), yy = std::get<1>(it->first), z = std::get<2>(it->first);
        if ((yy >> 12) == gDim) if (BrewData* f = brewAt(x, yy & 4095, z, false)) brewNetSend(x, yy & 4095, z, *f, true);
        ++it;
    }
}
static int brewOpenAt(int x, int y, int z) {   // 0 = nao e cervejaria; 1 = abriu (convidado: espera o host mandar o conteudo)
    if (!brewHere(x, y, z)) return 0;
    brewAt(x, y, z, true); gBrewUi = gChest = true; gFurn = false; gAnvUi = false; gTrashUi = false; gChEnder = false; gChShulker = false; gChWait = false; gChRows = 1; gChCnt = 1; gBrewPos[0] = x; gBrewPos[1] = y; gBrewPos[2] = z;
    if (brewGuest()) { *brewAt(x, y, z, true) = BrewData(); gBrewSnap = BrewData(); gChWait = true; gChT0 = SDL_GetTicks(); gBrewReq = gChT0;
        NBuf r = netMk(NP_CHESTREQ); r.put<int32_t>(x); r.put<int32_t>(y); r.put<int32_t>(z); netBroadcast(r); }
    return 1;
}
static void brewClosed() {
    if (brewGuest() && gBrewUi && !gChWait) brewSyncNow();
    if (brewGuest() && gBrewUi) gBrews.erase(ckey(gBrewPos[0], gBrewPos[1], gBrewPos[2]));
    if (!brewGuest() && gBrewUi) if (BrewData* b = brewAt(gBrewPos[0], gBrewPos[1], gBrewPos[2], false)) brewSetMeta(gBrewPos[0], gBrewPos[1], gBrewPos[2], *b);
    gBrewUi = gChest = false; gChRows = 3; gChCnt = 0; gChWait = false;
}
static bool brewFrame(double px, double py, double pz) {
    (void)py; if (!brewAt(gBrewPos[0], gBrewPos[1], gBrewPos[2], false)) return true;
    const double dx = px - (gBrewPos[0] + .5), dz = pz - (gBrewPos[2] + .5); if (dx * dx + dz * dz > 64.0) return true;
    if (gChWait) return SDL_GetTicks() - gChT0 > 2500;
    brewSyncNow(); return false;
}
// ---- salvar / carregar (saves/<mundo>/brewing.dat) ----
static bool brewAny(const BrewData& f) { for (const Slot& s : f.s) if (s.id) return true; return f.brew || f.fuel; }
static void brewSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/brewing.dat"; bool any = false; for (auto& kv : gBrews) if (brewAny(kv.second)) any = true;
    if (!any) { remove(path.c_str()); return; }
    FILE* fp = fopen(path.c_str(), "wb"); if (!fp) return; fprintf(fp, "BREW1\n");
    for (auto& kv : gBrews) { const BrewData& f = kv.second; if (!brewAny(f)) continue;
        fprintf(fp, "b %d %d %d %d %d\n", std::get<0>(kv.first), std::get<1>(kv.first), std::get<2>(kv.first), f.brew, f.fuel);
        for (int i = 0; i < 5; i++) { const Slot& s = f.s[i]; if (s.id && s.n > 0) fprintf(fp, "s %d %d %d %s\n", i, s.n, s.d, chestItemKey(s.id).c_str()); } }
    fclose(fp);
}
static void brewLoad(const std::string& dir) {   // depois de resetWorldData()
    gBrews.clear(); FILE* fp = fopen(("saves/" + dir + "/brewing.dat").c_str(), "rb"); if (!fp) return; static char ln[2048]; BrewData* cur2 = nullptr;
    while (fgets(ln, sizeof ln, fp)) {
        int x, y, z, a, b, i, n, d; char key[900];
        if (ln[0] == 'b' && sscanf(ln, "b %d %d %d %d %d", &x, &y, &z, &a, &b) == 5) { cur2 = &gBrews[ChestKey(x, y, z)]; cur2->brew = std::max(0, std::min(BREW_TIME, a)); cur2->fuel = std::max(0, std::min(20, b)); }
        else if (ln[0] == 's' && cur2 && sscanf(ln, "s %d %d %d %899s", &i, &n, &d, key) == 4 && i >= 0 && i < 5 && n > 0) { const int id = chestItemFromKey(key); if (id) { cur2->s[i].id = id; cur2->s[i].n = std::min(n, maxStack(id)); cur2->s[i].d = d; } }
    }
    fclose(fp);
}
// ---- garrafa de vidro: enche com agua (fonte/fluxo ou caldeirao) e vira Garrafa d'Agua ----
static bool bottleUse(Slot& s, const float* eye, const float* dir) {
    const PotIng& I = potIngs(); if (!I.bottle || s.id != I.bottle || !gPotId[0][0]) return false;
    RayHit r; if (!rayCast(eye, dir, gBlockReach, r, true)) return false; const int b = getB(r.x, r.y, r.z);
    if (b == XB_DECOR && (getM(r.x, r.y, r.z) & 3) == 0 && !brewHere(r.x, r.y, r.z)) {   // caldeirao: tira 1 nivel
        const int cm = getM(r.x, r.y, r.z), lv = (cm >> 2) & 3; if (lv <= 0) return false;
        setBM(r.x, r.y, r.z, XB_DECOR, (cm & 3) | ((lv - 1) << 2)); netSendBlock(r.x, r.y, r.z, XB_DECOR, (cm & 3) | ((lv - 1) << 2));
    } else if (!isWater(b)) return false;
    sndEvtUi("bottle.fill");
    if (!creative) { if (s.n <= 1) { s.id = gPotId[0][0]; s.n = 1; } else { --s.n; if (addItem(gPotId[0][0], 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, gPotId[0][0], 1, 10); } }
    else addItem(gPotId[0][0], 1);
    return true;
}
