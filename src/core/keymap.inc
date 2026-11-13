// ================= BUG-015: remapeamento de teclas (keys.cfg) =================
// keys.cfg na raiz, uma troca por linha: `I=W` = a tecla I passa a fazer o que W faz (e W passa a fazer o que I fazia).
// Vale para letras a-z e digitos 0-9 (andar, inventario, soltar, chat etc.); comentario com #. Sem arquivo = teclas padrao.
static int gKeySwap[128];
static void keymapLoad() {
    kbLoad();   // patchKeys.1
    for (int i = 0; i < 128; i++) gKeySwap[i] = i;
    FILE* f = fopen("keys.cfg", "rb"); if (!f) return; char ln[128];
    auto ok = [](int c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); };
    while (fgets(ln, sizeof ln, f)) {
        if (ln[0] == '#') continue; const char* eq = strchr(ln, '='); if (!eq || eq == ln) continue;
        int a = tolower((unsigned char)ln[0]), b = tolower((unsigned char)eq[1]);
        if (!ok(a) || !ok(b) || a == b || gKeySwap[a] != a || gKeySwap[b] != b) continue;
        gKeySwap[a] = b; gKeySwap[b] = a;
    }
    fclose(f);
}
static int keymapSym(int sym) { sym = sym > 0 && sym < 128 ? gKeySwap[sym] : sym; return kbSym(sym); }   // patchKeys.1: keys.cfg e depois as teclas de Opcoes > Controles
static int keymapSc(int c) { return c >= 'a' && c <= 'z' ? SDL_SCANCODE_A + (c - 'a') : c >= '1' && c <= '9' ? 30 + (c - '1') : c == '0' ? 39 : -1; }
static const Uint8* keymapState() {   // copia do estado do teclado com as trocas aplicadas
    const Uint8* s = SDL_GetKeyboardState(nullptr); static Uint8 kk[512]; memcpy(kk, s, 512);
    for (int c = '0'; c <= 'z'; c++) { const int d = gKeySwap[c]; if (d == c) continue; const int a = keymapSc(c), b = keymapSc(d); if (a >= 0 && b >= 0) kk[a] = s[b]; }
    if (gKbAny) { static Uint8 k2[512]; memcpy(k2, kk, 512); kbState(kk, k2); }   // patchKeys.1
    return kk;
}
