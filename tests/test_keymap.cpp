#include <cstdio>
#include <cstring>
#include <cctype>
typedef unsigned char Uint8; enum { SDL_SCANCODE_A = 4 };
static Uint8 fake[512]; const Uint8* SDL_GetKeyboardState(int*) { return fake; }
static bool gKbAny = false; static int kbSym(int s) { return s; } static void kbState(Uint8*, const Uint8*) {} static void kbLoad() {}   // stubs: keymap.inc depende de keybinds.inc (patchKeys.1); aqui so o keys.cfg e testado
#include "../src/core/keymap.inc"
int main() {
    int f = 0; FILE* o = fopen("keys.cfg", "wb"); fputs("# t\ni=w\n9=0\nx=x\n", o); fclose(o);
    keymapLoad(); fake[4 + 'i' - 'a'] = 1;
    const Uint8* k = keymapState();
    if (!k[4 + 'w' - 'a']) { puts("FALHA: i nao virou w"); f++; } if (k[4 + 'i' - 'a']) { puts("FALHA: i ainda ativo"); f++; }
    if (keymapSym('i') != 'w' || keymapSym('w') != 'i' || keymapSym('q') != 'q' || keymapSym('x') != 'x') { puts("FALHA sym"); f++; }
    if (keymapSym('9') != '0') { puts("FALHA digito"); f++; }
    remove("keys.cfg"); printf("test_keymap: %d falhas\n", f); return f != 0;
}
