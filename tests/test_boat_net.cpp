// Bote no multiplayer (boat.inc + net.inc): autoridade por montaria, estados em fila, disputa, golpe, remocao, animacao visual. Roda o main.cpp REAL.
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
// "envia" o pacote do bote do cliente A para o cliente B: serializa com o netSelf de A e aplica no estado de B
static std::vector<BoatEnt> A, B;
static void swapIn(std::vector<BoatEnt>& v) { std::swap(BOATS, v); }
int main() {
    arena(); Phys P; netOn = true;
    // ---- HOST (netSelf 0) cria um bote e o convidado (netSelf 1) o aprende ----
    netSelf = 0; BOATS.clear(); gBoatRide = 0; boatSpawn(BOAT_BIRCH, 0.5, AY + 1.0, 0.5, 33.f);
    const int id = BOATS[0].id; CHECK(id == 1);
    for (int i = 0; i < 40; i++) boatTickAll(P, 0, 0, 0, 0, false);
    NBuf pk = boatNetPacket(BOATS[0]); A = BOATS; BOATS.clear();
    netSelf = 1; pk.r = 2; boatNetRecv(pk);   // convidado recebe (pula tipo+remetente)
    CHECK(BOATS.size() == 1 && BOATS[0].id == id && BOATS[0].type == BOAT_BIRCH);
    CHECK(!boatAuth(BOATS[0]));   // sem montaria: quem simula e o host
    // bote remoto segue os estados e anima (balanço) sem fisica local
    const double y0 = BOATS[0].y; boatTickAll(P, 0, 0, 0, 0, false); boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(fabs(BOATS[0].y - y0) < 1e-6 && BOATS[0].age >= 2);
    // ---- convidado monta: assume a autoridade e passa a simular ----
    gBoatRide = id; boatTickAll(P, 0, 0, 1, 0, false);
    CHECK(BOATS[0].rider == 1 && boatAuth(BOATS[0]));
    for (int i = 0; i < 40; i++) boatTickAll(P, 0, 0, 1, 0, false);   // W: avanca
    const double dist = hypot(BOATS[0].x - 0.5, BOATS[0].z - 0.5); CHECK(dist > 1.0);
    CHECK(BOATS[0].paddle[0] && BOATS[0].paddlePos[0] > 0.f);
    CHECK(fabs(BOATS[0].pitchV) > 1e-4 || BOATS[0].lastFwd > 0.01f);   // animacao visual reagiu ao movimento
    NBuf st = boatNetPacket(BOATS[0]); const BoatEnt guestView = BOATS[0]; BOATS.clear();
    // ---- host recebe o estado do convidado: deixa de simular e segue o convidado ----
    netSelf = 0; BOATS = A; gBoatRide = 0; st.r = 2; boatNetRecv(st);
    CHECK(BOATS[0].rider == 1 && !boatAuth(BOATS[0]) && BOATS[0].q.size() == 1);
    boatTickAll(P, 0, 0, 0, 0, false);
    CHECK(fabs(BOATS[0].x - guestView.x) < 1e-9 && fabs(BOATS[0].z - guestView.z) < 1e-9 && BOATS[0].paddle[0]);
    CHECK(boatOfRider(1) == &BOATS[0] && !boatOfRider(2));
    // host nao assume bote ocupado: entra como PASSAGEIRO (patchBoat2.1) e o convidado segue conduzindo
    { float eye[3] = {(float)guestView.x, (float)guestView.y + 3.f, (float)guestView.z}, d[3] = {0, -1, 0}; CHECK(boatInteract(eye, d, 6.f) && gBoatRide == id && gBoatPaxSelf && BOATS[0].rider == 1 && BOATS[0].pax[0] == 0); gBoatRide = 0; gBoatPaxSelf = false; BOATS[0].pax[0] = -1; }
    // golpe do host em bote que o convidado controla: vai pela rede, nao aplica localmente
    { float eye[3] = {(float)BOATS[0].x, (float)BOATS[0].y + 3.f, (float)BOATS[0].z}, d[3] = {0, -1, 0}; const float dm = BOATS[0].damage; CHECK(boatHit(eye, d, 0, false, 6.f) && BOATS[0].damage == dm); }
    // dono aplica o golpe recebido
    A = BOATS; BOATS.clear(); netSelf = 1; BOATS.push_back(guestView); NBuf ht = netMk(NP_BOATHIT); ht.put<int32_t>(id); ht.put<float>(15.f); ht.put<uint8_t>(0); ht.r = 2;
    { BoatEnt* e = boatById(ht.get<int32_t>()); const float dmg = ht.get<float>(); CHECK(e && boatAuth(*e)); boatApplyHit(*e, dmg, false); CHECK(e->damage >= 15.f && e->timeSinceHit == 10); }
    // ---- disputa: dois montam ao mesmo tempo -> o menor id vence ----
    gBoatRide = id; BOATS[0].rider = 1; BNState o{}; o.x = BOATS[0].x; o.y = BOATS[0].y; o.z = BOATS[0].z; o.rider = 2; boatNetApply(id, BOAT_BIRCH, o);
    CHECK(gBoatRide == id);          // id 1 < 2: continuo montado
    o.rider = 0; boatNetApply(id, BOAT_BIRCH, o); CHECK(gBoatRide == 0);   // id 0 < 1: perdi
    // ---- desmontar devolve ao host; host (rider<0) retoma de onde parou ----
    gBoatRide = id; boatTickAll(P, 0, 0, 0, 0, false); gBoatRide = 0; boatTickAll(P, 0, 0, 0, 0, false); CHECK(BOATS[0].rider == -1);
    // ---- remocao ----
    NBuf dl = netMk(NP_BOATDEL); (void)dl; BoatEnt* e = boatById(id); if (e) { boatEject(*e); e->dead = true; } boatTickAll(P, 0, 0, 0, 0, false); CHECK(BOATS.empty());
    // ---- visual: curva inclina o casco para fora, sem afetar a fisica ----
    netOn = false; netSelf = 0; BOATS.clear(); gBoatRide = 0; boatSpawn(BOAT_OAK, 0.5, AY + 1.0, 0.5, 0.f); gBoatRide = BOATS[0].id;
    for (int i = 0; i < 60; i++) boatTickAll(P, 0, 1, 1, 0, false);   // frente + direita
    float pi, ro, bo; boatVisOff(BOATS[0], 0.5f, pi, ro, bo);
    printf("  curva: roll=%.2f pitch=%.2f bob=%.3f\n", ro, pi, bo);
    CHECK(ro > 1.f && fabs(ro) < 9.f && fabs(pi) < 6.f && fabs(bo) < .03f);
    printf("%d/%d checks, %d falhas\n", checks - fails, checks, fails); return fails != 0;
}
