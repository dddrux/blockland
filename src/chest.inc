// ---------- BAU: armazenamento por posicao (Parte 1 de 3) ----------
// O bloco do bau (XB_CHEST, redstone.inc) so guarda orientacao+tipo no meta; o CONTEUDO fica aqui, num mapa por posicao (x,y,z) com 27 slots
// (3 linhas x 9, como o TileEntityChest do 1.12.2). So os tipos 0 (bau) e 1 (bau armadilhado) guardam itens aqui; o Ender (2) e as caixas de
// shulker (3..18) ficam para a Parte 3. Parte 2 = tela (GUI) do bau; Parte 3 = bau duplo, tampa animada, Ender, rede.
// Itens sao salvos por NOME do arquivo do pack (sobrevivem a troca de resource pack, como os blocos do chunks16.dat) ou por id numerico (<PK0).
// Quebrar/substituir o bloco solta o conteudo (BlockChest.breakBlock -> InventoryHelper.dropInventoryItems), tambem no criativo.
static const int CHEST_SLOTS = 27;
struct ChestInv { Slot s[CHEST_SLOTS]; bool empty() const { for (const Slot& q : s) if (q.id && q.n > 0) return false; return true; } };
typedef std::tuple<int, int, int> ChestKey;
static inline ChestKey ckey(int x, int y, int z) { return ChestKey(x, y + (gDim << 12), z); }   // dimensao na chave: mesma coordenada no Nether e na superficie sao baus diferentes
static std::map<ChestKey, ChestInv> gChests;
static inline int chestType(int meta) { return (meta >> 2) & 31; }
static inline bool chestIsDbl(int meta) { const int t = chestType(meta); return t >= 19 && t <= 22; }               // metade de bau duplo (ver redstone.inc)
static inline int chestBase(int meta) { const int t = chestType(meta); return t >= 19 && t <= 22 ? (t - 19) >> 1 : t; }   // 0 normal, 1 armadilhado
static inline bool chestStores(int meta) { return chestBase(meta) <= 1; }   // bau / bau armadilhado (simples ou duplo)
// BAU DO ENDER (type 2): UM inventario de 27 slots POR JOGADOR, igual em todos os baus do Ender do mundo (InventoryEnderChest do 1.12.2).
// Quem manda e o host (ou o jogo local): guarda um inventario por jogador (chave = nome; o do proprio host = "@host") em saves/<mundo>/ender.dat.
// O convidado pede (NP_ENDERREQ) ao abrir, recebe so o dele (NP_ENDER com alvo = seu id) e manda de volta (alvo 255) o que mudou.
static std::map<std::string, ChestInv> gEnderMap; static ChestInv gEndGuest;
static void chestsClear() { gChests.clear(); gEnderMap.clear(); gEndGuest = ChestInv(); }
// conteudo do bau em (x,y,z); create=true cria vazio. nullptr se la nao ha um bau que guarda itens.
static ChestInv* chestAt(int x, int y, int z, bool create = false) {
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return nullptr;
    auto it = gChests.find(ckey(x, y, z)); if (it != gChests.end()) return &it->second;
    return create ? &gChests[ckey(x, y, z)] : nullptr;
}
// coloca n do item no bau (empilha nos iguais, depois vazios); devolve o que nao coube (para funil/dispensador no futuro)
static int chestAdd(ChestInv& c, int id, int n) {
    if (id <= 0) return n;
    for (int pass = 0; pass < 2 && n > 0; pass++) for (int i = 0; i < CHEST_SLOTS && n > 0; i++) {
        Slot& s = c.s[i]; if (pass == 0 ? (s.id == id && s.n > 0 && s.n < maxStack(id)) : (s.id == 0 || s.n <= 0)) {
            if (pass == 1) s = Slot(); const int put = std::min(n, maxStack(id) - s.n); s.id = id; s.n += put; n -= put; }
    }
    return n;
}
static int chestCount(const ChestInv& c) { int t = 0; for (const Slot& s : c.s) if (s.id && s.n > 0) t += s.n; return t; }
// o bloco do bau SAIU (quebrado, explodido, substituido): solta o conteudo e esquece o mapa. Chamado por setBM; so quem manda no mundo (local/host)
// solta itens (o convidado nao tem o conteudo, e o host recebe a edicao e solta).
static bool chestPartner(int x, int y, int z, int* px, int* pz) {   // outra metade do bau duplo em (x,y,z) (o bloco ainda tem o estado antigo)
    if (getB(x, y, z) != XB_CHEST) return false; const int m = getM(x, y, z); if (!chestIsDbl(m)) return false;
    const int d = m & 3, half = (chestType(m) - 19) & 1, sg = half ? -1 : 1, ax = x + CH_RV[d][0] * sg, az = z + CH_RV[d][1] * sg;
    if (getB(ax, y, az) != XB_CHEST) return false; const int n = getM(ax, y, az);
    if (!chestIsDbl(n) || (n & 3) != d || chestBase(n) != chestBase(m) || (((chestType(n) - 19) & 1) == half)) return false;
    *px = ax; *pz = az; return true;
}
// BlockChest.canPlaceBlockAt + juncao (1.12.2): so um vizinho do mesmo tipo e que ainda nao seja duplo; junta se o vizinho esta no eixo perpendicular a frente.
// devolve o meta do bau novo (dir | tipo<<2) ou -1 se nao pode colocar
static int chestPlaceMeta(int x, int y, int z, int var, int dir) {
    static const int D[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; int cnt = 0, sx = 0, sz = 0;
    for (const auto& d : D) { const int bx = x + d[0], bz = z + d[1];
        if (getB(bx, y, bz) == XB_CHEST && chestBase(getM(bx, y, bz)) == var) { if (chestIsDbl(getM(bx, y, bz))) return -1; cnt++; sx = bx; sz = bz; } }
    if (cnt > 1) return -1;
    int type = var;
    if (cnt == 1 && (((dir & 1) == 0) ? sz == z : sx == x)) { const bool left = sx - x == CH_RV[dir][0] && sz - z == CH_RV[dir][1]; type = 19 + 2 * var + (left ? 0 : 1); }
    return dir | (type << 2);
}
static void chestAfterPlace(int x, int y, int z, int meta) {   // o bau novo e metade de duplo: a parceira vira a outra metade e assume a mesma frente
    if (!chestIsDbl(meta)) return;
    const int d = meta & 3, half = (chestType(meta) - 19) & 1, base = chestBase(meta), sg = half ? -1 : 1, ax = x + CH_RV[d][0] * sg, az = z + CH_RV[d][1] * sg;
    const int nm = d | ((19 + 2 * base + (1 - half)) << 2);
    setBM(ax, y, az, XB_CHEST, nm); netSendBlock(ax, y, az, XB_CHEST, nm);
}
static void chestOnRemoved(int x, int y, int z) {
    { int px, pz; if (chestPartner(x, y, z, &px, &pz)) {   // quebrou uma metade: a outra vira bau simples (guarda o conteudo dela)
        const int m = getM(px, y, pz), nm = (m & 3) | (chestBase(m) << 2), sv = gRsLastMeta;
        setBM(px, y, pz, XB_CHEST, nm); gRsLastMeta = sv; netSendBlock(px, y, pz, XB_CHEST, nm); } }
    auto it = gChests.find(ckey(x, y, z)); if (it == gChests.end()) return;
    if (!(netSock >= 0 && !netHost)) for (const Slot& s : it->second.s) if (s.id && s.n > 0) {   // InventoryHelper: posicao aleatoria dentro da celula, velocidade pequena
        int left = s.n; while (left > 0) { const int k = std::min(left, 1 + rand() % 21); left -= k;
            spawnDrop(x + .1 + frand() * .8, y + .1 + frand() * .8, z + .1 + frand() * .8, (frand() - .5) * .1, frand() * .1 + .2, (frand() - .5) * .1, s.id, k, 10); }
    }
    gChests.erase(it);
}
// ---------------- salvar / carregar (saves/<mundo>/chests.dat) ----------------
static std::string chestItemKey(int id) { if (isPk(id)) return "@" + PK[id - PK0].file; return "#" + std::to_string(id); }
static int chestItemFromKey(const std::string& k) {
    if (k.size() < 2) return 0;
    if (k[0] == '#') { const int id = atoi(k.c_str() + 1); return id > 0 && id < PK0 ? id : 0; }
    if (k[0] == '@') { auto it = gPkByFile.find(k.substr(1)); return it != gPkByFile.end() ? it->second : 0; }   // item que o pack atual nao tem: some
    return 0;
}
static void enderSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/ender.dat"; bool any = false; for (auto& kv : gEnderMap) if (!kv.second.empty()) any = true;
    if (!any) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fprintf(f, "ENDER1\n");
    for (auto& kv : gEnderMap) { if (kv.second.empty()) continue; fprintf(f, "p %s\n", kv.first.c_str());
        for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = kv.second.s[i]; if (s.id && s.n > 0) fprintf(f, "s %d %d %s\n", i, s.n, chestItemKey(s.id).c_str()); } }
    fclose(f);
}
static void enderLoad(const std::string& dir) {
    gEnderMap.clear(); FILE* f = fopen(("saves/" + dir + "/ender.dat").c_str(), "rb"); if (!f) return; char ln[1024]; ChestInv* cur = nullptr;
    while (fgets(ln, sizeof ln, f)) {
        int i, n; char key[900];
        if (ln[0] == 'p' && ln[1] == ' ') { std::string nm = ln + 2; while (!nm.empty() && (nm.back() == '\n' || nm.back() == '\r')) nm.pop_back(); cur = &gEnderMap[nm]; }
        else if (ln[0] == 's' && cur && sscanf(ln, "s %d %d %899s", &i, &n, key) == 3 && i >= 0 && i < CHEST_SLOTS && n > 0) {
            const int id = chestItemFromKey(key); if (id) { cur->s[i].id = id; cur->s[i].n = std::min(n, maxStack(id)); } }
    }
    fclose(f);
}
static void chestsSave(const std::string& dir) {
    enderSave(dir);
    const std::string path = "saves/" + dir + "/chests.dat";
    if (gChests.empty()) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fprintf(f, "CHESTS1\n");
    for (auto& kv : gChests) {
        if (kv.second.empty()) continue;
        fprintf(f, "c %d %d %d\n", std::get<0>(kv.first), std::get<1>(kv.first), std::get<2>(kv.first));
        for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = kv.second.s[i]; if (s.id && s.n > 0) fprintf(f, "s %d %d %s\n", i, s.n, chestItemKey(s.id).c_str()); }
    }
    fclose(f);
}
static void chestsLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    gChests.clear(); enderLoad(dir);
    FILE* f = fopen(("saves/" + dir + "/chests.dat").c_str(), "rb"); if (!f) return; char ln[1024]; ChestInv* cur = nullptr;
    while (fgets(ln, sizeof ln, f)) {
        int x, y, z, i, n; char key[900];
        if (ln[0] == 'c' && sscanf(ln, "c %d %d %d", &x, &y, &z) == 3) cur = &gChests[ChestKey(x, y, z)];
        else if (ln[0] == 's' && cur && sscanf(ln, "s %d %d %899s", &i, &n, key) == 3 && i >= 0 && i < CHEST_SLOTS && n > 0) {
            const int id = chestItemFromKey(key); if (id) { cur->s[i].id = id; cur->s[i].n = std::min(n, maxStack(id)); }
        }
    }
    fclose(f);
}

