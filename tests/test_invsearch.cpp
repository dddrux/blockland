// patchInv.1: busca do inventario aceita UTF-8 (c cedilha), apaga caractere inteiro, Ctrl+A/Backspace, e 1-9 sobre slot nao digita.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    creative = true; gTable = false; gChest = false; ctab = TAB_SEARCH; searchQ.clear();
    CHECK(invSearching());
    invSearchText("Ma\xC3\xA7\xC3\xA3"); CHECK(searchQ == "ma\xC3\xA7\xC3\xA3");
    invSearchBack(); CHECK(searchQ == "ma\xC3\xA7");
    invSearchBack(); CHECK(searchQ == "ma");
    gSrchSel = true; invSearchText("x"); CHECK(searchQ == "x" && !gSrchSel);
    gSrchSel = true; invSearchBack(); CHECK(searchQ.empty());
    gSrchEat = '1'; invSearchText("1"); CHECK(searchQ.empty() && gSrchEat == 0);
    invSearchText("1"); CHECK(searchQ == "1");
    printf("%d/%d ok\n", checks - fails, checks); return fails != 0;
}
