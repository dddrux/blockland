// ---------- Nether: dimensao, geracao basica do terreno, fogo (isqueiro), portal e animacoes ----------
// Dimensao = outro conjunto de chunks: ao trocar, o mundo atual (W, savedC, savedM, gFloor, gSpill) vai para gNs e o da outra dimensao volta.
// Salvo em saves/<mundo>/chunks16.dat (superficie) e chunks16n.dat (nether) + dim.txt (dimensao do jogador). Nao sincroniza dimensao no multiplayer.
struct NetherStash {
    std::unordered_map<int64_t, Chunk*> W; std::unordered_map<int64_t, std::vector<BT>> sc; std::unordered_map<int64_t, std::vector<uint8_t>> sm;
    std::unordered_map<int64_t, std::array<int16_t, 256>> fl; std::unordered_map<int64_t, std::vector<std::array<int, 4>>> sp; bool loaded[2] = {true, false};
};
static NetherStash gNs3[3];   // gNs3[d] = mundo da dimensao d enquanto ela NAO e a atual (0 superficie, 1 Nether, 2 End)
static bool gDimLoaded[3] = {true, false, false};
static int gNetherrackId = 0;
static std::unordered_map<int64_t, Chunk*>& dimWorld(int d) { return d == gDim ? W : gNs3[d].W; }
static std::unordered_map<int64_t, Chunk*>& netherOtherW() { return gNs3[gDim ? 0 : 1].W; }
static void dimSwapAll(NetherStash& n) { std::swap(W, n.W); std::swap(savedC, n.sc); std::swap(savedM, n.sm); std::swap(gFloor, n.fl); std::swap(gSpill, n.sp); }
static void dimSwitch(int nd) {
    if (nd == gDim) return;
    gChest = false; gChEnder = false; gChWait = false;
    resetBlockUpdates(); wqCur.clear(); wqNext.clear(); wqSet.clear(); lqCur.clear(); lqNext.clear(); lqSet.clear();
    MOBS.clear(); BOATS.clear(); gBoatRide = 0; gBoatCamId = 0; DROPS.clear();
    { const int od = gDim; dimSwapAll(gNs3[nd]); std::swap(gNs3[nd], gNs3[od]); gDim = nd; }   // atual <-> pilha de nd; os dados antigos vao para a pilha od
    if (!gDimLoaded[nd]) {   // primeira vez nesta sessao: le os chunks salvos dessa dimensao (se existirem)
        gDimLoaded[nd] = true;
        const std::string fn = nd == 2 ? "chunks16e.dat" : nd ? "chunks16n.dat" : "chunks16.dat";
        if (FILE* f = fopen(("saves/" + gCurDir + "/" + fn).c_str(), "rb")) { fclose(f); gChunkFile = nd == 2 ? "chunks16e.dat" : nd ? "chunks16n.dat" : "chunks16.dat"; loadSavedChunks(gCurDir); gChunkFile = "chunks16.dat"; }
    }
    gKick = true; gEpoch++;
}
static void netherReset() {   // resetWorldData(): libera a outra dimensao guardada
    if (gDim) { dimSwapAll(gNs3[0]); std::swap(gNs3[0], gNs3[gDim]); }   // volta a superficie ao vivo; o resto vai para as pilhas e e liberado abaixo
    gDim = 0;
    static auto pDel = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    for (int d = 0; d < 3; d++) { NetherStash& n = gNs3[d]; for (auto& kv : n.W) { Chunk* c = kv.second; if (pDel) { if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo); } delete[] c->lc; delete c; }
        n.W.clear(); n.sc.clear(); n.sm.clear(); n.fl.clear(); n.sp.clear(); }
    gDimLoaded[0] = true; gDimLoaded[1] = gDimLoaded[2] = false;
}
static void netherOnWorldLoad(const std::string& dir) {   // apos carregar o mundo: volta para a dimensao em que o jogador estava
    int d = 0; if (FILE* f = fopen(("saves/" + dir + "/dim.txt").c_str(), "rb")) { if (fscanf(f, "%d", &d) != 1) d = 0; fclose(f); }
    if (d == 1 || d == 2) dimSwitch(d);
}

// ---- terreno basico do Nether (ruido de valor 3D em grade 4x4x4 interpolada; teto e piso de bedrock, mar de lava em y<=31) ----
static inline uint32_t nhash(int x, int y, int z, uint32_t s) { uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)z * 2147483629u + s * 1274126177u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static float nnoise(float x, float y, float z, uint32_t s) {
    const int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z); float fx = x - xi, fy = y - yi, fz = z - zi;
    fx = fx * fx * (3.f - 2.f * fx); fy = fy * fy * (3.f - 2.f * fy); fz = fz * fz * (3.f - 2.f * fz);
    auto v = [&](int a, int b, int c) { return (nhash(xi + a, yi + b, zi + c, s) & 0xFFFF) / 32767.5f - 1.f; };
    const float x00 = lerpf(v(0, 0, 0), v(1, 0, 0), fx), x10 = lerpf(v(0, 1, 0), v(1, 1, 0), fx), x01 = lerpf(v(0, 0, 1), v(1, 0, 1), fx), x11 = lerpf(v(0, 1, 1), v(1, 1, 1), fx);
    return lerpf(lerpf(x00, x10, fy), lerpf(x01, x11, fy), fz);
}
static float nDens(float wx, float y, float wz, uint32_t sd) {
    const float s = (nnoise(wx * .017f, y * .028f, wz * .017f, sd) * .6f + nnoise(wx * .036f, y * .06f, wz * .036f, sd + 7) * .3f + nnoise(wx * .08f, y * .13f, wz * .08f, sd + 13) * .1f) * 1.9f;
    const float bot = std::max(0.f, std::min(1.6f, (31.f - y) / 14.f)), top = std::max(0.f, std::min(1.6f, (y - 96.f) / 20.f));
    return s + bot + top;
}

