// Fornalha (patchFur.1): receitas, combustivel, fundir 200 ticks, acender/apagar (meta bit2), soltar ao quebrar, salvar/carregar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static int dropTotal() { int t = 0; for (auto& d : DROPS) t += d.n; return t; }
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    PK.resize(8); for (int i = 0; i < 8; i++) { PK[i].file = "block/f" + std::to_string(i); PK[i].name = "F" + std::to_string(i); }
    PK[3].file = "block/furnace"; gPkByFile["block/furnace"] = PK0 + 3; gFurnId = PK0 + 3;
    PK[4].file = "item/stick"; PK[5].file = "block/oak_planks"; PK[6].file = "block/stone_slab"; PK[7].file = "item/iron_ingot";
    // 1) receitas e combustivel
    CHECK(furnResult(IRONORE) == IT_IRON); CHECK(furnResult(GOLDORE) == IT_GOLD); CHECK(furnResult(COBBLE) == STONE); CHECK(furnResult(IT_BEEF) == IT_STEAK); CHECK(furnResult(IT_PORK) == IT_CPORK); CHECK(furnResult(DIRT) == 0); CHECK(furnResult(0) == 0);
    CHECK(furnFuel(IT_COAL) == 1600); CHECK(furnFuel(LOG) == 300); CHECK(furnFuel(PLANKS) == 300); CHECK(furnFuel(COALBLK) == 16000); CHECK(furnFuel(IT_STICK) == 100); CHECK(furnFuel(DIRT) == 0); CHECK(furnFuel(IT_IRON) == 0);
    CHECK(furnFuel(PK0 + 4) == 100); CHECK(furnFuel(PK0 + 5) == 300); CHECK(furnFuel(PK0 + 6) == 0); CHECK(furnFuel(PK0 + 7) == 0);
    // 2) fundir: 1 minerio + 1 carvao => 200 ticks, 1 lingote; carvao gasto 1; acende (meta bit2) e apaga
    setBM(2, AY, 2, gFurnId, 2); FurnData* f = furnAt(2, AY, 2, true); CHECK(f != nullptr); CHECK(furnAt(4, AY, 2, true) == nullptr);
    f->s[0] = {IRONORE, 3, 0}; f->s[1] = {IT_COAL, 1, 0};
    CHECK(!(getM(2, AY, 2) & 4));
    for (int t = 0; t < 199; t++) furnTick();
    CHECK(f->s[2].id == 0 && f->cook == 199); CHECK((getM(2, AY, 2) & 4) != 0); CHECK((getM(2, AY, 2) & 3) == 2);   // frente preservada
    furnTick(); CHECK(f->s[2].id == IT_IRON && f->s[2].n == 1 && f->s[0].n == 2 && f->s[1].id == 0 && f->cook == 0);
    for (int t = 0; t < 400; t++) furnTick(); CHECK(f->s[2].n == 3 && f->s[0].id == 0);   // 1 carvao = 1600 ticks => 3 itens, sobra fogo
    CHECK(f->burn > 0 && (getM(2, AY, 2) & 4) != 0);
    for (int t = 0; t < 1200; t++) furnTick(); CHECK(f->burn == 0); furnTick(); CHECK(!(getM(2, AY, 2) & 4));   // fogo acaba => apaga
    // 3) sem combustivel nao funde; progresso volta 2/tick sem fogo; resultado diferente trava
    f->s[0] = {COBBLE, 1, 0}; for (int t = 0; t < 50; t++) furnTick(); CHECK(f->cook == 0 && f->s[2].id == IT_IRON);
    f->s[1] = {LOG, 2, 0}; for (int t = 0; t < 20; t++) furnTick(); CHECK(f->cook == 0 && f->burn == 0 && f->s[1].n == 2);   // resultado ocupado por lingote: pedra nao cabe
    f->s[2] = Slot(); for (int t = 0; t < 10; t++) furnTick(); CHECK(f->burn > 0 && f->cook > 0 && f->s[1].n == 1);
    for (int t = 0; t < 40; t++) furnTick(); f->s[0] = Slot(); const int c0 = f->cook; for (int t = 0; t < 5; t++) furnTick(); CHECK(f->cook == std::max(0, c0 - 10));   // sem entrada: seta recua 2/tick
    // 4) pilha cheia no resultado trava; slot de resultado so retira
    f->s[0] = {IRONORE, 1, 0}; f->s[2] = {IT_IRON, maxStack(IT_IRON), 0}; CHECK(!furnCanSmelt(*f)); f->s[2].n--; CHECK(furnCanSmelt(*f));
    // 5) shift-click da mochila: fundivel -> entrada, combustivel -> combustivel
    f->s[0] = Slot(); f->s[1] = Slot(); gFurn = gChest = true; gFurnPos[0] = 2; gFurnPos[1] = AY; gFurnPos[2] = 2;
    Slot a = {IRONORE, 5, 0}; furnShift(a); CHECK(f->s[0].id == IRONORE && f->s[0].n == 5 && a.n == 0);
    Slot b = {IT_COAL, 3, 0}; furnShift(b); CHECK(f->s[1].id == IT_COAL && f->s[1].n == 3 && b.n == 0);
    Slot d = {DIRT, 3, 0}; furnShift(d); CHECK(d.n == 3);
    cur = Slot(); f->s[2] = {IT_IRON, 4, 0}; furnTakeOut(0, false); CHECK(cur.id == IT_IRON && cur.n == 4 && f->s[2].id == 0);
    cur = {DIRT, 1, 0}; f->s[2] = {IT_IRON, 4, 0}; furnTakeOut(0, false); CHECK(f->s[2].n == 4 && cur.id == DIRT);   // nao troca item diferente
    CHECK(furnSlotRef(1) == &f->s[1]); CHECK(chestSlotRef(1) == &f->s[1]); CHECK(furnSlotRef(3) == nullptr);
    CHECK(!furnFrame(2.5, AY, 2.5)); CHECK(furnFrame(40, AY, 40));
    furnClosed(); CHECK(!gFurn && !gChest && gChRows == 3); cur = Slot();
    // 6) abrir: convidado de MP nao abre (2), host abre (1), nao-fornalha 0
    { const int ps = netSock; const bool ph = netHost; netHost = true; CHECK(furnOpenAt(2, AY, 2) == 1 && !gChWait); furnClosed(); netSock = ps; netHost = ph; CHECK(furnOpenAt(6, AY, 6) == 0); }
    // 7) salvar/carregar
    f->burn = 123; f->burnMax = 300; f->cook = 77; f->s[0] = {IRONORE, 7, 0}; f->s[1] = Slot(); f->s[2] = {IT_IRON, 2, 0};
    fs::create_directories("saves/_tfurn"); furnSave("_tfurn"); gFurns.clear(); furnLoad("_tfurn");
    f = furnAt(2, AY, 2); CHECK(f != nullptr && f->burn == 123 && f->burnMax == 300 && f->cook == 77 && f->s[0].id == IRONORE && f->s[0].n == 7 && f->s[2].id == IT_IRON && f->s[2].n == 2 && f->s[1].id == 0);
    // 8) quebrar solta tudo; girar/acender nao
    DROPS.clear(); const int tot = f->s[0].n + f->s[1].n + f->s[2].n; setBM(2, AY, 2, gFurnId, 3 | 4); CHECK(dropTotal() == 0 && furnAt(2, AY, 2) != nullptr);
    setBM(2, AY, 2, AIR, 0); CHECK(dropTotal() == tot && gFurns.count(ChestKey(2, AY, 2)) == 0);
    fs::remove_all("saves/_tfurn");
    // 9) multiplayer: pacote do host (com progresso) -> convidado; edicao do convidado -> host (so itens)
    { const int ps = netSock; const bool ph = netHost, pc = netConn;
      setBM(2, AY, 2, gFurnId, 1); FurnData* h = furnAt(2, AY, 2, true); *h = FurnData(); h->s[0] = {IRONORE, 4, 0}; h->s[1] = {IT_COAL, 2, 0}; h->s[2] = {IT_IRON, 1, 0}; h->burn = 800; h->burnMax = 1600; h->cook = 100;
      NBuf pk = furnNetBuild(2, AY, 2, *h, true); CHECK(pk.n > 100 && pk.n < 1400);
      netSock = 5; netHost = false; netConn = true;   // convidado
      gFurns.clear(); furnAt(2, AY, 2, true); NBuf r = pk; r.get<uint8_t>(); r.get<uint8_t>(); chestNetRecv(r); FurnData* g = furnAt(2, AY, 2);
      CHECK(g && g->s[0].id == 0);   // sem tela aberta: pacote ignorado
      gFurns.clear(); CHECK(furnOpenAt(2, AY, 2) == 1 && gFurn && gChWait && furnSlotRef(0) != nullptr); r = pk; r.get<uint8_t>(); r.get<uint8_t>(); chestNetRecv(r); g = furnAt(2, AY, 2);
      CHECK(!gChWait && g->s[0].id == IRONORE && g->s[0].n == 4 && g->s[1].id == IT_COAL && g->s[2].id == IT_IRON && g->burn > 0 && g->burnMax == 255 && g->cook >= 98 && g->cook <= 100);
      float fl, ar; CHECK(furnProgress(&fl, &ar) && fl > .45f && fl < .55f && ar > .45f && ar < .55f);
      g->s[0].n = 3; { NBuf e = furnNetBuild(2, AY, 2, *g, false); netSock = 5; netHost = true; gFurns.clear(); FurnData* hh = furnAt(2, AY, 2, true); hh->burn = 50; hh->s[0] = {IRONORE, 4, 0}; NBuf q = e; q.get<uint8_t>(); q.get<uint8_t>(); chestNetRecv(q);
        CHECK(hh->s[0].n == 3 && hh->s[1].id == IT_COAL && hh->burn == 50 && gFurnWatch.count(ckey(2, AY, 2)) == 1); }   // host: itens do convidado, progresso do host
      netHost = false; gFurns[ckey(2, AY, 2)] = FurnData(); gFurnSnap = FurnData(); gFurnReq = SDL_GetTicks(); furnClosed(); CHECK(!gFurn && gFurns.count(ckey(2, AY, 2)) == 0);   // convidado nao guarda
      netSock = ps; netHost = ph; netConn = pc; }
    {   // patchFur.3: XP ao retirar (host/offline: guardado; shift e clique parcial proporcionais)
        const bool cr = creative; creative = false; gXpTot = 0; srand(1);
        FurnData* x = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], true); (void)x;
        CHECK(furnXp(IT_IRON) > .69f && furnXp(IT_IRON) < .71f && furnXp(IT_DIAMOND) == 1.0f && furnXp(IT_STEAK) > .34f && furnXp(IT_COAL) < .11f);
        gFurn = true; FurnData* g = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], true); if (g) { *g = FurnData(); g->s[2] = {IT_DIAMOND, 4, 0}; g->xp = 4.0f; cur = Slot(); furnTakeOut(0, false);
            CHECK(gXpTot == 4 && g->xp == 0.f && cur.n == 4); }
        creative = true; gXpTot = 0; if (g) { g->s[2] = {IT_DIAMOND, 2, 0}; g->xp = 2.f; cur = Slot(); furnTakeOut(0, false); CHECK(gXpTot == 0); }   // criativo nao ganha XP
        creative = cr; }
    printf("test_furnace: %d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
