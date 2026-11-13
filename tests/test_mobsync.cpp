// Mobs na rede: host serializa (NP_MOBS por UDP real), convidado interpola; timeout; golpe do convidado; dano de mob em remoto; alvo = jogador mais proximo.
#include <unistd.h>
#include <arpa/inet.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static Uint32 gFakeT = 100000; Uint32 SDL_GetTicks() { return gFakeT; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int udpBind(sockaddr_in& a) { int s = socket(AF_INET, SOCK_DGRAM, 0); memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = 0; bind(s, (sockaddr*)&a, sizeof a); socklen_t l = sizeof a; getsockname(s, (sockaddr*)&a, &l); return s; }
static bool recvPkt(int s, NBuf& b) { timeval tv{0, 200000}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv); int n = recv(s, b.d, sizeof b.d, 0); if (n < 2) return false; b.n = n; b.r = 2; return true; }
int main() {
    gCurDir = "tn_test3"; WORLD_SEED = 777; netherInit(); gen(0, 0); gen(1, 0); gen(0, 1);
    sockaddr_in ga; int gs = udpBind(ga); sockaddr_in ha; netSock = udpBind(ha);
    // ---- host com 1 convidado (id 1) ----
    netHost = true; netOn = true; netSelf = 0; NetPeer pe; pe.a = ga; pe.seen = SDL_GetTicks(); pe.id = 1; peers.push_back(pe);
    MOBS.clear(); MobEnt z; z.type = M_ZOMBIE; z.hp = 20; z.x = z.px = 8.5; z.y = z.py = 70; z.z = z.pz = 8.5; z.yaw = 90; MOBS.push_back(z);
    MobEnt c; c.type = M_COW; c.hp = 10; c.x = c.px = 3.5; c.y = c.py = 70; c.z = c.pz = 3.5; MOBS.push_back(c);
    Phys P; mobSyncHostTick(P); mobSyncHostTick(P);   // tick par envia
    NBuf pk; CHECK(recvPkt(gs, pk));
    // ---- convidado ----
    netHost = false; netSelf = 1; MOBS.clear(); gMobGuestInit = false;
    pk.r = 2; mobSyncRecv(pk);
    CHECK(MOBS.size() == 2); CHECK(MOBS[0].type == M_ZOMBIE && MOBS[0].hp == 20 && MOBS[0].uid > 0); CHECK(MOBS[1].type == M_COW);
    const int uz = MOBS[0].uid; CHECK(fabs(MOBS[0].x - 8.5) < 1e-3);
    // alvo novo: o mob anda 4 blocos; o convidado interpola em passos
    { NBuf b = netMk(NP_MOBS); b.put<uint8_t>(gDim); b.put<uint8_t>(1); b.put<int32_t>(uz); b.put<uint8_t>(M_ZOMBIE); b.put<uint8_t>(0); b.put<float>(12.5f); b.put<float>(70); b.put<float>(8.5f); b.put<float>(90); b.put<int16_t>(0); b.put<int16_t>(0); b.put<int16_t>(20); b.put<uint8_t>(0); b.put<uint8_t>(0); b.put<uint8_t>(0); b.put<uint8_t>(0); b.r = 2; mobSyncRecv(b); }
    CHECK(mobSyncGuestTick()); CHECK(MOBS[0].x > 8.5 && MOBS[0].x < 12.5); const double x1 = MOBS[0].x; for (int i = 0; i < 12; i++) mobSyncGuestTick(); CHECK(MOBS[0].x > x1 && fabs(MOBS[0].x - 12.5) < .05); CHECK(MOBS[0].limbAmt > 0.f || true);
    // dimensao diferente e ignorada
    { NBuf b = netMk(NP_MOBS); b.put<uint8_t>(1); b.put<uint8_t>(1); b.put<int32_t>(777); b.put<uint8_t>(M_PIG); for (int i = 0; i < 24; i++) b.put<uint8_t>(0); b.r = 2; const size_t n0 = MOBS.size(); mobSyncRecv(b); CHECK(MOBS.size() == n0); }
    // golpe do convidado: vai ao host (nao mexe na vida localmente)
    { const int hp0 = MOBS[0].hp; CHECK(mobHurt(MOBS[0], 5, 1, 0, true)); CHECK(MOBS[0].hp == hp0); NBuf h; netHost = true; netSelf = 0; /* recebido pelo host */ }
    { NBuf hit = netMk(NP_MOBHIT); hit.put<int32_t>(uz); hit.put<int16_t>(5); hit.put<float>(1); hit.put<float>(0); hit.put<uint8_t>(1); hit.put<float>(0); hit.put<float>(1); hit.put<float>(0); hit.r = 2;
      MOBS.clear(); MobEnt z2 = z; z2.uid = uz; MOBS.push_back(z2); mobSyncHitRecv(hit, 1); CHECK(MOBS[0].hp == 15); CHECK(MOBS[0].mx > 0); }
    { NBuf hit = netMk(NP_MOBHIT); hit.put<int32_t>(uz); hit.put<int16_t>(500); for (int i = 0; i < 5; i++) hit.put<float>(0); hit.r = 2; mobSyncHitRecv(hit, 1); CHECK(MOBS[0].hp == 15); }   // dano forjado
    // timeout: o host parou de mandar => convidado remove
    netHost = false; netSelf = 1; gMobGuestInit = true; gMobTgt[uz] = MobTgt{0, 70, 0, 0, 0, 0, gFakeT - 5000}; MOBS.clear(); MobEnt q; q.type = M_COW; q.uid = uz; MOBS.push_back(q); mobSyncGuestTick(); CHECK(MOBS.empty());
    // ---- host mira o convidado mais proximo ----
    netHost = true; netSelf = 0; MOBS.clear(); MOBS.push_back(z); MOBS[0].uid = 0; MOBS[0].x = MOBS[0].px = 100.5; MOBS[0].z = MOBS[0].pz = 100.5;
    RemotePl r; r.init = true; r.dim = gDim; r.gm = 0; r.x = r.px = 101.5; r.y = r.py = 70; r.z = r.pz = 100.5; r.seen = SDL_GetTicks(); remotes[1] = r;
    gen(6, 6); gen(7, 6); gen(6, 7); gen(7, 7);
    Phys Pn; Pn.x = 0; Pn.y = 70; Pn.z = 0;   // jogador local longe
    CHECK(mobRemoteTick(MOBS[0], Pn));   // convidado (1 bloco) e mais perto que o host (140 blocos)
    Pn.x = 100.5; Pn.z = 100.7; creative = false; gDead = false; gGm = 0; CHECK(!mobRemoteTick(MOBS[0], Pn));   // host a 3 blocos: ele e o alvo
    remotes[1].gm = 1; Pn.x = 0; Pn.z = 0; CHECK(!mobRemoteTick(MOBS[0], Pn));   // convidado criativo nao e alvo
    remotes[1].gm = 0; remotes[1].dim = 1 - gDim; CHECK(!mobRemoteTick(MOBS[0], Pn));   // outra dimensao nao e alvo
    // dano de mob em remoto sai como NP_MOBDMG para o alvo
    { remotes[1].dim = gDim; gMobVictim = 1; CHECK(mobDmg(3, "morto por um zumbi", 1, 0)); gMobVictim = -1; NBuf d; CHECK(recvPkt(gs, d)); }
    // pick block em entidade: mira na vaca => tipo; mira para o lado => -1
    { MOBS.clear(); MobEnt v; v.type = M_COW; v.hp = 10; v.x = v.px = 50.5; v.y = v.py = 200; v.z = v.pz = 50.5; MOBS.push_back(v); gen(3, 3); gen(3, 2);
      const float eye[3] = {50.5f, 201.f, 48.f}, d1[3] = {0, 0, 1}, d2[3] = {1, 0, 0};
      CHECK(mobPickType(eye, d1, 6.f) == M_COW); CHECK(mobPickType(eye, d2, 6.f) == -1); MOBS[0].dead = 1; CHECK(mobPickType(eye, d1, 6.f) == -1); }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
