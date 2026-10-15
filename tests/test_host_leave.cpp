// patch5.69: host sai/cai -> convidado perde todos os remotos; pausa com LAN aberta. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    remotes[0]; remotes[1]; remotes[2]; gNetNames[0] = "host"; gNetNames[1] = "a"; gNetNames[2] = "b";
    CHECK(remotes.size() == 3);
    netDropAllRemotes();
    CHECK(remotes.empty()); CHECK(gNetNames.empty());
    netDropAllRemotes(); CHECK(remotes.empty());   // idempotente (BYE repetido)
    CHECK(GP_DEF[GA_PREV] == GB_LEFT && GP_DEF[GA_NEXT] == GB_RIGHT && gGpMap[GA_PREV] == GB_LEFT);   // hotbar sem LB/RB
    CHECK(GP_DEF[GA_ATTACK] == GB_RT && GP_DEF[GA_USE] == GB_LT);   // quebrar/colocar continuam nos gatilhos
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
