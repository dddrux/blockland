// Etapa 1.3: gancho de vizinhos, ticks agendados, colocacao orientada e tronco com eixo. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define NEAR(a, b) (fabs((double)(a) - (double)(b)) < 1e-4)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true;
    for (int z = -8; z < 24; z++) for (int x = -10; x < 24; x++) for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    gNoBlockHooks = false;
}
static void clearArena() { gNoBlockHooks = true; for (int z = -8; z < 24; z++) for (int x = -10; x < 24; x++) for (int y = AY; y < AY + 16; y++) setB(x, y, z, AIR); gNoBlockHooks = false; DROPS.clear(); resetBlockUpdates(); }

// ---- ganchos de teste ----
struct Rec { int x, y, z, fx, fy, fz; };
static std::vector<Rec> nbCalls; static std::vector<std::array<int, 3>> addCalls, tickCalls;
static int depth = 0, maxDepth = 0;
static void hNeighbor(int x, int y, int z, int fx, int fy, int fz) { nbCalls.push_back({x, y, z, fx, fy, fz}); }
static void hAdded(int x, int y, int z) { addCalls.push_back({x, y, z}); }
static void hDomino(int x, int y, int z, int fx, int fy, int fz) { ++depth; maxDepth = std::max(maxDepth, depth); if (fx == x - 1 && getB(fx, fy, fz) != 711) setB(x, y, z, AIR); --depth; }
static void hFlip(int x, int y, int z, int, int, int) { setB(x, y, z, getB(x, y, z) == 712 ? 713 : 712); }
static void hTick(int x, int y, int z) { tickCalls.push_back({x, y, z}); }
static void hTickRepeat(int x, int y, int z) { tickCalls.push_back({x, y, z}); scheduleTick(x, y, z, 2); }
static bool placeAt(int id, int hx, int hy, int hz, int face, double px = 100, double py = 100, double pz = 100, int* out = nullptr) {
    PlaceCtx c{hx, hy, hz, face, .5, .5, .5, 0.f, 0.f, false, px, py, pz}; return placeBlock(id, c, out);
}
// vertices do bloco (wx,wy,wz) no mesh: agrupa em quads (4 em 4) e devolve por face
struct Quad { V v[4]; int f; int tile; };
static std::vector<Quad> quadsOf(Chunk* c, int bx, int by, int bz) {
    std::vector<Quad> out;
    for (size_t i = 0; i + 3 < c->mesh.size(); i += 4) {
        const V* q = &c->mesh[i]; bool in = true; for (int k = 0; k < 4; k++) if (q[k].x < bx - .01f || q[k].x > bx + 1.01f || q[k].y < by - .01f || q[k].y > by + 1.01f || q[k].z < bz - .01f || q[k].z > bz + 1.01f) in = false;
        if (!in) continue; Quad Q; memcpy(Q.v, q, sizeof Q.v);
        const float C[3][2] = {{q[0].x, bx + .5f}, {q[0].y, by + .5f}, {q[0].z, bz + .5f}};
        Q.f = -1; for (int a = 0; a < 3; a++) { bool same = true; for (int k = 1; k < 4; k++) { const float cc = a == 0 ? q[k].x : a == 1 ? q[k].y : q[k].z; if (fabsf(cc - C[a][0]) > 1e-4f) same = false; } if (same) Q.f = a * 2 + (C[a][0] > C[a][1] ? 0 : 1); }
        float cu = 0, cv = 0; for (int k = 0; k < 4; k++) { cu += q[k].u; cv += q[k].v; } cu /= 4; cv /= 4;
        Q.tile = (int)floorf(cv * ATLR) * ATL + (int)floorf(cu * ATL); out.push_back(Q);
    }
    return out;
}
// "v da textura acompanha a dimensao dim": vertices com a mesma coordenada em dim tem o mesmo v, e coordenadas diferentes dao v diferente
static bool vTracks(const Quad& q, int dim) {
    for (int a = 0; a < 4; a++) for (int b = a + 1; b < 4; b++) {
        const float ca = dim == 0 ? q.v[a].x : dim == 1 ? q.v[a].y : q.v[a].z, cb = dim == 0 ? q.v[b].x : dim == 1 ? q.v[b].y : q.v[b].z;
        const bool same = fabsf(ca - cb) < 1e-4f, vs = fabsf(q.v[a].v - q.v[b].v) < 1e-6f;
        if (same != vs) return false;
    }
    return true;
}

