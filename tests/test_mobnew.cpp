// test_mobnew: morcego voa e urso polar anda/e neutro (patchMobNew.1). Rodar: tools/t.sh mobnew
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }   // stub: a sonda de ticks do morcego
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    WORLD_SEED = 12345; const int AY = 150;
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z < 30; z++) for (int x = -30; x < 30; x++) for (int y = AY - 8; y < AY + 12; y++) setB(x, y, z, y < AY ? STONE : AIR);
    MOBS.clear(); spawnMob(M_BAT, 0.5, AY + 1.0, 0.5, 0.f); { MobEnt& e = MOBS[0]; double mn = 1e9, mx = -1e9, d = 0; for (int k = 0; k < 2000; k++) { const double px = e.x, pz = e.z; tickMob(e, 0.0, AY, 25.0); mn = std::min(mn, e.y); mx = std::max(mx, e.y); d += hypot(e.x - px, e.z - pz); }
        CHECK(std::isfinite(e.x + e.y + e.z)); CHECK(mx - mn > .8); CHECK(mn > AY + .3); CHECK(d > 3.0); CHECK(e.hp == 6); }   // voa, paira acima do chao e anda
    MOBS.clear(); spawnMob(M_POLAR, 0.5, AY + 1.0, 0.5, 0.f); { MobEnt& e = MOBS[0]; double d = 0; for (int k = 0; k < 3000; k++) { const double px = e.x, pz = e.z; tickMob(e, 0.0, AY, 25.0); d += hypot(e.x - px, e.z - pz); }
        CHECK(e.hp == 30); CHECK(d > 5.0); CHECK(e.ground); }   // neutro: so anda sem atacar de longe
    // spawn natural do morcego: caverna escura (y 40..44) enterrada em pedra, jogador la dentro
    for (int z = -40; z < 40; z++) for (int x = -40; x < 40; x++) for (int y = 30; y < 70; y++) setB(x, y, z, (y >= 40 && y <= 45) ? AIR : STONE);
    Phys P; memset(&P, 0, sizeof P); P.x = 0.5; P.y = 41; P.z = 0.5; P.px = P.x; P.py = P.y; P.pz = P.z; gPvpP = &P; gDead = false; netOn = false; gDim = 0;
    for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) { Chunk* c = getC(cx, cz); if (!c) { gen(cx, cz); c = getC(cx, cz); } if (c && !c->lc) c->lc = new uint8_t[CW * CH * CW](); }   // luz em cache toda escura (sk=0, bl=0): caverna fechada
    MOBS.clear(); for (int k = 0; k < 6000; k++) wildSpawn(P);
    int nb = 0, bad = 0; for (const MobEnt& e : MOBS) if (e.type == M_BAT && !e.dead) { nb++; if (e.y < 39 || e.y > 47) bad++; }
    CHECK(nb >= 1); CHECK(nb <= 5); CHECK(bad == 0);   // nasce em caverna, respeita o teto de 5 por jogador
    printf("test_mobnew: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
