// Bau (Parte 1): armazenamento por posicao, soltar ao quebrar, salvar/carregar. Pack real so para o item do pack (sem pack => parte SKIP).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static int dropTotal() { int t = 0; for (auto& d : DROPS) t += d.n; return t; }
int main() {
    if (chdir("..") != 0) return 99;
    const bool pack = fs::exists("assets/resourcepacks/Ashen_16x.zip");
    if (pack) { applyMcPack(); loadPackItems(); xbInit(); loadCreativeCfg(); pkApplyItemData(); rsNames(); ptbrApply(); }
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    // 1) so onde ha bau: bau normal (tipo 0) e armadilhado (1) guardam; Ender (2) e shulker (3) ainda nao; ar nao
    setBM(2, AY, 2, XB_CHEST, (0 << 2) | 2); setBM(4, AY, 2, XB_CHEST, (1 << 2) | 0); setBM(6, AY, 2, XB_CHEST, (2 << 2) | 0); setBM(8, AY, 2, XB_CHEST, (3 << 2) | 0);
    CHECK(chestAt(2, AY, 2) == nullptr);                       // sem create: nao inventa conteudo
    ChestInv* c = chestAt(2, AY, 2, true); CHECK(c != nullptr);
    CHECK(chestAt(4, AY, 2, true) != nullptr); CHECK(chestAt(6, AY, 2, true) == nullptr); CHECK(chestAt(8, AY, 2, true) != nullptr);   /* patchOpt.1: shulker guarda como o bau */ CHECK(chestAt(10, AY, 2, true) == nullptr);
    CHECK(chestAt(2, AY, 2) == c);                              // mesmo ponteiro/conteudo
    CHECK(c->empty());
    // 2) chestAdd: empilha nos iguais, depois vazios; respeita maxStack; devolve o resto
    const int ms = maxStack(STONE);
    CHECK(chestAdd(*c, STONE, ms + 10) == 0); CHECK(c->s[0].id == STONE && c->s[0].n == ms && c->s[1].n == 10);
    CHECK(chestAdd(*c, STONE, 5) == 0); CHECK(c->s[1].n == 15 && c->s[2].id == 0);
    CHECK(chestAdd(*c, DIRT, 3) == 0); CHECK(c->s[2].id == DIRT && c->s[2].n == 3);
    ChestInv full; for (int i = 0; i < CHEST_SLOTS; i++) { full.s[i].id = STONE; full.s[i].n = ms; } CHECK(chestAdd(full, STONE, 7) == 7); CHECK(chestAdd(full, DIRT, 7) == 7);
    CHECK(chestAdd(*c, 0, 4) == 4);                             // id 0 nunca entra
    CHECK(chestCount(*c) == ms + 15 + 3 && !c->empty());
    // 3) girar/abrir muda so o meta => NAO solta nada e mantem o conteudo
    DROPS.clear(); setBM(2, AY, 2, XB_CHEST, (0 << 2) | 3); CHECK(dropTotal() == 0); CHECK(chestAt(2, AY, 2) && chestCount(*chestAt(2, AY, 2)) == ms + 18);
    // 4) quebrar (ID muda): solta tudo, esquece o mapa; total solto = total guardado
    setBM(2, AY, 2, AIR, 0); CHECK(dropTotal() == ms + 18); CHECK(gChests.count(ChestKey(2, AY, 2)) == 0);
    { const size_t n0 = DROPS.size(); setBM(2, AY, 2, XB_CHEST, 2); CHECK(chestAt(2, AY, 2) == nullptr); setBM(2, AY, 2, AIR, 0); CHECK(DROPS.size() == n0); }   // bau novo no mesmo lugar nasce vazio e quebrar vazio nao solta nada
    // 5) convidado de multiplayer nao solta (o host solta ao receber a edicao); host/local sim
    { ChestInv* a = chestAt(4, AY, 2); CHECK(a != nullptr); a->s[0].id = DIRT; a->s[0].n = 9; const int ps = netSock; const bool ph = netHost; DROPS.clear();
      netSock = 5; netHost = false; setBM(4, AY, 2, AIR, 0); CHECK(dropTotal() == 0); CHECK(gChests.count(ChestKey(4, AY, 2)) == 0); netSock = ps; netHost = ph; }
    // 6) salvar/carregar: ids core por numero, item do pack por nome do arquivo, slots vazios/baus vazios nao entram, posicoes negativas ok
    setBM(-3, AY, -2, XB_CHEST, 2); setBM(12, AY, 12, XB_CHEST, 6);
    ChestInv* n = chestAt(-3, AY, -2, true); n->s[0].id = STONE; n->s[0].n = 5; n->s[26].id = DIRT; n->s[26].n = ms; int pkItem = 0; if (pack && PK.size() > 10) { pkItem = PK0 + 5; n->s[13].id = pkItem; n->s[13].n = 2; }
    chestAt(12, AY, 12, true);   // vazio: nao deve ser salvo
    fs::create_directories("saves/_tchest"); chestsSave("_tchest");
    std::string txt; { FILE* f = fopen("saves/_tchest/chests.dat", "rb"); CHECK(f != nullptr); if (f) { char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, f)) > 0) txt.append(b, r); fclose(f); } }
    CHECK(txt.find("c -3 " + std::to_string(AY) + " -2") != std::string::npos); CHECK(txt.find(" 12 ") == std::string::npos);
    chestsClear(); CHECK(gChests.empty()); chestsLoad("_tchest");
    { ChestInv* m = chestAt(-3, AY, -2); CHECK(m != nullptr); if (m) { CHECK(m->s[0].id == STONE && m->s[0].n == 5); CHECK(m->s[26].id == DIRT && m->s[26].n == ms); CHECK(m->s[1].id == 0);
        if (pkItem) { CHECK(m->s[13].id == pkItem && m->s[13].n == 2); CHECK(txt.find("@" + PK[pkItem - PK0].file) != std::string::npos); } } }
    CHECK(chestAt(12, AY, 12) == nullptr);
    // 7) arquivo ruim/ausente nao quebra; salvar sem baus apaga o arquivo
    chestsClear(); chestsSave("_tchest"); CHECK(!fs::exists("saves/_tchest/chests.dat")); chestsLoad("_tchest"); CHECK(gChests.empty());
    { FILE* f = fopen("saves/_tchest/chests.dat", "wb"); fputs("CHESTS1\nlixo\nc 1 2\ns 99 5 #1\nc 0 0 0\ns 0 5 #1\ns 1 -3 #1\ns 2 5 @nao/existe\ns 3 1 #999999\n", f); fclose(f); }
    chestsLoad("_tchest"); CHECK(gChests.size() == 1); CHECK(gChests.count(ChestKey(0, 0, 0)) == 1); { ChestInv& g = gChests[ChestKey(0, 0, 0)]; CHECK(g.s[0].id == 1 && g.s[0].n == 5); CHECK(g.s[1].id == 0 && g.s[2].id == 0 && g.s[3].id == 0); }
    fs::remove_all("saves/_tchest");
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
