// patchBug.2: bloco inteiro de mod (fora do Decocraft) encaixa na grade; icone plano de mod nao e posicionavel; Decocraft segue livre.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static const int AY = 150;
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static PropPack mk(const char* mod) {
    PropPack pk; pk.mod = mod; pk.dir = std::string("mods/") + mod;
    PropModel full; full.bb[0] = 0; full.bb[1] = 0; full.bb[2] = 0; full.bb[3] = 1; full.bb[4] = 1; full.bb[5] = 1; PropBox b{}; b.g[0] = .5f; b.g[1] = .5f; b.g[2] = .5f; b.g[3] = .5f; b.g[4] = .5f; b.g[5] = .5f; b.g[6] = 1; full.boxes.push_back(b);
    PropModel flat; flat.bb[0] = -.5f; flat.bb[1] = 0; flat.bb[2] = -.03f; flat.bb[3] = .5f; flat.bb[4] = 1; flat.bb[5] = .03f; PropBox c{}; c.g[0] = .5f; c.g[1] = .5f; c.g[2] = .015f; c.g[3] = .5f; c.g[4] = .5f; c.g[5] = .015f; c.g[6] = 1; flat.boxes.push_back(c);
    PropModel lad; lad.bb[0] = -.5f; lad.bb[1] = 0; lad.bb[2] = .45f; lad.bb[3] = .5f; lad.bb[4] = 1; lad.bb[5] = .45f; PropBox d{}; d.g[0] = 0; d.g[1] = .5f; d.g[2] = .45f; d.g[3] = .5f; d.g[4] = .5f; d.g[5] = 0; d.g[6] = 1; lad.boxes.push_back(d);
    pk.models.push_back(full); pk.models.push_back(flat); pk.models.push_back(lad);
    PropItem a; a.id = "cubo"; a.name = "Cubo"; a.model = 0; PropItem f; f.id = "icone"; f.name = "Icone"; f.model = 1; PropItem l; l.id = "escada"; l.name = "Escada"; l.model = 2; pk.items.push_back(a); pk.items.push_back(f); pk.items.push_back(l);
    return pk;
}
int main() {
    if (chdir("..") != 0) return 99;
    gPropPacks.clear(); gPropPacks.push_back(mk("fakemod")); gPropPacks.push_back(mk("decocraft")); gPropBase = {0, 3}; gPropTotal = 6;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 4; y++) setB(x, y, z, AIR); }
    const int cubo = PROP0, icone = PROP0 + 1, escada = PROP0 + 2, dCubo = PROP0 + 3, dIcone = PROP0 + 4;
    CHECK(isProp(cubo) && propGridBlock(cubo) && !propGridBlock(icone) && !propGridBlock(dCubo));
    CHECK(propModFlat(icone) && !propModFlat(cubo) && !propModFlat(dIcone));
    CHECK(propPlace(cubo, 4, AY - 1, 4, 3, 0, 40, AY, 40, 4.8, 4.2) && fabs(gProps.back().x - 4.5) < 1e-4 && fabs(gProps.back().z - 4.5) < 1e-4);   // posicao livre ignorada: centro da celula
    CHECK(!propModFlat(escada));   // fino de verdade (z=.45, espessura 0): nao e sprite de item
    const size_t n = gProps.size(); CHECK(!propPlace(icone, 8, AY - 1, 8, 3, 0, 40, AY, 40) && gProps.size() == n);   // icone plano de mod: nao posiciona
    CHECK(propPlace(dCubo, 12, AY - 1, 12, 3, 0, 40, AY, 40, 12.8, 12.2) && fabs(gProps.back().x - 12.5) > 1e-4);   // Decocraft: continua livre (1/8)
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
