// patchHorse.2: sela, montaria, domar e salvar. Roda o main.cpp REAL.
#include <unistd.h>
#include <sys/stat.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    Phys P; memset(&P, 0, sizeof P); P.x = 0.5; P.y = 70; P.z = 0.5;
    MOBS.clear(); MobEnt e; e.type = M_HORSE; e.uid = 9; e.x = 3; e.y = 70; e.z = 3; e.hp = 20; MOBS.push_back(e);
    CHECK(isEquine(M_HORSE) && isEquine(M_DONKEY) && isEquine(M_MULE) && isEquine(M_SHORSE) && !isEquine(M_PIG) && !isEquine(M_LLAMA));
    // selvagem: monta, nao anda; quando o teste de domar passa, vira domado; com temperamento 0 derruba
    gRideUid = 9; MOBS[0].rideLim = 5; MOBS[0].temper = 0;
    bool riding = true; for (int i = 0; i < 6 && riding; i++) riding = horseTick(P, true, false, false, false, false, 0.f);
    CHECK(!riding && gRideUid == 0 && !MOBS[0].tame);   // derrubado
    gRideUid = 9; MOBS[0].rideT = 0; MOBS[0].temper = 100;
    for (int i = 0; i < 6; i++) riding = horseTick(P, true, false, false, false, false, 0.f);
    CHECK(riding && MOBS[0].tame && gRideUid == 9);      // domado, segue montado
    CHECK(fabs(P.x - MOBS[0].x) < 1e-6 && P.y > MOBS[0].y + .3 && P.y < MOBS[0].y + 1.2);
    // sem sela nao guia; com sela guia
    horseTick(P, true, false, false, false, false, 33.f); CHECK(gRideFwd == 0.f);
    MOBS[0].saddled = true; horseTick(P, true, false, false, true, false, 33.f); CHECK(gRideFwd > .9f && gRideJump && gRideYaw == 33.f);
    // agachar desmonta
    CHECK(!horseTick(P, false, false, true, false, false, 0.f) && gRideUid == 0);
    // salvar/carregar sela e domado
    mkdir("saves", 0755); mkdir("saves/t_horse", 0755); boatsSave("t_horse");
    MOBS.clear(); boatsLoad("t_horse");
    bool ok = false; for (const MobEnt& m : MOBS) if (m.type == M_HORSE && m.saddled && m.tame) ok = true; CHECK(ok);
    // porco: so guia com a cenoura no palito
    MOBS.clear(); { MobEnt p; p.type = M_PIG; p.uid = 4; p.x = 2; p.y = 70; p.z = 2; p.hp = 10; p.saddled = true; p.tame = true; MOBS.push_back(p); }
    gRideUid = 4; horseTick(P, true, false, false, true, false, 10.f, false); CHECK(gRideFwd == 0.f && !gRideJump);
    horseTick(P, true, false, false, true, false, 10.f, true); CHECK(gRideFwd > .9f && !gRideJump && P.y < MOBS[0].y + .6);
    CHECK(isSaddleable(M_PIG) && !isSaddleable(M_COW));
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
