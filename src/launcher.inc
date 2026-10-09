// ---------- mini launcher: nome do jogador + skin + botao Jogar (tela "launcher" em screens.inc) ----------
// Perfil salvo em profile.cfg ("name=...", "skin=arquivo.png"). Skins ficam em assets/skins/*.png (64x64 ou 64x32); arrastar um PNG para a janela
// (PC) copia a skin para essa pasta e a seleciona. O nome aparece no chat e nas mensagens de entrada/saida da rede (net.inc).
static std::string gPlayerName, gSkinFile;      // gSkinFile = nome do arquivo dentro de assets/skins ("" = a padrao)
static std::vector<std::string> gSkinList;      // arquivos .png de assets/skins, em ordem alfabetica
static std::string profClean(const std::string& in) {   // 1..16 caracteres imprimiveis (UTF-8 mantido), sem '<' '>' '|' '\n'
    std::string o; int n = 0;
    for (size_t i = 0; i < in.size() && n < 16; i++) {
        const unsigned char c = (unsigned char)in[i]; if (c < 32 || c == 127 || c == '<' || c == '>' || c == '|') continue;
        if (c == ' ' && o.empty()) continue; o += (char)c; if ((c & 0xC0) != 0x80) n++;
    }
    while (!o.empty() && o.back() == ' ') o.pop_back(); return o;
}
static void skinScan() {
    gSkinList.clear(); std::error_code ec;
    for (auto& de : std::filesystem::directory_iterator("assets/skins", ec)) {
        if (!de.is_regular_file(ec)) continue; std::string n = de.path().filename().string(), l = n;
        for (char& c : l) c = (char)tolower((unsigned char)c);
        if (l.size() > 4 && l.compare(l.size() - 4, 4, ".png") == 0) gSkinList.push_back(n);
    }
    std::sort(gSkinList.begin(), gSkinList.end());
}
static int skinIndex() { for (size_t i = 0; i < gSkinList.size(); i++) if (gSkinList[i] == gSkinFile) return (int)i; return -1; }
static void profileSave() {
    FILE* f = fopen("profile.cfg", "wb"); if (!f) return;
    fprintf(f, "name=%s\nskin=%s\n", gPlayerName.c_str(), gSkinFile.c_str()); fclose(f);
}
static bool skinUse(const std::string& file) {   // carrega assets/skins/<file> (atualiza o modelo na hora)
    if (file.empty() || !loadSkin(("assets/skins/" + file).c_str())) return false; gSkinFile = file; return true;
}
static void profileLoad() {   // chamado uma vez na inicializacao, DEPOIS de carregar a skin padrao
    skinScan(); FILE* f = fopen("profile.cfg", "rb"); if (!f) return; char ln[200];
    while (fgets(ln, sizeof ln, f)) {
        std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        if (s.compare(0, 5, "name=") == 0) gPlayerName = profClean(s.substr(5));
        else if (s.compare(0, 5, "skin=") == 0) { const std::string k = s.substr(5); if (k.find('/') == std::string::npos && k.find('\\') == std::string::npos) skinUse(k); }
    }
    fclose(f);
}
static void skinStep(int d) {   // botoes < > do launcher
    skinScan(); if (gSkinList.empty()) return; const int n = (int)gSkinList.size(), i = skinIndex();
    for (int k = 1; k <= n; k++) { const int j = ((i < 0 ? (d > 0 ? -1 : 0) : i) + d * k % n + n) % n; if (skinUse(gSkinList[j])) { profileSave(); return; } }
}
static bool skinDropped(const char* path) {   // PNG arrastado para a janela: valida, copia para assets/skins e usa
    if (!loadSkin(path)) return false;
    std::error_code ec; std::filesystem::create_directories("assets/skins", ec); std::string n = std::filesystem::path(path).filename().string();
    for (char& c : n) if (c == '/' || c == '\\' || c == '|') c = '_';
    if (!std::filesystem::exists("assets/skins/" + n, ec)) std::filesystem::copy_file(path, "assets/skins/" + n, ec);
    skinScan(); gSkinFile = n; profileSave(); return true;
}