// ---------------- tela do bau: abrir, fechar, sincronizar (Parte 2 e 3) ----------------
// Bau duplo = 54 slots: a metade de menor X (oeste) ou menor Z (norte) vem primeiro (InventoryLargeChest do 1.12.2).
// Multiplayer: o HOST e dono do conteudo. O convidado pede (NP_CHESTREQ), recebe NP_CHEST e, a cada mudanca, manda o estado novo; o host repassa.
static int gChPos[2][3], gChCnt = 0; static ChestInv gChSnap[2]; static bool gChLoaded[2]; static Uint32 gChT0 = 0;
static bool chestEq(const ChestInv& a, const ChestInv& b) { for (int i = 0; i < CHEST_SLOTS; i++) if (a.s[i].id != b.s[i].id || a.s[i].n != b.s[i].n) return false; return true; }
static int chestView(int x, int y, int z, int pos[2][3], ChestInv* inv[2]) {   // 0 = nao e bau que guarda; 1 simples; 2 duplo (superior primeiro)
    int px, pz, n = 1; const int a[3] = {x, y, z};
    if (chestPartner(x, y, z, &px, &pz)) { const int b[3] = {px, y, pz}; const bool pUp = px < x || pz < z; for (int i = 0; i < 3; i++) { pos[0][i] = pUp ? b[i] : a[i]; pos[1][i] = pUp ? a[i] : b[i]; } n = 2; }
    else for (int i = 0; i < 3; i++) pos[0][i] = a[i];
    for (int h = 0; h < n; h++) { inv[h] = chestAt(pos[h][0], pos[h][1], pos[h][2], true); if (!inv[h]) return 0; }
    return n;
}
static bool chestGuest() { return netSock >= 0 && !netHost && netConn; }
static Slot* chestSlotRef(int k) {
    if (!gChest || k < 0 || k >= gChCnt * 27) return nullptr;
    if (gChEnder) return k < 27 ? &(chestGuest() ? gEndGuest : gEnderMap["@host"]).s[k] : nullptr;
    ChestInv* c = chestAt(gChPos[k / 27][0], gChPos[k / 27][1], gChPos[k / 27][2], false); return c ? &c->s[k % 27] : nullptr;
}
static void netSendChest(int x, int y, int z, const ChestInv& c) {
    if (!netOn) return;
    NBuf b = netMk(NP_CHEST); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z);
    for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
    netBroadcast(b);
}
static void chestNetRecv(NBuf& b) {
    const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(); ChestInv t;
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>(); const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } }
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return;
    int h = -1; if (gChest) for (int k = 0; k < gChCnt; k++) if (gChPos[k][0] == x && gChPos[k][1] == y && gChPos[k][2] == z) h = k;
    ChestInv* c = chestAt(x, y, z, netHost || h >= 0 || gChests.count(ckey(x, y, z)) > 0); if (!c) return;   // convidado so guarda o que abriu
    *c = t;
    if (h >= 0) { gChSnap[h] = t; if (!gChLoaded[h]) { gChLoaded[h] = true; bool all = true; for (int k = 0; k < gChCnt; k++) all = all && gChLoaded[k]; if (all) gChWait = false; } }
}
static void chestNetReq(NBuf& b) {   // so o host responde (a todos: quem nao tem o bau aberto ignora)
    if (!netHost) return;
    const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(); int pos[2][3]; ChestInv* inv[2];
    const int n = chestView(x, y, z, pos, inv); for (int h = 0; h < n; h++) netSendChest(pos[h][0], pos[h][1], pos[h][2], *inv[h]);
}
static void netSendEnder(int target, const ChestInv& c) {   // target = id do convidado (host -> convidado) ou 255 (convidado -> host)
    if (!netOn) return;
    NBuf b = netMk(NP_ENDER); b.put<uint8_t>((uint8_t)target);
    for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
    if (netHost && target != 255) { for (auto& p : peers) if (p.id == target) netTo(p.a, b); } else netBroadcast(b);   // do host, so para o dono
}
static void enderNetRecv(NBuf& b, int sender) {
    const int target = b.get<uint8_t>(); ChestInv t;
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>(); const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } }
    if (netHost) { if (target == 255 && sender != netSelf) gEnderMap[netLabel(sender)] = t; return; }   // convidado -> host: guarda o inventario DELE
    if (target != netSelf) return;   // pacote repassado de/para outro jogador
    gEndGuest = t; if (gChest && gChEnder) { gChSnap[0] = t; gChLoaded[0] = true; gChWait = false; }
}
static void enderNetReq(int sender) { if (netHost && sender != netSelf) netSendEnder(sender, gEnderMap[netLabel(sender)]); }
static void chestLid(bool open) {   // tampa aberta = bit 7 do meta (vai pela rede como qualquer edicao de bloco)
    for (int h = 0; h < gChCnt; h++) { const int x = gChPos[h][0], y = gChPos[h][1], z = gChPos[h][2];
        if (getB(x, y, z) != XB_CHEST) continue; const int m = getM(x, y, z), nm = open ? (m | 128) : (m & ~128);
        if (nm != m) { setBM(x, y, z, XB_CHEST, nm); netSendBlock(x, y, z, XB_CHEST, nm); } }
}
// 0 = nao e bau; 1 = abriu (o chamador abre a tela); 2 = bloqueado por bloco em cima (consome o clique sem abrir)
static int chestOpenAt(int x, int y, int z) {
    if (getB(x, y, z) == XB_CHEST && chestType(getM(x, y, z)) == 2) {   // Bau do Ender: inventario pessoal
        if (rsFullCube(getB(x, y + 1, z))) return 2;
        gChest = true; gChEnder = true; gChCnt = 1; gChRows = 3; gChPos[0][0] = x; gChPos[0][1] = y; gChPos[0][2] = z;
        if (chestGuest()) gEndGuest = ChestInv();
        gChSnap[0] = chestGuest() ? gEndGuest : gEnderMap["@host"]; gChLoaded[0] = !chestGuest(); gChWait = chestGuest(); gChT0 = SDL_GetTicks();
        if (gChWait) { NBuf b = netMk(NP_ENDERREQ); netBroadcast(b); }
        chestLid(true); return 1;
    }
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return 0;
    int pos[2][3]; ChestInv* inv[2]; const int n = chestView(x, y, z, pos, inv); if (!n) return 0;
    for (int h = 0; h < n; h++) if (rsFullCube(getB(pos[h][0], pos[h][1] + 1, pos[h][2]))) return 2;
    gChest = true; gChEnder = false; gChCnt = n; gChRows = 3 * n; memcpy(gChPos, pos, sizeof pos);
    for (int h = 0; h < n; h++) { gChSnap[h] = *inv[h]; gChLoaded[h] = !chestGuest(); }
    gChWait = chestGuest(); gChT0 = SDL_GetTicks();
    if (gChWait) { NBuf b = netMk(NP_CHESTREQ); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z); netBroadcast(b); }
    chestLid(true); return 1;
}
static void chestSyncNow() {   // manda por rede o que mudou desde o ultimo envio
    if (gChWait) return;
    if (gChEnder) { ChestInv& e = chestGuest() ? gEndGuest : gEnderMap["@host"]; if (!chestEq(e, gChSnap[0])) { gChSnap[0] = e; if (chestGuest()) netSendEnder(255, e); } return; }
    for (int h = 0; h < gChCnt; h++) { ChestInv* c = chestAt(gChPos[h][0], gChPos[h][1], gChPos[h][2], false); if (!c || chestEq(*c, gChSnap[h])) continue; gChSnap[h] = *c; netSendChest(gChPos[h][0], gChPos[h][1], gChPos[h][2], *c); }
}
// a cada quadro com a tela aberta; true = fechar (bau sumiu, jogador longe (8 blocos, como o canInteractWith), convidado sem resposta)
static bool chestFrame(double px, double py, double pz) {
    if (!gChest) return false;
    for (int h = 0; h < gChCnt; h++) if (getB(gChPos[h][0], gChPos[h][1], gChPos[h][2]) != XB_CHEST) return true;
    const double dx = px - (gChPos[0][0] + .5), dy = py - gChPos[0][1], dz = pz - (gChPos[0][2] + .5); (void)dy;
    if (dx * dx + dz * dz > 64.0) return true;
    if (gChWait) return SDL_GetTicks() - gChT0 > 2500;
    chestSyncNow(); return false;
}
static void chestClosed() {   // chamado por closeInv
    if (!gChest) return;
    chestSyncNow(); chestLid(false);
    if (chestGuest()) for (int h = 0; h < gChCnt; h++) gChests.erase(ckey(gChPos[h][0], gChPos[h][1], gChPos[h][2]));   // o convidado nao guarda o conteudo
    if (gChEnder && chestGuest()) gEndGuest = ChestInv();   // o convidado nao guarda o conteudo
    gChest = false; gChEnder = false; gChWait = false; gChCnt = 0;
}

