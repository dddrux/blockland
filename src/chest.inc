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
static bool propChestHere(int x, int y, int z);   // (abaixo) patchProp.6: Container na rede
static const int PCH_OFF = 1 << 20;   // patchProp.5c: baus VIRTUAIS de props Container: y = y real + PCH_OFF (mesma tabela gChests, mesmo save chests.dat)
static bool propChestHere(int x, int y, int z);   // definida no fim deste arquivo
static inline int chestType(int meta) { return (meta >> 2) & 31; }
static inline bool chestIsDbl(int meta) { const int t = chestType(meta); return t >= 19 && t <= 22; }               // metade de bau duplo (ver redstone.inc)
static inline int chestBase(int meta) { const int t = chestType(meta); return t >= 19 && t <= 22 ? (t - 19) >> 1 : t; }   // 0 normal, 1 armadilhado
static inline bool chestStores(int meta) { return chestBase(meta) <= 1 || (chestType(meta) >= 3 && chestType(meta) <= 18); }   // patchOpt.1: caixas de shulker (tipos 3..18) guardam 27 slots como o bau   // bau / bau armadilhado (simples ou duplo)
// BAU DO ENDER (type 2): UM inventario de 27 slots POR JOGADOR, igual em todos os baus do Ender do mundo (InventoryEnderChest do 1.12.2).
// Quem manda e o host (ou o jogo local): guarda um inventario por jogador (chave = nome; o do proprio host = "@host") em saves/<mundo>/ender.dat.
// O convidado pede (NP_ENDERREQ) ao abrir, recebe so o dele (NP_ENDER com alvo = seu id) e manda de volta (alvo 255) o que mudou.
static std::map<std::string, ChestInv> gEnderMap; static ChestInv gEndGuest;
static void chestsClear() { gChests.clear(); gEnderMap.clear(); gEndGuest = ChestInv(); }
// conteudo do bau em (x,y,z); create=true cria vazio. nullptr se la nao ha um bau que guarda itens.
static ChestInv* chestAt(int x, int y, int z, bool create = false) {
    if (y >= PCH_OFF) { if (!propChestHere(x, y - PCH_OFF, z)) return nullptr; }
    else if (getB(x, y, z) && (getB(x, y, z) == gDropId || getB(x, y, z) == gDispId || getB(x, y, z) == gHopId)) {}   // patchLogic.2: dropper/dispenser guardam itens (9 slots usados)
    else if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return nullptr;
    auto it = gChests.find(ckey(x, y, z)); if (it != gChests.end()) return &it->second;
    return create ? &gChests[ckey(x, y, z)] : nullptr;
}
// coloca n do item no bau (empilha nos iguais, depois vazios); devolve o que nao coube (para funil/dispensador no futuro)
static int chestAdd(ChestInv& c, int id, int n, const std::string& nbt = std::string()) {
    if (id <= 0) return n;
    for (int pass = 0; pass < 2 && n > 0; pass++) for (int i = 0; i < CHEST_SLOTS && n > 0; i++) {
        Slot& s = c.s[i]; if (pass == 0 ? (s.id == id && s.nbt == nbt && s.n > 0 && s.n < maxStack(id)) : (s.id == 0 || s.n <= 0)) {
            if (pass == 1) s = Slot(); const int put = std::min(n, maxStack(id) - s.n); s.id = id; s.nbt = nbt; s.n += put; n -= put; }
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
static void chestOnRemoved(int x, int y, int z, bool isChest) {
    if (isChest) { int px, pz; if (chestPartner(x, y, z, &px, &pz)) {   // quebrou uma metade: a outra vira bau simples (guarda o conteudo dela)
        const int m = getM(px, y, pz), nm = (m & 3) | (chestBase(m) << 2), sv = gRsLastMeta;
        setBM(px, y, pz, XB_CHEST, nm); gRsLastMeta = sv; netSendBlock(px, y, pz, XB_CHEST, nm); } }
    auto it = gChests.find(ckey(x, y, z)); if (it == gChests.end()) return;
    if (!(netSock >= 0 && !netHost)) for (const Slot& s : it->second.s) if (s.id && s.n > 0) {   // InventoryHelper: posicao aleatoria dentro da celula, velocidade pequena
        int left = s.n; while (left > 0) { const int k = std::min(left, 1 + rand() % 21); left -= k;
            spawnDrop(x + .1 + frand() * .8, y + .1 + frand() * .8, z + .1 + frand() * .8, (frand() - .5) * .1, frand() * .1 + .2, (frand() - .5) * .1, s.id, k, 10, 0, s.nbt); }
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
// ---------------- NBT do item bau: BlockEntityTag:{Items:[...]} em texto compacto ----------------
// Formato: "BET1|" + por pilha nao vazia "<slot>,<n>,<tam>:<chave do item><tam>:<nbt da pilha>" (chave = mesma do chests.dat; o nbt da pilha e recursivo: bau dentro de bau).
// Sem aspas/quebras de linha, entao cabe numa linha "x ..." dos saves. Vazio = bau sem itens (o item fica sem NBT, como no Java).
static std::string chestNbtPack(const ChestInv& c) {
    std::string o;
    for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; if (s.id <= 0 || s.n <= 0) continue;
        const std::string k = chestItemKey(s.id); o += std::to_string(i) + "," + std::to_string(s.n) + "," + std::to_string(k.size()) + ":" + k + std::to_string(s.nbt.size()) + ":" + s.nbt; }
    if (o.empty() || o.size() > 60000) return std::string();
    return "BET1|" + o;
}
static bool chestNbtUnpack(const std::string& t, ChestInv& out) {   // false = texto invalido ou marcador "?hash" (NBT que nunca chegou pela rede)
    out = ChestInv(); if (t.compare(0, 5, "BET1|") != 0) return false;
    size_t p = 5; auto num = [&](char end, long& v) { size_t e = t.find(end, p); if (e == std::string::npos || e == p || e - p > 9) return false; for (size_t q = p; q < e; q++) if (!isdigit((unsigned char)t[q])) return false; v = atol(t.c_str() + p); p = e + 1; return true; };
    while (p < t.size()) {
        long i, n, kl, nl; if (!num(',', i) || !num(',', n) || !num(':', kl) || kl < 0 || p + kl > t.size()) { out = ChestInv(); return false; }
        const std::string key = t.substr(p, kl); p += kl;
        if (!num(':', nl) || nl < 0 || p + nl > t.size()) { out = ChestInv(); return false; }
        const std::string sub = t.substr(p, nl); p += nl;
        const int id = chestItemFromKey(key); if (id && i >= 0 && i < CHEST_SLOTS && n > 0) { out.s[i].id = id; out.s[i].n = std::min((int)n, maxStack(id)); out.s[i].nbt = sub; }
    }
    return true;
}
static void chestApplyNbt(int x, int y, int z, const std::string& nbt) {   // bau recem colocado a partir do item com conteudo (so host/offline: o convidado nao guarda conteudo)
    if (nbt.empty() || (netSock >= 0 && !netHost)) return;
    ChestInv t; if (!chestNbtUnpack(nbt, t)) return; ChestInv* c = chestAt(x, y, z, true); if (!c) return; *c = t;
}
static std::string chestPickNbt(int x, int y, int z) {   // pick block com Ctrl: so baus que guardam (normal/armadilhado, cada metade do duplo e uma TileEntity); Ender e shulker ficam de fora
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return std::string();
    ChestInv* c = chestAt(x, y, z, false); return c ? chestNbtPack(*c) : std::string();
}
static void enderSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/ender.dat"; bool any = false; for (auto& kv : gEnderMap) if (!kv.second.empty()) any = true;
    if (!any) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fprintf(f, "ENDER1\n");
    for (auto& kv : gEnderMap) { if (kv.second.empty()) continue; fprintf(f, "p %s\n", kv.first.c_str());
        for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = kv.second.s[i]; if (s.id && s.n > 0) { fprintf(f, "s %d %d %s\n", i, s.n, chestItemKey(s.id).c_str()); if (!s.nbt.empty()) fprintf(f, "x %s\n", s.nbt.c_str()); } } }
    fclose(f);
}
static void enderLoad(const std::string& dir) {
    gEnderMap.clear(); FILE* f = fopen(("saves/" + dir + "/ender.dat").c_str(), "rb"); if (!f) return; static char ln[131072]; ChestInv* cur = nullptr; Slot* lastS = nullptr;   // linha "x <nbt>" = NBT da ultima pilha lida
    while (fgets(ln, sizeof ln, f)) {
        int i, n; char key[900];
        if (ln[0] == 'x' && ln[1] == ' ') { if (lastS) { lastS->nbt = ln + 2; while (!lastS->nbt.empty() && (lastS->nbt.back() == '\n' || lastS->nbt.back() == '\r')) lastS->nbt.pop_back(); } continue; }
        if (ln[0] == 'p' && ln[1] == ' ') { std::string nm = ln + 2; while (!nm.empty() && (nm.back() == '\n' || nm.back() == '\r')) nm.pop_back(); cur = &gEnderMap[nm]; }
        else if (ln[0] == 's' && cur && sscanf(ln, "s %d %d %899s", &i, &n, key) == 3 && i >= 0 && i < CHEST_SLOTS && n > 0) {
            const int id = chestItemFromKey(key); lastS = nullptr; if (id) { cur->s[i].id = id; cur->s[i].n = std::min(n, maxStack(id)); lastS = &cur->s[i]; } }
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
        for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = kv.second.s[i]; if (s.id && s.n > 0) { fprintf(f, "s %d %d %s\n", i, s.n, chestItemKey(s.id).c_str()); if (!s.nbt.empty()) fprintf(f, "x %s\n", s.nbt.c_str()); } }
    }
    fclose(f);
}
static void chestsLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    gChests.clear(); enderLoad(dir);
    FILE* f = fopen(("saves/" + dir + "/chests.dat").c_str(), "rb"); if (!f) return; static char ln[131072]; ChestInv* cur = nullptr; Slot* lastS = nullptr;   // linha "x <nbt>" = NBT da ultima pilha lida
    while (fgets(ln, sizeof ln, f)) {
        int x, y, z, i, n; char key[900];
        if (ln[0] == 'x' && ln[1] == ' ') { if (lastS) { lastS->nbt = ln + 2; while (!lastS->nbt.empty() && (lastS->nbt.back() == '\n' || lastS->nbt.back() == '\r')) lastS->nbt.pop_back(); } continue; }
        if (ln[0] == 'c' && sscanf(ln, "c %d %d %d", &x, &y, &z) == 3) cur = &gChests[ChestKey(x, y, z)];
        else if (ln[0] == 's' && cur && sscanf(ln, "s %d %d %899s", &i, &n, key) == 3 && i >= 0 && i < CHEST_SLOTS && n > 0) {
            const int id = chestItemFromKey(key); lastS = nullptr; if (id) { cur->s[i].id = id; cur->s[i].n = std::min(n, maxStack(id)); lastS = &cur->s[i]; }
        }
    }
    fclose(f);
}

