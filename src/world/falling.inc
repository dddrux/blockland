// ================= BLOCOS QUE CAEM (patch5.60): areia e cascalho (FallingBlock do Minecraft) =================
// Sem apoio (ar, liquido, fogo, mato) o bloco vira entidade, cai (gravidade .04, arrasto .98) e pousa na celula de cima do primeiro obstaculo.
// So o host simula (runWorldTick); o convidado ve o bloco sumir e reaparecer (a animacao da queda nao e enviada).
struct FallEnt { double x, y, z, px, py, pz, my; int id, m; bool vis = false; };   // vis: so animacao no convidado (BUG-056)
static std::vector<FallEnt> gFallEnts;
static int gEggId = 0;   // patchEnd.1: "block/dragon_egg" do pack atual (cai como areia)
static std::vector<char> gPowder(65536, 0);   // patchLeves.1: ids dos "<cor>_concrete_powder" do pack (caem como areia)
static std::unordered_map<int, int> gPowderConc;   // BUG-008: id do "<cor>_concrete_powder" -> id do "<cor>_concrete" (vira concreto ao tocar agua)
static bool powderTouchWater(int x, int y, int z, bool wasWater = false) {   // BlockConcretePowder.tryTouchWater (1.12): agua em qualquer lado exceto embaixo
    auto it = gPowderConc.find(getB(x, y, z)); if (it == gPowderConc.end()) return false;
    static const int D[5][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}}; bool w = wasWater;
    for (auto& d : D) if (!w && getC(fdiv(x + d[0], CW), fdiv(z + d[2], CW)) && isWater(getB(x + d[0], y + d[1], z + d[2]))) w = true;
    if (!w) return false;
    const int m = getM(x, y, z); setBM(x, y, z, it->second, m); netSendBlock(x, y, z, it->second, m); return true;
}
static inline bool fallBlock(int b) { return b == SAND || b == GRAVEL || (gEggId && b == gEggId) || ((unsigned)b < 65536u && gPowder[b]); }
// patchEnd.1: BlockDragonEgg.teleport (1.12): 1000 tentativas a +-8/+-8/+-8... achar ar; o ovo some daqui e aparece la
static bool eggTeleport(int x, int y, int z) {
    if (!gEggId || getB(x, y, z) != gEggId) return false;
    for (int i = 0; i < 1000; i++) {
        const int nx = x + (rand() % 16) - (rand() % 16), ny = y + (rand() % 8) - (rand() % 8), nz = z + (rand() % 16) - (rand() % 16);
        if (ny < 1 || ny >= CH - 1 || !getC(fdiv(nx, CW), fdiv(nz, CW)) || getB(nx, ny, nz) != AIR) continue;
        setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); setBM(nx, ny, nz, gEggId, 0); netSendBlock(nx, ny, nz, gEggId, 0);
        scheduleTick(nx, ny, nz, 2); return true;
    }
    return false;
}
static inline bool fallPass(int b) { return b == AIR || isLiquid(b) || b == XB_FIRE || xbReplaceable(b); }
static void fallNetRecv(double x, double y, double z, int id, int m) { gFallEnts.push_back({x, y, z, x, y, z, 0., id, m, true}); }   // convidado: so a animacao (BUG-056)
static void fallTickAt(int x, int y, int z) {
    if (netOn && !netHost) return;   // BUG-056: so o host decide a queda; o convidado recebe NP_FALL + blocos
    const int b = getB(x, y, z); if (!fallBlock(b) || y <= 0 || !getC(fdiv(x, CW), fdiv(z, CW))) return;
    if (!fallPass(getB(x, y - 1, z))) return;
    gFallEnts.push_back({x + .5, (double)y, z + .5, x + .5, (double)y, z + .5, 0., b, getM(x, y, z)});
    if (netOn && netHost) { NBuf nb = netMk(NP_FALL); nb.put<double>(x + .5); nb.put<double>((double)y); nb.put<double>(z + .5); nb.put<int32_t>(b); nb.put<int32_t>(getM(x, y, z)); netBroadcast(nb); }
    setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0);
    if (fallBlock(getB(x, y + 1, z))) scheduleTick(x, y + 1, z, 2);   // a pilha de cima cai junto
}
// ---- patch5.62: esponja (BlockSponge 1.12): ao ser colocada ou quando agua encosta, absorve agua num raio de 6 passos (ate 65 blocos, ar tambem propaga) e vira esponja molhada ----
static int gSpongeId = 0, gWetSpongeId = 0;   // ids do pack atual ("block/sponge", "block/wet_sponge"); 0 = o pack nao tem
static int gSpongeBusy = 0;   // patchSponge.1: sem reentrada (cada agua removida avisa a propria esponja, que ainda era seca, e recomecava a absorver)
static void spongeAbsorb(int x, int y, int z) {
    if (!gSpongeId || getB(x, y, z) != gSpongeId || getM(x, y, z) != 0 || gSpongeBusy) return;   // so esponja SECA (meta 0) absorve
    struct Busy { Busy() { gSpongeBusy++; } ~Busy() { gSpongeBusy--; } } busyG;
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
    std::vector<std::array<int, 4>> q; q.push_back({x, y, z, 0}); std::unordered_set<int64_t> seen; seen.insert(k3(x, y, z)); int got = 0;
    for (size_t h = 0; h < q.size() && got < 65; h++) for (auto& d : D) {
        const int nx = q[h][0] + d[0], ny = q[h][1] + d[1], nz = q[h][2] + d[2], dist = q[h][3];
        if (ny < 0 || ny >= CH || !getC(fdiv(nx, CW), fdiv(nz, CW)) || !seen.insert(k3(nx, ny, nz)).second) continue;
        const int nb = getB(nx, ny, nz);
        if (isWater(nb)) { setBM(nx, ny, nz, AIR, 0); netSendBlock(nx, ny, nz, AIR, 0); if (++got >= 65) break; if (dist < 6) q.push_back({nx, ny, nz, dist + 1}); }
        else if (nb == AIR && dist < 6) q.push_back({nx, ny, nz, dist + 1});
    }
    if (got > 0) { const int wb = gWetSpongeId ? gWetSpongeId : gSpongeId, wm = gWetSpongeId ? 0 : 1; setBM(x, y, z, wb, wm); netSendBlock(x, y, z, wb, wm); }   // patchSponge.1: sem bloco molhado no pack, a propria esponja ganha meta 1 (molhada) e nunca mais absorve
}
static void spongeHooksInit() {
    if (gSpongeId) gHookSel[gSpongeId] = 0;   // pack anterior
    auto s = gPkByFile.find("block/sponge"), w = gPkByFile.find("block/wet_sponge");
    if (w == gPkByFile.end()) w = gPkByFile.find("block/sponge_wet");   // patchSponge.1: nome vanilla 1.12.2 quando o renome do pk112 nao se aplica
    gSpongeId = s != gPkByFile.end() ? s->second : 0; gWetSpongeId = w != gPkByFile.end() ? w->second : 0;
    if (!gSpongeId) return;
    BlockHooks h; h.added = [](int x, int y, int z) { spongeAbsorb(x, y, z); };
    h.neighbor = [](int x, int y, int z, int, int, int) { static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}}; for (auto& d : D) if (isWater(getB(x + d[0], y + d[1], z + d[2]))) { spongeAbsorb(x, y, z); break; } };
    blockHooksSet(gSpongeId, h);
}
// ---- patch5.63: vitoria-regia e vinha (XB 1004/1005; modelo/selecao/colisao em redstone.inc) ----
static bool lilyUse(Slot& s, const float* eye, const float* dir) {   // ItemLilyPad: o raio atravessa liquidos; so fonte de agua com ar por cima
    if (!s.id || xbItemBlock(s.id) != XB_LILY) return false;
    RayHit r; if (!rayCast(eye, dir, gBlockReach, r, true)) return true;
    const int x = r.x, y = r.y + 1, z = r.z; if (getB(r.x, r.y, r.z) != WATER || y >= CH || !xbFree(getB(x, y, z))) return true;
    setBM(x, y, z, XB_LILY, 0); netSendBlock(x, y, z, XB_LILY, 0); if (!creative && --s.n <= 0) s = Slot(); return true;
}
static void lilyNeighbor(int x, int y, int z, int, int, int) {   // sem fonte de agua embaixo: solta o item
    if (getB(x, y, z) != XB_LILY || getB(x, y - 1, z) == WATER) return;
    const int it = rsDropItem(x, y, z, XB_LILY); setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); if (it) spawnDrop(x + .5, y + .3, z + .5, 0, .1, 0, it, 1, 10);
}
static void vineNeighbor(int x, int y, int z, int, int, int) {   // cada lado precisa de bloco solido (ou da vinha de cima com o mesmo lado); sem nenhum lado sobra nada
    if (getB(x, y, z) != XB_VINE) return;
    static const int S[4][2] = {{0, 1}, {-1, 0}, {0, -1}, {1, 0}};
    const int m = getM(x, y, z); int nm = 0; const bool up = getB(x, y + 1, z) == XB_VINE; const int um = up ? getM(x, y + 1, z) : 0;
    for (int k = 0; k < 4; k++) if (((m >> k) & 1) && (solid(getB(x + S[k][0], y, z + S[k][1])) || ((um >> k) & 1))) nm |= 1 << k;
    if ((m & 16) && solid(getB(x, y + 1, z))) nm |= 16;
    if (nm == m) return;
    if (!nm) { setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); } else { setBM(x, y, z, XB_VINE, nm); netSendBlock(x, y, z, XB_VINE, nm); }
}
static void lilyVineHooksInit() { static bool done = false; if (done) return; done = true; BlockHooks a; a.neighbor = lilyNeighbor; blockHooksSet(XB_LILY, a); BlockHooks b; b.neighbor = vineNeighbor; blockHooksSet(XB_VINE, b); }
// ---- patch5.64: TNT (bloco do pack "block/tnt" + entidade TNT acesa; explosao = explosionAt de forca 4 em hostile.inc) ----
static void tntPrime(double x, double y, double z, int fuse) {   // EntityTNTPrimed: pulinho com leve desvio horizontal
    const double a = frand() * 6.2831853; TNTS.push_back({x, y, z, x, y, z, -sin(a) * .02, .2, -cos(a) * .02, fuse}); sndEvtAt("tnt.fuse", x, y, z);   // patchAud.OGG: pavio chiando
    if (netOn && netHost) { NBuf b = netMk(NP_TNT); b.put<double>(x); b.put<double>(y); b.put<double>(z); b.put<uint8_t>((uint8_t)std::max(0, std::min(fuse, 255))); netBroadcast(b); }   // o convidado so desenha
}
static void tntNetRecv(double x, double y, double z, int fuse) { const double a = 0; TNTS.push_back({x, y, z, x, y, z, a, .2, a, fuse}); sndEvtAt("tnt.fuse", x, y, z); }   // convidado: entidade so visual (nao explode; os blocos chegam por NP_BLOCKS)
static bool tntIgnite(int x, int y, int z) {   // so host/offline: troca o bloco pela entidade com 80 ticks de pavio
    if (!gTntId || getB(x, y, z) != gTntId || (netOn && !netHost)) return false;
    setB(x, y, z, AIR); netSendBlock(x, y, z, AIR); tntPrime(x + .5, y, z + .5, 80); return true;
}
static void tntNeighbor(int x, int y, int z, int, int, int) {   // fogo ao lado ou redstone ligada acendem
    if (!gTntId || getB(x, y, z) != gTntId) return;
    bool fire = false; for (int s = 0; s < 6; s++) if (getB(x + PS_DX[s], y + PS_DY[s], z + PS_DZ[s]) == XB_FIRE) fire = true;
    if (fire || rsBlockPowered(x, y, z, -99999, -99999, -99999)) tntIgnite(x, y, z);
}
static void tntHooksInit() {
    if (gTntId) gHookSel[gTntId] = 0;   // pack anterior
    auto t = gPkByFile.find("block/tnt"); gTntId = t != gPkByFile.end() ? t->second : 0; if (!gTntId) return;
    BlockHooks h; h.added = [](int x, int y, int z) { tntNeighbor(x, y, z, 0, 0, 0); }; h.neighbor = tntNeighbor; blockHooksSet(gTntId, h);
}
static void jukeInit(); static void logicInit();   // jukebox.inc / logic.inc
static void tntLiquidPush(TntEnt& t) {   // Entity.handleWaterMovement/handleLavaMovement: soma o vetor de fluxo das celulas tocadas e empurra
    const int x0 = (int)floor(t.x - .49), x1 = (int)floor(t.x + .49), z0 = (int)floor(t.z - .49), z1 = (int)floor(t.z + .49), y0 = (int)floor(t.y + .02), y1 = (int)floor(t.y + .96);
    double wx = 0, wz = 0, lx = 0, lz = 0; bool inW = false, inL = false;
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
        const int bl = getB(x, y, z);
        if (isWater(bl)) {
            double fx, fz; flowVector(x, y, z, fx, fz);
            if (liqMeta(bl) >= 8) { const double l = sqrt(fx * fx + fz * fz); if (l > 0) { fx /= l; fz /= l; } }
            wx += fx; wz += fz; inW = true;
        } else if (isLava(bl)) {
            const int i = std::min(7, lavaMeta(bl) & 7); double fx = 0, fz = 0; static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
            for (int d = 0; d < 4; d++) { const int nb = getB(x + DX[d], y, z + DZ[d]); if (isLava(nb)) { const int j = std::min(7, lavaMeta(nb) & 7); fx += DX[d] * (j - i); fz += DZ[d] * (j - i); } else if (!solid(nb) && isLava(getB(x + DX[d], y - 1, z + DZ[d]))) { fx += DX[d] * 8; fz += DZ[d] * 8; } }
            const double l = sqrt(fx * fx + fz * fz); if (l > 0) { lx += fx / l; lz += fz / l; } inL = true;
        }
    }
    double l = sqrt(wx * wx + wz * wz); if (inW && l > 0) { t.vx += wx / l * .014; t.vz += wz / l * .014; }
    l = sqrt(lx * lx + lz * lz); if (inL && l > 0) { t.vx += lx / l * .007; t.vz += lz / l * .007; }
}
static void tntTick(Phys& P) {   // host/offline: fisica simples + pavio; no fim explode (forca 4)
    if (TNTS.empty()) return; const bool guest = netOn && !netHost; int boomed = 0;   // patch5.71: no maximo TNT_MAX_BOOM explosoes por tick (as demais esperam 1 tick no pavio 0): muita TNT junta nao trava mais num quadro so   // convidado: so anima e some (a explosao em si vem do host)
    for (size_t i = 0; i < TNTS.size();) {
        TntEnt& t = TNTS[i]; t.px = t.x; t.py = t.y; t.pz = t.z;
        tntLiquidPush(t);   // patchTnt.1: correnteza empurra a TNT acesa (agua 0.014, lava 0.007); o bloco de TNT nao anda
        t.vy = std::max(t.vy - .04, -.9); t.x += t.vx; t.y += t.vy; t.z += t.vz; t.vx *= .98; t.vy *= .98; t.vz *= .98;
        const int bx = (int)floor(t.x), by = (int)floor(t.y), bz = (int)floor(t.z);
        if (solid(getB(bx, by, bz))) { t.y = by + 1; t.vy = 0; t.vx *= .7; t.vz *= .7; }
        if (t.y < -64) { TNTS.erase(TNTS.begin() + i); continue; }
        if (t.fuse > 0) --t.fuse;
        if (t.fuse <= 0) { if (!guest && boomed >= TNT_MAX_BOOM) { i++; continue; } boomed++; const TntEnt c = t; TNTS.erase(TNTS.begin() + i); if (!guest) explosionAt(c.x, c.y + .0625, c.z, 4.f, P, "explodido por TNT", nullptr); continue; }
        i++;
    }
}
static void tntDraw(float pt) {   // cubo do pack; pisca (brilho aditivo) a cada 5 ticks do pavio
    if (TNTS.empty() || !gTntId) return; glColor4f(1.f, 1.f, 1.f, 1.f);
    for (const TntEnt& t : TNTS) {
        const double ix = t.px + (t.x - t.px) * pt, iy = t.py + (t.y - t.py) * pt, iz = t.pz + (t.z - t.pz) * pt; entLightAt(ix, iy, iz, gEntDim);
        const float sc = .98f + (t.fuse < 10 ? (10 - t.fuse) * .01f : 0.f);
        rdPushMatrix(); rdTranslatef((float)ix, (float)iy, (float)iz); rdScalef(sc, sc, sc); rdTranslatef(-.5f, 0.f, -.5f); drawHeldRaw(gTntId); rdPopMatrix();
        if ((t.fuse / 5) % 2 == 0) { rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE); glColor4f(1.f, 1.f, 1.f, .55f);
            rdPushMatrix(); rdTranslatef((float)ix, (float)iy, (float)iz); rdScalef(sc + .01f, sc + .01f, sc + .01f); rdTranslatef(-.5f, 0.f, -.5f); drawHeldRaw(gTntId); rdPopMatrix();
            rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(1.f, 1.f, 1.f, 1.f); }
    }
}
static void fallHooksInit() {
    BlockHooks h; h.neighbor = [](int x, int y, int z, int, int, int) { if (powderTouchWater(x, y, z)) return; if (fallBlock(getB(x, y, z))) scheduleTick(x, y, z, 2); };
    h.added = [](int x, int y, int z) { if (powderTouchWater(x, y, z)) return; scheduleTick(x, y, z, 2); }; h.tick = fallTickAt;
    blockHooksSet(SAND, h); blockHooksSet(GRAVEL, h);
    gPowderConc.clear(); std::fill(gPowder.begin(), gPowder.end(), 0);
    for (const auto& kv : gPkByFile) { const std::string& f = kv.first; static const char K[] = "_concrete_powder"; if (f.size() > sizeof K && f.compare(f.size() - (sizeof K - 1), sizeof K - 1, K) == 0 && (unsigned)kv.second < 65536u) { gPowder[kv.second] = 1; blockHooksSet(kv.second, h); auto c = gPkByFile.find(f.substr(0, f.size() - 7)); if (c != gPkByFile.end()) gPowderConc[kv.second] = c->second; } }
    { auto it = gPkByFile.find("block/dragon_egg"); gEggId = it != gPkByFile.end() ? it->second : 0; if (gEggId) blockHooksSet(gEggId, h); }
    if (gCactusId) { BlockHooks c; c.neighbor = [](int x, int y, int z, int, int, int) {   /* patchMech.2: BlockCactus.canBlockStay (base some ou bloco solido ao lado = quebra e solta o item) */
            if (getB(x, y, z) != gCactusId) return; bool stay = solid(getB(x, y - 1, z)) || getB(x, y - 1, z) == gCactusId;
            static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1}; for (int s = 0; s < 4 && stay; s++) if (solid(getB(x + DX[s], y, z + DZ[s])) && getB(x + DX[s], y, z + DZ[s]) != gCactusId) stay = false;
            if (!stay) { blockDrops(x, y, z, gCactusId); setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); } };
        blockHooksSet(gCactusId, c); }
    spongeHooksInit(); lilyVineHooksInit(); tntHooksInit(); jukeInit(); logicInit();
}
static void fallTickAll() {
    for (size_t i = 0; i < gFallEnts.size();) {
        FallEnt& e = gFallEnts[i]; const int cx = (int)floor(e.x), cz = (int)floor(e.z);
        if (!getC(fdiv(cx, CW), fdiv(cz, CW))) { ++i; continue; }   // chunk descarregado: espera
        e.px = e.x; e.py = e.y; e.pz = e.z; e.my = (e.my - .04) * .98;
        double dy = e.my; int landY = -9999; bool gone = false;
        while (dy < 0) {
            const double st = std::max(dy, -.5), ny = e.y + st; const int cy = (int)floor(ny);
            if (cy < 0) { gone = true; break; }
            if (!fallPass(getB(cx, cy, cz))) { landY = cy + 1; break; }
            e.y = ny; dy -= st;
        }
        if (landY != -9999 && e.vis) gone = true;   // convidado: so visual, o bloco pousado chega pela rede
        else if (landY != -9999) {
            if (landY < CH && fallPass(getB(cx, landY, cz))) { const bool inWater = isWater(getB(cx, landY, cz)); setBM(cx, landY, cz, e.id, e.m); netSendBlock(cx, landY, cz, e.id, e.m); powderTouchWater(cx, landY, cz, inWater); gNoLiftUntil = SDL_GetTicks() + 60000u; }
            else if (landY < CH && e.id) spawnDrop(e.x, landY + .3, e.z, 0, .1, 0, e.id, 1, 10);   // patchEnt.1: celula ocupada (bloco colocado no meio): vira item, nao some
            gone = true;
        }
        if (gone) gFallEnts.erase(gFallEnts.begin() + i); else ++i;
    }
}
// patchEnt.1: blocos em queda e TNT acesa sao entidades solidas: nao se coloca bloco dentro delas (World.checkNoEntityCollision)
static bool fallEntBlocks(const AABB& bx) {
    for (const FallEnt& e : gFallEnts) if (AABB{e.x - .49, e.y, e.z - .49, e.x + .49, e.y + .98, e.z + .49}.intersects(bx)) return true;
    for (const TntEnt& t : TNTS) if (AABB{t.x - .49, t.y, t.z - .49, t.x + .49, t.y + .98, t.z + .49}.intersects(bx)) return true;
    return false;
}
static void drawFalling(float pt) {   // (VIS) cubo caindo, interpolado entre ticks; sem teste headless
    if (gFallEnts.empty()) return;
    static const float C[6][4][3] = {{{0,0,0},{0,0,1},{0,1,1},{0,1,0}}, {{1,0,1},{1,0,0},{1,1,0},{1,1,1}}, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    static const float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f); rdDisable(GL_CULL_FACE);
    for (const FallEnt& e : gFallEnts) {
        const float ox = (float)(e.px + (e.x - e.px) * pt - .49), oy = (float)(e.py + (e.y - e.py) * pt), oz = (float)(e.pz + (e.z - e.pz) * pt - .49);
        entLightAt(ox + .5, oy + .5, oz + .5, gEntDim); glColor3f(gEntDim[0], gEntDim[1], gEntDim[2]);
        glBegin(GL_QUADS);
        static const float FSH[6] = {.6f, .6f, .5f, 1.f, .8f, .8f};   // patchOpt.1: sombreamento por face (topo 1, Z .8, X .6, base .5) como o bloco no mundo
        for (int f = 0; f < 6; f++) { const int t = blkTile(e.id, f == 2 ? 2 : f == 3 ? 0 : 1); glColor3f(gEntDim[0] * FSH[f], gEntDim[1] * FSH[f], gEntDim[2] * FSH[f]);
            for (int k = 0; k < 4; k++) { glTexCoord2f((t % ATL + UV[k][0]) / (float)ATL, (t / ATL + UV[k][1]) / (float)ATLR); glVertex3f(ox + C[f][k][0] * .98f, oy + C[f][k][1] * .98f, oz + C[f][k][2] * .98f); } }
        glEnd();
    }
    entDimReset(); glColor4f(1.f, 1.f, 1.f, 1.f);
}
static void dragonWorldSave(const std::string& dir); static void dragonWorldLoad(const std::string& dir);   // dragon.inc (patchEnd.1)

