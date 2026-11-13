// patchCapa.1/.2: capas — decodificador GIF (contra quadros do Pillow), capas prontas, PNG/GIF -> capa, compactacao de rede (RGB332+PackBits),
// NP_CAPE (host guarda/repassa, validacao do dono, fila paginada, esquecer ao sair). Fixtures: /tmp/cape_t (gerados pelo run_cape.sh)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static Uint32 gFakeT = 1000;
Uint32 SDL_GetTicks() { return gFakeT; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int udp() { const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); netSockNonBlock(s); return s; }
static int recvWait(int s, uint8_t* b, int n, int ms = 200) { for (int i = 0; i < ms; i++) { int r = (int)recvfrom(s, (char*)b, n, 0, nullptr, nullptr); if (r > 0) return r; usleep(1000); } return -1; }
static void pump(int n = 6) { for (int i = 0; i < n; i++) { gFakeT += 20; netUpdate(0.02f); usleep(1500); } }
static void drain(int s) { uint8_t t[2048]; while (recvfrom(s, (char*)t, sizeof t, 0, nullptr, nullptr) > 0) {} }
static sockaddr_in hg() { sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons(47933); return a; }
static void hello(int s, const std::string& nm) { sockaddr_in a = hg(); NBuf h; h.put<uint8_t>(NP_HELLO); h.put<uint8_t>(255); h.put<uint8_t>(NET_VER); h.put<uint8_t>(0); h.put<uint8_t>((uint8_t)nm.size()); for (char c : nm) h.put<uint8_t>((uint8_t)c); h.put<uint8_t>(1); sendto(s, (char*)h.d, h.n, 0, (sockaddr*)&a, sizeof a); pump(); }
static std::vector<uint8_t> slurp(const char* p) { std::vector<uint8_t> d; FILE* f = fopen(p, "rb"); if (!f) return d; uint8_t b[4096]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); fclose(f); return d; }
static void sendCape(int s, int sender, int owner, int kind, int id, int gen, const std::vector<uint8_t>& enc) {   // como um cliente: NP_CAPE em pacotes de CAPE_CH
    sockaddr_in a = hg(); const int nch = kind == 2 ? (int)((enc.size() + CAPE_CH - 1) / CAPE_CH) : 0;
    for (int i = 0; i < std::max(1, nch); i++) { NBuf b; b.put<uint8_t>(NP_CAPE); b.put<uint8_t>((uint8_t)sender); b.put<uint8_t>((uint8_t)owner); b.put<uint8_t>((uint8_t)kind); b.put<uint8_t>((uint8_t)id); b.put<uint8_t>((uint8_t)gen);
        const int len = kind == 2 ? std::min<int>(CAPE_CH, (int)enc.size() - i * CAPE_CH) : 0; b.put<uint16_t>((uint16_t)nch); b.put<uint16_t>((uint16_t)i); b.put<uint16_t>((uint16_t)len); for (int k = 0; k < len; k++) b.put<uint8_t>(enc[(size_t)i * CAPE_CH + k]);
        sendto(s, (char*)b.d, b.n, 0, (sockaddr*)&a, sizeof a); if (i % 8 == 7) pump(2); }
    pump(10);
}
static int countCape(int s, int owner) { int n = 0; uint8_t b[2048]; for (;;) { if (recvWait(s, b, sizeof b, 60) < 1) break; if (b[0] == NP_CAPE && b[2] == owner) n++; } return n; }
int main() {
    if (access("/tmp/cape_t/anim.gif", F_OK) != 0) (void)!system("python3 tools/gen_cape_fixture.py /tmp/cape_t 2>/dev/null || python3 ../tools/gen_cape_fixture.py /tmp/cape_t");   // patchPend.7: fixture do Pillow
    chdir("/tmp");
    printf("[GIF: decodificador contra o Pillow]\n");
    { GifImg g; CHECK(gifLoadFile("/tmp/cape_t/anim.gif", g)); CHECK(g.w == 40 && g.h == 30 && g.fr.size() == 4);
      const auto raw = slurp("/tmp/cape_t/anim.raw"); CHECK(raw.size() == 4 * 40 * 30 * 4);
      for (int f = 0; f < 4 && f < (int)g.fr.size(); f++) { int bad = 0; for (int i = 0; i < 40 * 30; i++) for (int c = 0; c < 3; c++) if (g.fr[f][i * 4 + c] != raw[(size_t)f * 4800 + i * 4 + c]) bad++; CHECK(bad == 0); }
      CHECK(g.delay.size() == 4 && g.delay[0] == 50 && g.delay[1] == 120 && g.delay[2] == 300 && g.delay[3] == 40);
      GifImg t; CHECK(gifLoadFile("/tmp/cape_t/trans.gif", t)); const auto tr = slurp("/tmp/cape_t/trans.raw");
      int bad = 0; for (int i = 0; i < 16 * 16; i++) if ((t.fr[0][i * 4 + 3] != 0) != (tr[i * 4 + 3] != 0)) bad++; CHECK(bad == 0);
      std::vector<uint8_t> junk = {'G', 'I', 'F', '8', '9', 'a', 5}; GifImg j; CHECK(!gifDecode(junk, j)); std::vector<uint8_t> cut = slurp("/tmp/cape_t/anim.gif"); cut.resize(cut.size() / 2); GifImg c2; (void)gifDecode(cut, c2); CHECK(c2.fr.size() < 4); }
    printf("[capas prontas]\n");
    for (int id = 1; id <= CAPE_NBI; id++) { CapeAnim a; capeBuiltin(id, a); CHECK(!a.fr.empty() && a.fr[0].size() == (size_t)CAPE_FB && a.ms.size() == a.fr.size());
        bool opaque = true; for (int y = 2; y < 34; y++) for (int x = 2; x < 22; x++) if (a.fr[0][((size_t)y * CAPE_W + x) * 4 + 3] != 255) opaque = false; CHECK(opaque);   // face de fora toda preenchida
        CHECK(((id == 7 || id == 8) ? a.fr.size() > 1 : a.fr.size() == 1));
        if (getenv("CAPE_DUMP")) { char p[64]; snprintf(p, sizeof p, "/tmp/cape_%d.raw", id); FILE* f = fopen(p, "wb"); fwrite(a.fr[0].data(), 1, CAPE_FB, f); fclose(f); } }
    { CapeAnim a, b; capeBuiltin(2, a); capeBuiltin(2, b); CHECK(a.fr[0] == b.fr[0]); capeBuiltin(1, b); CHECK(a.fr[0] != b.fr[0]);   // comunismo != vermelha lisa
      int gold = 0; for (int i = 0; i < CAPE_W * CAPE_H; i++) if (a.fr[0][i * 4] > 240 && a.fr[0][i * 4 + 1] > 190 && a.fr[0][i * 4 + 2] < 60) gold++; CHECK(gold > 60); }   // ha emblema dourado
    printf("[arquivo -> capa]\n");
    { CapeAnim a; CHECK(capeFromFile("/tmp/cape_t/anim.gif", a)); CHECK(a.fr.size() == 4 && a.ms.size() == 4); uint32_t T = 0; for (auto m : a.ms) T += m; CHECK(T >= 400 && T <= 600);
      CapeAnim p; CHECK(capeFromFile("/tmp/cape_t/plain.png", p)); CHECK(p.fr.size() == 1 && p.fr[0][((size_t)10 * CAPE_W + 10) * 4 + 2] > 150);   // azul na face de fora
      CapeAnim l; CHECK(capeFromFile("/tmp/cape_t/layout.png", l)); CHECK(l.fr[0][((size_t)6 * CAPE_W + 6) * 4] == 255 && l.fr[0][((size_t)50 * CAPE_W + 100) * 4 + 3] == 0);   // layout 64x32 usado como esta (x2)
      CHECK(!capeFromFile("/tmp/cape_t/nao_existe.png", p)); }
    printf("[compactacao de rede]\n");
    { std::vector<uint8_t> in, out, back; for (int i = 0; i < 5000; i++) in.push_back((uint8_t)((i / 37) % 3 == 0 ? 7 : i * 13)); capeRle(in, out); CHECK(capeUnRle(out.data(), out.size(), back, in.size()) && back == in);
      CHECK(!capeUnRle(out.data(), out.size(), back, 100)); uint8_t bad[3] = {200, 0, 5}; CHECK(!capeUnRle(bad, 1, back, 10));
      CapeAnim a, b; capeBuiltin(7, a); std::vector<uint8_t> enc; capePack(a, enc); CHECK(enc.size() < (size_t)8 * CAPE_W * CAPE_H); CHECK(capeUnpack(enc.data(), enc.size(), b) && b.fr.size() == a.fr.size() && b.ms == a.ms);
      int worst = 0; for (size_t i = 0; i < a.fr[0].size(); i += 4) for (int c = 0; c < 3; c++) worst = std::max(worst, abs((int)a.fr[0][i + c] - (int)b.fr[0][i + c])); CHECK(worst <= 40);   // RGB332: erro de quantizacao limitado
      CapeAnim g; CHECK(capeFromFile("/tmp/cape_t/anim.gif", g)); std::vector<uint8_t> e2; capePack(g, e2); printf("  (gif 4 quadros = %zu bytes, %zu pacotes)\n", e2.size(), (e2.size() + CAPE_CH - 1) / CAPE_CH);
      CHECK(!capeUnpack(e2.data(), e2.size() / 2, b)); }
    printf("[rede: host guarda, valida e repassa]\n");
    const int G = udp(), H = udp();
    CHECK(netStartHost(47933)); gPlayerName = "Alex"; netOn = true;
    hello(G, "Zeh"); drain(G);
    CapeAnim gif; capeFromFile("/tmp/cape_t/anim.gif", gif); std::vector<uint8_t> enc; capePack(gif, enc);
    sendCape(G, 1, 1, 1, 2, 5, {}); CHECK(gCapes.count(1) && gCapes[1].done && gCapes[1].kind == 1 && gCapes[1].id == 2 && gCapes[1].anim.fr.size() == 1);   // pronta (comunismo)
    sendCape(G, 1, 1, 2, 0, 6, enc); CHECK(gCapes[1].kind == 2 && gCapes[1].anim.fr.size() == 4 && gCapes[1].enc == enc && gCapes[1].gen == 6);                    // GIF em varios pacotes
    sendCape(G, 1, 0, 1, 3, 1, {}); CHECK(!gCapes.count(0));                  // convidado nao manda capa em nome do host
    sendCape(G, 1, 2, 1, 3, 1, {}); CHECK(!gCapes.count(2));                  // nem de outro convidado
    sendCape(G, 1, 1, 1, 99, 7, {}); CHECK(gCapes[1].kind == 2);             // id invalido: ignora
    { std::vector<uint8_t> half(enc.begin(), enc.begin() + CAPE_CH); sendCape(G, 1, 1, 2, 0, 9, half); /* nch=1 mas dados incompletos: falha ao decodificar */ CHECK(gCapes[1].gen == 6 && gCapes[1].anim.fr.size() == 4); }
    sendCape(G, 1, 1, 0, 0, 10, {}); CHECK(gCapes[1].kind == 0 && gCapes[1].anim.fr.empty() && gCapes[1].done);   // tirou a capa
    sendCape(G, 1, 1, 2, 0, 11, enc);
    printf("[quem entra depois recebe; reenvio periodico; fila paginada]\n");
    drain(H); hello(H, "Zed"); pump(60); CHECK(countCape(H, 1) >= (int)((enc.size() + CAPE_CH - 1) / CAPE_CH));
    gCapeSelCur = 3; gCapeMyEnc.clear(); gCapeGen = 4; drain(H); gFakeT += 31000; netUpdate(0.05f); pump(20); CHECK(countCape(H, 0) >= 1);   // host reenvia a PROPRIA capa (pronta id 3)
    { CapeAnim m; capeFromFile("/tmp/cape_t/anim.gif", m); gCapeMine = m; gCapeMine.finish(); capePack(gCapeMine, gCapeMyEnc); gCapeSelCur = CAPE_NBI + 1; for (int i = 0; i < 3; i++) netCapeSendMine(nullptr); CHECK(gCapeQ.size() <= (gCapeMyEnc.size() + CAPE_CH - 1) / CAPE_CH);   // nao empilha versoes antigas do mesmo dono
      pump(1); CHECK(gCapeQ.size() + 6 >= (gCapeMyEnc.size() + CAPE_CH - 1) / CAPE_CH || gCapeQ.empty()); }
    printf("[esquecer ao sair + lado convidado]\n");
    netSkinForget(1); CHECK(!gCapes.count(1));
    netStop(); CHECK(gCapes.empty() && gCapeQ.empty() && gCapeT == 0);
    netOn = true; netHost = false; netSelf = 2;
    { NBuf b = netMk(NP_CAPE); b.put<uint8_t>(1); b.put<uint8_t>(1); b.put<uint8_t>(2); b.put<uint8_t>(1); b.put<uint16_t>(0); b.put<uint16_t>(0); b.put<uint16_t>(0); b.r = 2; netCapeRecv(0, b); CHECK(gCapes.count(1) && gCapes[1].kind == 1);   // do host, em nome do 1: vale
      NBuf c = netMk(NP_CAPE); c.put<uint8_t>(2); c.put<uint8_t>(1); c.put<uint8_t>(3); c.put<uint8_t>(1); c.put<uint16_t>(0); c.put<uint16_t>(0); c.put<uint16_t>(0); c.r = 2; netCapeRecv(0, c); CHECK(!gCapes.count(2));   // a minha: ignora
      NBuf d = netMk(NP_CAPE); d.put<uint8_t>(1); d.put<uint8_t>(1); d.put<uint8_t>(5); d.put<uint8_t>(2); d.put<uint16_t>(0); d.put<uint16_t>(0); d.put<uint16_t>(0); d.r = 2; netCapeRecv(3, d); CHECK(gCapes[1].id == 2);   // convidado 3 falando pelo 1: ignora
      NBuf e; e.put<uint8_t>(NP_CAPE); e.put<uint8_t>(0); e.r = 2; netCapeRecv(0, e); CHECK(gCapes[1].id == 2); }   // pacote curto
    netStop();
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
