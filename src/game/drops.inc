// ---------- itens no chão (EntityItem do Minecraft 1.8.9) ----------
// Caixa 0.25^3, gravidade 0.04/tick, arrasto 0.98 (chão: 0.6*0.98), delay de coleta 10 ticks (bloco) / 40 (jogado),
// coleta na caixa do jogador expandida (1.0, 0.5, 1.0), junta pilhas próximas, some após 5 min (6000 ticks).
struct DropEnt { double x, y, z, px, py, pz, vx, vy, vz; int id, n, age, delay; float bob; uint32_t nid; std::string nbt; };   // nbt: dados extras da pilha (ex.: conteudo do bau)
static std::vector<DropEnt> DROPS;
static bool gItemPhys = false;   // patchItemPhys.1: item no chao deitado, sem girar/flutuar; caindo/na agua = rolando (ligado por modules/itemphysic)
struct PickFx { double x, y, z; int id, age, who; };   // who: -1 = jogador local, senão id do jogador remoto que coletou   // animação de coleta: o item voa até o jogador em 3 ticks (EntityItem -> S0D)
static std::vector<PickFx> PICKS;
static int netSelf = 0; static uint32_t netNextId = 0;   // nid = (dono << 24) | contador: identifica o drop entre os jogadores
static void netSendDrop(const DropEnt& d); static void netSendDropSet(uint32_t nid, int n, int picker = 255);
static bool netRemotePos(int id, float pt, double* o);   // posição interpolada de um jogador remoto (net.inc)
static inline float frand() { return rand() / (float)RAND_MAX; }
static int sndMatOf(int b) {   // patchAud.2: material do bloco (indices de SND_BRK/PLC/STP)
    if (b == GRASS || b == DIRT || b == LEAVES || isLeafB(b) || (b >= XB_PLANT && b < XB_RAIL) || b == CLAY) return 1;
    if (b == XB_PANE || b == XB_CPANE) return 7;   // vidraça: som de vidro
    if (b == GRAVEL) return 2; if (b == SAND) return 3; if (b == LOG || b == PLANKS || b == BOOKSHELF) return 4; if (b == WOOL) return 5; if (b == SNOWBLK) return 6;
    if (isPk(b)) { const std::string& f = PK[b - PK0].file; auto has = [&](const char* t) { return f.find(t) != std::string::npos; };
        if (has("glass")) return 7; if (has("wool") || has("carpet")) return 5; if (has("snow")) return 6; if (has("gravel")) return 2;
        if (has("sand") && !has("sandstone")) return 3; if (has("planks") || has("log") || has("wood") || has("crafting") || has("bookshelf") || has("chest") || has("jukebox") || has("note_block") || has("pumpkin") || has("melon")) return 4;
        if (has("dirt") || has("grass") || has("podzol") || has("mycelium") || has("farmland") || has("clay") || has("leaves") || has("sapling") || has("flower") || has("tallgrass") || has("fern")) return 1; }
    return 0;
}
static const char* sndBlockEvt(int b, bool place) { const int m = sndMatOf(b); return place ? SND_PLC[m] : SND_BRK[m]; }
static void spawnDrop(double x, double y, double z, double vx, double vy, double vz, int id, int n, int delay, uint32_t nid = 0, const std::string& nbt = std::string()) {
    if (!id || n <= 0 || DROPS.size() > 600) return;
    const bool local = nid == 0;
    if (local) nid = ((uint32_t)netSelf << 24) | (++netNextId & 0xFFFFFF);
    DROPS.push_back({x, y, z, x, y, z, vx, vy, vz, id, n, 0, delay, frand() * 6.2831853f, nid, nbt});
    if (local) netSendDrop(DROPS.back());
}
static void dropFromBlock(int bx, int by, int bz, int id, int n = 1) {   // Block.spawnAsEntity
    spawnDrop(bx + .15 + frand() * .7, by + .15 + frand() * .7, bz + .15 + frand() * .7, frand() * .2 - .1, .2, frand() * .2 - .1, id, n, 10);
}
static void cropDrops(int bx, int by, int bz);   // crops.inc
static int gLampOn = 0, gLampOff = 0, gNoteId = 0;   // logic.inc
static int gDropSilk = 0, gDropFort = 0;   // patchEnch.4: toque suave/fortuna da ferramenta que quebrou o bloco (frame_begin.inc liga so durante o blockDrops do jogador)
static void blockDrops(int bx, int by, int bz, int b) {   // tabela de drops (quantidades/chances como no Minecraft)
    if (gDropSilk && b && b != WATER && !isXB(b)) { dropFromBlock(bx, by, bz, b, 1); return; }   // toque suave: solta o proprio bloco (VIS: variantes/meta)
    if (gLampOn && b == gLampOn && gLampOff) b = gLampOff;   // lampada acesa solta a apagada
    if (gShearBreak) {   // patchShear.1: com tesoura, folhas/teia/mato soltam o proprio bloco
        bool self = isLeafB(b);
        if (!self && b >= PK0 && b < PK0 + (int)PK.size()) { const std::string& f = PK[b - PK0].file; for (const char* k : {"cobweb", "tallgrass", "dead_bush", "deadbush", "fern", "vine"}) if (f.find(k) != std::string::npos) self = true; }
        if (self) { dropFromBlock(bx, by, bz, b, 1); return; }
    }
    if (b == XB_CROP) { cropDrops(bx, by, bz); return; }   // patchCrop.1
    if (isXB(b)) { const int it = isRs(b) ? rsDropItem(bx, by, bz, b) : xbDropItem(b, getB(bx, by, bz) == b ? getM(bx, by, bz) : gRsLastMeta); if (it) dropFromBlock(bx, by, bz, it, 1); return; }
    if (b >= PK0 && leafKind(b)) {   // folhas do pack: 5% de muda da propria especie, senao nada (como no MC sem tesoura)
        if (rand() % 100 < 5) { std::string f = PK[b - PK0].file; const size_t p = f.rfind("_leaves"); if (p != std::string::npos) { auto it = gPkByFile.find(f.substr(0, p) + "_sapling"); if (it != gPkByFile.end()) dropFromBlock(bx, by, bz, it->second, 1); } }
        return;
    }
    int id = dropOf(b), n = 1;
    if (b == XB_PLANT && rand() % 8 == 0) { const int sd = pkFind("item/wheat_seeds"); if (sd) { dropFromBlock(bx, by, bz, sd, 1); return; } }   // patchSurv.1: grama alta solta semente de trigo (12,5%): sem isso nao ha como plantar trigo
    if (b >= PK0 && b < PK0 + (int)PK.size()) {   // patchSurv.1: blocos do pack que soltam outro item (como o Java sem toque suave)
        const std::string& f = PK[b - PK0].file; auto drop = [&](const char* nm, int lo, int hi) { const int it = pkFind(nm); if (!it) return false; id = it; n = lo + (hi > lo ? rand() % (hi - lo + 1) : 0); return true; };
        if (f == "block/emerald_ore") drop("item/emerald", 1, 1); else if (f == "block/nether_quartz_ore" || f == "block/quartz_ore") drop("item/quartz", 1, 1);
        else if (f == "block/melon") drop("item/melon_slice", 3, 7); else if (f == "block/snow") drop("item/snowball", 4, 4);
        else if (f == "block/clay") drop("item/clay_ball", 4, 4); else if (f == "block/glowstone") drop("item/glowstone_dust", 2, 4);
    }
    switch (b) {
        case CLAY: { const int it = pkFind("item/clay_ball"); if (it) { id = it; n = 4; } break; }
        case SNOWBLK: { const int it = pkFind("item/snowball"); if (it) { id = it; n = 4; } break; }
        case GLOWSTONE: { const int it = pkFind("item/glowstone_dust"); if (it) { id = it; n = 2 + rand() % 3; } break; }
        case GRAVEL: if (rand() % 10 == 0) id = IT_FLINT; break;
        case LAPISORE: n = 4 + rand() % 5; break;
        case REDSTONEORE: n = 4 + rand() % 2; break;
        case BOOKSHELF: id = IT_BOOK; n = 3; break;
    }
    if (id && gDropFort > 0 && id != b) {   // fortuna: carvao/diamante/esmeralda/quartzo multiplicam; lapis/redstone somam (BlockOre.quantityDroppedWithBonus)
        const bool mul = b == COALORE || b == DIAMONDORE || (b >= PK0 && b < PK0 + (int)PK.size() && (PK[b - PK0].file == "block/emerald_ore" || PK[b - PK0].file == "block/nether_quartz_ore" || PK[b - PK0].file == "block/quartz_ore"));
        if (mul) n *= std::max(1, rand() % (gDropFort + 2)); else if (b == LAPISORE || b == REDSTONEORE) n += rand() % (gDropFort + 1);
    }
    if (id) dropFromBlock(bx, by, bz, id, n);
}
static void throwItem(const double* eye, float yawDeg, float pitchDeg, int id, int n, const std::string& nbt = std::string()) {   // EntityPlayer.dropItem(throw)
    const float sy = sinf(yawDeg * .0174533f), cy = cosf(yawDeg * .0174533f), sp = sinf(pitchDeg * .0174533f), cp = cosf(pitchDeg * .0174533f);
    double vx = -sy * cp * .3, vy = sp * .3 + .1, vz = -cy * cp * .3;
    const float a = frand() * 6.2831853f, r = frand() * .02f;
    vx += cosf(a) * r; vy += (frand() - frand()) * .1; vz += sinf(a) * r;
    sndEvtAt("item.drop", eye[0], eye[1], eye[2]);
    spawnDrop(eye[0], eye[1] - .3, eye[2], vx, vy, vz, id, n, 40, 0, nbt);
}
static bool dropHits(double x, double y, double z) {
    const int x0 = (int)floor(x - .125), x1 = (int)floor(x + .125), y0 = (int)floor(y), y1 = (int)floor(y + .25), z0 = (int)floor(z - .125), z1 = (int)floor(z + .125);
    for (int yy = y0; yy <= y1; yy++) for (int zz = z0; zz <= z1; zz++) for (int xx = x0; xx <= x1; xx++) if (solid(getB(xx, yy, zz))) return true;
    return false;
}
static void tickDrops(double ex, double ey, double ez) {
    static unsigned tc = 0; tc++;
    for (size_t i = 0; i < PICKS.size();) { if (++PICKS[i].age >= 3) { PICKS[i] = PICKS.back(); PICKS.pop_back(); } else ++i; }
    for (size_t i = 0; i < DROPS.size();) {
        DropEnt& d = DROPS[i];
        if (!getC(fdiv((int)floor(d.x), CW), fdiv((int)floor(d.z), CW))) { ++i; continue; }   // chunk não carregado: congela
        if (gPerf) { const double fx0 = d.x - ex, fz0 = d.z - ez, R0 = RAD * 16.0; if (fx0 * fx0 + fz0 * fz0 > R0 * R0) { ++i; continue; } }   // patchRpiOpt.6: item alem do raio de render: congela
        d.px = d.x; d.py = d.y; d.pz = d.z; d.age++; if (d.delay > 0) d.delay--;
        if (gCactusId) { const int cx0 = (int)floor(d.x), cy0 = (int)floor(d.y + .1), cz0 = (int)floor(d.z); bool hit = getB(cx0, cy0, cz0) == gCactusId || getB(cx0, cy0 - 1, cz0) == gCactusId;   /* patchMech.2: cacto destroi item em cima/dentro/ao lado */
            for (int s = 0; s < 4 && !hit; s++) { static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1}; if (getB((int)floor(d.x + DX[s] * .3), cy0, (int)floor(d.z + DZ[s] * .3)) == gCactusId) hit = true; }
            if (hit) { DROPS[i] = DROPS.back(); DROPS.pop_back(); continue; } }
        if (d.y < -64.0 || isLava(getB((int)floor(d.x), (int)floor(d.y + .1), (int)floor(d.z))) || getB((int)floor(d.x), (int)floor(d.y + .1), (int)floor(d.z)) == XB_FIRE) { DROPS[i] = DROPS.back(); DROPS.pop_back(); continue; }   // lava queima e o vazio engole itens
        const bool water = isWater(getB((int)floor(d.x), (int)floor(d.y + .1), (int)floor(d.z)));
        if (water) { if (d.vy < .06) d.vy += .002; } else d.vy -= .04;
        if (water) { double fx, fy, fz; flowFull((int)floor(d.x), (int)floor(d.y + .1), (int)floor(d.z), fx, fy, fz); d.vx += fx * .014; d.vy += fy * .014; d.vz += fz * .014; }   // patchOpt.2: corrente empurra o item (Entity.handleWaterMovement: 0,014/tick)
        if (dropHits(d.x, d.y, d.z)) d.y += .1;   // preso dentro de um bloco: empurra para cima
        const double m = std::max(fabs(d.vx), std::max(fabs(d.vy), fabs(d.vz)));
        const int steps = std::max(1, (int)ceil(m / .1));
        double sx = d.vx / steps, sy = d.vy / steps, sz = d.vz / steps;
        for (int s = 0; s < steps; s++) {
            if (sy != 0) { if (!dropHits(d.x, d.y + sy, d.z)) d.y += sy; else { if (sy < 0) { const int hb = getB((int)floor(d.x), (int)floor(d.y + sy), (int)floor(d.z)); const int sl = slimeBlkId(); const double v0 = d.vy; d.y = floor(d.y + sy) + 1.0 + 1e-4; d.vy = (sl > 0 && hb == sl && v0 < -.1) ? -v0 * .8 : 0; } else d.vy = 0; sy = 0; } }
            if (sx != 0) { if (!dropHits(d.x + sx, d.y, d.z)) d.x += sx; else { d.vx = 0; sx = 0; } }
            if (sz != 0) { if (!dropHits(d.x, d.y, d.z + sz)) d.z += sz; else { d.vz = 0; sz = 0; } }
        }
        const bool ground = dropHits(d.x, d.y - .01, d.z);
        const double f = water ? .95 : ground ? .6 * .98 : .98;
        d.vx *= f; d.vz *= f; d.vy *= water ? .98 : .98;
        bool gone = d.age >= 6000 || d.y < -10;
        if (!gone && d.delay <= 0 && gGm != 3 && !gDead) {   // coleta (espectador e jogador morto não coletam): caixa do jogador expandida (1.0, 0.5, 1.0)
            if (d.x + .125 > ex - 1.3 && d.x - .125 < ex + 1.3 && d.z + .125 > ez - 1.3 && d.z - .125 < ez + 1.3 && d.y + .25 > ey - .5 && d.y < ey + 2.3) {
                const int got = d.n, left = addItem(d.id, d.n, d.nbt);
                if (left < got) { sndEvtAt("item.pickup", ex, ey + .5, ez); if (left <= 0) PICKS.push_back({d.x, d.y, d.z, d.id, 0, -1}); netSendDropSet(d.nid, left, left <= 0 ? netSelf : 255); }
                d.n = left; gone = left <= 0;
            }
        }
        if (gone) { DROPS[i] = DROPS.back(); DROPS.pop_back(); continue; }
        ++i;
    }
    if ((tc & 1) == 0 && DROPS.size() > 1 && DROPS.size() < 400)   // junta pilhas
        for (size_t i = 0; i < DROPS.size(); i++) for (size_t j = DROPS.size() - 1; j > i; j--) {
            DropEnt &a = DROPS[i], &b = DROPS[j];
            if (netOn && (((a.nid >> 24) != (uint32_t)netSelf) || ((b.nid >> 24) != (uint32_t)netSelf))) continue;   // online: só o dono junta
            if (a.id != b.id || a.nbt != b.nbt || fabs(a.x - b.x) > .75 || fabs(a.z - b.z) > .75 || fabs(a.y - b.y) > .25 || a.n + b.n > maxStack(a.id)) continue;
            a.n += b.n; a.delay = std::max(a.delay, b.delay); a.age = std::min(a.age, b.age);
            netSendDropSet(a.nid, a.n); netSendDropSet(b.nid, 0);
            DROPS[j] = DROPS.back(); DROPS.pop_back();
        }
}
static void propDrawUnit(int id);   // propsys.inc
static void drawDrops(float pt) {
    if (DROPS.empty()) return;
    glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D);
    rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .3f);
    rdEnable(GL_LIGHTING); rdEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    for (const DropEnt& d : DROPS) {
        if (entFarFrom(d.x, d.z, std::min(gPerf ? 40.0 : 48.0, RAD * 16.0))) continue;   // patchRpiOpt.6 / patchFps.5: item no chao a 48 blocos nao e desenhado: item longe nao e desenhado
        if (entOutView(d.x, d.y, d.z, 1.f)) continue;   // patchRpiCull.1
        const double ix = d.px + (d.x - d.px) * pt, iy = d.py + (d.y - d.py) * pt, iz = d.pz + (d.z - d.pz) * pt;
        float lt3[3]; entLightAt(d.x, d.y + .25, d.z, lt3);   // luz do local (céu com ciclo dia/noite + bloco)
        const float amb[4] = {lt3[0], lt3[1], lt3[2], 1.f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
        const float age = d.age + pt, bobY = sinf(age / 10.f + d.bob) * .1f + .1f, rot = (age / 20.f + d.bob) * 57.29578f;
        const bool blk = isBlockLike(d.id); const float sc = blk ? .25f : .5f;
        const bool ipOn = gItemPhys, ipGnd = ipOn && fabs(d.y - d.py) < 1e-6;   // patchItemPhys.1: parado em y = no chao
        const int copies = d.n > 48 ? 5 : d.n > 32 ? 4 : d.n > 16 ? 3 : d.n > 1 ? 2 : 1;
        for (int k = 0; k < copies; k++) {
            rdPushMatrix();
            rdTranslatef((float)ix, (float)iy + (ipOn ? (ipGnd ? (blk ? 0.f : .02f - sc * .5f) : .1f) : bobY) + sc * .5f, (float)iz);
            if (k > 0) rdTranslatef((hash2(k, (int)(d.bob * 100), 3) - .5f) * .3f, (hash2(k, (int)(d.bob * 100), 5) - .5f) * .1f, (hash2(k, (int)(d.bob * 100), 7) - .5f) * .3f);
            if (ipOn) { const float ya = d.bob * 57.29578f; if (ipGnd) { rdRotatef(ya, 0, 1, 0); if (!blk) rdRotatef(90.f, 1, 0, 0); } else { rdRotatef(age * 14.f + ya, 0, 1, 0); rdRotatef(age * 19.f + ya * .7f, 1, 0, 0); } }   // patchItemPhys.1
            else rdRotatef(rot, 0, 1, 0);
            rdScalef(sc, sc, sc); rdTranslatef(-.5f, -.5f, -.5f);
            gGlintOn = enchIsNbt(d.nbt);   // patchEnch.2
            if (isProp(d.id)) propDrawUnit(d.id); else drawMeshG(meshOf(d.id));   // patchProp.2
            gGlintOn = false;
            rdPopMatrix();
        }
    }
    rdDisable(GL_COLOR_MATERIAL); rdDisable(GL_LIGHTING);
}
static void drawPickups(float pt, double ex, double ey, double ez) {   // item voando até o jogador
    if (PICKS.empty()) return;
    glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D);
    rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .3f);
    rdEnable(GL_LIGHTING); rdEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const float amb[4] = {1, 1, 1, 1}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    for (const PickFx& p : PICKS) {
        double tg[3] = {ex, ey, ez}; if (p.who >= 0 && !netRemotePos(p.who, pt, tg)) continue;   // alvo: jogador local ou o remoto que pegou o item
        float t = (p.age + pt) / 3.f; t = std::min(1.f, t * t);
        const float sc = isBlockLike(p.id) ? .25f : .5f;
        const double qx = p.x + (tg[0] - p.x) * t, qy = p.y + (tg[1] + .8 - p.y) * t, qz = p.z + (tg[2] - p.z) * t;
        { float lt3[3]; entLightAt(qx, qy, qz, lt3); const float am2[4] = {lt3[0], lt3[1], lt3[2], 1.f}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT, am2); }
        rdPushMatrix();
        rdTranslatef((float)qx, (float)qy + sc * .5f, (float)qz);
        rdRotatef(p.age * 18.f, 0, 1, 0); rdScalef(sc, sc, sc); rdTranslatef(-.5f, -.5f, -.5f);
        drawMesh(meshOf(p.id));
        rdPopMatrix();
    }
    rdDisable(GL_COLOR_MATERIAL); rdDisable(GL_LIGHTING);
}
