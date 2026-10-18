// patchMus.1: decodifica discos/musicas novas (rodar da RAIZ do jogo)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
int main() {
    int bad = 0, n = 0;
    for (const char* f : {"records/13","records/11","records/cat","records/ward","music/game/hal1","music/game/piano2","music/game/nuance1","music/game/creative/creative3","music/game/nether/nether2","music/game/end/end"}) {
        SndBuf* b = sndGet(f); n++; if (!b || b->frames < 44100) { bad++; printf("FALHOU %s\n", f); } else printf("ok %s %d frames %d Hz\n", f, (int)b->frames, (int)b->rate);
    }
    printf("%d faixas, %d falhas\n", n, bad); return bad ? 1 : 0;
}