// ---- Nether 1.12.2 (patchNether.3): terreno (nether112.inc), fortaleza por pecas (nether_fort.inc) ----
static int gNBrickId = 0, gSpawnerId = 0, gNSoul = 0;
static std::vector<std::array<int, 3>> gFortChests;   // baus gerados que ainda esperam o loot (fortChestFlush)
static uint32_t netherSd() { return (uint32_t)((uint64_t)WORLD_SEED ^ ((uint64_t)WORLD_SEED >> 32)) ^ 0x4E455448u; }
#include "nether112.inc"
#include "nether_fort.inc"
// spawner da sala do trono: blazes nascem perto dele quando o jogador esta a < 16 blocos (ate 6 por perto)
static void blazeSpawnTick(Phys& P) {
    if (gDim != 1 || gDead || (netOn && !netHost)) return;
    static int t = 0; if (++t < 20) return; t = 0; fortChestFlush();
    static std::map<std::array<int, 3>, int> timer;
    std::vector<std::array<double, 3>> pls; activePlayers(P, pls);
    for (const auto& pl : pls) { std::vector<std::array<int, 3>> sp; fortSpawners(pl[0], pl[2], sp);
        for (const auto& s : sp) {
            const int sx = s[0], sy = s[1], sz = s[2];
            if (std::fabs(pl[0] - sx - .5) > 16 || std::fabs(pl[2] - sz - .5) > 16 || std::fabs(pl[1] - sy) > 12) continue;
            if (getB(sx, sy, sz) != gSpawnerId || !gSpawnerId) continue;
            int& tm = timer[s]; if (tm <= 0) tm = 100 + rand() % 200; tm -= 20; if (tm > 0) continue; tm = 200 + rand() % 400;
            int near = 0; for (const MobEnt& e : MOBS) if (e.type == M_BLAZE && !e.dead && std::fabs(e.x - sx) < 8 && std::fabs(e.z - sz) < 8) near++;
            for (int k = 0, n = 1 + rand() % 3; k < n && near < 6; k++) {
                for (int tr = 0; tr < 16; tr++) {   // sala do trono e pequena: tenta varias posicoes livres
                    const double x = sx + .5 + (rand() % 7 - 3), z = sz + .5 + (rand() % 7 - 3), y = sy + rand() % 3;
                    if (solid(getB((int)floor(x), (int)floor(y), (int)floor(z))) || solid(getB((int)floor(x), (int)floor(y) + 1, (int)floor(z)))) continue;
                    spawnMob(M_BLAZE, x, y, z, frand() * 360.f); near++; break;
                }
            }
        } }
}
static void netherGen(Chunk* c, int cx, int cz) {
    if (!gNetherrackId) gNetherrackId = pkFind("block/netherrack") ? pkFind("block/netherrack") : STONE;
    gNSoul = pkFind("block/soul_sand") ? pkFind("block/soul_sand") : SAND;
    netherBase(c, cx, cz); netherFortress(c, cx, cz); netherDecor(c, cx, cz);
}

// ---- fogo ----
static bool fireFlam(int b) { return b == PLANKS || b == LOG || b == LEAVES || b == WOOL || b == BOOKSHELF || isLeafB(b) || isLogB(b); }
static bool fireBaseOk(int x, int y, int z) {
    if (solid(getB(x, y - 1, z))) return true;
    static const int D[5][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}};
    for (auto& d : D) if (fireFlam(getB(x + d[0], y + d[1], z + d[2]))) return true;
    return false;
}
static int fireSideMask(int x, int y, int z) {   // sem chao: chamas coladas nas paredes inflamaveis (bit 1 +X, 2 -X, 4 +Z, 8 -Z); com chao: 0 = chamas no chao
    if (solid(getB(x, y - 1, z))) return 0;
    return (fireFlam(getB(x + 1, y, z)) ? 1 : 0) | (fireFlam(getB(x - 1, y, z)) ? 2 : 0) | (fireFlam(getB(x, y, z + 1)) ? 4 : 0) | (fireFlam(getB(x, y, z - 1)) ? 8 : 0);
}
static void fireSync(int x, int y, int z) { const int m = fireSideMask(x, y, z); if ((getM(x, y, z) & 15) != m) { setBM(x, y, z, XB_FIRE, m); netSendBlock(x, y, z, XB_FIRE, m); } }
static void fireNeighbor(int x, int y, int z, int, int, int) {
    if (getB(x, y, z) != XB_FIRE) return;
    if (!fireBaseOk(x, y, z) || isWater(getB(x, y + 1, z))) { setB(x, y, z, AIR); netSendBlock(x, y, z, AIR); return; }
    fireSync(x, y, z);
}
static void fireAdded(int x, int y, int z) { scheduleTick(x, y, z, 30 + rand() % 10); fireSync(x, y, z); }
static void fireTick(int x, int y, int z) {
    if (getB(x, y, z) != XB_FIRE) return;
    if (!fireBaseOk(x, y, z) || isWater(getB(x, y + 1, z))) { setB(x, y, z, AIR); netSendBlock(x, y, z, AIR); return; }   // sem chao nem inflamavel ao lado: some (antes ficava voando)
    fireSync(x, y, z);
    const bool eternal = gNetherrackId && getB(x, y - 1, z) == gNetherrackId; bool nearFlam = false;
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
    for (auto& d : D) {
        const int nx = x + d[0], ny = y + d[1], nz = z + d[2], nb = getB(nx, ny, nz);
        if (!fireFlam(nb)) continue;
        nearFlam = true;
        if (rand() % 3 == 0) { setB(nx, ny, nz, XB_FIRE); netSendBlock(nx, ny, nz, XB_FIRE); }   // queima o bloco inflamavel e o fogo se espalha
    }
    if (!eternal && !nearFlam && rand() % 4 == 0) { setB(x, y, z, AIR); netSendBlock(x, y, z, AIR); return; }
    scheduleTick(x, y, z, 30 + rand() % 10);
}

// ---- portal: moldura de obsidiana (interior 2..21 x 3..21), meta bit0 = eixo (0 = plano em X, 1 = plano em Z) ----

