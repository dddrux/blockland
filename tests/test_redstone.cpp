// Aba REDSTONE: todo item tem modelo (malha) e e posicionavel, validado contra o resource pack REAL. Sem pack => SKIP.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    if (access("assets/resourcepacks/Ashen_16x.zip", F_OK)) { printf("SKIP: resource pack ausente\n"); return 0; }
    applyMcPack(); loadPackItems(); xbInit(); registerPackTab(); CHECK(gPack.ok);
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 12; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    printf("[malhas]\n");
    for (int b = XB_DUST; b < XB_END; b++) for (int m : {0, 1, 5, 15, 21, 40, 70, 100, 130, 200}) { if (b == XB_DUST && m > 15 && m != 21) continue; if (rsQuads(b, m).empty()) { printf("  malha vazia id %d meta %d\n", b, m); ++fails; } ++checks; }
    printf("[itens da aba]\n");
    int tot = 0, placed = 0; std::vector<int> items = gCreative[2];
    for (int id : items) {
        tot++; const int xb = xbItemBlock(id); const std::string nm = isPk(id) ? PK[id - PK0].name : std::string("core#") + std::to_string(id);
        if (!xb) { printf("  sem modelo (cubo/outro): %s [%s]\n", nm.c_str(), isPk(id) ? PK[id - PK0].file.c_str() : "-"); continue; }
        if (!isRs(xb)) continue;   // tocha/alavanca ja existiam
        gNoBlockHooks = true; for (int y = AY; y < AY + 4; y++) setB(5, y, 5, AIR); setB(6, AY, 5, STONE); setB(6, AY + 1, 5, STONE); gNoBlockHooks = false;
        int hh[3] = {5, AY - 1, 5}, pp[3] = {5, AY, 5};   // clique no chao
        bool ok = rsPlace(xb, rsItemVar(id), hh, pp, 0.f, 9.5, AY, 9.5);
        if (!ok) { int h2[3] = {6, AY, 5}, p2[3] = {5, AY, 5}; ok = rsPlace(xb, rsItemVar(id), h2, p2, 0.f, 9.5, AY, 9.5); }   // parede (botao/gancho)
        if (!ok) { int h3[3] = {6, AY + 1, 5}, p3[3] = {5, AY + 1, 5}; ok = rsPlace(xb, rsItemVar(id), h3, p3, 0.f, 9.5, AY, 9.5); }
        CHECK(ok); if (!ok) { printf("  nao colocou: %s\n", nm.c_str()); continue; }
        placed++;
        int found = 0, fx = 0, fy = 0, fz = 0; for (int y = AY; y < AY + 3; y++) if (isRs(getB(5, y, 5)) && !found) { found = getB(5, y, 5); fx = 5; fy = y; fz = 5; }
        CHECK(found == xb || (xb == XB_PIST && found == XB_PIST + 1));
        AABB bb[SH_MAXB]; const int nb = blockBoxesId(found, fx, fy, fz, true, bb); CHECK(nb >= 1);
        CHECK(rsSupported(found, fx, fy, fz) || found == XB_DOOR);
        { const int it = rsDropItem(fx, fy, fz, found); if (found == XB_DOOR && (getM(fx, fy, fz) & 8)) CHECK(it == 0); else CHECK(it == id); }
    }
    printf("itens: %d, colocados por modelo novo: %d\n", tot, placed);
    // usar: porta abre (as duas metades), alcapao, repetidor
    gNoBlockHooks = true; for (int y = AY; y < AY + 4; y++) setB(5, y, 5, AIR); gNoBlockHooks = false;
    { int hh[3] = {5, AY - 1, 5}, pp[3] = {5, AY, 5}; CHECK(rsPlace(XB_DOOR, 1, hh, pp, 0.f, 9.5, AY, 9.5)); CHECK(getB(5, AY + 1, 5) == XB_DOOR && (getM(5, AY + 1, 5) & 8));
      CHECK(rsUse(5, AY, 5)); CHECK((getM(5, AY, 5) & 4) && (getM(5, AY + 1, 5) & 4)); gNoBlockHooks = false; }
    printf("[porta/alcapao: forma == malha; lado da lamina; icones; nomes]\n");
    for (int f = 0; f < 4; f++) {   // porta fechada: a lamina fica no lado f
        AABB bb[SH_MAXB]; setBM(8, AY, 8, XB_DOOR, f | (1 << 5)); const int n = blockBoxesId(XB_DOOR, 8, AY, 8, true, bb, f | (1 << 5)); const AABB sl = rsSlab(f);
        CHECK(n == 1 && fabs(bb[0].x0 - 8 - sl.x0) < 1e-3 && fabs(bb[0].x1 - 8 - sl.x1) < 1e-3 && fabs(bb[0].z0 - 8 - sl.z0) < 1e-3 && fabs(bb[0].z1 - 8 - sl.z1) < 1e-3);
        for (int hi = 0; hi < 2; hi++) {   // aberta: gira 90 em torno da dobradica (esq +1, dir -1)
            const int m = f | 4 | (hi << 4) | (1 << 5); const int n2 = blockBoxesId(XB_DOOR, 8, AY, 8, true, bb, m); const AABB so = rsSlab((f + (hi ? 3 : 1)) & 3);
            CHECK(n2 == 1 && fabs(bb[0].x0 - 8 - so.x0) < 1e-3 && fabs(bb[0].x1 - 8 - so.x1) < 1e-3 && fabs(bb[0].z0 - 8 - so.z0) < 1e-3 && fabs(bb[0].z1 - 8 - so.z1) < 1e-3);
            CHECK(blockBoxesId(XB_DOOR, 8, AY, 8, false, bb, m) == 1);   // colisao == selecao
        }
    }
    setBM(8, AY, 8, AIR, 0);
    for (int id : items) { if (!isPk(id)) continue; const int xb = xbItemBlock(id); if (!isRs(xb)) continue;
        if (xb == XB_PLATE || xb == XB_BTN || xb == XB_GATE || xb == XB_TRAP || xb == XB_DAY || xb == XB_CHEST) { CHECK(rsIconQuads(id) != nullptr); CHECK(!PK[id - PK0].cube); }
        const std::string& nm = PK[id - PK0].name; const bool pt = nm.find("Placa") == 0 || nm.find("Bot") == 0 || nm.find("Por") == 0 || nm.find("Al") == 0 || nm.find("Pist") == 0 || nm.find("Fun") == 0 || nm.find("Rep") == 0 || nm.find("Comp") == 0 || nm.find("Disp") == 0 || nm.find("Larg") == 0 || nm.find("Obs") == 0 || nm.find("Gan") == 0 || nm.find("Sen") == 0 || nm.find("Ba") == 0;
        CHECK(pt); if (!pt) printf("  nome nao traduzido: %s\n", nm.c_str()); }
    { std::vector<XQuad> a = rsQuads(XB_REPT, 0), c = rsQuads(XB_COMP, 0); CHECK(a.size() >= 10 && c.size() >= 15); }   // base + tochas
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
