// ================= BLOCOS QUE CAEM (patch5.60): areia e cascalho (FallingBlock do Minecraft) =================
// Sem apoio (ar, liquido, fogo, mato) o bloco vira entidade, cai (gravidade .04, arrasto .98) e pousa na celula de cima do primeiro obstaculo.
// So o host simula (runWorldTick); o convidado ve o bloco sumir e reaparecer (a animacao da queda nao e enviada).
struct FallEnt { double x, y, z, px, py, pz, my; int id, m; };
static std::vector<FallEnt> gFallEnts;
static inline bool fallBlock(int b) { return b == SAND || b == GRAVEL; }
static inline bool fallPass(int b) { return b == AIR || isLiquid(b) || b == XB_FIRE || xbReplaceable(b); }
static void fallTickAt(int x, int y, int z) {
    const int b = getB(x, y, z); if (!fallBlock(b) || y <= 0 || !getC(fdiv(x, CW), fdiv(z, CW))) return;
    if (!fallPass(getB(x, y - 1, z))) return;
    gFallEnts.push_back({x + .5, (double)y, z + .5, x + .5, (double)y, z + .5, 0., b, getM(x, y, z)});
    setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0);
    if (fallBlock(getB(x, y + 1, z))) scheduleTick(x, y + 1, z, 2);   // a pilha de cima cai junto
}
// ---- patch5.62: esponja (BlockSponge 1.12): ao ser colocada ou quando agua encosta, absorve agua num raio de 6 passos (ate 65 blocos, ar tambem propaga) e vira esponja molhada ----
static int gSpongeId = 0, gWetSpongeId = 0;   // ids do pack atual ("block/sponge", "block/wet_sponge"); 0 = o pack nao tem
static void spongeAbsorb(int x, int y, int z) {
    if (!gSpongeId || getB(x, y, z) != gSpongeId) return;
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
    std::vector<std::array<int, 4>> q; q.push_back({x, y, z, 0}); std::unordered_set<int64_t> seen; seen.insert(k3(x, y, z)); int got = 0;
    for (size_t h = 0; h < q.size() && got < 65; h++) for (auto& d : D) {
        const int nx = q[h][0] + d[0], ny = q[h][1] + d[1], nz = q[h][2] + d[2], dist = q[h][3];
        if (ny < 0 || ny >= CH || !getC(fdiv(nx, CW), fdiv(nz, CW)) || !seen.insert(k3(nx, ny, nz)).second) continue;
        const int nb = getB(nx, ny, nz);
        if (isWater(nb)) { setBM(nx, ny, nz, AIR, 0); netSendBlock(nx, ny, nz, AIR, 0); if (++got >= 65) break; if (dist < 6) q.push_back({nx, ny, nz, dist + 1}); }
        else if (nb == AIR && dist < 6) q.push_back({nx, ny, nz, dist + 1});
    }
    if (got > 0 && gWetSpongeId) { setBM(x, y, z, gWetSpongeId, 0); netSendBlock(x, y, z, gWetSpongeId, 0); }
}
static void spongeHooksInit() {
    if (gSpongeId) gHookSel[gSpongeId] = 0;   // pack anterior
    auto s = gPkByFile.find("block/sponge"), w = gPkByFile.find("block/wet_sponge");
    gSpongeId = s != gPkByFile.end() ? s->second : 0; gWetSpongeId = w != gPkByFile.end() ? w->second : 0;
    if (!gSpongeId) return;
    BlockHooks h; h.added = [](int x, int y, int z) { spongeAbsorb(x, y, z); };
    h.neighbor = [](int x, int y, int z, int, int, int) { static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}}; for (auto& d : D) if (isWater(getB(x + d[0], y + d[1], z + d[2]))) { spongeAbsorb(x, y, z); break; } };
    blockHooksSet(gSpongeId, h);
}
// ---- patch5.63: vitoria-regia e vinha (XB 1004/1005; modelo/selecao/colisao em redstone.inc) ----
static bool lilyUse(Slot& s, const float* eye, const float* dir) {   // ItemLilyPad: o raio atravessa liquidos; so fonte de agua com ar por cima
    if (!s.id || xbItemBlock(s.id) != XB_LILY) return false;
    RayHit r; if (!rayCast(eye, dir, 5.f, r, true)) return true;
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
    const double a = frand() * 6.2831853; TNTS.push_back({x, y, z, x, y, z, -sin(a) * .02, .2, -cos(a) * .02, fuse});
    if (netOn && netHost) { NBuf b = netMk(NP_TNT); b.put<double>(x); b.put<double>(y); b.put<double>(z); b.put<uint8_t>((uint8_t)std::max(0, std::min(fuse, 255))); netBroadcast(b); }   // o convidado so desenha
}
static void tntNetRecv(double x, double y, double z, int fuse) { const double a = 0; TNTS.push_back({x, y, z, x, y, z, a, .2, a, fuse}); }   // convidado: entidade so visual (nao explode; os blocos chegam por NP_BLOCKS)
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
static void tntTick(Phys& P) {   // host/offline: fisica simples + pavio; no fim explode (forca 4)
    if (TNTS.empty()) return; const bool guest = netOn && !netHost; int boomed = 0;   // patch5.71: no maximo TNT_MAX_BOOM explosoes por tick (as demais esperam 1 tick no pavio 0): muita TNT junta nao trava mais num quadro so   // convidado: so anima e some (a explosao em si vem do host)
    for (size_t i = 0; i < TNTS.size();) {
        TntEnt& t = TNTS[i]; t.px = t.x; t.py = t.y; t.pz = t.z;
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
        glPushMatrix(); glTranslatef((float)ix, (float)iy, (float)iz); glScalef(sc, sc, sc); glTranslatef(-.5f, 0.f, -.5f); drawHeldRaw(gTntId); glPopMatrix();
        if ((t.fuse / 5) % 2 == 0) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glColor4f(1.f, 1.f, 1.f, .55f);
            glPushMatrix(); glTranslatef((float)ix, (float)iy, (float)iz); glScalef(sc + .01f, sc + .01f, sc + .01f); glTranslatef(-.5f, 0.f, -.5f); drawHeldRaw(gTntId); glPopMatrix();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(1.f, 1.f, 1.f, 1.f); }
    }
}
static void fallHooksInit() {
    BlockHooks h; h.neighbor = [](int x, int y, int z, int, int, int) { if (fallBlock(getB(x, y, z))) scheduleTick(x, y, z, 2); };
    h.added = [](int x, int y, int z) { scheduleTick(x, y, z, 2); }; h.tick = fallTickAt;
    blockHooksSet(SAND, h); blockHooksSet(GRAVEL, h); spongeHooksInit(); lilyVineHooksInit(); tntHooksInit();
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
        if (landY != -9999) {
            if (landY < CH && fallPass(getB(cx, landY, cz))) { setBM(cx, landY, cz, e.id, e.m); netSendBlock(cx, landY, cz, e.id, e.m); }
            gone = true;
        }
        if (gone) gFallEnts.erase(gFallEnts.begin() + i); else ++i;
    }
}
static void drawFalling(float pt) {   // (VIS) cubo caindo, interpolado entre ticks; sem teste headless
    if (gFallEnts.empty()) return;
    static const float C[6][4][3] = {{{0,0,0},{0,0,1},{0,1,1},{0,1,0}}, {{1,0,1},{1,0,0},{1,1,0},{1,1,1}}, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    static const float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glDisable(GL_CULL_FACE);
    for (const FallEnt& e : gFallEnts) {
        const float ox = (float)(e.px + (e.x - e.px) * pt - .49), oy = (float)(e.py + (e.y - e.py) * pt), oz = (float)(e.pz + (e.z - e.pz) * pt - .49);
        entLightAt(ox + .5, oy + .5, oz + .5, gEntDim); glColor3f(gEntDim[0], gEntDim[1], gEntDim[2]);
        glBegin(GL_QUADS);
        for (int f = 0; f < 6; f++) { const int t = blkTile(e.id, f == 2 ? 2 : f == 3 ? 0 : 1);
            for (int k = 0; k < 4; k++) { glTexCoord2f((t % ATL + UV[k][0]) / (float)ATL, (t / ATL + UV[k][1]) / (float)ATLR); glVertex3f(ox + C[f][k][0] * .98f, oy + C[f][k][1] * .98f, oz + C[f][k][2] * .98f); } }
        glEnd();
    }
    entDimReset(); glColor4f(1.f, 1.f, 1.f, 1.f);
}