// ---- patch5.61: lava incendeia inflamaveis (BlockStaticLiquid.updateTick simplificado) ----
// A cada 20 ticks (so host/offline): por chunk carregado, 4 colunas ao acaso; cada celula de lava da coluna tenta 3 vezes acender o ar ao lado/acima que encosta em bloco inflamavel.
static void lavaIgniteTick() {
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}};
    for (auto& kv : W) {
        const Chunk* c = kv.second; if (!c) continue;
        const int cx = (int)(kv.first >> 32), cz = (int)(uint32_t)(kv.first & 0xFFFFFFFFu);
        for (int s = 0; s < 4; s++) {
            const int lx = rand() & 15, lz = rand() & 15;
            for (int y = 1; y < CH - 1; y++) {
                if (!isLava(c->b[idx(lx, y, lz)])) continue;
                const int x = cx * CW + lx, z = cz * CW + lz;
                for (int k = 0; k < 3; k++) {
                    const int fx = x + rand() % 3 - 1, fy = y + rand() % 2, fz = z + rand() % 3 - 1;
                    if (!getC(fdiv(fx, CW), fdiv(fz, CW)) || getB(fx, fy, fz) != AIR) continue;
                    bool flam = false; for (auto& d : D) if (fireFlam(getB(fx + d[0], fy + d[1], fz + d[2]))) { flam = true; break; }
                    if (flam) { setB(fx, fy, fz, XB_FIRE); netSendBlock(fx, fy, fz, XB_FIRE); break; }
                }
            }
        }
    }
}
static inline bool nEmpty(int b) { return b == AIR || b == XB_FIRE; }
static bool portalFrame(int x, int y, int z, int ax, int& bx, int& by, int& bz, int& w, int& h) {
    const int dx = ax == 0 ? 1 : 0, dz = ax == 0 ? 0 : 1;
    int n = 0; while (n++ < 22 && y > 1 && nEmpty(getB(x, y - 1, z))) y--;
    if (getB(x, y - 1, z) != OBSIDIAN || !nEmpty(getB(x, y, z))) return false;
    n = 0; while (n++ < 22 && nEmpty(getB(x - dx, y, z - dz)) && getB(x - dx, y - 1, z - dz) == OBSIDIAN) { x -= dx; z -= dz; }
    w = 0; while (w < 22 && nEmpty(getB(x + dx * w, y, z + dz * w)) && getB(x + dx * w, y - 1, z + dz * w) == OBSIDIAN) w++;
    if (w < 2 || w > 21 || getB(x + dx * w, y, z + dz * w) != OBSIDIAN || getB(x - dx, y, z - dz) != OBSIDIAN) return false;
    for (h = 0; h < 22; h++) {
        if (getB(x, y + h, z) == OBSIDIAN) { for (int i = 0; i < w; i++) if (getB(x + dx * i, y + h, z + dz * i) != OBSIDIAN) return false; break; }
        for (int i = 0; i < w; i++) if (!nEmpty(getB(x + dx * i, y + h, z + dz * i))) return false;
        if (getB(x - dx, y + h, z - dz) != OBSIDIAN || getB(x + dx * w, y + h, z + dz * w) != OBSIDIAN) return false;
    }
    if (h < 3 || h > 21) return false;
    bx = x; by = y; bz = z; return true;
}
static void portalFill(int bx, int by, int bz, int ax, int w, int h) {
    const int dx = ax == 0 ? 1 : 0, dz = ax == 0 ? 0 : 1;
    gNoBlockHooks = true;   // sem cascata: enquanto enche, vizinhos ainda vazios derrubariam o portal
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) { const int X = bx + dx * i, Y = by + j, Z = bz + dz * i; setBM(X, Y, Z, XB_PORTAL, ax); netSendBlock(X, Y, Z, XB_PORTAL, ax); }
    gNoBlockHooks = false;
}
static bool tryLightPortal(int x, int y, int z) {
    for (int ax = 0; ax < 2; ax++) { int bx, by, bz, w, h; if (portalFrame(x, y, z, ax, bx, by, bz, w, h)) { portalFill(bx, by, bz, ax, w, h); return true; } }
    return false;
}
static void portalNeighbor(int x, int y, int z, int, int, int) {
    if (getB(x, y, z) != XB_PORTAL || (getM(x, y, z) & 2)) return;   // meta bit1 = portal do End (nao cai com vizinhos)
    const int ax = getM(x, y, z) & 1, dx = ax == 0 ? 1 : 0, dz = ax == 0 ? 0 : 1;
    auto ok = [&](int a, int b, int c) { const int t = getB(a, b, c); return t == OBSIDIAN || t == XB_PORTAL; };
    if (!(ok(x, y - 1, z) && ok(x, y + 1, z) && ok(x - dx, y, z - dz) && ok(x + dx, y, z + dz))) { setB(x, y, z, AIR); netSendBlock(x, y, z, AIR); }
}
// ---- portal do End: 12 molduras com olho em volta de 3x3 abrem o portal (BlockEndPortalFrame.PORTAL_PATTERN); meta do portal bit1 = End ----
static bool tryOpenEnd(int x, int y, int z) {
    if (!gEndFrameId) return false;
    auto fr = [&](int a, int c) { return getB(a, y, c) == gEndFrameId && (getM(a, y, c) & 4); };
    for (int sx = -1; sx <= 1; sx += 2) for (int t = -1; t <= 1; t++) for (int ax = 0; ax < 2; ax++) {   // o encaixe atual pode ser qualquer um dos 12: testa todos os centros possiveis
        const int cx = ax == 0 ? x - 2 * sx : x - t, cz = ax == 0 ? z - t : z - 2 * sx;
        bool ok = true;
        for (int k = -1; k <= 1 && ok; k++) ok = fr(cx + k, cz - 2) && fr(cx + k, cz + 2) && fr(cx - 2, cz + k) && fr(cx + 2, cz + k);
        if (!ok) continue;
        for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) { setBM(cx + dx, y, cz + dz, XB_PORTAL, 2); netSendBlock(cx + dx, y, cz + dz, XB_PORTAL, 2); }
        return true;
    }
    return false;
}
// isqueiro (flint and steel): fogo na celula da face mirada; se fechar uma moldura de obsidiana, abre o portal. Desgaste 65 usos (Slot.d)
static bool tntIgnite(int x, int y, int z);   // falling.inc
static bool flintUse(Slot& s, const float* eye, const float* dir) {
    if (!gFlintSteel || s.id != gFlintSteel) return false;
    RayHit r; if (!rayCast(eye, dir, 5.f, r, false)) return true;
    if (tntIgnite(r.x, r.y, r.z)) { if (!creative && ++s.d >= 65) s = Slot(); return true; }   // isqueiro em TNT: acende
    const int tx = r.x + r.nx, ty = r.y + r.ny, tz = r.z + r.nz;
    if (ty < 1 || ty >= CH - 1 || getB(tx, ty, tz) != AIR || !fireBaseOk(tx, ty, tz)) return true;
    setBM(tx, ty, tz, XB_FIRE, 0); netSendBlock(tx, ty, tz, XB_FIRE); sndEvtAt("fire.ignite", tx + .5, ty + .5, tz + .5);
    tryLightPortal(tx, ty, tz);
    if (!creative && ++s.d >= 65) s = Slot();
    return true;
}

