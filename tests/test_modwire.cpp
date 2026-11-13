// BUG-046: ids de mod viajam por nome (pasta/bloco), nao pelo id local. Headless; inclui o main.cpp real.
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    { // BUG-046: mesmo mod em ids diferentes entre maquinas => viaja por nome, volta no id local
      modName[130] = "Foo"; modKey[130] = "modA/foo"; modName[131] = "Bar"; modKey[131] = "modA/bar"; gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 130; gModByHash[netFnvMod("modA/bar")] = 131;
      NBuf b; netPutEdit(b, {1, 2, 3, 131, 4}); b.r = 0; b.get<int32_t>(); b.get<int32_t>(); b.get<int32_t>(); const int wid = b.get<int32_t>();
      CHECK(wid < 0 && netWireId(130) != netWireId(131));
      gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 140; gModByHash[netFnvMod("modA/bar")] = 141;           // outra maquina
      CHECK(netUnwireId(wid) == 141 && netUnwireId(netWireId(130)) == 140);
      gModByHash.erase(netFnvMod("modA/bar")); CHECK(netUnwireId(wid) == -1);                                       // mod ausente => desconhecido
      CHECK(netWireId(COBBLE) == COBBLE && netWireId(150) == 150);                                                  // vanilla e id sem nome seguem crus
      modName[130].clear(); modKey[130].clear(); modName[131].clear(); modKey[131].clear(); gModByHash.clear(); }
    { // BUG-046b: drop, item na mao e armadura tambem por nome
      modName[130] = "Foo"; modKey[130] = "modA/foo"; modName[131] = "Bar"; modKey[131] = "modA/bar";
      gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 130; gModByHash[netFnvMod("modA/bar")] = 131;
      DropEnt d{}; d.id = 131; d.n = 3; d.nid = 7; NBuf b = netDropPacket(d); b.r = 2; b.get<uint32_t>(); for (int i = 0; i < 6; i++) b.get<float>();
      const int32_t wid = b.get<int32_t>(); const int cnt = b.get<int16_t>();
      gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 140; gModByHash[netFnvMod("modA/bar")] = 141;
      CHECK(netUnwireId(wid) == 141 && cnt == 3);
      Slot arm[4] = {}; arm[0].id = 130; arm[0].n = 1; arm[2].id = 131; arm[2].n = 1; arm[3].id = COBBLE; arm[3].n = 1;
      NBuf x = netXPacket(1, 1, 2, 3, 0, 0, 0, 131, false, 0, -1, false, false, 0, arm); x.r = 4;
      RemotePl r; netStateParse(r, x, 0);
      CHECK(!r.q.empty() && r.q.back().held == 141 && r.q.back().worn[0] == 140 && r.q.back().worn[1] == 0 && r.q.back().worn[2] == 141 && r.q.back().worn[3] == COBBLE && x.r == x.n);
      modName[130].clear(); modKey[130].clear(); modName[131].clear(); modKey[131].clear(); gModByHash.clear(); }
    { // BUG-046c: item do NP_MOBUSE por nome
      modName[130] = "Foo"; modKey[130] = "modA/foo"; gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 130;
      NBuf b; b.put<int32_t>(netWireId(130)); b.r = 0; gModByHash.clear(); gModByHash[netFnvMod("modA/foo")] = 150;
      CHECK(netUnwireId(b.get<int32_t>()) == 150);
      modName[130].clear(); modKey[130].clear(); gModByHash.clear(); }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