// ---------------- tela do bau: abrir, fechar, sincronizar (Parte 2 e 3) ----------------
// Bau duplo = 54 slots: a metade de menor X (oeste) ou menor Z (norte) vem primeiro (InventoryLargeChest do 1.12.2).
// Multiplayer: o HOST e dono do conteudo. O convidado pede (NP_CHESTREQ), recebe NP_CHEST e, a cada mudanca, manda o estado novo; o host repassa.
static int gChPos[2][3], gChCnt = 0; static ChestInv gChSnap[2]; static bool gChLoaded[2]; static Uint32 gChT0 = 0;
static bool chestEq(const ChestInv& a, const ChestInv& b) { for (int i = 0; i < CHEST_SLOTS; i++) if (a.s[i].id != b.s[i].id || a.s[i].n != b.s[i].n || a.s[i].nbt != b.s[i].nbt) return false; return true; }
static int chestView(int x, int y, int z, int pos[2][3], ChestInv* inv[2]) {   // 0 = nao e bau que guarda; 1 simples; 2 duplo (superior primeiro)
    int px, pz, n = 1; const int a[3] = {x, y, z};
    if (chestPartner(x, y, z, &px, &pz)) { const int b[3] = {px, y, pz}; const bool pUp = px < x || pz < z; for (int i = 0; i < 3; i++) { pos[0][i] = pUp ? b[i] : a[i]; pos[1][i] = pUp ? a[i] : b[i]; } n = 2; }
    else for (int i = 0; i < 3; i++) pos[0][i] = a[i];
    for (int h = 0; h < n; h++) { inv[h] = chestAt(pos[h][0], pos[h][1], pos[h][2], true); if (!inv[h]) return 0; }
    return n;
}
static bool chestGuest() { return netSock >= 0 && !netHost && netConn; }
static Slot* chestSlotRef(int k) {
    if (gFurn) return furnSlotRef(k);   // fornalha reaproveita a tela do bau (furnace.inc)
    if (!gChest || k < 0 || k >= gChCnt * 27) return nullptr;
    if (gChEnder) return k < 27 ? &(chestGuest() ? gEndGuest : gEnderMap["@host"]).s[k] : nullptr;
    ChestInv* c = chestAt(gChPos[k / 27][0], gChPos[k / 27][1], gChPos[k / 27][2], false); return c ? &c->s[k % 27] : nullptr;
}
// cauda opcional (no FIM do pacote): u8 quantidade + (u8 slot, u32 hash do NBT) por pilha com NBT; o texto vai antes, em NP_NBT
static void chestNbtTailPut(NBuf& b, const ChestInv& c) {
    int cnt = 0; for (const Slot& s : c.s) if (s.id > 0 && s.n > 0 && !s.nbt.empty()) cnt++;
    b.put<uint8_t>((uint8_t)cnt); for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; if (s.id > 0 && s.n > 0 && !s.nbt.empty()) { b.put<uint8_t>((uint8_t)i); b.put<uint32_t>(nbtHash(s.nbt)); } }
}
static void chestNbtTailGet(NBuf& b, ChestInv& t) {
    const int cnt = b.get<uint8_t>(); for (int k = 0; k < cnt; k++) { const int i = b.get<uint8_t>(); const uint32_t h = b.get<uint32_t>(); if (i >= 0 && i < CHEST_SLOTS && t.s[i].id > 0) t.s[i].nbt = nbtResolve(h); }
}
static void netSendChest(int x, int y, int z, const ChestInv& c) {
    if (!netOn) return;
    for (const Slot& s : c.s) if (s.id > 0 && s.n > 0) netSendNbt(s.nbt);
    NBuf b = netMk(NP_CHEST); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z);
    for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
    chestNbtTailPut(b, c); netBroadcast(b);
}
static void furnNetRecv(NBuf& b, int x, int y, int z); static void furnNetReq(int x, int y, int z);   // furnace.inc: fornalha reaproveita NP_CHEST/NP_CHESTREQ
static void chestNetRecv(NBuf& b) {
    const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(); ChestInv t;
    if (gFurnId && getB(x, y, z) == gFurnId) { furnNetRecv(b, x, y, z); return; }
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>(); const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } }
    chestNbtTailGet(b, t);
    if (y >= PCH_OFF) { if (!propChestHere(x, y - PCH_OFF, z)) return; } else if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return;   // y >= PCH_OFF: Container (prop)
    int h = -1; if (gChest) for (int k = 0; k < gChCnt; k++) if (gChPos[k][0] == x && gChPos[k][1] == y && gChPos[k][2] == z) h = k;
    ChestInv* c = chestAt(x, y, z, netHost || h >= 0 || gChests.count(ckey(x, y, z)) > 0); if (!c) return;   // convidado so guarda o que abriu
    *c = t;
    if (h >= 0) { gChSnap[h] = t; if (!gChLoaded[h]) { gChLoaded[h] = true; bool all = true; for (int k = 0; k < gChCnt; k++) all = all && gChLoaded[k]; if (all) gChWait = false; } }
}
static void chestNetReq(NBuf& b) {   // so o host responde (a todos: quem nao tem o bau aberto ignora)
    if (!netHost) return;
    const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(); int pos[2][3]; ChestInv* inv[2];
    if (y >= PCH_OFF) { if (ChestInv* pc = chestAt(x, y, z, false)) netSendChest(x, y, z, *pc); return; }   // Container (prop)
    if (gFurnId && getB(x, y, z) == gFurnId) { furnNetReq(x, y, z); return; }
    const int n = chestView(x, y, z, pos, inv); for (int h = 0; h < n; h++) netSendChest(pos[h][0], pos[h][1], pos[h][2], *inv[h]);
}
static void netSendEnder(int target, const ChestInv& c) {   // target = id do convidado (host -> convidado) ou 255 (convidado -> host)
    if (!netOn) return;
    for (const Slot& s : c.s) if (s.id > 0 && s.n > 0) netSendNbt(s.nbt);
    NBuf b = netMk(NP_ENDER); b.put<uint8_t>((uint8_t)target);
    for (int i = 0; i < CHEST_SLOTS; i++) { const Slot& s = c.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
    chestNbtTailPut(b, c);
    if (netHost && target != 255) { for (auto& p : peers) if (p.id == target) netTo(p.a, b); } else netBroadcast(b);   // do host, so para o dono
}
static void enderNetRecv(NBuf& b, int sender) {
    const int target = b.get<uint8_t>(); ChestInv t;
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>(); const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } }
    chestNbtTailGet(b, t);
    if (netHost) { if (target == 255 && sender != netSelf) gEnderMap[netLabel(sender)] = t; return; }   // convidado -> host: guarda o inventario DELE
    if (target != netSelf) return;   // pacote repassado de/para outro jogador
    gEndGuest = t; if (gChest && gChEnder) { gChSnap[0] = t; gChLoaded[0] = true; gChWait = false; }
}
static void enderNetReq(int sender) { if (netHost && sender != netSelf) netSendEnder(sender, gEnderMap[netLabel(sender)]); }
static void chestLid(bool open) {   // tampa aberta = bit 7 do meta (vai pela rede como qualquer edicao de bloco)
    for (int h = 0; h < gChCnt; h++) { const int x = gChPos[h][0], y = gChPos[h][1], z = gChPos[h][2];
        if (y >= PCH_OFF || getB(x, y, z) != XB_CHEST) continue; const int m = getM(x, y, z), nm = open ? (m | 128) : (m & ~128);
        if (nm != m) { setBM(x, y, z, XB_CHEST, nm); netSendBlock(x, y, z, XB_CHEST, nm); } }
}
// 0 = nao e bau; 1 = abriu (o chamador abre a tela); 2 = bloqueado por bloco em cima (consome o clique sem abrir)
static int chestOpenAt(int x, int y, int z) {
    if (getB(x, y, z) == XB_CHEST && chestType(getM(x, y, z)) == 2) {   // Bau do Ender: inventario pessoal
        if (rsFullCube(getB(x, y + 1, z))) return 2;
        gChest = true; gChEnder = true; gChShulker = false; gChCnt = 1; gChRows = 3; gChPos[0][0] = x; gChPos[0][1] = y; gChPos[0][2] = z;
        if (chestGuest()) gEndGuest = ChestInv();
        gChSnap[0] = chestGuest() ? gEndGuest : gEnderMap["@host"]; gChLoaded[0] = !chestGuest(); gChWait = chestGuest(); gChT0 = SDL_GetTicks();
        if (gChWait) { NBuf b = netMk(NP_ENDERREQ); netBroadcast(b); }
        chestLid(true); sndEvtAt("chest.open", x + .5, y + .5, z + .5); return 1;
    }
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return 0;
    int pos[2][3]; ChestInv* inv[2]; const int n = chestView(x, y, z, pos, inv); if (!n) return 0;
    for (int h = 0; h < n; h++) if (rsFullCube(getB(pos[h][0], pos[h][1] + 1, pos[h][2]))) return 2;
    gChest = true; gChEnder = false; gChShulker = chestType(getM(x, y, z)) >= 3; gChCnt = n; gChRows = 3 * n; memcpy(gChPos, pos, sizeof pos);
    for (int h = 0; h < n; h++) { gChSnap[h] = *inv[h]; gChLoaded[h] = !chestGuest(); }
    gChWait = chestGuest(); gChT0 = SDL_GetTicks();
    if (gChWait) { NBuf b = netMk(NP_CHESTREQ); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z); netBroadcast(b); }
    chestLid(true); sndEvtAt("chest.open", x + .5, y + .5, z + .5); return 1;
}
static void chestSyncNow() {   // manda por rede o que mudou desde o ultimo envio
    if (gChWait) return;
    if (gChEnder) { ChestInv& e = chestGuest() ? gEndGuest : gEnderMap["@host"]; if (!chestEq(e, gChSnap[0])) { gChSnap[0] = e; if (chestGuest()) netSendEnder(255, e); } return; }
    for (int h = 0; h < gChCnt; h++) { ChestInv* c = chestAt(gChPos[h][0], gChPos[h][1], gChPos[h][2], false); if (!c || chestEq(*c, gChSnap[h])) continue; gChSnap[h] = *c; netSendChest(gChPos[h][0], gChPos[h][1], gChPos[h][2], *c); }
}
// a cada quadro com a tela aberta; true = fechar (bau sumiu, jogador longe (8 blocos, como o canInteractWith), convidado sem resposta)
static void chestSignalChanged(int x, int y, int z);
static bool furnFrame(double px, double py, double pz);   // furnace.inc
static bool chestFrame(double px, double py, double pz) {
    if (!gChest) return false;
    if (gFurn) return furnFrame(px, py, pz);
    for (int h = 0; h < gChCnt; h++) { if (gChPos[h][1] >= PCH_OFF) { if (!propChestHere(gChPos[h][0], gChPos[h][1] - PCH_OFF, gChPos[h][2])) return true; } else if (getB(gChPos[h][0], gChPos[h][1], gChPos[h][2]) != XB_CHEST) return true; }
    const double dx = px - (gChPos[0][0] + .5), dy = py - gChPos[0][1], dz = pz - (gChPos[0][2] + .5); (void)dy;
    if (dx * dx + dz * dz > 64.0) return true;
    if (gChWait) return SDL_GetTicks() - gChT0 > 2500;
    chestSyncNow(); if (!chestGuest()) for (int h = 0; h < gChCnt; h++) chestSignalChanged(gChPos[h][0], gChPos[h][1], gChPos[h][2]); return false;
}
static int chestSignal(int x, int y, int z) {   // saida do comparador (1.12.2 Container.calcRedstoneFromInventory): 0 vazio; senao floor(1 + enchimento * 14); bau duplo soma as duas metades
    if (getB(x, y, z) != XB_CHEST || !chestStores(getM(x, y, z))) return -1; ChestInv* c = chestAt(x, y, z); double sum = 0; int slots = CHEST_SLOTS;
    auto add = [&](ChestInv* q) { for (const Slot& s : q->s) if (s.id > 0 && s.n > 0) sum += (double)s.n / maxStack(s.id); };
    if (c) add(c); int px, pz; if (chestPartner(x, y, z, &px, &pz)) { slots += CHEST_SLOTS; if (ChestInv* p = chestAt(px, y, pz)) add(p); }
    return sum <= 0 ? 0 : (int)std::floor(1 + sum / slots * 14);
}
static void chestSignalChanged(int x, int y, int z) {   // conteudo mudou: comparadores vizinhos reavaliam (host/offline; o convidado nao guarda o conteudo)
    if (y >= PCH_OFF) return;
    static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& d : D) if (getB(x + d[0], y, z + d[1]) == XB_COMP) rsCompCheck(x + d[0], y, z + d[1]);
}
static void furnClosed();   // furnace.inc
static void chestClosed() {   // chamado por closeInv
    if (!gChest) return;
    if (gFurn) { furnClosed(); return; }
    chestSyncNow(); chestLid(false); sndEvtAt("chest.close", gChPos[0][0] + .5, gChPos[0][1] + .5, gChPos[0][2] + .5); if (!chestGuest()) for (int h = 0; h < gChCnt; h++) chestSignalChanged(gChPos[h][0], gChPos[h][1], gChPos[h][2]);
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

// ---------- receitas que dependem do pack (itens com id dinamico: escada, laje, cerca, porta, bau, tocha...) ----------
// Recriadas quando o pack muda (assinatura = ids dos itens de saida), pois gRsDrop/gXbDrop so existem depois de loadCreativeCfg().
static std::vector<Recipe>& packRecipes() {
    static std::vector<Recipe> R; static long sig = -1;
    auto rd = [&](int xb, int v) { return (xb >= XB_DUST && xb < XB_END) ? gRsDrop[xb - XB_DUST][v & 31] : 0; };
    const int furn = gPkByFile.count("block/furnace") ? gPkByFile["block/furnace"] : 0;
    const int tor = gXbDrop[XB_TORCH - XB0], rail = gXbDrop[XB_RAIL - XB0];
    const long s = (long)rd(XB_STAIR, 0) * 31 + rd(XB_CHEST, 0) * 17 + rd(XB_DOOR, 1) * 13 + tor * 7 + furn + rail * 3 + rd(XB_LADDER, 0) * 5 + boatItemId(0) * 11 + rd(XB_DOOR, 2) * 19 + rd(XB_BED, 1) * 23 + std::max(0, pkFind("item/clock_00")) * 29 + std::max(0, pkFind("item/compass_00")) * 37;
    if (s == sig) return R;
    sig = s; R.clear();
    auto sh = [&](std::vector<std::string> rows, std::initializer_list<std::pair<char, int>> k, int out, int n = 1) {
        if (!out) return; Recipe r; r.rows = rows; for (auto& p : k) r.k[p.first] = p.second; r.out = out; r.n = n; R.push_back(r); };
    const int P = PLANKS, S = IT_STICK;
    { const int clk = std::max(0, pkFind("item/clock_00")), cmp = std::max(0, pkFind("item/compass_00"));   // patchClock.2: relogio (4 ouro + redstone) e bussola (4 ferro + redstone)
      sh({" G ", "GRG", " G "}, {{'G', IT_GOLD}, {'R', IT_REDSTONE}}, clk); sh({" I ", "IRI", " I "}, {{'I', IT_IRON}, {'R', IT_REDSTONE}}, cmp); }
    // escadas e lajes (Java 1.8.9: 6 blocos -> 4 escadas; 3 blocos -> 6 lajes)
    const struct { int var, blk; } ST[] = {{0, PLANKS}, {1, COBBLE}, {7, SANDSTONE}, {8, STONEBRICK}};
    for (auto& t : ST) { sh({"B  ", "BB ", "BBB"}, {{'B', t.blk}}, rd(XB_STAIR, t.var), 4); sh({"BBB"}, {{'B', t.blk}}, rd(XB_SLAB, t.var), 6); }
    sh({"BBB"}, {{'B', STONE}}, rd(XB_SLAB, 10), 6);
    {   // patchBlk.1: escadas e lajes dos demais materiais da 1.12.2 (madeiras, tijolos, tijolos do Nether, quartzo, arenito vermelho, purpur)
        const auto pk1 = [&](std::initializer_list<const char*> fs) { for (const char* f : fs) { const int i = pkFind((std::string("block/") + f).c_str()); if (i) return i; } return 0; };
        const struct { int var, blk; } MORE[] = {{2, pk1({"spruce_planks"})}, {3, pk1({"birch_planks"})}, {4, pk1({"jungle_planks"})}, {5, pk1({"acacia_planks"})}, {6, pk1({"dark_oak_planks"})},
            {9, BRICKS}, {11, pk1({"nether_bricks", "nether_brick"})}, {12, pk1({"quartz_block"})}, {13, pk1({"red_sandstone", "red_sandstone_normal"})}, {14, pk1({"purpur_block"})}};
        for (auto& t : MORE) if (t.blk) { sh({"B  ", "BB ", "BBB"}, {{'B', t.blk}}, rd(XB_STAIR, t.var), 4); sh({"BBB"}, {{'B', t.blk}}, rd(XB_SLAB, t.var), 6); }
    }
    // cerca, portao, porta, alcapao, bau, placa de pressao, botao
    sh({"PSP", "PSP"}, {{'P', P}, {'S', S}}, rd(XB_FENCE, 0), 3);
    sh({"SPS", "SPS"}, {{'P', P}, {'S', S}}, rd(XB_GATE, 0));
    sh({"PP", "PP", "PP"}, {{'P', P}}, rd(XB_DOOR, 1), 3);
    sh({"II", "II", "II"}, {{'I', IT_IRON}}, rd(XB_DOOR, 0), 3);
    sh({"PPP", "PPP"}, {{'P', P}}, rd(XB_TRAP, 1), 2);
    sh({"PPP", "P P", "PPP"}, {{'P', P}}, rd(XB_CHEST, 0));
    // patchCraft.1: barcos (5 tabuas da madeira), bau e cerca/portao das outras madeiras (tabuas do pack: nome 1.13 ou 1.12)
    sh({"P P", "PPP"}, {{'P', P}}, boatItemId(0));
    static const char* WD[5] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (int w = 0; w < 5; w++) {
        const std::string a = std::string("block/") + WD[w] + "_planks", b = std::string("block/planks_") + (w == 4 ? "big_oak" : WD[w]);
        auto f = gPkByFile.find(a); if (f == gPkByFile.end()) f = gPkByFile.find(b); if (f == gPkByFile.end()) continue;
        const int Q = f->second;
        sh({"Q Q", "QQQ"}, {{'Q', Q}}, boatItemId(w + 1));
        sh({"QQQ", "Q Q", "QQQ"}, {{'Q', Q}}, rd(XB_CHEST, 0));
        sh({"QSQ", "QSQ"}, {{'Q', Q}, {'S', S}}, rd(XB_FENCE, w + 1), 3);
        sh({"SQS", "SQS"}, {{'Q', Q}, {'S', S}}, rd(XB_GATE, w + 1));
        sh({"QQ", "QQ", "QQ"}, {{'Q', Q}}, rd(XB_DOOR, w + 2), 3);   // patchCraft.2: porta de cada madeira (variantes 2..6)
    }
    {   // patchCraft.2: cama de cada cor = 3 la da cor + 3 tabuas (variante = ordem dos corantes; a branca ja existe acima)
        static const char* CL[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
        static const char* C12[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "silver", "cyan", "purple", "blue", "brown", "green", "red", "black"};
        for (int c = 1; c < 16; c++) {
            const int wl = pkFind((std::string("block/") + CL[c] + "_wool").c_str()), w2 = wl ? wl : pkFind((std::string("block/wool_colored_") + C12[c]).c_str());
            if (w2) sh({"WWW", "PPP"}, {{'W', w2}, {'P', P}}, rd(XB_BED, c));
        }
    }
    sh({"PP"}, {{'P', P}}, rd(XB_PLATE, 1));
    sh({"SS"}, {{'S', STONE}}, rd(XB_PLATE, 0));
    sh({"P"}, {{'P', P}}, rd(XB_BTN, 1));
    sh({"S"}, {{'S', STONE}}, rd(XB_BTN, 0));
    // tocha, escada de mao, trilho, fornalha, cama
    sh({"C", "S"}, {{'C', IT_COAL}, {'S', S}}, tor, 4);
    sh({"S S", "SSS", "S S"}, {{'S', S}}, rd(XB_LADDER, 0), 3);
    sh({"I I", "ISI", "I I"}, {{'I', IT_IRON}, {'S', S}}, rail, 16);
    sh({"CCC", "C C", "CCC"}, {{'C', COBBLE}}, furn);
    sh({"WWW", "PPP"}, {{'W', WOOL}, {'P', P}}, rd(XB_BED, 0));
    // painel de vidro, barras de ferro, muro, carpete, placa
    const int glass = gPkByFile.count("block/glass") ? gPkByFile["block/glass"] : 0;
    if (glass) sh({"GGG", "GGG"}, {{'G', glass}}, rd(XB_PANE, 1), 16);
    sh({"III", "III"}, {{'I', IT_IRON}}, rd(XB_PANE, 0), 16);
    sh({"CCC", "CCC"}, {{'C', COBBLE}}, rd(XB_WALL, 0), 6);
    sh({"WW"}, {{'W', WOOL}}, rd(XB_CARPET, 0), 3);
    sh({"PPP", "PPP", " S "}, {{'P', P}, {'S', S}}, rd(XB_SIGN, 0), 3);
    return R;
}

// ---------------- patchProp.5c: props Container (Decocraft) guardam itens na tela do bau (27 slots), so locais (sem rede) ----------------
static void propChestKey(const PropEnt& e, int* k) { k[0] = (int)floorf(e.x); k[1] = (int)floorf(e.y + .01f) + PCH_OFF; k[2] = (int)floorf(e.z); }
static bool propChestHere(int x, int y, int z) {
    for (const PropEnt& e : gProps) { int pk = 0; const PropItem* it = propItem(e.id, &pk); if (!it || it->beh != "Container") continue; int k[3]; propChestKey(e, k); if (k[0] == x && k[1] - PCH_OFF == y && k[2] == z) return true; }
    return false;
}
static bool propContainerOpen(int i) {   // abre a tela do bau para o prop i; true = o chamador deve abrir o inventario
    if (i < 0 || i >= (int)gProps.size()) return false; int k[3]; propChestKey(gProps[i], k);
    ChestInv* inv = chestAt(k[0], k[1], k[2], true); if (!inv) return false;
    gChest = true; gChEnder = false; gFurn = false; gChCnt = 1; gChRows = 3; for (int a = 0; a < 3; a++) gChPos[0][a] = k[a];
    gChSnap[0] = *inv; gChLoaded[0] = !chestGuest(); gChWait = chestGuest(); gChT0 = SDL_GetTicks();   // convidado: o conteudo mora no host (pede e espera, como num bau)
    if (gChWait) { NBuf b = netMk(NP_CHESTREQ); b.put<int32_t>(k[0]); b.put<int32_t>(k[1]); b.put<int32_t>(k[2]); netBroadcast(b); }
    return true;
}
static void propContainerRemoved(const PropEnt& e) {   // prop removido: solta o conteudo e apaga o bau virtual
    int k[3]; propChestKey(e, k); auto it = gChests.find(ckey(k[0], k[1], k[2])); if (it == gChests.end()) return;
    if (!chestGuest()) for (const Slot& s : it->second.s) if (s.id && s.n > 0) { int left = s.n; while (left > 0) { const int q = std::min(left, 1 + rand() % 21); left -= q;
        spawnDrop(e.x + (frand() - .5) * .4, e.y + .5, e.z + (frand() - .5) * .4, (frand() - .5) * .1, frand() * .1 + .2, (frand() - .5) * .1, s.id, q, 10, 0, s.nbt); } }
    gChests.erase(it);
}
