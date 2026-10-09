// Bote (boat.inc): flutuacao, remar, virar, aterrar (derrapar), bater e quebrar, colocar com o raio. Roda o main.cpp REAL.
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
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) { for (int y = AY - 1; y < AY + 20; y++) setB(x, y, z, y < AY ? STONE : AIR); }
}
static void pool() { for (int z = -20; z <= 20; z++) for (int x = -20; x <= 20; x++) for (int y = AY; y <= AY + 2; y++) setB(x, y, z, WATER); }
int main() {
    arena(); pool();
    printf("[flutuar]\n");
    BOATS.clear(); gBoatRide = 0; Phys P;
    boatSpawn(BOAT_OAK, 0.5, AY + 6.0, 0.5, 0.f);
    for (int i = 0; i < 200; i++) boatTickAll(P, 0, 0, 0, 0, false);
    BoatEnt& b = BOATS[0];
    printf("  y=%.3f status=%d (superficie %.3f)\n", b.y, b.status, AY + 3 - 1 / 9.0);
    CHECK(b.status == BS_IN_WATER);
    CHECK(b.y > AY + 2.0 && b.y < AY + 3.0);          // boia na superficie, nao afunda nem voa
    double y1 = b.y; for (int i = 0; i < 40; i++) boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(fabs(BOATS[0].y - y1) < .02);               // estavel
    printf("[remar]\n");
    gBoatRide = BOATS[0].id; const double z0 = BOATS[0].z;
    for (int i = 0; i < 60; i++) boatTickAll(P, 0, 0, 1, 0, false);   // W com yaw_mc 0 = +Z
    printf("  z: %.2f -> %.2f  vel=%.3f\n", z0, BOATS[0].z, BOATS[0].mz);
    CHECK(BOATS[0].z > z0 + 4.0);
    CHECK(fabs(P.x - BOATS[0].x) < 1e-9 && fabs(P.y - (BOATS[0].y + BOAT_SEAT_Y)) < 1e-9);   // jogador no assento
    CHECK(BOATS[0].paddle[0] && BOATS[0].paddle[1]);
    printf("[virar]\n");
    const float yw = BOATS[0].yaw; for (int i = 0; i < 20; i++) boatTickAll(P, 0, 1, 1, 0, false);   // D = direita: yaw_mc cresce
    CHECK(BOATS[0].yaw > yw + 5.f);
    CHECK(boatYawO(BOATS[0]) < 180.f - yw - 5.f + 360.f);
    printf("[desmontar]\n");
    CHECK(!boatTickAll(P, 0, 0, 0, 0, true) && gBoatRide == 0);
    printf("[colocar com o raio]\n");
    BOATS.clear(); const float eye[3] = {.5f, AY + 5.f, .5f}; float dir[3] = {0, -1, 0};
    CHECK(boatPlace(BOAT_BIRCH, eye, dir, 0.f, 5.f));
    CHECK(BOATS.size() == 1 && BOATS[0].type == BOAT_BIRCH);
    printf("  y colocado=%.3f\n", BOATS[0].y);
    CHECK(BOATS[0].y > AY + 2.5 && BOATS[0].y < AY + 3.0);
    float up[3] = {0, 1, 0}; CHECK(!boatPlace(BOAT_OAK, eye, up, 0.f, 5.f));   // olhando pro ceu: nada
    printf("[bater]\n");
    const float e2[3] = {.5f, (float)BOATS[0].y + .3f, -2.f}, d2[3] = {0, 0, 1};
    for (int i = 0; i < 4; i++) CHECK(boatHit(e2, d2, 0, false, 3.f));
    CHECK(BOATS.size() == 1);                         // 4 socos = 40 de dano, ainda nao quebra (>40)
    CHECK(boatHit(e2, d2, 0, false, 3.f) && BOATS[0].dead);
    boatTickAll(P, 0, 0, 0, 0, false); CHECK(BOATS.empty());
    printf("[terra]\n");
    for (int z = 22; z <= 28; z++) for (int x = -3; x <= 3; x++) setB(x, AY, z, STONE);
    boatSpawn(BOAT_OAK, 0.5, AY + 1.5, 24.5, 0.f);
    for (int i = 0; i < 20; i++) boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(BOATS.size() == 1 && BOATS[0].status == BS_ON_LAND);
    CHECK(fabs(BOATS[0].y - (AY + 1)) < 1e-6);
    BOATS[0].mz = .3; for (int i = 0; i < 30; i++) boatTickAll(P, 0, 0, 0, 0, false);
    printf("  deslizou ate z=%.2f\n", BOATS[0].z);
    CHECK(BOATS[0].z < 28.3);                        // para na borda/atrito
    printf("[queda]\n");
    BOATS.clear(); boatSpawn(BOAT_SPRUCE, 0.5, AY + 14.0, 0.5, 0.f); for (int z = -2; z <= 2; z++) for (int x = -2; x <= 2; x++) { setB(x, AY, z, STONE); setB(x, AY + 1, z, AIR); setB(x, AY + 2, z, AIR); }
    for (int i = 0; i < 40; i++) boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(BOATS.empty());                            // caiu >3 blocos em terra: quebra (3 tabuas + 2 gravetos)
    printf("[colisao do casco]\n");
    BOATS.clear(); gBoatRide = 0;
    boatSpawn(BOAT_OAK, 0.5, AY + 1.0, 0.5, 0.f);   // bote apoiado no chao (pedra em AY dentro de |x|,|z|<=2)
    { Phys Q; Q.x = 0.5; Q.y = AY + 3.0; Q.z = 0.5; Q.px = Q.x; Q.py = Q.y; Q.pz = Q.z;
      for (int i = 0; i < 40; i++) { Q.my -= .08; Q.my *= .98; moveEntity(Q, 0, Q.my, 0); if (Q.onGround) break; }
      printf("  jogador pousou em y=%.3f (topo do bote %.3f)\n", Q.y, BOATS[0].y + BOAT_H);
      CHECK(fabs(Q.y - (BOATS[0].y + BOAT_H)) < 1e-6);   // fica em cima do bote
      Phys W2; W2.x = -2.0; W2.y = AY + 1.0; W2.z = 0.5; W2.px = W2.x; W2.onGround = true;
      for (int i = 0; i < 30; i++) { W2.my -= .08; W2.my *= .98; moveEntity(W2, .1, W2.my, 0); }
      CHECK(W2.y > BOATS[0].y + BOAT_H - 1e-6);   // degrau 0,6 > casco 0,5625: sobe no bote, como no Minecraft
    }
    printf("[animais embarcam]\n");
    BOATS.clear(); MOBS.clear(); gBoatRide = 0;
    boatSpawn(BOAT_OAK, 10.5, AY + 1.0, 10.5, 0.f); spawnMob(M_PIG, 11.7, AY + 1.0, 10.5, 0.f); spawnMob(M_COW, 10.5, AY + 1.0, 11.7, 0.f); spawnMob(M_SHEEP, 9.2, AY + 1.0, 10.5, 0.f);
    { Phys Q; for (int i = 0; i < 5; i++) boatTickAll(Q, 0, 0, 0, 0, false);
      int n = 0; for (auto& m : MOBS) if (m.rideBoat == BOATS[0].id) n++;
      printf("  passageiros: %d\n", n);
      CHECK(n == 2);   // maximo 2
      for (auto& m : MOBS) if (m.rideBoat) CHECK(fabs(m.x - BOATS[0].x) < 1.0 && fabs(m.y - (BOATS[0].y + .15)) < 1e-9);
      boatBreak(BOATS[0], false); for (auto& m : MOBS) CHECK(m.rideBoat == 0);   // quebrar o bote solta os animais
    }
    printf("[salvar/carregar]\n");
    BOATS.clear(); MOBS.clear(); gBoatRide = 0; gBoatNext = 0; system("mkdir -p saves/__t");
    boatSpawn(BOAT_BIRCH, 3.25, AY + 2.5, -4.75, 33.f); boatSpawn(BOAT_ACACIA, 7.0, AY + 1.0, 2.0, 90.f); gBoatRide = BOATS[1].id;
    boatsSave("__t"); BOATS.clear(); gBoatRide = 0; gBoatNext = 0; boatsLoad("__t");
    CHECK(BOATS.size() == 2 && BOATS[0].type == BOAT_BIRCH && fabs(BOATS[0].x - 3.25) < 1e-6 && fabs(BOATS[0].yaw - 33.f) < 1e-3);
    CHECK(gBoatRide == BOATS[1].id && gBoatNext == 2);
    for (int i = 0; i < 3; i++) boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(BOATS.size() == 2);
    printf("[chunk nao carregado nao derruba o bote]\n");
    BOATS.clear(); gBoatRide = 0; boatSpawn(BOAT_OAK, 5000.5, AY + 10.0, 5000.5, 0.f);
    for (int i = 0; i < 100; i++) boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(BOATS.size() == 1 && BOATS[0].y == AY + 10.0);
    printf("%d/%d checks, %d falhas\n", checks - fails, checks, fails);
    return fails ? 1 : 0;
}
