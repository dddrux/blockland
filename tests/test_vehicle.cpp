// Veiculos (vehicle.inc): carro acelera/vira/freia, sobe degrau de 1 bloco, para na parede, barco flutua, aviao decola, descer so no chao. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 200;
static void arena() {
    for (int cz = -3; cz <= 3; cz++) for (int cx = -3; cx <= 3; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -40; z <= 40; z++) for (int x = -40; x <= 40; x++) for (int y = AY - 1; y < AY + 25; y++) setB(x, y, z, y < AY ? STONE : AIR);
}
static int defOf(const char* k) { for (int d = 0; d < VEH_N; d++) if (!strcmp(VEHD[d].key, k)) return d; return -1; }
int main() {
    arena(); Phys P; VEHS.clear(); gVehRide = 0;
    printf("[carro]\n");
    const int kart = defOf("go_kart"); CHECK(kart >= 0);
    vehSpawn(kart, 0.5, AY + 3.0, 0.5, 0.f);
    for (int i = 0; i < 40; i++) vehTickAll(P, 0, 0, 0, 0, false, false);
    VehEnt* e = &VEHS[0]; printf("  y=%.3f ground=%d\n", e->y, (int)e->ground);
    CHECK(e->ground && fabs(e->y - AY) < .05);
    gVehRide = e->id; const double z0 = e->z;
    for (int i = 0; i < 60; i++) vehTickAll(P, 0, 0, 1, 0, false, false);
    e = &VEHS[0]; printf("  z: %.2f -> %.2f v=%.3f\n", z0, e->z, e->v);
    CHECK(e->z > z0 + 8 && e->v > .3f && e->v <= vehM(kart).maxSpd + 1e-4);
    CHECK(fabs(P.z - (e->z + vehM(kart).seatZ)) < .01 && fabs(P.y - (e->y + vehM(kart).seatY)) < .01);   // jogador preso ao assento
    const float yaw0 = e->yaw; for (int i = 0; i < 30; i++) vehTickAll(P, 0, 1, 1, 0, false, false);
    CHECK(VEHS[0].yaw > yaw0 + 15.f);   // D = vira a direita (yaw cresce)
    for (int i = 0; i < 80; i++) vehTickAll(P, 0, 0, 0, 1, false, false);
    CHECK(VEHS[0].v <= 0.f && VEHS[0].v >= -vehM(kart).maxSpd * .35f - 1e-4);   // freia e da re (limitada)
    printf("[degrau e parede]\n");
    VEHS.clear(); gVehRide = 0; vehSpawn(kart, 0.5, AY + .01, 0.5, 0.f);
    for (int x = -5; x <= 5; x++) for (int z = 6; z <= 7; z++) setB(x, AY, z, STONE);   // degrau de 1 bloco
    for (int x = -5; x <= 5; x++) for (int y = AY; y <= AY + 3; y++) setB(x, y, 14, STONE);   // parede
    gVehRide = VEHS[0].id; double ymax = 0; for (int i = 0; i < 120; i++) { vehTickAll(P, 0, 0, 1, 0, false, false); ymax = std::max(ymax, VEHS[0].y); }
    e = &VEHS[0]; printf("  x=%.2f y=%.2f z=%.2f ymax=%.2f\n", e->x, e->y, e->z, ymax);
    CHECK(ymax >= AY + .95);   // subiu o degrau
    CHECK(e->z < 14.0 - vehM(kart).hr + .05);   // nao atravessa a parede
    printf("[descer]\n");
    vehTickAll(P, 0, 0, 0, 0, false, true); CHECK(gVehRide == 0); { std::vector<AABB> l; collBoxes(AABB{P.x - .3, P.y, P.z - .3, P.x + .3, P.y + 1.8, P.z + .3}, l); CHECK(l.empty()); }
    printf("[barco]\n");
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY; y <= AY + 2; y++) setB(x, y, z, WATER);
    VEHS.clear(); gVehRide = 0; const int boat = defOf("speed_boat"); vehSpawn(boat, 0.5, AY + 5.0, -20.5, 0.f);
    for (int i = 0; i < 200; i++) vehTickAll(P, 0, 0, 0, 0, false, false);
    e = &VEHS[0]; printf("  y=%.3f (agua ate %d)\n", e->y, AY + 3); CHECK(e->y > AY + 1.5 && e->y < AY + 3.3);
    gVehRide = e->id; const double bz = e->z; for (int i = 0; i < 60; i++) vehTickAll(P, 0, 0, 1, 0, false, false);
    CHECK(VEHS[0].z > bz + 8 && VEHS[0].y > AY + 1.5 && VEHS[0].y < AY + 3.3);
    printf("[aviao]\n");
    for (int z = -30; z <= 30; z++) for (int x = -30; x <= 30; x++) for (int y = AY; y <= AY + 2; y++) setB(x, y, z, AIR);
    VEHS.clear(); gVehRide = 0; const int pl = defOf("sports_plane"); vehSpawn(pl, 0.5, AY + .01, -35.5, 0.f);
    gVehRide = VEHS[0].id; for (int i = 0; i < 120; i++) vehTickAll(P, 0, 0, 1, 0, false, false);
    printf("  v=%.3f z=%.1f y=%.2f\n", VEHS[0].v, VEHS[0].z, VEHS[0].y); CHECK(VEHS[0].v > vehM(pl).maxSpd * .55f);
    for (int i = 0; i < 40; i++) vehTickAll(P, 0, 0, 1, 0, true, false);
    printf("  voo: y=%.2f\n", VEHS[0].y); CHECK(VEHS[0].y > AY + 3.0);
    vehTickAll(P, 0, 0, 1, 0, false, true); CHECK(gVehRide != 0);   // no ar: nao desce
    printf("[item]\n"); CHECK(vehDefOfItem(0) == -1);
    printf("%s: %d/%d\n", fails ? "FALHOU" : "OK", checks - fails, checks); return fails ? 1 : 0;
}
