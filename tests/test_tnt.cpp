// patch5.64: TNT (acender com fogo/isqueiro, pavio de 80 ticks, explosao forca 4, cadeia, dano) + explosao do creeper preservada. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120, T = 1300;
static void arena() { for (int z = -12; z < 20; z++) for (int x = -12; x < 20; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 12; y++) setB(x, y, z, AIR); } }
static void block(int r, int cx, int cy, int cz) { for (int y = cy - r; y <= cy + r; y++) for (int z = cz - r; z <= cz + r; z++) for (int x = cx - r; x <= cx + r; x++) if (y >= AY) setB(x, y, z, STONE); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gPkByFile["block/tnt"] = T; tntHooksInit(); CHECK(gTntId == T);
    Phys P; P.x = 40; P.y = AY; P.z = 40; P.mx = P.my = P.mz = 0; gHP = 20; gDead = false; creative = false; gInvul = 0; netOn = false; TNTS.clear();
    // acender: isqueiro/fogo ao lado/direto
    arena(); setB(5, AY, 5, T); CHECK(getB(5, AY, 5) == T);
    CHECK(tntIgnite(5, AY, 5) && getB(5, AY, 5) == AIR && TNTS.size() == 1 && TNTS[0].fuse == 80);
    CHECK(!tntIgnite(5, AY, 5));   // ja nao e TNT
    TNTS.clear(); arena(); setB(5, AY, 5, T); setBM(6, AY, 5, XB_FIRE, 0); CHECK(getB(5, AY, 5) == AIR && TNTS.size() == 1);   // fogo ao lado acende (gancho)
    // pavio: 80 ticks; cai e para no chao
    TNTS.clear(); arena(); block(4, 8, AY + 4, 8); for (int y = AY + 1; y <= AY + 7; y++) for (int z = 6; z <= 10; z++) for (int x = 6; x <= 10; x++) setB(x, y, z, AIR);   // caverna 5x5x7 dentro de um bloco de pedra
    for (int y = AY; y <= AY + 7; y++) for (int z = 4; z <= 12; z++) for (int x = 4; x <= 12; x++) if (x < 6 || x > 10 || z < 6 || z > 10 || y == AY + 8) setB(x, y, z, STONE);
    tntPrime(8.5, AY + 1, 8.5, 80); int ticks = 0; while (!TNTS.empty() && ticks < 200) { tntTick(P); ticks++; }
    CHECK(ticks == 80 && TNTS.empty());   // 80 ticks de pavio
    CHECK(getB(8, AY + 1, 8) == AIR && getB(8, AY + 3, 8) == AIR);   // abriu o espaco
    int pedra = 0; for (int y = AY; y <= AY + 8; y++) for (int z = 4; z <= 12; z++) for (int x = 4; x <= 12; x++) if (getB(x, y, z) == STONE) pedra++;
    CHECK(pedra < 9 * 9 * 9 - 125);   // removeu pedra alem da caverna original (5x5x7=175 vazios)
    // dano ao jogador perto (forca 4: raio 8) e nada longe
    arena(); gHP = 20; P.x = 8.5; P.y = AY; P.z = 8.5; tntPrime(8.5, AY, 10.5, 1); tntTick(P); CHECK(gHP < 20);
    arena(); gHP = 20; P.x = 40; P.z = 40; tntPrime(8.5, AY, 10.5, 1); tntTick(P); CHECK(gHP == 20);
    // cadeia: TNT dentro do raio da explosao acende (fuse 10..29)
    arena(); TNTS.clear(); setB(9, AY, 8, T); setB(8, AY, 9, T); tntPrime(8.5, AY, 8.5, 1); tntTick(P);
    CHECK(TNTS.size() >= 1 && TNTS[0].fuse >= 10 && TNTS[0].fuse < 30 && getB(9, AY, 8) != T);
    // convidado: TNT so visual (anima e some) e boomReceive aplica dano/recuo
    arena(); block(2, 8, AY + 2, 8); TNTS.clear(); netOn = true; netHost = false; netSelf = 2; tntNetRecv(8.5, AY + 2, 8.5, 3); CHECK(TNTS.size() == 1 && TNTS[0].fuse == 3);
    for (int i = 0; i < 5; i++) tntTick(P); CHECK(TNTS.empty() && getB(8, AY + 2, 8) == STONE);   // sumiu sem destruir nada
    gPvpP = &P; gHP = 20; gDead = false; creative = false; gInvul = 0; P.mx = 0; boomReceive(6, true, .5f, .25f, 0.f); CHECK(gHP < 20 && P.mx == .5f && P.my >= .25f);
    gHP = 20; creative = true; boomReceive(6, true, 1.f, 0.f, 0.f); CHECK(gHP == 20); creative = false; netOn = false; TNTS.clear();
    // sem rede de guest: convidado nao processa
    arena(); TNTS.clear(); netOn = true; netHost = false; setB(1, AY, 1, T); CHECK(!tntIgnite(1, AY, 1) && getB(1, AY, 1) == T); netOn = false; setB(1, AY, 1, AIR); TNTS.clear();   // convidado nao acende
    // creeper continua igual: forca 3 abre menos que TNT e mata o proprio mob
    arena(); block(3, 8, AY + 3, 8); MOBS.clear(); MobEnt e; e.type = M_CREEPER; e.x = 8.5; e.y = AY + 3; e.z = 8.5; e.hp = 20; e.dead = 0; MOBS.push_back(e); P.x = 40; P.z = 40;
    creeperBoom(MOBS[0], P); CHECK(MOBS[0].hp == 0 && MOBS[0].dead == 20 && getB(8, AY + 3, 8) == AIR);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
