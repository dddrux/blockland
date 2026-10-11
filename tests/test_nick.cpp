// Parte 2: nickname (regras do oficial + unico no servidor). Funcoes puras + handshake HELLO por UDP real em loopback. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static Uint32 gFakeT = 1000;
Uint32 SDL_GetTicks() { return gFakeT; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static int udp() { const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); netSockNonBlock(s); return s; }
static void lo(sockaddr_in& a, int port) { a = {}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons((uint16_t)port); }
static int recvWait(int s, uint8_t* b, int n, int ms = 300) { for (int i = 0; i < ms; i++) { int r = (int)recvfrom(s, (char*)b, n, 0, nullptr, nullptr); if (r > 0) return r; usleep(1000); } return -1; }
static void pump() { for (int i = 0; i < 4; i++) { gFakeT += 20; netPoll(gFakeT); usleep(1500); } }
// envia HELLO com o nick (bytes crus) e devolve o 1o byte da resposta (NP_WELCOME ou NP_KICK); msg recebe o texto do KICK
static int hello(int sock, const std::string& nm, std::string* msg = nullptr) {
    sockaddr_in hg; lo(hg, 47931);
    NBuf h; h.put<uint8_t>(NP_HELLO); h.put<uint8_t>(255); h.put<uint8_t>(NET_VER); h.put<uint8_t>(0); h.put<uint8_t>((uint8_t)nm.size()); for (char c : nm) h.put<uint8_t>((uint8_t)c);
    sendto(sock, (const char*)h.d, h.n, 0, (sockaddr*)&hg, sizeof hg); pump();
    uint8_t buf[256]; const int n = recvWait(sock, buf, sizeof buf); if (n < 1) return -1;
    if (buf[0] == NP_KICK && msg) { const int l = buf[2]; *msg = std::string((char*)buf + 3, (size_t)l); }
    return buf[0];
}
static std::string peerName(int id) { for (auto& p : peers) if (p.id == id) return p.name; return "?"; }

int main() {
    SECTION("regras do nick");
    CHECK(nickValid("Steve") && nickValid("a_b_9") && nickValid("A") && nickValid("1234567890123456"));
    CHECK(!nickValid("") && !nickValid("12345678901234567") && !nickValid("Zé") && !nickValid("a b") && !nickValid("a-b") && !nickValid("<x>"));
    CHECK(nickSanitize("Steve") == "Steve" && nickSanitize("a b") == "a_b" && nickSanitize("Zé Mário") == "Z_Mrio");
    CHECK(nickSanitize("<>|!?") == "" && nickSanitize("") == "");
    CHECK(nickSanitize("12345678901234567890") == "1234567890123456" && nickSanitize("12345678901234567890").size() == 16);
    CHECK(nickValid(nickSanitize("qualquer coisa ÁÉÍ 123 muito longa demais")) );
    SECTION("igualdade sem diferenciar caixa, sugestao e nome automatico");
    CHECK(nickEq("Zeh", "zEH") && !nickEq("Zeh", "Zeh_") && !nickEq("Zeh", "Zeb"));
    CHECK(nickSuggest("Zeh", [](const std::string&) { return false; }) == "Zeh_1");
    CHECK(nickSuggest("Zeh", [](const std::string& c) { return c == "Zeh_1" || c == "Zeh_2"; }) == "Zeh_3");
    { const std::string r = nickSuggest("1234567890123456", [](const std::string&) { return false; }); CHECK(r.size() == 16 && r == "12345678901234_1" && nickValid(r)); }
    { const std::string r = nickSuggest("", [](const std::string&) { return false; }); CHECK(r == "Jogador_1"); }
    CHECK(nickIsAuto("Jogador") && nickIsAuto("Jogador_2") && nickIsAuto("jogador 12") && !nickIsAuto("Jogadora") && !nickIsAuto("Zeh") && !nickIsAuto("Jog"));
    SECTION("host: nick unico");
    CHECK(netStartHost(47931)); gPlayerName = "Alex";
    creative = true; WORLD_SEED = 1; gWorldTime = 0;
    const int A = udp(), B = udp(), C = udp(), D = udp(), E = udp(), F = udp(), G = udp();
    std::string m;
    CHECK(hello(A, "Zeh") == NP_WELCOME && peers.size() == 1 && peers[0].name == "Zeh");
    CHECK(hello(A, "Zeh") == NP_WELCOME && peers.size() == 1);                       // HELLO repetido do mesmo convidado nao e conflito
    CHECK(hello(B, "zeh", &m) == NP_KICK && peers.size() == 1);                      // mesmo nick, outra caixa
    CHECK(m.find("zeh") != std::string::npos && m.find("zeh_1") != std::string::npos);   // diz o nick e sugere variacao (na grafia do jogador)
    CHECK(hello(C, "ALEX", &m) == NP_KICK && peers.size() == 1);                     // nick do host
    CHECK(hello(B, "Zeh_1") == NP_WELCOME && peers.size() == 2 && peerName(2) == "Zeh_1");   // aceita a sugestao
    SECTION("host: sanitiza o que chega");
    CHECK(hello(C, "Zé Mário") == NP_WELCOME && peers.size() == 3 && peerName(3) == "Z_Mrio");
    SECTION("host: nome padrao troca sozinho (sem recusar)");
    CHECK(hello(D, "Jogador 1") == NP_WELCOME && peerName(4) == "Jogador_1");
    CHECK(hello(E, "Jogador 1") == NP_WELCOME && peerName(5) == "Jogador_2");
    CHECK(hello(F, "") == NP_WELCOME && peers.size() == 6 && nickValid(peerName(6)) && nickIsAuto(peerName(6)));
    SECTION("host: fantasma que caiu libera o nick");
    gFakeT += 3000;   // ninguem mandou nada: A virou fantasma (> 2 s)
    { NetPeer* a = nullptr; for (auto& p : peers) if (p.name == "Zeh") a = &p; CHECK(a != nullptr); }
    CHECK(hello(G, "zeh") == NP_WELCOME);                                            // volta por outra porta e conserva o nick
    { int n = 0; for (auto& p : peers) if (nickEq(p.name, "Zeh")) n++; CHECK(n == 1); }
    CHECK(netLabel(peers.back().id) == peers.back().name);
    netStop(); close(A); close(B); close(C); close(D); close(E); close(F); close(G);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
