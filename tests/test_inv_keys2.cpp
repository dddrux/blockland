// Q na paleta do criativo e shift+duplo clique (patch5.15).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static void clr() { for (Slot& s : inv) s = Slot(); for (Slot& s : armor) s = Slot(); offh = Slot(); cur = Slot(); throwQ.clear(); gChest = false; }
static bool findPx(int target, int w, int h, int& X, int& Y) { for (int y = 0; y < h; y += 2) for (int x = 0; x < w; x += 2) if (hitSlot(w, h, x, y) == target) { X = x; Y = y; return true; } return false; }
int main() {
    if (chdir("..") != 0) return 99;
    clr(); creative = true; const int it = palItem(SL_PAL);
    CHECK(it != 0);
    invDropKey(SL_PAL, false); CHECK(throwQ.size() == 1 && throwQ[0].id == it && throwQ[0].n == 1);          // Q na paleta: 1
    invDropKey(SL_PAL, true); CHECK(throwQ.size() == 2 && throwQ[1].n == maxStack(it));                      // Ctrl+Q: pilha
    creative = false; invDropKey(SL_PAL, false); CHECK(throwQ.size() == 2);                                  // fora do criativo: nada
    clr(); inv[1] = {STONE, 4}; inv[20] = {STONE, 5}; inv[22] = {DIRT, 3};
    int X = 0, Y = 0, w = 854, h = 480; CHECK(findPx(1, w, h, X, Y));
    invMouseDown(SDL_BUTTON_LEFT, w, h, X, Y, true, 1000);                                                   // 1o shift-clique: hotbar -> mochila
    CHECK(!inv[1].id && inv[20].n == 9);
    invMouseDown(SDL_BUTTON_LEFT, w, h, X, Y, true, 1100);                                                   // shift+duplo: todas as pedras do inventario
    CHECK(!inv[20].id && inv[22].id == DIRT);                                                                // as pedras voltaram para a hotbar, terra ficou
    int s = 0; for (int i = 0; i < 9; i++) if (inv[i].id == STONE) s += inv[i].n; CHECK(s == 9);
    clr(); inv[1] = {STONE, 4}; inv[20] = {STONE, 5};
    invMouseDown(SDL_BUTTON_LEFT, w, h, X, Y, true, 1000); invMouseDown(SDL_BUTTON_LEFT, w, h, X, Y, true, 1500);   // lento: nao e duplo
    CHECK(!inv[20].id == false);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
