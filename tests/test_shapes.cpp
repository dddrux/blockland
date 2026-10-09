// Etapa 1.2: formas de bloco, raycast por AABB e colisao real. Roda o main.cpp REAL (funcoes static).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define NEAR(a, b) (fabs((double)(a) - (double)(b)) < 1e-4)
#define SECTION(n) printf("[%s]\n", n)

static const int AY = 200;   // arena: ar em y 200..215, piso de pedra em y=199 (chunks 0,0 e vizinhos)
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); }
}
static bool cast(double ox, double oy, double oz, double dx, double dy, double dz, RayHit& r, float reach = 5.f, bool liq = false) {
    const float o[3] = {(float)ox, (float)oy, (float)oz}, d[3] = {(float)dx, (float)dy, (float)dz}; return rayCast(o, d, reach, r, liq);
}
static AABB firstBox(int x, int y, int z, bool sel) { AABB b[SH_MAXB]; int n = blockBoxes(x, y, z, sel, b); return n ? b[0] : AABB{0, 0, 0, 0, 0, 0}; }

// regras ANTIGAS (antes da Etapa 1.2), para provar que os blocos existentes nao mudaram de comportamento
static bool oldSolid(int b) { return b != AIR && !isLiquid(b) && !(isXB(b) && !xbSolid(b)); }
static bool oldPick(int b) { return oldSolid(b) || isXB(b); }
static bool xbGap(int id) { return id == 929 || (id >= 934 && id <= 939); }   // ids dentro de 900..963 sem bloco

// forma de teste com VARIAS caixas e dependente de vizinho, como sera a cerca: poste + braco para +X se o vizinho for o mesmo bloco
static int fenceFn(int x, int y, int z, int id, int meta, bool sel, AABB* out) {
    int n = 0; out[n++] = {.375, 0, .375, .625, sel ? 1.0 : 1.5, .625};
    if (getB(x + 1, y, z) == id) out[n++] = {.625, .375, .4375, 1, sel ? .9375 : 1.5, .5625};
    return n;
}

