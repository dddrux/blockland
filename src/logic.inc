// patchLogic.1: lampada de redstone (acende/apaga com energia) e bloco de notas (clique muda a nota; energia toca).
// Blocos do pack: block/redstone_lamp (apagada) e block/redstone_lamp_on (acesa, luz 15); block/note_block (meta: bits 0-4 = nota 0..24, bit 5 = recebendo energia).
static void lampNeighbor(int x, int y, int z, int, int, int) {
    if (!gLampOn || !gLampOff || (netOn && !netHost)) return;   // so o host/solo decide (o convidado recebe o bloco)
    const int b = getB(x, y, z); if (b != gLampOn && b != gLampOff) return;
    const bool p = rsBlockPowered(x, y, z, -99999, -99999, -99999);
    if (p && b == gLampOff) { setBM(x, y, z, gLampOn, 0); netSendBlock(x, y, z, gLampOn, 0); }
    else if (!p && b == gLampOn) { setBM(x, y, z, gLampOff, 0); netSendBlock(x, y, z, gLampOff, 0); }
}
static void notePlay(int x, int y, int z, int n) {   // instrumento pelo bloco de baixo (como no 1.12): madeira baixo, areia/cascalho caixa, vidro chimbal, pedra bumbo, resto harpa
    const int below = getB(x, y - 1, z), m = below == AIR ? 0 : sndMatOf(below);
    const char* f = m == 4 ? "note/bass" : (m == 2 || m == 3) ? "note/snare" : m == 7 ? "note/hat" : (below != AIR && solid(below) && m == 0) ? "note/bd" : "note/harp";
    sndPlayAt(f, x + .5, y + .5, z + .5, 1.f, powf(2.f, (n - 12) / 12.f), 4);
}
static void noteNeighbor(int x, int y, int z, int, int, int) {
    if (!gNoteId || getB(x, y, z) != gNoteId || (netOn && !netHost)) return;
    const int m = getM(x, y, z); const bool p = rsBlockPowered(x, y, z, -99999, -99999, -99999);
    if (p && !(m & 32)) { setBM(x, y, z, gNoteId, m | 32); netSendBlock(x, y, z, gNoteId, m | 32); notePlay(x, y, z, m & 31); }
    else if (!p && (m & 32)) { setBM(x, y, z, gNoteId, m & ~32); netSendBlock(x, y, z, gNoteId, m & ~32); }
}
static bool noteUse(int x, int y, int z) {   // clique direito: proxima nota (0..24) e toca
    if (!gNoteId || getB(x, y, z) != gNoteId) return false;
    const int m = getM(x, y, z), n = ((m & 31) + 1) % 25, nm = (m & ~31) | n;
    setBM(x, y, z, gNoteId, nm); netSendBlock(x, y, z, gNoteId, nm); notePlay(x, y, z, n); return true;
}
// ---- dropper / dispenser: container de 9 slots (1 linha na tela do bau); energia (borda de subida) solta 1 item da frente; meta: bits 0-1 = frente (0 N,1 L,2 S,3 O), bit 3 (8) = ja disparou
static void dispFire(int x, int y, int z) {
    ChestInv* c = chestAt(x, y, z, true); if (!c) return;
    int pick[9], np = 0; for (int i = 0; i < 9; i++) if (c->s[i].id > 0 && c->s[i].n > 0) pick[np++] = i;
    if (!np) { sndEvtAt("button.click", x + .5, y + .5, z + .5); return; }   // vazio: "click" de falha
    Slot& s = c->s[pick[rand() % np]]; static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0}; const int d = getM(x, y, z) & 3, fx = x + DX[d], fz = z + DZ[d];
    const int id = s.id; const std::string nbt = s.nbt; bool moved = false;
    if (getB(x, y, z) == gDropId) {   // dropper: se a frente e um bau, empurra para dentro dele
        ChestInv* t = getB(fx, y, fz) == XB_CHEST ? chestAt(fx, y, fz, false) : nullptr;
        if (t) { if (chestAdd(*t, id, 1, nbt) == 0) { moved = true; netSendChest(fx, y, fz, *t); } else return; }
    }
    if (!moved && getB(x, y, z) == gDispId) {   // patchLogic.4: dispenser dispara flecha (v 1.1, imprecisao 6) e acende TNT (pavio 80) em vez de soltar o item
        const double sx = x + .5 + DX[d] * .7, sz = z + .5 + DZ[d] * .7;
        if (id == IT_ARROW) { const double v = 1.1; arrowSpawn(sx, y + .5, sz, DX[d] * v + arrowGauss() * .0075 * v * 6.0, .1 + arrowGauss() * .0075 * v * 6.0, DZ[d] * v + arrowGauss() * .0075 * v * 6.0, false, -1, true); moved = true; }
        else if (gTntId && id == gTntId) { tntPrime(sx, y + .5, sz, 80); moved = true; }
        else if (gBktEmpty && (id == gBktWater || id == gBktLava || id == gBktEmpty)) {   // patchLogic.5: balde cheio esvazia na celula da frente (ar); balde vazio enche numa FONTE de agua/lava
            const int tb = getB(fx, y, fz); int rep = 0;
            if (id != gBktEmpty && tb == AIR) { if (id == gBktWater && gDim == 1) rep = gBktEmpty; else { setB(fx, y, fz, id == gBktWater ? WATER : LAVA); netSendBlock(fx, y, fz, id == gBktWater ? WATER : LAVA); rep = gBktEmpty; } }
            else if (id == gBktEmpty && (tb == WATER || tb == LAVA)) { setB(fx, y, fz, AIR); netSendBlock(fx, y, fz, AIR); rep = tb == WATER ? gBktWater : gBktLava; }
            if (rep) { if (s.n <= 1) { s.id = rep; s.n = 1; s.nbt.clear(); } else { --s.n; chestAdd(*c, rep, 1, ""); } netSendChest(x, y, z, *c); sndEvtAt("dispenser.launch", x + .5, y + .5, z + .5); return; }
        }
        if (moved) { if (--s.n <= 0) s = Slot(); netSendChest(x, y, z, *c); sndEvtAt("dispenser.launch", x + .5, y + .5, z + .5); return; }
    }
    if (!moved) spawnDrop(x + .5 + DX[d] * .7, y + .5, z + .5 + DZ[d] * .7, DX[d] * (getB(x, y, z) == gDispId ? .3 : .2) + (frand() - .5) * .05, .1, DZ[d] * (getB(x, y, z) == gDispId ? .3 : .2) + (frand() - .5) * .05, id, 1, 10, 0, nbt);
    if (--s.n <= 0) s = Slot(); netSendChest(x, y, z, *c);
    sndEvtAt(getB(x, y, z) == gDispId ? "dispenser.launch" : "button.click", x + .5, y + .5, z + .5);
}
static void dispNeighbor(int x, int y, int z, int, int, int) {
    const int b = getB(x, y, z); if (!gDropId || (b != gDropId && b != gDispId) || (netOn && !netHost)) return;
    const int m = getM(x, y, z); const bool p = rsBlockPowered(x, y, z, -99999, -99999, -99999);
    if (p && !(m & 8)) { setBM(x, y, z, b, m | 8); netSendBlock(x, y, z, b, m | 8); dispFire(x, y, z); }
    else if (!p && (m & 8)) { setBM(x, y, z, b, m & ~8); netSendBlock(x, y, z, b, m & ~8); }
}
// ---- funil (hopper): 9 slots usados; a cada 8 ticks puxa 1 item do container/item solto em cima e empurra 1 para o container de baixo; desligado por redstone. Saida sempre para baixo.
static bool hopCont(int b) { return b && (b == XB_CHEST || b == gDropId || b == gDispId || b == gHopId); }
static void hopperOne(int x, int y, int z) {
    if (rsBlockPowered(x, y, z, -99999, -99999, -99999)) return;
    ChestInv* h = chestAt(x, y, z, true); if (!h) return; bool ch = false;
    { const int ob = getB(x, y - 1, z); ChestInv* t = hopCont(ob) ? chestAt(x, y - 1, z, false) : nullptr;   // empurra (nao devolve para outro funil que aponte para ele: so baixo)
      if (t) for (int i = 0; i < 9; i++) if (h->s[i].id > 0 && h->s[i].n > 0) { if (chestAdd(*t, h->s[i].id, 1, h->s[i].nbt) == 0) { if (--h->s[i].n <= 0) h->s[i] = Slot(); netSendChest(x, y - 1, z, *t); ch = true; } break; } }
    { const int ib = getB(x, y + 1, z); ChestInv* t = hopCont(ib) ? chestAt(x, y + 1, z, false) : nullptr; bool got = false;   // puxa de cima
      if (t) for (int i = 0; i < 9 && !got; i++) if (t->s[i].id > 0 && t->s[i].n > 0) { if (chestAdd(*h, t->s[i].id, 1, t->s[i].nbt) == 0) { if (--t->s[i].n <= 0) t->s[i] = Slot(); netSendChest(x, y + 1, z, *t); got = ch = true; } break; }
      if (!got && !t) for (size_t k = 0; k < DROPS.size(); k++) { DropEnt& d = DROPS[k]; if (d.id <= 0 || d.n <= 0 || d.x < x || d.x > x + 1 || d.z < z || d.z > z + 1 || d.y < y + .9 || d.y > y + 2.1) continue;
          const int left = chestAdd(*h, d.id, d.n, d.nbt); if (left < d.n) { ch = true; if (left <= 0) DROPS.erase(DROPS.begin() + k); else d.n = left; break; } } }
    if (ch) netSendChest(x, y, z, *h);
}
static void hopperTick(double px, double py, double pz) {   // 1x por tick; funis perto do jogador (raio 12) disparam a cada 8 ticks (so host/solo)
    static int tk = 0; if (!gHopId || ++tk % 8 || (netOn && !netHost)) return;
    const int cx = (int)floor(px), cy = (int)floor(py), cz = (int)floor(pz);
    for (int y = std::max(1, cy - 10); y <= std::min(CH - 2, cy + 10); y++) for (int z = cz - 12; z <= cz + 12; z++) for (int x = cx - 12; x <= cx + 12; x++) if (getB(x, y, z) == gHopId) hopperOne(x, y, z);
}
static bool dispOpen(int x, int y, int z) {   // clique direito: abre a tela (1 linha de 9 slots); true = o chamador abre o inventario
    const int b = getB(x, y, z); if (!b || (b != gDropId && b != gDispId && b != gHopId)) return false;
    ChestInv* inv = chestAt(x, y, z, true); if (!inv) return false;
    gChest = true; gChEnder = false; gFurn = false; gChShulker = false; gChCnt = 1; gChRows = 1; gChPos[0][0] = x; gChPos[0][1] = y; gChPos[0][2] = z;
    gChSnap[0] = *inv; gChLoaded[0] = !chestGuest(); gChWait = chestGuest(); gChT0 = SDL_GetTicks();
    if (gChWait) { NBuf nb = netMk(NP_CHESTREQ); nb.put<int32_t>(x); nb.put<int32_t>(y); nb.put<int32_t>(z); netBroadcast(nb); }
    return true;
}
static void logicInit() {
    gHopId = 0; { auto t = gPkByFile.find("block/hopper_outside"); if (t != gPkByFile.end()) gHopId = t->second; }   // patchLogic.3
    if (gDropId) gHookSel[gDropId] = 0; if (gDispId) gHookSel[gDispId] = 0; gDropId = gDispId = 0;
    { auto t = gPkByFile.find("block/dropper"); if (t != gPkByFile.end()) gDropId = t->second; }
    { auto t = gPkByFile.find("block/dispenser"); if (t != gPkByFile.end()) gDispId = t->second; }
    { BlockHooks h; h.added = [](int x, int y, int z) { dispNeighbor(x, y, z, 0, 0, 0); }; h.neighbor = dispNeighbor; if (gDropId) blockHooksSet(gDropId, h); if (gDispId) blockHooksSet(gDispId, h); }
    if (gLampOn) gHookSel[gLampOn] = 0; if (gLampOff) gHookSel[gLampOff] = 0; if (gNoteId) gHookSel[gNoteId] = 0;
    gLampOn = gLampOff = gNoteId = 0;
    for (const char* n : {"block/redstone_lamp", "block/redstone_lamp_off"}) { auto t = gPkByFile.find(n); if (t != gPkByFile.end()) { gLampOff = t->second; break; } }
    { auto t = gPkByFile.find("block/redstone_lamp_on"); if (t != gPkByFile.end()) gLampOn = t->second; }
    { auto t = gPkByFile.find("block/note_block"); if (t != gPkByFile.end()) gNoteId = t->second; }
    BlockHooks h; h.added = [](int x, int y, int z) { lampNeighbor(x, y, z, 0, 0, 0); }; h.neighbor = lampNeighbor;
    if (gLampOn && gLampOff) { blockHooksSet(gLampOn, h); blockHooksSet(gLampOff, h); }
    BlockHooks n; n.added = [](int x, int y, int z) { noteNeighbor(x, y, z, 0, 0, 0); }; n.neighbor = noteNeighbor; if (gNoteId) blockHooksSet(gNoteId, n);
}
