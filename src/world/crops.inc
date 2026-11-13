// patchCrop.1: enxada -> terra arada, semente/cenoura/batata plantam em XB_CROP, crescimento aleatorio perto do jogador, colheita com drops do Java.
// Tipos de XB_CROP (m = tipo<<3 | idade 0..7): 0 trigo, 1 cenoura, 2 batata, 3 beterraba. Sem hidratacao/luz/pisoteio (limites no PENDENCIAS).
static int cropItem(const char* f) { return pkFind(f); }
static int cropSeedType(int id) {
    static const char* NM[5] = {"item/wheat_seeds", "item/carrot", "item/potato", "item/beetroot_seeds", "item/nether_wart"};
    for (int i = 0; i < 5; i++) { const int p = cropItem(NM[i]); if (p && p == id) return i; }
    return -1;
}
static int cropUse(Slot& s, const int* hh, const int* pp, bool rpt) {   // 0 nada, 1 so balanca, 2 balanca e gasta 1 do item
    if (rpt || !s.id) return 0;
    const int hb = getB(hh[0], hh[1], hh[2]);
    if (s.id >= IT_WHOE && s.id <= IT_DHOE) {
        if ((hb == GRASS || hb == DIRT) && getB(hh[0], hh[1] + 1, hh[2]) == AIR) { setBM(hh[0], hh[1], hh[2], XB_FARM, 0); netSendBlock(hh[0], hh[1], hh[2], XB_FARM, 0); itemWear(s, 1); return 1; }
        return 0; }
    {   // patchCrop.2: farinha de osso numa colheita: +2..5 estagios (beterraba: 1 com 2/3 de chance, como o Java); gasta 1 mesmo se falhar
        static int bm = -2; if (bm == -2) { bm = cropItem("item/bone_meal"); if (!bm) bm = cropItem("item/dye_powder_white"); }
        if (bm > 0 && s.id == bm && hb == XB_CROP) {
            const int m0 = getM(hh[0], hh[1], hh[2]), age = m0 & 7; if (age >= 7 || ((m0 >> 3) & 7) == 4) return 0;
            const int inc = ((m0 >> 3) & 7) == 3 ? (rand() % 3 ? 1 : 0) : 2 + rand() % 4, m1 = (m0 & ~7) | std::min(7, age + inc);
            setBM(hh[0], hh[1], hh[2], XB_CROP, m1); netSendBlock(hh[0], hh[1], hh[2], XB_CROP, m1); return 2;
        }
    }
    const int ty = cropSeedType(s.id);
    static const int ssd = pkFind("block/soul_sand");
    if (ty >= 0 && (ty == 4 ? (ssd && hb == ssd) : (hb == XB_FARM && !(getM(hh[0], hh[1], hh[2]) & 1))) && getB(hh[0], hh[1] + 1, hh[2]) == AIR) {
        const int m = ty << 3; setBM(hh[0], hh[1] + 1, hh[2], XB_CROP, m); netSendBlock(hh[0], hh[1] + 1, hh[2], XB_CROP, m); return 2; }
    return 0;
}
// patchCrop.1: terra arada com umidade (meta bits 1..3 = 0..7; bit 0 = caminho) e crescimento do vanilla 1.12 (BlockCrops.getGrowthChance)
static bool cropWaterNear(int x, int y, int z) { for (int dy = 0; dy <= 1; dy++) for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) if (isWater(getB(x + dx, y + dy, z + dz))) return true; return false; }
static bool cropRainOn(int x, int y, int z) { if (gRain < .5f || gDim != 0) return false; for (int yy = y + 1; yy < CH; yy++) { const int b = getB(x, yy, z); if (b != AIR && b != XB_CROP) return false; } return true; }
static void farmTick(int x, int y, int z) {   // BlockFarmland.updateTick
    const int m = getM(x, y, z); if (m & 1) return; const int mo = (m >> 1) & 7;
    if (cropWaterNear(x, y, z) || cropRainOn(x, y, z)) { if (mo != 7) { setBM(x, y, z, XB_FARM, 7 << 1); netSendBlock(x, y, z, XB_FARM, 7 << 1); } }
    else if (mo > 0) { setBM(x, y, z, XB_FARM, (mo - 1) << 1); netSendBlock(x, y, z, XB_FARM, (mo - 1) << 1); }
    else if (getB(x, y + 1, z) != XB_CROP) { setBM(x, y, z, DIRT, 0); netSendBlock(x, y, z, DIRT, 0); }   // seca e sem plantacao: vira terra
}
// patchSapling.1: muda cresce em arvore. Gera a arvore num chunk temporario centrado em (8,8) e copia para o mundo (so onde ha ar/folha, como treePut)
static bool saplingCanGrow(int x, int y, int z, int sp) {
    for (int h = 1; h <= 4; h++) { const int b = getB(x, y + h, z); if (b != AIR && !isLeafB(b) && !xbReplaceable(b)) return false; }
    return sp != 5 || true;
}
static void saplingGrow(int x, int y, int z) {
    const int b0 = getB(x, y, z); if (!xbIsSapling(b0)) return; const int sp = b0 - XB_SAPLING;   // 0 carvalho 1 abeto 2 betula 3 selva 4 acacia 5 carvalho-escuro
    int ox = x, oz = z;
    if (sp == 5) {   // carvalho-escuro so cresce com 4 mudas em 2x2 (como o MC); origem = canto -x/-z
        bool ok = false;
        for (int a = -1; a <= 0 && !ok; a++) for (int c = -1; c <= 0 && !ok; c++)
            if (getB(x + a, y, z + c) == b0 && getB(x + a + 1, y, z + c) == b0 && getB(x + a, y, z + c + 1) == b0 && getB(x + a + 1, y, z + c + 1) == b0) { ox = x + a; oz = z + c; ok = true; }
        if (!ok) return;
    }
    if (!saplingCanGrow(ox, y, oz, sp)) return;
    std::unique_ptr<Chunk> sc(new Chunk()); memset(sc->b, 0, sizeof sc->b); memset(sc->m, 0, sizeof sc->m);
    const int FAR_CX = 1 << 24, savedX = gTcx, savedZ = gTcz, savedLog = gTreeLog, savedLeaf = gTreeLeaf;
    gTcx = FAR_CX; gTcz = FAR_CX;   // chunk de referencia inexistente: o que passar de 16x16 vai pro gSpill falso e e descartado
    static const int SPEC[6] = {0, 1, 2, 3, 4, 5};
    treeSpecies(SPEC[sp]);
    TreeCtx t{sc.get(), JR2((int64_t)ox * 341873128712LL + (int64_t)oz * 132897987541LL + (int64_t)rand() * 7919)};
    const int ly = y;
    switch (sp) {
    case 0: if (t.r.nextInt(10) == 0) mcBigTree(t, 8, ly, 8); else mcTreeSmall(t, 8, ly, 8, 4, false); break;
    case 1: if (t.r.nextInt(2)) mcPine(t, 8, ly, 8); else mcSpruce(t, 8, ly, 8); break;
    case 2: mcTreeSmall(t, 8, ly, 8, 5, false); break;
    case 3: mcTreeSmall(t, 8, ly, 8, 4, true); break;
    case 4: mcAcacia(t, 8, ly, 8); break;
    default: mcDarkOak(t, 8, ly, 8); break;
    }
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) gSpill.erase(key(FAR_CX + dx, FAR_CX + dz));
    gTcx = savedX; gTcz = savedZ; gTreeLog = savedLog; gTreeLeaf = savedLeaf;
    if (sp == 5) for (int a = 0; a < 2; a++) for (int c = 0; c < 2; c++) { setB(ox + a, y, oz + c, AIR); }
    else setB(x, y, z, AIR);
    for (int yy = y; yy < std::min(CH, y + 40); yy++) for (int lz = 0; lz < CW; lz++) for (int lx = 0; lx < CW; lx++) {
        const int id = sc->b[idx(lx, yy, lz)]; if (!id) continue;
        const int wx = ox + lx - 8, wz = oz + lz - 8, cur = getB(wx, yy, wz); if (!getC(fdiv(wx, CW), fdiv(wz, CW))) continue;
        const bool leaf = isLeafB(id);
        if (!(cur == AIR || xbReplaceable(cur) || (!leaf && isLeafB(cur)))) continue;
        const int m = sc->m[idx(lx, yy, lz)];
        setBM(wx, yy, wz, id, m); netSendBlock(wx, yy, wz, id, m);
    }
}
static void cropRandomTick() {   // como o randomTick do Java: amostra blocos perto do jogador (cada bloco ~1x a cada 72 s)
    if (!gPvpP) return;
    const int cx = (int)floor(gPvpP->x), cy = (int)floor(gPvpP->y), cz = (int)floor(gPvpP->z);
#ifdef BL_RPI
    for (int i = 0; i < 48; i++) {   // patchRpi.8: metade das amostras no Pi (as culturas crescem mais devagar, custo de CPU cai)
#else
    for (int i = 0; i < 96; i++) {
#endif
        const int x = cx + rand() % 65 - 32, y = cy + rand() % 33 - 16, z = cz + rand() % 65 - 32; const int b = getB(x, y, z);
        if (b == XB_FARM) { farmTick(x, y, z); continue; }
        if (b == DIRT && getM(x, y, z) == 0) {   // BUG-024: grama se espalha na terra (randomTick do BlockGrass: vizinha 3x5x3 + topo livre)
            const int ab = getB(x, y + 1, z); if (ab != AIR && (solid(ab) || isWater(ab))) continue;
            if (rand() % 4) continue;
            const int sx = x + rand() % 3 - 1, sy = y + rand() % 5 - 3, sz = z + rand() % 3 - 1;
            if (getB(sx, sy, sz) == GRASS && getB(sx, sy + 1, sz) == AIR) { setBM(x, y, z, GRASS, 0); netSendBlock(x, y, z, GRASS, 0); }
            continue; }
        if (xbIsSapling(b)) { if (rand() % 4 == 0) { const int sm = getM(x, y, z); if (!(sm & 8)) { setBM(x, y, z, b, sm | 8); netSendBlock(x, y, z, b, sm | 8); } else saplingGrow(x, y, z); } continue; }   // patchSapling.1: 1/4 por amostra: 1o aviso (estagio), 2o cresce (MC usa 1/7)
        if (gCactusId && b == gCactusId) {   // patchGrow.1: BlockCactus.updateTick: topo livre, altura < 3; idade 0..15 e ao chegar em 15 cresce 1 bloco
            if (getB(x, y + 1, z) != AIR) continue;
            int h = 1; while (h < 3 && getB(x, y - h, z) == gCactusId) h++;
            if (h >= 3) continue;
            const int age = getM(x, y, z);
            if (age < 15) { setBM(x, y, z, b, age + 1); netSendBlock(x, y, z, b, age + 1); continue; }
            static const int CX[4] = {1, -1, 0, 0}, CZ[4] = {0, 0, 1, -1}; bool side = false;
            for (int s = 0; s < 4; s++) if (solid(getB(x + CX[s], y + 1, z + CZ[s]))) side = true;   // canBlockStay do novo bloco: sem solido ao lado
            if (side) continue;
            setBM(x, y + 1, z, b, 0); netSendBlock(x, y + 1, z, b, 0); setBM(x, y, z, b, 0); netSendBlock(x, y, z, b, 0);
            continue; }
        if (b != XB_CROP) continue;
        const int m = getM(x, y, z), ty = (m >> 3) & 7; if ((m & 7) >= (ty == 4 ? 3 : 7)) continue;
        if (ty == 4) { if (rand() % 10) continue; }
        else {   // getGrowthChance: base 1 + terra de baixo (arada 1, molhada 3; vizinhas /4); mesma cultura em fileira/diagonal /2
            float f = 1.f;
            for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) { float ff = 0.f; const int bb = getB(x + dx, y - 1, z + dz);
                if (bb == XB_FARM && !(getM(x + dx, y - 1, z + dz) & 1)) { ff = 1.f; if (((getM(x + dx, y - 1, z + dz) >> 1) & 7) > 0) ff = 3.f; }
                if (dx || dz) ff /= 4.f; f += ff; }
            auto same = [&](int ax, int az) { return getB(ax, y, az) == XB_CROP && ((getM(ax, y, az) >> 3) & 7) == ty; };
            const bool row = same(x - 1, z) || same(x + 1, z), col = same(x, z - 1) || same(x, z + 1), dia = same(x - 1, z - 1) || same(x + 1, z - 1) || same(x - 1, z + 1) || same(x + 1, z + 1);
            if ((row && col) || dia) f /= 2.f;
            if (rand() % ((int)(25.f / f) + 1)) continue;
        }
        setBM(x, y, z, XB_CROP, m + 1); netSendBlock(x, y, z, XB_CROP, m + 1);
    }
}
static void cropDrops(int bx, int by, int bz) {
    const int m = getB(bx, by, bz) == XB_CROP ? getM(bx, by, bz) : gRsLastMeta, ty = (m >> 3) & 7; const bool ripe = (m & 7) >= (ty == 4 ? 3 : 7);
    if (ty == 4) { const int w = cropItem("item/nether_wart"); if (w) dropFromBlock(bx, by, bz, w, ripe ? 2 + rand() % 3 : 1); return; }   // verruga: 2..4 madura, 1 verde
    const int seed = cropItem(ty == 3 ? "item/beetroot_seeds" : "item/wheat_seeds"), crop = cropItem(ty == 0 ? "item/wheat" : ty == 1 ? "item/carrot" : ty == 2 ? "item/potato" : "item/beetroot");
    if (!ripe) { const int it = (ty == 0 || ty == 3) ? seed : crop; if (it) dropFromBlock(bx, by, bz, it, 1); return; }   // verde: so devolve a semente/raiz
    if (ty == 0 || ty == 3) {   // trigo/beterraba maduros: 1 produto + 0..3 sementes
        if (crop) dropFromBlock(bx, by, bz, crop, 1);
        int n = 0; for (int k = 0; k < 3; k++) if (rand() % 100 < 57) n++; if (seed && n) dropFromBlock(bx, by, bz, seed, n);
    } else {   // cenoura/batata maduras: 1 + binomial(3, .57)
        int n = 1; for (int k = 0; k < 3; k++) if (rand() % 100 < 57) n++; if (crop) dropFromBlock(bx, by, bz, crop, n);
        if (ty == 2 && rand() % 50 == 0) { const int pz = cropItem("item/poisonous_potato"); if (pz) dropFromBlock(bx, by, bz, pz, 1); }
    }
}

// patchMech.2: terra arada pisoteada (BlockFarmland.onFallenUpon 1.12: chance = queda - 0,5 vira terra)
static void mechFarmTick(const Phys& P) {
    static float fl = 0.f;
    if (P.flying || P.inWater || P.noclip) { fl = 0.f; return; }
    if (!P.onGround) { if (P.y < P.py) fl += (float)(P.py - P.y); return; }
    if (fl > .5f) { const int x = (int)floor(P.x), y = (int)floor(P.y - .01), z = (int)floor(P.z);
        if (getB(x, y, z) == XB_FARM && (rand() % 1000) / 1000.f < fl - .5f) { setBM(x, y, z, DIRT, 0); netSendBlock(x, y, z, DIRT, 0); } }
    fl = 0.f;
}
