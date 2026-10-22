// ---------- mundo ----------
struct V { float x, y, z, u, v, r, g, b, a; };
#if defined(BL_VULKAN) || defined(BL_GLES_NATIVE)
static_assert(sizeof(V) == 36, "vkgl.inc/glesnative.inc assumem V = 9 floats (x,y,z,u,v,r,g,b,a)");
#endif
struct Chunk { BT b[CW * CH * CW]; uint8_t m[CW * CH * CW]; /* metadados por voxel (estado/orientacao/potencia), 0 = padrao */ std::vector<V> mesh, wmesh; uint32_t sOff[17] = {0}, wOff[17] = {0}; bool secOk = false;   /* inicio de cada secao vertical de 16 blocos dentro da malha (solida / agua): culling por secao */ int ngv = 0; /* vertices de vidro colorido/gelo no FIM de wmesh (passe translucido proprio) */ bool dirty = true, wdirty = false, hasTrans = false;   /* hasTrans: tem vidro colorido/gelo na malha translucida (agua so nao basta refazer) */ bool gl = false, glw = false, hasWater = false; bool farM = false; int maxY = -1; uint8_t* lc = nullptr; GLuint vbo = 0, wvbo = 0; int nv = 0, nwv = 0; float born = -1.f, rborn = -1.f; };
static std::unordered_map<int64_t, Chunk*> W;
static bool gKick = true;       // algo mudou (chunk novo / sujo): vale a pena varrer o mundo de novo (stream() pula tudo quando nada mudou)
static unsigned gEpoch = 1;     // sobe quando o conjunto de chunks carregados muda (invalida o cache de chunks visiveis)
#ifdef BL_RPI
static int gSmoothMode = 0;   // patchRpiOpt.1: Raspberry = luz plana por face
#else
static int gSmoothMode = 2;
#endif
// patchLight.1: Iluminacao Suave (opcoes de video): 0 OFF = luz plana por face, 1 Minima = luz suave sem oclusao ambiente, 2 Maxima = suave + AO
static bool gShadowsOn = false;   // sombras (F7); definido aqui para ser usado antes de shadows.inc
static bool gLeafFast = true;   // patchGfx.2: Graficos Rapidos (padrao)
static bool gNetherFx = false, gLeafCull = true;   // patchGfx.1: shader do Nether ligado; culling de folhas em aglomerados (ajustes d17/d18, tela Detalhes)
static bool gMeshFar = false;   // build() em modo "longe": sem plantas/modelos pequenos (a nevoa ja os esconde)
static int64_t key(int cx, int cz) { return ((int64_t)cx << 32) ^ (uint32_t)cz; }
static int fdiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static inline int idx(int x, int y, int z) { return (y * CW + z) * CW + x; }
// patchRpi.6: cache direto de 16 entradas por (cx,cz): getB/getM faziam uma busca em hash por chamada (o mesher chama milhoes por chunk). Chunks so saem de W em W.clear() (menu.inc) => getCReset()
static int64_t gCK[16]; static Chunk* gCV[16]; static bool gCInit = false;
static void getCReset() { for (int i = 0; i < 16; i++) { gCK[i] = INT64_MIN; gCV[i] = nullptr; } gCInit = true; }
static inline Chunk* getC(int cx, int cz) {
    if (!gCInit) getCReset();
    const int64_t k = key(cx, cz); const int s = (int)((cx * 5 + cz * 3) & 15);
    if (gCK[s] == k) return gCV[s];
    auto it = W.find(k); if (it == W.end()) return nullptr;
    gCK[s] = k; gCV[s] = it->second; return it->second;
}

