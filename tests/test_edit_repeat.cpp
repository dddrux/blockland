// patch5.67: edicoes de bloco reenviadas 2x (UDP sem confirmacao). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    netSock = -1; netOn = false; gEditRepeat.clear(); netEdits.clear();
    netSendBlock(1, 100, 1, STONE, 0); CHECK(gEditRepeat.empty());   // offline: nada pendente
    netOn = true; netHost = true;
    netSendBlock(1, 100, 1, STONE, 0); netSendBlock(2, 100, 1, DIRT, 0); netSendBlock(3, 100, 1, STONE, 0); netSendBlock(3, 100, 1, DIRT, 5);   // a ultima edicao da celula 3 vale
    CHECK(gEditRepeat.size() == 3);
    CHECK(netEditRepeatTick(1000) == 3);       // 1o reenvio
    CHECK(netEditRepeatTick(1100) == 0);       // intervalo < 150 ms
    CHECK(netEditRepeatTick(1200) == 3);       // 2o reenvio
    CHECK(gEditRepeat.empty() && netEditRepeatTick(1400) == 0);   // acabou
    for (int i = 0; i < 200; i++) netSendBlock(i, 100, 5, STONE, 0);
    CHECK(netEditRepeatTick(2000) == 80);      // no maximo 80 por rodada
    CHECK(netEditRepeatTick(2200) == 80);
    netOn = false; netEditRepeatTick(2400); CHECK(gEditRepeat.empty());   // saiu do multiplayer: limpa
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