// bau do ferreiro da vila (LootTableList.CHESTS_VILLAGE_BLACKSMITH, 1.12.2; sem selas/armaduras de cavalo/mudas): 3..8 rolagens em slots aleatorios
static void vilChestLoot(int x, int y, int z, int64_t seed) {
    struct L { int id, mn, mx, w; };
    static const L T[] = {{IT_DIAMOND, 1, 3, 3}, {IT_IRON, 1, 5, 10}, {IT_GOLD, 1, 3, 5}, {IT_BREAD, 1, 3, 15}, {IT_APPLE, 1, 3, 15}, {IT_IPICK, 1, 1, 5}, {IT_ISWORD, 1, 1, 5},
        {IT_A0 + 5, 1, 1, 5}, {IT_A0 + 4, 1, 1, 5}, {IT_A0 + 6, 1, 1, 5}, {IT_A0 + 7, 1, 1, 5}, {OBSIDIAN, 3, 7, 5}};
    int tot = 0; for (auto& t : T) tot += t.w;
    JR2 r(seed); ChestInv& ci = gChests[ckey(x, y, z)];
    for (int n = 3 + r.nextInt(6); n > 0; n--) {
        int k = r.nextInt(tot); const L* p = T; for (auto& t : T) { p = &t; if ((k -= t.w) < 0) break; }
        int s = r.nextInt(CHEST_SLOTS); for (int i = 0; i < CHEST_SLOTS && ci.s[s].n > 0; i++) s = (s + 1) % CHEST_SLOTS;
        ci.s[s].id = p->id; ci.s[s].n = p->mn + (p->mx > p->mn ? r.nextInt(p->mx - p->mn + 1) : 0);
    }
}
