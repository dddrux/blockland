// Auditoria: abre o pack ativo e lista texturas block/item que NAO estao ligadas a bloco/item do jogo (viram extras PK) e quais extras sao cubo.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
int main() {
    if (!gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("pack nao abriu\n"); return 1; }
    const std::string T = "assets/minecraft/textures/";
    for (const char* k : {"block", "item"}) {
        auto L = gPack.listPng(T + k + "/"); printf("%s: %d texturas no pack\n", k, (int)L.size());
    }
    applyMcPack();
    printf("gPackUsed=%d PK=%d\n", (int)gPackUsed.size(), (int)PK.size());
    int nc = 0, ni = 0; FILE* f = fopen("/tmp/pk_extras.txt", "w");
    for (auto& e : PK) { fprintf(f, "%s %s %s\n", e.cube ? "CUBE" : "ITEM", e.file.c_str(), e.name.c_str()); (e.cube ? nc : ni)++; }
    fclose(f); printf("extras cubo=%d outros=%d\n", nc, ni);
    for (const char* k : {"block", "item"}) { f = fopen((std::string("/tmp/pk_used_") + k + ".txt").c_str(), "w");
        for (auto& n : gPackUsed) if (n.compare(0, strlen(k), k) == 0) fprintf(f, "%s\n", n.c_str()); fclose(f); }
    return 0;
}
