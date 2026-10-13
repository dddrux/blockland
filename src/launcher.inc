// ---------- mini launcher: nome do jogador + skin + botao Jogar (tela "launcher" em screens.inc) ----------
// Perfil salvo em profile.cfg ("name=...", "skin=arquivo.png"). Skins ficam em assets/skins/*.png (64x64 ou 64x32); arrastar um PNG para a janela
// (PC) copia a skin para essa pasta e a seleciona. O nome aparece no chat e nas mensagens de entrada/saida da rede (net.inc).
static std::string gPlayerName, gP2Name, gSkinFile, gSkin2File;   // gP2Name = nick do jogador 2 (split)   // gSkin2File = skin do jogador 2 (split screen)
static bool gSkin2Dirty = false; static uint8_t gSkin2Px[64 * 64 * 4]; static bool gSkin2Has = false;   // main_loop recarrega a textura do jogador 2; Px = pixels p/ preview      // gSkinFile = nome do arquivo dentro de assets/skins ("" = a padrao)
static std::vector<std::string> gSkinList;      // arquivos .png de assets/skins, em ordem alfabetica
static std::vector<std::array<uint8_t, 256>> gSkinHead;   // miniatura 8x8 (rosto + chapeu) de cada skin de gSkinList, para a lista do launcher
static bool gSkinFocusPending = false; static std::string gLMsg; static Uint32 gLMsgT = 0;   // rolar a lista ate a skin escolhida; aviso curto do launcher (arquivo carregado/invalido)
static void lmsg(const std::string& m) { gLMsg = m; gLMsgT = SDL_GetTicks(); }
static std::string profClean(const std::string& in) {   // 1..16 caracteres imprimiveis (UTF-8 mantido), sem '<' '>' '|' '\n'
    std::string o; int n = 0;
    for (size_t i = 0; i < in.size() && n < 16; i++) {
        const unsigned char c = (unsigned char)in[i]; if (c < 32 || c == 127 || c == '<' || c == '>' || c == '|') continue;
        if (c == ' ' && o.empty()) continue; o += (char)c; if ((c & 0xC0) != 0x80) n++;
    }
    while (!o.empty() && o.back() == ' ') o.pop_back(); return o;
}
// ---- nickname (Parte 2): igual ao Minecraft oficial = 1..16 caracteres de [A-Za-z0-9_] ----
static const int NICK_MAX = 16;
static bool nickChar(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }
static bool nickValid(const std::string& s) { if (s.empty() || (int)s.size() > NICK_MAX) return false; for (unsigned char c : s) if (!nickChar(c)) return false; return true; }
static std::string nickSanitize(const std::string& in) {   // espaco vira '_'; o resto invalido (acento, simbolo) e descartado; corta em 16. "" se nada sobrar
    std::string o; for (unsigned char c : in) { if ((int)o.size() >= NICK_MAX) break; if (c == ' ') o += '_'; else if (nickChar(c)) o += (char)c; }
    return o;
}
static bool nickEq(const std::string& a, const std::string& b) {   // nick e unico sem diferenciar maiuscula/minuscula, como no oficial
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    return true;
}
static std::string nickSuggest(const std::string& base, const std::function<bool(const std::string&)>& taken) {   // Zeh -> Zeh_1, Zeh_2... (sempre <= 16)
    const std::string b = base.empty() ? std::string("Jogador") : base;
    for (int k = 1; k < 10000; k++) {
        const std::string suf = "_" + std::to_string(k), c = b.substr(0, std::max(0, NICK_MAX - (int)suf.size())) + suf;
        if (!taken(c)) return c;
    }
    return b;
}
static bool nickIsAuto(const std::string& n) {   // nome padrao automatico ("Jogador", "Jogador 2", "Jogador_2"): o host pode trocar sem recusar a entrada
    if (n.size() < 7 || !nickEq(n.substr(0, 7), "Jogador")) return false;
    for (size_t i = 7; i < n.size(); i++) if (!(isdigit((unsigned char)n[i]) || n[i] == '_' || n[i] == ' ')) return false;
    return true;
}
static bool skinUse(const std::string& file); static void profileSave();
static void skinScan() {
    gSkinList.clear(); std::error_code ec;
    for (auto& de : std::filesystem::directory_iterator("assets/skins", ec)) {
        if (!de.is_regular_file(ec)) continue; std::string n = de.path().filename().string(), l = n;
        for (char& c : l) c = (char)tolower((unsigned char)c);
        if (l.size() > 4 && l.compare(l.size() - 4, 4, ".png") == 0) gSkinList.push_back(n);
    }
    std::sort(gSkinList.begin(), gSkinList.end());
    gSkinHead.assign(gSkinList.size(), std::array<uint8_t, 256>{});
    for (size_t k = 0; k < gSkinList.size(); k++) {   // rosto (8,8) com o chapeu (40,8) por cima
        std::vector<uint8_t> px; int w, h; if (!loadPNG(("assets/skins/" + gSkinList[k]).c_str(), px, w, h) || w != 64 || (h != 64 && h != 32)) continue;
        for (int j = 0; j < 8; j++) for (int i = 0; i < 8; i++) {
            const uint8_t* f = &px[((8 + j) * 64 + 8 + i) * 4]; const uint8_t* ht = &px[((8 + j) * 64 + 40 + i) * 4]; uint8_t* o = &gSkinHead[k][(j * 8 + i) * 4];
            const uint8_t* q = (h == 64 && ht[3] >= 128) ? ht : f; memcpy(o, q, 4); if (q == ht) o[3] = 255;
        }
    }
}
static void skinSelect(int i) { if (i >= 0 && i < (int)gSkinList.size() && skinUse(gSkinList[i])) profileSave(); }
static int skinIndex() { for (size_t i = 0; i < gSkinList.size(); i++) if (gSkinList[i] == gSkinFile) return (int)i; return -1; }
static void profileSave() {
    FILE* f = fopen("profile.cfg", "wb"); if (!f) return;
    fprintf(f, "name=%s\nnick2=%s\nskin=%s\nskin2=%s\n", gPlayerName.c_str(), gP2Name.c_str(), gSkinFile.c_str(), gSkin2File.c_str()); fclose(f);
}
static bool skinUse(const std::string& file) {   // carrega assets/skins/<file> (atualiza o modelo na hora)
    if (file.empty() || !loadSkin(("assets/skins/" + file).c_str())) return false; gSkinFile = file; return true;
}
static void skin2Preview();
static void profileLoad() {   // chamado uma vez na inicializacao, DEPOIS de carregar a skin padrao
    skinScan(); FILE* f = fopen("profile.cfg", "rb"); if (!f) { skin2Preview(); return; } char ln[200];
    while (fgets(ln, sizeof ln, f)) {
        std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        if (s.compare(0, 5, "name=") == 0) gPlayerName = nickSanitize(s.substr(5));
        else if (s.compare(0, 6, "nick2=") == 0) gP2Name = nickSanitize(s.substr(6));
        else if (s.compare(0, 6, "skin2=") == 0) { const std::string k = s.substr(6); if (k.find('/') == std::string::npos && k.find('\\') == std::string::npos) gSkin2File = k; }
        else if (s.compare(0, 5, "skin=") == 0) { const std::string k = s.substr(5); if (k.find('/') == std::string::npos && k.find('\\') == std::string::npos) skinUse(k); }
    }
    fclose(f); skin2Preview();
}
static void skinStep(int d) {   // botoes < > do launcher
    skinScan(); if (gSkinList.empty()) return; const int n = (int)gSkinList.size(), i = skinIndex();
    for (int k = 1; k <= n; k++) { const int j = ((i < 0 ? (d > 0 ? -1 : 0) : i) + d * k % n + n) % n; if (skinUse(gSkinList[j])) { profileSave(); return; } }
}
static bool skinDropped(const char* path) {   // PNG arrastado para a janela: valida, copia para assets/skins e usa
    if (!loadSkin(path)) return false;
    std::error_code ec; std::filesystem::create_directories("assets/skins", ec); std::string n = std::filesystem::path(path).filename().string();
    for (char& c : n) if (c == '/' || c == '\\' || c == '|') c = '_';
    if (n.size() < 5 || n.compare(n.size() - 4, 4, ".png") != 0) n += ".png";
    for (int k = 2; std::filesystem::exists("assets/skins/" + n, ec) && !std::filesystem::equivalent(path, "assets/skins/" + n, ec) && std::filesystem::file_size(path, ec) != std::filesystem::file_size("assets/skins/" + n, ec) && k < 100; k++)   // mesmo nome com conteudo diferente: n_2.png...
        n = std::filesystem::path(path).stem().string() + "_" + std::to_string(k) + ".png";
    if (!std::filesystem::exists("assets/skins/" + n, ec)) std::filesystem::copy_file(path, "assets/skins/" + n, ec);
    skinScan(); gSkinFile = n; gSkinFocusPending = true; profileSave(); lmsg("Skin carregada: " + n); return true;
}
static void skin2Preview() {   // pixels da skin do jogador 2 para a previa do launcher
    gSkin2Has = false;
    if (gSkin2File.empty()) { skinScan(); for (const std::string& s : gSkinList) if (s != gSkinFile && s != "default.png") { gSkin2File = s; break; } }   // padrao: a 1a skin diferente da do jogador 1
    if (gSkin2File.empty()) return; std::vector<uint8_t> px; int w, h;
    if (!loadPNG(("assets/skins/" + gSkin2File).c_str(), px, w, h) || w != 64 || (h != 64 && h != 32)) return;
    memset(gSkin2Px, 0, sizeof gSkin2Px); memcpy(gSkin2Px, px.data(), (size_t)w * h * 4); gSkin2Has = true;   // (64x32 antigas aparecem so com a metade de cima: ok na previa)
}
static void skin2Step(int d) {   // botoes < > da skin do jogador 2
    skinScan(); if (gSkinList.empty()) return; const int n = (int)gSkinList.size(); int i = -1;
    for (int k = 0; k < n; k++) if (gSkinList[k] == gSkin2File) i = k;
    gSkin2File = gSkinList[((i < 0 ? (d > 0 ? -1 : 0) : i) + d + n) % n]; gSkin2Dirty = true; skin2Preview(); profileSave();
}

