// test_keybinds: teclas de controle configuraveis (patchKeys.1). Rodar: tools/t.sh keybinds
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode k) { return (SDL_Scancode)(k & 0xFFF); }   // stub: sym == scancode
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode s) { return (SDL_Keycode)s; }
const char* SDL_GetScancodeName(SDL_Scancode s) { static char b[16]; snprintf(b, sizeof b, "K%d", (int)s); return b; }
SDL_Scancode SDL_GetScancodeFromName(const char* n) { return n[0] == 'K' ? (SDL_Scancode)atoi(n + 1) : (SDL_Scancode)0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    fs::remove_all("/tmp/blkb"); fs::create_directories("/tmp/blkb"); if (chdir("/tmp/blkb") != 0) return 99;
    kbLoad(); CHECK(!gKbAny); CHECK(kbSym(SDL_SCANCODE_W) == SDL_SCANCODE_W);
    int fwd = -1, jump = -1, sneak = -1; for (int a = 0; a < KB_N; a++) { if (!strcmp(KB_ACTS[a].id, "fwd")) fwd = a; if (!strcmp(KB_ACTS[a].id, "jump")) jump = a; if (!strcmp(KB_ACTS[a].id, "sneak")) sneak = a; }
    kbSet(fwd, SDL_SCANCODE_UP);   // andar para frente na seta: seta age como W, W fica sem funcao
    CHECK(gKbAny); CHECK(kbSym(SDL_SCANCODE_UP) == SDL_SCANCODE_W); CHECK(kbSym(SDL_SCANCODE_W) == 0);
    kbSet(fwd, SDL_SCANCODE_SPACE);   // tecla ja usada por Pular: troca (Pular vai para a seta)
    CHECK(gKbBind[jump] == SDL_SCANCODE_UP); CHECK(kbSym(SDL_SCANCODE_SPACE) == SDL_SCANCODE_W); CHECK(kbSym(SDL_SCANCODE_UP) == SDL_SCANCODE_SPACE);
    { Uint8 s[512] = {0}, kk[512]; s[SDL_SCANCODE_SPACE] = 1; memcpy(kk, s, 512); kbState(kk, s); CHECK(kk[SDL_SCANCODE_W] == 1); CHECK(kk[SDL_SCANCODE_SPACE] == 0); }
    kbSet(sneak, SDL_SCANCODE_ESCAPE); CHECK(gKbBind[sneak] == SDL_SCANCODE_LSHIFT);   // Esc nao pode ser tecla
    gKbBind[fwd] = KB_ACTS[fwd].def; gKbBind[jump] = KB_ACTS[jump].def; kbRebuild(); kbSave(); kbSet(fwd, SDL_SCANCODE_F5 + 1); kbLoad();   // salvar/carregar
    CHECK(gKbBind[fwd] == SDL_SCANCODE_F5 + 1);
    kbResetAll(); CHECK(!gKbAny); CHECK(kbSym(SDL_SCANCODE_W) == SDL_SCANCODE_W);
    printf("test_keybinds: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
