// patchAud.2: material por bloco + eventos de passo/quebrar/colocar/dano/comer em events.cfg (rodar da RAIZ do jogo, com assets/sounds completo)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    CHECK(sndMatOf(STONE) == 0 && sndMatOf(GRASS) == 1 && sndMatOf(DIRT) == 1 && sndMatOf(GRAVEL) == 2 && sndMatOf(SAND) == 3 && sndMatOf(LOG) == 4 && sndMatOf(PLANKS) == 4 && sndMatOf(WOOL) == 5 && sndMatOf(SNOWBLK) == 6);
    CHECK(!strcmp(sndBlockEvt(SAND, false), "block.break.sand") && !strcmp(sndBlockEvt(LOG, true), "block.place.wood") && !strcmp(sndBlockEvt(COBBLE, true), "block.place"));
    for (int m = 0; m < 8; m++) { CHECK(sndEvtGet(SND_BRK[m]) != nullptr); CHECK(sndEvtGet(SND_PLC[m]) != nullptr); CHECK(sndEvtGet(SND_STP[m]) != nullptr); }
    for (const char* e : {"player.hurt", "player.fall.small", "player.fall.big", "eat", "burp", "ui.click", "explode", "tnt.fuse", "door.open", "door.close", "chest.open", "chest.close", "button.click", "button.click.wood", "lever.click", "fire.ignite", "fire.ambient", "lava.ambient", "lava.pop", "splash"}) CHECK(sndEvtGet(e) != nullptr);
    CHECK(sndEvtGet("step.wood")->f.size() >= 4 && sndEvtGet("player.hurt")->f.size() == 3);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
