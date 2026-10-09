// Estandarte 3D colocavel (em pe e na parede), com o resource pack REAL. Sem pack => SKIP.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static int bannerItem(int c) { for (int id : gCreative[1]) if (isPk(id) && xbItemBlock(id) == XB_BANNER && gXbVar[id] == c) return id; return 0; }
int main() {
    if (chdir("..") != 0) return 99;
    if (!fs::exists("assets/resourcepacks/Ashen_16x.zip")) { printf("SKIP: resource pack ausente\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg();
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 12; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    for (int c = 0; c < 16; c++) { const int it = bannerItem(c); CHECK(it != 0); CHECK(it && gBnDrop[c] == it); CHECK(it && !PK[it - PK0].cube); }
    CHECK(entTile("banner/banner_base") >= 0);
    pkApplyItemData(); rsNames(); ptbrApply();   // mesma ordem de registerPackTab()
    // nomes: estandarte NAO pode herdar o nome da bigorna (gXbVar e compartilhado); cada cor tem o seu, e a bigorna continua certa
    { static const char* NM[16] = {"Estandarte branco", "Estandarte laranja", "Estandarte magenta", "Estandarte azul-claro", "Estandarte amarelo", "Estandarte verde-limão", "Estandarte rosa", "Estandarte cinza", "Estandarte cinza-claro", "Estandarte ciano", "Estandarte roxo", "Estandarte azul", "Estandarte marrom", "Estandarte verde", "Estandarte vermelho", "Estandarte preto"};
      for (int c = 0; c < 16; c++) { const int it = bannerItem(c); CHECK(it && PK[it - PK0].name == NM[c]); }
      int nAnv = 0; for (auto& kv : gXbItem) if (kv.second == XB_ANVIL && isPk(kv.first)) { ++nAnv; CHECK(PK[kv.first - PK0].name.rfind("Bigorna", 0) == 0); } CHECK(nAnv >= 1); }
    // em pe: topo do chao, jogador olhando para o norte (yaw 0) => frente voltada ao sul (rotacao 0), cor vermelha (14)
    gXbPlaceVar = 14; int hh[3] = {5, AY - 1, 5}, pp[3] = {5, AY, 5};
    CHECK(xbPlace(XB_BANNER, hh, pp, 0.f, 0, 0, 0));
    CHECK(getB(5, AY, 5) == XB_BANNER); CHECK(((getM(5, AY, 5) >> 4) & 15) == 14); CHECK((getM(5, AY, 5) & 15) == 0);
    { const std::vector<XQuad>& q = bnQuads(XB_BANNER, getM(5, AY, 5)); CHECK(q.size() == 18);   // haste + barra + pano = 3 caixas x 6 faces
      float ymin = 9, ymax = -9; for (const XQuad& x : q) for (int k = 0; k < 4; k++) { ymin = std::min(ymin, x.p[k][1]); ymax = std::max(ymax, x.p[k][1]); }
      CHECK(fabsf(ymin) < .001f); CHECK(fabsf(ymax - 29.333f / 16.f) < .002f);   // haste toca o chao; topo = barra a 1,83 bloco
      int tinted = 0; for (const XQuad& x : q) if (x.tc[0] < .99f || x.tc[1] < .5f) tinted++; CHECK(tinted == 6);   // so o pano e tingido
      float fz = -9; for (const XQuad& x : q) if (x.tc[1] < .5f) for (int k = 0; k < 4; k++) fz = std::max(fz, x.p[k][2]); CHECK(fz > .55f); }   // frente (+Z, sul)
    { AABB tmp[SH_MAXB]; CHECK(blockBoxesId(XB_BANNER, 5, AY, 5, false, tmp, 0) == 0); CHECK(blockBoxesId(XB_BANNER, 5, AY, 5, true, tmp, 0) == 1 && fabs(tmp[0].y1 - (AY + 1)) < 1e-6 && fabs(tmp[0].x0 - 5.25) < 1e-6); }
    // na parede: apontando para a face sul de uma pedra em (8,AY,8) => banner em (8,AY,9), dir 2 (frente ao sul)
    gNoBlockHooks = true; setB(8, AY, 8, STONE); gNoBlockHooks = false;
    gXbPlaceVar = 3; int h2[3] = {8, AY, 8}, p2[3] = {8, AY, 9};
    CHECK(xbPlace(XB_BANNER, h2, p2, 0.f, 0, 0, 0)); CHECK(getB(8, AY, 9) == XB_WBANNER); CHECK((getM(8, AY, 9) & 3) == 2); CHECK(((getM(8, AY, 9) >> 4) & 15) == 3);
    { AABB tmp[SH_MAXB]; CHECK(blockBoxesId(XB_WBANNER, 8, AY, 9, true, tmp, getM(8, AY, 9)) == 1); CHECK(tmp[0].z1 - 9 < .13 && tmp[0].y1 - AY < .79 && tmp[0].y1 - AY > .78);   // fina, junto a pedra (norte da celula)
      CHECK(blockBoxesId(XB_WBANNER, 8, AY, 9, false, tmp, getM(8, AY, 9)) == 0);
      const std::vector<XQuad>& q = bnQuads(XB_WBANNER, getM(8, AY, 9)); CHECK(q.size() == 12); float ymin = 9; for (const XQuad& x : q) for (int k = 0; k < 4; k++) ymin = std::min(ymin, x.p[k][1]); CHECK(ymin < 0); }   // pano pende abaixo da celula
    { Chunk* c = getC(0, 0); build(0, 0, c); const int bt = entTile("banner/banner_base"); int n = 0, nw = 0;   // o mesher emite as 12 faces do pano+barra+haste (em pe) e as da parede, usando a textura do estandarte
      for (size_t i = 0; i + 3 < c->mesh.size(); i += 4) { const V* q = &c->mesh[i]; const int ty = (int)floorf(q[0].v * ATLR), tx = (int)floorf(q[0].u * ATL); const bool inBn = ty >= ATLR0 + 1 && tx < 4; if (!inBn) continue;
          if (q[0].x > 5 && q[0].x < 6 && q[0].z > 5 && q[0].z < 6) n++; if (q[0].x > 8 && q[0].x < 9 && q[0].z > 9 && q[0].z < 10) nw++; }
      CHECK(bt >= 0); CHECK(n == 18); CHECK(nw == 12); }
    CHECK(xbSupported(XB_WBANNER, 8, AY, 9)); CHECK(xbSupported(XB_BANNER, 5, AY, 5));
    gNoBlockHooks = true; setB(8, AY, 8, AIR); setB(5, AY - 1, 5, AIR); gNoBlockHooks = false;
    CHECK(!xbSupported(XB_WBANNER, 8, AY, 9)); CHECK(!xbSupported(XB_BANNER, 5, AY, 5));
    // drop: a cor certa (meta guardado ao quebrar)
    CHECK(xbDropItem(XB_BANNER, 14 << 4) == gBnDrop[14]); CHECK(xbDropItem(XB_WBANNER, (3 << 4) | 2) == gBnDrop[3]);
    // gancho de vizinhos remove o estandarte sem suporte
    gNoBlockHooks = true; setB(5, AY, 5, AIR); gNoBlockHooks = false; setB(5, AY - 1, 5, STONE); gXbPlaceVar = 0; CHECK(xbPlace(XB_BANNER, hh, pp, 90.f, 0, 0, 0)); CHECK((getM(5, AY, 5) & 15) == 12);   // olhando a oeste => frente a leste (12)
    setB(5, AY - 1, 5, AIR); CHECK(getB(5, AY, 5) == AIR);
    // sprite e malha: o item continua plano e opaco
    for (int c = 0; c < 16; c++) { const int t = entSprite(1, c); CHECK(t >= 0); int tr = 0; for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const int a = atlas[((t / ATL * 16 + y) * ATW + (t % ATL * 16 + x)) * 4 + 3]; if (a && a != 255) tr++; } CHECK(tr == 0); }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
