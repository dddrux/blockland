// ================= patchCapa.1: selecao de capa nas Configuracoes (usa gSt de screens.inc) =================
static void capeSyncSel() {   // todo quadro: Configuracoes mudou "capesel" -> aplica; na 1a vez resolve o arquivo salvo pelo hash do nome
    static bool init = false; static int last = -1;
    if (!init) { init = true; capeScan(); int s = gsi("capesel", 0); if (s > CAPE_NBI) { const int hh = gsi("capehash", 0); s = 0; for (size_t i = 0; i < gCapeFiles.size(); i++) if (capeHash(gCapeFiles[i]) == hh) s = 1 + CAPE_NBI + (int)i; gSt["capesel"] = s; } }
    const int s = std::max(0, std::min(capeCount() - 1, gsi("capesel", 0)));
    if (s != last) { last = s; if (!capeSelectIdx(s)) { gSt["capesel"] = last = 0; capeSelectIdx(0); }
        gSt["capehash"] = s > CAPE_NBI ? capeHash(gCapeFiles[(size_t)(s - 1 - CAPE_NBI)]) : 0; }
}
static bool capeDropped(const char* path) {   // PNG/GIF arrastado: copia para assets/capes e usa
    std::string ext = std::filesystem::path(path).extension().string(); for (char& c : ext) c = (char)tolower((unsigned char)c);
    if (ext != ".gif" && ext != ".png") return false; CapeAnim t; if (!capeFromFile(path, t)) return false;
    std::error_code ec; std::filesystem::create_directories("assets/capes", ec); std::string n = std::filesystem::path(path).filename().string(); for (char& c : n) if (c == '/' || c == '\\' || c == '|') c = '_';
    if (!std::filesystem::exists("assets/capes/" + n, ec)) std::filesystem::copy_file(path, "assets/capes/" + n, ec);
    capeScan(); for (size_t i = 0; i < gCapeFiles.size(); i++) if (gCapeFiles[i] == n) { gSt["capesel"] = 1 + CAPE_NBI + (int)i; return true; }
    return false;
}
static bool capeDropFile(const char* path) {   // so GIF vira capa ao soltar (PNG continua sendo skin; PNG de capa vai em assets/capes)
    std::string e = std::filesystem::path(path).extension().string(); for (char& c : e) c = (char)tolower((unsigned char)c);
    return e == ".gif" && capeDropped(path);
}
// ---- patchCapa.4: editor de capa (cores + padrao) -> assets/capes/Personalizada.png (entra na lista, rede e preview como qualquer PNG) ----
static void introPngWrite(const char* path, const std::vector<uint8_t>& rgb, int w, int h);   // splash.inc (incluido depois)
static bool capeCustomApply() {
    auto C = [](const char* id, float d) { return (int)std::max(0.f, std::min(255.f, floorf(gsf(id, d) * 255.f + .5f))); };
    const int M[3] = {C("cm.r", 200 / 255.f), C("cm.g", 30 / 255.f), C("cm.b", 30 / 255.f)}, A[3] = {C("ca.r", 1.f), C("ca.g", 215 / 255.f), C("ca.b", 0.f)};
    const int pat = std::max(0, std::min(6, gsi("cep", 0)));
    std::vector<uint8_t> ou(20 * 32 * 4, 255), in;
    for (int y = 0; y < 32; y++) for (int x = 0; x < 20; x++) {
        bool a = false;
        switch (pat) { case 1: a = (x / 4) % 2 == 1; break; case 2: a = x >= 8 && x < 12; break; case 3: a = x < 2 || x >= 18 || y < 2 || y >= 30; break;
            case 4: a = (x >= 8 && x < 12) || (y >= 11 && y < 15); break; case 5: a = ((x / 4) + (y / 4)) % 2 == 1; break; default: break; }
        const float t = pat == 6 ? y / 31.f : (a ? 1.f : 0.f);
        for (int c = 0; c < 3; c++) ou[((size_t)y * 20 + x) * 4 + c] = (uint8_t)(M[c] + (A[c] - M[c]) * t + .5f);
    }
    in.assign(20 * 32 * 4, 255); for (int i = 0; i < 20 * 32; i++) for (int c = 0; c < 3; c++) in[i * 4 + c] = (uint8_t)(ou[i * 4 + c] * 55 / 100);
    const uint8_t edge[3] = {(uint8_t)A[0], (uint8_t)A[1], (uint8_t)A[2]};
    std::vector<uint8_t> t; capeLayout(t, ou, in, edge);
    std::vector<uint8_t> rgb((size_t)CAPE_W * CAPE_H * 3); for (int i = 0; i < CAPE_W * CAPE_H; i++) for (int c = 0; c < 3; c++) rgb[(size_t)i * 3 + c] = t[(size_t)i * 4 + c];
    std::error_code ec; std::filesystem::create_directories("assets/capes", ec); introPngWrite("assets/capes/Personalizada.png", rgb, CAPE_W, CAPE_H);
    capeScan(); for (size_t i = 0; i < gCapeFiles.size(); i++) if (gCapeFiles[i] == "Personalizada.png") { const int s = 1 + CAPE_NBI + (int)i; gSt["capesel"] = s; return capeSelectIdx(s); }
    return false;
}
