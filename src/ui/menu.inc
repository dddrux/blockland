// ================= menus: titulo (1.21.5) + mundo unico (saves/mundo) com chunks, seed e jogador salvos =================
// Um jogador -> entra direto no mundo salvo (cria na primeira vez). Placeholder: Multiplayer, Realms, Options etc. sem logica.
namespace fs = std::filesystem;
enum { SC_PLAY = 0, SC_TITLE, SC_WORLDS, SC_CREATE, SC_OTHER, SC_LOAD };
static int gScreen = SC_TITLE, gMenuAct = 0;   // gMenuAct: 1 = iniciar mundo gMenuDir, 2 = sair
static std::string gMenuDir, gCurDir, gOtherTitle;
static void spawnSave() { if (gCurDir.empty()) return; FILE* f = fopen(("saves/" + gCurDir + "/spawn.dat").c_str(), "w"); if (!f) return; fprintf(f, "%.3f %.3f %.3f\n", gSpX, gSpY, gSpZ); fclose(f); }
static bool spawnLoad() { if (gCurDir.empty()) return false; FILE* f = fopen(("saves/" + gCurDir + "/spawn.dat").c_str(), "r"); if (!f) return false; double x, y, z; bool ok = false; if (fscanf(f, "%lf %lf %lf", &x, &y, &z) == 3 && y > 0 && y < CH) { gSpX = x; gSpY = y; gSpZ = z; ok = true; } fclose(f); return ok; }
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
    std::string out; char lb[64];   // patchRpiOpt.8: monta o texto na memoria e so escreve no disco se mudou desde a ultima gravacao (autosave a cada 5 s abria/gravava o arquivo no cartao SD toda vez)
    auto put = [&](int idx, const Slot& s) { if (s.id <= 0 || s.n <= 0) return; if (isPk(s.id)) { snprintf(lb, sizeof lb, "p %d %d ", idx, s.n); out += lb; out += PK[s.id - PK0].file; out += "\n"; } else { snprintf(lb, sizeof lb, "i %d %d %d\n", idx, s.n, s.id); out += lb; }
        if (s.d > 0) { snprintf(lb, sizeof lb, "d %d\n", s.d); out += lb; }
        if (!s.nbt.empty() && s.nbt[0] != '?') { out += "x "; out += s.nbt; out += "\n"; } };   // "x <nbt>" logo depois da pilha: NBT (bau com conteudo)
    for (int i = 0; i < 36; i++) put(i, inv[i]);
    for (int i = 0; i < 4; i++) put(36 + i, armor[i]);
    put(40, offh); put(41, cur);
    snprintf(lb, sizeof lb, "hp %d %d\n", gHP, gFood); out += lb;
    static std::string lastOut, lastFile; const std::string pf = playerFile(); if (pf == lastFile && out == lastOut) return;
    FILE* f = fopen(pf.c_str(), "wb"); if (!f) return; fwrite(out.data(), 1, out.size(), f); fclose(f); lastOut = out; lastFile = pf;
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
struct ChEnc { bool pre = false; int64_t key = 0; const Chunk* c = nullptr; std::vector<uint16_t> o; std::vector<uint8_t> mo; };
struct ChEncJob { std::vector<ChEnc>* v; size_t a, b; };
static int chEncThread(void* p) {   // blocos em RLE de 16 bits + metadados em RLE (run u16, valor u8): so le o chunk, seguro em paralelo
    ChEncJob* j = (ChEncJob*)p; const size_t N = (size_t)CW * CH * CW;
    for (size_t k = j->a; k < j->b; k++) { ChEnc& e = (*j->v)[k]; if (e.pre) continue; const BT* b = e.c->b; const uint8_t* m = e.c->m;
        for (size_t i = 0; i < N;) { const uint16_t v = b[i]; size_t q = i + 1; while (q < N && b[q] == v && q - i < 65535) q++; e.o.push_back((uint16_t)(q - i)); e.o.push_back(v); i = q; }
        for (size_t i = 0; i < N;) { const uint8_t v = m[i]; size_t q = i + 1; while (q < N && m[q] == v && q - i < 65535) q++; const uint16_t r = (uint16_t)(q - i); e.mo.push_back((uint8_t)(r & 255)); e.mo.push_back((uint8_t)(r >> 8)); e.mo.push_back(v); i = q; } }
    return 0;
}
static void writeChunkFile(const std::string& path, std::unordered_map<int64_t, Chunk*>& M, const std::unordered_map<int64_t, std::vector<uint16_t>>* sc = nullptr, const std::unordered_map<int64_t, std::vector<uint8_t>>* sm = nullptr) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) { logError("Falha ao salvar os chunks em %s", path.c_str()); return; }
    fwrite("BL18", 1, 4, f);   // formato: paleta de nomes do pack (ids >= PK0 sobrevivem a troca de resource pack) + por chunk: blocos em RLE de 16 bits + metadados em RLE (run u16, valor u8)
    int32_t np = (int32_t)PK.size(); fwrite(&np, 4, 1, f);
    for (auto& p : PK) { uint16_t l = (uint16_t)p.file.size(); fwrite(&l, 2, 1, f); fwrite(p.file.data(), 1, l, f); }
    std::vector<ChEnc> E; E.reserve(M.size() + (sc ? sc->size() : 0)); for (auto& kv : M) { ChEnc e; e.key = kv.first; e.c = kv.second; E.push_back(std::move(e)); }
    if (sc) {   // patchUnload.1: chunks que NAO estao em memoria (descarregados ou ainda nao revisitados) ja estao codificados em savedC/savedM: entram no arquivo como estao
        std::vector<uint8_t> zm; for (auto& kv : *sc) { if (M.count(kv.first)) continue; ChEnc e; e.pre = true; e.key = kv.first; e.o = kv.second;
            if (sm) { auto it = sm->find(kv.first); if (it != sm->end()) e.mo = it->second; }
            if (e.mo.empty()) { if (zm.empty()) { std::vector<uint8_t> z((size_t)CW * CH * CW, 0); zm = savedEncM(z.data(), z.size()); } e.mo = zm; }
            E.push_back(std::move(e)); } }
    int32_t n = (int32_t)E.size(); fwrite(&n, 4, 1, f);
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
// patchMP.2: o save temporario saves/_mp e apagado a cada entrada em servidor => inventario/posicao do convidado resetavam ao trocar de mundo. Guarda por seed em saves/_guest/<seed>/ (ignorado na lista de mundos: comeca com '_')
static std::string guestStashDir(long long seed) { char k[48]; snprintf(k, sizeof k, "_%08x_%u", (unsigned)ntohl(netHostAddr.sin_addr.s_addr), (unsigned)ntohs(netHostAddr.sin_port)); return "saves/_guest/" + std::to_string(seed) + k; }   // BUG-061: chave = seed + IP:porta do host (antes so seed: servidores diferentes com o mesmo seed compartilhavam inventario)
static void guestStash(long long seed, double x, double y, double z) {
    if (gCurDir != "_mp") return; std::error_code ec; const std::string d = guestStashDir(seed); fs::create_directories(d, ec);
    fs::copy_file("saves/_mp/player_guest.dat", d + "/player_guest.dat", fs::copy_options::overwrite_existing, ec);
    FILE* f = fopen((d + "/pos.txt").c_str(), "wb"); if (f) { fprintf(f, "%.3f %.3f %.3f\n", x, y, z); fclose(f); }
}
static bool guestRestore(long long seed, double& x, double& y, double& z) {   // chamado depois de recriar saves/_mp; true se havia posicao salva
    std::error_code ec; const std::string d = guestStashDir(seed);
    if (fs::exists(d + "/player_guest.dat", ec)) fs::copy_file(d + "/player_guest.dat", "saves/_mp/player_guest.dat", fs::copy_options::overwrite_existing, ec);
    double a, b, c; FILE* f = fopen((d + "/pos.txt").c_str(), "rb"); if (!f) return false; const bool ok = fscanf(f, "%lf %lf %lf", &a, &b, &c) == 3; fclose(f);
    if (ok) { x = a; y = b; z = c; } return ok;
}
static void saveCurWorld(double x, double y, double z) {
    if (gCurDir.empty()) return;
    if (gChest) chestClosed();   // baú aberto: fecha a tampa (bit 7 do meta) e manda o conteúdo antes de salvar
    const Uint32 svT0 = SDL_GetTicks(); logInfo("Saving world '%s'...", gCurDir.c_str());
    savePlayer();
    if (netSock >= 0 && !netHost) { guestStash((long long)WORLD_SEED, x, y, z); return; }   // convidado: o mundo e do host (so o jogador e salvo; patchMP.2 guarda por seed)
    boatsSave(gCurDir); cartsSave(gCurDir); chestsSave(gCurDir); furnSave(gCurDir); brewSave(gCurDir); propsWorldSave(gCurDir); signsSave(gCurDir); dragonWorldSave(gCurDir);
    SaveInfo s; s.dir = gCurDir; s.seed = WORLD_SEED; s.mode = gGm; s.hard = gHard ? 1 : 0; s.played = (long long)time(nullptr); s.px = x; s.py = y; s.pz = z; s.wtime = gWorldTime; s.wx = gWeather; s.wxl = gWxLeft;
    { std::string t0; FILE* fc = fopen(("saves/" + gCurDir + "/level.cfg").c_str(), "rb"); if (fc) { char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, fc)) > 0) t0.append(b, r); fclose(fc); }
      s.name = cfgGet(t0, "name"); if (s.name.empty()) s.name = gCurDir; }   // mantém o nome dado na criação/edição
    writeCfg(s);
    { writeChunkFile("saves/" + gCurDir + "/chunks16.dat", dimWorld(0), &dimSavedC(0), &dimSavedM(0));   // superficie
      if (!dimWorld(1).empty() || !dimSavedC(1).empty()) writeChunkFile("saves/" + gCurDir + "/chunks16n.dat", dimWorld(1), &dimSavedC(1), &dimSavedM(1));   // nether
      if (!dimWorld(2).empty() || !dimSavedC(2).empty()) writeChunkFile("saves/" + gCurDir + "/chunks16e.dat", dimWorld(2), &dimSavedC(2), &dimSavedM(2));   // end
      if (!dimWorld(3).empty() || !dimSavedC(3).empty()) writeChunkFile("saves/" + gCurDir + "/chunks16r.dat", dimWorld(3), &dimSavedC(3), &dimSavedM(3));   // patchBlood.1: Dimensao Rubra
      if (FILE* fd = fopen(("saves/" + gCurDir + "/dim.txt").c_str(), "wb")) { fprintf(fd, "%d", gDim); fclose(fd); } }
    modulesWorldSave(gCurDir);   // patchMod.2
    logInfo("Mundo salvo em %u ms (%zu chunks)", (unsigned)(SDL_GetTicks() - svT0), dimWorld(0).size());
}
static void resetWorldData() {
    modulesWorldReset();   // patchMod.2
    genJobAbort();   // patchGenSlice.1: chunk meio gerado de um mundo que esta sendo descartado
    netherReset();   // libera a outra dimensao (nether.inc) e volta para a superficie
    static auto pDel = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    for (auto& kv : W) { Chunk* c = kv.second; if (pDel) { if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo); } delete[] c->lc; delete c; }
    gChest = false; gChEnder = false; gChWait = false;   // tela de baú aberta num mundo que está sendo descartado
    resetBlockUpdates(); W.clear(); getCReset(); gKick = true; gEpoch++; savedC.clear(); savedM.clear(); gSpill.clear(); gFloor.clear(); keepLeaves.clear(); leafPending.clear(); wqCur.clear(); lqCur.clear(); lqNext.clear(); lqSet.clear(); wqNext.clear(); wqSet.clear(); MOBS.clear(); BOATS.clear(); gBoatRide = 0; gBoatCamId = 0; DROPS.clear(); chestsClear();
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
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, mtDirt); glColor4f(dark, dark, dark, gUiA);
    const float s = 32.f * G; quad(x0, y0, x1 - x0, y1 - y0, x0 / s, y0 / s, x1 / s, y1 / s);
}
static void invDirtQuad(float x, float y, float w, float h, float dark) {   // patchInvTouch.1: terra do inventario no celular (carrega a textura sob demanda)
    if (!mtDirt) mtDirt = mtLoad("gui/options_background.png", "assets/textures/blocks/dirt_side.png", true);
    if (!mtDirt) { flat(x, y, w, h, .28f * dark / .3f, .2f * dark / .3f, .12f * dark / .3f, 1.f); return; }
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, mtDirt); glColor4f(dark, dark, dark, 1.f);
    const float t = 32.f * G; quad(x, y, w, h, x / t, y / t, (x + w) / t, (y + h) / t);
}
static void mField(int x, int y, int w, const std::string& t, bool focus) {
    U((float)(x - 1), (float)(y - 1), (float)(w + 2), 22.f, focus ? 0xFFFFFF : 0xA0A0A0); U((float)x, (float)y, (float)w, 20.f, 0x000000);
    std::string s = t; if (focus && (SDL_GetTicks() / 500) % 2 == 0) s += "_"; uText((float)(x + 4), (float)(y + 6), s.c_str(), 0xE0E0E0, true);
}
static void mIcon(GLuint t, float ux, float uy, float s) { if (t) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, t); glColor4f(1, 1, 1, gUiA); quad(OX + ux * G, OY + uy * G, s * G, s * G, 0, 0, 1, 1); } }
static void mIcon2(GLuint t, float ux, float uy, float w, float h) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, t); glColor4f(1, 1, 1, gUiA); quad(OX + ux * G, OY + uy * G, w * G, h * G, 0, 0, 1, 1); }
// ---- patchPE.2: titulo estilo MCPE 1.1.5 (Android, Raspberry e PC com o HUD touch ligado: F8) ----
static bool peUi() {   // patchJava.5: PC = titulo do Java 1.12 (Um jogador/Multijogador/Realms/Opcoes/Sair); Android = titulo PE; BL_MENU=pe volta o PE no PC
#ifdef __ANDROID__
    return true;
#else
    static const bool pe = [] { const char* e = getenv("BL_MENU"); return e && !strcmp(e, "pe"); }(); return pe;
#endif
}   // (antes: sempre true, patchPE.3)   // patchPE.3: titulo PE e o principal em todas as plataformas (F8 so liga o HUD touch no jogo)
static void skFrame(float x, float y, float w, float h, unsigned col, float t = 1.f, float al = 1.f) {   // contorno retangular (4 faixas) em unidades da GUI
    U(x, y, w, t, col, al); U(x, y + h - t, w, t, col, al); U(x, y + t, t, h - 2 * t, col, al); U(x + w - t, y + t, t, h - 2 * t, col, al);
}
static void titleLogoBox(int sw, int sh, float& lw, float& lh, float& ly) {   // logo + menu formam um bloco centralizado na vertical
    lw = sh < 240 ? 170.f : sh < 330 ? 235.f : 290.f; lw = std::min(lw, sw * .9f); lh = lw * 382.f / 1024.f;
#ifdef __ANDROID__
    const float total = lh + 18.f + 80.f;
#else
    const float total = lh + 24.f + 108.f;
#endif
    ly = std::max(sh < 240 ? 4.f : 8.f, (sh - total) * .30f);
}
static void peTitleRects(int sw, int sh, MR* o) {   // 0 Jogar, 1 Multijogador, 2 Configuracoes, 3 Mods, 4 boneco = botao das skins, 5 sair (so PC), 6 = pes do boneco (x,y)
    float lw, lh, ly; titleLogoBox(sw, sh, lw, lh, ly);   // patchTitle.1: menu logo abaixo do logo (nada se sobrepoe em qualquer tamanho de tela)
    const int bw = std::min(200, sw * 56 / 100), y0 = (int)(ly + lh) + (sh < 240 ? 10 : 18), cx = sw / 2, hw = (bw - 4) / 2;   // patchMenu.4: botoes 200x20 (mesma grade do menu de pausa), grupos com respiro de 8
    o[0] = {cx - bw / 2, y0, bw, 20}; o[1] = {cx - bw / 2, y0 + 24, bw, 20}; o[2] = {cx - bw / 2, y0 + 52, hw, 20}; o[3] = {cx - bw / 2 + bw - hw, y0 + 52, hw, 20};
    o[4] = {0, 0, 0, 0}; o[5] = {cx - bw / 2, y0 + 80, bw, 20}; o[6] = {o[4].x + 10, o[4].y + 20, 0, 0};   // patchSkin.5: cabeca do jogador (botao quadrado) ao lado de Configuracoes/Mods = perfil e skin
}
// patchSkin.4: blur do panorama atras do menu (captura da tela + varias copias deslocadas) e redesenho do fundo borrado com alfa para o fade-in do boneco
static GLuint gBlurTex = 0; static int gBlurW = 0, gBlurH = 0;
static void blurCapture(int w, int h) {
    if (!gBlurTex) glGenTextures(1, &gBlurTex);
    glBindTexture(GL_TEXTURE_2D, gBlurTex);
    if (w != gBlurW || h != gBlurH) { glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); gBlurW = w; gBlurH = h; }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
}
static void blurQuad(int w, int h, float ox, float oy, float a) {
    glColor4f(1.f, 1.f, 1.f, a); glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(ox, oy); glTexCoord2f(1, 1); glVertex2f(ox + w, oy); glTexCoord2f(1, 0); glVertex2f(ox + w, oy + h); glTexCoord2f(0, 0); glVertex2f(ox, oy + h); glEnd();
}
static void blurQuadR(float qw, float qh, float u, float v, float a) {   // quad (0,0)-(qw,qh) com a parte (0..u, 0..v) da textura (v=1 e o topo)
    glColor4f(1.f, 1.f, 1.f, a); glBegin(GL_QUADS);
    glTexCoord2f(0, v); glVertex2f(0, 0); glTexCoord2f(u, v); glVertex2f(qw, 0); glTexCoord2f(u, 0); glVertex2f(qw, qh); glTexCoord2f(0, 0); glVertex2f(0, qh); glEnd();
}
static void blurScreen(int w, int h) {   // patchSkin.8: desfoque suave por reducao (1/2, depois 1/4, filtro bilinear) e ampliacao = sem "fantasmas" das copias deslocadas
    if (w < 16 || h < 16) return;
    rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdDisable(GL_SCISSOR_TEST); rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D);
    auto setup = [&](int vw, int vh) { rdViewport(0, 0, vw, vh); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, vw, vh, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); };
    blurCapture(w, h);
    const int w2 = w / 2, h2 = h / 2, w4 = w / 4, h4 = h / 4;
    setup(w2, h2); blurQuadR((float)w2, (float)h2, 1.f, 1.f, 1.f); glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w2, h2);
    setup(w4, h4); blurQuadR((float)w4, (float)h4, w2 / (float)w, h2 / (float)h, 1.f); glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w4, h4);
    setup(w, h); blurQuadR((float)w, (float)h, w4 / (float)w, h4 / (float)h, 1.f);
    blurCapture(w, h); glColor4f(1.f, 1.f, 1.f, 1.f); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW);
}
static void uHead3D(float cx, float cy, float u, const uint8_t* d, float alpha); static void skinScan(); static int skinIndex();
// patchSplash.1: frases do texto amarelo do menu (escolhe uma por abertura do jogo; "Faça de tudo!" continua a mais comum)
static const char* menuSplash() {
    static const char* const L[] = {"Faça de tudo!", "Faça de tudo!", "Cavar, cavar, cavar!", "Cuidado com o creeper!", "Feito em C++!", "Também roda no celular!", "Bloco por bloco!",
        "Não olhe nos olhos do enderman!", "Já plantou hoje?", "Tem uma caverna ali!", "Diamante? Só mais um nível!", "Ssssss...", "Nunca cave para baixo!", "Leve tochas!",
        "O Nether te espera!", "Construa algo incrível!", "Multijogador na rede local!", "Tela dividida!", "Sobreviva à primeira noite!", "Vaca é vida!",
        "Cuidado com a lava!", "Durma antes do anoitecer!", "Remando de bote!", "Slime pula, slime quica!", "O dragão te aguarda!", "Minere com estilo!",
        "Tudo é cubo!", "Sem lag (quase)!", "Quase sem bugs!", "Gosta de abóbora?", "Pegue o tridente!", "Hora de explorar!", "Um bloco de cada vez!",
        "Cuidado onde pisa!", "Alimente os animais!", "Já viu o pôr do sol?", "Ovelha rosa existe!", "Bote para dois!", "Redstone é magia!", "Faça um farol!"};
    static int i = -1; if (i < 0) i = (int)((SDL_GetTicks() * 2654435761u + (unsigned)time(nullptr)) % (sizeof L / sizeof *L));
    return L[i];
}
static void drawTitlePE(int sw, int sh, int ux, int uy) {
    const int cx = sw / 2; MR b[7]; peTitleRects(sw, sh, b);
    const char* cp = "Copyright Pie Dev."; uText(2.f, (float)sh - 10, cp, 0xFFFFFF, true); uText((float)(sw - uWidth(GAME_VERSION_TEXT) - 2), (float)sh - 10, GAME_VERSION_TEXT, 0xFFFFFF, true);
    float lw, lh, ly; titleLogoBox(sw, sh, lw, lh, ly);
    if (mtLogo) mIcon2(mtLogo, cx - lw / 2, ly, lw, lh);
    { const char* sp = menuSplash(); const float sc = lw / 260.f, pul = 1.f - fabsf(sinf((SDL_GetTicks() % 1000) / 1000.f * 6.2831853f) * .055f);   // patchMenu.1: splash preso ao canto inferior direito do logo, escala com o logo
      const float f0 = std::max(.55f, 1.15f * sc), f = f0 * std::min(1.f, lw * .62f / std::max(1.f, uWidth(sp) * f0)) * pul, tw = uWidth(sp) * f, half = tw * .5f * .94f + 3.f;
      float px = cx + lw * .5f - tw * .3f, py = ly + lh * .68f;   /* patchUI.1: sobe o splash amarelo (o PNG do logo tem margem transparente embaixo) */
      px = std::min(px, (float)sw - 2.f - half); px = std::max(px, half + 2.f);
      rdPushMatrix(); rdTranslatef(px * (float)G, py * (float)G, 0); rdRotatef(-20.f, 0, 0, 1); rdScalef(f, f, 1);
      text(-uWidth(sp) * .5f * G + (float)G * .6f, -4.f * G + (float)G * .6f, (float)G, sp, .25f, .25f, 0.f, (float)G, gUiA * .8f);   // sombra
      text(-uWidth(sp) * .5f * G, -4.f * G, (float)G, sp, 1.f, 1.f, 0.f, (float)G, gUiA); rdPopMatrix(); }
    mBtn(b[0], "Jogar", true, ux, uy); mBtn(b[1], "Multijogador", true, ux, uy); mBtn(b[2], "Configurações", true, ux, uy); mBtn(b[3], "Mods", true, ux, uy);
#ifndef __ANDROID__
    mBtn(b[5], "Sair", true, ux, uy);
#endif
    if (!gSkinHintSeen && !splashActive()) {   // patchSkin.6: aviso apontando para Configuracoes (launcher antigo descontinuado)
        static const char* const ln[4] = {"Launcher antigo", "descontinuado! Agora", "fica em Configurações", "> Perfil."};
        int tw = 0; for (const char* t : ln) tw = std::max(tw, uWidth(t)); const int bw = tw + 14, bh = 4 * 11 + 10; const int bx = std::max(2, b[2].x - 18 - bw), by = std::max(4, b[2].y + 10 - bh / 2);
        U((float)bx, (float)by, (float)bw, (float)bh, 0x000000, .85f); skFrame((float)bx, (float)by, (float)bw, (float)bh, 0xFFFF55, 1.f);
        for (int k = 0; k < 4; k++) uText((float)(bx + 7), (float)(by + 6 + k * 11), ln[k], k == 0 ? 0xFFFF55 : 0xFFFFFF, true);
        const float bob = sinf(SDL_GetTicks() / 180.f) * 2.f, ax = (float)(bx + bw + 2) + bob, ay = (float)(b[2].y + 10);   // seta para a direita (Configuracoes)
        U(ax, ay - 1.f, 8.f, 3.f, 0xFFFF55, 1.f); for (int r = 0; r < 5; r++) U(ax + 8.f + r, ay - 5.f + r, 1.f, 11.f - 2.f * r, 0xFFFF55, 1.f);
    }
   // dica; o contorno (aura) e desenhado em volta da silhueta do boneco
}
static void mIconFb(int kind, float x, float y) {   // patchFix.Cape: icones 16x16 desenhados por codigo (o pack nao traz gui/sprites/icon/*.png): 0 idioma (globo), 1 acessibilidade (figura)
    for (int j = 0; j < 16; j++) for (int i = 0; i < 16; i++) {
        const float dx = i - 7.5f, dy = j - 7.5f; bool on = false;
        if (kind == 0) { const float r = sqrtf(dx * dx + dy * dy), e = sqrtf((dx / 3.4f) * (dx / 3.4f) + (dy / 6.5f) * (dy / 6.5f));
            on = fabsf(r - 6.5f) < .75f || (r < 6.6f && (fabsf(dy) < .6f || fabsf(dx) < .6f || fabsf(e - 1.f) < .15f)); }
        else { const float hr = sqrtf(dx * dx + (j - 2.5f) * (j - 2.5f));
            on = hr < 1.9f || (j >= 5 && j <= 6 && i >= 2 && i <= 13) || (j >= 7 && j <= 10 && i >= 6 && i <= 9) || (j >= 11 && j <= 15 && ((i >= 6 - (j - 11) / 2 - 0 && i < 8 - (j - 11) / 2) || (i >= 8 + (j - 11) / 2 && i < 10 + (j - 11) / 2))); }
        if (on) U(x + i, y + j, 1.f, 1.f, 0xFFFFFF, gUiA);
    }
}
static void drawMenu(int w, int h) {
    if (!mtInit) {
        mtInit = true; if (!mtDirt) mtDirt = mtLoad("gui/options_background.png", "assets/textures/blocks/dirt_side.png", true);
        mtIcon[0] = mtLoad("gui/sprites/icon/language.png", "assets/textures/gui/icon_language.png", false); mtIcon[1] = mtLoad("gui/sprites/icon/accessibility.png", "assets/textures/gui/icon_accessibility.png", false);   /* patchIcone.1: o pack nao traz gui/sprites/icon/*; PNG embutido em assets/textures/gui (mesmo caminho do logo); sem ele, mIconFb desenha por codigo */ mtIcon[2] = mtLoad("gui/sprites/icon/news.png", nullptr, false);
        mtUnk = mtLoad("misc/unknown_pack.png", nullptr, false); mtLogo = mtLoad("__x", "assets/textures/gui/logo.png", false);
    }
    rdViewport(0, 0, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0;
    const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, ux = gMX / G, uy = gMY / G;
    const float top = (float)(sh / 4);
    if (gScreen == SC_TITLE && peUi()) { sndMusicStop(); drawTitlePE(sw, sh, ux, uy); }
    else if (gScreen == SC_TITLE) {   // fundo = panorama 3D desenhado antes (drawPanorama)
        sndMusicStop();   // voltou ao titulo: para a musica do jogo
        {   // patchJava.5: GuiMainMenu.drawScreen: degrade branco 50% no topo -> transparente, depois transparente -> preto 50% embaixo (sobre o panorama)
            rdDisable(GL_TEXTURE_2D); glBegin(GL_QUADS);
            glColor4f(1.f, 1.f, 1.f, .5f * gUiA); glVertex2f(0.f, 0.f); glVertex2f((float)w, 0.f); glColor4f(1.f, 1.f, 1.f, 0.f); glVertex2f((float)w, (float)h); glVertex2f(0.f, (float)h);
            glColor4f(0.f, 0.f, 0.f, 0.f); glVertex2f(0.f, 0.f); glVertex2f((float)w, 0.f); glColor4f(0.f, 0.f, 0.f, .5f * gUiA); glVertex2f((float)w, (float)h); glVertex2f(0.f, (float)h);
            glEnd(); rdEnable(GL_TEXTURE_2D);
        }
        const bool small = sh < 240 || BL_SMALL_UI;   // tela pequena (Raspberry 640x420): botoes mais juntos e rodape em 2 linhas, sem sobrepor
        const bool twoLn = uWidth(GAME_VERSION_TEXT) + uWidth("Copyright Pie Dev. Todos os direitos reservados.") + 8 > sw;   // versao e copyright nao cabem lado a lado
        uText(2.f, (float)sh - (twoLn ? 20 : 10), GAME_VERSION_TEXT, 0xFFFFFF, true);   // versao (canto inferior esquerdo)
        { const char* cp = "Copyright Pie Dev. Todos os direitos reservados."; uText((float)(sw - uWidth(cp) - 2), (float)sh - 10, cp, 0xFFFFFF, true); }
        if (mtLogo) { const float lw = 256.f, lh = lw * 382.f / 1024.f; mIcon2(mtLogo, cx - lw / 2, floorf((top + 48 - lh) / 2), lw, lh); }
        { const char* sp = menuSplash(); const float f = (1.8f - fabsf(sinf((SDL_GetTicks() % 1000) / 1000.f * 6.2831853f) * .1f)) * 100.f / (uWidth(sp) + 32);
          const float spHW = uWidth(sp) * .5f * f * 0.94f + 3.f; const float spX = std::min((float)(cx + 112), (float)sw - spHW - 2.f);   // splash girado: nao passa da borda direita
          rdPushMatrix(); rdTranslatef(spX * (float)G, (floorf((top + 48 + 95.f) / 2) - 36.f) * G, 0); rdRotatef(-20.f, 0, 0, 1); rdScalef(f, f, 1);
          text(-uWidth(sp) * .5f * G, -4.f * G, (float)G, sp, 1.f, 1.f, 0.f, (float)G, gUiA); rdPopMatrix(); }
        const int y0 = (int)top + 48;
        const int g1 = small ? 22 : 24, g2 = small ? 44 : 48, g3 = small ? 70 : 84;
        MR b[7] = {{cx - 100, y0, 200, 20}, {cx - 100, y0 + g1, 200, 20}, {cx - 100, y0 + g2, 200, 20}, {cx - 100, y0 + g3, 98, 20}, {cx + 2, y0 + g3, 98, 20}, {cx - 124, y0 + g3, 20, 20}, {cx + 104, y0 + g3, 20, 20}};
        const char* lb[5] = {"Um jogador", "Multijogador", "Minecraft Realms", "Opções...", "Sair do jogo"};
        for (int i = 0; i < 5; i++) mBtn(b[i], lb[i], true, ux, uy);
        mBtn(b[5], "", true, ux, uy); mBtn(b[6], "", true, ux, uy); if (mtIcon[0]) mIcon(mtIcon[0], (float)b[5].x + 2, (float)b[5].y + 2, 16); else mIconFb(0, (float)b[5].x + 2, (float)b[5].y + 2); if (mtIcon[1]) mIcon(mtIcon[1], (float)b[6].x + 2, (float)b[6].y + 2, 16); else mIconFb(1, (float)b[6].x + 2, (float)b[6].y + 2); mIcon(mtIcon[2], (float)(b[2].x + 178), (float)b[2].y + 2, 16);
    } else drawPage(w, h, false);   // demais telas (screens.inc)
    rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
}
static GLuint gIntroBgTex = 0, gLoadBgTex = 0, gMenuBgTex = 0;   // gMenuBgTex = panorama borrado do menu (cache/menu_bg.png): aparece NA HORA ao voltar do jogo
//    // gLoadBgTex = terreno diferente do menu (cache/load_bg.png), so na geracao de mundo
//    // patchIntro.2: fundo em cache (panorama borrado salvo em cache/intro_bg.png); usado na abertura e na geracao de terreno
static bool introBgDraw(GLuint tex, int w, int h, float a) {   // desenha o fundo em cache cobrindo a tela (corte central, sem distorcer); false se nao ha cache
    if (!tex || a <= 0.f) return false;
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); glColor4f(1, 1, 1, a); glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0); glTexCoord2f(1, 0); glVertex2f((float)w, 0); glTexCoord2f(1, 1); glVertex2f((float)w, (float)h); glTexCoord2f(0, 1); glVertex2f(0, (float)h); glEnd();
    return true;
}
static void drawLoad(int w, int h, float prog, bool done, float alpha) {   // patchIntro.1: geracao de terreno estilo console (fundo borrado, titulo, barra verde quadrada, dica em caixa)
    rdViewport(0, 0, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0; const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, cy = sh / 2; const float ux = sw * .75f;   /* patchLoad.3: coluna de UI na metade direita (ceu/sol); o castelo do fundo fica livre na esquerda */
    if (alpha >= .999f) { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    gUiA = alpha;
    if (introBgDraw(gLoadBgTex, w, h, alpha)) {} else if (mtDirt) mBg(w, h, 0, 0, (float)w, (float)h, .25f);   // fundo = panorama borrado do titulo
    rdDisable(GL_TEXTURE_2D); glBegin(GL_QUAD_STRIP); for (int k = 0; k <= 8; k++) { const float y = h * k / 8.f, a = (.10f + .40f * k / 8.f) * alpha; glColor4f(0, 0, 0, a); glVertex2f(0, y); glVertex2f((float)w, y); } glEnd();   // escurece de leve, mais embaixo
    rdEnable(GL_TEXTURE_2D);
    if (mtLogo) { const float lw = std::min(sw * .34f, 190.f), lh = lw * 382.f / 1024.f; mIcon2(mtLogo, ux - lw / 2, std::max(8.f, sh * .07f), lw, lh); }
    const float bw = std::min(300.f, sw * .40f), bx = ux - bw / 2, by = (float)(cy + 2), bh = 5.f; const float pg = done ? 1.f : std::max(0.f, std::min(1.f, prog));
    const char* tt = done ? "Mundo pronto!" : "Iniciando o servidor"; const float tsc = std::max(.8f, std::min(2.f, (sw - bx - 6.f) / std::max(1.f, (float)uWidth(tt)))); rdPushMatrix(); rdTranslatef(bx * G, (cy - 34.f) * G, 0); rdScalef(tsc, tsc, 1.f); uText(0.f, 0.f, tt, 0xFFFFFF, true); rdPopMatrix();   /* patchUI.1: titulo nunca passa da borda direita, em qualquer escala */   // titulo alinhado a esquerda, na mesma coluna da barra
    uText(bx, by - 11.f, done ? "Pronto!" : "Gerando área de spawn", 0xD8D8D8, true); char pc[8]; snprintf(pc, sizeof pc, "%d%%", (int)(pg * 100.f + .5f)); uText(bx + bw - uWidth(pc), by - 11.f, pc, 0xD8D8D8, true);
    U(bx, by, bw, bh, 0x3A3F44, alpha * .85f); const float fw = std::floor(bw * pg); if (fw > 0.f) U(bx, by, fw, bh, 0x35D43B, alpha);   // barra flat: trilho + preenchimento liso, sem moldura/brilho
    static const char* tips[] = { "Conecte controles extras e aperte START neles para entrar na partida a qualquer momento.", "Aperte START em um controle extra para dividir a tela entre 2 e 4 jogadores.", "Dê duplo clique no boneco do título para trocar a skin.",
        "Dormir numa cama define o seu ponto de renascimento e faz a noite passar.", "Funis movem itens entre baús, fornalhas e dispensers.", "Use tochas para iluminar cavernas e impedir que monstros apareçam.", "O TNT na água não destrói blocos.", "Fornalhas funcionam com carvão, madeira e até baldes de lava." };
    static Uint32 lastCall = 0; static int tip = 0; const Uint32 nowT = SDL_GetTicks(); if (nowT - lastCall > 1500) tip = (int)(nowT / 7u) % 8; lastCall = nowT;   // nova dica a cada novo carregamento
    const float tw = std::min(sw * .40f, 300.f), tx = ux - tw / 2; std::vector<std::string> ln; { std::string cur, wd; const char* t = tips[tip];
        for (const char* q = t;; q++) { if (*q == ' ' || !*q) { std::string nx = cur.empty() ? wd : cur + " " + wd; if (uWidth(nx.c_str()) > tw - 20 && !cur.empty()) { ln.push_back(cur); cur = wd; } else cur = nx; wd.clear(); if (!*q) break; } else wd += *q; } if (!cur.empty()) ln.push_back(cur); }
    const float th = std::max(2, (int)ln.size()) * 11.f + 14.f, ty = std::min((float)sh - th - 10.f, sh * .72f);   // caixa larga embaixo, como no console: fundo azul-petroleo e contorno branco fino
    U(tx, ty, tw, th, 0x233A40, alpha * .92f); U(tx, ty, tw, 1, 0xFFFFFF, alpha); U(tx, ty + th - 1, tw, 1, 0xFFFFFF, alpha); U(tx, ty, 1, th, 0xFFFFFF, alpha); U(tx + tw - 1, ty, 1, th, 0xFFFFFF, alpha);
    for (size_t k = 0; k < ln.size(); k++) uText(tx + 10, ty + 7 + k * 11.f, ln[k].c_str(), 0xFFFFFF, true);
    gUiA = 1.f; rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
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
    // patchSeed.1: seed com oceano em volta da origem (nenhuma terra em 56 blocos): procura terra firme em aneis de chunks cada vez mais longe (gera o chunk se faltar); antes nascia na superficie da agua
    for (int r = 5; r <= 14; r++)
        for (int dz = -r; dz <= r; dz++) for (int dx = -r; dx <= r; dx++) {
            if (std::max(abs(dx), abs(dz)) != r) continue;
            const int ccx = cx + dx, ccz = cz + dz; if (!getC(ccx, ccz)) gen(ccx, ccz);
            for (int oz2 = 4; oz2 < 16; oz2 += 4) for (int ox2 = 4; ox2 < 16; ox2 += 4) {
                const int x = ccx * CW + ox2, z = ccz * CW + oz2; int t;
                if (!spawnTop(x, z, t) || t <= SEA + 1 || t > SEA + 60) continue;
                bool wet = false; for (int ax = -1; ax <= 1 && !wet; ax++) for (int az = -1; az <= 1; az++) { int tt; if (!spawnTop(x + ax, z + az, tt) || tt <= SEA) { wet = true; break; } }
                if (wet || getB(x, t + 1, z) != AIR || getB(x, t + 2, z) != AIR) continue;
                ox = x + .5; oy = t + 1.0; oz = z + .5; return;
            }
        }
    ox = bx + .5; oz = bz + .5; oy = SEA + 30.0; for (int y = CH - 3; y > 0; y--) if (getB(bx, y, bz) != AIR) { oy = y + 1.0; break; }
}
static void drawFadeBlack(int w, int h, float a) {   // cobre a tela com preto (fade de saida do jogo / entrada do titulo)
    if (a <= 0.f) return; rdViewport(0, 0, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rdDisable(GL_TEXTURE_2D); glColor4f(0, 0, 0, std::min(1.f, a)); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd();
    rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
}
static void goOther(const char* t) { gOtherTitle = t; pageGo("generico"); }
static void menuEvent(const SDL_Event& e, int w, int h) {
    G = guiScaleFor(w, h); const int sw = (w + G - 1) / G, sh = (h + G - 1) / G, cx = sw / 2, y0 = sh / 4 + 48;
    if (e.type == SDL_MOUSEMOTION) { gMX = e.motion.x; gMY = e.motion.y; if (gScreen == SC_OTHER) pageEvent(e, w, h); return; }
    if (gScreen == SC_LOAD) return;
    const int ux = gMX / G, uy = gMY / G;
    if (gScreen == SC_OTHER) { pageEvent(e, w, h); return; }
    if (e.type == SDL_KEYDOWN && gScreen == SC_TITLE && e.key.keysym.sym == SDLK_F8) { touchHud = !touchHud; touchReset(); return; }   // PC: liga/desliga o layout mobile no titulo
    if (e.type == SDL_KEYDOWN) { if (gScreen == SC_TITLE) { if (e.key.keysym.sym == SDLK_n) newWorldPage(); else if (e.key.keysym.sym == SDLK_m) pageGo("mods"); } return; }   // N = Criar Novo Mundo, M = Mod List (telas extras)
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) return;
    if (gScreen == SC_TITLE && peUi()) {
        if (!gSkinHintSeen) { gSkinHintSeen = true; profileSave(); }   // qualquer clique dispensa o aviso da primeira vez
        MR b[7]; peTitleRects(sw, sh, b); for (int i = 0; i < 6; i++) if (inR(b[i], ux, uy)) { sndEvtUi("ui.click"); if (i == 0) openSingleplayer(); else if (i == 1) pageGo("multi"); else if (i == 2) pageGo("peset"); else if (i == 3) pageGo("mods"); else if (i == 4) pageGo("peskin");   // duas vezes no boneco abrem skins e opcoes
#ifndef __ANDROID__
            else if (i == 5) gMenuAct = 2;
#endif
            break; }
        return;
    }
    if (gScreen == SC_TITLE) {
        const bool small = sh < 240 || BL_SMALL_UI; const int g1 = small ? 22 : 24, g2 = small ? 44 : 48, g3 = small ? 70 : 84;   // igual ao drawTitle
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

// ---- patchNet.3: onde cada convidado parou (o host guarda; ao reentrar ele volta na mesma dimensao e posicao) ----
static std::map<std::string, std::array<double, 4>> gMpRes; static std::string gMpResDir;   // nick minusculo -> {dim, x, y, z}
static std::string mpResKey(const std::string& n) { std::string k; for (char c : n) k += (char)tolower((unsigned char)c); return k; }
static void mpResLoad() {
    if (gMpResDir == gCurDir) return; gMpRes.clear(); gMpResDir = gCurDir; if (gCurDir.empty()) return;
    FILE* f = fopen(("saves/" + gCurDir + "/mp_players.txt").c_str(), "rb"); if (!f) return;
    char nm[80]; double d, x, y, z; while (fscanf(f, "%79s %lf %lf %lf %lf", nm, &d, &x, &y, &z) == 5) gMpRes[nm] = {d, x, y, z}; fclose(f);
}
static void mpResRecord(const std::string& nick, int dim, double x, double y, double z) {
    if (gCurDir.empty() || nick.empty() || dim < 0 || dim > 2 || !std::isfinite(x + y + z)) return; mpResLoad();
    const std::array<double, 4> v = {(double)dim, x, y, z}; auto it = gMpRes.find(mpResKey(nick)); if (it != gMpRes.end() && it->second == v) return; gMpRes[mpResKey(nick)] = v;
    FILE* f = fopen(("saves/" + gCurDir + "/mp_players.txt").c_str(), "wb"); if (!f) return;
    for (auto& kv : gMpRes) fprintf(f, "%s %d %.3f %.3f %.3f\n", kv.first.c_str(), (int)kv.second[0], kv.second[1], kv.second[2], kv.second[3]); fclose(f);
}
static bool mpResGet(const std::string& nick, int& dim, double& x, double& y, double& z) {
    if (gCurDir.empty()) return false; mpResLoad(); auto it = gMpRes.find(mpResKey(nick)); if (it == gMpRes.end()) return false;
    dim = (int)it->second[0]; x = it->second[1]; y = it->second[2]; z = it->second[3]; return true;
}
