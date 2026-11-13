// patchTick.1: ordem dos ticks agendados (due -> prioridade -> ordem do pedido; 1o pedido da celula vence). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static std::vector<int> gOrd;
static void rec(int x, int y, int z) { (void)y; (void)z; gOrd.push_back(x); }
static const int AY = 200;
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true;
    for (int x = 0; x < 16; x++) setB(x, AY, 0, STONE);
    gNoBlockHooks = false;
}
static void fresh() { gTicks.clear(); gTickSet.clear(); gTickSeq = 0; gOrd.clear(); }
int main() {
    arena(); BlockHooks h; h.tick = rec; blockHooksSet(STONE, h);
    // pedido de delay 4 feito ANTES de um de delay 1: quando os dois ja venceram, o mais cedo roda primeiro
    fresh(); scheduleTick(1, AY, 0, 4); scheduleTick(2, AY, 0, 1);
    for (int i = 0; i < 4; i++) runWorldTick();
    CHECK(gOrd.size() == 2 && gOrd[0] == 2 && gOrd[1] == 1);
    // mesmo due: prioridade menor roda antes, mesmo agendada depois
    fresh(); scheduleTick(3, AY, 0, 2, 0); scheduleTick(4, AY, 0, 2, -2); scheduleTick(5, AY, 0, 2, 3);
    for (int i = 0; i < 3; i++) runWorldTick();
    CHECK(gOrd.size() == 3 && gOrd[0] == 4 && gOrd[1] == 3 && gOrd[2] == 5);
    // mesmo due e prioridade: ordem do pedido
    fresh(); scheduleTick(8, AY, 0, 1); scheduleTick(6, AY, 0, 1); scheduleTick(7, AY, 0, 1);
    runWorldTick(); runWorldTick();
    CHECK(gOrd.size() == 3 && gOrd[0] == 8 && gOrd[1] == 6 && gOrd[2] == 7);
    // 1o pedido da celula vence: o duplicado mais cedo/prioritario e ignorado
    fresh(); scheduleTick(9, AY, 0, 5); scheduleTick(9, AY, 0, 1, -3);
    CHECK(gTicks.size() == 1 && gTicks[0].due == gWorldTick + 5 && gTicks[0].prio == 0 && tickPending(9, AY, 0));
    // nada roda antes da hora
    fresh(); scheduleTick(10, AY, 0, 3); runWorldTick(); runWorldTick(); CHECK(gOrd.empty()); runWorldTick(); runWorldTick(); CHECK(gOrd.size() == 1 && gOrd[0] == 10 && !tickPending(10, AY, 0));
    // tick pedido dentro do callback so roda na proxima coleta (delay minimo 1)
    static bool again; again = true; BlockHooks h2; h2.tick = [](int x, int y, int z) { gOrd.push_back(x); if (again) { again = false; scheduleTick(x + 1, y, z, 0); } }; blockHooksSet(STONE, h2);
    fresh(); scheduleTick(11, AY, 0, 1); runWorldTick(); CHECK(gOrd.size() == 1 && gOrd[0] == 11); runWorldTick(); CHECK(gOrd.size() == 2 && gOrd[1] == 12);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
