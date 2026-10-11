// joystick estilo Bedrock: zona morta, 8 direcoes, correr na borda da frente, agachar no botao separado
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int SW = 2400, SH = 1080;
static void fing(int type, int64_t id, float x, float y) { SDL_Event e; memset(&e, 0, sizeof e); e.type = type; e.tfinger.fingerId = id; e.tfinger.x = x / SW; e.tfinger.y = y / SH; touchEvent(e, SW, SH); }
int main() {
    touchHud = true; touchReset(); const TLay L = touchLay(SW, SH); const float R = L.jR;
    fing(SDL_FINGERDOWN, 1, L.jcx, L.jcy); CHECK(!tvF && !tvB && !tvL && !tvR && !tvSprint);          // centro = parado
    fing(SDL_FINGERMOTION, 1, L.jcx + R * .1f, L.jcy - R * .1f); CHECK(!tvF);                          // zona morta
    fing(SDL_FINGERMOTION, 1, L.jcx, L.jcy - R * .6f); CHECK(tvF && !tvL && !tvR && !tvSprint);        // frente
    fing(SDL_FINGERMOTION, 1, L.jcx + R * .5f, L.jcy - R * .5f); CHECK(tvF && tvR && !tvL);            // diagonal frente-direita
    fing(SDL_FINGERMOTION, 1, L.jcx - R * .6f, L.jcy); CHECK(tvL && !tvF && !tvB);                     // esquerda
    fing(SDL_FINGERMOTION, 1, L.jcx - R * .5f, L.jcy + R * .5f); CHECK(tvB && tvL);                    // diagonal de tras tambem anda
    fing(SDL_FINGERMOTION, 1, L.jcx, L.jcy - R * 1.1f); CHECK(tvF && tvSprint);                        // borda da frente = correr
    fing(SDL_FINGERMOTION, 1, L.jcx + R * 1.1f, L.jcy); CHECK(tvR && !tvSprint);                       // borda lateral nao corre
    fing(SDL_FINGERUP, 1, 0, 0); CHECK(!tvF && !tvR && !tvSprint);                                   // soltou = para
    const bool s0 = tvSneakT; fing(SDL_FINGERDOWN, 2, L.sneak.x + 5, L.sneak.y + 5); CHECK(tvSneakT != s0); fing(SDL_FINGERUP, 2, 0, 0);
    fing(SDL_FINGERDOWN, 3, L.jump.x + 5, L.jump.y + 5); CHECK(tvJump); fing(SDL_FINGERUP, 3, 0, 0); CHECK(!tvJump);
    fing(SDL_FINGERDOWN, 4, L.jcx, L.jcy); fing(SDL_FINGERMOTION, 4, L.jcx, L.jcy - R * .8f); fing(SDL_FINGERDOWN, 5, L.jump.x + 5, L.jump.y + 5); CHECK(tvF && tvJump);   // multitoque
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
