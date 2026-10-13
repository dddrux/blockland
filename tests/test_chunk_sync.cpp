// v23: chunk do save do host -> convidado (NP_CHUNKREQ/NP_CHUNK) por UDP real em loopback. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    const size_t N = (size_t)CW * CH * CW; const int64_t k = key(3, -2);
    std::vector<BT> d(N); std::vector<uint8_t> m(N);
    for (size_t i = 0; i < N; i++) { d[i] = (i % 97 < 40) ? STONE : (i % 7 == 0 ? (BT)(XB_STAIR) : (BT)((i * 31) % 20)); m[i] = (uint8_t)((i / 13) % 5); }
    savedC[k] = d; savedM[k] = m; gDim = 0;
    netSockInit(); int rx = socket(AF_INET, SOCK_DGRAM, 0), tx = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = 0; bind(rx, (sockaddr*)&a, sizeof a);
    socklen_t l = sizeof a; getsockname(rx, (sockaddr*)&a, &l); netSock = tx; netSelf = 0; { int v = 8 << 20; setsockopt(rx, SOL_SOCKET, SO_RCVBUF, (const char*)&v, sizeof v); }
    netChunkServe(a, 3, -2);
    savedC.erase(k); savedM.erase(k); gChRx[k] = ChRx(); int got = 0;
    for (int i = 0; i < 400; i++) { NBuf b; int n = (int)recvfrom(rx, (char*)b.d, sizeof b.d, MSG_DONTWAIT, nullptr, nullptr); if (n < 2) break; b.n = n; b.get<uint8_t>(); b.get<uint8_t>(); netChunkRecv(b); got++; }
    printf("  fragmentos: %d\n", got);
    CHECK(got >= 1); CHECK(gChOK.count(k) == 1); CHECK(savedC.count(k) == 1 && savedM.count(k) == 1);
    CHECK(savedC[k] == d); CHECK(savedM[k] == m);
    // chunk sem save no host: "nada" (1 fragmento) => convidado gera pela seed
    const int64_t k2 = key(9, 9); gChRx[k2] = ChRx(); netChunkServe(a, 9, 9);
    { NBuf b; int n = (int)recvfrom(rx, (char*)b.d, sizeof b.d, 0, nullptr, nullptr); b.n = n; b.get<uint8_t>(); b.get<uint8_t>(); netChunkRecv(b); }
    CHECK(gChOK.count(k2) == 1 && savedC.count(k2) == 0);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
