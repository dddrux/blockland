// test_food112: comida/efeitos conforme 1.12.2 (baiacu, fruto do coro, maca encantada, exaustao da Fome). Rodar: tools/t.sh food112
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int add(const char* f) { PkItem e; e.file = f; e.name = f; e.fold = f; e.cube = false; PK.push_back(e); return PK0 + (int)PK.size() - 1; }
static void reset() { gDead = false; creative = false; gHP = 20; gFood = 10; gSat = 0.f; gExh = 0.f; gHungerT = gHungerA = gRegenFx = gPoisonT = 0; fxClear(); }
int main() {
    srand(1234);
    const int PUF = add("item/pufferfish"), PUF2 = add("item/fish_pufferfish_raw"), CHO = add("item/chorus_fruit"), EGA = add("item/enchanted_golden_apple"), GA = add("item/golden_apple"), ROT = add("item/rotten_flesh"), CLO = add("item/tropical_fish");
    FoodInfo f;
    // tabela (fome, modificador de saturacao, fx)
    CHECK(foodInfo(PUF, f) && f.h == 1 && fabsf(f.m - .1f) < 1e-6 && f.fx == 6);
    CHECK(foodInfo(PUF2, f) && f.h == 1 && f.fx == 6);
    CHECK(foodInfo(CHO, f) && f.h == 4 && fabsf(f.m - .3f) < 1e-6 && f.fx == 7);
    CHECK(foodInfo(EGA, f) && f.h == 4 && fabsf(f.m - 1.2f) < 1e-6 && f.fx == 8);
    CHECK(foodInfo(GA, f) && f.h == 4 && f.fx == 3);          // maca normal inalterada
    CHECK(foodInfo(ROT, f) && f.h == 4 && f.fx == 1);         // carne podre inalterada
    CHECK(foodInfo(CLO, f) && f.h == 1 && f.fx == 0);         // peixe-palhaco inalterado
    // sempre comestivel: so macas de ouro e fruto do coro
    CHECK(foodAlwaysEdible(CHO)); CHECK(foodAlwaysEdible(EGA)); CHECK(foodAlwaysEdible(GA));
    CHECK(!foodAlwaysEdible(PUF)); CHECK(!foodAlwaysEdible(ROT));
    reset(); gFood = 20; CHECK(survCanEat(CHO)); CHECK(!survCanEat(PUF)); CHECK(!survCanEat(ROT));   // fome cheia: so os "sempre comestiveis"
    reset(); CHECK(survCanEat(PUF));
    // tooltip: indices novos nao estouram o vetor FX e mostram o texto certo
    CHECK(foodTip(PUF).find("Veneno IV") != std::string::npos); CHECK(foodTip(CHO).find("Teletransporte") != std::string::npos);
    CHECK(foodTip(EGA).find("Resist") != std::string::npos); CHECK(foodTip(ROT).find("Fome (80%)") != std::string::npos);
    // baiacu: Fome III 15 s + Veneno IV 60 s
    reset(); { Slot s; s.id = PUF; s.n = 2; survEatNow(s); CHECK(s.n == 1); }
    CHECK(gFood == 11); CHECK(gHungerT == 300 && gHungerA == 2);
    CHECK(fxOn(FX_POISON) && gFxA[FX_POISON] == 3 && gFxT[FX_POISON] == 1200);
    // maca encantada: Regen II 20 s + Resist. ao fogo 5 min; maca normal nao mexe nos efeitos novos
    reset(); { Slot s; s.id = EGA; s.n = 1; survEatNow(s); CHECK(s.id == 0); }
    CHECK(fxOn(FX_REGEN) && gFxA[FX_REGEN] == 1 && gFxT[FX_REGEN] == 400); CHECK(fxOn(FX_FIRE) && gFxT[FX_FIRE] == 6000); CHECK(gRegenFx == 0);
    reset(); { Slot s; s.id = GA; s.n = 1; survEatNow(s); } CHECK(gRegenFx == 100 && !fxOn(FX_REGEN) && !fxOn(FX_FIRE));
    // carne podre: Fome I (amp 0); nao rebaixa um efeito mais forte ativo
    reset(); gHungerA = 2; gHungerT = 0; { int t = 0; while (gHungerT == 0 && t++ < 200) { Slot s; s.id = ROT; s.n = 1; survEatNow(s); } }
    CHECK(gHungerT == 600 && gHungerA == 0);
    reset(); gHungerT = 300; gHungerA = 2; survHunger(600, 0); CHECK(gHungerT == 300 && gHungerA == 2);
    reset(); gHungerT = 100; gHungerA = 0; survHunger(300, 2); CHECK(gHungerT == 300 && gHungerA == 2);
    // exaustao da Fome por tick: 0,005 x (nivel+1)
    { Phys P; memset(&P, 0, sizeof P); P.x = 0.5; P.y = 200; P.z = 0.5; gPvpP = &P;
      reset(); gHungerT = 10; gHungerA = 0; gSat = 5.f; survTick(P); CHECK(fabsf(gExh - .005f) < 1e-5 && gHungerT == 9);
      reset(); gHungerT = 10; gHungerA = 2; gSat = 5.f; survTick(P); CHECK(fabsf(gExh - .015f) < 1e-5);
      reset(); gSat = 5.f; survTick(P); CHECK(gExh < 1e-6); }   // sem efeito: nada
    // fruto do coro: teleporte
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int AY = 200;
    for (int z = -24; z <= 24; z++) for (int x = -24; x <= 24; x++) for (int y = AY - 12; y <= AY + 14; y++) setB(x, y, z, y < AY ? STONE : AIR);
    { Phys P; memset(&P, 0, sizeof P); int ok = 0, bad = 0;
      for (int i = 0; i < 60; i++) {
          reset(); P.x = P.px = 2.5; P.y = P.py = AY; P.z = P.pz = 2.5;
          if (chorusTeleport(P)) { ok++; if (fabs(P.x - 2.5) > 8.01 || fabs(P.z - 2.5) > 8.01 || fabs(P.y - AY) > 1e-9 || P.px != P.x || P.pz != P.z) bad++; }
          else if (P.x != 2.5 || P.y != AY || P.z != 2.5) bad++;   // falha nao move
      }
      CHECK(ok >= 55); CHECK(bad == 0); }
    // sem vaga (tudo solido): falha e nao move
    for (int z = -24; z <= 24; z++) for (int x = -24; x <= 24; x++) for (int y = AY; y <= AY + 14; y++) setB(x, y, z, STONE);
    { Phys P; memset(&P, 0, sizeof P); P.x = 2.5; P.y = AY; P.z = 2.5; CHECK(!chorusTeleport(P)); CHECK(P.x == 2.5 && P.y == AY && P.z == 2.5); }
    // agua cobrindo o chao: vaga com liquido e invalida
    for (int z = -24; z <= 24; z++) for (int x = -24; x <= 24; x++) for (int y = AY; y <= AY + 14; y++) setB(x, y, z, y < AY + 3 ? WATER : AIR);
    { Phys P; memset(&P, 0, sizeof P); P.x = 2.5; P.y = AY; P.z = 2.5; CHECK(!chorusTeleport(P)); }
    // morto: nao teleporta
    for (int z = -24; z <= 24; z++) for (int x = -24; x <= 24; x++) for (int y = AY; y <= AY + 14; y++) setB(x, y, z, AIR);
    { Phys P; memset(&P, 0, sizeof P); P.x = 2.5; P.y = AY; P.z = 2.5; reset(); gDead = true; CHECK(!chorusTeleport(P)); gDead = false; }
    printf("test_food112: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
