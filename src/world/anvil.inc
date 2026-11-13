// ---------- BIGORNA (patchAnvil.1): ContainerRepair simplificado; estado local (nao e tile entity): SL_CHEST+0 esquerda, +1 direita, +2 resultado ----------
// reparo com material (1/4 da durabilidade por unidade), combinar 2 itens iguais (soma durabilidade + 12%, junta encantamentos), custo em niveis.
// Falta: renomear (sem campo de texto), custo acumulado por trabalho anterior, desgaste do bloco, livro encantado (lote 4).
static int gAnvPos[3] = {0, 0, 0}; static int gAnvUsed = 0; static bool gAnvMat = false;
static int anvTier(int id) {   // 0 madeira, 1 pedra, 2 ferro, 3 ouro, 4 diamante; -1 = sem material
    if (id == IT_WPICK) return 0; if (id == IT_SPICK) return 1; if (id >= IT_IPICK && id <= IT_DPICK) return 2 + id - IT_IPICK;
    if (id == IT_WSWORD) return 0; if (id >= IT_SSWORD && id <= IT_DSWORD) return 1 + id - IT_SSWORD;
    if (id >= IT_WAXE && id <= IT_DAXE) return id - IT_WAXE; if (id >= IT_WSHOVEL && id <= IT_DSHOVEL) return id - IT_WSHOVEL; if (id >= IT_WHOE && id <= IT_DHOE) return id - IT_WHOE;
    return -1;
}
static bool anvIsMat(const Slot& L, const Slot& R) {   // R repara L?
    if (!L.id || !R.id) return false;
    if (isArmor(L.id)) { static const int M[4] = {IT_LEATHER, IT_IRON, IT_GOLD, IT_DIAMOND}; const int m = (L.id - IT_A0) / 4; return m >= 0 && m < 4 && R.id == M[m]; }
    const int t = anvTier(L.id); if (t < 0) return false;
    if (t == 0) return R.id == PLANKS || (isPk(R.id) && PK[R.id - PK0].file.find("planks") != std::string::npos);
    if (t == 1) return R.id == COBBLE; if (t == 2) return R.id == IT_IRON; if (t == 3) return R.id == IT_GOLD; return R.id == IT_DIAMOND;
}
static int anvWeightMul(int eid) { const EnchType* t = enchType(eid); if (!t) return 1; return t->weight >= 10 ? 1 : t->weight >= 5 ? 2 : t->weight >= 2 ? 4 : 8; }   // comum 1, incomum 2, raro 4, muito raro 8
static void anvCompute() {
    gAnv[2] = Slot(); gAnvCost = 0; gAnvUsed = 0; gAnvMat = false; gAnvTooExp = false; gAnvOk = false;
    const Slot& L = gAnv[0]; const Slot& R = gAnv[1]; if (!L.id || !R.id || L.n != 1) return;
    if (!L.nbt.empty() && !enchIsNbt(L.nbt) && L.nbt.compare(0, 2, "~w") != 0) return;
    const int mx = itemMaxDur(L.id); Slot out = L; int cost = 0; bool changed = false;
    if (mx > 0 && anvIsMat(L, R)) {
        if (L.d <= 0) return; const int unit = std::max(1, mx / 4); int d = L.d, u = 0;
        while (d > 0 && u < R.n) { d = std::max(0, d - unit); u++; cost++; }
        out.d = d; gAnvUsed = u; gAnvMat = true; changed = u > 0;
    } else if (R.id == L.id && R.n == 1) {
        if (mx > 0 && (L.d > 0 || R.d > 0)) { const int rem = (mx - L.d) + (mx - R.d) + mx * 12 / 100; out.d = std::max(0, mx - rem); if (out.d != L.d) { cost += 2; changed = true; } }
        if (enchIsNbt(R.nbt)) {
            std::vector<std::pair<int, int>> v = enchList(out.nbt);
            for (const auto& pr : enchList(R.nbt)) {
                if (!enchCanApply(L.id, pr.first)) continue;
                int* cur = nullptr; bool conf = false; for (auto& q : v) { if (q.first == pr.first) cur = &q.second; else if (enchConflict(q.first, pr.first)) conf = true; }
                if (conf) { cost += 1; continue; }
                const EnchType* t = enchType(pr.first); const int c0 = cur ? *cur : 0;
                const int nl = c0 == pr.second ? std::min(t->maxL, c0 + 1) : std::max(c0, pr.second);
                if (nl == c0) continue;
                if (cur) *cur = nl; else v.push_back({pr.first, nl});
                cost += nl * anvWeightMul(pr.first); changed = true;
            }
            out.nbt = enchPack(v);
        }
    } else return;
    if (!changed) return;
    const int wl = enchWork(L.nbt), wr = R.id == L.id ? enchWork(R.nbt) : 0;   // patchAnvil.2: trabalho anterior
    cost += ((1 << wl) - 1) + ((1 << wr) - 1); out.nbt = enchWithWork(out.nbt, std::max(wl, wr) + 1);
    if (cost < 1) cost = 1; gAnvCost = cost;
    if (!creative && cost >= 40) { gAnvTooExp = true; return; }
    gAnvOk = creative || xpLevel(gXpTot) >= cost; out.n = 1; gAnv[2] = out;
}
static Slot* anvSlotRef(int k) { return k >= 0 && k < 3 ? &gAnv[k] : nullptr; }
static int anvOpenAt(int x, int y, int z) {   // 0 = nao e bigorna
    const int b = getB(x, y, z); if (!(b >= XB_ANVIL && b < XB_ANVIL + 4)) return 0;
    for (Slot& s : gAnv) s = Slot(); gAnvUi = gChest = true; gFurn = false; gTrashUi = false; gBrewUi = false; gChEnder = false; gChShulker = false; gChWait = false; gChRows = 1; gChCnt = 1;
    gAnvPos[0] = x; gAnvPos[1] = y; gAnvPos[2] = z; anvCompute(); return 2;
}
static bool anvFrame(double px, double py, double pz) {   // true = fechar
    (void)py; const int b = getB(gAnvPos[0], gAnvPos[1], gAnvPos[2]); if (!(b >= XB_ANVIL && b < XB_ANVIL + 4)) return true;
    const double dx = px - (gAnvPos[0] + .5), dz = pz - (gAnvPos[2] + .5); if (dx * dx + dz * dz > 64.0) return true;
    anvCompute(); return false;
}
static void anvClosed() {   // devolve o que sobrou nos slots de entrada (mochila; sem espaco, solta no bloco)
    for (int k = 0; k < 2; k++) { Slot s = gAnv[k]; gAnv[k] = Slot(); if (!s.id) continue; mergeInto(s, ordMainHot(true)); if (s.id && s.n > 0) dropFromBlock(gAnvPos[0], gAnvPos[1] + 1, gAnvPos[2], s.id, s.n); }
    gAnv[2] = Slot(); gAnvUi = gChest = false; gChRows = 3; gChCnt = 0; gChWait = false;
}
static void anvShift(Slot& s) {   // shift-click da mochila: esquerda, depois direita
    if (!s.id) return; if (!gAnv[0].id) { gAnv[0] = s; s = Slot(); } else if (!gAnv[1].id) { gAnv[1] = s; s = Slot(); }
    anvCompute();
}
static void anvTakeOut(int btn, bool shift) {   // slot de resultado: so retirar (custa niveis)
    (void)btn; Slot* o = &gAnv[2]; if (!o->id) return; if (!creative && !gAnvOk) return;
    Slot res = *o;
    if (shift) { Slot t = res; mergeInto(t, ordMainHot(true)); if (t.id) return; } else { if (cur.id) return; cur = res; }
    if (!creative) { float fr = 0.f; const int L = xpLevel(gXpTot, &fr), tg = std::max(0, L - gAnvCost); int base = 0; for (int k = 0; k < tg; k++) base += xpToNext(k); gXpTot = base + (int)(fr * xpToNext(tg)); }
    gAnv[0] = Slot(); if (gAnvMat) { gAnv[1].n -= gAnvUsed; if (gAnv[1].n <= 0) gAnv[1] = Slot(); } else gAnv[1] = Slot();
    sndPlay("random/anvil_use", .8f, .95f + (rand() % 100) / 1000.f); anvCompute();
}
