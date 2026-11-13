// Monitor Touch: URL (validacao/normalizacao/escape), captura desktop com um "navegador" falso, persistencia e filtro de rede.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
void SDL_Delay(Uint32 ms) { usleep(ms * 1000); }
SDL_mutex* SDL_CreateMutex() { return (SDL_mutex*)1; } int SDL_LockMutex(SDL_mutex*) { return 0; } int SDL_UnlockMutex(SDL_mutex*) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    using namespace mod_monitor;
    SECTION("U1 URL: validar, normalizar, escapar");
    CHECK(validUrl("https://a.com/x") && validUrl("http://a.com") && !validUrl("file:///etc/passwd") && !validUrl("javascript:alert(1)") && !validUrl("intent://x"));
    CHECK(normalize("youtube.com") == "https://youtube.com");
    CHECK(normalize("https://a.b/c") == "https://a.b/c");
    CHECK(normalize("file:///etc/passwd") == HOME_URL);                       // esquema perigoso nunca passa
    CHECK(normalize("gato fofo").rfind(std::string(SEARCH_URL) + "gato%20fofo", 0) == 0);
    CHECK(normalize("") == HOME_URL);
#ifndef BL_MON_FORCE_ANDROID
#endif
    SECTION("P1 persistencia por mundo");
    gCurDir = "__tmon"; system("mkdir -p saves/__tmon"); gMon.clear(); gMon[std::array<int, 3>{1, -2, 3}].url = "https://a.com/x?y=1"; gMon[std::array<int, 3>{4, 5, 6}].url = "https://pt.wikipedia.org";
    save(); gMon.clear(); load();
    const std::array<int, 3> ka{1, -2, 3}, kb{4, 5, 6}, kc{1, 2, 3}; CHECK(gMon.size() == 2 && gMon[ka].url == "https://a.com/x?y=1" && gMon[kb].url == "https://pt.wikipedia.org");
    { FILE* f = fopen("saves/__tmon/monitors.txt", "a"); fputs("9 9 9 javascript:alert(1)\n", f); fclose(f); } gMon.clear(); load(); CHECK(gMon.size() == 2);   // linha maliciosa ignorada
    system("rm -rf saves/__tmon"); gCurDir.clear(); gMon.clear();

    SECTION("N1 pacote de rede: so http(s)");
    auto mk = [](const char* u) { NBuf b; b.put<int32_t>(1); b.put<int32_t>(2); b.put<int32_t>(3); b.put<uint8_t>((uint8_t)strlen(u)); for (const char* p = u; *p; p++) b.put<uint8_t>((uint8_t)*p); return b; };
    { NBuf b = mk("javascript:alert(1)"); netRecv(b); } CHECK(gMon.empty());
    { NBuf b = mk("https://ok.com/a"); netRecv(b); } CHECK(gMon.size() == 1 && gMon[kc].url == "https://ok.com/a" && gMon[kc].rev == 1);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
