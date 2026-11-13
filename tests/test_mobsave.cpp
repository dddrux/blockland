// patchBreed.3: mobs.dat guarda filhote/idade/amor/recarga/tosquia; vaca-cogumelo aceita trigo. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    std::error_code ec; fs::create_directories("/tmp/bl_ms/saves/t", ec); if (chdir("/tmp/bl_ms") != 0) return 2;
    MOBS.clear(); BOATS.clear();
    MobEnt a; a.type = M_COW; a.x = 3; a.y = 70; a.z = 4; a.hp = 5; a.baby = true; a.babyAge = 1234; a.love = 300; a.breedCd = 777; MOBS.push_back(a);
    MobEnt b; b.type = M_SHEEP; b.x = 1; b.y = 70; b.z = 2; b.hp = 8; b.sheared = true; MOBS.push_back(b);
    MobEnt c; c.type = M_MOOSH; c.x = 9; c.y = 70; c.z = 9; c.hp = 10; c.baby = true; MOBS.push_back(c);
    boatsSave("t"); MOBS.clear(); mobsLoadFile("t");
    CHECK(MOBS.size() == 3);
    if (MOBS.size() == 3) { CHECK(MOBS[0].baby && MOBS[0].babyAge == 1234 && MOBS[0].love == 300 && MOBS[0].breedCd == 777 && MOBS[0].hp == 5);
        CHECK(!MOBS[1].baby && MOBS[1].sheared); CHECK(MOBS[2].baby && MOBS[2].type == M_MOOSH); }
    // arquivo antigo (7 campos): continua carregando, adulto
    { FILE* f = fopen("saves/t/mobs.dat", "wb"); fprintf(f, "MOBS1\nm %d 1 70 1 0 4 0\n", M_PIG); fclose(f); }
    MOBS.clear(); mobsLoadFile("t"); CHECK(MOBS.size() == 1 && !MOBS[0].baby && MOBS[0].love == 0);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
