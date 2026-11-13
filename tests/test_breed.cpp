// patchBreed.1: reproducao de animais (breedTick). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static void put(int type, double x, double z, int love, int cd = 0, bool baby = false) { MobEnt e; e.type = type; e.x = x; e.y = 70; e.z = z; e.love = love; e.breedCd = cd; e.baby = baby; MOBS.push_back(e); }
int main() {
    // par perto, ambos em amor: nasce 1 filhote, recarga 6000, amor zerado
    MOBS.clear(); put(M_COW, 0, 0, 600); put(M_COW, 1, 0, 600); breedTick();
    CHECK(MOBS.size() == 3 && MOBS[2].baby && MOBS[2].type == M_COW && MOBS[0].love == 0 && MOBS[1].love == 0 && MOBS[0].breedCd == 6000 && MOBS[1].breedCd == 6000);
    breedTick(); CHECK(MOBS.size() == 3);   // sem amor: nada nasce de novo
    // especies diferentes: nao nasce
    MOBS.clear(); put(M_COW, 0, 0, 600); put(M_PIG, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 2);
    // so um em amor: nao nasce
    MOBS.clear(); put(M_SHEEP, 0, 0, 600); put(M_SHEEP, 1, 0, 0); breedTick(); CHECK(MOBS.size() == 2);
    // longe (6 blocos): anda ate o parceiro, nao nasce ainda
    MOBS.clear(); put(M_PIG, 0, 0, 600); put(M_PIG, 6, 0, 600); breedTick(); CHECK(MOBS.size() == 2 && MOBS[0].hasT && MOBS[0].tx == 6 && MOBS[1].hasT && MOBS[1].tx == 0);
    // muito longe (>8): ignora
    MOBS.clear(); put(M_PIG, 0, 0, 600); put(M_PIG, 12, 0, 600); breedTick(); CHECK(!MOBS[0].hasT && MOBS.size() == 2);
    // filhote nao entra no amor
    MOBS.clear(); put(M_CHICKEN, 0, 0, 600, 0, true); put(M_CHICKEN, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 2);
    // amor e recarga decaem 1 por tick
    MOBS.clear(); put(M_COW, 0, 0, 5, 3); breedTick(); CHECK(MOBS[0].love == 4 && MOBS[0].breedCd == 2);
    // morto nao conta
    MOBS.clear(); put(M_COW, 0, 0, 600); put(M_COW, 1, 0, 600); MOBS[1].dead = 1; breedTick(); CHECK(MOBS.size() == 2);
    // filhote segue o adulto (a cada 10 ticks): 6 blocos => anda; colado (<3) => fica; outra especie => ignora
    MOBS.clear(); put(M_COW, 0, 0, 0, 0, true); put(M_COW, 6, 0, 0); for (int i = 0; i < 10; i++) breedTick(); CHECK(MOBS[0].hasT && MOBS[0].tx == 6);
    MOBS.clear(); put(M_COW, 0, 0, 0, 0, true); put(M_COW, 2, 0, 0); for (int i = 0; i < 10; i++) breedTick(); CHECK(!MOBS[0].hasT);
    MOBS.clear(); put(M_COW, 0, 0, 0, 0, true); put(M_PIG, 6, 0, 0); for (int i = 0; i < 10; i++) breedTick(); CHECK(!MOBS[0].hasT);
    // patchBaby.3: coelho tambem se reproduz (pelagens diferentes cruzam; o assassino nao)
    MOBS.clear(); put(M_RABBIT, 0, 0, 600); put(M_RABBIT, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 3 && MOBS[2].baby && MOBS[2].type == M_RABBIT);
    MOBS.clear(); put(M_RABBIT, 0, 0, 600); put(M_RWHITE, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 3 && MOBS[2].baby && isRabbit(MOBS[2].type));
    MOBS.clear(); put(M_RABBIT, 0, 0, 600); put(M_RKILLER, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 2);
    MOBS.clear(); put(M_RABBIT, 0, 0, 600); put(M_PIG, 1, 0, 600); breedTick(); CHECK(MOBS.size() == 2);
    CHECK(mobCanBaby(M_RABBIT) && mobCanBaby(M_RGOLD) && !mobCanBaby(M_RKILLER) && !mobCanBaby(M_ZOMBIE) && mobCanBaby(M_COW));
    spawnMob(M_RABBIT, 0, 70, 0, 0.f, true); CHECK(MOBS.back().baby); spawnMob(M_ZOMBIE, 0, 70, 0, 0.f, true); CHECK(!MOBS.back().baby);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
