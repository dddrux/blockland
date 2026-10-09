// Etapa 2 (parte 1): blocos com FRENTE, validados contra o resource pack REAL (assets/resourcepacks). Sem pack => SKIP.
#include <unistd.h>
#include <fstream>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 12; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
}
struct Quad { V v[4]; int f; int tile; };
static std::vector<Quad> quadsOf(Chunk* c, int bx, int by, int bz) {
    std::vector<Quad> out;
    for (size_t i = 0; i + 3 < c->mesh.size(); i += 4) {
        const V* q = &c->mesh[i]; bool in = true; for (int k = 0; k < 4; k++) if (q[k].x < bx - .01f || q[k].x > bx + 1.01f || q[k].y < by - .01f || q[k].y > by + 1.01f || q[k].z < bz - .01f || q[k].z > bz + 1.01f) in = false;
        if (!in) continue; Quad Q; memcpy(Q.v, q, sizeof Q.v);
        const float C[3][2] = {{q[0].x, bx + .5f}, {q[0].y, by + .5f}, {q[0].z, bz + .5f}};
        Q.f = -1; for (int a = 0; a < 3; a++) { bool same = true; for (int k = 1; k < 4; k++) { const float cc = a == 0 ? q[k].x : a == 1 ? q[k].y : q[k].z; if (fabsf(cc - C[a][0]) > 1e-4f) same = false; } if (same) Q.f = a * 2 + (C[a][0] > C[a][1] ? 0 : 1); }   // 0 -X? ver abaixo
        float cu = 0, cv = 0; for (int k = 0; k < 4; k++) { cu += q[k].u; cv += q[k].v; } cu /= 4; cv /= 4;
        Q.tile = (int)floorf(cv * ATLR) * ATL + (int)floorf(cu * ATL); out.push_back(Q);
    }
    return out;
}
// Quad::f usa a MESMA numeracao do mesher: 0 +X, 1 -X, 2 topo (+Y), 3 base (-Y), 4 +Z, 5 -Z
static int tileOfFile(const char* f) { auto it = gPkByFile.find(f); return it == gPkByFile.end() ? -1 : PK[it->second - PK0].tile; }
static int idOf(const char* f) { auto it = gPkByFile.find(f); return it == gPkByFile.end() ? -1 : it->second; }