static void killLooseEnts(int& k) {   // /kill @e: so o host (ou jogo local) remove
    if (netOn && !netHost) return;
    for (const DropEnt& d : DROPS) { k++; if (d.nid) netSendDropSet(d.nid, 0, 255); } DROPS.clear();
    k += (int)TNTS.size() + (int)gFallEnts.size(); TNTS.clear(); gFallEnts.clear();
}
static bool entCmdLocal() { return !netOn || netHost; }   // comandos em @e so mexem nas entidades no jogo local / no host

// ---- BUG-040: carrinhos do host visiveis ao convidado (NP_CARTS: host manda posicao/giro a cada 3 ticks; convidado so interpola e desenha) ----
static std::map<int, Uint32> gCartGhostSeen;
static void cartNetRecv(NBuf& b) {
    const int dim = b.get<uint8_t>(), n = b.get<uint8_t>(); const Uint32 now = SDL_GetTicks();
    for (int i = 0; i < n; i++) {
        const int id = b.get<int32_t>(), ty = b.get<uint8_t>(); const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(), yw = b.get<float>();
        if (dim != gDim || id <= 0) continue;
        CartEnt* g = nullptr; for (CartEnt& c : gCartGhost) if (c.id == id) { g = &c; break; }
        if (!g) { CartEnt c; c.id = id; c.x = c.px = x; c.y = c.py = y; c.z = c.pz = z; c.yaw = yw; gCartGhost.push_back(c); g = &gCartGhost.back(); }
        g->type = ty; g->mx = x; g->my = y; g->mz = z; g->pushX = yw; gCartGhostSeen[id] = now;
    }
}
static void cartNetTick() {
    static int tk = 0;
    if (!netOn) { if (!gCartGhost.empty()) { gCartGhost.clear(); gCartGhostSeen.clear(); } return; }
    if (netHost) {
        if (!gCartGhost.empty()) { gCartGhost.clear(); gCartGhostSeen.clear(); }
        if (++tk % 3 != 0 || CARTS.empty()) return;
        std::vector<const CartEnt*> v; for (const CartEnt& c : CARTS) if (!c.dead && v.size() < 40) v.push_back(&c);
        if (v.empty()) return;
        NBuf b = netMk(NP_CARTS); b.put<uint8_t>((uint8_t)gDim); b.put<uint8_t>((uint8_t)v.size());
        for (const CartEnt* c : v) { b.put<int32_t>(c->id); b.put<uint8_t>((uint8_t)c->type); b.put<float>((float)c->x); b.put<float>((float)c->y); b.put<float>((float)c->z); b.put<float>(c->yaw); }
        netBroadcast(b); return;
    }
    const Uint32 now = SDL_GetTicks();
    for (size_t i = 0; i < gCartGhost.size();) {
        CartEnt& g = gCartGhost[i]; const auto it = gCartGhostSeen.find(g.id);
        if (it == gCartGhostSeen.end() || now - it->second > 1500u) { if (it != gCartGhostSeen.end()) gCartGhostSeen.erase(it); gCartGhost.erase(gCartGhost.begin() + i); continue; }
        g.px = g.x; g.py = g.y; g.pz = g.z; g.x += (g.mx - g.x) * .4; g.y += (g.my - g.y) * .4; g.z += (g.mz - g.z) * .4;
        const float d = fmodf((float)g.pushX - g.yaw + 540.f, 360.f) - 180.f; g.yaw += d * .4f; ++i;
    }
}
