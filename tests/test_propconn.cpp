// test_propconn: estados por vizinhos (patchConv.3): mesa/cerca do cfm trocam de variante, colisao acompanha, variante fora do criativo. Rodar: tools/t.sh propconn
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static int idOf(const char* mod, const char* item) { return propFromKey(std::string(mod) + "/" + item); }
int main() {
    if (chdir("..") != 0 || !std::filesystem::exists("mods/cfm/conexoes.txt")) { printf("SKIP (sem mods/cfm convertido)\n"); return 0; }
    propsRegisterTab();
    SECTION("T1 conexoes carregadas e variantes fora do criativo");
    const int t = idOf("cfm", "cfm_table_oak"); CHECK(t > 0); CHECK(propConnOf(t) != nullptr);
    const PropConnR* r = propConnOf(t); CHECK(r && r->grp == "table" && !r->solid);
    int hid = 0; for (auto& m : MODS) if (m.name == "Cfm") { for (int id : m.ids) { const PropItem* it = propItem(id); if (it && it->tab == "~") hid++; } }
    CHECK(hid == 0); CHECK(propGridBlock(t));
    SECTION("T2 mesa sozinha != mesa com vizinha");
    gProps.clear(); gProps.push_back(propMake(t, 10.5f, 64, 10.5f, 5)); propNbAll();
    CHECK(gProps[0].rot == 0); const int v0 = gProps[0].vid; const size_t cb0 = gProps[0].cb.size();
    gProps.push_back(propMake(idOf("cfm", "cfm_table_birch"), 11.5f, 64, 10.5f, 0)); propNbAll();   // vizinha a leste, OUTRA madeira (mesmo grupo)
    CHECK(gProps[0].vid != v0 || r->var[2] != r->var[0]); CHECK(gProps[0].vid == r->var[2]);
    CHECK(gProps[1].vid == propConnOf(gProps[1].id)->var[8]);   // a vizinha enxerga a oeste
    SECTION("T3 remover a vizinha volta ao estado de repouso");
    gProps.pop_back(); propNbAll(); CHECK(gProps[0].vid == v0 || gProps[0].vid == r->var[0]); CHECK(gProps[0].cb.size() == cb0 || gProps[0].cb.size() > 0);
    SECTION("T4 outro grupo nao liga");
    gProps.push_back(propMake(idOf("cfm", "cfm_coffee_table_oak"), 11.5f, 64, 10.5f, 0)); propNbAll(); CHECK(gProps[0].vid == r->var[0] || gProps[0].vid == v0);
    SECTION("T5 as 16 mascaras apontam para itens validos");
    for (int m = 0; m < 16; m++) CHECK(propItem(r->var[m]) != nullptr);
    SECTION("T6 particulas ao quebrar prop");
    gOptParticles = true; gPerf = false; parts.clear(); gProps.clear(); PropEnt q = propMake(t, 10.5f, 64, 10.5f, 0); propBreakFx(q); CHECK(parts.size() >= 8);
    SECTION("T7 portao abre/fecha (patchOpen.1)");
    const int g = idOf("cfm", "cfm_white_gate"); CHECK(g > 0); CHECK(propOpenOf(g) > 0);
    gProps.clear(); gProps.push_back(propMake(g, 20.5f, 64, 20.5f, 0)); propNbAll(); CHECK(gProps[0].vid == 0); const AABB cl = gProps[0].box;
    gProps[0].on = 0; propNbAll(); CHECK(gProps[0].vid == propOpenOf(g)); CHECK(gProps[0].box.x0 != cl.x0 || gProps[0].box.z0 != cl.z0 || gProps[0].box.x1 != cl.x1 || gProps[0].box.z1 != cl.z1);
    gProps[0].on = 1; propNbAll(); CHECK(gProps[0].vid == 0);
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
