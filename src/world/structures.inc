// ---------- ESTRUTURAS ESPALHADAS (patchStruct.1): MapGenScatteredFeature 1.12.2 (regiao de 32 chunks, minimo 8, sal 14357617) ----------
// Templo do deserto (21x21), iglu (tundra), cabana da bruxa (pantano). Escrita por chunk: cada chunk pinta so a sua fatia (como stronghold.inc).
// Simplificacoes: sem rotacao aleatoria (porta ao sul); altura = media do terreno em 5 pontos; templo da selva = lote seguinte.
struct ScCk { Chunk* c; int cx0, cz0, bx, bz, gy; };
static ScCk gSc;
static std::vector<std::array<int, 4>> gScChests;   // x, y, z, tabela (0 deserto, 1 iglu, 2 cabana)
static int scPk(const char* a, const char* b, int fb) { int v = pkFind(a); if (!v && b) v = pkFind(b); return v ? v : fb; }
static void scPut(int x, int y, int z, int id, int meta = 0) {   // x,z relativos ao canto da estrutura; y relativo ao chao medio
    const int wx = gSc.bx + x, wz = gSc.bz + z, wy = gSc.gy + y, lx = wx - gSc.cx0, lz = wz - gSc.cz0;
    if (lx < 0 || lx > 15 || lz < 0 || lz > 15 || wy < 1 || wy > CH - 2) return;
    const int i = idx(lx, wy, lz); if (gSc.c->b[i] == BEDROCK) return;
    gSc.c->b[i] = (BT)id; gSc.c->m[i] = (uint8_t)meta;
    if (id == XB_CHEST) gScChests.push_back({wx, wy, wz, 0});
}
static void scFill(int x0, int y0, int z0, int x1, int y1, int z1, int id, int meta = 0) {
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
        const int wx = gSc.bx + x, wz = gSc.bz + z; if (wx < gSc.cx0 || wx > gSc.cx0 + 15 || wz < gSc.cz0 || wz > gSc.cz0 + 15) continue; scPut(x, y, z, id, meta); }
}
static void scChestTable(int x, int y, int z, int table) { scPut(x, y, z, XB_CHEST); if (!gScChests.empty() && gScChests.back()[0] == gSc.bx + x) gScChests.back()[3] = table; }
static void scDesertTemple() {
    const int SS = SANDSTONE, W = 21, D = 21, TNTB = scPk("block/tnt", "block/tnt_side", SS), CH_S = scPk("block/sandstone_carved", "block/chiseled_sandstone", SS);
    const int ORA = scPk("block/hardened_clay_stained_orange", "block/orange_terracotta", SS), BLU = scPk("block/hardened_clay_stained_blue", "block/blue_terracotta", SS);
    scFill(0, -4, 0, W - 1, 0, D - 1, SS);
    for (int i = 1; i <= 9; i++) { scFill(i, i, i, W - 1 - i, i, D - 1 - i, SS); scFill(i + 1, i, i + 1, W - 2 - i, i, D - 2 - i, AIR); }   // anel de arenito por nivel, interior oco
    scPut(10, 9, 10, SS);   // fecha o topo
    for (int cx : {0, 16}) for (int cz : {0, 16}) { scFill(cx, 0, cz, cx + 4, 9, cz + 4, SS); scFill(cx + 1, 1, cz + 1, cx + 3, 8, cz + 3, AIR); scFill(cx + 2, 2, cz + (cz ? 0 : 4), cx + 2, 3, cz + (cz ? 0 : 4), AIR); scFill(cx, 10, cz, cx + 4, 10, cz + 4, SS); }   // 4 torres
    scFill(9, 1, 0, 11, 3, 4, AIR);                                   // entrada ao sul
    scFill(8, -1, 8, 12, -1, 12, CH_S);                              // piso decorado
    for (int i = 5; i <= 15; i++) { scPut(i, 0, 5, i % 2 ? ORA : BLU); scPut(i, 0, 15, i % 2 ? BLU : ORA); scPut(5, 0, i, i % 2 ? ORA : BLU); scPut(15, 0, i, i % 2 ? BLU : ORA); }
    scFill(7, -15, 7, 13, -1, 13, SS);                               // bloco da camara secreta
    scFill(8, -13, 8, 12, -10, 12, AIR); scFill(10, -9, 10, 10, 0, 10, AIR);   // camara + poco central
    scPut(10, -14, 10, XB_PLATE, 0); scFill(9, -15, 9, 11, -15, 11, TNTB);    // placa de pressao sobre 3x3 de TNT
    scChestTable(8, -13, 10, 0); scChestTable(12, -13, 10, 0); scChestTable(10, -13, 8, 0); scChestTable(10, -13, 12, 0);
}
static void scIgloo() {
    const int SN = SNOWBLK, CT = CTABLE; const double R = 3.5;
    for (int z = 0; z < 7; z++) for (int x = 0; x < 7; x++) {
        const double dx = x - 3, dz = z - 3; scPut(x, -1, z, SN); scPut(x, 0, z, SN);   // piso
        for (int y = 1; y <= 4; y++) { const double d = dx * dx + dz * dz + (double)y * y; if (d <= R * R) scPut(x, y, z, d > (R - 1) * (R - 1) ? SN : AIR); else scPut(x, y, z, AIR); }
    }
    scFill(3, 1, 5, 3, 2, 6, AIR); scPut(2, 1, 2, CT);
    scPut(3, 5, 3, SN);
}
static void scWitchHut() {
    const int PL = PLANKS, LG = LOG, fy = 2;   // piso em y=2 (pernas descem ate o chao)
    for (int cx : {1, 5}) for (int cz : {1, 5}) scFill(cx, -4, cz, cx, fy - 1, cz, LG);
    scFill(1, fy, 1, 5, fy, 5, PL); scFill(1, fy + 1, 1, 5, fy + 3, 5, PL); scFill(2, fy + 1, 2, 4, fy + 3, 4, AIR);
    scFill(0, fy + 4, 0, 6, fy + 4, 6, PL);
    scFill(3, fy + 1, 5, 3, fy + 2, 5, AIR); scFill(1, fy + 2, 3, 1, fy + 2, 3, AIR); scFill(5, fy + 2, 3, 5, fy + 2, 3, AIR);   // porta e janelas
    scPut(2, fy + 1, 2, CTABLE);
}
static void scMoss(int x0, int y0, int z0, int x1, int y1, int z1) {   // junglePyramidsRandomBlocks: 40% cobble, 60% mossy (por celula, deterministico)
    const int mo = scPk("block/cobblestone_mossy", "block/mossy_cobblestone", MOSSY);
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) scPut(x, y, z, (nhash(gSc.bx + x, gSc.gy + y, gSc.bz + z, 31) % 100) < 40 ? COBBLE : mo);
}
static void scJungleTemple() {   // JunglePyramid 12x10x15 (rotacao fixa)
    scMoss(0, -4, 0, 11, 0, 14);
    scMoss(2, 1, 2, 9, 2, 2); scMoss(2, 1, 12, 9, 2, 12); scMoss(2, 1, 3, 2, 2, 11); scMoss(9, 1, 3, 9, 2, 11);
    scMoss(1, 3, 1, 10, 6, 1); scMoss(1, 3, 13, 10, 6, 13); scMoss(1, 3, 2, 1, 6, 12); scMoss(10, 3, 2, 10, 6, 12);
    scMoss(2, 3, 2, 9, 3, 12); scMoss(2, 6, 2, 9, 6, 12); scMoss(3, 7, 3, 8, 7, 11); scMoss(4, 8, 4, 7, 8, 10);
    scFill(3, 1, 3, 8, 2, 11, AIR); scFill(4, 3, 6, 7, 3, 9, AIR); scFill(2, 4, 2, 9, 5, 12, AIR); scFill(4, 6, 5, 7, 6, 9, AIR); scFill(5, 7, 6, 6, 7, 8, AIR);
    scFill(5, 1, 2, 6, 2, 2, AIR); scFill(5, 2, 12, 6, 2, 12, AIR); scFill(5, 5, 1, 6, 5, 1, AIR); scFill(5, 5, 13, 6, 5, 13, AIR);
    for (int i = 0; i <= 14; i += 14) { scFill(2, 4, i, 2, 5, i, AIR); scFill(4, 4, i, 4, 5, i, AIR); scFill(7, 4, i, 7, 5, i, AIR); scFill(9, 4, i, 9, 5, i, AIR); }   // ameias
    for (int i = 0; i <= 11; i += 11) for (int j = 2; j <= 12; j += 2) scFill(i, 4, j, i, 5, j, AIR);
    scPut(1, 5, 5, AIR); scPut(10, 5, 5, AIR); scPut(1, 5, 9, AIR); scPut(10, 5, 9, AIR);   // janelas
    // escada de entrada (cobble, material 1) subindo ate a porta ao norte e degraus internos ate o andar de cima
    for (int st = 0; st < 3; st++) for (int x = 5; x <= 6; x++) { scFill(x, -4, 3 + 0, x, 0, 3, COBBLE); }   // soleira
    for (int x = 5; x <= 6; x++) { scPut(x, 1, 3, XB_STAIR, 2 | (1 << 3)); }   // degrau interno
    scPut(5, 4, 11, XB_CHEST); if (!gScChests.empty()) gScChests.back()[3] = 3; scPut(6, 1, 11, XB_CHEST); if (!gScChests.empty()) gScChests.back()[3] = 3;
    scPut(3, 4, 3, XB_LADDER, 3);
}
static void scatterChunk(Chunk* c, int cx, int cz) {
    if (gDim) return;
    for (int sz = cz - 1; sz <= cz; sz++) for (int sx = cx - 1; sx <= cx; sx++) {
        const int rx = fdiv(sx, 32), rz = fdiv(sz, 32); JR2 r(0);
        r.setSeed((int64_t)((uint64_t)((int64_t)rx * 341873128712LL) + (uint64_t)((int64_t)rz * 132897987541LL)) + (int64_t)WORLD_SEED + 14357617LL);
        const int ax = rx * 32 + r.nextInt(24), az = rz * 32 + r.nextInt(24); if (ax != sx || az != sz) continue;
        const int bio = biomeAt((float)(sx * 16 + 8), (float)(sz * 16 + 8)); const int kind = bio == B_DESERT ? 0 : bio == B_TUNDRA ? 1 : bio == B_SWAMP ? 2 : bio == B_JUNGLE ? 3 : -1; if (kind < 0) continue;
        const int bx = sx * 16, bz = sz * 16, SW = kind == 0 ? 21 : kind == 3 ? 12 : 7, SD = kind == 0 ? 21 : kind == 3 ? 15 : 7; int g = 0, n = 0;
        const int PX[5] = {0, SW - 1, 0, SW - 1, SW / 2}, PZ[5] = {0, 0, SD - 1, SD - 1, SD / 2}; for (int q = 0; q < 5; q++) { g += vilGround(bx + PX[q], bz + PZ[q]); n++; }
        g /= n; if (g < SEA - 1) continue;
        gSc = {c, cx * 16, cz * 16, bx, bz, g};
        if (kind == 0) scDesertTemple(); else if (kind == 1) scIgloo(); else if (kind == 2) scWitchHut(); else scJungleTemple();
    }
}
static void scChestFlush() {   // loot (tabelas 1.12.2 simplificadas; deterministico pela posicao)
    for (size_t i = 0; i < gScChests.size();) {
        const auto p = gScChests[i]; if (getB(p[0], p[1], p[2]) != XB_CHEST) { if (!getC(fdiv(p[0], CW), fdiv(p[2], CW))) { ++i; continue; } gScChests.erase(gScChests.begin() + i); continue; }
        ChestInv* ci = chestAt(p[0], p[1], p[2], true);
        if (ci && ci->empty()) {
            uint32_t h = nhash(p[0], p[1], p[2], 77); auto R = [&](int n) { h = h * 1664525u + 1013904223u; return (int)((h >> 10) % (uint32_t)n); };
            static const int bone = pkFind("item/bone"), rot = pkFind("item/rotten_flesh"), em = pkFind("item/emerald"), sad = pkFind("item/saddle"), gun = pkFind("item/gunpowder"), sp = pkFind("item/spider_eye"), gap = pkFind("item/apple_golden"), apl = pkFind("item/apple"), wh = pkFind("item/wheat"), coal = pkFind("item/coal"), ng = pkFind("item/gold_nugget"), ax = pkFind("item/stone_axe");
            const int tb = p[3];
            for (int k = 0, n = 2 + R(5); k < n; k++) { const int w = R(100);
                if (tb == 0 || tb == 3) { if (w < 6) chestAdd(*ci, IT_DIAMOND, 1 + R(3)); else if (w < 22) chestAdd(*ci, IT_IRON, 1 + R(5)); else if (w < 38) chestAdd(*ci, IT_GOLD, 2 + R(5)); else if (w < 48 && em) chestAdd(*ci, em, 1 + R(3)); else if (w < 56 && sad) chestAdd(*ci, sad, 1); else if (w < 62 && gap) chestAdd(*ci, gap, 1); else if (w < 72 && sp) chestAdd(*ci, sp, 1 + R(3)); else if (w < 84 && gun) chestAdd(*ci, gun, 1 + R(3)); else if (w < 92 && bone) chestAdd(*ci, bone, 4 + R(6)); else if (rot) chestAdd(*ci, rot, 3 + R(5)); }
                else { if (w < 30 && apl) chestAdd(*ci, apl, 1 + R(3)); else if (w < 55 && coal) chestAdd(*ci, coal, 1 + R(4)); else if (w < 70 && ng) chestAdd(*ci, ng, 1 + R(3)); else if (w < 80 && wh) chestAdd(*ci, wh, 2 + R(3)); else if (w < 85 && ax) chestAdd(*ci, ax, 1); else if (rot) chestAdd(*ci, rot, 1 + R(3)); }
            }
        }
        gScChests.erase(gScChests.begin() + i);
    }
}
