// ---------- modulo treechop (port de HT's TreeChop 1.12.2, versao simplificada) ----------
// Quebrar um tronco derruba a arvore inteira (troncos conectados + folhas ao redor). Config: modulo_treechop.cfg
//   treechop_on=1  treechop_axe=1 (exige machado)  treechop_sneak=1 (agachado = nao derruba)  treechop_max=256  treechop_wear=1 (desgasta a ferramenta por tronco)
// Nao portado: os 13 blocos "tronco serrado" (estados) e o corte gradual por golpe do mod original; aqui a arvore cai de uma vez.
namespace mod_treechop {
    static bool gOn = true, gAxe = true, gSneak = true, gWear = true; static int gMax = 256; static bool gSneaking = false, gBusy = false;
    static void load() { api::Cfg c("treechop"); gOn = c.getInt("treechop_on", 1); gAxe = c.getInt("treechop_axe", 1); gSneak = c.getInt("treechop_sneak", 1); gWear = c.getInt("treechop_wear", 1); gMax = std::max(1, std::min(2048, c.getInt("treechop_max", 256))); }
    static void save() { api::Cfg c("treechop"); c.setInt("treechop_on", gOn); c.setInt("treechop_axe", gAxe); c.setInt("treechop_sneak", gSneak); c.setInt("treechop_wear", gWear); c.setInt("treechop_max", gMax); c.save(); }
    static bool heldIsAxe() {
        const Slot& s = inv[gModHeldSel]; if (!s.id || !isPk(s.id)) return false;
        const std::string& f = PK[s.id - PK0].file; return f.size() >= 4 && f.compare(f.size() - 4, 4, "_axe") == 0;   // item/<material>_axe
    }
    static void onTick(Phys& P) { gSneaking = P.sneak; }
    static void onBlockBreak(int x, int y, int z, int id) {
        if (!gOn || gBusy || !isLogB(id) || gGm == 3 || (gSneak && gSneaking)) return;
        if (gAxe && !heldIsAxe()) return;
        // flood fill 26-vizinhos, so ao redor/acima do tronco quebrado (nao desce abaixo do bloco inicial)
        std::vector<std::array<int, 3>> q{{x, y, z}}; std::set<long long> seen; auto K = [](int a, int b, int c) { return ((long long)(a + 30000000) << 40) ^ ((long long)(b + 512) << 24) ^ (long long)(c + 30000000); };
        seen.insert(K(x, y, z)); int leaves = 0;
        for (size_t i = 0; i < q.size() && (int)q.size() <= gMax; i++) {
            const auto p = q[i];
            for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
                if (!dx && !dy && !dz) continue; const int nx = p[0] + dx, ny = p[1] + dy, nz = p[2] + dz; if (ny < y || ny > 255) continue;
                if (!seen.insert(K(nx, ny, nz)).second) continue; const int b = getB(nx, ny, nz);
                if (isLogB(b)) q.push_back({nx, ny, nz}); else if (isLeafB(b)) leaves++;
            }
        }
        if (leaves < 3 || q.size() < 2) return;   // sem folhas = construcao, nao arvore
        gBusy = true;
        for (size_t i = 1; i < q.size(); i++) {   // [0] e o bloco que o jogo ja esta quebrando
            const int bx = q[i][0], by = q[i][1], bz = q[i][2], b = getB(bx, by, bz); if (!isLogB(b)) continue;
            spawnBreak(bx, by, bz, b); setB(bx, by, bz, AIR); netSendBlock(bx, by, bz, AIR); keepLeaves.erase(k3(bx, by, bz)); leafNeighbors(bx, by, bz);
            if (!creative) { blockDrops(bx, by, bz, b); if (gWear && inv[gModHeldSel].id) { toolWear(inv[gModHeldSel]); if (!inv[gModHeldSel].id) break; } }
        }
        gBusy = false;
    }
    static bool cmdTC(const std::vector<std::string>& a, Phys&) {
        if (a.empty()) { gOn = !gOn; save(); api::chat::ok(std::string("TreeChop: ") + (gOn ? "ligado" : "desligado")); return true; }
        if (a[0] == "axe") { gAxe = !gAxe; save(); api::chat::ok(std::string("TreeChop exige machado: ") + (gAxe ? "sim" : "nao")); return true; }
        if (a[0] == "sneak") { gSneak = !gSneak; save(); api::chat::ok(std::string("TreeChop agachado cancela: ") + (gSneak ? "sim" : "nao")); return true; }
        if (a[0] == "max" && a.size() > 1) { gMax = std::max(1, std::min(2048, api::arg::num(a, 1, gMax))); save(); api::chat::ok("TreeChop max = " + std::to_string(gMax)); return true; }
        return false;
    }
    static const api::Cmd CMDS[] = { {"treechop", "/treechop [axe|sneak|max <n>]", cmdTC} };
    static bool onCommand(const std::string& cmd, const std::vector<std::string>& args, Phys& P) { return api::dispatch(CMDS, sizeof CMDS / sizeof *CMDS, cmd, args, P); }
    static GameModule mk() { GameModule m; m.name = "treechop"; m.onInit = load; m.onTick = onTick; m.onCommand = onCommand; m.onBlockBreak = onBlockBreak; return m; }
}
GAME_MODULE_REGISTER_FN(treechop, mod_treechop::mk);
