// Clima (weather.inc): ciclo, chuva enche caldeirao com ceu aberto, teto impede, ceu escurece.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 8; z++) for (int x = -8; x < 8; x++) for (int y = 199; y < 256; y++) setB(x, y, z, y == 199 ? STONE : AIR);
    gDim = 0; netOn = false; gWeather = 1; gWxLeft = 1000000; gRain = 0; gThunder = 0;
    setBM(2, 200, 2, XB_DECOR, 0); setBM(-3, 200, -3, XB_DECOR, 0); setB(-3, 205, -3, STONE);   // aberto e com teto
    for (int i = 0; i < 16000; i++) { gWorldTime = i; wxTick(.5, 200, .5); }
    CHECK(gRain > .99f && gThunder < .01f);
    const bool cold = wxCold(.5, .5);
    if (!cold) { CHECK(((getM(2, 200, 2) >> 2) & 3) == 3); } else CHECK(((getM(2, 200, 2) >> 2) & 3) == 0);
    CHECK(((getM(-3, 200, -3) >> 2) & 3) == 0);   // com teto: nao enche
    CHECK(getB(2, 200, 2) == XB_DECOR && (getM(2, 200, 2) & 3) == 0);
    float top[3] = {.27f, .5f, .9f}, hor[3] = {.74f, .86f, 1.f}; gSkyScale = 1.f; wxSkyApply(top, hor);
    CHECK(hor[0] < .74f && top[2] < .9f && gSkyScale < 1.f);
    gWeather = 0; for (int i = 0; i < 1500; i++) wxTick(.5, 200, .5); CHECK(gRain < .01f);   // limpa: chuva some
    gWeather = 2; gWxLeft = 1000000; for (int i = 0; i < 1500; i++) wxTick(.5, 200, .5); CHECK(gThunder > .9f);
    gWeather = 1; gWxLeft = 1; wxTick(.5, 200, .5); CHECK(gWeather == 0 && gWxLeft >= 12000);   // timer expirou: volta ao limpo
    gDim = 1; for (int i = 0; i < 200; i++) wxTick(.5, 200, .5); CHECK(gRain == 0.f); gDim = 0;
    printf("test_weather: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
