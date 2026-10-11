// Testes headless da camada de metadados (Etapa 1.1). Inclui o main.cpp REAL do jogo (as funcoes sao static).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }   // unico simbolo SDL chamado pelos caminhos testados
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const size_t N = (size_t)CW * CH * CW;

static void loadArea(int r) { for (int cz = -r; cz <= r; cz++) for (int cx = -r; cx <= r; cx++) if (!getC(cx, cz)) gen(cx, cz); }
struct Snap { std::vector<BT> b; std::vector<uint8_t> m; };
static std::map<int64_t, Snap> snapshot() { std::map<int64_t, Snap> s; for (auto& kv : W) s[kv.first] = {std::vector<BT>(kv.second->b, kv.second->b + N), std::vector<uint8_t>(kv.second->m, kv.second->m + N)}; return s; }
static void mkSave(const std::string& dir) { fs::create_directories("saves/" + dir); gCurDir = dir; }

int main() {
    fs::remove_all("/tmp/blt"); fs::create_directories("/tmp/blt/saves"); if (chdir("/tmp/blt") != 0) return 99;
    WORLD_SEED = 1234; loadArea(1);

    SECTION("T1 meta inicial, acessores e limites");
    { bool allZero = true; for (auto& kv : W) for (size_t i = 0; i < N; i++) if (kv.second->m[i]) { allZero = false; break; }
      CHECK(allZero); }
    CHECK(getM(5, 70, 5) == 0);
    CHECK(getM(0, -1, 0) == 0 && getM(0, CH, 0) == 0);            // fora do mundo
    CHECK(getM(100000, 70, 100000) == 0);                           // chunk nao carregado: nao quebra
    setBM(100000, 70, 100000, STONE, 4);                            // idem na escrita: ignora sem crash
    setBM(3, 100, 3, STONE, 9);
    CHECK(getB(3, 100, 3) == STONE && getM(3, 100, 3) == 9);
    setB(3, 100, 3, STONE);                                         // mesmo id, meta diferente => zera
    CHECK(getB(3, 100, 3) == STONE && getM(3, 100, 3) == 0);
    setBM(3, 100, 3, DIRT, 200); setBM(3, 100, 3, DIRT, 201);
    CHECK(getM(3, 100, 3) == 201);                                  // troca so de meta
    setM(3, 100, 3, 7);
    CHECK(getB(3, 100, 3) == DIRT && getM(3, 100, 3) == 7);         // setM preserva o bloco
    setB(3, 100, 3, COBBLE); CHECK(getM(3, 100, 3) == 0);           // trocar de bloco com setB zera o meta
    setBM(3, 100, 3, COBBLE, 0x1FF); CHECK(getM(3, 100, 3) == 0xFF);// meta limitado a 8 bits
    setBM(-1, 70, -1, DIRT, 11); setBM(15, 70, 15, DIRT, 12); setBM(16, 70, 16, DIRT, 13);   // chunks (-1,-1), (0,0), (1,1)
    CHECK(getM(-1, 70, -1) == 11 && getM(15, 70, 15) == 12 && getM(16, 70, 16) == 13);
    CHECK(getC(-1, -1)->m[idx(15, 70, 15)] == 11 && getC(0, 0)->m[idx(15, 70, 15)] == 12 && getC(1, 1)->m[idx(0, 70, 0)] == 13);

    SECTION("T2 mudar so o meta refaz a malha (e a do vizinho na borda)");
    for (auto& kv : W) kv.second->dirty = false;
    setM(4, 100, 4, 3); CHECK(getC(0, 0)->dirty);                    // meta de bloco sem mudar id
    CHECK(!getC(-1, 0)->dirty);                                       // interior: vizinho nao precisa refazer
    for (auto& kv : W) kv.second->dirty = false;
    setM(0, 100, 4, 5); CHECK(getC(0, 0)->dirty && getC(-1, 0)->dirty);   // na borda: vizinho tambem
    for (auto& kv : W) kv.second->dirty = false;
    setBM(9, 100, 9, AIR, 3); CHECK(getC(0, 0)->dirty);              // AIR com meta nao cai no caminho "so agua"
    setBM(9, 100, 9, AIR, 0);
    for (auto& kv : W) kv.second->dirty = false;
    setBM(9, 100, 9, AIR, 0); CHECK(!getC(0, 0)->dirty);             // nada mudou => nada refeito

    SECTION("T3 save BL17 / load: round-trip exato de blocos e meta");
    for (int y = 60; y < 64; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) setBM(x, y, z, STONE, (x + z + y) & 15);   // camada com padrao
    for (int i = 0; i < 40; i++) setBM(i * 3 - 20, 90 + (i % 5), i * 2 - 30, WOOL, 255 - i);
    setBM(0, 0, 0, BEDROCK, 77); setBM(15, CH - 1, 15, STONE, 66);   // extremos do chunk (inicio e fim do RLE)
    auto before = snapshot();
    mkSave("t1"); saveCurWorld(0, 80, 0);
    { FILE* f = fopen("saves/t1/chunks16.dat", "rb"); char mg[4] = {}; CHECK(f && fread(mg, 1, 4, f) == 4 && !memcmp(mg, "BL18", 4)); if (f) fclose(f); }
    resetWorldData(); CHECK(W.empty());
    loadSavedChunks("t1"); loadArea(1);
    auto after = snapshot();
    CHECK(before.size() == after.size());
    int diffB = 0, diffM = 0; for (auto& kv : before) { auto it = after.find(kv.first); if (it == after.end()) { diffB++; continue; } for (size_t i = 0; i < N; i++) { diffB += kv.second.b[i] != it->second.b[i]; diffM += kv.second.m[i] != it->second.m[i]; } }
    CHECK(diffB == 0); CHECK(diffM == 0);
    CHECK(getM(0, 0, 0) == 77 && getM(15, CH - 1, 15) == 66 && getB(0, 0, 0) == BEDROCK);
    { long nz = 0; for (size_t i = 0; i < N; i++) nz += getC(0, 0)->m[i] != 0; printf("  (info) chunk 0,0 tem %ld voxels com meta != 0\n", nz); }
    { auto sz = fs::file_size("saves/t1/chunks16.dat"); printf("  (info) chunks16.dat = %ju bytes para %zu chunks\n", (uintmax_t)sz, after.size()); }

    SECTION("T4 compatibilidade: save antigo BL16 (sem meta) continua carregando");
    resetWorldData(); mkSave("old16");
    { FILE* f = fopen("saves/old16/chunks16.dat", "wb"); fwrite("BL16", 1, 4, f); int32_t np = 0, n = 1; fwrite(&np, 4, 1, f); fwrite(&n, 4, 1, f);
      int32_t c[2] = {0, 0}; fwrite(c, 4, 2, f); std::vector<uint16_t> o; size_t left = N; while (left) { size_t r = std::min<size_t>(left, 65535); o.push_back((uint16_t)r); o.push_back(COBBLE); left -= r; }
      int32_t len = (int32_t)o.size(); fwrite(&len, 4, 1, f); fwrite(o.data(), 2, o.size(), f); fclose(f); }
    loadSavedChunks("old16"); gen(0, 0);
    CHECK(getB(5, 50, 5) == COBBLE && getB(15, CH - 1, 15) == COBBLE && getM(5, 50, 5) == 0);
    { bool z = true; for (size_t i = 0; i < N; i++) if (getC(0, 0)->m[i]) z = false; CHECK(z); }

    SECTION("T5 troca de resource pack: bloco do pack some => vira pedra E perde o meta; bloco que mudou de id mantem o meta");
    resetWorldData(); PK.clear(); gPkByFile.clear(); gPkByHash.clear();
    { PkItem a; a.file = "block/foo"; a.cube = true; PK.push_back(a); PkItem b2; b2.file = "block/bar"; b2.cube = true; PK.push_back(b2); }
    gPkByFile["block/foo"] = PK0; gPkByFile["block/bar"] = PK0 + 1;
    gen(0, 0); setBM(2, 70, 2, PK0, 5); setBM(3, 70, 2, PK0 + 1, 6);
    mkSave("t5"); saveCurWorld(0, 80, 0);
    resetWorldData(); PK.clear(); gPkByFile.clear();
    { PkItem b2; b2.file = "block/bar"; b2.cube = true; PK.push_back(b2); PkItem a; a.file = "block/foo"; a.cube = true; PK.push_back(a); }   // pack novo: ordem trocada
    gPkByFile["block/bar"] = PK0; gPkByFile["block/foo"] = PK0 + 1;
    loadSavedChunks("t5"); gen(0, 0);
    CHECK(getB(2, 70, 2) == PK0 + 1 && getM(2, 70, 2) == 5);          // foo agora e PK0+1, meta preservado
    CHECK(getB(3, 70, 2) == PK0 && getM(3, 70, 2) == 6);
    resetWorldData(); PK.clear(); gPkByFile.clear(); { PkItem b2; b2.file = "block/bar"; b2.cube = true; PK.push_back(b2); } gPkByFile["block/bar"] = PK0;   // pack sem "foo"
    loadSavedChunks("t5"); gen(0, 0);
    CHECK(getB(2, 70, 2) == STONE && getM(2, 70, 2) == 0);            // sumiu: pedra sem meta orfao
    CHECK(getB(3, 70, 2) == PK0 && getM(3, 70, 2) == 6);
    resetWorldData(); PK.clear(); gPkByFile.clear();

    SECTION("T6 rede: edicao carrega o meta (serializacao, lote de 80, edicoes pendentes)");
    loadArea(1);
    netOn = false; netSendBlock(10, 70, 10, COBBLE, 6);
    CHECK(netEdits.count(k3(10, 70, 10)) && netEditId(netEdits[k3(10, 70, 10)]) == COBBLE && netEditMeta(netEdits[k3(10, 70, 10)]) == 6);
    netSendBlock(11, 70, 10, COBBLE);                                  // chamada antiga, sem meta => 0
    CHECK(netEditMeta(netEdits[k3(11, 70, 10)]) == 0);
    { NBuf b; netPutEdit(b, {1, 2, 3, COBBLE, 13}); b.r = 0;
      const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(), wid = b.get<int32_t>(), meta = b.get<uint8_t>();
      CHECK(x == 1 && y == 2 && z == 3 && netUnwireId(wid) == COBBLE && meta == 13 && b.r == b.n); }
    { PK.clear(); gPkByFile.clear(); gPkByHash.clear(); PkItem a; a.file = "block/foo"; a.cube = true; PK.push_back(a); gPkByHash[netFnvPk(a.file)] = PK0;
      NBuf b; netPutEdit(b, {-5, 64, 9, PK0, 255}); b.r = 0; b.get<int32_t>(); b.get<int32_t>(); b.get<int32_t>(); const int wid = b.get<int32_t>(); const int meta = b.get<uint8_t>();
      CHECK(netUnwireId(wid) == PK0 && meta == 255); PK.clear(); gPkByHash.clear(); }
    { NBuf b = netMk(NP_BLOCKS); b.put<uint8_t>(80); for (int i = 0; i < 80; i++) netPutEdit(b, {i, 70, i, STONE, i});
      CHECK(b.n == 3 + 80 * 17 && b.n <= (int)sizeof b.d); printf("  (info) lote de 80 edicoes = %d bytes (limite %d)\n", b.n, (int)sizeof b.d); }
    netApplyEdit(12, 70, 12, COBBLE, 9); CHECK(getB(12, 70, 12) == COBBLE && getM(12, 70, 12) == 9);
    netApplyEdit(12, 70, 12, COBBLE, 9); CHECK(getM(12, 70, 12) == 9);   // idempotente
    netApplyEdit(12, 70, 12, COBBLE, 2);  CHECK(getM(12, 70, 12) == 2);  // so o meta mudou
    netApplyEdit(1000000, 70, 1000000, COBBLE, 3);                       // chunk nao carregado => pendente, com meta
    { auto it = netPend.find(k3(1000000, 70, 1000000)); CHECK(it != netPend.end() && netEditId(it->second) == COBBLE && netEditMeta(it->second) == 3); }

    printf("\n%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