static int planeQuads(Chunk* c, int px, int by, int bz) { int n = 0; for (const Quad& q : quadsOf(c, px, by, bz)) if ((q.f == 0 || q.f == 1) && fabsf(q.v[0].x - px) < .01f) n++; return n; }
int main() {
    if (chdir("..") != 0) return 99;
    if (!fs::exists("assets/resourcepacks/Ashen_16x.zip")) { printf("SKIP: resource pack ausente\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); CHECK(gPack.ok);
    WORLD_SEED = 5; arena();

    SECTION("P1 tabelas de faces no pack real");
    const int tTop = tileOfFile("block/pumpkin_top"), tSide = tileOfFile("block/pumpkin_side"), tBot = tileOfFile("block/pumpkin_bottom");
    CHECK(tTop > 0 && tSide > 0 && tBot > 0 && tTop != tSide && tSide != tBot);
    const int idPump = idOf("block/pumpkin_top"), idCarved = idOf("block/carved_pumpkin"), idJack = idOf("block/jack_o_lantern");
    const int idFurn = idOf("block/furnace"), idDisp = idOf("block/dispenser"), idDrop = idOf("block/dropper");
    CHECK(idPump >= PK0 && idCarved >= PK0 && idJack >= PK0 && idFurn >= PK0 && idDisp >= PK0 && idDrop >= PK0);
    CHECK(idFurn == idOf("block/furnace_top"));                                           // fornalha = bloco da regra X_top
    { const PkItem& p = PK[idPump - PK0]; CHECK(p.ft[0] == tTop && p.ft[1] == tSide && p.ft[2] == tBot && p.fr < 0 && !gBlkFacing[idPump]); }   // abobora normal: sem frente (regressao)
    { const PkItem& m = PK[idOf("block/melon_top") - PK0]; CHECK(m.ft[0] == tileOfFile("block/melon_top") && m.ft[1] == tileOfFile("block/melon_side") && m.ft[2] == tileOfFile("block/melon_bottom") && m.fr < 0); }
    for (int id : {idCarved, idJack}) { const PkItem& p = PK[id - PK0]; CHECK(p.cube && p.ft[0] == tTop && p.ft[1] == tSide && p.ft[2] == tBot && p.fr == p.tile && gBlkFacing[id]); }   // topo/lado/base da abobora + rosto na frente
    CHECK(PK[idCarved - PK0].fr == tileOfFile("block/carved_pumpkin") && PK[idJack - PK0].fr == tileOfFile("block/jack_o_lantern"));
    { const int fs = tileOfFile("block/furnace_side"), ft = tileOfFile("block/furnace_top");
      for (int id : {idDisp, idDrop}) { const PkItem& p = PK[id - PK0]; CHECK(p.cube && p.ft[0] == ft && p.ft[1] == fs && p.ft[2] == ft && gBlkFacing[id]); }   // lados/topo/base como no 1.12
      CHECK(PK[idDisp - PK0].fr == tileOfFile("block/dispenser_front") && PK[idDrop - PK0].fr == tileOfFile("block/dropper_front") && PK[idFurn - PK0].fr == tileOfFile("block/furnace_front"));
      CHECK(PK[idDisp - PK0].name == "Dispenser" && PK[idDrop - PK0].name == "Dropper"); }
    CHECK(gPkEmit[idJack - PK0] == 15 && gPkEmit[idCarved - PK0] == 0);                    // so a jack-o'-lantern emite luz
    CHECK(gPkHard[idDisp - PK0] == 3.5f && gPkHard[idFurn - PK0] == 3.5f && gPkHard[idCarved - PK0] == 1.f && gPkTool[idCarved - PK0] == 2);   // dureza do 1.12 (abobora: machado)
    CHECK(isPkBlk(idDisp) && isBlk(idDisp));                                              // agora o dispenser e um bloco colocavel
    CHECK(pkFront(idCarved) == PK[idCarved - PK0].fr && pkFront(STONE) < 0 && pkFront(idPump) < 0);

    SECTION("P2 colocacao: a frente aponta para o jogador (4 direcoes)");
    // yaw 0 olha para -Z => frente aponta para +Z (sul, meta 2); yaw 90 olha para -X => frente +X (meta 1); yaw 180 olha +Z => frente -Z (meta 0); yaw 270 olha +X => frente -X (meta 3)
    { const float yaws[4] = {0.f, 90.f, 180.f, 270.f}; const int want[4] = {2, 1, 0, 3};
      for (int i = 0; i < 4; i++) { setB(8, AY, 8, AIR); PlaceCtx c{8, AY - 1, 8, 3, .5, 1, .5, yaws[i], 0.f, false, 100, 100, 100};
          CHECK(placeBlock(idCarved, c) && getB(8, AY, 8) == idCarved && getM(8, AY, 8) == want[i]); }
      setB(8, AY, 8, AIR); PlaceCtx c{8, AY - 1, 8, 3, .5, 1, .5, 0.f, 0.f, false, 100, 100, 100}; CHECK(placeBlock(idPump, c) && getM(8, AY, 8) == 0);   // abobora normal: meta 0 (sem orientacao)
      setB(8, AY, 8, AIR); }

    SECTION("P3 malha: so UMA face lateral tem o rosto; topo e base certos; nas 4 direcoes");
    { Chunk* c = getC(0, 0); const int tFront = PK[idCarved - PK0].fr;
      for (int d = 0; d < 4; d++) {
          setBM(8, AY + 3, 8, idCarved, d); build(0, 0, c); auto qs = quadsOf(c, 8, AY + 3, 8); CHECK(qs.size() == 6);
          // direcao 0 N(-Z)=f5, 1 L(+X)=f0, 2 S(+Z)=f4, 3 O(-X)=f1
          const int frontF = d == 0 ? 5 : d == 1 ? 0 : d == 2 ? 4 : 1; int nFront = 0, bad = 0;
          for (const Quad& q : qs) { if (q.f == 2) { if (q.tile != tTop) bad++; } else if (q.f == 3) { if (q.tile != tBot) bad++; }
              else if (q.f == frontF) { nFront++; if (q.tile != tFront) bad++; } else if (q.tile != tSide) bad++; }
          CHECK(nFront == 1 && bad == 0);
      }
      setBM(8, AY + 3, 8, idPump, 2); build(0, 0, c); { auto qs = quadsOf(c, 8, AY + 3, 8); int b = 0; for (auto& q : qs) if (q.tile != (q.f == 2 ? tTop : q.f == 3 ? tBot : tSide)) b++; CHECK(b == 0); }   // abobora normal ignora o meta
      setBM(8, AY + 3, 8, idDisp, 1); build(0, 0, c); { auto qs = quadsOf(c, 8, AY + 3, 8); int fr = 0, side = 0; for (auto& q : qs) { if (q.tile == PK[idDisp - PK0].fr) { fr++; CHECK(q.f == 0); }   // meta 1 = frente +X = f0
 else if (q.f != 2 && q.f != 3 && q.tile == PK[idDisp - PK0].ft[1]) side++; } CHECK(fr == 1 && side == 3); }
      setB(8, AY + 3, 8, AIR); }

    SECTION("P4 save/load e rede preservam a orientacao");
    { setBM(10, AY, 10, idCarved, 3); netSendBlock(10, AY, 10, idCarved, 3);
      CHECK(netEditId(netEdits[k3(10, AY, 10)]) == idCarved && netEditMeta(netEdits[k3(10, AY, 10)]) == 3);
      NBuf b; netPutEdit(b, {10, AY, 10, idCarved, 3}); b.r = 0; b.get<int32_t>(); b.get<int32_t>(); b.get<int32_t>(); const int wid = b.get<int32_t>(); const int meta = b.get<uint8_t>();
      CHECK(netUnwireId(wid) == idCarved && meta == 3);                                   // o id do pack viaja por hash do nome, o meta junto
      setB(10, AY, 10, AIR); }

    SECTION("P5 folhas como bloco (pack real)");
    { const int sl = idOf("block/spruce_leaves"), bl = idOf("block/birch_leaves"), sg = idOf("block/spruce_log");
      CHECK(sl > 0 && isPkBlk(sl) && bl > 0 && isPkBlk(bl) && sg > 0);
      CHECK(leafKind(sl) == 2 && leafKind(bl) == 3 && leafKind(LEAVES) == 1 && leafKind(STONE) == 0);   // abeto/betula: cor fixa
      CHECK(lightOpac(sl) == 1 && solid(sl));                                                            // deixa a luz passar, tem colisao
      CHECK(isLogB(sg) && isLogB(LOG) && !isLogB(sl));
      treeSpecies(1); CHECK(gTreeLeaf == sl && gTreeLog == sg); treeSpecies(0); CHECK(gTreeLeaf == LEAVES && gTreeLog == LOG);
      setB(12, AY, 12, sg); setB(12, AY + 1, 12, sl); CHECK(leafSupported(12, AY + 1, 12));               // tronco do pack sustenta folha do pack
      setB(12, AY, 12, AIR); CHECK(!leafSupported(12, AY + 1, 12));
      Chunk* c = getC(0, 0); build(0, 0, c); CHECK(quadsOf(c, 12, AY + 1, 12).size() == 6);
      setB(12, AY + 1, 12, AIR); }

    SECTION("P6 mesclas de folhas e mipmaps");
    { const int sl = idOf("block/spruce_leaves"), bl = idOf("block/birch_leaves"); Chunk* c = getC(0, 0);
      setB(12, AY + 2, 12, sl); setB(13, AY + 2, 12, sl); build(0, 0, c);
      CHECK(planeQuads(c, 13, AY + 2, 12) == 1);          // mesma especie: so UMA face no plano compartilhado
      setB(13, AY + 2, 12, bl); build(0, 0, c);
      CHECK(planeQuads(c, 13, AY + 2, 12) == 2);          // especies diferentes: cada uma mostra a sua (sem misturar)
      setB(12, AY + 2, 12, AIR); setB(13, AY + 2, 12, AIR);
      uint8_t src[4 * 4 * 4], dst[2 * 2 * 4]; memset(src, 0, sizeof src);                                // 2x2 de cima-esquerda: 1 pixel verde opaco + 3 furos
      src[0] = 10; src[1] = 200; src[2] = 30; src[3] = 255; mipDown(src, 4, 4, dst);
      CHECK(dst[0] == 10 && dst[1] == 200 && dst[2] == 30 && dst[3] == 64);                               // cor NAO escurece pelos furos; alfa = cobertura (1/4)
    }
    SECTION("P7 mudas: colocacao, solo e queda");
    { int sap = idOf("block/oak_sapling"); CHECK(sap > 0 && xbItemBlock(sap) == XB_SAPLING);
      CHECK(gXbDrop[XB_SAPLING - XB0] == sap && !gXM[XB_SAPLING - XB0].empty() && !pickable(AIR) && pickable(XB_SAPLING) && !solid(XB_SAPLING));
      setB(14, AY, 14, DIRT); setB(16, AY, 14, STONE);
      int hh[3] = {14, AY, 14}, pp[3] = {14, AY + 1, 14}; CHECK(xbPlace(XB_SAPLING, hh, pp, 0.f, 0, 0, 0) && getB(14, AY + 1, 14) == XB_SAPLING);   // terra: ok
      int h2[3] = {16, AY, 14}, p2[3] = {16, AY + 1, 14}; CHECK(!xbPlace(XB_SAPLING, h2, p2, 0.f, 0, 0, 0) && getB(16, AY + 1, 14) == AIR);       // pedra: recusa
      setB(14, AY, 14, STONE); CHECK(getB(14, AY + 1, 14) == AIR);                                         // perdeu o solo: cai sozinha pelo gancho de vizinhos
      CHECK(xbSoilOk(GRASS) && xbSoilOk(DIRT) && !xbSoilOk(SAND) && !xbSoilOk(STONE)); setB(14, AY, 14, AIR); setB(16, AY, 14, AIR); }

    SECTION("P8 nada reseta o filtro do atlas (os mipmaps morriam no 1o icone do hotbar)");
    { int bad = 0, binds = 0; for (auto& e : fs::directory_iterator("src")) { if (e.path().extension() != ".inc" && e.path().extension() != ".cpp") continue;
          std::ifstream in(e.path()); std::vector<std::string> L; std::string l; while (std::getline(in, l)) L.push_back(l);
          const bool upl = e.path().filename() == "assets_atlas.inc" || e.path().filename() == "main_loop.inc";   // unicos lugares que podem configurar o atlas (criacao + upload)
          for (size_t i = 0; i < L.size(); i++) if (L[i].find("glBindTexture(GL_TEXTURE_2D, atlasTex)") != std::string::npos) { binds++;
              for (size_t j = i; j < L.size() && j < i + 3; j++) if (L[j].find("GL_TEXTURE_MIN_FILTER") != std::string::npos && !upl) bad++; } }
      CHECK(binds > 3 && bad == 0); }


    SECTION("cacto: modelo recuado 1/16 nos lados, colisao 15/16, selecao ate o topo");
    { const int ca = idOf("block/cactus"); CHECK(ca > 0 && ca == gCactusId);
      arena(); setB(6, AY, 6, ca); Chunk* c = getC(0, 0); build(0, 0, c);
      int side = 0, topBot = 0, bad = 0;
      for (const Quad& q : quadsOf(c, 6, AY, 6)) {
          if (q.f == 2 || q.f == 3) { if (q.f == 2 && q.v[0].y > AY + .5f) topBot++; continue; }   // topo do cacto (a pedra embaixo agora desenha o topo dela: era o buraco)
          side++; const float px = q.v[0].x, pz = q.v[0].z;
          // +X/-X em x = 6+15/16 / 6+1/16; +Z/-Z em z = 6+15/16 / 6+1/16 (plano recuado), e ainda cobre 0..1 no outro eixo
          const float want = (q.f == 0 || q.f == 4) ? .9375f : .0625f; const float got = (q.f < 2) ? px - 6 : pz - 6;
          if (fabsf(got - want) > 1e-3f) bad++;
      }
      CHECK(side == 4 && bad == 0 && topBot == 1);
      AABB bb[8]; int n = blockBoxesId(ca, 6, AY, 6, false, bb), ns = blockBoxesId(ca, 6, AY, 6, true, bb);
      CHECK(n == 1 && ns == 1);
      n = blockBoxesId(ca, 6, AY, 6, false, bb); CHECK(fabs(bb[0].x0 - 6.0625) < 1e-6 && fabs(bb[0].x1 - 6.9375) < 1e-6 && fabs(bb[0].y1 - (AY + .9375)) < 1e-6);
      ns = blockBoxesId(ca, 6, AY, 6, true, bb); CHECK(fabs(bb[0].y1 - (AY + 1.0)) < 1e-6);
      setB(6, AY, 6, AIR); }

    SECTION("cacto: sem 'fantasma' na base (areia desenha o topo, luz passa, pilha sem faces coplanares)");
    { const int ca = idOf("block/cactus"); CHECK(lightOpac(ca) == 0);
      arena(); setB(8, AY - 1, 8, SAND); setB(8, AY, 8, ca); setB(10, AY, 10, ca); setB(10, AY + 1, 10, ca);
      Chunk* c = getC(0, 0); build(0, 0, c);
      int sandTop = 0; for (const Quad& q : quadsOf(c, 8, AY - 1, 8)) if (q.f == 2 && fabsf(q.v[0].y - AY) < .01f) sandTop++;
      CHECK(sandTop == 1);   // antes 0: o anel de 1/16 em volta do cacto mostrava o vazio
      int lowTop = 0, upBot = 0, upTop = 0;
      for (const Quad& q : quadsOf(c, 10, AY, 10)) if (q.f == 2 && fabsf(q.v[0].y - (AY + 1)) < .01f) lowTop++;
      for (const Quad& q : quadsOf(c, 10, AY + 1, 10)) { if (q.f == 3) upBot++; if (q.f == 2) upTop++; }
      CHECK(lowTop == 0 && upBot == 0 && upTop == 1);
      setB(8, AY, 8, AIR); setB(8, AY - 1, 8, STONE); setB(10, AY + 1, 10, AIR); setB(10, AY, 10, AIR); }

    SECTION("inventario: faces soltas do pack nao viram item (cactus_bottom, furnace_front...) e apontam para o bloco real");
    { registerPackTab();
      std::vector<int> all; for (int t = 0; t < NTAB; t++) for (int id : gCreative[t]) all.push_back(id);
      for (auto& m : MODS) for (int id : m.ids) all.push_back(id);
      int faces = 0; for (int id : all) if (pkIsFaceOnly(id)) { faces++; printf("  face solta: %s\n", PK[id - PK0].file.c_str()); }
      // sobram so faces sem equivalente real; essas ganham nome limpo (sem "Top/Side/Front...")
      for (int id : all) if (pkIsFaceOnly(id)) { const std::string& nm = PK[id - PK0].name; for (const char* w : {" Top", " Side", " Front", " Bottom", " Back"}) { const size_t l = strlen(w); CHECK(!(nm.size() > l && nm.compare(nm.size() - l, l, w) == 0)); } }
      std::vector<int> pk; for (auto& m : MODS) if (m.name == "Resource pack") pk = m.ids;
      CHECK(!pk.empty()); int inPack = 0; for (int id : pk) if (pkIsFaceOnly(id)) inPack++; CHECK(inPack == 0);   // aba extra sem nenhuma face
      auto count = [&](int want) { int n = 0; for (int id : all) if (id == want) n++; return n; };
      CHECK(count(idOf("block/cactus")) >= 1);                 // cacto de verdade esta no criativo (antes: "Cactus Side" plano)
      const int cside = idOf("block/cactus_side"), cbot = idOf("block/cactus_bottom"); CHECK(cside > 0 && cbot > 0);
      CHECK(count(cside) == 0 && count(cbot) == 0);
      CHECK(pkIsFaceOnly(cbot) && pkIsFaceOnly(cside) && PK[idOf("block/cactus") - PK0].cube);
      for (const char* f : {"block/lilac_top", "block/rose_bush_top", "block/tall_grass_top", "block/anvil", "block/torch"}) CHECK(!pkIsFaceOnly(idOf(f)) && count(idOf(f)) >= 1);   // colocaveis por modelo continuam no criativo
      CHECK(count(idOf("block/sunflower_top")) >= 1 && count(idOf("block/sunflower_front")) == 0);
      for (const char* f : {"block/furnace", "block/pumpkin", "block/tnt", "block/hay_block", "block/melon", "block/jukebox"}) { const int id = idOf(f); CHECK(id > 0 && PK[id - PK0].cube && count(id) >= 1); }
      CHECK(findItem("cactus bottom") == 0 || !pkIsFaceOnly(findItem("cactus bottom")));
    }

    SECTION("mao: itens de sprite diagonal giram 45 graus para ficar de pe; planos e blocos nao");
    { CHECK(heldDiagonal(IT_DAXE) && heldDiagonal(IT_DSWORD) && heldDiagonal(IT_IPICK) && heldDiagonal(IT_BOW));
      CHECK(!heldDiagonal(IT_APPLE) && !heldDiagonal(IT_STICK) && !heldDiagonal(STONE));
      const int pk = idOf("item/netherite_sword"); if (pk > 0) CHECK(heldDiagonal(pk));
      const int ap = idOf("item/golden_carrot"); if (ap > 0) CHECK(!heldDiagonal(ap)); }

    printf("\n%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
