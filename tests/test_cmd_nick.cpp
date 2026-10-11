// Parte 3A: /list e /nick (chat_debug.inc + net.inc). Roda o main.cpp REAL, sem rede de verdade.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static std::string last() { return chatLog.empty() ? "" : chatLog.back().s; }
static void run(const std::string& l) { Phys P{}; execCommand(l, P); }
int main() {
    chdir("/tmp");
    gPlayerName = "Zeh";
    run("/list"); CHECK(last() == "Há 1 jogador online: Zeh");
    run("/nick Zeh_9"); CHECK(gPlayerName == "Zeh_9" && last() == "Seu nick agora é Zeh_9");
    run("/nick Zé"); CHECK(gPlayerName == "Zeh_9" && last().find("inválido") != std::string::npos);
    run("/nick 12345678901234567"); CHECK(gPlayerName == "Zeh_9");
    run("/nick"); CHECK(last().find("Uso: /nick") == 0);
    run("/nick a b"); CHECK(gPlayerName == "Zeh_9" && last().find("Uso: /nick") == 0);
    run("/lista"); CHECK(last().find("Zeh_9") != std::string::npos);
    // host com 2 convidados: /list mostra todos; /nick e recusado em rede
    netSock = socket(AF_INET, SOCK_DGRAM, 0); netHost = true; netOn = true;
    sockaddr_in a{}; peers.push_back({a, 1000, 1, {}, true, "Alex"}); peers.push_back({a, 1000, 2, {}, true, "Steve"});
    run("/list"); CHECK(last() == "Há 3 jogadores online: Zeh_9, Alex, Steve");
    run("/nick Novo"); CHECK(gPlayerName == "Zeh_9" && last().find("rede") != std::string::npos);
    peers.clear(); close(netSock); netSock = -1; netHost = false; netOn = false;
    run("/nick Novo"); CHECK(gPlayerName == "Novo");
    FILE* f = fopen("profile.cfg", "r"); char b[64] = {0}; if (f) { fgets(b, sizeof b, f); fclose(f); } CHECK(std::string(b) == "name=Novo\n");
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
