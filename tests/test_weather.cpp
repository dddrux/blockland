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
    {   // patchRain.2: respingo (ParticleRain): nasce perto do jogador no topo do chao, pula e some; sem chuva nao nasce
        gDim = 0; gPerf = false; gOptParticles = true; gWxDrops.clear(); gWeather = 1; gWxLeft = 1000000; gRain = 1.f;
        int gy = 255; while (gy > 0 && getB(0, gy, 0) == AIR) gy--;
        for (int i = 0; i < 30; i++) wxTick(.5, gy + 2, .5);
        CHECK(!gWxDrops.empty() && gWxDrops.size() < 1500);
        bool ok = true; for (const WxDrop& d : gWxDrops) ok = ok && d.y >= d.gy - 1e-3f && d.spr >= 19 && d.spr <= 22 && d.sz >= .1f && d.sz <= .2f + 1e-4f;
        CHECK(ok);
        gWeather = 0; gRain = 0.f; for (int i = 0; i < 80; i++) wxTick(.5, gy + 2, .5); CHECK(gWxDrops.empty());   // sem chuva: as gotas acabam
        gWeather = 1;
    }
    {   // patchWx.6: operador convidado muda o clima no host; nao-operador e ignorado
        netOn = false; netHost = true; gOpIds.clear(); gOpIds.insert(0); gOpIds.insert(1); gWeather = 0; gWxLeft = 5; gWorldTime = 100; gDayCycle = true;
        auto pk = [&](int w, int cmd) { NBuf b = netMk(NP_TIME); b.put<int64_t>(500); b.put<uint8_t>(1 | (cmd ? 2 : 0) | ((w & 3) << 2)); b.r = 2; return b; };
        NBuf a = pk(2, 1); netTimeRecv(a, 1); CHECK(gWeather == 2 && gWxLeft >= 12000 && gWorldTime == 500);
        NBuf c = pk(0, 1); netTimeRecv(c, 2); CHECK(gWeather == 2);   // id 2 nao e operador
        NBuf d = pk(0, 0); netTimeRecv(d, 1); CHECK(gWeather == 2);   // sem bit de comando
        netHost = false;
    }
    {   // patchWx.7: raio fere mobs a <3 blocos (nao os distantes nem os mortos)
        netOn = false; creative = true; MOBS.clear(); gBoltCd = 1;
        MobEnt a, b, c; a.type = b.type = c.type = M_PIG; a.hp = b.hp = c.hp = 10; a.x = 10.5; a.y = 200; a.z = 10.5; b.x = 11.5; b.y = 200; b.z = 10.5; c.x = 20; c.y = 200; c.z = 20; MOBS.push_back(a); MOBS.push_back(b); MOBS.push_back(c);
        gBoltX = 10.5; gBoltY = 200; gBoltZ = 10.5; wxBoltStrike(50, 200, 50);
        CHECK(MOBS[0].hp == 5 && MOBS[1].hp == 5 && MOBS[2].hp == 10);
        MOBS.clear();
    }
    printf("test_weather: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
