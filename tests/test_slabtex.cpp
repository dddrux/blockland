// patchTex.3: texturas das lajes/escadas (RS_SM) precisam resolver no pack 1.12
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip"); applyMcPack();
    std::map<std::string, std::string> none;
    for (int v = 0; v < RS_SM_N; v++) { std::string sd, tp, bt; rsMat(v, sd, tp, bt);
        int a = xbTexTile(sd, none), b = xbTexTile(tp, none), c = xbTexTile(bt, none);
        if (a < 0 || b < 0 || c < 0) printf("  mat %d lado=%s(%d) topo=%s(%d) base=%s(%d)\n", v, sd.c_str(), a, tp.c_str(), b, bt.c_str(), c);
        CHECK(a >= 0 && b >= 0 && c >= 0);
        for (int t : {a, b, c}) if (!(t < 256 || t <= pkTileEnd())) { printf("  mat %d: tile %d alocado fora do atlas ja enviado\n", v, t); CHECK(false); } }
    { std::string sd, tp, bt; rsMat(7, sd, tp, bt);   // arenito: lado/topo/base = tiles do BLOCO (ja no atlas), nao tiles novos
      CHECK(xbTexTile(sd, none) == TILES[SANDSTONE][1] && xbTexTile(tp, none) == TILES[SANDSTONE][0] && xbTexTile(bt, none) == TILES[SANDSTONE][2]);
      CHECK(xbTexTile(sd, none) < gXbNext && xbTexTile(tp, none) <= pkTileEnd()); }
    for (auto& m : gPackMiss) printf("  MISS %s\n", m.c_str());
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
