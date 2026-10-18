// Parte 3B: /tell /msg /w /r, /tp <nick>, /puxar, /give <nick> (net.inc + chat_debug.inc). Roda o main.cpp REAL com UDP em loopback.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static std::string last() { return chatLog.empty() ? "" : chatLog.back().s; }
static Phys P{};
static void run(const std::string& l) { execCommand(l, P); }
static int A;
static int rx(uint8_t* b, int n) { for (int i = 0; i < 200; i++) { int r = (int)recvfrom(A, (char*)b, n, 0, nullptr, nullptr); if (r > 0) return r; usleep(1000); } return -1; }
static std::string txt(uint8_t* b) { return std::string((char*)b + 4, b[3]); }
int main() {
    chdir("/tmp"); gPlayerName = "Zeh"; P.x = 5; P.y = 64; P.z = 7;
    A = socket(AF_INET, SOCK_DGRAM, 0); netSockNonBlock(A);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); bind(A, (sockaddr*)&a, sizeof a);
    socklen_t al = sizeof a; getsockname(A, (sockaddr*)&a, &al);
    netSock = socket(AF_INET, SOCK_DGRAM, 0); netHost = true; netOn = true; netSelf = 0;
    peers.push_back({a, 1000, 1, {}, true, "Alex"});
    uint8_t b[256];
    // ---- /tell ----
    run("/tell alex oi mundo"); CHECK(last() == "Você sussurra para Alex: oi mundo");
    CHECK(rx(b, sizeof b) > 0 && b[0] == NP_PRIV && b[2] == 1 && txt(b) == "oi mundo");
    run("/msg Ninguem x"); CHECK(last() == "Jogador não encontrado: Ninguem");
    run("/w Zeh x"); CHECK(last().find("si mesmo") != std::string::npos);
    run("/tell Alex"); CHECK(last().find("Uso: /tell") == 0);
    { NBuf r; r.put<uint8_t>(0); r.put<uint8_t>(3); r.put<uint8_t>('o'); r.put<uint8_t>('i'); r.put<uint8_t>('!'); netCmdRecv(NP_PRIV, 1, r); }
    CHECK(last() == "Alex sussurra para você: oi!" && gLastTell == 1);
    { NBuf r; r.put<uint8_t>(9); r.put<uint8_t>(1); r.put<uint8_t>('x'); netCmdRecv(NP_PRIV, 1, r); } CHECK(last() == "Alex sussurra para você: oi!");   // outro alvo: ignora
    run("/r de volta"); CHECK(rx(b, sizeof b) > 0 && b[0] == NP_PRIV && b[2] == 1 && txt(b) == "de volta");
    // ---- /puxar, /tp, /give (host) ----
    run("/puxar Alex"); CHECK(rx(b, sizeof b) > 0 && b[0] == NP_TPTO && b[2] == 1); { double d[3]; memcpy(d, b + 3, 24); CHECK(d[0] == 5 && d[1] == 64 && d[2] == 7); }
    run("/tp Alex 1 2 3"); CHECK(rx(b, sizeof b) > 0 && b[0] == NP_TPTO); { double d[3]; memcpy(d, b + 3, 24); CHECK(d[0] == 1 && d[1] == 2 && d[2] == 3); }
    run("/give alex stone 5"); CHECK(rx(b, sizeof b) > 0 && b[0] == NP_GIVE && b[2] == 1); { int32_t it, n; memcpy(&it, b + 3, 4); memcpy(&n, b + 7, 4); CHECK(it > 0 && n == 5); }
    run("/puxar Nada"); CHECK(last() == "Jogador não encontrado: Nada");
    remotes[1].init = true; remotes[1].x = 10; remotes[1].y = 70; remotes[1].z = 20;
    run("/tp Alex"); CHECK(P.x == 10 && P.y == 70 && P.z == 20 && gTpHold == 1);
    run("/tp Zeh Alex"); CHECK(P.x == 10);   // mover a si mesmo ate o jogador
    // ---- recebimento: so o host (id 0) pode mover/dar ----
    gNetTp.on = false; { NBuf r; r.put<uint8_t>(0); r.put<double>(1); r.put<double>(2); r.put<double>(3); netCmdRecv(NP_TPTO, 1, r); } CHECK(!gNetTp.on);
    { NBuf r; r.put<uint8_t>(0); r.put<double>(1); r.put<double>(2); r.put<double>(3); netCmdRecv(NP_TPTO, 0, r); } CHECK(gNetTp.on && gNetTp.y == 2);
    for (Slot& s : inv) s = Slot();
    { NBuf r; r.put<uint8_t>(0); r.put<int32_t>(findItem("stone")); r.put<int32_t>(4); netCmdRecv(NP_GIVE, 1, r); } { int t = 0; for (Slot& s : inv) t += s.n; CHECK(t == 0); }
    { NBuf r; r.put<uint8_t>(0); r.put<int32_t>(findItem("stone")); r.put<int32_t>(4); netCmdRecv(NP_GIVE, 0, r); } { int t = 0; for (Slot& s : inv) t += s.n; CHECK(t == 4); }
    // ---- convidado nao move/da item a outros; /give <nick> em si mesmo vale como /give normal ----
    netHost = false; netSelf = 2; peers.clear(); gHostPlName = "Hostzao"; remotes[0].init = true;
    run("/puxar Hostzao"); CHECK(last().find("Só operadores") != std::string::npos);   // convidado sem op nao move outros
    run("/give Zeh stone 2"); CHECK(last().find("permissão") != std::string::npos);   // convidado sem op nao usa /give (chat_debug.inc:151)
    gOpIds.insert(netSelf);   // convidado OP: /give e /puxar valem (em si e em outros)
    run("/puxar Hostzao"); CHECK(last().find("foi puxado") != std::string::npos);
    run("/give Hostzao stone"); CHECK(last().find(" a Hostzao") != std::string::npos);
    run("/give Zeh stone 2"); CHECK(last().find("Entregue 2x") == 0);
    gOpIds.erase(netSelf);
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