// fora do mundo / chunk não carregado conta como sólido (esconde faces e impede cair no vazio)
static int getB(int x, int y, int z) {
    if (y < 0) return STONE;
    if (y >= CH) return AIR;
    Chunk* c = getC(fdiv(x, CW), fdiv(z, CW));
    if (!c) return STONE;
    return c->b[idx(x - fdiv(x, CW) * CW, y, z - fdiv(z, CW) * CW)];
}
// metadados do bloco (0..255): orientacao, estado, potencia de redstone etc. Fora do mundo / chunk nao carregado = 0.
static int getM(int x, int y, int z) {
    if (y < 0 || y >= CH) return 0;
    Chunk* c = getC(fdiv(x, CW), fdiv(z, CW));
    if (!c) return 0;
    return c->m[idx(x - fdiv(x, CW) * CW, y, z - fdiv(z, CW) * CW)];
}
static std::vector<std::array<int, 4>> gPropLightV;   // patchProp.9: luz de props (x,y,z,nivel) lida pelo motor de luz do chunk
static void dirtyC(int cx, int cz) { if (Chunk* c = getC(cx, cz)) { c->dirty = true; gKick = true; } }
static void wdirtyC(int cx, int cz) { if (Chunk* c = getC(cx, cz)) { c->wdirty = true; gKick = true; } }
// maior Y com bloco nao-ar (cache; reduz o trabalho de luz e malha)
static int chunkTopFresh(Chunk* c);   // patchGen.1
static int chunkTop(Chunk* c) {
    if (c->maxY >= 0) return c->maxY;
    int y = CH - 1;
    for (; y > 0; y--) { const BT* p = &c->b[idx(0, y, 0)]; bool any = false; for (int i = 0; i < CW * CW; i++) if (p[i]) { any = true; break; } if (any) break; }
    return c->maxY = y;
}
static int chunkTopFresh(Chunk* c) { c->maxY = -1; return chunkTop(c); }   // patchGen.1: ignora o cache (blocos escritos direto em c->b, sem setB, deixavam o topo velho => blocos acima dele sem malha = INVISIVEIS mas com hitbox)
// ---- fila de ticks da agua (estilo Minecraft: 1 passo a cada 0,25 s) ----
static std::vector<int64_t> wqCur, wqNext; static std::unordered_set<int64_t> wqSet;
static inline int64_t wkey(int x, int y, int z) { return (((int64_t)(x & 0x3FFFFFF)) << 38) | (((int64_t)(z & 0x3FFFFFF)) << 12) | (int64_t)(y & 0xFFF); }
static std::vector<int64_t> lqCur, lqNext; static std::unordered_set<int64_t> lqSet;   // fila da lava (tick de 1,5 s)
static void lavaSchedule(int x, int y, int z) { if (y < 0 || y >= CH) return; if (lqSet.insert(wkey(x, y, z)).second) lqNext.push_back(wkey(x, y, z)); }
static void waterSchedule(int x, int y, int z) { if (y < 0 || y >= CH) return; if (wqSet.insert(wkey(x, y, z)).second) wqNext.push_back(wkey(x, y, z)); }
static void fluidWake(int x, int y, int z) {
    static const int N[7][3] = {{0,0,0},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (auto& n : N) { const int nb = getB(x + n[0], y + n[1], z + n[2]); if (isWater(nb)) waterSchedule(x + n[0], y + n[1], z + n[2]); else if (isLava(nb)) lavaSchedule(x + n[0], y + n[1], z + n[2]); }
}
// Atualizacao de vizinhos (blockupdate.inc): todo setBM que muda algo avisa os vizinhos. gNoBlockHooks = true enquanto se aplica uma edicao RECEBIDA da rede
// (quem editou ja rodou a cascata localmente e transmite cada edicao resultante; repetir aqui duplicaria drops e edicoes).
static bool gNoBlockHooks = false;
static int gDropId = 0, gDispId = 0, gHopId = 0;   // patchLogic.2: dropper/dispenser (pack); ids resolvidos em logicInit
static void chestOnRemoved(int x, int y, int z, bool isChest = true);   // chest.inc: o bau saiu daqui => solta o conteudo
static int gFurnId = 0, gFurnOnTile = -1; static void furnOnRemoved(int x, int y, int z);   // furnace.inc: fornalha (bloco do pack) saiu => solta o conteudo; meta bit2 = acesa
static void blockChangedHook(int x, int y, int z, int oldId, int newId);
// Troca bloco E metadado de uma vez (como setBlock(x,y,z,id,meta) do Minecraft). setB() abaixo zera o meta.
static void lavaMixAround(int x, int y, int z);   // BlockLiquid.checkForMixing (definida junto da agua)
static int gUrgCx = 0x7fffffff, gUrgCz = 0;   // patchBreak.2: chunk da ultima troca de bloco (o unico que o Pi malha com urgencia)
static void setBM(int x, int y, int z, int v, int meta) {
    if (y < 0 || y >= CH) return;
    meta &= 0xFF;
    int cx = fdiv(x, CW), cz = fdiv(z, CW);
    Chunk* c = getC(cx, cz);
    if (!c) return;
    int lx = x - cx * CW, lz = z - cz * CW;
    int old = c->b[idx(lx, y, lz)];
    if (old == v && c->m[idx(lx, y, lz)] == meta) return;
    if ((old >= XB_DUST && old < XB_END) || (old >= XB_ANVIL && old < XB_ANVIL + 4) || old == XB_BANNER || old == XB_WBANNER) gRsLastMeta = c->m[idx(lx, y, lz)];
    if (old == gFurnId && old && v != old) furnOnRemoved(x, y, z);
    if (old && (old == gDropId || old == gDispId || old == gHopId) && v != old) chestOnRemoved(x, y, z, false);   // patchLogic.2: solta o conteudo
    if (old == XB_CHEST && v != XB_CHEST) chestOnRemoved(x, y, z);   // (so ao trocar o ID: girar/abrir muda so o meta)
    const int oldMeta = c->m[idx(lx, y, lz)];
    c->b[idx(lx, y, lz)] = (BT)v;
    c->m[idx(lx, y, lz)] = (uint8_t)meta;
    if (isWater(v)) c->hasWater = true;
    if (v != AIR && c->maxY >= 0 && y > c->maxY) c->maxY = y;
    const bool fluidOnly = old != v && (old == AIR || isWater(old)) && (v == AIR || isWater(v));   // (mudanca so de meta nunca e "so agua")
    if (fluidOnly) {   // so a agua mudou: refaz apenas a malha de agua (e dos vizinhos, por causa dos cantos)
        c->wdirty = true; gKick = true;
        for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dz) continue;
            if ((dx < 0 && lx != 0) || (dx > 0 && lx != CW - 1) || (dz < 0 && lz != 0) || (dz > 0 && lz != CW - 1)) continue;
            wdirtyC(cx + dx, cz + dz);
        }
    } else {
        c->dirty = true; gKick = true; gUrgCx = cx; gUrgCz = cz;
        // a luz alcanca ate 15 blocos: se opacidade/emissao mudou, os 8 vizinhos precisam refazer a malha
        if (lightOpacL(old) != lightOpacL(v) || lightEmit(old) != lightEmit(v) || (old == v && v >= XB_RTORCH && v < XB_RTORCH + 5 && ((oldMeta ^ meta) & 1)) || (old == v && v == gFurnId && v && ((oldMeta ^ meta) & 4)))   // tocha de redstone acendeu/apagou: so o meta mudou
            for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) {
                // patchBreak.2: so os vizinhos que a luz (15 celulas, distancia de Manhattan) realmente alcanca; antes eram SEMPRE 8 chunks => 9 malhas por bloco quebrado
                const int ex = dx < 0 ? lx + 1 : dx > 0 ? CW - lx : 0, ez = dz < 0 ? lz + 1 : dz > 0 ? CW - lz : 0;
                if (ex + ez <= 15) dirtyC(cx + dx, cz + dz);
            }
        if (lx == 0) dirtyC(cx - 1, cz);
        if (lx == CW - 1) dirtyC(cx + 1, cz);
        if (lz == 0) dirtyC(cx, cz - 1);
        if (lz == CW - 1) dirtyC(cx, cz + 1);
        if (isWater(old) || isWater(v)) for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) wdirtyC(cx + dx, cz + dz);
    }
    fluidWake(x, y, z);
    if (!gNoBlockHooks && (isWater(v) || isLava(v))) lavaMixAround(x, y, z);   // onBlockAdded/neighborChanged: a mistura agua+lava e IMEDIATA, sem esperar o tick
    if (!gNoBlockHooks) blockChangedHook(x, y, z, old, v);
}
static inline void setB(int x, int y, int z, int v) { setBM(x, y, z, v, 0); }                  // bloco novo = meta 0
static inline void setM(int x, int y, int z, int meta) { setBM(x, y, z, getB(x, y, z), meta); }  // so o metadado (refaz a malha)

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O2")   // patchRpiOpt.3
#endif
#include "worldgen.inc"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
// especie da arvore -> tronco/folha do PACK (carvalho = blocos centrais). Sem o pack (ou sem a especie) cai no carvalho: o mundo gerado sem pack nao muda.
static int gTreeLog = LOG, gTreeLeaf = LEAVES;
static int packBlkId(const std::string& f) { const int id = pkFind(f.c_str()); return isPkBlk(id) ? id : 0; }
static void treeSpecies(int sp) {   // 0 carvalho, 1 abeto, 2 betula, 3 selva, 4 acacia, 5 carvalho-escuro
    static const char* N[6] = {"", "spruce", "birch", "jungle", "acacia", "big_oak"};   // 5 = carvalho-escuro (log_big_oak / leaves_big_oak)
    gTreeLog = LOG; gTreeLeaf = LEAVES; if (!sp) return;
    auto pb = [&](const char* kind) { int id = packBlkId(std::string("block/") + N[sp] + "_" + kind); if (!id) id = packBlkId(std::string("block/") + kind + "_" + N[sp]); return id; };   // pack 1.12 usa log_spruce / leaves_spruce
    const int l = pb("log"), v = pb("leaves");
    if (l && v) { gTreeLog = l; gTreeLeaf = v; }   // so troca se a especie esta completa (tronco E folha)
}
static void leafAt(Chunk* c, int x, int y, int z) {
    treePut(c, x, y, z, (BT)gTreeLeaf, true);
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O2")   // patchRpiOpt.3
#endif
#include "trees_mc.inc"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
#include "village.inc"

// patchMap.2: chunks do save ficam COMPRIMIDOS (RLE) na RAM e so viram 128 KB + 64 KB quando o chunk e gerado (antes: 11.521 chunks x 192 KB ~ 2 GB)
static std::unordered_map<int64_t, std::vector<uint16_t>> savedC;   // blocos: pares (repeticoes, id); chunks do save carregado (menu.inc)
static std::unordered_map<int64_t, std::vector<uint8_t>> savedM;   // metadados: trincas (rep lo, rep hi, valor) (formato BL17)
static std::vector<uint16_t> savedEncB(const BT* a, size_t N) { std::vector<uint16_t> o; for (size_t i = 0; i < N;) { size_t j = i + 1; while (j < N && a[j] == a[i] && j - i < 65535) j++; o.push_back((uint16_t)(j - i)); o.push_back(a[i]); i = j; } return o; }
static std::vector<uint8_t> savedEncM(const uint8_t* a, size_t N) { std::vector<uint8_t> o; for (size_t i = 0; i < N;) { size_t j = i + 1; while (j < N && a[j] == a[i] && j - i < 65535) j++; const size_t r = j - i; o.push_back((uint8_t)(r & 255)); o.push_back((uint8_t)(r >> 8)); o.push_back(a[i]); i = j; } return o; }
static void savedDecB(const std::vector<uint16_t>& o, BT* out, size_t N) { size_t p = 0; for (size_t k = 0; k + 1 < o.size() && p < N; k += 2) { const size_t r = std::min<size_t>(o[k], N - p); std::fill(out + p, out + p + r, (BT)o[k + 1]); p += r; } }
static void savedDecM(const std::vector<uint8_t>& o, uint8_t* out, size_t N) { size_t q = 0; for (size_t k = 0; k + 2 < o.size() && q < N; k += 3) { const size_t r = std::min<size_t>((size_t)o[k] | ((size_t)o[k + 1] << 8), N - q); std::fill(out + q, out + q + r, o[k + 2]); q += r; } }
static void emitXB(Chunk* c, int b, int x, int y, int z, int wx, int wz);
static void emitEnchBook(Chunk* c, int x, int y, int z, int wx, int wz);   // entmodels.inc
static std::vector<V>* gXbGlass = nullptr;   // destino dos modelos translucidos (painel colorido) durante o build; null = malha normal
static void xbVeg(Chunk* c, int x, int h, int z, int wx, int wz, int bio);
static bool xbHasModel(int id);   // models.inc
static void netherGen(Chunk* c, int cx, int cz); static void endGen(Chunk* c, int cx, int cz);   // nether.inc   // nether.inc
// animacao do pistao (so visual; a logica continua instantanea): bloco empurrado/puxado/cabeca desliza por 2 ticks; a celula de destino fica fora da malha ate la
struct PAnim { int x, y, z, d, id, m, s0, e0; bool hide; long long tick0; uint32_t ms0; };   // destino (x,y,z); posicao = destino + dir[d]*(s0 + (e0-s0)*p)
static std::vector<PAnim> gPAnim;
static bool pAnimHides(int x, int y, int z) { for (const PAnim& a : gPAnim) if (a.hide && a.x == x && a.y == y && a.z == z) return true; return false; }
static void rsChunkLoaded(Chunk* c, int cx, int cz);   // blockupdate.inc: reagenda ticks que o save nao guarda (botao/placa apertados)
static bool netChunkWait(int cx, int cz);   // net.inc: convidado espera o chunk salvo do host
static void strongholdChunk(Chunk* c, int cx, int cz);   // stronghold.inc
static bool gPanoGen = false; static void panoFill(Chunk* c, int cx, int cz);   // panorama do titulo: terreno proprio (main_loop.inc), nao usa o gerador do mundo
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O2")   // patchRpiOpt.3
#endif
static void genImpl(int cx, int cz) {
    Chunk* c = new Chunk();
    memset(c->b, 0, sizeof c->b); memset(c->m, 0, sizeof c->m);
    if (gPanoGen) { panoFill(c, cx, cz); W[key(cx, cz)] = c; gKick = true; gEpoch++; for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(cx + dx, cz + dz); return; }
    { auto sv = savedC.find(key(cx, cz)); if (sv != savedC.end()) { savedDecB(sv->second, c->b, (size_t)CW * CH * CW);
        { auto sm = savedM.find(key(cx, cz)); if (sm != savedM.end()) savedDecM(sm->second, c->m, (size_t)CW * CH * CW); }   // saves antigos nao tem meta => 0
    W[key(cx, cz)] = c; gKick = true; gEpoch++; rsChunkLoaded(c, cx, cz);
        for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(cx + dx, cz + dz); return; } }
    if (gDim == 2) { endGen(c, cx, cz); W[key(cx, cz)] = c; gKick = true; gEpoch++; for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(cx + dx, cz + dz); return; }   // The End
    if (gDim == 1) { netherGen(c, cx, cz); W[key(cx, cz)] = c; gKick = true; gEpoch++; for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(cx + dx, cz + dz); return; }   // Nether
    int hm[CW][CW]; uint8_t bm[CW][CW]; int tp[256]; uint8_t bo[256];
    terrain112(cx, cz, c->b, bo, tp);   // terreno base estilo 1.12.2 (terrain112.inc)
    { std::array<int16_t, 256> fa; for (int i = 0; i < 256; i++) fa[i] = (int16_t)tp[i]; gFloor[key(cx, cz)] = fa; }   // alturas para os discos de areia dos chunks vizinhos
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) { const int h = tp[z * 16 + x]; hm[x][z] = h; bm[x][z] = bo[z * 16 + x]; if (h < SEA) c->hasWater = true; }
    carveCaves(c, cx, cz, bo);                 // cavernas + ravinas, porte exato do 1.12.2 (worldgen.inc); lava em y<10 sai daqui
    strongholdChunk(c, cx, cz);                  // patchStronghold.1: estruturas antes do populate (como o MC), minerios nao as substituem
    populateChunk(c, cx, cz);                   // minérios, discos de areia/argila/cascalho, esmeraldas (worldgen.inc)
    applySpill(c, cx, cz);                      // árvores dos vizinhos que cruzam a borda
    gTcx = cx; gTcz = cz;
    for (int z = 1; z < CW - 1; z++) for (int x = 1; x < CW - 1; x++) {   // patchTerrain.4: encostas ingremes de montanha expoem pedra/cascalho (como o Hills do MC), sem grama nem arvore no paredao
        const int h = hm[x][z]; if (bm[x][z] != B_MOUNTAINS || h <= SEA + 2) continue;
        const int d = std::max(std::max(abs(h - hm[x + 1][z]), abs(h - hm[x - 1][z])), std::max(abs(h - hm[x][z + 1]), abs(h - hm[x][z - 1])));
        BT& t = c->b[idx(x, h, z)]; if (t != GRASS && t != DIRT) continue;
        const float nz = hash2(cx * CW + x, cz * CW + z, 140);
        if (d >= 4) { t = nz > .75f ? (BT)GRAVEL : (BT)STONE; c->b[idx(x, h - 1, z)] = STONE; }
        else if (d == 3 && nz > .4f) t = (BT)STONE;
    }
    // patchTerrain.5: lagos de superficie (como WorldGenLakes): bacia elipsoidal rasa em terreno plano, agua no nivel da coluna mais baixa, fundo de areia; fica longe da borda do chunk
    if (hash2(cx * 131 + 7, cz * 137 + 3, 150) > .65f) {
        const int lx = 5 + (int)(hash2(cx * 131 + 7, cz * 137 + 3, 151) * 5.99f), lz = 5 + (int)(hash2(cx * 131 + 7, cz * 137 + 3, 152) * 5.99f), rx = 3 + (int)(hash2(cx * 131 + 7, cz * 137 + 3, 153) * 2.99f), rz = 3 + (int)(hash2(cx * 131 + 7, cz * 137 + 3, 154) * 2.99f);
        const int bio0 = bm[lx][lz]; int hmin = 1 << 20, hmax = -1; bool ok = bio0 == B_PLAINS || bio0 == B_FOREST || bio0 == B_TAIGA || bio0 == B_SWAMP || bio0 == B_SAVANNA;
        for (int z = std::max(0, lz - rz); z <= std::min(CW - 1, lz + rz) && ok; z++) for (int x = std::max(0, lx - rx); x <= std::min(CW - 1, lx + rx); x++) {
            const int h = hm[x][z]; const BT t = c->b[idx(x, h, z)];
            if (h <= SEA + 1 || h + 7 >= CH || (t != GRASS && t != DIRT) || bm[x][z] == B_DESERT) { ok = false; break; }
            hmin = std::min(hmin, h); hmax = std::max(hmax, h);
        }
        if (ok && hmax - hmin <= 3) {
            const int wl = hmin;
            for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
                const float dx = (float)(x - lx) / (rx + .5f), dz = (float)(z - lz) / (rz + .5f), d = dx * dx + dz * dz; if (d >= 1.f) continue;
                const int dep = 1 + (int)(2.f * sqrtf(1.f - d)); const int hb = wl - dep;   // 1 a 3 blocos de fundo
                for (int y = hb + 1; y <= wl; y++) c->b[idx(x, y, z)] = (BT)WATER;
                for (int y = wl + 1; y <= hm[x][z] + 6; y++) c->b[idx(x, y, z)] = (BT)AIR;
                c->b[idx(x, hb, z)] = (BT)SAND; hm[x][z] = hb;
            }
        }
    }
    // árvores no estilo BiomeDecorator do 1.12.2: população P={C-1,C}^2 (região +8), N tentativas por chunk em posição aleatória
    static const int TREE_N[NBIOME] = {0, 0, 0, 10, 0, 1, 50, 2, 10, 0, 3};   // treesPerChunk por bioma (oceano,praia,planície,floresta,deserto,savana,selva,pântano,taiga,tundra,montanhas)
    static const bool TREE_X[NBIOME] = {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1};     // extraTreeChance 0.1 (deserto/tundra/oceano/praia: sem árvores)
    {
        const int64_t wsd = (int64_t)WORLD_SEED; JR2 q(wsd); const int64_t ka = q.nextLong() / 2 * 2 + 1, kb = q.nextLong() / 2 * 2 + 1;
        const int pzT = pkFind("block/podzol"), cdT = pkFind("block/coarse_dirt");   // patchGen.1: taiga gigante tem podzol/terra grossa no topo (antes so GRASS recebia arvore)
        std::vector<std::pair<int, int>> tcand;   // patchTerrain.3: posicoes candidatas das areas vizinhas (mesma sequencia aleatoria) para evitar troncos colados atraves da borda do chunk
        for (int py2 = cz - 2; py2 <= cz + 1; py2++) for (int px2 = cx - 2; px2 <= cx + 1; px2++) {
            const int pb = biomeAt((float)(px2 * 16 + 16), (float)(py2 * 16 + 16));
            JR2 r((int64_t)(((uint64_t)((int64_t)px2 * ka) + (uint64_t)((int64_t)py2 * kb)) ^ (uint64_t)wsd) ^ 0x7E57ABLL);
            int n = TREE_N[pb]; if (TREE_X[pb] && r.nextFloat() < 0.1f) n++;
            if (pb == B_FOREST) { const float g = vnoise((px2 * 16 + 8) * .018f + 41.f, (py2 * 16 + 8) * .018f - 13.f); n = std::max(2, (int)(n * (.45f + 1.0f * g) + .5f)); }
            { const int rv = biomeRawAt((float)(px2 * 16 + 16), (float)(py2 * 16 + 16)), rb = rv >= 128 ? rv - 128 : rv; if (rb == 29) n = std::max(n, 20); else if (rb == 32 || rb == 33) n = std::max(n, 10); }   // patchGen.1
            for (int t = 0; t < n; t++) { const int tx = px2 * 16 + 8 + r.nextInt(16), tz = py2 * 16 + 8 + r.nextInt(16); tcand.push_back({tx, tz}); }
        }
        for (int pz = cz - 1; pz <= cz; pz++) for (int px = cx - 1; px <= cx; px++) {
            const int pb = biomeAt((float)(px * 16 + 16), (float)(pz * 16 + 16));
            JR2 r((int64_t)(((uint64_t)((int64_t)px * ka) + (uint64_t)((int64_t)pz * kb)) ^ (uint64_t)wsd) ^ 0x7E57ABLL);
            int n = TREE_N[pb]; if (TREE_X[pb] && r.nextFloat() < 0.1f) n++;
            if (pb == B_FOREST) { const float g = vnoise((px * 16 + 8) * .018f + 41.f, (pz * 16 + 8) * .018f - 13.f); n = std::max(2, (int)(n * (.45f + 1.0f * g) + .5f)); }   // patchTerrain.2: floresta com clareiras e trechos mais fechados (antes: densidade uniforme)
            { const int rv = biomeRawAt((float)(px * 16 + 16), (float)(pz * 16 + 16)), rb = rv >= 128 ? rv - 128 : rv; if (rb == 29) n = std::max(n, 20); else if (rb == 32 || rb == 33) n = std::max(n, 10); }   // patchGen.1: roofed 20, taiga gigante 10 (como o BiomeDecorator do 1.12)
            for (int t = 0; t < n; t++) {
                const int wx = px * 16 + 8 + r.nextInt(16), wz = pz * 16 + 8 + r.nextInt(16);
                const int x = wx - cx * CW, z = wz - cz * CW; if (x < 0 || x >= CW || z < 0 || z >= CW) continue;
                { bool near = false; for (const auto& tc : tcand) { if (tc.first == wx && tc.second == wz) continue; if (abs(tc.first - wx) <= 1 && abs(tc.second - wz) <= 1 && (tc.first < wx || (tc.first == wx && tc.second < wz))) { near = true; break; } }
                  if (near) continue; }   // candidata vizinha "anterior" (ordem fixa) colada: descarta; igual nos dois chunks da borda
                if (vilTreeBlocked(wx, wz)) continue;   // sem arvores dentro/perto de vilas
                const int h = hm[x][z], bio = bm[x][z], tb = c->b[idx(x, h, z)];
                if (h <= SEA || h + 16 >= CH || (tb != GRASS && !(tb == SNOWBLK && bio == B_TUNDRA) && !(pzT > 0 && tb == pzT) && !(cdT > 0 && tb == cdT))) continue;
                { bool bad = false;   // patchTerrain.2: como o canGrow do MC: sem tronco dentro de outro tronco/pedra/agua e sem troncos colados (2 blocos de folga)
                  for (int dz = -1; dz <= 1 && !bad; dz++) for (int dx = -1; dx <= 1 && !bad; dx++) { const int xx = x + dx, zz = z + dz; if (xx < 0 || xx >= CW || zz < 0 || zz >= CW) continue;
                      for (int dy = 1; dy <= 3; dy++) { const int q = c->b[idx(xx, h + dy, zz)]; if (q != AIR && !isLeafB(q) && !(q >= XB_PLANT && q < XB_PLANT + 24)) { bad = true; break; } } }
                  if (bad) continue; }
                placeTree(c, x, h, z, wx, wz, bio);
            }
        }
    }
    const int redSd = pkFind("block/red_sand"), cactus = pkFind("block/cactus"), pump = pkFind("block/pumpkin"), melon = pkFind("block/melon"), podzol = pkFind("block/podzol"), coarse = pkFind("block/coarse_dirt"), mossy = pkFind("block/mossy_cobblestone");
    for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
        const int wx = cx * CW + x, wz = cz * CW + z, h = hm[x][z], bio = bm[x][z]; if (h < 8 || h + 6 >= CH) continue;
        BT& tb = c->b[idx(x, h, z)];
        if (bio == B_DESERT && (tb == SAND || (redSd > 0 && tb == redSd)) && cactus && h > SEA + 1 && x > 0 && x < CW - 1 && z > 0 && z < CW - 1 && hash2(wx, wz, 123) > .993f) {
            const int k = 1 + (int)(hash2(wx, wz, 124) * 2.99f); bool ok = true;
            for (int i = 1; i <= k && ok; i++) if (c->b[idx(x, h + i, z)] || c->b[idx(x + 1, h + i, z)] || c->b[idx(x - 1, h + i, z)] || c->b[idx(x, h + i, z + 1)] || c->b[idx(x, h + i, z - 1)]) ok = false;
            if (ok) for (int i = 1; i <= k; i++) c->b[idx(x, h + i, z)] = (BT)cactus;
        } else if (tb == GRASS && h > SEA + 1 && c->b[idx(x, h + 1, z)] == AIR) {
            if ((bio == B_PLAINS || bio == B_FOREST) && pump && hash2(wx, wz, 125) > .9985f) c->b[idx(x, h + 1, z)] = (BT)pump;
            else if (bio == B_JUNGLE && melon && hash2(wx, wz, 126) > .997f) c->b[idx(x, h + 1, z)] = (BT)melon;
            else if (bio == B_TAIGA && podzol && vnoise(wx * .07f + 31.f, wz * .07f) > .58f) tb = (BT)podzol;
            else if (bio == B_SAVANNA && coarse && vnoise(wx * .09f + 77.f, wz * .09f - 5.f) > .62f) tb = (BT)coarse;
            else if (bio == B_TAIGA && mossy && x > 1 && x < CW - 2 && z > 1 && z < CW - 2 && h + 3 < CH && hash2(wx, wz, 128) > .9975f) {   // patchTerrain.1: pedregulho musgoso da taiga (cruz de 5 blocos de altura 1-2)
                c->b[idx(x, h, z)] = (BT)mossy; c->b[idx(x, h + 1, z)] = (BT)mossy;
                static const int AX[4] = {1, -1, 0, 0}, AZ[4] = {0, 0, 1, -1};
                for (int k = 0; k < 4; k++) if (hash2(wx + AX[k], wz + AZ[k], 129) > .35f) { const int ay = c->b[idx(x + AX[k], h, z + AZ[k])] == AIR ? h : h + 1; if (c->b[idx(x + AX[k], ay, z + AZ[k])] == AIR || ay == h + 1) c->b[idx(x + AX[k], ay, z + AZ[k])] = (BT)mossy; }
            }
            else xbVeg(c, x, h, z, wx, wz, bio);   // grama, samambaia, flores
        } else if (bio == B_DESERT && (tb == SAND || (redSd > 0 && tb == redSd)) && h > SEA + 1 && xbHasModel(XB_DEADBUSH) && c->b[idx(x, h + 1, z)] == AIR && hash2(wx, wz, 127) > .987f) {
            c->b[idx(x, h + 1, z)] = (BT)XB_DEADBUSH;   // patchTerrain.1: arbusto morto no deserto
        } else if (bio == B_SWAMP && h < SEA && h >= SEA - 4 && xbHasModel(XB_LILY) && isWater(c->b[idx(x, SEA - 1, z)]) && c->b[idx(x, SEA, z)] == AIR && vnoise(wx * .13f + 3.f, wz * .13f - 8.f) > .56f && hash2(wx, wz, 130) > .55f) {
            c->b[idx(x, SEA, z)] = (BT)XB_LILY;   // patchTerrain.1: vitoria-regia em manchas no pantano
        } else if (h < SEA - 1 && h >= SEA - 9 && tb == SAND && vnoise(wx * .11f + 5.f, wz * .11f + 9.f) > .74f) tb = CLAY;   // manchas de argila no fundo
    }
    placeVillages(c, cx, cz);                    // vilas (village.inc)
    W[key(cx, cz)] = c; gKick = true; gEpoch++;
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(cx + dx, cz + dz);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
// ---------- iluminação por voxel (céu + bloco), calculada numa região 3x3 chunks ----------
static const int RW = CW * 3;
static BT rBlk[RW * CH * RW]; static uint8_t rSky[RW * CH * RW], rBl[RW * CH * RW], rBt[RW * CH * RW];
static uint8_t rBc[8][RW * CH * RW]; static unsigned gMixMask = 0; static std::vector<int32_t> rEmi;   // rBc[t] = luz de bloco SO do tipo de emissor t (so preenchido quando ha 2+ tipos na regiao: mistura de cores)   // rBt = cor (tipo de emissor) da luz de bloco em cada celula
static inline int rI(int x, int y, int z) { return (y * RW + z) * RW + x; }
static std::vector<int32_t> lq;
static int rTop = CH - 1;   // acima disso so existe ar (ceu 15)
static inline int dimAmb();   // luz de bloco minima: Nether 7, End 10 (sem luz do ceu)
static const int NETHER_AMB = 7;   // nivel de luz de bloco minimo no Nether (~0.27 de brilho)
static inline int dimAmb() { return gDim == 2 ? 10 : NETHER_AMB; }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O2")   // patchPerf.4: luz e malha sempre em O2 (o build padrao e -O1): malha 5,6 -> 2,5 ms/chunk no bench
#endif
static void floodLight(uint8_t* L, uint8_t* T = nullptr) {
    static const int dx[6] = {1,-1,0,0,0,0}, dy[6] = {0,0,1,-1,0,0}, dz[6] = {0,0,0,0,1,-1};
    for (size_t h = 0; h < lq.size(); h++) {
        int i = lq[h], x = i % RW, z = (i / RW) % RW, y = i / (RW * RW), lv = L[i];
        if (lv <= 1) continue;
        for (int d = 0; d < 6; d++) {
            int nx = x + dx[d], ny = y + dy[d], nz = z + dz[d];
            if (nx < 0 || nx >= RW || nz < 0 || nz >= RW || ny < 0 || ny > rTop) continue;
            int j = rI(nx, ny, nz), o = gLOpLut[rBlk[j]];
            if (o >= 15) continue;
            int nl = lv - std::max(1, o);
            if (nl > L[j]) { L[j] = (uint8_t)nl; if (T) T[j] = T[i]; lq.push_back(j); }
        }
    }
}
static void computeLight(int cx, int cz) {
    int top = 0;
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (Chunk* n = getC(cx + dx, cz + dz)) top = std::max(top, chunkTopFresh(n));
    rTop = std::min(CH - 1, top + 2);
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) {
        Chunk* n = getC(cx + dx, cz + dz); int ox = (dx + 1) * CW, oz = (dz + 1) * CW;
        for (int y = 0; y <= rTop; y++) for (int z = 0; z < CW; z++) {
            if (n) memcpy(&rBlk[rI(ox, y, oz + z)], &n->b[idx(0, y, z)], CW * sizeof(BT)); else for (int q = 0; q < CW; q++) rBlk[rI(ox + q, y, oz + z)] = STONE;
        }
    }
    memset(rBl, 0, (size_t)(rTop + 1) * RW * RW); gMixMask = 0;
    lightLutRefresh(); lq.clear(); if (lq.capacity() < (1u << 16)) lq.reserve(1u << 16);
    static int16_t rHm[RW * RW];   // heightmap: y do primeiro bloco que filtra luz (de cima p/ baixo), -1 = coluna aberta
    for (int z = 0; z < RW; z++) for (int x = 0; x < RW; x++) {   // luz do ceu: coluna vertical
        int lvl = 15, h = -1;
        for (int y = rTop; y >= 0; y--) {
            const int i = rI(x, y, z), o = gLOpLut[rBlk[i]];
            if (h < 0 && o > 0) h = y;
            lvl = o >= 15 ? 0 : std::max(0, lvl - o);
            rSky[i] = (uint8_t)lvl;
        }
        rHm[z * RW + x] = (int16_t)h;
    }
    // Chunk LONGE (gMeshFar, a nevoa ja o esconde): luz so por coluna, sem propagacao lateral nem luz de bloco (como o LOD de luz dos mods de otimizacao).
    if (!gMeshFar) {
    // So entram na fila as celulas perto de uma superficie (y <= maior heightmap entre a coluna e as 4 vizinhas, +1): acima disso toda a
    // vizinhanca ja e 15 e nada pode melhorar. Resultado identico ao flood completo, com uma fracao do trabalho (ideia do Starlight).
    for (int z = 0; z < RW; z++) for (int x = 0; x < RW; x++) {
        int m = rHm[z * RW + x];
        if (x > 0) m = std::max(m, (int)rHm[z * RW + x - 1]); if (x < RW - 1) m = std::max(m, (int)rHm[z * RW + x + 1]);
        if (z > 0) m = std::max(m, (int)rHm[(z - 1) * RW + x]); if (z < RW - 1) m = std::max(m, (int)rHm[(z + 1) * RW + x]);
        for (int y = std::min(rTop, m + 1); y >= 0; y--) { const int i = rI(x, y, z); if (rSky[i] >= 2) lq.push_back(i); }
    }
    floodLight(rSky);
    lq.clear();
    rEmi.clear(); unsigned mk = 0;
    auto rFurnLit = [&](int i) { const int b = rBlk[i]; if (!b || b != gFurnId) return false; const int x = i % RW, z = (i / RW) % RW, y = i / (RW * RW); return (getM((cx - 1) * CW + x, y, (cz - 1) * CW + z) & 4) != 0; };   // fornalha acesa emite 13
    std::unordered_map<int, uint8_t> rPl;
    for (const auto& L : gPropLightV) { const int x = L[0] - (cx - 1) * CW, z = L[2] - (cz - 1) * CW; if (x >= 0 && x < RW && z >= 0 && z < RW && L[1] >= 0 && L[1] <= rTop) { uint8_t& v = rPl[rI(x, L[1], z)]; v = std::max<uint8_t>(v, (uint8_t)L[3]); } }
    auto rEm = [&](int i) -> uint8_t { uint8_t e = rFurnLit(i) ? (uint8_t)13 : gEmitLut[rBlk[i]]; if (!rPl.empty()) { auto f = rPl.find(i); if (f != rPl.end() && f->second > e) e = f->second; } return e; };
    auto rUnlit = [&](int i) { const int b = rBlk[i]; if (b < XB_RTORCH || b >= XB_RTORCH + 5) return false; const int x = i % RW, z = (i / RW) % RW, y = i / (RW * RW); return (getM((cx - 1) * CW + x, y, (cz - 1) * CW + z) & 1) != 0; };   // tocha de redstone apagada (meta bit0) nao emite
    const bool rNoPl = rPl.empty();   // patchRpi.7: sem luz de prop, ar nao emite: pula sem consultar tabelas
    for (int i = 0, ne = (rTop + 1) * RW * RW; i < ne; i++) if (rNoPl && !rBlk[i]) continue; else if (const uint8_t e = rUnlit(i) ? 0 : rEm(i)) { rBl[i] = e; rBt[i] = gTintLut[rBlk[i]]; mk |= 1u << rBt[i]; rEmi.push_back(i); lq.push_back(i); }
    floodLight(rBl, rBt);
    if (mk & (mk - 1)) {   // 2+ cores de luz na regiao: propaga cada tipo separado para os vertices poderem MISTURAR as cores (laranja + ciano = branco)
        gMixMask = mk;
        for (int t = 0; t < 8; t++) if (mk >> t & 1) {
            memset(rBc[t], 0, (size_t)(rTop + 1) * RW * RW); lq.clear();
            for (int i : rEmi) if (gTintLut[rBlk[i]] == t) { rBc[t][i] = rUnlit(i) ? 0 : rEm(i); lq.push_back(i); }
            floodLight(rBc[t]);
        }
    }
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
// patchWater.1: agua na borda de um chunk que acabou de ser gerado (ou de outro que ja existia ao lado) escorre para o espaco novo, como o vanilla ao carregar vizinhos
static void waterEdgeWake(int cx, int cz) {
    static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int d = 0; d < 4; d++) {
        if (!getC(cx + D[d][0], cz + D[d][1])) continue;
        for (int t = 0; t < CW; t++) {
            const int ax = D[d][0] ? (D[d][0] > 0 ? cx * CW + CW - 1 : cx * CW) : cx * CW + t, az = D[d][1] ? (D[d][1] > 0 ? cz * CW + CW - 1 : cz * CW) : cz * CW + t;
            const int bx = ax + D[d][0], bz = az + D[d][1];   // celula do vizinho encostada em (ax,az)
            for (int y = 1; y < 128; y++) {
                const int A = getB(ax, y, az), B = getB(bx, y, bz);
                if (isWater(A) && B == AIR) waterSchedule(ax, y, az);
                else if (isWater(B) && A == AIR) waterSchedule(bx, y, bz);
            }
        }
    }
}
static void gen(int cx, int cz) {   // mede a geracao (profiler em perf.inc)
    const double t0 = perfNow(); genImpl(cx, cz); const double ms = perfNow() - t0;
    gPf[PS_GEN] += ms; gPfGenN++; if (ms > gPfGenWorst) gPfGenWorst = ms;
    if (getC(cx, cz)) waterEdgeWake(cx, cz);
}
// cor da luz de bloco numa celula LOCAL (x,y,z) do chunk em construcao (so dentro de build() sem waterOnly): usada pelos modelos (tocha, haste do End, plantas)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O2")   // patchPerf.4: luz e malha sempre em O2 (o build padrao e -O1): malha 5,6 -> 2,5 ms/chunk no bench
#endif
static int regionChroma(int x, int y, int z) {
    if (y < 0 || y > rTop) return CC_DEF;
    const int i = rI(x + CW, y, z + CW); if (rBl[i] <= 0) return CC_DEF;
    float bc[3] = {0, 0, 0};
    if (gMixMask) { for (int t = 0; t < 8; t++) if ((gMixMask >> t & 1) && rBc[t][i]) { const float f = ltabB((float)rBc[t][i]); for (int k = 0; k < 3; k++) bc[k] += TINTC[t][k] * f; } }
    else for (int k = 0; k < 3; k++) bc[k] = TINTC[rBt[i]][k];
    const float mx = std::max(bc[0], std::max(bc[1], bc[2])); return mx > 1e-4f ? chromaCode(bc[0] / mx, bc[1] / mx, bc[2] / mx) : CC_DEF;
}
static void build(int cx, int cz, Chunk* c, bool waterOnly = false) {
    PerfBuildScope perfBuildScope;
    static const int D[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    static const int Q[6][4][3] = {
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}}, {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
        {{0,1,0},{1,1,0},{1,1,1},{0,1,1}}, {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}};
    static const float SH[6] = {.8f, .8f, 1.f, .5f, .65f, .65f};
    static const float UV[4][2] = {{0,1},{1,1},{1,0},{0,0}};
    c->wmesh.clear(); if (!waterOnly) c->hasTrans = false;
    std::vector<V> glassQ; gXbGlass = &glassQ;   // vidro colorido/gelo/slime: junta aqui e entra no fim de wmesh (a agua tem shader proprio; o vidro, nao)
    c->dirty = false; c->wdirty = false; if (!waterOnly) c->farM = gMeshFar;
    const int topY = chunkTopFresh(c);
    if (!waterOnly) {
        c->mesh.clear();
        { const double tl = perfNow(); computeLight(cx, cz); gBuildLight = perfNow() - tl; }
        if (!c->lc) c->lc = new uint8_t[CW * CH * CW];   // luz em cache (ceu<<4 | bloco): a malha da agua e refeita sem recalcular a luz
        for (int y = 0; y < CH; y++) for (int z = 0; z < CW; z++) for (int x = 0; x < CW; x++) {
            if (y > rTop) { c->lc[idx(x, y, z)] = (uint8_t)((gDim == 2 ? 0 : 15) << 4) | (gDim == 2 ? dimAmb() : 0); continue; }
            const int ri = rI(x + CW, y, z + CW); int sk0 = rSky[ri], bl0 = rBl[ri] & 15;
            { const int bb = c->b[idx(x, y, z)]; if (bb == XB_STAIR || bb == XB_SLAB) {   // useNeighborBrightness: a celula opaca nao guarda luz; usa a maior entre cima e os 4 lados
                static const int NX[5] = {0, 1, -1, 0, 0}, NY[5] = {1, 0, 0, 0, 0}, NZ[5] = {0, 0, 0, 1, -1};
                for (int k = 0; k < 5; k++) { const int yy = y + NY[k]; if (yy > rTop) { sk0 = 15; continue; } const int rj = rI(x + CW + NX[k], yy, z + CW + NZ[k]); sk0 = std::max<int>(sk0, rSky[rj]); bl0 = std::max<int>(bl0, rBl[rj] & 15); } } }
            c->lc[idx(x, y, z)] = (uint8_t)(((gDim == 2 ? 0 : sk0) << 4) | (gDim ? std::max<int>(bl0, dimAmb()) : bl0));
        }
    }
    auto gb = [&](int x, int y, int z) -> int {
        if (y < 0) return STONE;
        if (y >= CH) return AIR;
        const int lx = x - cx * CW, lz = z - cz * CW;
        if ((unsigned)lx < (unsigned)CW && (unsigned)lz < (unsigned)CW) return c->b[idx(lx, y, lz)];
        return getB(x, y, z);
    };
    // Altura dos cantos da agua, igual ao BlockFluidRenderer do Minecraft 1.8: media dos 4 blocos que dividem o canto.
    // Cantos vizinhos compartilham o mesmo valor, entao a superficie e continua (sem degraus).
    auto cornerH = [&](int px, int y, int pz, bool lv = false) -> float {
        float f = 0.f; int n = 0;
        for (int j = 0; j < 4; j++) {
            const int bx = px - (j & 1), bz = pz - ((j >> 1) & 1);
            if (lv ? isLava(gb(bx, y + 1, bz)) : isWater(gb(bx, y + 1, bz))) return 1.f;
            const int bb = gb(bx, y, bz);
            if (lv ? isLava(bb) : isWater(bb)) {
                const int m = lv ? lavaMeta(bb) : liqMeta(bb); const float pct = ((m >= 8 ? 0 : m) + 1) / 9.f;
                if (m >= 8 || m == 0) { f += pct * 10.f; n += 10; }
                f += pct; n++;
            } else if (!solid(bb)) { f += 1.f; n++; }
        }
        return n ? 1.f - f / n : .875f;
    };
    float tg[(CW + 1) * (CW + 1)][3], tl[(CW + 1) * (CW + 1)][3];   // cor de bioma nos cantos dos blocos (gradiente suave)
    if (!waterOnly) biomeTintsChunk(cx, cz, tg, tl);   // patchBio.2: grade suave em cache
    auto cellInfo = [&](const int* p, int& sk, int& bl, bool& op, int* tn = nullptr, int* ci = nullptr) {
        if (tn) *tn = 0; if (ci) *ci = -1;
        if (p[1] < 0) { sk = 0; bl = 0; op = true; return; }
        if (p[1] > rTop) { sk = gDim == 2 ? 0 : 15; bl = gDim == 2 ? dimAmb() : 0; op = false; return; }
        int i = rI(p[0], p[1], p[2]); op = gOpacLut[rBlk[i]] >= 15; sk = gDim == 2 ? 0 : rSky[i]; bl = rBl[i]; if (tn && bl > 0) *tn = rBt[i]; if (ci) *ci = i;
        if (rBlk[i] == XB_STAIR || rBlk[i] == XB_SLAB) {   // patchLeves.2: celula de escada/laje guarda luz 0 (bloqueia); o MC usa o maximo dos 6 vizinhos (useNeighborBrightness) => blocos inteiros ao lado nao escurecem
            static const int D6[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
            for (const auto& d : D6) { const int nx = p[0] + d[0], ny = p[1] + d[1], nz = p[2] + d[2]; if (nx < 0 || nx >= RW || nz < 0 || nz >= RW || ny < 0 || ny > rTop) continue; const int j = rI(nx, ny, nz); if (gDim != 2 && rSky[j] > sk) sk = rSky[j]; if (rBl[j] > bl) { bl = rBl[j]; if (tn) *tn = rBt[j]; } }
        }
        if (gDim && bl < dimAmb()) { if (bl == 0 && tn) *tn = 0; bl = dimAmb(); }   // Nether: luz ambiente minima (sem ceu)
    };

    std::vector<int16_t> hsC(gMeshFar ? RW * RW : 0, (int16_t)-2);   // topo solido por coluna da regiao (lazy), so no modo longe
    if (!waterOnly) c->sOff[0] = 0; c->wOff[0] = 0;
    for (int y = 0; y <= topY; y++) {
        if ((y & 15) == 0 && y > 0) { const int sc = y >> 4; if (!waterOnly) c->sOff[sc] = (uint32_t)c->mesh.size(); c->wOff[sc] = (uint32_t)c->wmesh.size(); }
        for (int z = 0; z < CW; z++)
            for (int x = 0; x < CW; x++) {
                int b = c->b[idx(x, y, z)];
                if (!b) continue;
                int wx = cx * CW + x, wz = cz * CW + z;
                if (!gPAnim.empty() && pAnimHides(wx, y, wz)) continue;   // pistao: o bloco desliza como entidade (drawPistonAnims)
                bool water = isWater(b);
                if (waterOnly && !water) continue;
                if (isLava(b)) {   // lava: opaca, emissiva (luz 15), na malha normal; altura dos cantos suave como a agua
                    const int above = gb(wx, y + 1, wz), below = gb(wx, y - 1, wz);
                    const bool needTop = !isLava(above), needBot = !(isLava(below) || lightOpac(below) >= 15);
                    static const int lfS[4] = {0, 1, 4, 5}, lnx[4] = {1, -1, 0, 0}, lnz[4] = {0, 0, 1, -1};
                    bool lop[4]; bool lany = needTop || needBot;
                    for (int f = 0; f < 4; ++f) { const int nb = gb(wx + lnx[f], y, wz + lnz[f]); lop[f] = !(isLava(nb) || lightOpac(nb) >= 15); lany |= lop[f]; }
                    if (!lany) continue;
                    const int lt = gLavaTile > 0 ? gLavaTile : TILES[WATER][1];
                    const float lu0 = (lt % ATL) / (float)ATL, lv0 = (lt / ATL) / (float)ATLR, lu1 = lu0 + 1.f / ATL, lv1 = lv0 + 1.f / ATLR;
                    float lh[2][2]; for (int ox = 0; ox < 2; ox++) for (int oz = 0; oz < 2; oz++) lh[ox][oz] = cornerH(wx + ox, y, wz + oz, true);
                    const float la = gShaderOK ? packLight(0.f, 15.75f) : 1.f;
                    std::vector<V>& lo = c->mesh;
                    if (needTop) {
                        lo.push_back({(float)wx,     y + lh[0][0], (float)wz,     lu0, lv0, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx + 1, y + lh[1][0], (float)wz,     lu1, lv0, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx + 1, y + lh[1][1], (float)wz + 1, lu1, lv1, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx,     y + lh[0][1], (float)wz + 1, lu0, lv1, 1.f, 1.f, 1.f, la});
                    }
                    for (int f = 0; f < 4; ++f) {
                        if (!lop[f]) continue;
                        const int qf = lfS[f];
                        for (int k = 0; k < 4; ++k) {
                            const int qx = (int)Q[qf][k][0], qz = (int)Q[qf][k][2];
                            const float yy = (Q[qf][k][1] > .5f) ? lh[qx][qz] : 0.f;
                            lo.push_back({(float)(wx + qx), y + yy, (float)(wz + qz), (k == 0 || k == 3) ? lu0 : lu1, (k < 2) ? lv1 : lv0, 1.f, 1.f, 1.f, la});
                        }
                    }
                    if (needBot) {
                        lo.push_back({(float)wx,     (float)y, (float)wz,     lu0, lv0, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx + 1, (float)y, (float)wz,     lu1, lv0, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx + 1, (float)y, (float)wz + 1, lu1, lv1, 1.f, 1.f, 1.f, la});
                        lo.push_back({(float)wx,     (float)y, (float)wz + 1, lu0, lv1, 1.f, 1.f, 1.f, la});
                    }
                    continue;
                }
                if (isXB(b)) { if (!gMeshFar) emitXB(c, b, x, y, z, wx, wz); continue; }   // modelos (tocha, alavanca, bigorna, plantas)

                if (!water) {
                    const int bxr = x + CW, bzr = z + CW;
                    const int axm = gBlkAxis[b] ? (c->m[idx(x, y, z)] & 3) : 0;   // eixo do bloco (0 Y, 1 X, 2 Z); 3 = tratado como Y
                    const int axd = axm == 1 ? 0 : axm == 2 ? 2 : 1;                // dimensao do eixo: 0 X, 1 Y, 2 Z
                    const bool emitter = lightEmit(b) > 0; const int lk = leafKind(b);
                    int leafN = 0; if (gLeafCull && isLeafB(b)) for (int f2 = 0; f2 < 6; f2++) { const int y2 = y + D[f2][1]; if (y2 >= 0 && y2 <= rTop && isLeafB(rBlk[rI(bxr + D[f2][0], y2, bzr + D[f2][2])])) leafN++; }   // patchGfx.1: vizinhas folha (aglomerado denso: 4+)
                    for (int f = 0; f < 6; f++) {
                        int Fc[3] = {bxr + D[f][0], y + D[f][1], bzr + D[f][2]};
                        int nb = Fc[1] < 0 ? STONE : Fc[1] > rTop ? AIR : rBlk[rI(Fc[0], Fc[1], Fc[2])];
#ifdef BL_RPI
                        if (isLeafB(b) && isLeafB(nb)) continue;   // Raspberry: folha so mostra face voltada para o ar (culling)
#endif
                        if (lightOpac(nb) >= 15) continue;   // folhas/água/ar não escondem a face (folhas "fancy")
                        const int tk = transKind(b); if (tk && nb == b) continue;   // vidro/gelo/slime: sem faces internas entre blocos iguais (como o MC)
                        if (b == gCactusId && b && nb == b && f >= 2 && f <= 3) continue;   // cacto empilhado: topo/base coincidem no mesmo plano (a textura de cima e a de baixo brigariam = pisca)
                        const bool leafPair = isLeafB(b) && isLeafB(nb), leafMix = leafPair && nb != b;   // especies diferentes encostadas: cada uma mostra sua propria face (sem misturar texturas num plano so)
                        if (leafPair && gLeafFast) continue;   // patchGfx.2: Graficos Rapidos: folhas opacas, so as faces expostas ao ar (culling total)
                        if (leafPair && leafN >= 6) continue;   // patchGfx.1: folha dentro de aglomerado: sem faces voltadas para outra folha (so as expostas ao ar)
                        if (leafPair && !leafMix && (f & 1)) continue;   // folha-folha: as duas faces ficam no mesmo plano (z-fighting = pisca); desenha só uma
                        // topo/base so nas faces perpendiculares ao eixo; nas outras o lado. Em eixo horizontal o grao da casca gira 90 graus
                        // nas faces cujo "v" da textura (Y nos lados, Z em cima/baixo) nao acompanha o eixo
                        const bool endF = axm != 0 && axm != 3 && (f >> 1) == axd;
                        const bool rotUV = axm != 0 && axm != 3 && !endF && (((f >> 1) == 1) ? 2 : 1) != axd;
                        int tile = (axm != 0 && axm != 3) ? blkTile(b, endF ? 0 : 1) : blkTile(b, f == 2 ? 0 : f == 3 ? 2 : 1);
                        if (b == gEndFrameId && b && (c->m[idx(x, y, z)] & 4) && f != 3) { const int tf = f == 2 ? gEndTopF : gEndSideF; if (tf >= 0) tile = tf; }   // moldura com olho: texturas *_f
                        if (gBlkFacing[b]) {   // frente: so a face horizontal que aponta para a direcao do bloco usa a textura do rosto
                            static const int FDIR[6] = {1, 3, -1, -1, 2, 0};   // face do mesher (+X,-X,+Y,-Y,+Z,-Z) -> direcao 0 N,1 L,2 S,3 O
                            const int ft = pkFront(b); if (ft >= 0 && FDIR[f] == (c->m[idx(x, y, z)] & 3)) tile = (b == gFurnId && gFurnOnTile >= 0 && (c->m[idx(x, y, z)] & 4)) ? gFurnOnTile : ft;   // fornalha acesa: furnace_front_on
                        }
                        const int tx0 = tile % ATL, ty0 = tile / ATL;
                        const int ax = f / 2, t1 = (ax + 1) % 3, t2 = (ax + 2) % 3;
                        float col[4][3], ucol[4][3], lp[4];   // col = cor com a luz do meio-dia (assada); ucol = só sombreamento (modo shader); lp = luz empacotada
                        int skF, blF, tF, iF; bool opF; cellInfo(Fc, skF, blF, opF, &tF, &iF);
                        if (gMeshFar && !emitter && (f == 3 || (skF < 8 && blF == 0))) {   // modo longe: so corta face ENTERRADA (com teto solido acima). Antes cortava pela luz do ceu, que a agua apaga -> fundo do oceano transparente e buracos no terreno
                            auto hsOf = [&](int hx, int hz) -> int { if (hx < 0 || hz < 0 || hx >= RW || hz >= RW) return -1; const int k = hz * RW + hx; if (hsC[k] == -2) { int hy = -1; for (int yy = rTop; yy >= 0; yy--) if (gOpacLut[rBlk[rI(hx, yy, hz)]] >= 15) { hy = yy; break; } hsC[k] = (int16_t)hy; } return hsC[k]; };
                            // patchShader.2: so corta se a celula esta coberta na coluna E nas 4 vizinhas (enterrada de verdade); antes um beiral/penhasco cortava faces visiveis de lado = buracos no terreno distante
                            if (Fc[1] < hsOf(Fc[0], Fc[2]) && Fc[1] < hsOf(Fc[0] + 1, Fc[2]) && Fc[1] < hsOf(Fc[0] - 1, Fc[2]) && Fc[1] < hsOf(Fc[0], Fc[2] + 1) && Fc[1] < hsOf(Fc[0], Fc[2] - 1)) continue;
                        }   // modo longe: face no escuro total (parede de caverna fundo no subsolo) nao e visivel de longe -> nao gera (corta ~3/4 dos vertices)
                        for (int k = 0; k < 4; k++) {
                            if (emitter) { col[k][0] = col[k][1] = col[k][2] = 1.f; ucol[k][0] = ucol[k][1] = ucol[k][2] = 1.f; lp[k] = packLight(0.f, 15.75f); continue; }
                            int s1 = Q[f][k][t1] * 2 - 1, s2 = Q[f][k][t2] * 2 - 1;
                            int A[3] = {Fc[0], Fc[1], Fc[2]}, B[3] = {Fc[0], Fc[1], Fc[2]}, C[3] = {Fc[0], Fc[1], Fc[2]};
                            A[t1] += s1; B[t2] += s2; C[t1] += s1; C[t2] += s2;
                            int skA, blA, skB, blB, skC, blC, tA, tB, tC, iA, iB, iC; bool oA, oB, oC;
                            cellInfo(A, skA, blA, oA, &tA, &iA); cellInfo(B, skB, blB, oB, &tB, &iB); cellInfo(C, skC, blC, oC, &tC, &iC);
                            if (oA) { skA = skF; blA = blF; tA = tF; iA = iF; }
                            if (oB) { skB = skF; blB = blF; tB = tF; iB = iF; }
                            if (oC || (oA && oB)) { skC = skF; blC = blF; tC = tF; iC = iF; }
#ifdef BL_RPI
                            if (true) {
#else
                            if (gSmoothMode == 0) {
#endif
                              oA = oB = oC = false; skA = skB = skC = skF; blA = blB = blC = blF; tA = tB = tC = tF; iA = iB = iC = iF; }   // luz PLANA por face (estilo Alpha; sempre no Raspberry): so a celula da frente, sem media de vertices nem AO
                            int tn = tF, tbl = blF; if (blA > tbl) { tbl = blA; tn = tA; } if (blB > tbl) { tbl = blB; tn = tB; } if (blC > tbl) { tbl = blC; tn = tC; }   // cor da luz: a do emissor mais forte ao redor do vertice
                            float sk = (skF + skA + skB + skC) * .25f, bl = (blF + blA + blB + blC) * .25f;
                            float bc[3], blv = bl; int cc = CC_DEF;   // cor da luz de bloco no vertice
                            if (gMixMask) {   // 2+ tipos de emissor: soma a contribuicao de cada tipo (media das 4 celulas) = cores se misturam
                                bc[0] = bc[1] = bc[2] = 0.f;
                                for (int t = 0; t < 8; t++) if (gMixMask >> t & 1) {
                                    const uint8_t* R = rBc[t]; const float s = ((iF >= 0 ? R[iF] : 0) + (iA >= 0 ? R[iA] : 0) + (iB >= 0 ? R[iB] : 0) + (iC >= 0 ? R[iC] : 0)) * .25f;
                                    if (s > .01f) { const float f = ltabB(s); for (int q = 0; q < 3; q++) bc[q] += TINTC[t][q] * f; }
                                }
                                const float mx = std::max(bc[0], std::max(bc[1], bc[2])), B1 = std::min(1.f, mx);
                                if (mx > .004f) { blv = std::min(15.75f, std::max(0.f, ltabBInv(B1))); cc = chromaCode(bc[0] / mx, bc[1] / mx, bc[2] / mx); for (int q = 0; q < 3; q++) bc[q] = std::min(1.f, bc[q]); }
                                else { blv = 0.f; bc[0] = bc[1] = bc[2] = 0.f; }
                            } else { const float f = bl > .01f ? ltabB(bl) : 0.f; for (int q = 0; q < 3; q++) bc[q] = TINTC[tn][q] * f; if (bl > .01f) cc = chromaCode(TINTC[tn][0], TINTC[tn][1], TINTC[tn][2]); }
#ifdef BL_RPI
                            int ao = 3;   // Raspberry: sem oclusao ambiente
#else
                            int ao = gSmoothMode < 2 ? 3 : (oA && oB) ? 0 : 3 - (int)oA - (int)oB - (int)oC;   // oclusão ambiente por vértice (so em Maxima)
#endif
                            static const float AOM[4] = {.50f, .68f, .84f, 1.f};
                            float sf = ltab(sk), bf = bl > .01f ? ltabB(bl) : 0.f, m = SH[f] * AOM[ao];
                            col[k][0] = std::max(sf, bc[0]) * m; col[k][1] = std::max(sf, bc[1]) * m; col[k][2] = std::max(sf, bc[2]) * m;
                            ucol[k][0] = ucol[k][1] = ucol[k][2] = m; lp[k] = packLight(sk, blv, cc);
                        }
                        auto quad = [&](float y0, float y1, int tm, int tileOv, float push) {   // tm: 0 sem cor, 1 grama, 2 folhagem
                            V v[4]; float lm[4];
                            const bool tb = (f == 2 || f == 3);
                            int tx = tileOv >= 0 ? tileOv % ATL : tx0, ty = tileOv >= 0 ? tileOv / ATL : ty0;
                            for (int k = 0; k < 4; k++) {
                                float yy = tb ? (float)Q[f][k][1] * (f == 2 ? y1 : 1.f) : y0 + (y1 - y0) * Q[f][k][1];
                                float r, g, bb, cr0, cg0, cb0, aa = 1.f;
                                const int kb = k < 2 ? k : 3 - k, kt = k < 2 ? 3 - k : k;
                                if (tb) { cr0 = col[k][0]; cg0 = col[k][1]; cb0 = col[k][2]; }
                                else { cr0 = lerpf(col[kb][0], col[kt][0], yy); cg0 = lerpf(col[kb][1], col[kt][1], yy); cb0 = lerpf(col[kb][2], col[kt][2], yy); }
                                lm[k] = cr0 + cg0 + cb0;   // luminância antes do tingimento (diagonal igual nas duas camadas)
                                if (gShaderOK) { const int ks = tb ? k : (yy > .5f ? kt : kb); r = ucol[ks][0]; g = ucol[ks][1]; bb = ucol[ks][2]; aa = lp[ks]; }   // cor sem luz + luz no alfa (o shader aplica o ciclo)
                                else { r = cr0; g = cg0; bb = cb0; }
                                if (tm == 3) { static const float FIX[2][3] = {{.380f, .600f, .380f}, {.502f, .655f, .333f}}; const float* ff = FIX[lk == 3]; r *= ff[0]; g *= ff[1]; bb *= ff[2]; }   // abeto / betula: cor fixa
                                else if (tm) {
                                    int gi = (z + Q[f][k][2]) * (CW + 1) + (x + Q[f][k][0]);
                                    const float* tt = tm == 1 ? tg[gi] : tl[gi];
                                    r *= tt[0]; g *= tt[1]; bb *= tt[2];
                                }
                                v[k] = {(float)(wx + Q[f][k][0]) + D[f][0] * push, y + yy + D[f][1] * push, (float)(wz + Q[f][k][2]) + D[f][2] * push,
                                        (tx + (rotUV ? (tb ? UV[k][1] : 1.f - yy) : UV[k][0])) / (float)ATL, (ty + (rotUV ? 1.f - UV[k][0] : (tb ? UV[k][1] : 1.f - yy))) / (float)ATLR, r, g, bb, aa};
                            }
                            // escolhe a diagonal pelo par mais escuro (evita artefato de interpolação do AO)
                            float d02 = lm[0] + lm[2], d13 = lm[1] + lm[3];
                            int rot = d02 > d13 ? 1 : 0;
                            for (int i = 0; i < 4; i++) (tk == 1 ? glassQ : c->mesh).push_back(v[(i + rot) & 3]); if (tk == 1) c->hasTrans = true;
                        };
                        
#ifdef BL_RPI
                        if (b == GRASS && f != 2 && f != 3 && (gMeshFar || BL_RPI_FASTGRASS)) quad(0.f, 1.f, 1, blkTile(b, 0), 0.f);   // patchRpiOpt.7: grama de longe = lado verde (1 quad com a textura do topo; sem a terra aparecendo no mip)
                        else
#endif
                        if (b == GRASS && f != 2 && f != 3) { quad(0.f, 1.f, 0, TILE_GRASS_BASE, 0.f); quad(0.f, 1.f, 1, TILE_GRASS_OVER, 0.f); }   // base + camada verde tingida (como o MC)
                        else quad(0.f, pkHeight(b), (b == GRASS && f == 2) ? 1 : (lk == 1 ? 2 : (lk == 2 || lk == 3) ? 3 : 0), -1, leafMix ? .003f : (b == gCactusId && b && f != 2 && f != 3) ? -.0625f : 0.f);   // cacto: lados recuados 1/16 (modelo do pack)
                    }
                    if (b == gEnchId && b && !gMeshFar) emitEnchBook(c, x, y, z, wx, wz);   // livro flutuante da mesa
                    if (b == gEndFrameId && b && gEndEye >= 0 && (c->m[idx(x, y, z)] & 4)) {   // olho do Ender: caixa 8x3x8 em (4,13,4), como end_portal_frame_filled.json
                        int Fe[3] = {bxr, y + 1, bzr}; int skE, blE; bool opE; cellInfo(Fe, skE, blE, opE);
                        const float sf = ltab((float)skE), bf = blE > 0 ? ltabB((float)blE) : 0.f, lp = packLight((float)skE, (float)blE);
                        for (int f = 0; f < 6; f++) {
                            if (f == 3) continue;   // sem face de baixo (f==2 = topo, f==3 = base)
                            const float m = SH[f]; const float cr = gShaderOK ? m : std::max(sf, bf) * m, cg = gShaderOK ? m : std::max(sf, bf * .93f) * m, cb = gShaderOK ? m : std::max(sf, bf * .78f) * m, aa = gShaderOK ? lp : 1.f;
                            for (int k = 0; k < 4; k++) {
                                const float px = (4.f + 8.f * Q[f][k][0]) / 16.f, py = (13.f + 3.f * Q[f][k][1]) / 16.f, pz = (4.f + 8.f * Q[f][k][2]) / 16.f;
                                float u = (4.f + 8.f * UV[k][0]) / 16.f, vv = f == 2 ? (4.f + 8.f * UV[k][1]) / 16.f : (1.f - (float)Q[f][k][1]) * 3.f / 16.f;
                                c->mesh.push_back({(float)wx + px, (float)y + py, (float)wz + pz, ((gEndEye % ATL) + u) / (float)ATL, ((gEndEye / ATL) + vv) / (float)ATLR, cr, cg, cb, aa});
                            }
                        }
                    }
                    continue;
                }

                std::vector<V>& out = c->wmesh;
                const size_t w0 = out.size();
                const int waterTile = TILES[WATER][1];
                const int tx = waterTile % ATL, ty = waterTile / ATL;
                const float u0 = tx / (float)ATL, v0 = ty / (float)ATLR;
                const float u1 = u0 + 1.f / ATL, v1 = v0 + 1.f / ATLR;
                const int above = gb(wx, y + 1, wz), below = gb(wx, y - 1, wz);
                const bool needTop = !isWater(above);
                static const int faceForSide[4] = {0, 1, 4, 5};
                static const int nx[4] = {1, -1, 0, 0};
                static const int nz[4] = {0, 0, 1, -1};
                bool open[4]; bool any = needTop || below == AIR;
                for (int f = 0; f < 4; ++f) { open[f] = gb(wx + nx[f], y, wz + nz[f]) == AIR; any |= open[f]; }
                if (!any) continue;   // agua totalmente cercada: nada para desenhar
                float hc[2][2];
                for (int ox = 0; ox < 2; ox++) for (int oz = 0; oz < 2; oz++) hc[ox][oz] = cornerH(wx + ox, y, wz + oz);
                float wtn[3] = {1, 1, 1}; if (gDim == 0) biomeWaterTint((float)wx + .5f, (float)wz + .5f, wtn);   // cor de bioma da agua (so overworld)

                if (needTop) {
                    out.push_back({(float)wx,     y + hc[0][0], (float)wz,     u0, v0, .82f,.92f,1.f,.62f});
                    out.push_back({(float)wx + 1, y + hc[1][0], (float)wz,     u1, v0, .82f,.92f,1.f,.62f});
                    out.push_back({(float)wx + 1, y + hc[1][1], (float)wz + 1, u1, v1, .82f,.92f,1.f,.62f});
                    out.push_back({(float)wx,     y + hc[0][1], (float)wz + 1, u0, v1, .82f,.92f,1.f,.62f});
                }
                // Lados so contra ar (contra agua a superficie ja e continua pelos cantos compartilhados).
                for (int f = 0; f < 4; ++f) {
                    if (!open[f]) continue;
                    const int qf = faceForSide[f];
                    const float s = SH[qf] * .90f;
                    for (int k = 0; k < 4; ++k) {
                        const int qx = (int)Q[qf][k][0], qz = (int)Q[qf][k][2];
                        const float yy = (Q[qf][k][1] > .5f) ? hc[qx][qz] : 0.f;
                        out.push_back({(float)(wx + qx), y + yy, (float)(wz + qz),
                                       (k == 0 || k == 3) ? u0 : u1, (k < 2) ? v1 : v0, s, s, 1.f, .62f});
                    }
                }
                if (below == AIR) {   // fundo visivel numa queda
                    out.push_back({(float)wx,     (float)y, (float)wz,     u0, v0, .45f,.45f,1.f,.28f});
                    out.push_back({(float)wx + 1, (float)y, (float)wz,     u1, v0, .45f,.45f,1.f,.28f});
                    out.push_back({(float)wx + 1, (float)y, (float)wz + 1, u1, v1, .45f,.45f,1.f,.28f});
                    out.push_back({(float)wx,     (float)y, (float)wz + 1, u0, v1, .45f,.45f,1.f,.28f});
                }
                if (wtn[0] != 1.f || wtn[1] != 1.f || wtn[2] != 1.f) for (size_t q = w0; q < out.size(); ++q) { out[q].r *= wtn[0]; out[q].g *= wtn[1]; out[q].b *= wtn[2]; }
                {   // luz da celula de agua (ceu +2 para a superficie nao ficar escura)
                    const uint8_t Lc = c->lc[idx(x, y, z)]; const int sk = Lc >> 4, bk = Lc & 15;
                    if (gShaderOK) { const float pk = packLight((float)sk, (float)bk); for (size_t q = w0; q < out.size(); ++q) out[q].a = pk; }   // o shader soma +2 ao céu da água e aplica o ciclo
                    else {
                        const float lw = std::max(ltab((float)std::min(15, sk + 2)), bk ? ltabB((float)bk) : 0.f);
                        for (size_t q = w0; q < out.size(); ++q) { out[q].r *= lw; out[q].g *= lw; out[q].b *= lw; }
                    }
                }
            }
    }
    for (int sc = (topY >> 4) + 1; sc <= 16; sc++) { if (!waterOnly) c->sOff[sc] = (uint32_t)c->mesh.size(); c->wOff[sc] = (uint32_t)c->wmesh.size(); }
    c->secOk = true;
    gXbGlass = nullptr; c->ngv = (int)glassQ.size(); if (c->ngv) c->hasTrans = true; if (c->ngv) c->wmesh.insert(c->wmesh.end(), glassQ.begin(), glassQ.end());   // wOff[] so cobre a agua (fim da parte de agua)
    c->nwv = (int)c->wmesh.size(); c->glw = true;
    if (!waterOnly) { c->nv = (int)c->mesh.size(); c->gl = true; }
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif

// ---- streaming de chunks ----
// Ordem de trabalho: chunks dentro do circulo de raio R+2, do mais "urgente" ao menos (perto primeiro; os que estao NA FRENTE
// do jogador ganham ate ~2 chunks de vantagem). Isso e so PRIORIDADE de carga: nada deixa de ser desenhado por estar fora da visao.
struct StreamOff { int dx, dz; int d2; bool mesh, farOff; };
static std::vector<StreamOff> gOrder; static int gOrdR = -1, gOrdFar = -1; static float gOrdFx = 0.f, gOrdFz = 0.f;
static int gStreamPcx = 0x7fffffff, gStreamPcz = 0, gStreamR = -1, gStreamFar = -1;
static void streamBuildOrder(int R, int farR, float fx, float fz) {
    gOrder.clear(); const int G = R + 2;
    struct T { StreamOff o; float k; }; std::vector<T> t; t.reserve((2 * G + 1) * (2 * G + 1));
    for (int dz = -G; dz <= G; dz++) for (int dx = -G; dx <= G; dx++) {
        const int d2 = dx * dx + dz * dz; if (d2 > G * G) continue;
        const float d = sqrtf((float)d2); float k = d;
        if (d > 1.5f) k -= 2.0f * ((dx * fx + dz * fz) / d);   // frente do jogador primeiro
        const float rm = R + .5f; t.push_back({{dx, dz, d2, (float)d2 <= rm * rm, farR < (1 << 20) && d2 >= farR * farR}, k});
    }
    std::sort(t.begin(), t.end(), [](const T& a, const T& b) { return a.k < b.k; });
    for (auto& e : t) gOrder.push_back(e.o);
    gOrdR = R; gOrdFar = farR; gOrdFx = fx; gOrdFz = fz;
}
// carrega/gera chunks ao redor do jogador com orcamento de tempo. Se nada mudou desde a ultima varredura (jogador no mesmo chunk,
// nenhum chunk novo/sujo) retorna na hora: antes cada quadro varria todos os (2R+3)^2 chunks (4225 no raio 32) com varias buscas no hash.
// farR: a partir dessa distancia (em chunks) a malha e feita em modo "longe" (sem plantas); use um valor enorme para desligar.
static void stream(int pcx, int pcz, int maxGen, int maxMesh, double budgetMs = -1.0, float fx = 0.f, float fz = -1.f, int farR = 1 << 20) {
    const int R = RAD; if (gDim) farR = 1 << 20;   // Nether: o modo 'longe' corta faces abaixo do topo solido da coluna (o teto de bedrock esconderia tudo)
    if (!gKick && pcx == gStreamPcx && pcz == gStreamPcz && R == gStreamR && farR == gStreamFar) return;   // nada a fazer
    const double t0 = perfNow();
    if (R != gOrdR || farR != gOrdFar || fx * gOrdFx + fz * gOrdFz < .8f) streamBuildOrder(R, farR, fx, fz);
    bool pending = false; int done = 0; const int G2 = (R + 2) * (R + 2);
    auto outOfTime = [&]() { return budgetMs >= 0 && done > 0 && perfNow() - t0 > budgetMs; };   // sempre faz ao menos 1 tarefa por chamada
    for (const StreamOff& o : gOrder) {
        const int cx = pcx + o.dx, cz = pcz + o.dz;
        Chunk* c = getC(cx, cz);
        if (!c) {
            if (o.d2 > G2) continue;
            if (maxGen <= 0 || outOfTime()) { pending = true; continue; }
            if (netChunkWait(cx, cz)) { pending = true; continue; }
            splashPoll(); gen(cx, cz); maxGen--; done++; pending = true;   // chunk novo: outra varredura vai achar a malha dele
            continue;
        }
        if (!o.mesh) continue;
        // troca entre malha "longe" (sem plantas) e "perto" com histerese de 1 chunk (evita refazer a malha toda hora na fronteira)
        if (c->lc && !c->dirty && farR < (1 << 20)) {
            const int f = farR, up = (f + 1) * (f + 1), dn = (f - 1) * (f - 1);
            if ((!c->farM && o.d2 >= up) || (c->farM && o.d2 <= dn)) { c->dirty = true; gKick = true; }
        }
        if (c->dirty || (c->wdirty && c->lc)) {
            if (maxMesh <= 0 || outOfTime()) { pending = true; continue; }
            bool ready = true;   // a luz precisa dos 8 vizinhos carregados
            for (int oz = -1; oz <= 1 && ready; oz++) for (int ox = -1; ox <= 1; ox++) if (!getC(cx + ox, cz + oz)) { ready = false; break; }
            if (!ready) { pending = true; continue; }
            splashPoll();
            { const int f = farR; const bool wasFar = c->lc && c->farM; gMeshFar = f < (1 << 20) && (wasFar ? o.d2 > (f - 1) * (f - 1) : o.d2 >= f * f); }   // histerese: mantem o estado dentro da faixa
            build(cx, cz, c, !c->dirty && !c->hasTrans); gMeshFar = false; maxMesh--; done++;
        }
    }
    gPf[PS_STREAM] += perfNow() - t0;
    if (!pending) { gKick = false; gStreamPcx = pcx; gStreamPcz = pcz; gStreamR = R; gStreamFar = farR; }
}

// ---------- física / interação ----------
static bool hit(float x, float y, float z, float height = 1.8f) {
    for (int iy = (int)floorf(y); iy <= (int)floorf(y + height); iy++)
        for (int iz = (int)floorf(z - .3f); iz <= (int)floorf(z + .3f); iz++)
            for (int ix = (int)floorf(x - .3f); ix <= (int)floorf(x + .3f); ix++)
                if (solid(getB(ix, iy, iz))) return true;
    return false;
}
// Raycast contínuo de voxel para a câmera. Ao contrário de amostrar a linha
// em passos fixos, o DDA cruza cada célula exatamente uma vez. Isso evita a
// alternância de distância quando a linha passa exatamente por uma quina.
static float raySolidDistance(const float* o, const float* d, float maxDist) {
    if (maxDist <= 0.f) return -1.f;
    float dx = d[0], dy = d[1], dz = d[2];
    float len = sqrtf(dx*dx + dy*dy + dz*dz);
    if (len < 0.000001f) return -1.f;
    dx /= len; dy /= len; dz /= len;

    int x = (int)floorf(o[0]), y = (int)floorf(o[1]), z = (int)floorf(o[2]);
    if (solid(getB(x,y,z))) return 0.f;

    const int sx = dx > 0.f ? 1 : (dx < 0.f ? -1 : 0);
    const int sy = dy > 0.f ? 1 : (dy < 0.f ? -1 : 0);
    const int sz = dz > 0.f ? 1 : (dz < 0.f ? -1 : 0);
    const float inf = 1e30f;
    const float txDelta = sx ? fabsf(1.f / dx) : inf;
    const float tyDelta = sy ? fabsf(1.f / dy) : inf;
    const float tzDelta = sz ? fabsf(1.f / dz) : inf;
    const float nextX = sx > 0 ? (floorf(o[0]) + 1.f) : floorf(o[0]);
    const float nextY = sy > 0 ? (floorf(o[1]) + 1.f) : floorf(o[1]);
    const float nextZ = sz > 0 ? (floorf(o[2]) + 1.f) : floorf(o[2]);
    float tx = sx ? fabsf((nextX - o[0]) / dx) : inf;
    float ty = sy ? fabsf((nextY - o[1]) / dy) : inf;
    float tz = sz ? fabsf((nextZ - o[2]) / dz) : inf;

    while (true) {
        float t;
        if (tx <= ty && tx <= tz) { x += sx; t = tx; tx += txDelta; }
        else if (ty <= tz)       { y += sy; t = ty; ty += tyDelta; }
        else                     { z += sz; t = tz; tz += tzDelta; }
        if (t > maxDist) break;
        if (solid(getB(x,y,z))) return t;
    }
    return -1.f;
}

// Mira do jogador: acerta a caixa de SELECAO real do bloco (blockshape.inc). h = bloco acertado; p = celula de colocacao (h + normal da face acertada).
// O resultado completo (face, ponto de impacto, caixa) fica em gRay, usado pelo contorno e pela colocacao orientada.
static RayHit gRay;
static bool ray(const float* o, const float* d, int* h, int* p) {
    if (!rayCast(o, d, 5.f, gRay)) return false;
    h[0] = gRay.x; h[1] = gRay.y; h[2] = gRay.z;
    p[0] = gRay.x + gRay.nx; p[1] = gRay.y + gRay.ny; p[2] = gRay.z + gRay.nz;
    return true;
}

// ---------- água: port do BlockDynamicLiquid (Minecraft 1.8) ----------
// Guiada por eventos: so celulas acordadas por setB (quebrar/colocar bloco, fluxo) sao processadas.
// Isso faz a agua encher buracos abertos debaixo d'agua e cair/espalhar como no Minecraft.
static const int WHX[4] = {1, -1, 0, 0}, WHZ[4] = {0, 0, 1, -1};
static bool gFlowLava = false;   // tipo de liquido do tick atual
static inline bool fIs(int b) { return gFlowLava ? isLava(b) : isWater(b); }
static inline int fMeta(int b) { return gFlowLava ? lavaMeta(b) : liqMeta(b); }
static inline int fBlock(int m) { return gFlowLava ? lavaBlock(m) : metaBlock(m); }
static int flowCost(int x, int y, int z, int depth, int from) {
    int best = 1000;
    for (int d = 0; d < 4; d++) {
        if (d == from) continue;
        const int nx = x + WHX[d], nz = z + WHZ[d], nb = getB(nx, y, nz);
        if (!solid(nb) && (!fIs(nb) || fMeta(nb) > 0)) {
            if (!solid(getB(nx, y - 1, nz))) return depth;
            if (depth < (gFlowLava ? 2 : 4)) { int j = flowCost(nx, y, nz, depth + 1, d ^ 1); if (j < best) best = j; }   // getSlopeFindDistance: lava 2, agua 4
        }
    }
    return best;
}
// BlockLiquid.checkForMixing: lava com agua em cima ou nos 4 lados (embaixo NAO conta) vira obsidiana (fonte) ou pedregulho (fluxo nivel 1..4).
// Nivel > 4 (e lava caindo) continua lava. Roda na hora em que o bloco entra ou um vizinho muda (onBlockAdded / neighborChanged).
static bool lavaMixCell(int x, int y, int z) {
    const int b = getB(x, y, z);
    if (!isLava(b)) return false;
    static const int N[5][3] = {{0,1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
    bool w = false; for (auto& n : N) if (isWater(getB(x + n[0], y + n[1], z + n[2]))) { w = true; break; }
    if (!w) return false;
    const int m = lavaMeta(b);
    if (m == 0) { setB(x, y, z, OBSIDIAN); return true; }
    if (m > 0 && m <= 4) { setB(x, y, z, COBBLE); return true; }
    return false;
}
static void lavaMixAround(int x, int y, int z) {
    static const int D6[7][3] = {{0,0,0},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (auto& d : D6) lavaMixCell(x + d[0], y + d[1], z + d[2]);
}
static bool lavaHarden(int x, int y, int z) { return lavaMixCell(x, y, z); }
static void blockDrops(int bx, int by, int bz, int b);
// BlockDynamicLiquid.canFlowInto: nao entra em bloco bloqueante, no mesmo liquido nem em lava; a lava pode entrar na agua. Plantas/tochas (sem colisao) sao lavadas.
static bool liqCanFlowInto(int nb) {
    if (nb == AIR) return true;
    if (solid(nb)) return false;
    if (isLiquid(nb)) return gFlowLava && isWater(nb);
    return true;
}
// tryFlowInto: agua leva o bloco (com drop); a lava so queima. Depois o setB ja dispara a mistura com agua vizinha (lava nivel <= 4 vira pedregulho).
static void liqFlowInto(int x, int y, int z, int level) {
    const int nb = getB(x, y, z);
    if (!liqCanFlowInto(nb)) return;
    if (nb != AIR && !gFlowLava && !isLiquid(nb) && !netOn) blockDrops(x, y, z, nb);
    setB(x, y, z, fBlock(level));
}
static void liquidTick(int x, int y, int z) {   // BlockDynamicLiquid: agua (decai 1/bloco) e lava (decai 2/bloco, sem fonte infinita)
    const int b = getB(x, y, z);
    gFlowLava = isLava(b);
    if (!gFlowLava && !isWater(b)) return;
    if (gFlowLava && lavaHarden(x, y, z)) return;
    const int decay = gFlowLava ? 2 : 1, fallId = gFlowLava ? LAVA_FALL : WATER_FALL;
    int i = fMeta(b);
    if (i > 0) {   // fluxo: recalcula a partir dos vizinhos
        int i1 = -100, src = 0;
        for (int d = 0; d < 4; d++) {
            int m = fMeta(getB(x + WHX[d], y, z + WHZ[d]));
            if (m < 0) continue;
            if (m == 0) src++;
            if (m >= 8) m = 0;
            i1 = (i1 >= 0 && m >= i1) ? i1 : m;
        }
        int j1 = i1 + decay;
        if (j1 >= 8 || i1 < 0) j1 = -1;
        const int ma = fMeta(getB(x, y + 1, z));
        if (ma >= 0) j1 = ma >= 8 ? ma : ma + 8;
        if (!gFlowLava && src >= 2) { const int bb = getB(x, y - 1, z); if (solid(bb) || bb == WATER) j1 = 0; }   // fonte infinita (so agua)
        if (j1 != i) {
            if (j1 < 0) { setB(x, y, z, AIR); return; }
            i = j1; setB(x, y, z, fBlock(j1));
        }
    }
    if (getB(x, y, z) != b && !fIs(getB(x, y, z))) return;   // endureceu/sumiu durante o proprio tick
    const int below = getB(x, y - 1, z);
    if (liqCanFlowInto(below)) {   // cai
        if (gFlowLava && isWater(below)) { setB(x, y - 1, z, STONE); return; }   // lava caindo na agua: a agua vira pedra
        liqFlowInto(x, y - 1, z, 8);
    } else if (i >= 0 && (i == 0 || solid(below))) {   // espalha para os lados (prefere o caminho com queda mais proxima)
        int j1 = i + decay; if (i >= 8) j1 = 1;
        if (j1 >= 8) return;
        int best = 1000, mask = 0;
        for (int d = 0; d < 4; d++) {
            const int nx = x + WHX[d], nz = z + WHZ[d], nb = getB(nx, y, nz);
            if (solid(nb) || (fIs(nb) && fMeta(nb) == 0)) continue;
            const int j = solid(getB(nx, y - 1, nz)) ? flowCost(nx, y, nz, 1, d ^ 1) : 0;
            if (j < best) mask = 0;
            if (j <= best) { mask |= 1 << d; best = j; }
        }
        for (int d = 0; d < 4; d++) if (mask & (1 << d)) liqFlowInto(x + WHX[d], y, z + WHZ[d], j1);
    }
}
static void decodeWKey(int64_t k, int& x, int& y, int& z) {
    x = (int)((k >> 38) & 0x3FFFFFF); if (x & 0x2000000) x -= 0x4000000;
    z = (int)((k >> 12) & 0x3FFFFFF); if (z & 0x2000000) z -= 0x4000000;
    y = (int)(k & 0xFFF);
}
static float waterTimer = 0.f, lavaTimer = 0.f;
static void simulateWater(float dt, int, int) {
    waterTimer += dt; lavaTimer += dt;
    if (wqCur.empty() && !wqNext.empty() && waterTimer >= .25f) { wqCur.swap(wqNext); waterTimer = 0.f; }   // tickrate da agua: 5 ticks = 0,25 s
    int budget = 800;   // limite por quadro (evita travadas em inundacoes grandes)
    while (!wqCur.empty() && budget-- > 0) {
        const int64_t k = wqCur.back(); wqCur.pop_back(); wqSet.erase(k);
        int x, y, z; decodeWKey(k, x, y, z); liquidTick(x, y, z);
    }
    if (lqCur.empty() && !lqNext.empty() && lavaTimer >= 1.5f) { lqCur.swap(lqNext); lavaTimer = 0.f; }   // lava: 30 ticks = 1,5 s
    budget = 200;
    while (!lqCur.empty() && budget-- > 0) {
        const int64_t k = lqCur.back(); lqCur.pop_back(); lqSet.erase(k);
        int x, y, z; decodeWKey(k, x, y, z); liquidTick(x, y, z);
    }
}

// ---------- decaimento de folhas (como o Minecraft: sem tronco a até 4 blocos, somem depois de um tempo) ----------
static std::unordered_set<int64_t> keepLeaves, leafPending;   // keepLeaves = folhas colocadas pelo jogador (persistentes)
static std::vector<int64_t> leafQueue;
static std::unordered_map<int64_t, float> leafDecay;
static inline int64_t k3(int x, int y, int z) { return (((int64_t)(x & 0x3FFFFFF)) << 38) | (((int64_t)(z & 0x3FFFFFF)) << 12) | (int64_t)(y & 0xFFF); }
static void unk3(int64_t k, int& x, int& y, int& z) {
    x = (int)((k >> 38) & 0x3FFFFFF); if (x & 0x2000000) x -= 0x4000000;
    z = (int)((k >> 12) & 0x3FFFFFF); if (z & 0x2000000) z -= 0x4000000;
    y = (int)(k & 0xFFF);
}
static void leafNeighbors(int x, int y, int z) {   // enfileira folhas vizinhas para checagem
    static const int D6[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (auto& d : D6) {
        int nx = x + d[0], ny = y + d[1], nz = z + d[2];
        if (!isLeafB(getB(nx, ny, nz))) continue;
        int64_t k = k3(nx, ny, nz);
        if (!keepLeaves.count(k) && leafPending.insert(k).second) leafQueue.push_back(k);
    }
}
static bool leafSupported(int x, int y, int z) {   // há tronco a no máximo 4 passos, andando só por folhas?
    struct N { int x, y, z, d; };
    static std::vector<N> st; static std::unordered_set<int64_t> seen;
    static const int D6[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    st.clear(); seen.clear(); st.push_back({x, y, z, 0}); seen.insert(k3(x, y, z));
    for (size_t i = 0; i < st.size(); i++) {
        N n = st[i];
        for (auto& d : D6) {
            int nx = n.x + d[0], ny = n.y + d[1], nz = n.z + d[2], nb = getB(nx, ny, nz);
            if (isLogB(nb)) return true;
            if (isLeafB(nb) && n.d + 1 < 4 && seen.insert(k3(nx, ny, nz)).second) st.push_back({nx, ny, nz, n.d + 1});
        }
    }
    return false;
}
static void blockDrops(int bx, int by, int bz, int b);
static void tickLeaves(float dt) {
    for (int budget = 48; budget > 0 && !leafQueue.empty(); budget--) {
        int64_t k = leafQueue.back(); leafQueue.pop_back(); leafPending.erase(k);
        int x, y, z; unk3(k, x, y, z);
        if (!isLeafB(getB(x, y, z)) || keepLeaves.count(k)) continue;
        if (!leafSupported(x, y, z) && !leafDecay.count(k)) leafDecay[k] = .4f + 4.f * hash2(x + y * 7, z, SDL_GetTicks());
    }
    for (auto it = leafDecay.begin(); it != leafDecay.end();) {
        it->second -= dt;
        if (it->second > 0.f) { ++it; continue; }
        int64_t k = it->first; it = leafDecay.erase(it);
        int x, y, z; unk3(k, x, y, z);
        if (isLeafB(getB(x, y, z)) && !keepLeaves.count(k) && !leafSupported(x, y, z)) { const int lb = getB(x, y, z); if (!netOn) blockDrops(x, y, z, lb); setB(x, y, z, AIR); leafNeighbors(x, y, z); }
    }
}
