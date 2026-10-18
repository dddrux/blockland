// ================= menus: titulo (1.21.5) + mundo unico (saves/mundo) com chunks, seed e jogador salvos =================
// Um jogador -> entra direto no mundo salvo (cria na primeira vez). Placeholder: Multiplayer, Realms, Options etc. sem logica.
namespace fs = std::filesystem;
enum { SC_PLAY = 0, SC_TITLE, SC_WORLDS, SC_CREATE, SC_OTHER, SC_LOAD };
static int gScreen = SC_TITLE, gMenuAct = 0;   // gMenuAct: 1 = iniciar mundo gMenuDir, 2 = sair
static std::string gMenuDir, gCurDir, gOtherTitle;
struct SaveInfo { std::string dir, name; int64_t seed = 0; int mode = 0, hard = 0; long long played = 0, wtime = 0; double px = 8.5, py = 0, pz = 8.5; bool hasPos = false; int wx = 0, wxl = 30000; };
static int gMX = 0, gMY = 0; static GLuint mtDirt = 0, mtIcon[3] = {0, 0, 0}, mtUnk = 0;
static bool mtInit = false; static GLuint mtLogo = 0;
static GLuint mtLoad(const char* rel, const char* fb, bool rep) {
    std::vector<uint8_t> px; int w = 0, h = 0; GLuint t = 0;
    if (!packRGBA(rel, px, w, h) && !(fb && loadPNG(fb, px, w, h))) return 0;
    glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, rep ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); return t;
}
// ---- saves ----
static std::string cfgGet(const std::string& txt, const char* k) {
    const std::string key = std::string(k) + "="; size_t p = 0;
    while (p < txt.size()) { size_t e = txt.find('\n', p); if (e == std::string::npos) e = txt.size(); if (txt.compare(p, key.size(), key) == 0) return txt.substr(p + key.size(), e - p - key.size()); p = e + 1; }
    return "";
}
static void writeCfg(const SaveInfo& s) {
    FILE* f = fopen(("saves/" + s.dir + "/level.cfg").c_str(), "wb"); if (!f) return;
    fprintf(f, "name=%s\nseed=%lld\nmode=%d\nhardcore=%d\nplayed=%lld\npx=%.3f\npy=%.3f\npz=%.3f\ntime=%lld\nweather=%d\nwxleft=%d\n", s.name.c_str(), (long long)s.seed, s.mode, s.hard, s.played, s.px, s.py, s.pz, s.wtime, s.wx, s.wxl); fclose(f);
}
// ---- jogador: inventario, armadura, mao secundaria, item no mouse, vida e fome (convidado usa arquivo proprio; blocos de pack salvos pelo nome) ----
static std::string playerFile() { return "saves/" + gCurDir + ((netSock >= 0 && !netHost) ? "/player_guest.dat" : "/player.dat"); }
static void savePlayer() {
    if (gCurDir.empty()) return;
    FILE* f = fopen(playerFile().c_str(), "wb"); if (!f) return;
    auto put = [&](int idx, const Slot& s) { if (s.id <= 0 || s.n <= 0) return; if (isPk(s.id)) fprintf(f, "p %d %d %s\n", idx, s.n, PK[s.id - PK0].file.c_str()); else fprintf(f, "i %d %d %d\n", idx, s.n, s.id);
        if (s.d > 0) fprintf(f, "d %d\n", s.d);
        if (!s.nbt.empty() && s.nbt[0] != '?') fprintf(f, "x %s\n", s.nbt.c_str()); };   // "x <nbt>" logo depois da pilha: NBT (bau com conteudo)
    for (int i = 0; i < 36; i++) put(i, inv[i]);
    for (int i = 0; i < 4; i++) put(36 + i, armor[i]);
    put(40, offh); put(41, cur);
    fprintf(f, "hp %d %d\n", gHP, gFood); fclose(f);
}
static bool loadPlayer() {
    FILE* f = fopen(playerFile().c_str(), "rb"); if (!f) return false;
    for (int i = 0; i < 36; i++) inv[i] = Slot();
    for (auto& a : armor) a = Slot();
    offh = Slot(); cur = Slot(); static char ln[131072]; int lastIdx = -1;
    auto slotAt = [&](int i) -> Slot* { return i >= 0 && i < 36 ? &inv[i] : i >= 36 && i < 40 ? &armor[i - 36] : i == 40 ? &offh : i == 41 ? &cur : nullptr; };
    while (fgets(ln, sizeof ln, f)) {
        int idx = -1, n = 0, id = 0; Slot s;
        if (ln[0] == 'x' && ln[1] == ' ') { Slot* t = slotAt(lastIdx); if (t && t->id) { t->nbt = ln + 2; while (!t->nbt.empty() && (t->nbt.back() == '\n' || t->nbt.back() == '\r')) t->nbt.pop_back(); } continue; }
        if (ln[0] == 'd' && ln[1] == ' ') { Slot* t = slotAt(lastIdx); int dv = 0; if (t && t->id && sscanf(ln, "d %d", &dv) == 1) t->d = std::max(0, dv); continue; }   // patchDur.1
        lastIdx = -1;
        if (ln[0] == 'h') { int hp = 20, fd = 20; if (sscanf(ln, "hp %d %d", &hp, &fd) == 2) { gHP = std::max(1, std::min(20, hp)); gFood = std::max(0, std::min(20, fd)); } continue; }
        if (ln[0] == 'i') { if (sscanf(ln, "i %d %d %d", &idx, &n, &id) != 3) continue; s = {id, n}; }
        else if (ln[0] == 'p') { char nm[512]; if (sscanf(ln, "p %d %d %511[^\r\n]", &idx, &n, nm) != 3) continue; auto it = gPkByFile.find(nm); if (it == gPkByFile.end()) continue; s = {it->second, n}; }
        else continue;
        if (s.id <= 0 || s.n <= 0) continue; s.n = std::min(s.n, maxStack(s.id));
        if (Slot* t = slotAt(idx)) { *t = s; lastIdx = idx; }
    }
    fclose(f); return true;
}
struct ChEnc { int64_t key = 0; const Chunk* c = nullptr; std::vector<uint16_t> o; std::vector<uint8_t> mo; };
struct ChEncJob { std::vector<ChEnc>* v; size_t a, b; };
static int chEncThread(void* p) {   // blocos em RLE de 16 bits + metadados em RLE (run u16, valor u8): so le o chunk, seguro em paralelo
    ChEncJob* j = (ChEncJob*)p; const size_t N = (size_t)CW * CH * CW;
    for (size_t k = j->a; k < j->b; k++) { ChEnc& e = (*j->v)[k]; const BT* b = e.c->b; const uint8_t* m = e.c->m;
        for (size_t i = 0; i < N;) { const uint16_t v = b[i]; size_t q = i + 1; while (q < N && b[q] == v && q - i < 65535) q++; e.o.push_back((uint16_t)(q - i)); e.o.push_back(v); i = q; }
        for (size_t i = 0; i < N;) { const uint8_t v = m[i]; size_t q = i + 1; while (q < N && m[q] == v && q - i < 65535) q++; const uint16_t r = (uint16_t)(q - i); e.mo.push_back((uint8_t)(r & 255)); e.mo.push_back((uint8_t)(r >> 8)); e.mo.push_back(v); i = q; } }
    return 0;
}
static void writeChunkFile(const std::string& path, std::unordered_map<int64_t, Chunk*>& M) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) { logError("Falha ao salvar os chunks em %s", path.c_str()); return; }
    fwrite("BL18", 1, 4, f);   // formato: paleta de nomes do pack (ids >= PK0 sobrevivem a troca de resource pack) + por chunk: blocos em RLE de 16 bits + metadados em RLE (run u16, valor u8)
    int32_t np = (int32_t)PK.size(); fwrite(&np, 4, 1, f);
    for (auto& p : PK) { uint16_t l = (uint16_t)p.file.size(); fwrite(&l, 2, 1, f); fwrite(p.file.data(), 1, l, f); }
    int32_t n = (int32_t)M.size(); fwrite(&n, 4, 1, f);
    std::vector<ChEnc> E; E.reserve(M.size()); for (auto& kv : M) { ChEnc e; e.key = kv.first; e.c = kv.second; E.push_back(std::move(e)); }
    { const size_t ne = E.size(); int nt = ne < 64 ? 1 : std::max(1, std::min(4, SDL_GetCPUCount())); std::vector<ChEncJob> jobs(nt); std::vector<SDL_Thread*> th(nt, nullptr);   // RLE em paralelo (salvar e sair mais rapido)
      for (int t = 0; t < nt; t++) jobs[t] = {&E, ne * t / nt, ne * (t + 1) / nt};
      for (int t = 1; t < nt; t++) th[t] = SDL_CreateThread(chEncThread, "chenc", &jobs[t]);
      chEncThread(&jobs[0]); for (int t = 1; t < nt; t++) { if (th[t]) SDL_WaitThread(th[t], nullptr); else chEncThread(&jobs[t]); } }
    for (ChEnc& e : E) {
        int32_t c[2] = {(int32_t)(e.key >> 32), (int32_t)(uint32_t)e.key}; fwrite(c, 4, 2, f);
        int32_t len = (int32_t)e.o.size(); fwrite(&len, 4, 1, f); fwrite(e.o.data(), 2, e.o.size(), f);
        int32_t mlen = (int32_t)e.mo.size(); fwrite(&mlen, 4, 1, f); fwrite(e.mo.data(), 1, e.mo.size(), f);
    }
    fclose(f);
}
static void saveCurWorld(double x, double y, double z) {
    if (gCurDir.empty()) return;
    if (gChest) chestClosed();   // baú aberto: fecha a tampa (bit 7 do meta) e manda o conteúdo antes de salvar
    const Uint32 svT0 = SDL_GetTicks(); logInfo("Saving world '%s'...", gCurDir.c_str());
    savePlayer();
    if (netSock >= 0 && !netHost) return;   // convidado: o mundo e do host (so o jogador e salvo)
    boatsSave(gCurDir); cartsSave(gCurDir); chestsSave(gCurDir); furnSave(gCurDir); propsWorldSave(gCurDir); signsSave(gCurDir); dragonWorldSave(gCurDir);
    SaveInfo s; s.dir = gCurDir; s.seed = WORLD_SEED; s.mode = gGm; s.hard = gHard ? 1 : 0; s.played = (long long)time(nullptr); s.px = x; s.py = y; s.pz = z; s.wtime = gWorldTime; s.wx = gWeather; s.wxl = gWxLeft;
    { std::string t0; FILE* fc = fopen(("saves/" + gCurDir + "/level.cfg").c_str(), "rb"); if (fc) { char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, fc)) > 0) t0.append(b, r); fclose(fc); }
      s.name = cfgGet(t0, "name"); if (s.name.empty()) s.name = gCurDir; }   // mantém o nome dado na criação/edição
    writeCfg(s);
    { writeChunkFile("saves/" + gCurDir + "/chunks16.dat", dimWorld(0));   // superficie
      if (!dimWorld(1).empty()) writeChunkFile("saves/" + gCurDir + "/chunks16n.dat", dimWorld(1));   // nether
      if (!dimWorld(2).empty()) writeChunkFile("saves/" + gCurDir + "/chunks16e.dat", dimWorld(2));   // end
      if (FILE* fd = fopen(("saves/" + gCurDir + "/dim.txt").c_str(), "wb")) { fprintf(fd, "%d", gDim); fclose(fd); } }
    logInfo("Mundo salvo em %u ms (%zu chunks)", (unsigned)(SDL_GetTicks() - svT0), dimWorld(0).size());
}
static void resetWorldData() {
    netherReset();   // libera a outra dimensao (nether.inc) e volta para a superficie
    static auto pDel = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    for (auto& kv : W) { Chunk* c = kv.second; if (pDel) { if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo); } delete[] c->lc; delete c; }
    gChest = false; gChEnder = false; gChWait = false;   // tela de baú aberta num mundo que está sendo descartado
    resetBlockUpdates(); W.clear(); gKick = true; gEpoch++; savedC.clear(); savedM.clear(); gSpill.clear(); gFloor.clear(); keepLeaves.clear(); leafPending.clear(); wqCur.clear(); lqCur.clear(); lqNext.clear(); lqSet.clear(); wqNext.clear(); wqSet.clear(); MOBS.clear(); BOATS.clear(); gBoatRide = 0; gBoatCamId = 0; DROPS.clear(); chestsClear();
}
static void loadSavedChunks(const std::string& dir) {
    const size_t N = (size_t)CW * CH * CW;
    if (FILE* f = fopen(("saves/" + dir + "/" + std::string(gChunkFile)).c_str(), "rb")) {
        char mg[4]; int32_t np = 0; logInfo("[LOAD] lendo %s/%s", dir.c_str(), gChunkFile);
        if (fread(mg, 1, 4, f) == 4 && (!memcmp(mg, "BL16", 4) || !memcmp(mg, "BL17", 4) || !memcmp(mg, "BL18", 4)) && fread(&np, 4, 1, f) == 1 && np >= 0 && np < 200000) {
            const bool hasMeta = memcmp(mg, "BL16", 4) != 0, oldPk = !memcmp(mg, "BL16", 4) || !memcmp(mg, "BL17", 4); const int pkBase = oldPk ? 1000 : PK0;   // BL16/17: PK0 era 1000 (v40 moveu para 1016)   // BL16 (antigo) nao tem metadados => tudo 0
            std::vector<uint16_t> remap(np); bool same = true;
            for (int i = 0; i < np; i++) {   // nome salvo -> id atual (bloco que o pack atual não tem vira pedra)
                uint16_t l = 0; char nm[512]; if (fread(&l, 2, 1, f) != 1 || l >= sizeof nm || fread(nm, 1, l, f) != l) { fclose(f); return; }
                nm[l] = 0; auto it = gPkByFile.find(nm); remap[i] = it != gPkByFile.end() ? (uint16_t)it->second : (uint16_t)STONE;
                if (remap[i] != PK0 + i || oldPk) same = false;
            }
            int32_t n = 0; if (fread(&n, 4, 1, f) != 1) { fclose(f); return; }
            std::vector<uint16_t> o;
            for (int i = 0; i < n; i++) {
                int32_t c[2], len = 0; if (fread(c, 4, 2, f) != 2 || fread(&len, 4, 1, f) != 1 || len < 0 || (size_t)len > N * 2) break;
                o.resize(len); if (fread(o.data(), 2, len, f) != (size_t)len) break;
                std::vector<uint16_t> d; d.reserve(len); size_t p = 0; std::vector<std::pair<size_t, size_t>> lost;   // trechos cujo bloco do pack sumiu (viraram pedra): meta descartado
                for (int k = 0; k + 1 < len && p < N; k += 2) { size_t r = std::min<size_t>(o[k], N - p); BT v = o[k + 1];
                    if (!same && v >= pkBase) { const bool miss = (v - pkBase) >= np; v = !miss ? remap[v - pkBase] : (BT)STONE; if (miss || v == STONE) lost.push_back({p, r}); }
                    if (v >= (BT)(PK0 + PK.size())) { v = (BT)STONE; lost.push_back({p, r}); }   // patchCrash.1: id fora do registro atual (save de outra versao/corrompido) vira pedra, nunca indice fora das tabelas
                    d.push_back((uint16_t)r); d.push_back(v); p += r; }   // fica em RLE (patchMap.2); decodifica so ao gerar o chunk
                if (hasMeta) {
                    int32_t mlen = 0; if (fread(&mlen, 4, 1, f) != 1 || mlen < 0 || (size_t)mlen > N * 3) break;
                    std::vector<uint8_t> mo(mlen); if (mlen && fread(mo.data(), 1, mlen, f) != (size_t)mlen) break;
                    if (!lost.empty()) { std::vector<uint8_t> md(N, 0); savedDecM(mo, md.data(), N);
                        for (auto& z : lost) std::fill(md.begin() + z.first, md.begin() + z.first + z.second, (uint8_t)0);
                        mo = savedEncM(md.data(), N); }
                    savedM[key(c[0], c[1])] = std::move(mo);
                }
                savedC[key(c[0], c[1])] = std::move(d);
            }
            logInfo("[LOAD] %.4s: %d chunks na paleta de %d nomes, %zu guardados", mg, n, np, savedC.size());
        } else logWarn("[LOAD] formato de chunks desconhecido ou invalido em saves/%s (ignorado: o mundo sera gerado de novo)", dir.c_str());
        fclose(f); return;
    }
    FILE* f = fopen(("saves/" + dir + "/chunks256.dat").c_str(), "rb"); if (!f) return;   // formato antigo (8 bits)
    int32_t n = 0; if (fread(&n, 4, 1, f) != 1) { fclose(f); return; }
    for (int i = 0; i < n; i++) {
        int32_t c[2]; std::vector<uint8_t> d(N);
        if (fread(c, 4, 2, f) != 2 || fread(d.data(), 1, d.size(), f) != d.size()) break;
        std::vector<BT> dd(d.begin(), d.end()); savedC[key(c[0], c[1])] = savedEncB(dd.data(), N);
    }
    fclose(f);
}
static int64_t randomSeed();
static void ensureWorld() {   // mundo unico: saves/mundo (criado com seed aleatoria na primeira vez)
    gMenuDir = "mundo"; std::error_code ec; fs::create_directories("saves/mundo", ec);
    if (fs::exists("saves/mundo/level.cfg", ec)) return;
    SaveInfo s; s.dir = gMenuDir; s.name = "Mundo"; s.mode = 0; s.played = (long long)time(nullptr);
    s.seed = randomSeed(); writeCfg(s);
}

