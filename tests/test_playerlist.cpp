// Lista de jogadores (Tab): layout em colunas, nivel de ping, ordem, PING/PONG e ping no roster.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static Uint32 gFakeT = 1000;
Uint32 SDL_GetTicks() { return gFakeT; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int udp() { const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); netSockNonBlock(s); return s; }
static int recvWait(int s, uint8_t* b, int n, int ms = 200) { for (int i = 0; i < ms; i++) { int r = (int)recvfrom(s, (char*)b, n, 0, nullptr, nullptr); if (r > 0) return r; usleep(1000); } return -1; }
static void pump() { for (int i = 0; i < 4; i++) { gFakeT += 20; netPoll(gFakeT); usleep(1500); } }
static void drain(int s) { uint8_t t[512]; while (recvfrom(s, (char*)t, sizeof t, 0, nullptr, nullptr) > 0) {} }
static bool getType(int s, int type, uint8_t* b) { for (int k = 0; k < 20; k++) { if (recvWait(s, b, 512) < 1) return false; if (b[0] == type) return true; } return false; }
static void send2(int s, int type, int sender, const std::string& nm, const uint8_t* pre = nullptr, int npre = 0) {
    sockaddr_in hg{}; hg.sin_family = AF_INET; hg.sin_addr.s_addr = htonl(INADDR_LOOPBACK); hg.sin_port = htons(47932);
    NBuf h; h.put<uint8_t>((uint8_t)type); h.put<uint8_t>((uint8_t)sender); for (int i = 0; i < npre; i++) h.put<uint8_t>(pre[i]);
    h.put<uint8_t>((uint8_t)nm.size()); for (char c : nm) h.put<uint8_t>((uint8_t)c);
    sendto(s, (const char*)h.d, h.n, 0, (sockaddr*)&hg, sizeof hg); pump();
}
static void hello(int s, const std::string& nm) { const uint8_t pre[3] = {(uint8_t)NET_VER, 0, 0}; send2(s, NP_HELLO, 255, nm, pre + 0, 2); }
static std::string str(const uint8_t* b, int& o) { const int l = b[o++]; std::string t((const char*)b + o, (size_t)l); o += l; return t; }
static std::map<int, std::string> roster(uint8_t* b) { std::map<int, std::string> m; int o = 2; const int n = b[o++]; for (int i = 0; i < n; i++) { const int id = b[o++]; m[id] = str(b, o); } return m; }

