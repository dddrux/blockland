// patchVoice.1: codec u-law, pacote NP_VOICE (envio/recepcao), jitter buffer, mixagem e proximidade
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 1000; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    // 1) u-law: ida e volta com erro pequeno e sinal preservado
    int worst = 0; for (int s = -32000; s <= 32000; s += 37) { const int d = vcDec(vcEnc(s)); worst = std::max(worst, std::abs(d - s) * 100 / std::max(256, std::abs(s))); CHECK((s >= 0) == (d >= 0) || std::abs(s) < 200); }
    CHECK(worst <= 7);   // <= 7% (u-law ~ 6 bits efetivos)
    CHECK(vcDec(vcEnc(0)) == 0 || std::abs(vcDec(vcEnc(0))) < 8);
    // 2) pacote: um quadro de 320 bytes cabe no NBuf e e lido de volta
    NBuf b = netMk(NP_VOICE); b.put<uint16_t>(5); for (int i = 0; i < VC_FRAME; i++) b.put<uint8_t>(vcEnc((int)(8000 * sinf(i * .3f))));
    CHECK(b.n == 2 + 2 + VC_FRAME && b.n < (int)sizeof b.d);
    netSelf = 0; NBuf r = b; r.r = 0; r.get<uint8_t>(); r.get<uint8_t>();
    gSndRate = 44100; gSndLX = gSndLY = gSndLZ = 0; gSndYaw = 0;
    voiceNetRecv(r, 3);                                   // remetente 3
    CHECK(gVcSpk.count(3) == 1 && !gVcSpk[3].active);     // 1 quadro: ainda enche o buffer (jitter)
    NBuf b2 = b; b2.r = 0; b2.get<uint8_t>(); b2.get<uint8_t>(); b2.d[2] = 6; voiceNetRecv(b2, 3);   // seq 6
    CHECK(gVcSpk[3].active && (int)gVcSpk[3].q.size() == VC_FRAME * 2);
    // 3) eco proprio e remetente invalido sao ignorados
    NBuf e = b; e.r = 0; e.get<uint8_t>(); e.get<uint8_t>(); voiceNetRecv(e, netSelf); voiceNetRecv(e, 1004); CHECK(gVcSpk.count(0) == 0 && gVcSpk.count(1004) == 0);
    // 4) mistura: gera som nos dois canais, consome 8000/44100 amostra por frame de saida
    RemotePl rp; rp.x = 5; rp.y = 0; rp.z = 0; remotes[3] = rp; gVoiceVol = 1.f; voiceTick(SDL_GetTicks());
    CHECK(fabsf(gVcSpk[3].gl - 1.f) < 1e-3f || gVcSpk[3].gl > 0.3f);
    std::vector<float> acc(2 * 441, 0.f); const size_t before = gVcSpk[3].q.size(); voiceMix(acc.data(), 441);
    float peak = 0; for (float v : acc) peak = std::max(peak, fabsf(v)); CHECK(peak > 500.f);
    const size_t used = before - gVcSpk[3].q.size(); CHECK(used >= 70 && used <= 90);   // 441 * 8000/44100 = 80
    // 5) distancia: longe (>48) = mudo; perto = volume cheio
    remotes[3].x = 100; voiceTick(SDL_GetTicks()); CHECK(gVcSpk[3].gl == 0.f && gVcSpk[3].gr == 0.f);
    remotes[3].x = 4; voiceTick(SDL_GetTicks()); CHECK(gVcSpk[3].gr > .99f && gVcSpk[3].gl > .3f && gVcSpk[3].gl < .5f);   // falante a direita: canal esquerdo cai (pan)
    // 6) fila nao cresce sem limite e esvazia ao acabar (volta a esperar o buffer)
    for (int i = 0; i < 40; i++) { NBuf c = b; c.r = 0; c.get<uint8_t>(); c.get<uint8_t>(); c.d[2] = (uint8_t)(10 + i); voiceNetRecv(c, 3); }
    CHECK((int)gVcSpk[3].q.size() <= VC_MAXQ);
    { std::vector<float> a2(2 * 4096, 0.f); for (int k = 0; k < 20; k++) voiceMix(a2.data(), 4096); } CHECK(!gVcSpk[3].active);
    // 7) sem rede o mic desliga sozinho
    gVoiceOn = true; netOn = false; voiceTick(SDL_GetTicks()); CHECK(!gVoiceOn);
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
