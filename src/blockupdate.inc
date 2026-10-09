// ================= atualizacao de vizinhos e ticks de bloco =================
// Equivalente simples do neighborChanged / onBlockAdded / updateTick do Minecraft. Cada id pode registrar ganchos (blockHooksSet):
//   neighbor(x,y,z, fx,fy,fz)  um vizinho (fx,fy,fz) do bloco (x,y,z) mudou -> o bloco reavalia (suporte, redstone, gravidade, portas...)
//   added(x,y,z)               o bloco acabou de ser colocado/trocado na celula
//   tick(x,y,z)                tick agendado com scheduleTick (delay em ticks de 20 TPS; redstone, areia, agua...)
// setBM() enfileira a celula alterada; a fila e drenada NA HORA (quem chamou ve o mundo ja estabilizado) mas em largura, sem recursao: uma
// cascata longa (coluna de tochas, areia empilhada) nao estoura a pilha, e ha um limite de passos contra laços infinitos entre dois blocos.
// Ganchos NAO rodam para edicoes vindas da rede (gNoBlockHooks) nem durante a geracao do mundo (gen escreve direto no chunk).
typedef void (*BlockNeighborFn)(int x, int y, int z, int fx, int fy, int fz);
typedef void (*BlockAddedFn)(int x, int y, int z);
typedef void (*BlockTickFn)(int x, int y, int z);
struct BlockHooks { BlockNeighborFn neighbor = nullptr; BlockAddedFn added = nullptr; BlockTickFn tick = nullptr; };
static uint8_t gHookSel[65536];            // 0 = sem ganchos; senao indice em gHooks
static std::vector<BlockHooks> gHooks(1);
static void blockHooksSet(int id, const BlockHooks& h) {
    if ((unsigned)id >= 65536u) return;
    gHooks.push_back(h); gHookSel[id] = (uint8_t)(gHooks.size() - 1);
}
static void blockHooksSetRange(int id0, int id1, const BlockHooks& h) {   // [id0, id1)
    gHooks.push_back(h); const uint8_t i = (uint8_t)(gHooks.size() - 1);
    for (int id = id0; id < id1 && id < 65536; id++) gHookSel[id] = i;
}

