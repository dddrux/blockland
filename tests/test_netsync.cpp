// patchNet.1/2: NP_DRAGON/DRAGONHIT, NP_BOLT, NP_FBALL (receptores)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static NBuf wireOf(const NBuf& b) { NBuf c = b; c.r = 0; c.get<uint8_t>(); c.get<uint8_t>(); return c; }
int main() {
    if (chdir("..") != 0) return 99;
    // convidado recebe o dragao do host
    netHost = false; NBuf d = netMk(NP_DRAGON); d.put<uint8_t>(1); d.put<float>(10.f); d.put<float>(80.f); d.put<float>(-5.f); d.put<float>(90.f); d.put<float>(2.f); d.put<int16_t>(150); d.put<uint8_t>(0); d.put<uint8_t>(3);
    { NBuf w = wireOf(d); dragonNetRecv(NP_DRAGON, w, 0); }
    CHECK(gDragon.on && gDragon.x == 10.0 && gDragon.y == 80.0 && gDragon.hp == 150.f && gDragon.hurtT == 3);
    { NBuf w = wireOf(d); dragonNetRecv(NP_DRAGON, w, 3); }   // de outro convidado: ignorado (x continua igual)
    d = netMk(NP_DRAGON); d.put<uint8_t>(0); { NBuf w = wireOf(d); dragonNetRecv(NP_DRAGON, w, 0); } CHECK(!gDragon.on);
    // host recebe golpe de convidado
    netHost = true; gDragon = DragonEnt(); gDragon.on = true; gDragon.hp = 200.f;
    NBuf h = netMk(NP_DRAGONHIT); h.put<int16_t>(7);
    { NBuf w = wireOf(h); dragonNetRecv(NP_DRAGONHIT, w, 2); } CHECK(gDragon.hp == 193.f && gDragon.hurtT == 10);
    { NBuf w = wireOf(h); dragonNetRecv(NP_DRAGONHIT, w, 0); } CHECK(gDragon.hp == 193.f);   // do proprio host: ignorado
    NBuf k = netMk(NP_DRAGONHIT); k.put<int16_t>(9999); { NBuf w = wireOf(k); dragonNetRecv(NP_DRAGONHIT, w, 2); } CHECK(gDragon.hp == 153.f);   // dano limitado a 40
    NBuf kk = netMk(NP_DRAGONHIT); kk.put<int16_t>(40); for (int i = 0; i < 4; i++) { NBuf w = wireOf(kk); dragonNetRecv(NP_DRAGONHIT, w, 2); } CHECK(gDragon.dead == 1);
    // raio
    netHost = false; NBuf bl = netMk(NP_BOLT); bl.put<float>(12.f); bl.put<float>(64.f); bl.put<float>(-30.f); bl.put<int32_t>(77);
    { NBuf w = wireOf(bl); wxBoltRecv(w, 0); } CHECK(gBoltPend && gBoltPx == 12.f && gBoltPz == -30.f && gBoltPSeed == 77);
    gBoltPend = false; { NBuf w = wireOf(bl); wxBoltRecv(w, 4); } CHECK(!gBoltPend);
    // bola de fogo
    FBALLS.clear(); NBuf fb = netMk(NP_FBALL); fb.put<uint8_t>(0); for (int i = 0; i < 6; i++) fb.put<float>(1.f * i);
    gDim = 0; { NBuf w = wireOf(fb); fbNetRecv(w, 0); } CHECK(FBALLS.size() == 1 && FBALLS[0].vz == 5.0);
    NBuf fc = netMk(NP_FBALL); fc.put<uint8_t>(1); for (int i = 0; i < 6; i++) fc.put<float>(0.f);
    { NBuf w = wireOf(fc); fbNetRecv(w, 0); } CHECK(FBALLS.size() == 1);   // outra dimensao: ignorada
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
