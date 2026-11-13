// Ovo arremessavel (egg.inc): cooldown, consumo, pilha 16, colisao com bloco/mob, chocar 1/8, criativo.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static void arena() {   // ar em y 200..215, piso em 199 (mundo gerado em volta da origem)
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 8; z++) for (int x = -8; x < 8; x++) for (int y = 199; y < 216; y++) setB(x, y, z, y == 199 ? STONE : AIR);
}
int main() {
    if (chdir("..") != 0) return 99;
    Phys P; P.x = .5; P.y = 200.0; P.z = .5; P.sneak = false; gDead = false; gGm = 0; creative = false;
    for (Slot& s : inv) s = Slot(); MOBS.clear(); parts.clear(); EGGS.clear(); gEggCd = 0;
    CHECK(maxStack(IT_EGG) == 16 && maxStack(IT_EGG_PIG) == 64);
    arena();
    Slot s; s.id = IT_EGG; s.n = 3;
    CHECK(eggThrow(P, 0.f, 0.f, s) && s.n == 2 && EGGS.size() == 1 && gEggCd == 10);
    CHECK(!eggThrow(P, 0.f, 0.f, s) && s.n == 2 && EGGS.size() == 1);        // cooldown
    for (int i = 0; i < 10; i++) eggsTick(P); CHECK(gEggCd == 0);
    CHECK(eggThrow(P, 0.f, 0.f, s) && s.n == 1);
    for (int i = 0; i < 10; i++) eggsTick(P);
    CHECK(eggThrow(P, 0.f, 0.f, s) && s.n == 0 && s.id == 0);                 // acabou: slot vazio
    CHECK(!eggThrow(P, 0.f, 0.f, s));
    // yaw 0 olha para -Z: ovo vai para z negativo; cai com gravidade
    EGGS.clear(); s.id = IT_EGG; s.n = 1; gEggCd = 0; eggThrow(P, 0.f, 0.f, s);
    const double z0 = EGGS[0].z, y0 = EGGS[0].y; eggStep(EGGS[0], P);
    CHECK(EGGS[0].z < z0 - 1.0 && EGGS[0].y < y0 + .1);
    // parede de pedra: o ovo quebra (some) com 8 particulas
    EGGS.clear(); parts.clear(); for (int y = 200; y < 210; y++) for (int x = -3; x <= 3; x++) setB(x, y, -6, STONE);
    s.id = IT_EGG; s.n = 1; gEggCd = 0; eggThrow(P, 0.f, 0.f, s); EGGS[0].vy = 0; EGGS[0].vx = 0;
    bool gone = false; for (int i = 0; i < 20 && !gone; i++) { auto& e = EGGS[0]; if (!eggStep(e, P)) gone = true; }
    CHECK(gone && parts.size() >= 8);
    // mob no caminho: empurra, nao morre (dano 0)
    EGGS.clear(); MOBS.clear(); spawnMob(M_COW, .5, 200.0, -3.0, 0.f); const int hp0 = MOBS[0].hp; MOBS[0].mz = 0;
    s.id = IT_EGG; s.n = 1; gEggCd = 0; eggThrow(P, 0.f, 0.f, s); EGGS[0].vy = 0; EGGS[0].vx = 0; EGGS[0].x = .5; EGGS[0].y = 201.3;
    gone = false; for (int i = 0; i < 5 && !gone; i++) if (!eggStep(EGGS[0], P)) gone = true;
    CHECK(gone && MOBS[0].hp == hp0 && MOBS[0].mz < -.1 && MOBS[0].hurt == 10);
    // chocar: ~1/8 em muitas tentativas (galinhas novas), nunca zero nem sempre
    srand(1234); MOBS.clear(); for (int i = 0; i < 4000; i++) eggHatch(.5, 200.0, -2.0);
    const int nm = (int)MOBS.size(); CHECK(nm > 4000 / 8 * .7 && nm < 4000 / 8 * 1.6);
    for (const MobEnt& m : MOBS) if (m.type != M_CHICKEN) { CHECK(false); break; }
    // egg remoto (NP_EGG): so visual; atravessa mob sem empurrar e nao choca
    { EGGS.clear(); MOBS.clear(); spawnMob(M_COW, .5, 200.0, -3.0, 0.f); MOBS[0].hurt = 0; MOBS[0].mz = 0; parts.clear();
      NBuf b; b.put<float>(.5f); b.put<float>(201.3f); b.put<float>(.5f); b.put<float>(0.f); b.put<float>(0.f); b.put<float>(-1.5f); b.r = 0;
      eggNetRecv(b, 3); CHECK(EGGS.size() == 1 && EGGS[0].owner == 3 && fabs(EGGS[0].vz + 1.5) < 1e-4);
      srand(7); bool g2 = false; for (int i = 0; i < 20 && !g2; i++) if (!eggStep(EGGS[0], P)) g2 = true;
      CHECK(g2 && MOBS.size() == 1 && MOBS[0].hurt == 0 && MOBS[0].mz == 0 && parts.size() >= 8); }
    // convidado: choco vira PEDIDO ao host (nao cria galinha local); host valida e cria
    { MOBS.clear(); netOn = true; netHost = false; int sent = 0; srand(1);
      for (int i = 0; i < 400; i++) { size_t before = MOBS.size(); eggHatch(.5, 200.0, .5); if (MOBS.size() != before) sent = -1000; }
      CHECK(sent == 0 && MOBS.empty());   // convidado nunca spawna mob local
      netHost = true; gEggHatchCd = 0; NBuf b; b.put<float>(.5f); b.put<float>(200.f); b.put<float>(.5f); b.put<uint8_t>(1); b.r = 0;
      eggHatchRecv(b, 2); CHECK(MOBS.size() == 1 && MOBS[0].type == M_CHICKEN);
      NBuf b2; b2.put<float>(.5f); b2.put<float>(200.f); b2.put<float>(.5f); b2.put<uint8_t>(1); b2.r = 0; eggHatchRecv(b2, 2); CHECK(MOBS.size() == 1);   // spam: cooldown
      gEggHatchCd = 0; NBuf b3; b3.put<float>(.5f); b3.put<float>(200.f); b3.put<float>(.5f); b3.put<uint8_t>(99); b3.r = 0; eggHatchRecv(b3, 2); CHECK(MOBS.size() == 1);   // n forjado
      gEggHatchCd = 0; NBuf b4; b4.put<float>(.5f); b4.put<float>(-500.f); b4.put<float>(.5f); b4.put<uint8_t>(1); b4.r = 0; eggHatchRecv(b4, 2); CHECK(MOBS.size() == 1);   // fora do mundo
      netOn = false; netHost = false; MOBS.clear(); }
    // cooldown proprio (P2 do split): nao mexe no gEggCd do jogador 1
    { creative = false; EGGS.clear(); s.id = IT_EGG; s.n = 3; gEggCd = 0; int cd2 = 0; CHECK(eggThrow(P, 0.f, 0.f, s, cd2) && cd2 == 10 && gEggCd == 0 && s.n == 2); CHECK(!eggThrow(P, 0.f, 0.f, s, cd2) && s.n == 2); }
    // criativo: nao consome
    creative = true; EGGS.clear(); s.id = IT_EGG; s.n = 2; gEggCd = 0; CHECK(eggThrow(P, 0.f, 0.f, s) && s.n == 2);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