struct UpdItem { int x, y, z; bool added; };
static std::vector<UpdItem> gUpdQ; static size_t gUpdHead = 0; static bool gUpdBusy = false;
static long long gUpdDropped = 0;   // vezes que o limite de seguranca cortou uma cascata (diagnostico/teste)
static const int UPD_LIMIT = 200000;
static void drainBlockUpdates() {
    if (gUpdBusy) return;   // ja drenando (chamada aninhada vinda de um gancho): o laco externo continua depois
    gUpdBusy = true;
    int steps = 0;
    while (gUpdHead < gUpdQ.size()) {
        if (++steps > UPD_LIMIT) { gUpdDropped++; gUpdQ.clear(); gUpdHead = 0; break; }
        const UpdItem u = gUpdQ[gUpdHead++];
        if (u.added) { const int id = getB(u.x, u.y, u.z); const uint8_t hs = gHookSel[id]; if (hs && gHooks[hs].added) gHooks[hs].added(u.x, u.y, u.z); }
        static const int D6[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (int d = 0; d < 6; d++) {
            const int nx = u.x + D6[d][0], ny = u.y + D6[d][1], nz = u.z + D6[d][2];
            if (ny < 0 || ny >= CH) continue;
            const int nid = getB(nx, ny, nz); const uint8_t hs = gHookSel[nid];
            if (hs && gHooks[hs].neighbor) gHooks[hs].neighbor(nx, ny, nz, u.x, u.y, u.z);
        }
    }
    gUpdQ.clear(); gUpdHead = 0; gUpdBusy = false;
}
static void blockChangedHook(int x, int y, int z, int oldId, int newId) {
    // atalho barato: se nem o proprio bloco novo nem nenhum vizinho tem gancho, nao ha nada a avisar (caso comum: agua fluindo, terreno)
    static const int D6[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const bool added = oldId != newId && gHookSel[newId] && gHooks[gHookSel[newId]].added;
    bool any = added;
    for (int d = 0; d < 6 && !any; d++) { const int ny = y + D6[d][1]; if (ny >= 0 && ny < CH && gHookSel[getB(x + D6[d][0], ny, z + D6[d][2])]) any = true; }
    if (!any) return;
    gUpdQ.push_back({x, y, z, oldId != newId});
    drainBlockUpdates();
}

// ---- ticks agendados (20 TPS). Rodam so em jogo local ou no host: o convidado recebe o resultado como edicoes de bloco ----
static long long gWorldTick = 0;
struct SchedTick { long long due; int x, y, z; };
static std::vector<SchedTick> gTicks; static std::unordered_set<int64_t> gTickSet;
static void scheduleTick(int x, int y, int z, int delay) {
    if (delay < 1) delay = 1;
    if (gTickSet.insert(k3(x, y, z)).second) gTicks.push_back({gWorldTick + delay, x, y, z});
}
static bool tickPending(int x, int y, int z) { return gTickSet.count(k3(x, y, z)) != 0; }
static void runWorldTick() {
    if (netOn && !netHost) return;
    ++gWorldTick;
    if (gTicks.empty()) return;
    std::vector<SchedTick> due, keep;
    for (const SchedTick& t : gTicks) (t.due <= gWorldTick ? due : keep).push_back(t);
    gTicks.swap(keep);
    for (const SchedTick& t : due) gTickSet.erase(k3(t.x, t.y, t.z));
    for (const SchedTick& t : due) {
        if (!getC(fdiv(t.x, CW), fdiv(t.z, CW))) continue;
        const uint8_t hs = gHookSel[getB(t.x, t.y, t.z)];
        if (hs && gHooks[hs].tick) gHooks[hs].tick(t.x, t.y, t.z);   // so o bloco que esta la AGORA reage (o agendado pode ter sido trocado)
    }
}
static void resetBlockUpdates() { gUpdQ.clear(); gUpdHead = 0; gUpdBusy = false; gTicks.clear(); gTickSet.clear(); }

// ---- tocha / alavanca / plantas: caem quando perdem o suporte (substitui as chamadas manuais a xbFixAround) ----
static void xbNeighbor(int x, int y, int z, int fx, int fy, int fz) {
    const int b = getB(x, y, z);
    if (!isXB(b) || xbSolid(b)) return;
    if (xbSupported(b, x, y, z)) { if (xbIsRail(b)) railNeighbor(x, y, z, b); else if (b == XB_DUST) rsDustFix(x, y, z); else if (b == XB_FENCE || b == XB_PANE || b == XB_WALL || b == XB_CPANE) rsConnFix(x, y, z, b); return; }
    spawnBreak(x, y, z, b);
    setB(x, y, z, AIR); netSendBlock(x, y, z, AIR);   // o setB avisa os vizinhos: a cascata (planta alta, tocha ao lado) continua sozinha
    if (!creative) blockDrops(x, y, z, b);
}
static const struct BlockUpdateInit {
    BlockUpdateInit() {
        BlockHooks h; h.neighbor = xbNeighbor;
        blockHooksSetRange(XB_TORCH, XB_TORCH + 5, h); blockHooksSetRange(XB_RTORCH, XB_RTORCH + 5, h);
        blockHooksSetRange(XB_LEVER, XB_LEVER + 24, h);
        blockHooksSetRange(XB_PLANT, XB_PLANT + 24, h);
        blockHooksSetRange(XB_SAPLING, XB_SAPLING + 6, h);
        blockHooksSetRange(XB_RAIL, XB_RAIL + 4, h);
        blockHooksSetRange(XB_DUST, XB_END, h); blockHooksSetRange(XB_BANNER, XB_BANNER + 1, h); blockHooksSetRange(XB_WBANNER, XB_WBANNER + 1, h);
    }
} blockUpdateInit;
