// ================= patchKeys.1: teclas de controle configuraveis (Opcoes > Controles) =================
// Cada acao tem uma tecla padrao (a que o resto do jogo ja le) e uma tecla escolhida. Os eventos/estado do teclado passam por uma
// tabela fisica->padrao (kbT): apertar a tecla escolhida age como a padrao. Escolher uma tecla ja usada TROCA as duas (sem conflito).
// Salvo em keybinds.cfg (uma linha `id=NOME_DA_TECLA`). keys.cfg (troca letra<->letra) continua valendo por cima.
struct KbAct { const char* id; const char* label; int def; };
static const KbAct KB_ACTS[] = {
    {"inv", "Inventário", SDL_SCANCODE_E}, {"h1", "Slot 1 da Barra Rápida", SDL_SCANCODE_1}, {"h2", "Slot 2 da Barra Rápida", SDL_SCANCODE_2},
    {"h3", "Slot 3 da Barra Rápida", SDL_SCANCODE_3}, {"h4", "Slot 4 da Barra Rápida", SDL_SCANCODE_4}, {"h5", "Slot 5 da Barra Rápida", SDL_SCANCODE_5},
    {"h6", "Slot 6 da Barra Rápida", SDL_SCANCODE_6}, {"h7", "Slot 7 da Barra Rápida", SDL_SCANCODE_7}, {"h8", "Slot 8 da Barra Rápida", SDL_SCANCODE_8},
    {"h9", "Slot 9 da Barra Rápida", SDL_SCANCODE_9}, {"drop", "Largar Item", SDL_SCANCODE_Q},
    {"fwd", "Andar para Frente", SDL_SCANCODE_W}, {"left", "Andar para Esquerda", SDL_SCANCODE_A}, {"back", "Andar para Trás", SDL_SCANCODE_S},
    {"right", "Andar para Direita", SDL_SCANCODE_D}, {"jump", "Pular", SDL_SCANCODE_SPACE}, {"sneak", "Agachar", SDL_SCANCODE_LSHIFT},
    {"sprint", "Correr", SDL_SCANCODE_LCTRL},
    {"tab", "Listar Jogadores", SDL_SCANCODE_TAB}, {"chat", "Abrir Bate-Papo", SDL_SCANCODE_T}, {"cmd", "Abrir Comando", SDL_SCANCODE_SLASH},
    {"shot", "Tirar Captura de Tela", SDL_SCANCODE_F2}, {"persp", "Alternar Perspectiva", SDL_SCANCODE_F5}, {"full", "Tela Cheia", SDL_SCANCODE_F11},
};
static const int KB_N = (int)(sizeof(KB_ACTS) / sizeof(*KB_ACTS));
// linhas da lista (act >= 0: acao; -1: titulo; -2: texto fixo da linha `fix`)
struct KbRow { const char* txt; int act; const char* fix; };
static const KbRow KB_ROWS[] = {
    {"Inventário", -1, 0}, {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {0, 4, 0}, {0, 5, 0}, {0, 6, 0}, {0, 7, 0}, {0, 8, 0}, {0, 9, 0}, {0, 10, 0},
    {"Movimento", -1, 0}, {0, 11, 0}, {0, 12, 0}, {0, 13, 0}, {0, 14, 0}, {0, 15, 0}, {0, 16, 0}, {0, 17, 0},
    {"Jogabilidade", -1, 0}, {"Atacar/Destruir", -2, "Botão 1"}, {"Usar Item/Colocar Bloco", -2, "Botão 2"}, {"Escolher Bloco", -2, "Botão 3"},
    {"Multijogador", -1, 0}, {0, 18, 0}, {0, 19, 0}, {0, 20, 0},
    {"Diversos", -1, 0}, {0, 21, 0}, {0, 22, 0}, {0, 23, 0},
};
static const int KB_NROWS = (int)(sizeof(KB_ROWS) / sizeof(*KB_ROWS));
static int gKbBind[KB_N + 1];      // tecla (scancode) escolhida por acao
static int gKbT[512];              // fisica -> padrao (0 = tecla sem funcao)
static bool gKbAny = false;        // alguma acao fora do padrao
static int gKbCap = -1;            // acao esperando a proxima tecla (-1 = nenhuma)
static void kbRebuild() {
    for (int i = 0; i < 512; i++) gKbT[i] = i;
    gKbAny = false;
    for (int a = 0; a < KB_N; a++) if (gKbBind[a] != KB_ACTS[a].def) { gKbAny = true; gKbT[gKbBind[a]] = KB_ACTS[a].def; }
    for (int a = 0; a < KB_N; a++) { bool used = false; for (int b = 0; b < KB_N; b++) if (gKbBind[b] == KB_ACTS[a].def) used = true; if (!used) gKbT[KB_ACTS[a].def] = 0; }
}
static void kbSave() {
    FILE* f = fopen("keybinds.cfg", "wb"); if (!f) return;
    fprintf(f, "# id=NOME_DA_TECLA (Opcoes > Controles); apagar o arquivo volta ao padrao\n");
    for (int a = 0; a < KB_N; a++) if (gKbBind[a] != KB_ACTS[a].def) fprintf(f, "%s=%s\n", KB_ACTS[a].id, SDL_GetScancodeName((SDL_Scancode)gKbBind[a]));
    fclose(f);
}
static void kbLoad() {
    for (int a = 0; a < KB_N; a++) gKbBind[a] = KB_ACTS[a].def;
    FILE* f = fopen("keybinds.cfg", "rb");
    if (f) { char ln[128];
        while (fgets(ln, sizeof ln, f)) {
            if (ln[0] == '#') continue; char* eq = strchr(ln, '='); if (!eq) continue; *eq = 0; std::string v = eq + 1; while (!v.empty() && (v.back() == '\n' || v.back() == '\r' || v.back() == ' ')) v.pop_back();
            const SDL_Scancode sc = SDL_GetScancodeFromName(v.c_str()); if (sc == SDL_SCANCODE_UNKNOWN || (int)sc >= 512) continue;
            for (int a = 0; a < KB_N; a++) if (!strcmp(ln, KB_ACTS[a].id)) { bool dup = false; for (int b = 0; b < KB_N; b++) if (b != a && gKbBind[b] == (int)sc) dup = true; if (!dup) gKbBind[a] = (int)sc; }
        }
        fclose(f); }
    kbRebuild();
}
static bool kbAllowed(int sc) { return sc > 0 && sc < 512 && sc != SDL_SCANCODE_ESCAPE && sc != SDL_SCANCODE_RETURN && sc != SDL_SCANCODE_KP_ENTER; }
static void kbSet(int a, int sc) {   // escolher tecla ja usada por outra acao troca as duas
    if (a < 0 || a >= KB_N || !kbAllowed(sc)) return;
    for (int b = 0; b < KB_N; b++) if (b != a && gKbBind[b] == sc) gKbBind[b] = gKbBind[a];
    gKbBind[a] = sc; kbRebuild(); kbSave();
}
static void kbReset(int a) { if (a < 0 || a >= KB_N) return; const int d = KB_ACTS[a].def; for (int b = 0; b < KB_N; b++) if (b != a && gKbBind[b] == d) gKbBind[b] = gKbBind[a]; gKbBind[a] = d; kbRebuild(); kbSave(); }
static void kbResetAll() { for (int a = 0; a < KB_N; a++) gKbBind[a] = KB_ACTS[a].def; kbRebuild(); kbSave(); }
static std::string kbName(int sc) {
    switch (sc) { case SDL_SCANCODE_SPACE: return "Espaço"; case SDL_SCANCODE_LSHIFT: return "Shift Esq."; case SDL_SCANCODE_RSHIFT: return "Shift Dir."; case SDL_SCANCODE_LCTRL: return "Ctrl Esq."; case SDL_SCANCODE_RCTRL: return "Ctrl Dir.";
        case SDL_SCANCODE_LALT: return "Alt Esq."; case SDL_SCANCODE_RALT: return "Alt Dir."; case SDL_SCANCODE_TAB: return "Tab"; case SDL_SCANCODE_SLASH: return "/"; case SDL_SCANCODE_UP: return "Seta Cima"; case SDL_SCANCODE_DOWN: return "Seta Baixo";
        case SDL_SCANCODE_LEFT: return "Seta Esq."; case SDL_SCANCODE_RIGHT: return "Seta Dir."; case SDL_SCANCODE_CAPSLOCK: return "Caps Lock"; default: break; }
    const char* n = SDL_GetScancodeName((SDL_Scancode)sc); return n && *n ? n : "?";
}
static int kbSym(int sym) {   // sym fisico -> sym padrao (0 = tecla sem funcao)
    if (!gKbAny) return sym;
    const int sc = (int)SDL_GetScancodeFromKey((SDL_Keycode)sym); if (sc <= 0 || sc >= 512) return sym;
    const int d = gKbT[sc]; return d == sc ? sym : (d ? (int)SDL_GetKeyFromScancode((SDL_Scancode)d) : 0);
}
static void kbState(Uint8* kk, const Uint8* s) {   // aplica a tabela ao estado do teclado (kk ja e copia de s)
    if (!gKbAny) return;
    for (int c = 0; c < 512; c++) if (gKbT[c] != c) { kk[c] = 0; if (gKbT[c]) kk[gKbT[c]] = 0; }
    for (int c = 0; c < 512; c++) if (gKbT[c] != c && gKbT[c]) kk[gKbT[c]] |= s[c];
}