int main() {
    fs::remove_all("/tmp/blt3"); fs::create_directories("/tmp/blt3"); if (chdir("/tmp/blt3") != 0) return 99;
    WORLD_SEED = 7; arena(); creative = false; netOn = false;

    SECTION("U1 gancho de vizinhos: quem e avisado, quando, e sem recursao");
    { BlockHooks h; h.neighbor = hNeighbor; h.added = hAdded; blockHooksSet(710, h);
      setB(9, AY, 8, 710); nbCalls.clear(); addCalls.clear();
      setB(8, AY, 8, STONE);                                     // vizinho (+X) do 710 mudou
      CHECK(nbCalls.size() == 1 && nbCalls[0].x == 9 && nbCalls[0].y == AY && nbCalls[0].fx == 8 && nbCalls[0].fy == AY);
      CHECK(addCalls.empty());                                   // 'added' so para o bloco que foi colocado
      nbCalls.clear(); setM(8, AY, 8, 5); CHECK(nbCalls.size() == 1);   // mudar so o meta do vizinho tambem avisa
      setBM(8, AY, 8, STONE, 5); nbCalls.clear(); setBM(8, AY, 8, STONE, 5); CHECK(nbCalls.empty());   // regravar o MESMO estado => ninguem e avisado
      nbCalls.clear(); setB(7, AY, 8, STONE); CHECK(nbCalls.empty());                              // 2 blocos de distancia: nao
      nbCalls.clear(); setB(9, AY + 1, 8, STONE); CHECK(nbCalls.size() == 1 && nbCalls[0].fy == AY + 1);   // vizinho de cima
      nbCalls.clear(); addCalls.clear(); setB(9, AY, 8, STONE); setB(9, AY, 8, 710);
      CHECK(addCalls.size() == 1 && addCalls[0][0] == 9);        // colocar o 710 dispara 'added'
      setB(8, AY, 8, AIR); setB(7, AY, 8, AIR); setB(9, AY, 8, AIR); setB(9, AY + 1, 8, AIR); }
    { gNoBlockHooks = true; nbCalls.clear(); setB(9, AY, 8, 710); setB(8, AY, 8, STONE); gNoBlockHooks = false; CHECK(nbCalls.empty());   // edicao da rede: sem ganchos
      setB(8, AY, 8, AIR); setB(9, AY, 8, AIR); nbCalls.clear(); }
    { BlockHooks h; h.neighbor = hDomino; blockHooksSet(711, h);
      gNoBlockHooks = true; setB(-9, AY, 8, STONE); for (int x = -8; x < 24; x++) setB(x, AY, 8, 711); gNoBlockHooks = false;
      maxDepth = 0; setB(-9, AY, 8, AIR);                        // derruba uma fileira de 32 em cadeia
      int left = 0; for (int x = -8; x < 24; x++) left += getB(x, AY, 8) == 711;
      CHECK(left == 0); CHECK(maxDepth == 1);                    // cascata completa e SEM recursao (nesting 1)
    }
    { BlockHooks h; h.neighbor = hFlip; blockHooksSet(712, h); blockHooksSet(713, h);
      gNoBlockHooks = true; setB(8, AY + 5, 8, 712); setB(9, AY + 5, 8, 713); gNoBlockHooks = false; resetBlockUpdates();
      const long long d0 = gUpdDropped; setB(8, AY + 6, 8, STONE);   // dois blocos que se alternam para sempre
      CHECK(gUpdDropped == d0 + 1);                              // o limite cortou o laço (nao travou)
      CHECK(gUpdQ.empty() && !gUpdBusy);                         // fila limpa e drenagem destravada
      setB(8, AY + 5, 8, AIR); setB(9, AY + 5, 8, AIR); setB(8, AY + 6, 8, AIR); }
    clearArena();

    SECTION("U2 tocha/alavanca/planta caem sozinhas ao perder o suporte (qualquer causa)");
    { gXbDrop[XB_TORCH - XB0] = IT_STICK; gXbDrop[XB_LEVER - XB0] = IT_STICK;
      // tocha em pe
      setB(8, AY, 8, STONE); setB(8, AY + 1, 8, XB_TORCH); DROPS.clear();
      setB(9, AY, 8, STONE); CHECK(getB(8, AY + 1, 8) == XB_TORCH);        // mudanca irrelevante ao lado: fica
      setB(8, AY, 8, AIR); CHECK(getB(8, AY + 1, 8) == AIR); CHECK(DROPS.size() == 1);   // sobrevivencia: dropa
      setB(9, AY, 8, AIR);
      creative = true; setB(8, AY, 8, STONE); setB(8, AY + 1, 8, XB_TORCH); DROPS.clear(); setB(8, AY, 8, AIR);
      CHECK(getB(8, AY + 1, 8) == AIR && DROPS.empty()); creative = false;   // criativo: cai sem dropar
      // tochas de parede: suporte em -X,-Z,+X,+Z
      const int DX[4] = {-1, 0, 1, 0}, DZ[4] = {0, -1, 0, 1};
      for (int k = 0; k < 4; k++) { const int tx = 8, tz = 8, sx = tx + DX[k], sz = tz + DZ[k];
          setB(sx, AY + 3, sz, STONE); setB(tx, AY + 3, tz, XB_TORCH + 1 + k); CHECK(getB(tx, AY + 3, tz) == XB_TORCH + 1 + k);
          setB(sx, AY + 3, sz, AIR); CHECK(getB(tx, AY + 3, tz) == AIR); }
      // alavanca: 12 estados x ligada/desligada
      const int LX[4] = {0, -1, 0, 1}, LZ[4] = {1, 0, -1, 0}; int bad = 0;
      for (int on = 0; on < 2; on++) for (int st = 0; st < 12; st++) { const int face = st >> 2, f = st & 3;
          const int sx = face == 0 ? 8 + LX[f] : 8, sy = AY + 6 + (face == 1 ? -1 : face == 2 ? 1 : 0), sz = face == 0 ? 8 + LZ[f] : 8;
          setB(sx, sy, sz, STONE); setB(8, AY + 6, 8, XB_LEVER + on * 12 + st); if (getB(8, AY + 6, 8) != XB_LEVER + on * 12 + st) bad++;
          setB(sx, sy, sz, AIR); if (getB(8, AY + 6, 8) != AIR) bad++; }
      CHECK(bad == 0);
      // planta e planta alta (cascata: o chao some -> metade de baixo -> metade de cima)
      setB(8, AY, 8, STONE); setB(8, AY + 1, 8, XB_PLANT + 2); setB(8, AY, 8, AIR); CHECK(getB(8, AY + 1, 8) == AIR);
      setB(8, AY, 8, STONE); setB(8, AY + 1, 8, XB_PLANT + 14); setB(8, AY + 2, 8, XB_PLANT + 15);
      setB(8, AY + 1, 8, AIR); CHECK(getB(8, AY + 2, 8) == AIR);            // tirar a de baixo leva a de cima
      setB(8, AY + 1, 8, XB_PLANT + 14); setB(8, AY + 2, 8, XB_PLANT + 15); setB(8, AY, 8, AIR);
      CHECK(getB(8, AY + 1, 8) == AIR && getB(8, AY + 2, 8) == AIR);        // tirar o chao leva as duas
      // bigorna nao depende de suporte (xbSolid)
      setB(8, AY + 4, 8, XB_ANVIL); setB(8, AY + 3, 8, STONE); setB(8, AY + 3, 8, AIR); CHECK(getB(8, AY + 4, 8) == XB_ANVIL); setB(8, AY + 4, 8, AIR);
      // edicao vinda da rede NAO cascateia (quem editou ja transmitiu cada remocao)
      setB(8, AY, 8, STONE); setB(8, AY + 1, 8, XB_TORCH); DROPS.clear();
      netApplyEdit(8, AY, 8, AIR, 0); CHECK(getB(8, AY, 8) == AIR && getB(8, AY + 1, 8) == XB_TORCH && DROPS.empty());
      CHECK(!gNoBlockHooks);                                                // a trava volta ao normal
      setB(8, AY + 1, 8, AIR); }
    clearArena();

    SECTION("U3 ticks agendados");
    { BlockHooks h; h.tick = hTick; blockHooksSet(714, h); BlockHooks hr; hr.tick = hTickRepeat; blockHooksSet(715, hr);
      setB(8, AY + 10, 8, 714); tickCalls.clear(); scheduleTick(8, AY + 10, 8, 3); CHECK(tickPending(8, AY + 10, 8));
      runWorldTick(); runWorldTick(); CHECK(tickCalls.empty()); runWorldTick(); CHECK(tickCalls.size() == 1 && !tickPending(8, AY + 10, 8));
      runWorldTick(); runWorldTick(); CHECK(tickCalls.size() == 1);          // nao repete sozinho
      tickCalls.clear(); scheduleTick(8, AY + 10, 8, 2); scheduleTick(8, AY + 10, 8, 5);   // duplicado na mesma celula conta uma vez
      for (int i = 0; i < 6; i++) runWorldTick(); CHECK(tickCalls.size() == 1);
      tickCalls.clear(); scheduleTick(8, AY + 10, 8, 0); runWorldTick(); CHECK(tickCalls.size() == 1);   // delay 0 vira 1
      tickCalls.clear(); scheduleTick(8, AY + 10, 8, 2); setB(8, AY + 10, 8, STONE); runWorldTick(); runWorldTick(); CHECK(tickCalls.empty());   // trocado antes: nao reage
      setB(9, AY + 10, 8, 715); tickCalls.clear(); scheduleTick(9, AY + 10, 8, 1); for (int i = 0; i < 7; i++) runWorldTick(); CHECK(tickCalls.size() == 4);   // auto-reagendado: t=1,3,5,7
      setB(9, AY + 10, 8, AIR); for (int i = 0; i < 4; i++) runWorldTick();
      setB(8, AY + 10, 8, 714); tickCalls.clear(); scheduleTick(8, AY + 10, 8, 1);
      netOn = true; netHost = false; const long long t0 = gWorldTick; runWorldTick(); CHECK(tickCalls.empty() && gWorldTick == t0);   // convidado nao roda ticks
      netHost = true; runWorldTick(); CHECK(tickCalls.size() == 1);                                                                       // host roda
      netOn = false; netHost = false; setB(8, AY + 10, 8, AIR); }
    clearArena();

    SECTION("U4 colocacao: alvo, substituiveis, sobreposicao pela forma real, eixo");
    { int o[3]; netEdits.clear();
      CHECK(placeAt(STONE, 8, AY - 1, 8, 3, 100, 100, 100, o) && o[0] == 8 && o[1] == AY && o[2] == 8 && getB(8, AY, 8) == STONE && getM(8, AY, 8) == 0);
      CHECK(netEdits.count(k3(8, AY, 8)) && netEditId(netEdits[k3(8, AY, 8)]) == STONE);
      CHECK(placeAt(STONE, 8, AY, 8, 1) && getB(9, AY, 8) == STONE);          // lado +X
      CHECK(placeAt(STONE, 8, AY, 8, 0) && getB(7, AY, 8) == STONE);          // lado -X
      CHECK(placeAt(STONE, 8, AY, 8, 5) && getB(8, AY, 9) == STONE && placeAt(STONE, 8, AY, 8, 4) && getB(8, AY, 7) == STONE);
      // alvo ocupado por tocha: recusa e nao destroi a tocha
      setB(10, AY, 8, XB_TORCH); CHECK(!placeAt(STONE, 9, AY, 8, 1) && getB(10, AY, 8) == XB_TORCH);
      // mato mirado e substituido (a celula do proprio mato, nao a vizinha)
      setB(12, AY, 8, XB_PLANT); CHECK(placeAt(STONE, 12, AY, 8, 4) && getB(12, AY, 8) == STONE && getB(12, AY, 7) == AIR);
      setB(12, AY, 12, XB_PLANT + 14); setB(12, AY + 1, 12, XB_PLANT + 15); CHECK(placeAt(STONE, 12, AY, 12, 3) && getB(12, AY, 12) == STONE && getB(12, AY + 1, 12) == AIR);   // a metade de cima some
      setB(12, AY, 12, AIR);
      // agua e substituivel
      setB(14, AY, 8, WATER); CHECK(placeAt(STONE, 14, AY - 1, 8, 3) && getB(14, AY, 8) == STONE);
      // limites
      CHECK(!placeAt(STONE, 8, CH - 1, 8, 3));                                  // y = CH
      CHECK(!placeAt(STONE, 100000, AY, 100000, 3));                            // chunk nao carregado
      CHECK(!placeAt(0, 8, AY - 1, 8, 3));                                      // id invalido
      // sobreposicao: cubo no corpo do jogador e recusado; laje de meia altura que passa por cima da cabeca nao
      const int ub = shapeAdd({{0, .5, 0, 1, 1, 1}}); blockShapeSet(702, ub, ub);
      CHECK(!placeAt(STONE, 8, AY, 8, 3, 8.5, AY + 1.0, 8.5));                  // cubo em (8,AY+1,8) com o jogador la dentro
      CHECK(placeAt(702, 8, AY, 8, 3, 8.5, AY - .45, 8.5) && getB(8, AY + 1, 8) == 702);   // cabeca em AY+1,35 < 1,5: a laje alta cabe
      setB(8, AY + 1, 8, AIR);
      CHECK(!placeAt(STONE, 8, AY, 8, 3, 8.5, AY - .45, 8.5) && getB(8, AY + 1, 8) == AIR);   // ja o cubo na mesma celula nao cabe
      // eixo do tronco pela face clicada (RayHit: 0/1 = X, 2/3 = Y, 4/5 = Z)
      setB(16, AY, 16, STONE);
      CHECK(placeAt(LOG, 16, AY, 16, 3) && getB(16, AY + 1, 16) == LOG && getM(16, AY + 1, 16) == 0);   // em cima: Y
      CHECK(placeAt(LOG, 16, AY, 16, 1) && getB(17, AY, 16) == LOG && getM(17, AY, 16) == 1);           // +X: eixo X
      CHECK(placeAt(LOG, 16, AY, 16, 0) && getB(15, AY, 16) == LOG && getM(15, AY, 16) == 1);           // -X: eixo X
      CHECK(placeAt(LOG, 16, AY, 16, 5) && getB(16, AY, 17) == LOG && getM(16, AY, 17) == 2);           // +Z: eixo Z
      CHECK(placeAt(LOG, 16, AY, 16, 4) && getB(16, AY, 15) == LOG && getM(16, AY, 15) == 2);           // -Z: eixo Z
      CHECK(netEditMeta(netEdits[k3(17, AY, 16)]) == 1 && netEditMeta(netEdits[k3(16, AY, 17)]) == 2);   // o meta vai pela rede
      CHECK(!placeAt(LOG, 16, AY, 16, 3));                                      // ja ocupado
      // blocos do pack com eixo, por nome
      PK.clear(); gPkByFile.clear(); gPkByHash.clear();
      for (const char* n : {"oak_log", "stripped_birch_wood", "hay_block", "stone_bricks", "acacia_planks", "crimson_stem", "polished_basalt", "oak_log_top"}) { PkItem it; it.file = std::string("block/") + n; it.cube = true; it.tile = 1; PK.push_back(it); }
      pkBuildProps();
      CHECK(gBlkAxis[PK0 + 0] && gBlkAxis[PK0 + 1] && gBlkAxis[PK0 + 2] && gBlkAxis[PK0 + 5] && gBlkAxis[PK0 + 6]);
      CHECK(!gBlkAxis[PK0 + 3] && !gBlkAxis[PK0 + 4]);
      setB(20, AY, 20, STONE); CHECK(placeAt(PK0 + 0, 20, AY, 20, 5) && getM(20, AY, 21) == 2);   // log do pack tambem usa a face
      PK.clear(); gPkByFile.clear(); gPkByHash.clear(); pkBuildProps(); CHECK(!gBlkAxis[PK0]); }
    clearArena();

    SECTION("U5 mesher: tile de topo/lado por eixo e grao girado");
    { Chunk* c = getC(0, 0);
      const int top = blkTile(LOG, 0), side = blkTile(LOG, 1), bot = blkTile(LOG, 2); CHECK(top != side && side != bot && top != bot);
      for (int ax = 0; ax < 3; ax++) {
          setBM(8, AY + 3, 8, LOG, ax); build(0, 0, c);
          auto qs = quadsOf(c, 8, AY + 3, 8); CHECK(qs.size() == 6);
          const int axd = ax == 0 ? 1 : ax == 1 ? 0 : 2; int ends = 0, sides = 0, bad = 0;
          for (const Quad& q : qs) { const bool isEnd = (q.f >> 1) == axd;
              if (isEnd) { ends++; if (q.tile != (ax == 0 && q.f == 3 ? bot : top)) bad++; }   // vertical: base usa a textura de base; horizontal: as duas pontas = topo
              else { sides++; if (q.tile != side) bad++; if (!vTracks(q, axd)) bad++; } }       // lado: textura de lado, com o grao ao longo do eixo
          CHECK(ends == 2 && sides == 4 && bad == 0);
      }
      setBM(8, AY + 3, 8, LOG, 3); build(0, 0, c); { auto qs = quadsOf(c, 8, AY + 3, 8); int b = 0; for (auto& q : qs) if ((q.f >> 1) == 1 ? (q.tile != (q.f == 3 ? bot : top)) : q.tile != side) b++; CHECK(b == 0); }   // meta 3 = como vertical
      setB(8, AY + 3, 8, STONE); build(0, 0, c); { auto qs = quadsOf(c, 8, AY + 3, 8); int b = 0; for (auto& q : qs) if (!vTracks(q, 1) && (q.f >> 1) != 1) b++; CHECK(b == 0); }   // bloco sem eixo ignora o meta
      setB(8, AY + 3, 8, AIR); }

    printf("\n%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
