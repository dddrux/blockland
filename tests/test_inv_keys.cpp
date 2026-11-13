// Atalhos do inventario: Q/Ctrl+Q/F sobre o slot, tecla 1-9 so com cursor vazio.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static void clr() { for (Slot& s : inv) s = Slot(); for (Slot& s : armor) s = Slot(); offh = Slot(); cur = Slot(); throwQ.clear(); gChest = false; }
int main() {
    if (chdir("..") != 0) return 99;
    clr(); inv[12] = {STONE, 10};
    invDropKey(12, false); CHECK(inv[12].n == 9 && throwQ.size() == 1 && throwQ[0].n == 1);        // Q: 1
    invDropKey(12, true); CHECK(!inv[12].id && throwQ.size() == 2 && throwQ[1].n == 9);            // Ctrl+Q: resto
    invDropKey(12, false); CHECK(throwQ.size() == 2);                                              // slot vazio
    inv[3] = {STONE, 5}; cur = {DIRT, 2}; invDropKey(3, true); CHECK(inv[3].n == 5 && throwQ.size() == 2);   // cursor cheio: nada
    clr(); inv[3] = {STONE, 5}; offh = {DIRT, 7};
    invOffhandKey(3); CHECK(inv[3].id == DIRT && inv[3].n == 7 && offh.id == STONE && offh.n == 5);   // F troca
    invOffhandKey(SL_OFF); CHECK(offh.id == STONE);                                                // sobre a propria offhand: nada
    clr(); offh = {STONE, 1}; invOffhandKey(SL_ARMOR); CHECK(!armor[0].id && offh.id == STONE);    // pedra nao entra no capacete
    cur = {DIRT, 1}; inv[0] = {STONE, 1}; invNumberKey(5, 0); CHECK(inv[0].id == STONE && !inv[5].id);   // 1-9 com cursor cheio: nada
    cur = Slot(); invNumberKey(5, 0); CHECK(inv[5].id == STONE && !inv[0].id);                     // 1-9 com cursor vazio: troca
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
