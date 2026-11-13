// Inventario/armadura/vida salvos em saves/mundo/player.dat e restaurados ao reentrar.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint64 SDL_GetPerformanceCounter(void) { return 123456789ULL; }
Uint32 SDL_GetTicks() { return 1000; }   // randomSeed() (ensureWorld) usa; sem isso o link com simbolos ignorados vira chamada nula (segfault antigo)
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    chdir("/tmp"); system("rm -rf /tmp/saves"); ensureWorld(); CHECK(fs::exists("saves/mundo/level.cfg")); gCurDir = "mundo";
    PK.clear(); gPkByFile.clear(); PK.push_back(PkItem{"block/granite", "Granito", "", true});  gPkByFile["block/granite"] = PK0;
    for (int i = 0; i < 36; i++) inv[i] = Slot(); for (auto& a : armor) a = Slot();
    inv[0] = {STONE, 64}; inv[7] = {PK0, 12}; inv[20] = {IT_APPLE, 3}; armor[1] = {IT_A0 + 1, 1}; offh = {DIRT, 5}; cur = {SAND, 9}; gHP = 13; gFood = 7;
    savePlayer();
    for (int i = 0; i < 36; i++) inv[i] = Slot(); for (auto& a : armor) a = Slot(); offh = Slot(); cur = Slot(); gHP = 20; gFood = 20;
    CHECK(loadPlayer());
    CHECK(inv[0].id == STONE && inv[0].n == 64); CHECK(inv[7].id == PK0 && inv[7].n == 12); CHECK(inv[20].id == IT_APPLE && inv[20].n == 3);
    CHECK(armor[1].id == IT_A0 + 1); CHECK(offh.id == DIRT && offh.n == 5); CHECK(cur.id == SAND && cur.n == 9); CHECK(gHP == 13 && gFood == 7); CHECK(inv[1].id == 0);
    // convidado nao grava chunks nem level.cfg do host
    netSock = 5; netHost = false; CHECK(playerFile().find("player_guest.dat") != std::string::npos);
    struct stat0 {}; WORLD_SEED = 42; saveCurWorld(0, 0, 0); CHECK(!fs::exists("saves/mundo/chunks16.dat")); CHECK(fs::exists("saves/mundo/player_guest.dat"));
    netSock = -1; netHost = false; saveCurWorld(1, 2, 3); CHECK(fs::exists("saves/mundo/chunks16.dat") && fs::exists("saves/mundo/level.cfg"));
    printf("%s (%d/%d)\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails != 0;
}
