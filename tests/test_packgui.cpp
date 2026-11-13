// patchTex.2: recortes do GUI do pack Default-Java-1.12.2 (widgets, containers)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    CHECK(gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip"));
    std::vector<uint8_t> o;
    CHECK(packGuiCrop("gui/widgets.png", 0, 0, 182, 22, o) && o.size() == 182 * 22 * 4);   // hotbar
    bool any = false; for (size_t i = 3; i < o.size(); i += 4) any |= o[i] != 0; CHECK(any);
    CHECK(packGuiCrop("gui/widgets.png", 0, 66, 200, 20, o) && o.size() == 200 * 20 * 4);   // botao normal
    CHECK(packGuiCrop("gui/widgets.png", 0, 86, 200, 20, o) && packGuiCrop("gui/widgets.png", 0, 46, 200, 20, o));
    CHECK(packGuiCrop("gui/container/inventory.png", 0, 0, 176, 166, o));
    CHECK(packGuiCrop("gui/container/crafting_table.png", 0, 0, 176, 166, o));
    CHECK(packGuiCrop("gui/container/creative_inventory/tab_items.png", 232, 0, 12, 15, o));
    for (int k = 0; k < 2; k++) for (int sl = 0; sl < 2; sl++) for (int c = 0; c < 7; c++)   // abas do criativo 28x32 (4 linhas x 7 colunas)
        CHECK(packGuiCrop("gui/container/creative_inventory/tabs.png", c * 28, (k ? 64 : 0) + (sl ? 32 : 0), 28, 32, o) && o.size() == 28 * 32 * 4);
    CHECK(packGuiCrop("gui/container/inventory.png", 7, 141, 18, 18, o) && o.size() == 18 * 18 * 4);   // slot: miolo cinza claro (139), nao escuro
    CHECK(o[(9 * 18 + 9) * 4] == 139 && o[(9 * 18 + 9) * 4 + 3] == 255);
    CHECK(!packGuiCrop("gui/widgets.png", 250, 250, 20, 20, o));    // fora da imagem
    CHECK(!packGuiCrop("gui/nao_existe.png", 0, 0, 4, 4, o));
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
