// patchSign.1: editor de texto da placa (linhas, limite de largura, backspace UTF-8), limpar ao ficar vazia, salvar/carregar.
#include <unistd.h>
#include <sys/stat.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0 || access("src/world/sign.inc", R_OK) != 0) { printf("SKIP\n"); return 0; }
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    setBM(3, 70, 3, XB_SIGN, 4);
    signOpen(3, 70, 3); CHECK(gSignEd);
    signText("Ola"); signKey(SDLK_RETURN); signText("mundo"); CHECK(gSignLine == 1);
    const SignT& s = gSignTxt[signKeyOf(3, 70, 3)];
    CHECK(s.l[0] == "Ola" && s.l[1] == "mundo");
    signKey(SDLK_BACKSPACE); CHECK(s.l[1] == "mund");
    signText("ç"); signKey(SDLK_BACKSPACE); CHECK(s.l[1] == "mund");   // apaga o caractere UTF-8 inteiro
    for (int i = 0; i < 30; i++) signText("WWWW"); CHECK(textWidth(s.l[1].c_str(), 1.f) <= 90.f);
    signKey(SDLK_UP); CHECK(gSignLine == 0); signKey(SDLK_ESCAPE); CHECK(!gSignEd);
    mkdir("saves", 0777); mkdir("saves/_tsign", 0777);
    signsSave("_tsign"); gSignTxt.clear(); signsLoad("_tsign");
    CHECK(gSignTxt.size() == 1); const SignT& t = gSignTxt[signKeyOf(3, 70, 3)];
    CHECK(t.x == 3 && t.y == 70 && t.l[0] == "Ola" && t.l[1] == s.l[1] && t.l[2].empty());
    signOpen(5, 70, 5); signDone(); CHECK(gSignTxt.size() == 1);   // placa vazia nao fica guardada
    setB(3, 70, 3, AIR); signDrawWorld(3, 70, 3);   // sem GL: so poda quando o bloco some
    {   // rede: pacote -> aplica; pacote vazio -> remove
        std::array<std::string, 4> l = {"A", "B\xc3\xa7", "", "D"}; NBuf b = netMk(NP_SIGN); signNetPut(b, 9, 71, 9, &l);
        b.r = 0; b.get<uint8_t>(); b.get<uint8_t>(); signNetRecv(b);
        CHECK(gSignTxt.count(signKeyOf(9, 71, 9)) && gSignTxt[signKeyOf(9, 71, 9)].l[1] == "B\xc3\xa7" && gSignTxt[signKeyOf(9, 71, 9)].l[3] == "D");
        NBuf e = netMk(NP_SIGN); signNetPut(e, 9, 71, 9, nullptr); e.r = 0; e.get<uint8_t>(); e.get<uint8_t>(); signNetRecv(e);
        CHECK(!gSignTxt.count(signKeyOf(9, 71, 9)));
    }
    remove("saves/_tsign/signs.dat"); rmdir("saves/_tsign");
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
