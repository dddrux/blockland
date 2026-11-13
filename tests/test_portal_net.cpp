// Portal criado pela viagem: a moldura de obsidiana tem de ir pela rede (netEdits) junto com o portal; jogador remoto em outra dimensao nao e alvo.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    gCurDir = "tn_test2"; WORLD_SEED = 777; netherInit();
    gen(0, 0); dimSwitch(1);
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) gen(cx, cz);
    netEdits.clear();
    int px, py, pz, ax; createPortal(5, 5, px, py, pz, ax);
    int obs = 0, port = 0, bad = 0;
    for (auto& kv : netEdits) { int x, y, z; unk3(kv.first, x, y, z); const int id = netEditId(kv.second); if (((y >> 10) & 3) != 1) bad++; if (id == OBSIDIAN) obs++; if (id == XB_PORTAL) port++; }
    printf("  obsidiana na rede=%d portal=%d\n", obs, port);
    CHECK(port == 6); CHECK(obs >= 10); CHECK(bad == 0);   // moldura 4x5 sem os cantos = 10 + base extra
    CHECK(getB(4, py, 5) == OBSIDIAN); CHECK(getB(7, py + 1, 5) == OBSIDIAN); CHECK(getB(5, py + 3, 5) == OBSIDIAN);
    RemotePl r; r.dim = 0; CHECK(r.dim != gDim);   // remoto na superficie, eu no Nether
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
