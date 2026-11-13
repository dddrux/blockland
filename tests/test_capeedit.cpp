// patchCapa.4: editor de capa (capeCustomApply): gera assets/capes/Personalizada.png, seleciona, recarrega com as cores/padrao pedidos
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const uint8_t* px(const CapeAnim& a, int x, int y) { return &a.fr[0][((size_t)y * CAPE_W + x) * 4]; }   // x,y em pixels do buffer 128x64
int main() {
    (void)!system("rm -rf /tmp/capeed_t && mkdir -p /tmp/capeed_t/assets/capes"); chdir("/tmp/capeed_t");
    gSl["cm.r"] = 10 / 255.f; gSl["cm.g"] = 20 / 255.f; gSl["cm.b"] = 30 / 255.f; gSl["ca.r"] = 200 / 255.f; gSl["ca.g"] = 210 / 255.f; gSl["ca.b"] = 220 / 255.f;
    gSt["cep"] = 0; CHECK(capeCustomApply());
    CHECK(access("assets/capes/Personalizada.png", F_OK) == 0);
    CHECK(gsi("capesel", 0) > CAPE_NBI); CHECK(gCapeSelCur == gsi("capesel", 0)); CHECK(!gCapeMine.fr.empty() && !gCapeMyEnc.empty());
    CapeAnim a; CHECK(capeFromFile("assets/capes/Personalizada.png", a)); CHECK(a.fr.size() == 1 && a.fr[0].size() == (size_t)CAPE_FB);
    if (!a.fr.empty()) {
        const uint8_t* f = px(a, 2 + 10, 2 + 16);   // frente, centro (lisa = cor principal)
        CHECK(f[0] == 10 && f[1] == 20 && f[2] == 30 && f[3] == 255);
        const uint8_t* in = px(a, 24 + 10, 2 + 16);   // face de dentro = 55% da principal
        CHECK(in[0] == 10 * 55 / 100 && in[2] == 30 * 55 / 100);
        const uint8_t* e = px(a, 2 + 5, 0);   // borda superior = cor de detalhe
        CHECK(e[0] == 200 && e[1] == 210 && e[2] == 220);
    }
    gSt["cep"] = 2; CHECK(capeCustomApply());   // faixa central: x 8..11 = detalhe, fora = principal
    CapeAnim b; CHECK(capeFromFile("assets/capes/Personalizada.png", b));
    if (!b.fr.empty()) { CHECK(px(b, 2 + 9, 2 + 5)[0] == 200); CHECK(px(b, 2 + 3, 2 + 5)[0] == 10); }
    gSt["cep"] = 6; CHECK(capeCustomApply());   // degrade: topo = principal, base = detalhe
    CapeAnim c; CHECK(capeFromFile("assets/capes/Personalizada.png", c));
    if (!c.fr.empty()) { CHECK(px(c, 2 + 5, 2 + 0)[0] == 10); CHECK(px(c, 2 + 5, 2 + 31)[0] == 200); }
    CHECK(gCapeFiles.size() == 1);   // reaplicar nao duplica
    printf("test_capeedit: %d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