// ---- viagem entre dimensoes ----
static bool findPortal(int X, int Z, int& px, int& py, int& pz, int& ax) {
    int best = 1 << 30; bool f = false;
    for (int dz = -16; dz <= 16; dz++) for (int dx = -16; dx <= 16; dx++) {
        const int d2 = dx * dx + dz * dz; if (d2 >= best) continue;
        for (int y = 1; y < 126; y++) if (getB(X + dx, y, Z + dz) == XB_PORTAL) { int yy = y; while (yy > 1 && getB(X + dx, yy - 1, Z + dz) == XB_PORTAL) yy--; px = X + dx; py = yy; pz = Z + dz; ax = getM(px, py, pz) & 1; best = d2; f = true; break; }
    }
    return f;
}
static void createPortal(int X, int Z, int& px, int& py, int& pz, int& ax) {
    int y0 = -1;
    if (gDim == 1) {
        for (int y = 110; y >= 33 && y0 < 0; y--) {
            if (!solid(getB(X, y - 1, Z)) || isLiquid(getB(X, y - 1, Z))) continue;
            bool clear = true; for (int yy = y; yy < y + 4 && clear; yy++) for (int z = Z - 1; z <= Z + 1; z++) for (int x = X - 1; x <= X + 2; x++) { const int b = getB(x, yy, z); if (solid(b) || isLiquid(b)) clear = false; }
            if (clear) y0 = y;
        }
        if (y0 < 0) y0 = 70;
    } else {
        for (int y = CH - 2; y > 1; y--) { const int b = getB(X, y, Z); if (isWater(b)) { y0 = SEA + 1; break; } if (solid(b) && !isLeafB(b)) { y0 = y + 1; break; } }
        if (y0 < 2 || y0 > CH - 8) y0 = 64;
    }
    gNoBlockHooks = true;
    // setBM + netSendBlock: antes a moldura so era colocada LOCALMENTE e o portal (portalFill) ia pela rede => os outros jogadores viam o portal sem as obsidianas
    auto put = [](int x, int y, int z, int id) { setBM(x, y, z, id, 0); netSendBlock(x, y, z, id, 0); };
    for (int z = Z - 1; z <= Z + 1; z++) for (int x = X - 1; x <= X + 2; x++) for (int y = y0 - 1; y <= y0 + 3; y++) put(x, y, z, y == y0 - 1 ? OBSIDIAN : AIR);
    for (int y = y0 - 1; y <= y0 + 3; y++) { put(X - 1, y, Z, OBSIDIAN); put(X + 2, y, Z, OBSIDIAN); }
    for (int x = X; x <= X + 1; x++) { put(x, y0 - 1, Z, OBSIDIAN); put(x, y0 + 3, Z, OBSIDIAN); }
    gNoBlockHooks = false;
    portalFill(X, y0, Z, 0, 2, 3);
    px = X; py = y0; pz = Z; ax = 0;
}
static float gNetherYaw = 0.f, gNetherPitch = 0.f; static bool gNetherTurn = false;   // main_loop aplica ao yaw/pitch da camera
static void netherTravel(Phys& P) {
    const int nd = gDim ? 0 : 1; const double k = nd ? 1.0 / 8.0 : 8.0;
    const int X = (int)floor(P.x * k), Z = (int)floor(P.z * k);
    dimSwitch(nd);
    for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) if (!getC(fdiv(X, CW) + dx, fdiv(Z, CW) + dz)) gen(fdiv(X, CW) + dx, fdiv(Z, CW) + dz);
    int px, py, pz, ax;
    if (!findPortal(X, Z, px, py, pz, ax)) createPortal(X, Z, px, py, pz, ax);
    { const int dx = ax == 0 ? 1 : 0, dz = ax == 0 ? 0 : 1; int lo = 0, hi = 0;   // largura do portal: o jogador sai no meio dele (como no Minecraft), dentro do plano
      while (lo < 22 && getB(px - dx * (lo + 1), py, pz - dz * (lo + 1)) == XB_PORTAL) lo++;
      while (hi < 22 && getB(px + dx * (hi + 1), py, pz + dz * (hi + 1)) == XB_PORTAL) hi++;
      const double off = (hi - lo) * .5; P.x = P.px = px + .5 + dx * off; P.z = P.pz = pz + .5 + dz * off; P.y = P.py = py + .01; }
    {   // vira o jogador para FORA do portal (perpendicular ao plano, pelo lado livre), nunca para a parede; forward = (-sin yaw, -cos yaw)
        const int nx = ax == 0 ? 0 : 1, nz = ax == 0 ? 1 : 0;
        const bool pos = !solid(getB(px + nx, py, pz + nz)) || solid(getB(px - nx, py, pz - nz));
        gNetherYaw = ax == 0 ? (pos ? 180.f : 0.f) : (pos ? -90.f : 90.f); gNetherPitch = 0.f; gNetherTurn = true;
    }
    P.mx = P.my = P.mz = 0; P.onGround = false; gFall = 0.f; gFire = 0; gKick = true;
}

