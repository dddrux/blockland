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
static void cropRandomTick() {   // como o randomTick do Java: amostra blocos perto do jogador; ~1/7 de chance de crescer um estagio
    if (!gPvpP) return;
    const int cx = (int)floor(gPvpP->x), cy = (int)floor(gPvpP->y), cz = (int)floor(gPvpP->z);
    for (int i = 0; i < 96; i++) {
        const int x = cx + rand() % 65 - 32, y = cy + rand() % 33 - 16, z = cz + rand() % 65 - 32;
        if (getB(x, y, z) != XB_CROP || rand() % 7) continue;
        const int m = getM(x, y, z); if ((m & 7) >= (((m >> 3) & 7) == 4 ? 3 : 7)) continue;
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