static void sendRaw(int s, const NBuf& b) { sockaddr_in hg{}; hg.sin_family = AF_INET; hg.sin_addr.s_addr = htonl(INADDR_LOOPBACK); hg.sin_port = htons(47932); sendto(s, (const char*)b.d, b.n, 0, (sockaddr*)&hg, sizeof hg); pump(); }
static void pong(int s, int id, uint32_t tok) { NBuf b; b.put<uint8_t>(NP_PONG); b.put<uint8_t>((uint8_t)id); b.put<uint32_t>(tok); sendRaw(s, b); }
// le o roster novo (id, nome, ping)
struct RE { int id; std::string nm; int ping; };
static std::vector<RE> rosterP(uint8_t* b) { std::vector<RE> v; int o = 2; const int n = b[o++]; for (int i = 0; i < n; i++) { RE e; e.id = b[o++]; e.nm = str(b, o); e.ping = b[o] | (b[o + 1] << 8); o += 2; v.push_back(e); } return v; }
int main() {
    chdir("/tmp"); uint8_t b[512];
    printf("[layout em colunas, igual ao oficial]\n");
    { int r, c;
      tabLayout(1, r, c); CHECK(r == 1 && c == 1);
      tabLayout(20, r, c); CHECK(r == 20 && c == 1);
      tabLayout(21, r, c); CHECK(r == 11 && c == 2);
      tabLayout(40, r, c); CHECK(r == 20 && c == 2);
      tabLayout(41, r, c); CHECK(r == 14 && c == 3);
      tabLayout(80, r, c); CHECK(r == 20 && c == 4); }
    printf("[nivel de ping]\n");
    CHECK(tabPingLevel(-1) == 5 && tabPingLevel(0) == 0 && tabPingLevel(149) == 0 && tabPingLevel(150) == 1 && tabPingLevel(299) == 1);
    CHECK(tabPingLevel(300) == 2 && tabPingLevel(599) == 2 && tabPingLevel(600) == 3 && tabPingLevel(999) == 3 && tabPingLevel(1000) == 4);
    const int G = udp(), H = udp();
    CHECK(netStartHost(47932)); gPlayerName = "zeta"; netOn = true;
    hello(G, "Bob"); CHECK(getType(G, NP_WELCOME, b)); hello(H, "alice"); CHECK(getType(H, NP_WELCOME, b));
    printf("[Tab no host: todos, ordem alfabetica sem diferenciar caixa]\n");
    { std::vector<TabEntry> v; tabBuild(v); CHECK(v.size() == 3 && v[0].name == "alice" && v[1].name == "Bob" && v[2].name == "zeta"); CHECK(v[2].id == netSelf && v[2].ping == 0); CHECK(v[0].ping == -1); }
    printf("[host manda PING a cada 2 s]\n");
    drain(G); drain(H); gFakeT += 2100; netUpdate(0.05f);
    CHECK(getType(G, NP_PING, b)); const uint32_t tok = b[2] | (b[3] << 8) | (b[4] << 16) | ((uint32_t)b[5] << 24); CHECK(tok == gFakeT);
    printf("[PONG vira ping; segundo PONG suaviza]\n");
    gFakeT += 80; pong(G, 1, tok); CHECK(netPingOf(1) >= 80 && netPingOf(1) <= 100 + 40);
    const int p1 = netPingOf(1); CHECK(p1 > 0);
    { for (auto& p : peers) if (p.id == 1) p.ping = 80; gFakeT += 160 - 80; pong(G, 1, tok + 0); }   // 2o pong: rtt = 160 + (tempo do pump) => media (80*3+rtt)/4
    CHECK(netPingOf(1) > 80 && netPingOf(1) < 140);
    printf("[PONG de outro id nao mexe no ping alheio nem de desconhecido]\n");
    { const int before = netPingOf(2); pong(G, 7, tok); CHECK(netPingOf(2) == before && netPingOf(7) == -1); }
    printf("[ping chega a todos pelo roster]\n");
    drain(H); gFakeT += 6000; for (auto& p : peers) p.seen = gFakeT; netUpdate(0.05f); CHECK(getType(H, NP_ROSTER, b));
    { auto v = rosterP(b); CHECK(v.size() == 3); bool ok = false; for (auto& e : v) if (e.id == 1 && e.ping == netPingOf(1)) ok = true; CHECK(ok); for (auto& e : v) if (e.id == 0) CHECK(e.ping == 0); }
    printf("[convidado: PING do host gera PONG com o mesmo token]\n");
    netStop(); netOn = true; netHost = false; netSelf = 2; netConn = true; netSock = udp();
    { const int A = udp(); sockaddr_in ha{}; ha.sin_family = AF_INET; ha.sin_addr.s_addr = htonl(INADDR_LOOPBACK); bind(A, (sockaddr*)&ha, sizeof ha); socklen_t al = sizeof ha; getsockname(A, (sockaddr*)&ha, &al); netHostAddr = ha;
      NBuf p = netMk(NP_PING); p.put<uint32_t>(0xA1B2C3D4u); p.d[1] = 0;
      sockaddr_in me{}; me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_LOOPBACK); bind(netSock, (sockaddr*)&me, sizeof me); socklen_t ml = sizeof me; getsockname(netSock, (sockaddr*)&me, &ml);
      netHostSeen = gFakeT; sendto(A, (const char*)p.d, p.n, 0, (sockaddr*)&me, sizeof me); usleep(3000); netPoll(gFakeT += 20);
      CHECK(getType(A, NP_PONG, b)); CHECK((b[2] | (b[3] << 8) | (b[4] << 16) | ((uint32_t)b[5] << 24)) == 0xA1B2C3D4u && b[1] == 2);
      printf("[convidado: roster com ping alimenta o Tab]\n");
      NBuf r = netMk(NP_ROSTER); r.put<uint8_t>(3);
      auto ent = [&](int id, const std::string& n, int pg) { r.put<uint8_t>((uint8_t)id); r.put<uint8_t>((uint8_t)n.size()); for (char c : n) r.put<uint8_t>(c); r.put<uint16_t>((uint16_t)pg); };
      ent(0, "Host", 0); ent(1, "ana", 45); ent(2, "Eu", 320);
      r.d[1] = 0; r.r = 2; netNameRecv(NP_ROSTER, 0, r);
      CHECK(netPingOf(1) == 45 && netPingOf(2) == 320 && netPingOf(0) == 0 && netPingOf(9) == -1);
      std::vector<TabEntry> v; tabBuild(v); CHECK(v.size() == 3 && v[0].name == "ana" && v[1].name == "Eu" && v[2].name == "Host");
      CHECK(v[1].ping == 320 && v[0].ping == 45);
      gTabHeld = true; CHECK(tabVisible()); netOn = false; CHECK(!tabVisible()); gTabHeld = false; }
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
