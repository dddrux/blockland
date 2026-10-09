// Bau (Partes 2 e 3): bau duplo (juncao, regras de colocacao, quebrar uma metade), tela (54 slots, shift-click), tampa, rede e salvar/carregar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static int dropTotal() { int t = 0; for (auto& d : DROPS) t += d.n; return t; }
static int yawFor(int pf) { for (int y = 0; y < 360; y += 90) if (facingFromYaw((float)y) == pf) return y; return 0; }
static bool place(int x, int z, int var, int pf) { const int hh[3] = {x, AY - 1, z}, pp[3] = {x, AY, z}; return rsPlace(XB_CHEST, var, hh, pp, (float)yawFor(pf), 100, AY, 100); }
int main() {
    if (chdir("..") != 0) return 99;
    const bool pack = fs::exists("assets/resourcepacks/Ashen_16x.zip");
    if (pack) { applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg(); pkApplyItemData(); rsNames(); ptbrApply(); }
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    chestsClear(); const int ms = maxStack(STONE);
    // 1) colocar A e B lado a lado olhando para o mesmo lado (frente S): juntam; A (oeste) = esquerda, B = direita
    CHECK(place(2, 2, 0, 0)); CHECK(chestType(getM(2, AY, 2)) == 0 && (getM(2, AY, 2) & 3) == 2);
    CHECK(place(3, 2, 0, 0));
    CHECK(chestType(getM(2, AY, 2)) == 19 && chestType(getM(3, AY, 2)) == 20 && (getM(2, AY, 2) & 3) == 2 && (getM(3, AY, 2) & 3) == 2);
    int px, pz; CHECK(chestPartner(2, AY, 2, &px, &pz) && px == 3 && pz == 2); CHECK(chestPartner(3, AY, 2, &px, &pz) && px == 2 && pz == 2);
    // regras: nao se coloca ao lado de um duplo (nem em frente a ele)
    CHECK(!place(4, 2, 0, 0)); CHECK(!place(2, 3, 0, 0)); CHECK(getB(4, AY, 2) == AIR && getB(2, AY, 3) == AIR);
    // armadilhado ao lado de normal NAO junta (tipo diferente)
    CHECK(place(8, 2, 0, 0)); CHECK(place(9, 2, 1, 0)); CHECK(chestType(getM(8, AY, 2)) == 0 && chestType(getM(9, AY, 2)) == 1);
    // frente em outro eixo: vizinho a oeste mas olhando para o leste -> simples
    CHECK(place(12, 2, 0, 0)); CHECK(place(13, 2, 0, 3)); CHECK(chestType(getM(12, AY, 2)) == 0 && chestType(getM(13, AY, 2)) == 0);
    // junta no eixo Z quando a frente e leste/oeste; vizinho assume a frente nova
    CHECK(place(15, 6, 0, 3)); CHECK(place(15, 7, 0, 3)); CHECK(chestType(getM(15, AY, 6)) >= 19 && chestType(getM(15, AY, 7)) >= 19 && (getM(15, AY, 6) & 3) == (getM(15, AY, 7) & 3) && (getM(15, AY, 6) & 3) == 1);
    // 2) vista: superior = menor X; 54 slots
    int pos[2][3]; ChestInv* cv[2];
    CHECK(chestView(3, AY, 2, pos, cv) == 2); CHECK(pos[0][0] == 2 && pos[1][0] == 3 && cv[0] != cv[1]); CHECK(chestView(8, AY, 2, pos, cv) == 1);
    CHECK(chestView(15, AY, 7, pos, cv) == 2 && pos[0][2] == 6);
    // 3) abrir: tampa (bit 7) nas duas metades; slots 0..26 = oeste, 27..53 = leste
    CHECK(chestOpenAt(3, AY, 2) == 1); CHECK(gChest && gChRows == 6 && (getM(2, AY, 2) & 128) && (getM(3, AY, 2) & 128));
    CHECK(chestType(getM(2, AY, 2)) == 19);
    chestAt(2, AY, 2)->s[0] = {STONE, 5}; chestAt(3, AY, 2)->s[0] = {DIRT, 7};
    CHECK(chestSlotRef(0) == &chestAt(2, AY, 2)->s[0] && chestSlotRef(27) == &chestAt(3, AY, 2)->s[0] && chestSlotRef(54) == nullptr);
    // shift-click da mochila para o bau, e de volta
    for (auto& s : inv) s = Slot(); inv[0] = {STONE, 10}; cur = Slot();
    invClick(0, SDL_BUTTON_LEFT, true); CHECK(inv[0].id == 0 && chestAt(2, AY, 2)->s[0].n == 15);   // empilha no existente
    inv[1] = {COBBLE, 3}; invClick(1, SDL_BUTTON_LEFT, true); CHECK(inv[1].id == 0 && chestAt(2, AY, 2)->s[1].id == COBBLE);   // primeiro vazio
    invClick(SL_CHEST + 27, SDL_BUTTON_LEFT, true); CHECK(chestAt(3, AY, 2)->s[0].id == 0 && inv[8].id == DIRT && inv[8].n == 7);   // bau -> hotbar (de tras)
    slotClick(*slotRef(SL_CHEST + 40), SDL_BUTTON_LEFT); CHECK(cur.id == 0);   // clique em slot vazio sem cursor: nada
    cur = {GRAVEL, 4}; invClick(SL_CHEST + 40, SDL_BUTTON_LEFT, false); CHECK(cur.id == 0 && chestAt(3, AY, 2)->s[13].id == GRAVEL);   // soltar no segundo bau
    chestSyncNow();
    // 4) rede: o conteudo vai e volta pelo pacote NP_CHEST
    { ChestInv* c = chestAt(2, AY, 2); NBuf b; b.put<int32_t>(2); b.put<int32_t>(AY); b.put<int32_t>(2);
      for (int i = 0; i < CHEST_SLOTS; i++) { const bool ok = i == 4; b.put<int32_t>(ok ? netWireId(DIRT) : 0); b.put<uint8_t>(ok ? 9 : 0); }
      netHost = true; chestNetRecv(b); CHECK(c->s[4].id == DIRT && c->s[4].n == 9 && c->s[0].id == 0 && gChSnap[0].s[4].n == 9); netHost = false;
      c->s[4] = Slot(); c->s[0] = {STONE, 15}; c->s[1] = {COBBLE, 3}; gChSnap[0] = *c; }   // o pacote traz o bau inteiro: refaz o conteudo
    // 5) fechar: tampa volta; cursor volta para a mochila
    cur = {STONE, 3}; closeInv(); CHECK(!gChest && !(getM(2, AY, 2) & 128) && !(getM(3, AY, 2) & 128) && cur.id == 0);
    // bau com bloco em cima nao abre
    setBM(8, AY + 1, 2, STONE, 0); CHECK(chestOpenAt(8, AY, 2) == 2 && !gChest); setBM(8, AY + 1, 2, AIR, 0);
    // 6) salvar/carregar (os dois lados)
    mkdir("saves/t_dbl", 0755); chestsSave("t_dbl"); chestsClear(); CHECK(chestAt(2, AY, 2) == nullptr); chestsLoad("t_dbl");
    CHECK(chestAt(2, AY, 2) && chestAt(2, AY, 2)->s[0].id == STONE && chestAt(2, AY, 2)->s[0].n == 15 && chestAt(3, AY, 2)->s[13].id == GRAVEL);
    // 7) quebrar uma metade: solta so o conteudo dela, a outra vira bau simples e guarda o seu
    const int d0 = dropTotal(); const int inB = chestCount(*chestAt(3, AY, 2));
    setBM(3, AY, 2, AIR, 0);
    CHECK(dropTotal() - d0 == inB && chestType(getM(2, AY, 2)) == 0 && (getM(2, AY, 2) & 3) == 2 && chestAt(2, AY, 2) && chestAt(2, AY, 2)->s[0].n == 15 && chestAt(3, AY, 2) == nullptr);
    CHECK(chestOpenAt(2, AY, 2) == 1 && gChRows == 3); closeInv();
    // 8) forma e modelo
    { AABB o[2]; rsShape(0, 0, 0, XB_CHEST, (2) | (19 << 2), false, o); CHECK(o[0].x0 < .07 && o[0].x1 > .99); }
    { AABB o[2]; rsShape(0, 0, 0, XB_CHEST, (2) | (20 << 2), false, o); CHECK(o[0].x0 < .01 && o[0].x1 < .94); }
    { AABB o[2]; rsShape(0, 0, 0, XB_CHEST, (0) | (19 << 2), false, o); CHECK(o[0].x0 < .01 && o[0].x1 < .94); }
    setBM(18, AY, 18, XB_CHEST, 19 << 2); { const int want = gRsDrop[XB_CHEST - XB_DUST][0]; CHECK(rsDropItem(18, AY, 18, XB_CHEST) == want); if (pack) CHECK(want != 0); }
    setBM(18, AY, 18, XB_CHEST, 22 << 2); { const int want = gRsDrop[XB_CHEST - XB_DUST][1]; CHECK(rsDropItem(18, AY, 18, XB_CHEST) == want); }
    if (pack) {   // malha: metades esquerda/direita usam as texturas *_left/_right; tampa aberta sobe (y > 1) e fechada nao
        CHECK(entTile("chest/normal_left") >= 0 && entTile("chest/normal_right") >= 0 && entTile("chest/trapped_left") >= 0 && entTile("chest/trapped_right") >= 0);
        const auto& L = rsQuads(XB_CHEST, 2 | (19 << 2)), &R = rsQuads(XB_CHEST, 2 | (20 << 2)), &S = rsQuads(XB_CHEST, 2), &SO = rsQuads(XB_CHEST, 2 | 128);
        CHECK(!L.empty() && !R.empty() && L.size() == R.size() && L[0].tile != R[0].tile && L[0].tile != S[0].tile);
        auto maxY = [](const std::vector<XQuad>& q) { float m = 0; for (auto& x : q) for (int k = 0; k < 4; k++) m = std::max(m, x.p[k][1]); return m; };
        CHECK(maxY(S) < .9f && maxY(SO) > 1.2f);
        // esquerda (frente S): vai de x 1/16 ate 1; direita de 0 ate 15/16
        auto xr = [](const std::vector<XQuad>& q, float& a, float& b) { a = 9; b = -9; for (auto& x : q) for (int k = 0; k < 4; k++) { a = std::min(a, x.p[k][0]); b = std::max(b, x.p[k][0]); } };
        float a, b; xr(L, a, b); CHECK(fabsf(a - 1.f / 16) < .01f && fabsf(b - 1.f) < .01f); xr(R, a, b); CHECK(fabsf(a) < .01f && fabsf(b - 15.f / 16) < .01f);
    }
    printf("%d/%d verificacoes ok\n", checks - fails, checks); return fails ? 1 : 0;
}