// ---- gerenciamento de mundos (tela "Selecionar Mundo"): listar, criar com seed, renomear, excluir ----
struct WInfo { std::string dir, name; int64_t seed = 0; int mode = 0, hard = 0; long long played = 0; };
static std::vector<WInfo> gWorlds;
static std::string slurp(const std::string& p) { std::string t; FILE* f = fopen(p.c_str(), "rb"); if (!f) return t; char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, f)) > 0) t.append(b, r); fclose(f); return t; }
static void scanWorlds() {
    gWorlds.clear(); std::error_code ec;
    for (auto& de : fs::directory_iterator("saves", ec)) {
        if (!de.is_directory(ec) || de.path().filename().string()[0] == '_') continue;   // "_mp" = mundo temporario do multiplayer
        const std::string t = slurp(de.path().string() + "/level.cfg"); if (t.empty()) continue;
        WInfo w; w.dir = de.path().filename().string(); w.name = cfgGet(t, "name"); if (w.name.empty()) w.name = w.dir;
        w.seed = (int64_t)strtoll(cfgGet(t, "seed").c_str(), nullptr, 10); w.mode = atoi(cfgGet(t, "mode").c_str()); w.hard = atoi(cfgGet(t, "hardcore").c_str()); w.played = atoll(cfgGet(t, "played").c_str());
        gWorlds.push_back(w);
    }
    std::sort(gWorlds.begin(), gWorlds.end(), [](const WInfo& a, const WInfo& b) { return a.played != b.played ? a.played > b.played : a.dir < b.dir; });
}
static int64_t randomSeed() {   // 64 bits (como new Random().nextLong())
    uint64_t z = (uint64_t)SDL_GetPerformanceCounter() ^ ((uint64_t)time(nullptr) << 32) ^ ((uint64_t)SDL_GetTicks() * 0x9E3779B97F4A7C15ULL);
    z += 0x9E3779B97F4A7C15ULL; z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL; z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL; return (int64_t)(z ^ (z >> 31));
}
// seed como no Minecraft: vazio = aleatória; número = o próprio número; texto = String.hashCode() do Java
static int64_t parseSeed(const std::string& s) {   // igual ao MC 1.12.2: vazio ou 0 = aleatoria; Long.parseLong; senao String.hashCode()
    if (s.empty()) return randomSeed();
    const bool neg = s[0] == '-'; size_t i = (neg || s[0] == '+') ? 1 : 0; bool num = i < s.size(); uint64_t acc = 0;
    for (size_t k = i; num && k < s.size(); k++) {
        if (s[k] < '0' || s[k] > '9') { num = false; break; }
        const uint64_t d = (uint64_t)(s[k] - '0'); if (acc > (UINT64_MAX - d) / 10) { num = false; break; }   // estouro => NumberFormatException => hashCode
        acc = acc * 10 + d;
    }
    if (num && acc > (neg ? 9223372036854775808ULL : 9223372036854775807ULL)) num = false;
    if (num) { const int64_t v = neg ? (int64_t)(0 - acc) : (int64_t)acc; return v != 0 ? v : randomSeed(); }
    uint32_t h = 0;   // String.hashCode(): h = 31*h + unidade UTF-16 (aqui decodifica o UTF-8 digitado)
    for (size_t k = 0; k < s.size();) {
        const unsigned char c = (unsigned char)s[k]; uint32_t cp = c; int n = 0;
        if (c >= 0xF0) { cp = c & 7; n = 3; } else if (c >= 0xE0) { cp = c & 15; n = 2; } else if (c >= 0xC0) { cp = c & 31; n = 1; }
        k++; for (; n > 0 && k < s.size(); n--, k++) cp = (cp << 6) | ((unsigned char)s[k] & 63);
        if (cp >= 0x10000) { cp -= 0x10000; h = h * 31u + (0xD800u + (cp >> 10)); h = h * 31u + (0xDC00u + (cp & 1023)); } else h = h * 31u + cp;
    }
    return (int64_t)(int32_t)h;
}
static std::string safeDirName(const std::string& n) {   // só ASCII seguro p/ nome de pasta (o nome bonito fica no level.cfg)
    std::string d; for (unsigned char c : n) d += (isalnum(c) || c == ' ' || c == '-' || c == '_') ? (char)c : '_';
    while (!d.empty() && (d.back() == ' ' || d.back() == '.')) d.pop_back(); while (!d.empty() && d[0] == ' ') d.erase(0, 1);
    return d.empty() ? "Mundo" : d;
}
static std::string createWorld(const std::string& name, const std::string& seedTxt, int mode) {
    std::error_code ec; fs::create_directories("saves", ec);
    const std::string base = safeDirName(name); std::string d = base; for (int n = 2; fs::exists("saves/" + d, ec); n++) d = base + " (" + std::to_string(n) + ")";
    fs::create_directories("saves/" + d, ec);
    FILE* f = fopen(("saves/" + d + "/level.cfg").c_str(), "wb"); if (!f) return "";
    fprintf(f, "name=%s\nseed=%lld\nmode=%d\nhardcore=%d\nplayed=%lld\n", name.c_str(), (long long)parseSeed(seedTxt), (mode == 2 ? 1 : mode == 3 ? 2 : mode == 4 ? 3 : 0), mode == 1 ? 1 : 0, (long long)time(nullptr)); fclose(f);   // sem px/py/pz: o jogo escolhe o spawn
    return d;
}
static bool dirOk(const std::string& d) { return !d.empty() && d.find('/') == std::string::npos && d.find('\\') == std::string::npos && d.find("..") == std::string::npos; }
static void deleteWorld(const std::string& d) { if (!dirOk(d)) return; std::error_code ec; fs::remove_all("saves/" + d, ec); }
static void renameWorld(const std::string& d, const std::string& nn) {
    if (!dirOk(d) || nn.empty()) return; const std::string p = "saves/" + d + "/level.cfg", t = slurp(p); std::string o; size_t q = 0; bool done = false;
    while (q < t.size()) { size_t e = t.find('\n', q); if (e == std::string::npos) e = t.size(); const std::string ln = t.substr(q, e - q); if (ln.compare(0, 5, "name=") == 0) { o += "name=" + nn + "\n"; done = true; } else o += ln + "\n"; q = e + 1; }
    if (!done) o = "name=" + nn + "\n" + o;
    FILE* f = fopen(p.c_str(), "wb"); if (f) { fwrite(o.data(), 1, o.size(), f); fclose(f); }
}
// ---- desenho ----
struct MR { int x, y, w, h; };
static void newWorldPage(); static void openSingleplayer(); static void pageGo(const std::string& name); static void drawPage(int w, int h, bool ingame); static bool pageEvent(const SDL_Event& e, int w, int h);   // telas em screens.inc
static bool inR(const MR& r, int ux, int uy) { return ux >= r.x && ux < r.x + r.w && uy >= r.y && uy < r.y + r.h; }
static int uWidth(const char* s) { int n = 0; for (const char* p = s; *p;) { int f, a; glyphMetrics(u8next(p), f, a); n += a; } return n; }
static void uCenter(int cx, float y, const char* s, unsigned c, bool sh = true) { uText((float)(cx - uWidth(s) / 2), y, s, c, sh); }
static void mBtn(const MR& r, const char* label, bool on, int ux, int uy) { PauseBtn b = {r.x, r.y, r.w, r.h, label}; pauseButton(b, !on ? 2 : inR(r, ux, uy) ? 1 : 0); }
static void mBg(int w, int h, float x0, float y0, float x1, float y1, float dark) {   // terra repetida, tile = 32 unidades
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, mtDirt); glColor4f(dark, dark, dark, gUiA);
    const float s = 32.f * G; quad(x0, y0, x1 - x0, y1 - y0, x0 / s, y0 / s, x1 / s, y1 / s);
}
static void mField(int x, int y, int w, const std::string& t, bool focus) {
    U((float)(x - 1), (float)(y - 1), (float)(w + 2), 22.f, focus ? 0xFFFFFF : 0xA0A0A0); U((float)x, (float)y, (float)w, 20.f, 0x000000);
    std::string s = t; if (focus && (SDL_GetTicks() / 500) % 2 == 0) s += "_"; uText((float)(x + 4), (float)(y + 6), s.c_str(), 0xE0E0E0, true);
}
static void mIcon(GLuint t, float ux, float uy, float s) { if (t) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, t); glColor4f(1, 1, 1, gUiA); quad(OX + ux * G, OY + uy * G, s * G, s * G, 0, 0, 1, 1); } }
static void mIcon2(GLuint t, float ux, float uy, float w, float h) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, t); glColor4f(1, 1, 1, gUiA); quad(OX + ux * G, OY + uy * G, w * G, h * G, 0, 0, 1, 1); }
static void drawMenu(int w, int h) {
    if (!mtInit) {
        mtInit = true; mtDirt = mtLoad("gui/options_background.png", "assets/textures/blocks/dirt_side.png", true);
        mtIcon[0] = mtLoad("gui/sprites/icon/language.png", nullptr, false); mtIcon[1] = mtLoad("gui/sprites/icon/accessibility.png", nullptr, false); mtIcon[2] = mtLoad("gui/sprites/icon/news.png", nullptr, false);
        mtUnk = mtLoad("misc/unknown_pack.png", nullptr, false); mtLogo = mtLoad("__x", "assets/textures/gui/logo.png", false);
    }
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0;
    const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, ux = gMX / G, uy = gMY / G;
    const float top = (float)(sh / 4);
    if (gScreen == SC_TITLE) {   // fundo = panorama 3D desenhado antes (drawPanorama)
        sndMusicStop();   // voltou ao titulo: para a musica do jogo
        const bool small = sh < 240;   // tela pequena (Raspberry 640x420): botoes mais juntos e rodape em 2 linhas, sem sobrepor
        const bool twoLn = uWidth(GAME_VERSION_TEXT) + uWidth("Copyright Pie Dev. Todos os direitos reservados.") + 8 > sw;   // versao e copyright nao cabem lado a lado
        uText(2.f, (float)sh - (twoLn ? 20 : 10), GAME_VERSION_TEXT, 0xFFFFFF, true);   // versao (canto inferior esquerdo)
        { const char* cp = "Copyright Pie Dev. Todos os direitos reservados."; uText((float)(sw - uWidth(cp) - 2), (float)sh - 10, cp, 0xFFFFFF, true); }
        if (mtLogo) { const float lw = 256.f, lh = lw * 382.f / 1024.f; mIcon2(mtLogo, cx - lw / 2, floorf((top + 48 - lh) / 2), lw, lh); }
        { const char* sp = "Faça de tudo!"; const float f = (1.8f - fabsf(sinf((SDL_GetTicks() % 1000) / 1000.f * 6.2831853f) * .1f)) * 100.f / (uWidth(sp) + 32);
          const float spHW = uWidth(sp) * .5f * f * 0.94f + 3.f; const float spX = std::min((float)(cx + 112), (float)sw - spHW - 2.f);   // splash girado: nao passa da borda direita
          glPushMatrix(); glTranslatef(spX * (float)G, (floorf((top + 48 + 95.f) / 2) - 36.f) * G, 0); glRotatef(-20.f, 0, 0, 1); glScalef(f, f, 1);
          text(-uWidth(sp) * .5f * G, -4.f * G, (float)G, sp, 1.f, 1.f, 0.f, (float)G, gUiA); glPopMatrix(); }
        const int y0 = (int)top + 48;
        const int g1 = small ? 22 : 24, g2 = small ? 44 : 48, g3 = small ? 70 : 84;
        MR b[7] = {{cx - 100, y0, 200, 20}, {cx - 100, y0 + g1, 200, 20}, {cx - 100, y0 + g2, 200, 20}, {cx - 100, y0 + g3, 98, 20}, {cx + 2, y0 + g3, 98, 20}, {cx - 124, y0 + g3, 20, 20}, {cx + 104, y0 + g3, 20, 20}};
        const char* lb[5] = {"Um jogador", "Multijogador", "Minecraft Realms", "Opções...", "Sair do jogo"};
        for (int i = 0; i < 5; i++) mBtn(b[i], lb[i], true, ux, uy);
        mBtn(b[5], "", true, ux, uy); mBtn(b[6], "", true, ux, uy); mIcon(mtIcon[0], (float)b[5].x + 2, (float)b[5].y + 2, 16); mIcon(mtIcon[1], (float)b[6].x + 2, (float)b[6].y + 2, 16); mIcon(mtIcon[2], (float)(b[2].x + 178), (float)b[2].y + 2, 16);
    } else drawPage(w, h, false);   // demais telas (screens.inc)
    glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
static void drawLoad(int w, int h, float prog, bool done, float alpha) {   // "Carregando nível" (fundo de terra escuro, barra 100x2); done = "Pronto!"
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0; const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, cy = sh / 2;
    if (alpha >= .999f) { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    gUiA = alpha;
    if (mtDirt) mBg(w, h, 0, 0, (float)w, (float)h, .25f);
    uCenter(cx, (float)(cy - 20), "Carregando nível", 0xFFFFFF); uCenter(cx, (float)(cy - 4 + 8), done ? "Pronto!" : "Construindo terreno", 0xFFFFFF);
    const float bx = (float)(cx - 50), by = (float)(cy + 16); U(bx, by, 100, 2, 0x808080, alpha); U(bx, by, 100.f * std::max(0.f, std::min(1.f, prog)), 2, 0x80FF80, alpha);
    gUiA = 1.f; glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
// spawn seguro: coluna de terra firme acima do mar, sem agua por perto, nem copa de arvore, com 2 blocos de ar livres
static bool spawnTop(int x, int z, int& top) {
    top = -1; for (int y = CH - 1; y >= 0; y--) { const int b = getB(x, y, z); if (b != AIR) { top = y; break; } }
    if (top < 1) return false; const int b = getB(x, top, z);
    return solid(b) && b != LEAVES && b != LOG && b != BEDROCK;
}
static void safeSpawn(int cx, int cz, double& ox, double& oy, double& oz) {
    const int bx = cx * CW + 8, bz = cz * CW + 8;
    for (int pass = 0; pass < 3; pass++)
        for (int d = 0; d <= 56; d++)
            for (int dz = -d; dz <= d; dz++) for (int dx = -d; dx <= d; dx++) {
                if (std::max(abs(dx), abs(dz)) != d) continue;
                const int x = bx + dx, z = bz + dz; int t;
                if (!spawnTop(x, z, t)) continue;
                if (pass < 2 && t <= SEA + 1) continue;
                if (pass == 0) { if (t > SEA + 40) continue; bool wet = false; for (int ax = -3; ax <= 3 && !wet; ax++) for (int az = -3; az <= 3; az++) { int tt; if (!spawnTop(x + ax, z + az, tt) || tt <= SEA) { wet = true; break; } } if (wet) continue; }
                if (getB(x, t + 1, z) != AIR || getB(x, t + 2, z) != AIR) continue;
                ox = x + .5; oy = t + 1.0; oz = z + .5; return;
            }
    ox = bx + .5; oz = bz + .5; oy = SEA + 30.0; for (int y = CH - 3; y > 0; y--) if (getB(bx, y, bz) != AIR) { oy = y + 1.0; break; }
}
static void drawFadeBlack(int w, int h, float a) {   // cobre a tela com preto (fade de saida do jogo / entrada do titulo)
    if (a <= 0.f) return; glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D); glColor4f(0, 0, 0, std::min(1.f, a)); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd();
    glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
static void goOther(const char* t) { gOtherTitle = t; pageGo("generico"); }
static void menuEvent(const SDL_Event& e, int w, int h) {
    G = guiScaleFor(w, h); const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, y0 = sh / 4 + 48;
    if (e.type == SDL_MOUSEMOTION) { gMX = e.motion.x; gMY = e.motion.y; if (gScreen == SC_OTHER) pageEvent(e, w, h); return; }
    if (gScreen == SC_LOAD) return;
    const int ux = gMX / G, uy = gMY / G;
    if (gScreen == SC_OTHER) { pageEvent(e, w, h); return; }
    if (e.type == SDL_KEYDOWN) { if (gScreen == SC_TITLE) { if (e.key.keysym.sym == SDLK_n) newWorldPage(); else if (e.key.keysym.sym == SDLK_m) pageGo("mods"); } return; }   // N = Criar Novo Mundo, M = Mod List (telas extras)
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) return;
    if (gScreen == SC_TITLE) {
        const bool small = sh < 240; const int g1 = small ? 22 : 24, g2 = small ? 44 : 48, g3 = small ? 70 : 84;   // igual ao drawTitle
        MR b[5] = {{cx - 100, y0, 200, 20}, {cx - 100, y0 + g1, 200, 20}, {cx - 100, y0 + g2, 200, 20}, {cx - 100, y0 + g3, 98, 20}, {cx + 2, y0 + g3, 98, 20}};
        for (const MR& q : b) if (inR(q, ux, uy)) { sndEvtUi("ui.click"); break; }   // patchAud.6: clique de botao do titulo
        if (inR(b[0], ux, uy)) openSingleplayer();
        else if (inR(b[1], ux, uy)) pageGo("multi");
        else if (inR(b[2], ux, uy)) goOther("Minecraft Realms");
        else if (inR(b[3], ux, uy)) pageGo("opcoes");
        else if (inR(b[4], ux, uy)) gMenuAct = 2;
        else if (inR({cx - 124, y0 + g3, 20, 20}, ux, uy)) pageGo("idioma");
        else if (inR({cx + 104, y0 + g3, 20, 20}, ux, uy)) goOther("Configurações de acessibilidade");
    }
}
