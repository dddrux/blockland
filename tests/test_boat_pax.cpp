// patchBoat2.1/4.1: bote com 2 jogadores (condutor + 1 passageiro), lotacao maxima 2 (bote de 4 removido), pacote com passageiro, assentos. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -2; cz <= 2; cz++) for (int cx = -2; cx <= 2; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY - 1; y < AY + 20; y++) setB(x, y, z, y < AY ? STONE : (y <= AY + 2 ? WATER : AIR));
}
int main() {
    arena(); Phys P; netOn = true; netSelf = 2; BOATS.clear(); gBoatRide = 0; gBoatPaxSelf = false; MOBS.clear();
    boatSpawn(BOAT_OAK, 0.5, AY + 1.0, 0.5, 0.f); const int id = BOATS[0].id;
    for (int i = 0; i < 30; i++) boatTickAll(P, 0, 0, 0, 0, false);
    // jogador 1 (id 1) conduz: estado chega ao jogador 2 (netSelf 2) com rider=1, pax=-1
    BOATS[0].rider = 1; BOATS[0].pax[0] = -1;
    // jogador 2 olha para o bote e clica: entra como passageiro (nao conduz)
    float eye[3] = {0.5f, (float)(AY + 3.0), -1.5f}, dir[3] = {0.f, -.5f, 1.f};
    CHECK(boatInteract(eye, dir, 6.f));
    CHECK(gBoatRide == id && gBoatPaxSelf && BOATS[0].pax[0] == 2 && BOATS[0].rider == 1);
    CHECK(!boatAuth(BOATS[0]));                       // quem conduz continua sendo o jogador 1
    CHECK(boatOcc(BOATS[0]) == 2);
    boatTickAll(P, 0, 0, 1, 0, false);                // W do passageiro nao move o bote
    CHECK(BOATS[0].rider == 1 && BOATS[0].pax[0] == 2);
    // assentos: condutor na frente (+0,2), passageiro atras (-0,6)
    { double f0, l0, f1, l1; boatSeatNet(BOATS[0], 1, f0, l0); boatSeatNet(BOATS[0], 2, f1, l1); CHECK(fabs(f0 - .2) < 1e-9 && fabs(f1 + .6) < 1e-9 && l0 == 0 && l1 == 0); }
    // patchBoat4.1: so 2 lugares: o 3o jogador (id 3) encontra o bote lotado
    { const int keep = netSelf; const int r0 = gBoatRide; const bool p0 = gBoatPaxSelf;
      netSelf = 3; gBoatRide = 0; gBoatPaxSelf = false; boatInteract(eye, dir, 6.f); CHECK(gBoatRide == 0 && boatPaxIdx(BOATS[0], 3) < 0);
      CHECK(BOATS[0].pax[0] == 2 && BOATS[0].pax[1] == -1 && BOATS[0].pax[2] == -1 && boatOcc(BOATS[0]) == 2);
      gBoatRide = r0; gBoatPaxSelf = p0; netSelf = keep; }
    // pacote leva o passageiro (ida e volta)
    { NBuf pk = boatNetPacket(BOATS[0]); pk.r = 2; std::vector<BoatEnt> keepB = BOATS; const int keepSelf = netSelf, keepRide = gBoatRide; const bool keepP = gBoatPaxSelf;
      BOATS.clear(); gBoatRide = 0; gBoatPaxSelf = false; netSelf = 7; boatNetRecv(pk);   // observador (id 7) recebe o estado do condutor
      CHECK(BOATS.size() == 1 && BOATS[0].rider == 1 && BOATS[0].pax[0] == 2 && boatOcc(BOATS[0]) == 2);
      BOATS = keepB; netSelf = keepSelf; gBoatRide = keepRide; gBoatPaxSelf = keepP; }
    // tipo >= 6 (save/rede antigos) vira o bote normal da mesma madeira
    { BNState o{}; o.x = 40; o.y = AY + 1.0; o.z = 40; boatNetApply(999, BOAT_DARK_OAK + BOAT_N, o); BoatEnt* e = boatById(999); CHECK(e && e->type == BOAT_DARK_OAK && boatCap(e->type) == 2); BOATS.erase(std::remove_if(BOATS.begin(), BOATS.end(), [](const BoatEnt& q) { return q.id == 999; }), BOATS.end()); }
    // condutor sai (rider -1 chega do host): passageiro assume o comando
    BOATS[0].rider = -1; boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(!gBoatPaxSelf && BOATS[0].rider == 2 && BOATS[0].pax[0] == -1 && boatAuth(BOATS[0]));
    // passageiro desmonta (sneak): libera o assento
    gBoatRide = id; gBoatPaxSelf = true; BOATS[0].rider = 1; BOATS[0].pax[0] = 2; boatTickAll(P, 0, 0, 0, 0, true);
    CHECK(gBoatRide == 0 && !gBoatPaxSelf && BOATS[0].pax[0] == -1);
    // disputa: o condutor confirma OUTRO passageiro (id 3): quem perdeu desce
    gBoatRide = id; gBoatPaxSelf = true; BOATS[0].rider = 1; BOATS[0].pax[0] = 2; BNState s{}; s.x = BOATS[0].x; s.y = BOATS[0].y; s.z = BOATS[0].z; s.rider = 1; s.pax[0] = 3;
    boatNetApply(id, BOAT_OAK, s); CHECK(gBoatRide == 0 && !gBoatPaxSelf);
    // split: 2 locais no mesmo bote e 3o recusado
    netOn = false; netSelf = 0; BOATS[0].rider = -1; for (int k = 0; k < 3; k++) BOATS[0].pax[k] = -1; gBoatRide = id; gBoatPaxSelf = false; Phys Q; gBoatXP[0] = &Q; gBoatX[0] = id;
    CHECK(boatOcc(BOATS[0]) == 2);
    gBoatXP[1] = &Q; boatInteract(eye, dir, 6.f, gBoatX[1]); CHECK(gBoatX[1] == 0);   // 3o local recusado (lotacao 2)
    { MobEnt m; m.type = M_PIG; m.x = BOATS[0].x; m.y = BOATS[0].y; m.z = BOATS[0].z; MOBS.push_back(m); boatBoard(); CHECK(MOBS.back().rideBoat == 0);   // animal nao entra (lotado)
      gBoatX[0] = gBoatX[1] = gBoatX[2] = 0; gBoatXP[0] = gBoatXP[1] = gBoatXP[2] = nullptr; MOBS.clear(); }
    printf("[bote normal = 2 lugares]\n");
    BOATS.clear(); MOBS.clear(); gBoatRide = 0; gBoatPaxSelf = false; netOn = true; netSelf = 0;
    boatSpawn(BOAT_OAK, 0.5, AY + 1.0, 0.5, 0.f); BOATS[0].rider = 1; { Phys Q; (void)Q; }
    CHECK(boatCap(BOATS[0].type) == 2);
    CHECK(boatPaxAdd(BOATS[0], 2)); CHECK(boatOcc(BOATS[0]) == 2); CHECK(boatOcc(BOATS[0]) >= boatCap(BOATS[0].type));   // cheio: o 3o nao entra
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
