// patchSlime.2: slime (tamanhos, pulos, divisao, drops, save, flags de rede, slime chunk). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
int main() {
    std::error_code ec; fs::create_directories("/tmp/bl_sl/saves/t", ec); if (chdir("/tmp/bl_sl") != 0) return 2;
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY - 1; y < AY + 12; y++) setB(x, y, z, y < AY ? STONE : AIR);
    gDead = false; creative = false; gGm = 0; gInvul = 0;
    MOBS.clear(); gSlimeQ.clear(); Phys P; P.x = 20; P.y = AY; P.z = 0; P.px = P.x; P.py = P.y; P.pz = P.z;
    // tamanho, caixa e vida
    spawnSlime(4, 0.5, AY, 0.5, 0.f); spawnSlime(2, 5.5, AY, 5.5, 0.f); spawnSlime(1, -5.5, AY, 5.5, 0.f);
    CHECK(MOBS.size() == 3 && MOBS[0].hp == 16 && MOBS[1].hp == 4 && MOBS[2].hp == 1);
    CHECK(fabs(mobW(MOBS[0]) - .51f * 4) < 1e-4 && fabs(mobH(MOBS[1]) - .51f * 2) < 1e-4);
    for (int i = 0; i < 40; i++) { spawnMob(M_SLIME, 0, AY, 0, 0.f); const int s = MOBS.back().ssz; CHECK(s == 1 || s == 2 || s == 4); CHECK(MOBS.back().hp == s * s); MOBS.pop_back(); }
    // pula: sai do lugar e sobe (sem alvo, passeia)
    { MOBS.erase(MOBS.begin() + 1, MOBS.end()); MobEnt& e = MOBS[0]; double maxY = e.y, d0x = e.x, d0z = e.z; bool air = false;
      for (int i = 0; i < 200; i++) { tickMob(MOBS[0], 1000, AY, 1000); maxY = std::max(maxY, MOBS[0].y); if (!MOBS[0].ground) air = true; }
      CHECK(air && maxY > AY + .5); CHECK(fabs(MOBS[0].x - d0x) + fabs(MOBS[0].z - d0z) > 1.0);
      CHECK(MOBS[0].y >= AY - .01); }   // nao atravessa o chao
    // persegue o jogador (pulos mais frequentes, vira para ele)
    { MOBS.clear(); spawnSlime(2, 0.5, AY, 0.5, 0.f); P.x = 12; P.px = 12; const double d0 = 12 - 0.5;   // dentro dos 16 blocos de alcance
      for (int i = 0; i < 300; i++) { Phys Q = P; tickMob(MOBS[0], P.x, P.y, P.z); hostileTick(MOBS[0], Q); }
      CHECK(MOBS[0].chase && fabs(MOBS[0].x - P.x) < d0 - 4.0); }
    // dano: so medio/grande machuca; pequeno nao
    { MOBS.clear(); spawnSlime(2, 5.0, AY, 0.0, 0.f); Phys Q; Q.x = 5.2; Q.y = AY; Q.z = 0.0; Q.px = Q.x; Q.py = Q.y; Q.pz = Q.z; const int hp0 = gHP; gDead = false; creative = false; gInvul = 0; gGm = 0;
      MOBS[0].atk = 0; hostileTick(MOBS[0], Q); const int d2 = hp0 - gHP;
      MOBS.clear(); spawnSlime(1, 5.0, AY, 0.0, 0.f); gInvul = 0; const int hp1 = gHP; MOBS[0].atk = 0; hostileTick(MOBS[0], Q);
      CHECK(d2 == 2 && hp1 == gHP); }
    // divisao ao morrer: 2-4 filhotes com metade do tamanho; o pequeno solta bola e nao divide
    { MOBS.clear(); gSlimeQ.clear(); spawnSlime(4, 0.5, AY, 0.5, 0.f); mobHurt(MOBS[0], 100, 1, 0, true);
      CHECK(MOBS[0].dead >= 1 && gSlimeQ.size() >= 2 && gSlimeQ.size() <= 4); for (auto& s : gSlimeQ) CHECK(s.sz == 2);
      const size_t n = gSlimeQ.size(); slimeFlush(); CHECK(gSlimeQ.empty() && MOBS.size() == 1 + n);
      for (size_t i = 1; i < MOBS.size(); i++) CHECK(MOBS[i].type == M_SLIME && MOBS[i].ssz == 2 && MOBS[i].hp == 4);
      MOBS.clear(); gSlimeQ.clear(); spawnSlime(1, 0.5, AY, 0.5, 0.f); mobHurt(MOBS[0], 100, 1, 0, true); CHECK(gSlimeQ.empty() && MOBS[0].dead >= 1); }
    // save/load guarda o tamanho; arquivo antigo (12 campos) carrega com tamanho valido
    { MOBS.clear(); BOATS.clear(); spawnSlime(4, 3, AY, 4, 0.f); MOBS[0].hp = 9; spawnSlime(2, 1, AY, 2, 0.f); boatsSave("t"); MOBS.clear(); mobsLoadFile("t");
      CHECK(MOBS.size() == 2 && MOBS[0].type == M_SLIME && MOBS[0].ssz == 4 && MOBS[0].hp == 9 && MOBS[1].ssz == 2 && MOBS[1].hp == 4);
      FILE* f = fopen("saves/t/mobs.dat", "wb"); fprintf(f, "MOBS1\nm %d 1 70 1 0 99 0 0 0 0 0 0\n", M_SLIME); fclose(f);
      MOBS.clear(); mobsLoadFile("t"); CHECK(MOBS.size() == 1 && (MOBS[0].ssz == 1 || MOBS[0].ssz == 2 || MOBS[0].ssz == 4) && MOBS[0].hp <= MOBS[0].ssz * MOBS[0].ssz); }
    // slime chunk: ~10% dos chunks, deterministico
    { int n = 0; for (int cz = -50; cz < 50; cz++) for (int cx = -50; cx < 50; cx++) { const bool a = isSlimeChunk(cx, cz); CHECK(a == isSlimeChunk(cx, cz)); n += a; } CHECK(n > 700 && n < 1300); }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
