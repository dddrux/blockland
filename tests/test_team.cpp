// Times (/team): comandos em solo, copia pelo NP_ROSTER no convidado, cor e ordem do Tab. Roda o main.cpp REAL, sem rede de verdade.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static std::string last() { return chatLog.empty() ? "" : chatLog.back().s; }
static bool anyLine(const std::string& s) { for (const auto& l : chatLog) if (l.s.find(s) != std::string::npos) return true; return false; }
static void run(const std::string& l) { Phys P{}; execCommand(l, P); }
static void putS(NBuf& r, const std::string& s) { r.put<uint8_t>((uint8_t)s.size()); for (char c : s) r.put<uint8_t>((uint8_t)c); }
int main() {
    chdir("/tmp"); gPlayerName = "Zeh"; netSelf = 0;
    printf("[comandos em solo]\n");
    run("/team add Red red"); CHECK(gTeams.size() == 1 && gTeams[0].col == 12 && last() == "Time Red criado");
    run("/team add Red"); CHECK(gTeams.size() == 1 && last().find("Já existe") == 0);
    run("/team add Ruim! red"); CHECK(gTeams.size() == 1 && last().find("inválido") != std::string::npos);
    run("/team add Blue azul"); CHECK(gTeams.size() == 2 && gTeams[1].col == 9);
    run("/team color Blue verde"); CHECK(gTeams[1].col == 10);
    run("/team color Blue cor_que_nao_existe"); CHECK(gTeams[1].col == 10 && last().find("Cor desconhecida") == 0);
    run("/team join Nope"); CHECK(last().find("Time inexistente") == 0);
    run("/team join Red Ninguem"); CHECK(last().find("Jogador não encontrado") == 0);
    run("/team join red"); CHECK(teamStrOf(0) == "Red" && teamRgbOf(0) == 0xFF5555 && last() == "Zeh entrou no time Red");   // nome do time sem diferenciar maiuscula
    run("/team list"); CHECK(anyLine("Time Red (red): Zeh") && anyLine("Time Blue (green): sem membros"));
    run("/team leave"); CHECK(teamStrOf(0).empty() && teamRgbOf(0) == 0xFFFFFF && last() == "Zeh saiu do time");
    run("/team leave"); CHECK(last().find("não está em nenhum time") != std::string::npos);
    run("/team join Blue"); run("/team remove Blue"); CHECK(gTeams.size() == 1 && teamStrOf(0).empty());   // remover o time tira os membros
    run("/team"); CHECK(anyLine("Time Red"));
    printf("[convidado: o roster traz times e membros]\n");
    netHost = false; netOn = true; netConn = true; netSelf = 2; gNetNames.clear();
    NBuf r = netMk(NP_ROSTER); r.put<uint8_t>(3);
    auto ent = [&](int id, const std::string& n, int pg, const std::string& tm) { r.put<uint8_t>((uint8_t)id); putS(r, n); r.put<uint16_t>((uint16_t)pg); putS(r, tm); };
    ent(0, "Host", 0, ""); ent(1, "ana", 45, "Red"); ent(2, "Zeh", 10, "");
    r.put<uint8_t>(1); putS(r, "Red"); r.put<uint8_t>(12);
    r.d[1] = 0; r.r = 2; netNameRecv(NP_ROSTER, 0, r);
    CHECK(gTeams.size() == 1 && gTeams[0].name == "Red" && gTeams[0].col == 12);
    CHECK(teamStrOf(1) == "Red" && teamRgbOf(1) == 0xFF5555 && teamStrOf(0).empty() && !teamSame(2, 1));
    printf("[Tab: sem time primeiro, depois por time, cada um em ordem alfabetica; nome na cor do time]\n");
    { std::vector<TabEntry> v; tabBuild(v); CHECK(v.size() == 3 && v[0].name == "Host" && v[1].name == "Zeh" && v[2].name == "ana" && v[2].rgb == 0xFF5555 && v[0].rgb == 0xFFFFFF); }
    gTeamOfId[2] = "Red"; CHECK(teamSame(2, 1));   // agora eu tambem estou no time Red: nametag dele aparece atras de paredes
    { std::vector<TabEntry> v; tabBuild(v); CHECK(v.size() == 3 && v[0].name == "Host" && v[1].name == "ana" && v[2].name == "Zeh"); }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