// ---------- The End (ChunkGeneratorEnd + WorldGenSpikes + WorldGenEndPodium, sem dragao/crystals/enderman) ----------
// ilha principal: f = 100 - dist (clamp -100..80); ilhas externas: chunks com k*k+l*l > 4096 sorteados (no vanilla: simplex < -0.9), f6 = 100 - dist/8*f3, f3 = (|k|*3439+|l|*147)%13+9
static float endTopAt(int wx, int wz, uint32_t sd) { return 62.f + 2.5f * nnoise(wx * .07f, .5f, wz * .07f, sd) + 7.f * sqrtf(std::max(0.f, std::min(1.f, (100.f - sqrtf((float)(wx * wx + wz * wz))) / 80.f))); }
static void endGen(Chunk* c, int cx, int cz) {
    const uint32_t sd = ((uint32_t)((uint64_t)WORLD_SEED ^ ((uint64_t)WORLD_SEED >> 32))) ^ 0x454E4400u;
    const int es = pkFind("block/end_stone"), stone = es ? es : STONE;
    struct Isl { int x, z; float f3; }; std::vector<Isl> isl;
    if (cx * cx + cz * cz > 39 * 39) for (int i = -12; i <= 12; i++) for (int j = -12; j <= 12; j++) { const int k = cx + i, l = cz + j; if (k * k + l * l > 4096 && (nhash(k, l, 0, sd + 5) & 1023u) < 30u) isl.push_back({k * 16 + 8, l * 16 + 8, (float)((abs(k) * 3439 + abs(l) * 147) % 13 + 9)}); }
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
        const int wx = cx * CW + x, wz = cz * CW + z;
        float h = std::max(-100.f, std::min(80.f, 100.f - sqrtf((float)(wx * wx + wz * wz))));
        for (const Isl& I : isl) { const float dx = (float)(wx - I.x), dz = (float)(wz - I.z); h = std::max(h, std::max(-100.f, std::min(80.f, 100.f - sqrtf(dx * dx + dz * dz) / 8.f * I.f3))); }
        if (h <= 0.f) continue;
        const float s = h / 80.f, top = endTopAt(wx, wz, sd), thick = 38.f * powf(s, .85f) + 4.f * nnoise(wx * .02f + 9.f, .5f, wz * .02f, sd + 3) * s;
        if (thick < 1.5f) continue;
        const float bot = top - thick;
        for (int y = std::max(1, (int)floorf(bot - 3.f)); y <= std::min(126, (int)top); y++) if (y >= bot + 3.f * nnoise(wx * .09f, y * .09f, wz * .09f, sd + 11)) c->b[idx(x, y, z)] = (BT)stone;
    }
    {   // WorldGenEndIsland (ilhotas flutuantes, 1/14 por chunk, 2a ilhota 1/4; so a partir de 1024 blocos): camadas de raio f = 3..6 que encolhe 0,5 ou 1,5 por camada; reconstruido da memoria do codigo 1.12.2 (a fonte nao estava acessivel)
        for (int ncz = cz - 1; ncz <= cz + 1; ncz++) for (int ncx = cx - 1; ncx <= cx + 1; ncx++) {
            if ((long)ncx * ncx + (long)ncz * ncz <= 4096L || nhash(ncx, ncz, 0, sd + 90) % 14u) continue;
            for (int n = 0; n < 2; n++) {
                if (n == 1 && nhash(ncx, ncz, 1, sd + 90) % 4u) break;
                const uint32_t q = (uint32_t)(n * 8 + 2); const int ix = ncx * CW + 8 + (int)(nhash(ncx, ncz, q, sd + 92) % 16u), iy = 55 + (int)(nhash(ncx, ncz, q + 1, sd + 92) % 16u), iz = ncz * CW + 8 + (int)(nhash(ncx, ncz, q + 2, sd + 92) % 16u);
                float f = (float)(3 + nhash(ncx, ncz, q + 3, sd + 92) % 3u + 1); int lay = 0;
                for (int i = 0; f > 0.5f; --i, lay++) {
                    for (int j = (int)floorf(-f); j <= (int)ceilf(f); j++) for (int k = (int)floorf(-f); k <= (int)ceilf(f); k++) {
                        if ((float)(j * j + k * k) > (f + 1.f) * (f + 1.f)) continue;
                        const int lx = ix + j - cx * CW, lz = iz + k - cz * CW, y = iy + i; if (lx < 0 || lx >= CW || lz < 0 || lz >= CW || y < 1 || y >= CH - 1) continue;
                        c->b[idx(lx, y, lz)] = (BT)stone;
                    }
                    f = (float)((double)f - ((double)(nhash(ncx, ncz, q + 4 + (uint32_t)lay, sd + 92) % 2u) + 0.5));
                }
            }
        }
    }
    {   // 10 pilares de obsidiana em circulo de raio 42 (raio 2 + i/3, altura 76 + 3i, i embaralhado); topo de bedrock com fogo (no lugar do cristal)
        int ord[10]; for (int i = 0; i < 10; i++) ord[i] = i; for (int i = 9; i > 0; i--) std::swap(ord[i], ord[nhash(i, 0, 0, sd + 71) % (uint32_t)(i + 1)]);
        for (int j = 0; j < 10; j++) {
            const double a = 2.0 * (-3.14159265358979 + 3.14159265358979 / 10.0 * j); const int px = (int)floor(42.0 * cos(a)), pz = (int)floor(42.0 * sin(a)), r = 2 + ord[j] / 3, hgt = 76 + ord[j] * 3;
            for (int dz = -r; dz <= r; dz++) for (int dx = -r; dx <= r; dx++) {
                if (dx * dx + dz * dz > r * r + 1) continue;
                const int lx = px + dx - cx * CW, lz = pz + dz - cz * CW; if (lx < 0 || lx >= CW || lz < 0 || lz >= CW) continue;
                for (int y = 0; y < hgt; y++) c->b[idx(lx, y, lz)] = OBSIDIAN;
                for (int y = hgt; y < std::min(CH - 1, hgt + 10); y++) c->b[idx(lx, y, lz)] = AIR;
                if (!dx && !dz) { c->b[idx(lx, hgt, lz)] = BEDROCK; c->b[idx(lx, hgt + 1, lz)] = (BT)XB_FIRE; }
            }
            if (ord[j] < 3) {   // patchEnd.10: pilares guardados (WorldGenSpikes isGuarded): gaiola de barras de ferro 5x5, 4 de altura, com teto
                auto cage = [&](int jj, int kk, int ll) { return std::abs(jj) <= 2 && std::abs(kk) <= 2 && ll >= 0 && ll <= 3 && (std::abs(jj) == 2 || std::abs(kk) == 2 || ll == 3); };
                for (int ll = 0; ll <= 3; ll++) for (int kk = -2; kk <= 2; kk++) for (int jj = -2; jj <= 2; jj++) {
                    if (!cage(jj, kk, ll)) continue; const int lx = px + jj - cx * CW, lz = pz + kk - cz * CW, yy = hgt + ll; if (lx < 0 || lx >= CW || lz < 0 || lz >= CW || yy >= CH - 1) continue;
                    int mk = 0; for (int d = 0; d < 4; d++) if (cage(jj + RS_DX[d], kk + RS_DZ[d], ll)) mk |= 1 << d;
                    c->b[idx(lx, yy, lz)] = (BT)XB_PANE; c->m[idx(lx, yy, lz)] = (uint8_t)mk;
                }
            }
        }
    }
    {   // pedestal de saida (WorldGenEndPodium, portal ativo): bacia de bedrock, portal em volta, pilar de bedrock de 4 com tochas
        const int py = (int)endTopAt(0, 0, sd) + 1;
        for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) {
            const int lx = dx - cx * CW, lz = dz - cz * CW; if (lx < 0 || lx >= CW || lz < 0 || lz >= CW) continue;
            for (int dy = -1; dy <= 32; dy++) {
                const float d = sqrtf((float)(dx * dx + dy * dy + dz * dz)); if (d > 3.5f) continue; const int y = py + dy; if (y < 1 || y >= CH - 1) continue;
                if (dy < 0) c->b[idx(lx, y, lz)] = d <= 2.5f ? (BT)BEDROCK : (BT)stone;
                else if (dy > 0) c->b[idx(lx, y, lz)] = AIR;
                else if (d > 2.5f) c->b[idx(lx, y, lz)] = BEDROCK;
                else { c->b[idx(lx, y, lz)] = (BT)XB_PORTAL; c->m[idx(lx, y, lz)] = 2; }
            }
            if (!dx && !dz) for (int k = 0; k < 4; k++) c->b[idx(lx, py + k, lz)] = BEDROCK;
            if (py + 2 < CH - 1) { const int t = (dx == 0 && dz == -1) ? 3 : (dx == 0 && dz == 1) ? 1 : (dx == 1 && dz == 0) ? 0 : (dx == -1 && dz == 0) ? 2 : -1; if (t >= 0) c->b[idx(lx, py + 2, lz)] = (BT)(XB_TORCH + 1 + t); }   // tocha na parede do pilar (suporte: 0 -X, 1 -Z, 2 +X, 3 +Z)
        }
    }
}
// portal do End: entra = vai para o End (plataforma de obsidiana 5x5 em 100,48,0); no End leva de volta ao spawn
static void endTravel(Phys& P) {
    if (gDim == 2) { dimSwitch(0); P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = gSpY; P.mx = P.my = P.mz = 0; P.onGround = false; gRespHold = 200; survRespawnFix(P); gFall = 0.f; gKick = true; return; }
    dimSwitch(2);
    for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) if (!getC(fdiv(100, CW) + dx, dz)) gen(fdiv(100, CW) + dx, dz);
    for (int dz = -2; dz <= 2; dz++) for (int dx = 98; dx <= 102; dx++) { setB(dx, 48, dz, OBSIDIAN); for (int y = 49; y <= 52; y++) setB(dx, y, dz, AIR); }
    P.x = P.px = 100.5; P.z = P.pz = .5; P.y = P.py = 49.0; gNetherYaw = 90.f; gNetherPitch = 0.f; gNetherTurn = true;
    P.mx = P.my = P.mz = 0; P.onGround = false; gFall = 0.f; gFire = 0; gKick = true;
}
static int gPortalT = 0; static bool gPortalCool = false; static int gNSpawnT = 0;
static void netherMobs(const Phys& P) {   // porcos-zumbis: nascem em netherrack ao redor do jogador (24..44 blocos) e somem quando ficam longe (>80)
    if (gDim != 1) return;
    int n = 0; for (size_t i = 0; i < MOBS.size();) { MobEnt& m = MOBS[i]; if (m.type == M_PIGMAN && (fabs(m.x - P.x) > 80 || fabs(m.z - P.z) > 80)) { MOBS.erase(MOBS.begin() + i); continue; } if (m.type == M_PIGMAN && !m.dead) n++; i++; }
    if (++gNSpawnT < 60 || n >= 10) return;
    gNSpawnT = 0;
    const int grp = 1 + rand() % 3; const double a = (rand() % 628) / 100.0, r = 24 + rand() % 21; const int X = (int)floor(P.x + cos(a) * r), Z = (int)floor(P.z + sin(a) * r);
    if (!getC(fdiv(X, CW), fdiv(Z, CW))) return;
    for (int y = 100; y > 33; y--) {
        if (!solid(getB(X, y - 1, Z)) || isLiquid(getB(X, y - 1, Z)) || getB(X, y, Z) != AIR || getB(X, y + 1, Z) != AIR) continue;
        for (int k = 0; k < grp; k++) { const double sx = X + .5 + (rand() % 5 - 2) * .4, sz = Z + .5 + (rand() % 5 - 2) * .4; spawnMob(M_PIGMAN, sx, y, sz, (float)(rand() % 360)); }
        return;
    }
}
static void netherTick(Phys& P) {   // 1x por tick (20 TPS): fogo queima, portal leva o jogador (4 s; instantaneo no criativo)
    const int fx = (int)floor(P.x), fz = (int)floor(P.z), fy = (int)floor(P.y);
    bool inP = false, inF = false;
    for (int y = fy; y <= fy + 1; y++) { const int b = getB(fx, y, fz); if (b == XB_PORTAL && !(getM(fx, y, fz) & 2)) inP = true; if (b == XB_FIRE) inF = true; }
    netherMobs(P);
    { static bool endCool = false; const int eb = getB(fx, fy, fz); const bool inE = eb == XB_PORTAL && (getM(fx, fy, fz) & 2) && P.y < fy + .75;
      if (inE) { if (!endCool) { endCool = true; endTravel(P); return; } } else endCool = false; }   // portal do End: instantaneo
    if (inF && !creative && gFire < 160) gFire = 160;
    if (inP) { if (!gPortalCool && (creative || ++gPortalT >= 80)) { gPortalT = 0; gPortalCool = true; netherTravel(P); } }
    else { gPortalT = 0; gPortalCool = false; }
}