static int skinPickFile() {   // seletor do sistema (desktop). 1 = skin carregada, 0 = cancelou/invalida, -1 = sem seletor (abre o navegador proprio)
#ifdef __ANDROID__
    return -1;
#else
    std::string out;
#ifdef _WIN32
    FILE* f = _popen("powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.OpenFileDialog; $d.Filter='Skin PNG|*.png'; $d.Title='Escolher skin'; if($d.ShowDialog() -eq 'OK'){$d.FileName}\"", "r");
#else
    FILE* f = popen("if command -v zenity >/dev/null 2>&1; then zenity --file-selection --title='Escolher skin' --file-filter='PNG | *.png' 2>/dev/null; elif command -v kdialog >/dev/null 2>&1; then kdialog --getopenfilename . '*.png' 2>/dev/null; else exit 127; fi", "r");
#endif
    if (!f) return -1;
    char b[1024]; while (fgets(b, sizeof b, f)) out += b;
#ifdef _WIN32
    const int st = _pclose(f);
    if (st != 0) return -1;
#else
    const int st = pclose(f);
    if (st != -1 && ((st >> 8) & 0xFF) == 127) return -1;
#endif
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) out.pop_back();
    if (out.empty()) return 0;
    if (!skinDropped(out.c_str())) { lmsg("Arquivo inválido: use PNG 64x64 ou 64x32"); return 0; }
    return 1;
#endif
}
