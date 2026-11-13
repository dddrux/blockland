// Etapa 3: nick do jogador 2 (split), /nick2, /list com split, perfil e chat que nao deixa forjar o nick de outro.
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
    sockaddr_in hg{}; hg.sin_family = AF_INET; hg.sin_addr.s_addr = htonl(INADDR_LOOPBACK); hg.sin_port = htons(47933);
    NBuf h; h.put<uint8_t>((uint8_t)type); h.put<uint8_t>((uint8_t)sender); for (int i = 0; i < npre; i++) h.put<uint8_t>(pre[i]);
    h.put<uint8_t>((uint8_t)nm.size()); for (char c : nm) h.put<uint8_t>((uint8_t)c);
    sendto(s, (const char*)h.d, h.n, 0, (sockaddr*)&hg, sizeof hg); pump();
}
static void hello(int s, const std::string& nm) { const uint8_t pre[3] = {(uint8_t)NET_VER, 0, 0}; send2(s, NP_HELLO, 255, nm, pre + 0, 2); }
static std::string str(const uint8_t* b, int& o) { const int l = b[o++]; std::string t((const char*)b + o, (size_t)l); o += l; return t; }
static std::map<int, std::string> roster(uint8_t* b) { std::map<int, std::string> m; int o = 2; const int n = b[o++]; for (int i = 0; i < n; i++) { const int id = b[o++]; m[id] = str(b, o); o += 2; } return m; }   // 2 bytes de ping depois de cada nick

static void sendRaw(int s, const NBuf& b) { sockaddr_in hg{}; hg.sin_family = AF_INET; hg.sin_addr.s_addr = htonl(INADDR_LOOPBACK); hg.sin_port = htons(47933); sendto(s, (const char*)b.d, b.n, 0, (sockaddr*)&hg, sizeof hg); pump(); }
static void say(int s, int id, const std::string& t) { NBuf b; b.put<uint8_t>(NP_CHAT); b.put<uint8_t>((uint8_t)id); b.put<uint8_t>((uint8_t)t.size()); for (char c : t) b.put<uint8_t>((uint8_t)c); sendRaw(s, b); }
static bool chatHas(const std::string& t) { for (auto& l : chatLog) if (l.s == t) return true; return false; }
int main() {
    chdir("/tmp"); uint8_t b[512];
    printf("[nick do jogador 2]\n");
    gPlayerName = "Alex"; gP2Name = ""; gSplit = true;
    CHECK(p2Nick() == "Jogador_2");
    gPlayerName = "Jogador_2"; CHECK(p2Nick() != "Jogador_2" && nickValid(p2Nick()) && !nickEq(p2Nick(), netName()));
    gPlayerName = "Alex"; gP2Name = "Segundo"; CHECK(p2Nick() == "Segundo");
    gP2Name = "alex"; CHECK(!nickEq(p2Nick(), "Alex") && nickValid(p2Nick()));   // igual ao do jogador 1 (sem caixa): ganha sufixo
    gP2Name = "x y!"; CHECK(p2Nick() == "Jogador_2");   // invalido: padrao
    printf("[/nick2 valida]\n");
    gP2Name = "";
    CHECK(cmdNick2Change("a b") != "" && cmdNick2Change("ALEX") != "" && cmdNick2Change(std::string(17, 'a')) != "");
    CHECK(cmdNick2Change("Zeca") == "" && gP2Name == "Zeca" && p2Nick() == "Zeca");
    CHECK(cmdNickChange("zeca").find("jogador 2") != std::string::npos);   // P1 nao pode pegar o nick do P2
    printf("[/nick2 e /list pelo chat]\n");
    { Phys P{}; execCommand("/nick2 Mano", P); } CHECK(gP2Name == "Mano");
    { Phys P{}; execCommand("/nick2 Alex", P); } CHECK(gP2Name == "Mano" && chatLog.back().col == 0xFF5555);
    { Phys P{}; execCommand("/list", P); } CHECK(chatLog.back().s.find("Mano") != std::string::npos && chatLog.back().s.find("Alex") != std::string::npos && chatLog.back().s.find("2 jogadores") != std::string::npos);
    gSplit = false; { Phys P{}; execCommand("/nick2 Outro", P); } CHECK(gP2Name == "Mano");   // sem split nao vale
    { Phys P{}; execCommand("/list", P); } CHECK(chatLog.back().s.find("Mano") == std::string::npos);
    printf("[nick2 salvo no perfil]\n");
    gP2Name = "Mano"; profileSave(); gP2Name = ""; profileLoad(); CHECK(gP2Name == "Mano");
    gSplit = true;
    printf("[rede: P2 nunca colide com jogador da rede]\n");
    const int G = udp(), H = udp();
    CHECK(netStartHost(47933)); gPlayerName = "Alex"; netOn = true; gP2Name = "Bob";
    hello(G, "Bob"); CHECK(getType(G, NP_WELCOME, b)); hello(H, "Cris"); CHECK(getType(H, NP_WELCOME, b));
    CHECK(!nickEq(p2Nick(), "Bob") && nickValid(p2Nick()));
    CHECK(cmdNick2Change("cris") != "");
    printf("[chat: so com o proprio nick]\n");
    drain(H); chatLog.clear();
    say(G, 1, "<Bob> oi"); CHECK(chatHas("<Bob> oi")); CHECK(getType(H, NP_CHAT, b));
    drain(H); chatLog.clear();
    say(G, 1, "<Cris> eu sou o Cris"); CHECK(!chatHas("<Cris> eu sou o Cris")); CHECK(!getType(H, NP_CHAT, b));   // forjado: descartado, nao chega ao host nem aos outros
    say(G, 1, "mensagem sem nick"); CHECK(!chatHas("mensagem sem nick"));
    say(G, 1, "<Bob> "); CHECK(!chatHas("<Bob> "));
    say(G, 1, "<Bob> a\nb"); CHECK(!chatHas("<Bob> a\nb"));
    say(G, 2, "<Bob> id falso"); CHECK(!chatHas("<Bob> id falso"));   // id de remetente diferente do da conexao
    say(G, 1, "<Bob> /me teste"); CHECK(chatHas("<Bob> /me teste"));
    CHECK(netChatOk(1, "<Bob> x") && !netChatOk(1, "<bob> x") && !netChatOk(2, "<Bob> x") && netChatOk(2, "<Cris> x"));
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
