// Sistema de modulos: registro, ganchos e comando. Inclui o modulo de EXEMPLO de proposito (o jogo real so o liga em modules_list.inc).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#include "../src/modules/exemplo/exemplo.inc"   // registra "exemplo" na inicializacao estatica (antes do main)
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
SDL_mutex* SDL_CreateMutex() { return (SDL_mutex*)1; } int SDL_LockMutex(SDL_mutex*) { return 0; } int SDL_UnlockMutex(SDL_mutex*) { return 0; }   // monitor (modulo real) usa mutex
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static int gI = 0, gT = 0, gS = 0, gH = 0, gW = 0;
int main() {
    SECTION("M1 registro estatico e ordem");
    const size_t n0 = gModules().size(); bool hasEx = false; for (auto& m : gModules()) hasEx |= std::string(m.name) == "exemplo"; CHECK(n0 >= 1 && hasEx);   // exemplo + modulos reais (monitor)
    static const GameModule B = [] { GameModule m; m.name = "b"; m.onInit = [] { gI++; }; m.onTick = [](Phys&) { gT++; }; m.onDrawWorld = [](float) { gW++; };
        m.onDrawHud = [](int, int) { gH++; }; m.onShutdown = [] { gS++; };
        m.onCommand = [](const std::string& c, const std::vector<std::string>& a, Phys&) { return c == "bcmd" && a.size() == 2 && a[0] == "x" && a[1] == "y"; }; return m; }();
    ModuleReg r(B); CHECK(gModules().size() == n0 + 1);

    SECTION("M2 ganchos chamam todos os modulos (os que deixaram nullptr sao ignorados)");
    GameModule bare; bare.name = "vazio"; ModuleReg r2(bare);   // nenhum gancho: nao pode quebrar
    Phys P; memset(&P, 0, sizeof P);
    modulesInit(); modulesTick(P); modulesTick(P); modulesDrawWorld(.5f); modulesDrawHud(10, 10); modulesShutdown();
    CHECK(gI == 1 && gT == 2 && gW == 1 && gH == 1 && gS == 1);
    CHECK(mod_exemplo::ticks == 2);   // o exemplo contou os 2 ticks (onInit zerou antes)

    SECTION("M3 comandos: modulo trata o seu, o resto cai no 'desconhecido'");
    CHECK(modulesCommand("bcmd", {"x", "y"}, P));
    CHECK(!modulesCommand("bcmd", {"x"}, P));
    CHECK(!modulesCommand("nada", {}, P));
    CHECK(modulesCommand("exemplo", {}, P));
    const size_t before = chatLog.size(); execCommand("/exemplo", P); CHECK(chatLog.size() == before + 1);       // via execCommand real
    const size_t b2 = chatLog.size(); execCommand("/cmdquenaoexiste", P); CHECK(chatLog.size() == b2 + 1);       // ainda mostra o erro
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
