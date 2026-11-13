// patchWx.8: trovao atrasado pela distancia + eventos de chuva/trovao (rodar da RAIZ com assets/sounds completo)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    const SndEvt* r = sndEvtGet("weather.rain"); const SndEvt* th = sndEvtGet("weather.thunder");
    CHECK(r && r->f.size() == 8 && th && th->f.size() == 3);
    creative = true; gBoltX = 10; gBoltZ = 0; gBoltY = 70; gBoltCd = 1;
    wxBoltStrike(10, 70, 0); CHECK(gThT == 0 && gThVol > 1.4f);          // raio colado: quase imediato e alto
    wxBoltStrike(210, 70, 0); CHECK(gThT == 40 && gThVol < .5f);         // 200 blocos: atrasa 40 ticks e fica baixo
    wxBoltStrike(1000, 70, 0); CHECK(gThT == 50 && gThVol >= .29f && gThVol <= .31f);
    gSndOn = false; gRain = 1.f; gDim = 0; wxRainSound(0, 70, 0); CHECK(gRainBuf == nullptr);   // sem audio: nada
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
