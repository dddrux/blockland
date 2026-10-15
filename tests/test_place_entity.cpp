// patch5.59: bloco com colisao nao e colocado onde ha jogador remoto, mob, bote ou carrinho (como o Minecraft); nada e empurrado.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 150;
static bool tryPlace() {   // coloca STONE na celula (4, AY, 4) mirando o chao; jogador (quem coloca) longe
    setB(4, AY, 4, AIR);
    PlaceCtx c{}; c.hx = 4; c.hy = AY - 1; c.hz = 4; c.face = 3; c.fx = .5; c.fy = 1; c.fz = .5; c.px = 40; c.py = AY; c.pz = 40; c.yaw = 0;
    const bool ok = placeBlock(STONE, c); if (ok) setB(4, AY, 4, AIR); return ok;
}
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 4; y++) setB(x, y, z, AIR); }
    CHECK(tryPlace());   // vazio: coloca
    { RemotePl r; r.init = true; r.x = 4.5; r.y = AY; r.z = 4.5; remotes[5] = r; }
    CHECK(!tryPlace());   // jogador remoto em cima da celula
    remotes[5].gm = 3; CHECK(tryPlace());   // espectador nao impede
    remotes[5].gm = 0; remotes[5].y = AY + 1.9; CHECK(tryPlace());   // acima da celula (cabeca acima do bloco)
    remotes[5].y = AY - 0.5; CHECK(!tryPlace());   // pes abaixo, corpo atravessa a celula
    remotes.clear(); CHECK(tryPlace());
    MobEnt e; e.type = 0; e.x = 4.5; e.y = AY; e.z = 4.5; e.dead = false; MOBS.push_back(e);
    CHECK(!tryPlace());   // mob na celula
    MOBS[0].dead = true; CHECK(tryPlace()); MOBS.clear();
    CartEnt k; k.x = 4.5; k.y = AY; k.z = 4.5; CARTS.push_back(k); CHECK(!tryPlace()); CARTS[0].dead = true; CHECK(tryPlace()); CARTS.clear();
    BoatEnt b; b.x = 4.5; b.y = AY; b.z = 4.5; BOATS.push_back(b); CHECK(!tryPlace()); BOATS.clear();
    CHECK(tryPlace());
    // o mob fica onde esta (nao foi empurrado)
    MOBS.push_back(e); tryPlace(); CHECK(fabs(MOBS[0].x - 4.5) < 1e-9 && fabs(MOBS[0].y - AY) < 1e-9);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
