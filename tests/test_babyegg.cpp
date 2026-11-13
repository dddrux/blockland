// patchBabyEgg.1: ovo de spawn clicado num mob da mesma especie = filhote (eggBabyUse). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;   // piso de pedra em y=199, ar acima
static void arena() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) for (int y = AY - 1; y < AY + 16; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
}
static void put(int type, double x, double z, bool baby = false) { MobEnt e; e.type = type; e.x = x; e.y = AY; e.z = z; e.baby = baby; MOBS.push_back(e); }
static Slot eggOf(const char* mob, int n) { PkItem it; it.file = std::string("item/") + mob + "_spawn_egg"; it.name = mob; PK.push_back(it); Slot s; s.id = PK0 + (int)PK.size() - 1; s.n = n; return s; }
int main() {
    arena();
    Slot cow = eggOf("cow", 3), pig = eggOf("pig", 3), zom = eggOf("zombie", 3), rab = eggOf("rabbit", 2), he = eggOf("horse", 1), we = eggOf("wolf", 1), ve = eggOf("villager", 1);
    const float eye[3] = {0.f, AY + .5f, -2.f}, dir[3] = {0.f, 0.f, 1.f}, off[3] = {5.f, AY + .5f, -2.f};
    CHECK(eggMob(cow.id) == M_COW && eggMob(pig.id) == M_PIG && eggMob(zom.id) == M_ZOMBIE && eggMob(rab.id) == M_RABBIT);
    // vaca adulta + ovo de vaca: nasce 1 filhote da mesma especie na posicao dela; sobrevivencia gasta 1 ovo
    MOBS.clear(); put(M_COW, 0, 0);
    CHECK(eggBabyUse(cow, eye, dir, false)); CHECK(MOBS.size() == 2 && MOBS[1].baby && MOBS[1].type == M_COW && MOBS[1].x == 0 && MOBS[1].z == 0 && !MOBS[0].baby && cow.n == 2);
    // criativo: nao gasta
    CHECK(eggBabyUse(cow, eye, dir, true) && cow.n == 2 && MOBS.size() == 3);
    // clicar num filhote tambem gera filhote (EntityAgeable nao olha a idade)
    MOBS.clear(); put(M_COW, 0, 0, true); CHECK(eggBabyUse(cow, eye, dir, true) && MOBS.size() == 2 && MOBS[1].baby);
    // outra especie: nao faz nada (cai no fluxo normal do ovo)
    MOBS.clear(); put(M_COW, 0, 0); const int n0 = pig.n; CHECK(!eggBabyUse(pig, eye, dir, false) && MOBS.size() == 1 && pig.n == n0);
    // especie sem filhote (zumbi): nao
    MOBS.clear(); put(M_ZOMBIE, 0, 0); CHECK(!eggBabyUse(zom, eye, dir, false) && MOBS.size() == 1);
    // mira fora do mob: nao
    MOBS.clear(); put(M_COW, 0, 0); CHECK(!eggBabyUse(cow, off, dir, false) && MOBS.size() == 1);
    // morto nao conta
    MOBS.clear(); put(M_COW, 0, 0); MOBS[0].dead = 1; CHECK(!eggBabyUse(cow, eye, dir, false) && MOBS.size() == 1);
    // bloco entre o jogador e o mob: nao
    MOBS.clear(); put(M_COW, 0, 0); setB(0, AY, -1, STONE); CHECK(!eggBabyUse(cow, eye, dir, false) && MOBS.size() == 1); setB(0, AY, -1, AIR);
    // item que nao e ovo
    MOBS.clear(); put(M_COW, 0, 0); Slot nada; nada.id = IT_EGG; nada.n = 1; CHECK(!eggBabyUse(nada, eye, dir, false) && MOBS.size() == 1 && nada.n == 1);
    // coelho: filhote mantem a pelagem do coelho clicado; ultimo ovo some do slot
    MOBS.clear(); put(M_RWHITE, 0, 0); rab.n = 1; CHECK(eggBabyUse(rab, eye, dir, false) && MOBS.size() == 2 && MOBS[1].baby && MOBS[1].type == M_RWHITE && rab.id == 0);
    // P150: filhotes de cavalo/burro/mula/lhama/lobo/gato/aldeao; zumbi/esqueleto-cavalo e hostis continuam sem filhote
    CHECK(mobCanBaby(M_HORSE) && mobCanBaby(M_DONKEY) && mobCanBaby(M_MULE) && mobCanBaby(M_LLAMA) && mobCanBaby(M_LLAMAW) && mobCanBaby(M_WOLF) && mobCanBaby(M_OCELOT) && mobCanBaby(M_CATRED) && mobCanBaby(M_VILLAGER) && mobCanBaby(M_VSMITH));
    CHECK(!mobCanBaby(M_ZHORSE) && !mobCanBaby(M_SHORSE) && !mobCanBaby(M_ZOMBIE) && !mobCanBaby(M_CREEPER) && !mobCanBaby(M_IGOLEM) && !mobCanBaby(M_RKILLER));
    {
      MOBS.clear(); put(M_HORSE, 0, 0); CHECK(eggBabyUse(he, eye, dir, false) && MOBS.size() == 2 && MOBS[1].baby && MOBS[1].type == M_HORSE && !MOBS[0].baby);
      MOBS.clear(); put(M_WOLF, 0, 0); CHECK(eggBabyUse(we, eye, dir, false) && MOBS.size() == 2 && MOBS[1].baby && MOBS[1].type == M_WOLF);
      MOBS.clear(); put(M_VILLAGER, 0, 0); CHECK(eggBabyUse(ve, eye, dir, false) && MOBS.size() == 2 && MOBS[1].baby && MOBS[1].type == M_VILLAGER);
      MOBS.clear(); put(M_COW, 0, 0); he.n = 1; CHECK(!eggBabyUse(he, eye, dir, false) && MOBS.size() == 1); }   // ovo de cavalo em vaca: nada
    CHECK(mobSameKind(M_HORSE, M_HGRAY) && mobSameKind(M_LLAMA, M_LLAMAW) && mobSameKind(M_OCELOT, M_CATRED) && mobSameKind(M_VILLAGER, M_VLIB) && !mobSameKind(M_HORSE, M_DONKEY) && !mobSameKind(M_HORSE, M_LLAMA) && !mobSameKind(M_WOLF, M_OCELOT) && !mobSameKind(M_RWHITE, M_RKILLER) && mobSameKind(M_RWHITE, M_RGOLD));   // P150
    printf("babyegg: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
