// patchBlk.3: criativo 1.12.2 com o pack Default-Java-1.12.2: aliases de nome 1.12->1.13 resolvem; madeiras de todos os tipos nas abas (precisa do pack)
// Auditoria: abre o pack ativo e lista texturas block/item que NAO estao ligadas a bloco/item do jogo (viram extras PK) e quais extras sao cubo.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FALHOU: %s\n", m); fails++; } } while (0)
static bool hasName(const char* n) { for (auto& v : gCreative) for (int id : v) if (!strcmp(itemName(id), n)) return true; return false; }
int main() {
    if (!gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("SKIP: pack Default-Java-1.12.2.zip ausente (assets/resourcepacks)\n"); return 0; }
    applyMcPack(); registerPackTab();
    CHECK(gCreative[0].size() >= 190, "BUILD >= 190"); CHECK(gCreative[1].size() >= 160, "DECO >= 160");
    int tot = 0; for (auto& v : gCreative) tot += (int)v.size(); CHECK(tot >= 640, "total >= 640");
    static const char* W[] = {"abeto", "bétula", "selva", "acácia", "carvalho escuro"};
    static const char* P[] = {"Tábuas de %s", "Tronco de %s", "Folhas de %s", "Muda de %s", "Laje de %s", "Escada de %s", "Cerca de %s", "Portão de %s", "Alçapão de %s", "Porta de %s", "Barco de %s", "Placa de pressão de %s"};
    for (const char* w : W) for (const char* p : P) { char b[96]; snprintf(b, sizeof b, p, w); if (!hasName(b)) { char m[160]; snprintf(m, sizeof m, "falta '%s' (variante com 'da %s'?)", b, w); if (strstr(w, "selva")) { char b2[96]; snprintf(b2, sizeof b2, p, "selva"); std::string x = b2; size_t k = x.find(" de selva"); if (k != std::string::npos) x.replace(k, 9, " da selva"); if (hasName(x.c_str())) continue; } CHECK(false, m); } }
    CHECK(hasName("Porta de carvalho") && hasName("Porta de ferro"), "portas carvalho/ferro");
    CHECK(isLeafB(LEAVES), "isLeafB(LEAVES)");
    for (int t = 0; t < NTAB; t++) for (int id : gCreative[t]) { if (tileOfId(id) < 0) { printf("tile<0: %s\n", itemName(id)); fails++; } }
    printf("test_creative: %d falhas, abas %d/%d/%d\n", fails, (int)gCreative[0].size(), (int)gCreative[1].size(), tot);
    return 0;
}