// ---- animacao: fogo (fire_0/fire_1) e portal (nether_portal) trocam o conteudo dos tiles no atlas, como a tocha/bussola ----
static std::vector<std::array<uint8_t, 1024>> gFireFr[2], gPortFr; static int gFireTile[2] = {-1, -1}, gPortTile = -1;
static int netherStrip(const char* file, std::vector<std::array<uint8_t, 1024>>& frames) {   // aloca um tile para a tira vertical animada do pack
    frames.clear(); std::vector<uint8_t> px; int w = 0, h = 0;
    { std::string f = file; bool ok = packRGBA(f, px, w, h);   // patchPortal.2: nomes do pack 1.12 (portal, fire_layer_N)
      if (!ok) { if (f == "block/nether_portal.png") ok = packRGBA("block/portal.png", px, w, h); else if (f == "block/fire_0.png") ok = packRGBA("block/fire_layer_0.png", px, w, h); else if (f == "block/fire_1.png") ok = packRGBA("block/fire_layer_1.png", px, w, h); }
      if (!ok || w < 1 || h < w || gXbNext <= pkTileEnd() + 8) return -1; }
    const int n = std::min(h / w, 64);
    for (int k = 0; k < n; k++) { std::vector<uint8_t> f((size_t)w * w * 4); memcpy(&f[0], &px[(size_t)k * w * w * 4], (size_t)w * w * 4); std::array<uint8_t, 1024> a; scaleRGBA(f, w, w, a.data(), 16, 16); frames.push_back(a); }
    const int tile = gXbNext--; putTile16(tile, frames[0].data()); return tile;
}
static void drawFireOverlay() {   // (VIS) renderFireInFirstPerson: 2 camadas do fogo na frente da camera (espaco da camera, depois do bob), 90% opaco
    if (gFire <= 0 || creative || gDead || gFireTile[0] < 0) return;
    const int tile = gFireTile[0];
    const float u0 = (tile % ATL) / (float)ATL, u1 = u0 + 1.f / ATL, v0 = (tile / ATL) / (float)ATLR, v1 = v0 + 1.f / ATLR;
    const GLboolean dt = glIsEnabled(GL_DEPTH_TEST), cull = glIsEnabled(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glColor4f(1.f, 1.f, 1.f, .9f);
    for (int i = 0; i < 2; i++) {
        glPushMatrix(); glTranslatef(-(i * 2 - 1) * .24f, -.3f, 0.f); glRotatef((i * 2 - 1) * 10.f, 0, 1, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(u1, v1); glVertex3f(-.5f, -.5f, -.5f); glTexCoord2f(u0, v1); glVertex3f(.5f, -.5f, -.5f); glTexCoord2f(u0, v0); glVertex3f(.5f, .5f, -.5f); glTexCoord2f(u1, v0); glVertex3f(-.5f, .5f, -.5f);
        glEnd(); glPopMatrix();
    }
    glColor4f(1.f, 1.f, 1.f, 1.f); if (dt) glEnable(GL_DEPTH_TEST); if (cull) glEnable(GL_CULL_FACE);
}
static void drawFireWH(float w, float h) {   // chamado dentro da matriz da entidade (origem = pes): 2 camadas de planos com fire_0/fire_1 animados (sem luz: fogo brilha). Mobs, jogador remoto e o proprio jogador em 3a pessoa
    if (gFireTile[0] < 0 || gFireTile[1] < 0) return;
    glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glColor4f(1.f, 1.f, 1.f, 1.f);
    const GLboolean cull = glIsEnabled(GL_CULL_FACE); glDisable(GL_CULL_FACE);
    auto quad = [&](float x0, float z0, float x1, float z1, float hh, int tile) {
        const float u0 = (tile % ATL) / (float)ATL, u1 = u0 + 1.f / ATL, v0 = (tile / ATL) / (float)ATLR, v1 = v0 + 1.f / ATLR;
        glTexCoord2f(u0, v1); glVertex3f(x0, 0, z0); glTexCoord2f(u1, v1); glVertex3f(x1, 0, z1); glTexCoord2f(u1, v0); glVertex3f(x1, hh, z1); glTexCoord2f(u0, v0); glVertex3f(x0, hh, z0);
    };
    glBegin(GL_QUADS);
    quad(-w, -w, w, w, h * .95f, gFireTile[0]); quad(w, -w, -w, w, h * .95f, gFireTile[0]);
    quad(0, -w * .9f, 0, w * .9f, h * .8f, gFireTile[1]); quad(-w * .9f, 0, w * .9f, 0, h * .8f, gFireTile[1]);
    glEnd();
    if (cull) glEnable(GL_CULL_FACE);
}
static void drawMobFire(const MobEnt& e, float pt) { (void)pt; const MobDef& d = MD[e.type]; drawFireWH(d.w * .75f, d.h); }
static XQuad nQuad(float x0, float z0, float x1, float z1, float h, int tile, bool rev) {
    XQuad q; q.tile = tile; q.tint = false; q.shade = false;
    const float P4[4][3] = {{x0, 0, z0}, {x1, 0, z1}, {x1, h, z1}, {x0, h, z0}}, U[4] = {0, 1, 1, 0}, V[4] = {1, 1, 0, 0};
    for (int k = 0; k < 4; k++) { const int s = rev ? 3 - k : k; memcpy(q.p[k], P4[s], sizeof q.p[k]); q.u[k] = U[s]; q.v[k] = V[s]; }
    return q;
}
static void nPlane(std::vector<XQuad>& out, float x0, float z0, float x1, float z1, float h, int tile) { out.push_back(nQuad(x0, z0, x1, z1, h, tile, false)); out.push_back(nQuad(x0, z0, x1, z1, h, tile, true)); }
static int gEndTile[4] = {-1, -1, -1, -1};   // 0 = fundo escuro, 1..3 = camadas de estrelas (rolam em velocidades diferentes: paralaxe como o end_portal do 1.12.2)
static XQuad nTop(float y, int tile) {
    XQuad q; q.tile = tile; q.tint = false; q.shade = false;
    const float P4[4][3] = {{0, y, 0}, {1, y, 0}, {1, y, 1}, {0, y, 1}}, U[4] = {0, 1, 1, 0}, V[4] = {0, 0, 1, 1};
    for (int k = 0; k < 4; k++) { memcpy(q.p[k], P4[k], sizeof q.p[k]); q.u[k] = U[k]; q.v[k] = V[k]; }
    return q;
}
static void fallHooksInit();   // falling.inc
static void netherInit() {   // depois de xbInit() (modelos e tiles novos) e antes de subir o atlas
    gNetherrackId = pkFind("block/netherrack");
    for (auto& q : gFireSideQ) q.clear(); gXM[XB_FIRE - XB0].clear(); gXM[XB_PORTAL - XB0].clear(); gPortalZ.clear(); gEndPortalQ.clear(); gFireTile[0] = gFireTile[1] = gPortTile = -1; gFireFr[0].clear(); gFireFr[1].clear(); gPortFr.clear();
    for (int k = 0; k < 4; k++) gEndTile[k] = gXbNext > pkTileEnd() + 8 ? gXbNext-- : -1;   // patchEnd.1: tiles do portal do End alocados PRIMEIRO (antes ficavam por ultimo, exigiam 64 livres e o portal sumia com o atlas cheio)
    if (gPack.ok) {   // patchPortal.1: portal primeiro (era o ultimo a pegar tile e ficava sem textura quando o atlas enchia), depois fogo
        gPortTile = netherStrip("block/nether_portal.png", gPortFr); gFireTile[0] = netherStrip("block/fire_0.png", gFireFr[0]); gFireTile[1] = netherStrip("block/fire_1.png", gFireFr[1]);
        if (gPortTile < 0) fprintf(stderr, "portal: sem tile livre no atlas ou block/nether_portal.png ausente (livres: %d)\n", gXbNext - pkTileEnd());
    }
    if (gEndTile[3] < 0) { const int fb = gEndTile[0] >= 0 ? gEndTile[0] : gPortTile; for (int k = 0; k < 4; k++) if (gEndTile[k] < 0) gEndTile[k] = fb; }   // sem tiles livres: reaproveita um (o portal fica visivel, so sem paralaxe)
    if (gEndTile[0] >= 0 && gEndTile[1] >= 0 && gEndTile[2] >= 0 && gEndTile[3] >= 0) {   // topo em 12/16 (altura do BlockEndPortal)   // quatro lajes empilhadas ate 12/16 (altura da caixa): de baixo p/ cima fundo + 3 camadas de estrelas
        const float ys[4] = {.50f, .60f, .70f, .75f};
        for (int k = 0; k < 4; k++) { gEndPortalQ.push_back(nTop(ys[k], gEndTile[k])); XQuad r = nTop(ys[k], gEndTile[k]); std::swap(r.p[1], r.p[3]); std::swap(r.u[1], r.u[3]); std::swap(r.v[1], r.v[3]); gEndPortalQ.push_back(r); }
    }
    if (gPack.ok) {
        /* patchPortal.1: tiles do portal/fogo alocados antes (abaixo) */
        if (gFireTile[0] >= 0 && gFireTile[1] >= 0) {
            std::vector<XQuad>& f = gXM[XB_FIRE - XB0];
            nPlane(f, 0, 0, 1, 1, 1.4f, gFireTile[0]); nPlane(f, 1, 0, 0, 1, 1.4f, gFireTile[0]);   // dois planos diagonais (camada 0)
            nPlane(f, .5f, .05f, .5f, .95f, 1.2f, gFireTile[1]); nPlane(f, .05f, .5f, .95f, .5f, 1.2f, gFireTile[1]);   // dois planos axiais (camada 1)
            for (int mk = 1; mk < 16; mk++) {   // chamas na parede: um plano (2 camadas) colado em cada lado inflamavel
                std::vector<XQuad>& v = gFireSideQ[mk];
                if (mk & 1) { nPlane(v, .95f, 0, .95f, 1, 1.4f, gFireTile[0]); nPlane(v, .9f, 0, .9f, 1, 1.2f, gFireTile[1]); }
                if (mk & 2) { nPlane(v, .05f, 0, .05f, 1, 1.4f, gFireTile[0]); nPlane(v, .1f, 0, .1f, 1, 1.2f, gFireTile[1]); }
                if (mk & 4) { nPlane(v, 0, .95f, 1, .95f, 1.4f, gFireTile[0]); nPlane(v, 0, .9f, 1, .9f, 1.2f, gFireTile[1]); }
                if (mk & 8) { nPlane(v, 0, .05f, 1, .05f, 1.4f, gFireTile[0]); nPlane(v, 0, .1f, 1, .1f, 1.2f, gFireTile[1]); }
            }
        }
        if (gPortTile >= 0) { nPlane(gXM[XB_PORTAL - XB0], 0, .5f, 1, .5f, 1.f, gPortTile); nPlane(gPortalZ, .5f, 0, .5f, 1, 1.f, gPortTile); }
    }
    { const int ps = shapeAddFn([](int, int, int, int, int meta, bool, AABB* o) { o[0] = (meta & 2) ? AABB{0, 0, 0, 1, .75, 1} : (meta & 1) ? AABB{.375, 0, 0, .625, 1, 1} : AABB{0, 0, .375, 1, 1, .625}; return 1; });   // 1.12.2 BlockPortal: placa de 4/16 de espessura; sem colisao, mas selecionavel
      const int fs = shapeAddFn([](int, int, int, int, int meta, bool, AABB* o) {   // patch5.68: contorno do fogo = fina placa onde estao as chamas (chao: 1/16 embaixo; parede: 1/16 colado em cada lado), nao o cubo inteiro
          const int mk = meta & 15; if (!mk) { o[0] = AABB{0, 0, 0, 1, .0625, 1}; return 1; }
          int n = 0; const double t = .0625;
          if (mk & 1) o[n++] = AABB{1 - t, 0, 0, 1, 1, 1}; if (mk & 2) o[n++] = AABB{0, 0, 0, t, 1, 1};
          if (mk & 4) o[n++] = AABB{0, 0, 1 - t, 1, 1, 1}; if (mk & 8) o[n++] = AABB{0, 0, 0, 1, 1, t};
          return n; });
      blockShapeSet(XB_FIRE, SHAPE_NONE, fs); blockShapeSet(XB_PORTAL, SHAPE_NONE, ps); }   // fogo: sem colisao, selecionavel (socar apaga); portal: sem colisao nem selecao
    BlockHooks hf; hf.neighbor = fireNeighbor; hf.added = fireAdded; hf.tick = fireTick; blockHooksSet(XB_FIRE, hf); fallHooksInit();
    BlockHooks hp; hp.neighbor = portalNeighbor; blockHooksSet(XB_PORTAL, hp);
}
static uint32_t gEndLast = 0;
static void netherAnimUpdate() {
    const uint32_t t = SDL_GetTicks();
    auto put = [&](int tile, const std::array<uint8_t, 1024>& fr) {
        uint8_t* dst = atlas + ((size_t)(tile / ATL) * 16 * ATW + (size_t)(tile % ATL) * 16) * 4; bool same = true;
        for (int y = 0; y < 16 && same; y++) same = memcmp(dst + (size_t)y * ATW * 4, fr.data() + y * 64, 64) == 0;
        if (same) return;
        for (int y = 0; y < 16; y++) memcpy(dst + (size_t)y * ATW * 4, fr.data() + y * 64, 64);
        compassUploadTile(tile);
    };
    for (int k = 0; k < 2; k++) if (gFireTile[k] >= 0 && !gFireFr[k].empty()) put(gFireTile[k], gFireFr[k][(t / 100u + (unsigned)k * 11u) % gFireFr[k].size()]);
    if (gPortTile >= 0 && !gPortFr.empty()) {   // portal: quadros do pack misturados (interpolate) + rolagem vertical suave, o redemoinho do Minecraft
        const size_t n = gPortFr.size(); const double ph = (double)t / 120.0; const size_t fa = (size_t)ph % n, fb = (fa + 1) % n; const float f = (float)(ph - floor(ph));
        const float sc = fmodf((float)t * .006f, 16.f); const int s0 = (int)sc; const float sf = sc - s0;
        std::array<uint8_t, 1024> fr;
        auto smp = [&](int y, int x, int ch) { const int i = ((y & 15) * 16 + x) * 4 + ch; return gPortFr[fa][i] * (1.f - f) + gPortFr[fb][i] * f; };
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) for (int ch = 0; ch < 4; ch++) fr[(y * 16 + x) * 4 + ch] = (uint8_t)(smp(y + s0, x, ch) * (1.f - sf) + smp(y + s0 + 1, x, ch) * sf + .5f);
        put(gPortTile, fr);
    }
    if (gEndTile[0] >= 0 && gEndTile[3] >= 0 && gEndTile[0] != gEndTile[3] && gEndTile[1] != gEndTile[2] && t - gEndLast >= 45u) {   // portal do End: estrelas procedurais (verde-agua/azul/branco) rolando em 3 camadas
        gEndLast = t; const float ts = (float)t * .001f;
        auto hs = [](int x, int y, int sd) { uint32_t h = (uint32_t)(x & 15) * 374761393u + (uint32_t)(y & 15) * 668265263u + (uint32_t)sd * 2147483629u; h = (h ^ (h >> 13)) * 1274126177u; return (h ^ (h >> 16)) & 0xFFFFu; };
        static const float COL[3][3] = {{.45f, 1.f, .85f}, {.40f, .70f, 1.f}, {.85f, 1.f, .95f}}, SPD[3][2] = {{.55f, .35f}, {-.9f, .5f}, {1.4f, -.8f}}; static const uint32_t TH[3] = {9000u, 6500u, 4500u};
        static bool bgDone = false;
        if (!bgDone) { bgDone = true; std::array<uint8_t, 1024> b; for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const float n = (hs(x, y, 91) & 255) / 255.f; uint8_t* o = &b[(y * 16 + x) * 4]; o[0] = (uint8_t)(6 + n * 10); o[1] = (uint8_t)(26 + n * 26); o[2] = (uint8_t)(34 + n * 26); o[3] = 255; } put(gEndTile[0], b); }
        for (int k = 0; k < 3; k++) {
            const float ox = ts * SPD[k][0], oy = ts * SPD[k][1]; const int ix = (int)floorf(ox), iy = (int)floorf(oy); const float fx = ox - ix, fy = oy - iy;
            std::array<uint8_t, 1024> fr;
            for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
                auto st = [&](int a, int b2) { const uint32_t h = hs(a + ix, b2 + iy, 17 + k * 13); return h < TH[k] ? 0.45f + 0.55f * (0.5f + 0.5f * sinf(ts * 3.f + (float)(h & 63))) : 0.f; };
                const float v = (st(x, y) * (1 - fx) + st(x + 1, y) * fx) * (1 - fy) + (st(x, y + 1) * (1 - fx) + st(x + 1, y + 1) * fx) * fy;
                uint8_t* o = &fr[(y * 16 + x) * 4]; const float m = 0.55f + 0.45f * std::min(1.f, v);
                o[0] = (uint8_t)(COL[k][0] * 255.f * m); o[1] = (uint8_t)(COL[k][1] * 255.f * m); o[2] = (uint8_t)(COL[k][2] * 255.f * m); o[3] = v > 0.3f ? 255 : 0;   // opaco com alpha test: estrela inteira ou nada
            }
            put(gEndTile[1 + k], fr);
        }
    }
}
