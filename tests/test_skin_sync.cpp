// Sincronizacao de skin: NP_SKIN (16 pacotes), host guarda/repassa, validacao do dono, esquecer ao sair. Handshake UDP real em loopback.
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
static void pump() { for (int i = 0; i < 6; i++) { gFakeT += 20; netPoll(gFakeT); usleep(1500); } }
static void drain(int s) { uint8_t t[2048]; while (recvfrom(s, (char*)t, sizeof t, 0, nullptr, nullptr) > 0) {} }
static sockaddr_in hg() { sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons(47931); return a; }
static void hello(int s, const std::string& nm) { sockaddr_in a = hg(); NBuf h; h.put<uint8_t>(NP_HELLO); h.put<uint8_t>(255); h.put<uint8_t>(NET_VER); h.put<uint8_t>(0); h.put<uint8_t>((uint8_t)nm.size()); for (char c : nm) h.put<uint8_t>((uint8_t)c); sendto(s, (const char*)h.d, h.n, 0, (sockaddr*)&a, sizeof a); pump(); }
static std::vector<uint8_t> pat(int seed) { std::vector<uint8_t> v(SKIN_BYTES); for (int i = 0; i < SKIN_BYTES; i++) v[i] = (uint8_t)(i * 7 + seed); return v; }
static void sendSkin(int s, int sender, int owner, const std::vector<uint8_t>& px, int nch = SKIN_NCH) {
    sockaddr_in a = hg();
    for (int i = 0; i < nch; i++) { NBuf b; b.put<uint8_t>(NP_SKIN); b.put<uint8_t>((uint8_t)sender); b.put<uint8_t>((uint8_t)owner); b.put<uint8_t>((uint8_t)i); for (int k = 0; k < SKIN_CH; k++) b.put<uint8_t>(px[i * SKIN_CH + k]); sendto(s, (const char*)b.d, b.n, 0, (sockaddr*)&a, sizeof a); }
    pump();
}
static int countSkin(int s, int owner) { int n = 0; uint8_t b[2048]; for (;;) { if (recvWait(s, b, sizeof b, 60) < 1) break; if (b[0] == NP_SKIN && b[2] == owner) n++; } return n; }
int main() {
    chdir("/tmp"); const int G = udp(), H = udp();
    CHECK(netStartHost(47931)); gPlayerName = "Alex"; netOn = true;
    for (int i = 0; i < SKIN_BYTES; i++) skin[i] = (uint8_t)(i * 3 + 1);   // skin do host
    hello(G, "Zeh"); drain(G);
    printf("[host recebe e guarda a skin do convidado]\n");
    const auto A = pat(11), B = pat(99);
    sendSkin(G, 1, 1, A); CHECK(gSkins.count(1) && gSkins[1].px == A && gSkins[1].dirty);
    gSkins[1].dirty = false; sendSkin(G, 1, 1, A); CHECK(!gSkins[1].dirty);          // igual: nao recria textura
    sendSkin(G, 1, 1, B, SKIN_NCH - 1); CHECK(gSkins[1].px == A && !gSkins[1].dirty); // incompleta: ignora
    sendSkin(G, 1, 1, B, SKIN_NCH); CHECK(gSkins[1].px == B && gSkins[1].dirty);      // 2a metade do envio incompleto + completo = troca
    printf("[validacao do dono]\n");
    sendSkin(G, 1, 0, A); CHECK(!gSkins.count(0));       // convidado nao manda skin em nome do host
    sendSkin(G, 1, 2, A); CHECK(!gSkins.count(2));       // nem de outro convidado
    printf("[quem entra depois recebe as skins guardadas + a do host]\n");
    drain(H); hello(H, "Zed");
    CHECK(countSkin(H, 1) == SKIN_NCH);
    hello(H, "Zed"); drain(H);
    printf("[repasse e reenvio periodico]\n");
    drain(H); sendSkin(G, 1, 1, A); CHECK(countSkin(H, 1) >= SKIN_NCH);   // host repassa ao outro convidado
    drain(H); gFakeT += 21000; netUpdate(0.05f); CHECK(countSkin(H, 0) == SKIN_NCH);   // e reenvia a skin do host
    printf("[esquecer ao sair]\n");
    netSkinForget(1); CHECK(!gSkins.count(1));
    printf("[lado convidado]\n");
    netStop(); netOn = true; netHost = false; netSelf = 2;
    { auto recv = [&](int sender, int owner, int idx, const std::vector<uint8_t>& px) { NBuf b = netMk(NP_SKIN); b.put<uint8_t>((uint8_t)owner); b.put<uint8_t>((uint8_t)idx); for (int k = 0; k < SKIN_CH; k++) b.put<uint8_t>(px[idx * SKIN_CH + k]); b.r = 2; netSkinRecv(sender, b); };
      for (int i = 0; i < SKIN_NCH; i++) recv(0, 1, i, A); CHECK(gSkins.count(1) && gSkins[1].px == A);       // do host, em nome de outro: vale
      for (int i = 0; i < SKIN_NCH; i++) recv(0, 2, i, B); CHECK(!gSkins.count(2));                          // a minha propria: ignora
      for (int i = 0; i < SKIN_NCH; i++) recv(3, 1, i, B); CHECK(gSkins[1].px == A);                          // convidado 3 falando pelo 1: ignora
      NBuf bad = netMk(NP_SKIN); bad.put<uint8_t>(1); bad.put<uint8_t>(200); bad.r = 2; netSkinRecv(1, bad); CHECK(gSkins[1].px == A);   // indice invalido / pacote curto
    }
    netStop(); CHECK(gSkins.empty() && gSkinT == 0);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