int main() {
    fs::remove_all("/tmp/blt2"); fs::create_directories("/tmp/blt2"); if (chdir("/tmp/blt2") != 0) return 99;
    WORLD_SEED = 99; arena();

    SECTION("S1 tabela == regras antigas para TODOS os ids (nenhum bloco existente mudou)");
    { int dS = 0, dP = 0, gapBad = 0;
      for (int id = 0; id < 65536; id++) {
          if (isRs(id)) continue;   // aba Redstone: formas proprias, testadas em test_redstone.cpp
          if (xbGap(id)) { if (solid(id) || pickable(id)) gapBad++; continue; }
          dS += solid(id) != oldSolid(id); dP += pickable(id) != oldPick(id);
      }
      CHECK(dS == 0); CHECK(dP == 0); CHECK(gapBad == 0);
      CHECK(!solid(AIR) && !pickable(AIR) && !solid(WATER) && !pickable(WATER) && !pickable(WATER_FALL));
      CHECK(solid(STONE) && pickable(STONE) && solid(LEAVES) && solid(PK0 + 5)); }

    SECTION("S2 geometria das formas (dentro da celula, encostada no lado do suporte)");
    { const int DXt[4] = {-1, 0, 1, 0}, DZt[4] = {0, -1, 0, 1};   // tocha de parede k: suporte em -X,-Z,+X,+Z
      for (int id = XB0; id < XB_END; id++) { if (xbGap(id) || isRs(id)) continue; if (id >= XB_ANVIL && id < XB_ANVIL + 4) continue;
          const AABB b = firstBox(8, AY, 8, true); (void)b;
          AABB bb[SH_MAXB]; setB(8, AY, 8, id); const int n = blockBoxes(8, AY, 8, true, bb);
          CHECK(n == 1); if (n) { CHECK(bb[0].x0 >= 8 && bb[0].x1 <= 9 && bb[0].y0 >= AY && bb[0].y1 <= AY + 1 && bb[0].z0 >= 8 && bb[0].z1 <= 9); CHECK(bb[0].x1 > bb[0].x0 && bb[0].y1 > bb[0].y0 && bb[0].z1 > bb[0].z0); }
          CHECK(blockBoxes(8, AY, 8, false, bb) == 0); }   // tocha/alavanca/planta: sem colisao
      setB(8, AY, 8, AIR);
      setB(8, AY, 8, XB_TORCH); { AABB b = firstBox(8, AY, 8, true); CHECK(NEAR(b.y0, AY) && NEAR(b.y1, AY + .6) && NEAR(b.x1 - b.x0, .2) && NEAR(b.z1 - b.z0, .2)); }
      for (int k = 0; k < 4; k++) { setB(8, AY, 8, XB_TORCH + 1 + k); AABB b = firstBox(8, AY, 8, true);
          const double e0 = DXt[k] ? (DXt[k] < 0 ? b.x0 - 8 : 9 - b.x1) : (DZt[k] < 0 ? b.z0 - 8 : 9 - b.z1);
          CHECK(NEAR(e0, 0)); CHECK(b.y0 > AY && b.y1 < AY + 1); }   // encostada no bloco de suporte, solta do chao
      const int DXl[4] = {0, -1, 0, 1}, DZl[4] = {1, 0, -1, 0};   // alavanca de parede f: suporte em +Z,-X,-Z,+X
      for (int on = 0; on < 2; on++) for (int st = 0; st < 12; st++) { setB(8, AY, 8, XB_LEVER + on * 12 + st); AABB b = firstBox(8, AY, 8, true);
          if (st < 4) { const int f = st; const double e0 = DXl[f] ? (DXl[f] < 0 ? b.x0 - 8 : 9 - b.x1) : (DZl[f] < 0 ? b.z0 - 8 : 9 - b.z1); CHECK(NEAR(e0, 0)); }
          else if (st < 8) CHECK(NEAR(b.y0, AY));   // no chao
          else CHECK(NEAR(b.y1, AY + 1)); }          // no teto
      setB(8, AY, 8, XB_ANVIL); { AABB c = firstBox(8, AY, 8, false), s = firstBox(8, AY, 8, true); CHECK(NEAR(c.z1 - c.z0, 1) && NEAR(c.x1 - c.x0, .75) && NEAR(c.x0, 8.125)); CHECK(NEAR(s.x0, c.x0) && NEAR(s.z1, c.z1)); }
      setB(8, AY, 8, XB_ANVIL + 1); { AABB c = firstBox(8, AY, 8, false); CHECK(NEAR(c.x1 - c.x0, 1) && NEAR(c.z1 - c.z0, .75)); }
      setB(8, AY, 8, AIR); }

    SECTION("S3 raycast em cubos: face, ponto, celula de colocacao e alcance");
    { RayHit r; setB(8, AY, 8, STONE);
      CHECK(cast(8.5, AY + .5, 3.5, 0, 0, 1, r)); CHECK(r.hit && r.x == 8 && r.y == AY && r.z == 8 && r.face == 4 && NEAR(r.t, 4.5) && NEAR(r.hz, 8.0));
      CHECK(r.nx == 0 && r.ny == 0 && r.nz == -1);                        // colocar em (8,AY,7)
      CHECK(!cast(8.5, AY + .5, 2.5, 0, 0, 1, r));                         // 5,5 > alcance 5
      CHECK(cast(8.5, AY + 4.5, 8.5, 0, -1, 0, r) && r.face == 3 && r.y == AY && NEAR(r.t, 3.5) && r.ny == 1);
      CHECK(cast(3.5, AY + .5, 8.5, 1, 0, 0, r) && r.face == 0 && r.nx == -1);
      CHECK(cast(13.5, AY + .5, 8.5, -1, 0, 0, r) && r.face == 1 && r.nx == 1);
      CHECK(cast(8.5, AY + .5, 13.5, 0, 0, -1, r) && r.face == 5 && r.nz == 1);
      CHECK(!cast(4.2, AY + 3.0, 4.3, .6, -.6, .6, r, 5.f));                          // diagonal: o piso esta a 5,196 > alcance
      CHECK(cast(4.2, AY + 3.0, 4.3, .6, -.6, .6, r, 6.f) && r.x == 7 && r.y == AY - 1 && r.z == 7 && r.face == 3 && NEAR(r.hy, AY) && NEAR(r.hx, 7.2) && NEAR(r.hz, 7.3));   // diagonal acerta o topo do piso, no ponto exato
      // origem DENTRO do bloco: nao acerta o proprio bloco (acerta so o que vem depois)
      CHECK(!cast(8.5, AY + .5, 8.5, 0, 0, 1, r) );
      setB(8, AY, 8, AIR);
      // agua nao e mirada, a menos que se peca liquidos (balde)
      setB(10, AY, 8, WATER); CHECK(!cast(10.5, AY + .5, 4.5, 0, 0, 1, r)); CHECK(cast(10.5, AY + .5, 4.5, 0, 0, 1, r, 5.f, true) && r.x == 10 && r.z == 8);
      setB(10, AY, 8, AIR);
      // wrapper antigo ray(): h = bloco, p = celula de colocacao
      setB(8, AY, 8, STONE); { float o[3] = {8.5f, AY + .5f, 3.5f}, d[3] = {0, 0, 1}; int h[3], p[3]; CHECK(ray(o, d, h, p) && h[2] == 8 && p[2] == 7 && p[0] == 8 && p[1] == AY && gRay.hit && gRay.face == 4); }
      setB(8, AY, 8, AIR); }

    SECTION("S4 raycast em formas parciais: o 'vazio' da celula nao e acertado");
    { RayHit r; setB(8, AY, 8, XB_TORCH); setB(8, AY, 11, STONE);   // pedra a 3 blocos atras da tocha; raios de 'passa' partem de z=7
      CHECK(cast(8.5, AY + .3, 4.5, 0, 0, 1, r) && r.x == 8 && r.z == 8 && r.face == 4 && NEAR(r.t, 3.9) && NEAR(r.box.z0, 8.4));   // acerta a haste
      CHECK(cast(8.1, AY + .3, 7.0, 0, 0, 1, r) && r.z == 11 && r.x == 8 && NEAR(r.t, 4.0));                                                           // canto da celula: passa e acerta a pedra atras
      CHECK(cast(8.5, AY + .9, 7.0, 0, 0, 1, r) && r.z == 11);                                                                       // acima da chama (y>0.6): passa
      CHECK(cast(8.5, AY + 5, 8.5, 0, -1, 0, r) && r.y == AY && r.face == 3 && NEAR(r.hy, AY + .6) && r.ny == 1);                    // por cima: topo da haste
      setB(8, AY, 8, XB_LEVER + 0);   // alavanca de parede, suporte +Z: caixa z em 0.5..1
      CHECK(cast(8.5, AY + .5, 4.5, 0, 0, 1, r) && r.z == 8 && NEAR(r.t, 4.0) && r.face == 4);
      CHECK(cast(8.1, AY + .5, 7.0, 0, 0, 1, r) && r.z == 11);
      setB(8, AY, 8, XB_PLANT + 2);   // flor
      CHECK(cast(8.5, AY + .3, 4.5, 0, 0, 1, r) && r.z == 8 && NEAR(r.box.z0, 8.3));
      CHECK(cast(8.5, AY + .8, 7.0, 0, 0, 1, r) && r.z == 11);   // acima da flor (0.6)
      setB(8, AY, 8, XB_PLANT + 14); setB(8, AY + 1, 8, XB_PLANT + 15);   // planta alta: as duas metades sao mira
      CHECK(cast(8.5, AY + 1.3, 4.5, 0, 0, 1, r) && r.z == 8 && r.y == AY + 1);
      setB(8, AY, 8, AIR); setB(8, AY + 1, 8, AIR); setB(8, AY, 11, AIR); }

    SECTION("S5 forma de varias caixas calculada por funcao (padrao da cerca/painel)");
    { const int FID = 700, fi = shapeAddFn(fenceFn); blockShapeSet(FID, fi, fi);
      RayHit r; setB(8, AY, 8, FID);
      { AABB b[SH_MAXB]; CHECK(blockBoxes(8, AY, 8, true, b) == 1); CHECK(blockBoxes(8, AY, 8, false, b) == 1 && NEAR(b[0].y1, AY + 1.5)); }   // selecao 1,0 / colisao 1,5
      setB(9, AY, 8, FID);
      { AABB b[SH_MAXB]; CHECK(blockBoxes(8, AY, 8, true, b) == 2); }
      CHECK(cast(8.5, AY + .6, 4.5, 0, 0, 1, r) && r.x == 8 && r.z == 8 && NEAR(r.box.z0, 8.375) && NEAR(r.box.x1, 8.625));   // poste
      CHECK(cast(8.8, AY + .6, 4.5, 0, 0, 1, r) && r.x == 8 && NEAR(r.box.x1, 9.0) && NEAR(r.box.z0, 8.4375));                  // braco
      CHECK(cast(8.8, AY + .2, 4.5, 0, 0, 1, r) == false || !(r.x == 8 && r.z == 8));                                          // abaixo do braco: passa
      CHECK(cast(8.3, AY + .6, 4.5, 0, 0, 1, r) == false || !(r.x == 8 && r.z == 8));                                          // entre a borda e o poste: passa
      setB(9, AY, 8, AIR);
      { AABB b[SH_MAXB]; CHECK(blockBoxes(8, AY, 8, true, b) == 1); }                                                          // vizinho some => braco some
      // dois blocos de forma diferente na mesma celula nao se confundem: ar nao tem caixa
      CHECK(blockBoxes(9, AY, 8, true, nullptr) == 0);
      setB(8, AY, 8, AIR); }

    SECTION("S6 colisao do jogador usa a forma real");
    { std::vector<AABB> l;
      // 6a. cerca de 1,5: cair em cima para em 1,5 (e continua parado: precisa varrer a celula de baixo)
      setB(8, AY, 8, 700); Phys P; P.x = 8.5; P.y = AY + 4; P.z = 8.5;
      moveEntity(P, 0, -6, 0); CHECK(NEAR(P.y, AY + 1.5) && P.onGround);
      moveEntity(P, 0, -.0784, 0); CHECK(NEAR(P.y, AY + 1.5) && P.onGround);   // sem varrer y0-1 cairia atraves do poste ate o piso
      // 6b. tocha: sem colisao
      setB(8, AY, 8, AIR); setB(8, AY, 10, XB_TORCH); P = Phys(); P.x = 8.5; P.y = AY; P.z = 8.5; P.onGround = true;
      moveEntity(P, 0, 0, 2.0); CHECK(NEAR(P.z, 10.5));
      // 6c. bigorna Z (k=0): parede em x=8.125; jogador chega de -X e para a 0,3 dela
      setB(8, AY, 10, AIR); setB(8, AY, 12, XB_ANVIL); P = Phys(); P.x = 6.0; P.y = AY; P.z = 12.5; P.onGround = true;
      moveEntity(P, 3.0, 0, 0); CHECK(NEAR(P.x, 8.125 - .3) && P.collH);
      // 6d. laje (0,5): sobe sozinho (step 0,6) e fica em cima, sem flutuar nem afundar
      const int hs = shapeAdd({{0, 0, 0, 1, .5, 1}}); blockShapeSet(701, hs, hs); setB(14, AY, 4, 701);
      P = Phys(); P.x = 12.5; P.y = AY; P.z = 4.5; P.onGround = true;
      for (int i = 0; i < 8; i++) { moveEntity(P, .25, 0, 0); moveEntity(P, 0, -.0784, 0); }
      CHECK(NEAR(P.y, AY + .5) && P.x > 14.0);
      // 6e. bloco cheio continua igual
      setB(14, AY, 8, STONE); setB(14, AY + 1, 8, STONE); P = Phys(); P.x = 12.0; P.y = AY; P.z = 8.5; P.onGround = true;
      moveEntity(P, 5.0, 0, 0); CHECK(NEAR(P.x, 14 - .3));
      setB(14, AY, 8, AIR); setB(14, AY + 1, 8, AIR); setB(14, AY, 4, AIR); setB(8, AY, 12, AIR); }

    printf("\n%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
