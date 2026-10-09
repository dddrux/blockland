// ---------- Monitor Touch / desktop: Chrome/Edge headless controlado por DevTools Protocol (CDP) ----------
// Incluido por monitor.inc DENTRO de namespace mod_monitor (so desktop). Sem dependencias externas:
//  (1) decodificador JPEG baseline (os quadros do screencast do Chrome chegam em JPEG -> viram textura E sao reenviados aos outros jogadores como estao);
//  (2) cliente WebSocket minimo + HTTP GET (loopback);  (3) processo do navegador;  (4) thread "monshot" (pagina viva, ~15 quadros/s).
// Ideia igual a do WebDisplays/MCEF (Chromium fora da tela + URL sincronizada); aqui o Chromium e o do sistema e a imagem viaja por UDP.
#ifdef _WIN32
typedef SOCKET mon_sock_t;
#define MON_SEND_FLAGS 0
static bool monWouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
#else
typedef int mon_sock_t;
#ifdef MSG_NOSIGNAL
#define MON_SEND_FLAGS MSG_NOSIGNAL
#else
#define MON_SEND_FLAGS 0
#endif
static bool monWouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR; }
#endif

// ======== JPEG baseline (huffman sequencial, 8 bits, 1 ou 3 componentes, qualquer subamostragem, restart) ========
struct JHuff { int cnt[17] = {}, minc[17] = {}, vptr[17] = {}; uint8_t sym[256] = {}; bool ok = false; };
struct JComp { int id = 0, h = 1, v = 1, tq = 0, td = 0, ta = 0, pw = 0, ph = 0, pred = 0; std::vector<uint8_t> pl; };
static const uint8_t JZZ[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
struct JRd {
    const uint8_t* p; const uint8_t* e; uint32_t buf = 0; int n = 0; bool mk = false;
    int bit() { if (!n) { if (mk || p >= e) { mk = true; return 0; } const uint8_t c = *p++; if (c == 0xFF) { if (p < e && *p == 0) p++; else { p--; mk = true; return 0; } } buf = c; n = 8; } n--; return (buf >> n) & 1; }
    int bits(int k) { int v = 0; while (k--) v = (v << 1) | bit(); return v; }
    int ext(int s) { const int v = bits(s); return v < (1 << (s - 1)) ? v - (1 << s) + 1 : v; }
    int dec(const JHuff& h) { int c = 0; for (int l = 1; l <= 16; l++) { c = (c << 1) | bit(); const int d = c - h.minc[l]; if (d >= 0 && d < h.cnt[l]) return h.sym[h.vptr[l] + d]; } return -1; }
    void restart() { n = 0; mk = false; while (p + 1 < e && !(p[0] == 0xFF && p[1] >= 0xD0 && p[1] <= 0xD7)) p++; if (p + 1 < e) p += 2; }
};
static void jIdct(const int* in, uint8_t* o, int stride) {
    static float C[8][8]; static bool init = false;
    if (!init) { for (int u = 0; u < 8; u++) for (int x = 0; x < 8; x++) C[u][x] = (u ? 1.f : 0.70710678f) * cosf((2 * x + 1) * u * 3.14159265f / 16.f) * .5f; init = true; }
    float t[64];
    for (int v = 0; v < 8; v++) for (int x = 0; x < 8; x++) { float s = 0; for (int u = 0; u < 8; u++) s += C[u][x] * in[v * 8 + u]; t[v * 8 + x] = s; }
    for (int x = 0; x < 8; x++) for (int y = 0; y < 8; y++) { float s = 0; for (int v = 0; v < 8; v++) s += C[v][y] * t[v * 8 + x]; const int r = (int)(s + 128.5f); o[y * stride + x] = (uint8_t)(r < 0 ? 0 : r > 255 ? 255 : r); }
}
static bool jpegDecode(const uint8_t* d, size_t n, std::vector<uint32_t>& out, int& W, int& H) {   // out: RGBA (R no byte baixo)
    if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return false;
    int qt[4][64] = {}; JHuff dc[4], ac[4]; JComp cs[3]; int nc = 0, ri = 0; W = H = 0; size_t i = 2; const uint8_t* ent = nullptr;
    while (i + 4 <= n && !ent) {
        if (d[i] != 0xFF) { i++; continue; } const int m = d[i + 1]; if (m == 0xFF) { i++; continue; }
        if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { i += 2; continue; } if (m == 0xD9) break;
        const size_t L = ((size_t)d[i + 2] << 8) | d[i + 3]; if (L < 2 || i + 2 + L > n) return false; const uint8_t* s = d + i + 4; const uint8_t* se = d + i + 2 + L;
        if (m == 0xDB) { while (s < se) { const int pq = s[0] >> 4, t = s[0] & 3; s++; for (int k = 0; k < 64; k++) { if (s + (pq ? 2 : 1) > se) return false; qt[t][JZZ[k]] = pq ? (s[0] << 8 | s[1]) : s[0]; s += pq ? 2 : 1; } } }
        else if (m == 0xC4) { while (s + 17 <= se) { JHuff& h = (s[0] >> 4) ? ac[s[0] & 3] : dc[s[0] & 3]; int tot = 0; for (int l = 1; l <= 16; l++) { h.cnt[l] = s[l]; tot += s[l]; } s += 17; if (tot > 256 || s + tot > se) return false;
                memcpy(h.sym, s, tot); s += tot; int code = 0, k = 0; for (int l = 1; l <= 16; l++) { h.vptr[l] = k; h.minc[l] = code; code += h.cnt[l]; k += h.cnt[l]; code <<= 1; } h.ok = true; } }
        else if (m == 0xC0 || m == 0xC1) { if (se - s < 6 || s[0] != 8) return false; H = s[1] << 8 | s[2]; W = s[3] << 8 | s[4]; nc = s[5]; if ((nc != 1 && nc != 3) || se - s < 6 + 3 * nc) return false;
            for (int c = 0; c < nc; c++) { cs[c].id = s[6 + 3 * c]; cs[c].h = s[7 + 3 * c] >> 4; cs[c].v = s[7 + 3 * c] & 15; cs[c].tq = s[8 + 3 * c] & 3; if (cs[c].h < 1 || cs[c].h > 4 || cs[c].v < 1 || cs[c].v > 4) return false; } }
        else if (m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) return false;   // progressivo / aritmetico / sem perdas: nao suportado
        else if (m == 0xDD) { if (se - s >= 2) ri = s[0] << 8 | s[1]; }
        else if (m == 0xDA) { if (!nc || se - s < 1 + 2 * nc || s[0] != nc) return false;
            for (int k = 0; k < nc; k++) { int f = -1; for (int c = 0; c < nc; c++) if (cs[c].id == s[1 + 2 * k]) f = c; if (f < 0) return false; cs[f].td = s[2 + 2 * k] >> 4 & 3; cs[f].ta = s[2 + 2 * k] & 3; }
            ent = se; }
        i += 2 + L;
    }
    if (!ent || W <= 0 || H <= 0 || W > 4096 || H > 4096) return false;
    int Hm = 1, Vm = 1; for (int c = 0; c < nc; c++) { Hm = std::max(Hm, cs[c].h); Vm = std::max(Vm, cs[c].v); }
    const int mx = (W + 8 * Hm - 1) / (8 * Hm), my = (H + 8 * Vm - 1) / (8 * Vm);
    for (int c = 0; c < nc; c++) { if (!dc[cs[c].td].ok || !ac[cs[c].ta].ok) return false; cs[c].pw = mx * cs[c].h * 8; cs[c].ph = my * cs[c].v * 8; cs[c].pl.assign((size_t)cs[c].pw * cs[c].ph, 0); }
    JRd r; r.p = ent; r.e = d + n; int blk[64], cnt = 0;
    for (int yy = 0; yy < my; yy++) for (int xx = 0; xx < mx; xx++, cnt++) {
        if (ri && cnt && cnt % ri == 0) { r.restart(); for (int c = 0; c < nc; c++) cs[c].pred = 0; }
        for (int c = 0; c < nc; c++) { JComp& C = cs[c];
            for (int by = 0; by < C.v; by++) for (int bx = 0; bx < C.h; bx++) {
                memset(blk, 0, sizeof blk); const int t = r.dec(dc[C.td]); if (t < 0 || t > 11) return false; C.pred += t ? r.ext(t) : 0; blk[0] = C.pred * qt[C.tq][0];
                for (int k = 1; k < 64;) { const int rs = r.dec(ac[C.ta]); if (rs < 0) return false; const int ss = rs & 15, run = rs >> 4;
                    if (!ss) { if (run == 15) { k += 16; continue; } break; } k += run; if (k > 63) break; blk[JZZ[k]] = r.ext(ss) * qt[C.tq][JZZ[k]]; k++; }
                jIdct(blk, &C.pl[(size_t)((yy * C.v + by) * 8) * C.pw + (xx * C.h + bx) * 8], C.pw); } }
    }
    out.resize((size_t)W * H);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        auto smp = [&](const JComp& c) { return (int)c.pl[(size_t)(y * c.v / Vm) * c.pw + (x * c.h / Hm)]; };
        int R, G, B; if (nc == 1) R = G = B = smp(cs[0]); else { const int Y = smp(cs[0]), cb = smp(cs[1]) - 128, cr = smp(cs[2]) - 128;
            R = Y + (int)(1.402f * cr); G = Y - (int)(.344136f * cb + .714136f * cr); B = Y + (int)(1.772f * cb); }
        R = R < 0 ? 0 : R > 255 ? 255 : R; G = G < 0 ? 0 : G > 255 ? 255 : G; B = B < 0 ? 0 : B > 255 ? 255 : B;
        out[(size_t)y * W + x] = 0xFF000000u | ((uint32_t)B << 16) | ((uint32_t)G << 8) | (uint32_t)R;
    }
    return true;
}

// ======== base64 / JSON minimo ========
static std::string b64dec(const std::string& s) {
    static int8_t T[256]; static bool init = false;
    if (!init) { memset(T, -1, sizeof T); const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; for (int i = 0; i < 64; i++) T[(uint8_t)A[i]] = (int8_t)i; init = true; }
    std::string o; o.reserve(s.size() * 3 / 4); uint32_t v = 0; int n = 0;
    for (unsigned char c : s) { if (T[c] < 0) continue; v = (v << 6) | (uint32_t)T[c]; n += 6; if (n >= 8) { n -= 8; o += (char)((v >> n) & 0xFF); } }
    return o;
}
static bool jsonStr(const std::string& m, const char* key, std::string& out, size_t from = 0) {   // primeira ocorrencia de "key": "valor"
    const std::string k = std::string("\"") + key + "\""; size_t p = m.find(k, from); if (p == std::string::npos) return false; p += k.size();
    while (p < m.size() && (m[p] == ' ' || m[p] == ':')) p++; if (p >= m.size() || m[p] != '"') return false; p++; size_t e = p;
    while (e < m.size() && m[e] != '"') { if (m[e] == '\\') e++; e++; } if (e >= m.size()) return false; out = m.substr(p, e - p); return true;
}
static int jsonInt(const std::string& m, const char* key) {
    const std::string k = std::string("\"") + key + "\""; size_t p = m.find(k); if (p == std::string::npos) return -1; p += k.size();
    while (p < m.size() && (m[p] == ' ' || m[p] == ':')) p++; return p < m.size() && isdigit((unsigned char)m[p]) ? atoi(m.c_str() + p) : -1;
}

static void mlog(const char* fmt, ...) {   // log do jogo (logs/latest.log), seguro entre threads
    char b[600]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a); if (gLock) SDL_LockMutex(gLock); logInfo("[Monitor] %s", b); if (gLock) SDL_UnlockMutex(gLock);
}
// ======== WebSocket (cliente, texto, loopback) + HTTP GET ========
struct Ws {
    int s = -1; std::string rb, cont; bool closed = false;
    static bool wait(int sk, int ms, bool wr) { fd_set f; FD_ZERO(&f); FD_SET((mon_sock_t)sk, &f); timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000; return select(sk + 1, wr ? nullptr : &f, wr ? &f : nullptr, nullptr, &tv) > 0; }
    bool open(int port, int ms) {
        netSockInit(); close(); closed = false; rb.clear(); cont.clear(); s = (int)socket(AF_INET, SOCK_STREAM, 0); if (s < 0) { s = -1; return false; }
        sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_port = htons((uint16_t)port); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        netSockNonBlock(s); connect((mon_sock_t)s, (sockaddr*)&a, sizeof a);
        if (!wait(s, ms, true)) { close(); return false; } int err = 0; socklen_t l = sizeof err; getsockopt((mon_sock_t)s, SOL_SOCKET, SO_ERROR, (char*)&err, &l); if (err) { close(); return false; } return true;
    }
    void close() { if (s >= 0) netSockClose(s); s = -1; closed = true; }
    bool sendAll(const std::string& d) {
        size_t o = 0; int bad = 0; while (o < d.size()) { const int k = (int)send((mon_sock_t)s, d.data() + o, (int)(d.size() - o), MON_SEND_FLAGS); if (k > 0) { o += (size_t)k; bad = 0; continue; } if (++bad > 100 || !wait(s, 2000, true)) { closed = true; return false; } } return true;
    }
    void readRaw(int ms, std::string& dst) {
        if (s < 0 || closed || !wait(s, ms, false)) return; char buf[65536]; const int k = (int)recv((mon_sock_t)s, buf, sizeof buf, 0);
        if (k > 0) dst.append(buf, (size_t)k); else if (k == 0 || !monWouldBlock()) closed = true;
    }
    bool upgrade(int port, const std::string& path) {
        if (!sendAll("GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port) + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n")) return false;
        const Uint32 t0 = SDL_GetTicks(); size_t e; while ((e = rb.find("\r\n\r\n")) == std::string::npos) { if (closed || SDL_GetTicks() - t0 > 4000) return false; readRaw(100, rb); }
        const bool ok = rb.compare(0, 12, "HTTP/1.1 101") == 0; rb.erase(0, e + 4); return ok;
    }
    bool sendFrame(int op, const std::string& p) {
        std::string f; f += (char)(0x80 | op); const size_t L = p.size(); uint8_t mk[4]; for (int i = 0; i < 4; i++) mk[i] = (uint8_t)(rand() & 255);
        if (L < 126) f += (char)(0x80 | L); else if (L < 65536) { f += (char)(0x80 | 126); f += (char)(L >> 8); f += (char)(L & 255); } else { f += (char)(0x80 | 127); for (int i = 7; i >= 0; i--) f += (char)((uint64_t)L >> (8 * i) & 255); }
        f.append((const char*)mk, 4); for (size_t i = 0; i < L; i++) f += (char)((uint8_t)p[i] ^ mk[i & 3]); return sendAll(f);
    }
    bool sendText(const std::string& t) { return sendFrame(1, t); }
    bool parse(std::string& out) {
        for (;;) {
            if (rb.size() < 2) return false; const uint8_t b0 = (uint8_t)rb[0], b1 = (uint8_t)rb[1]; uint64_t L = b1 & 0x7F; size_t h = 2;
            if (L == 126) { if (rb.size() < 4) return false; L = ((uint8_t)rb[2] << 8) | (uint8_t)rb[3]; h = 4; }
            else if (L == 127) { if (rb.size() < 10) return false; L = 0; for (int i = 2; i < 10; i++) L = (L << 8) | (uint8_t)rb[i]; h = 10; }
            const bool masked = (b1 & 0x80) != 0; if (masked) h += 4; if (L > (16u << 20)) { closed = true; return false; } if (rb.size() < h + L) return false;
            std::string p = rb.substr(h, (size_t)L); if (masked) for (size_t i = 0; i < p.size(); i++) p[i] = (char)((uint8_t)p[i] ^ (uint8_t)rb[h - 4 + (i & 3)]);
            rb.erase(0, h + (size_t)L); const int op = b0 & 15; const bool fin = (b0 & 0x80) != 0;
            if (op == 8) { closed = true; return false; } if (op == 9) { sendFrame(10, p); continue; } if (op == 10) continue;
            if (op == 1 || op == 2) cont = p; else if (op == 0) cont += p; else continue;
            if (fin) { out.swap(cont); cont.clear(); return true; }
        }
    }
    bool next(std::string& out, int ms) { if (parse(out)) return true; if (closed) return false; readRaw(ms, rb); return parse(out); }
};
static std::string httpGet(int port, const std::string& path) {
    Ws w; if (!w.open(port, 500)) return ""; w.sendAll("GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port) + "\r\nConnection: close\r\n\r\n");
    std::string r; const Uint32 t0 = SDL_GetTicks(); for (;;) { if (w.closed || SDL_GetTicks() - t0 > 3000) break; w.readRaw(100, r); const size_t e = r.find("\r\n\r\n"); if (e == std::string::npos) continue;
        const size_t cl = r.find("Content-Length: "); if (cl != std::string::npos && cl < e && r.size() >= e + 4 + (size_t)atoi(r.c_str() + cl + 16)) break; }   // nao espera o servidor fechar
    w.close(); const size_t e = r.find("\r\n\r\n"); return e == std::string::npos ? "" : r.substr(e + 4);
}

// ======== processo do navegador ========
#ifdef _WIN32
static HANDLE gProc = nullptr;
#else
static pid_t gPid = -1;
#endif
static std::string gProf;
static bool gNoSandbox = false; static std::string gBrowserLog, gUA; static int gMonProf = getenv("BLOCKLAND_MON_GPU") ? 3 : 0;
static bool launchBrowser(const std::string& exe, int port, int w, int h) {
    namespace fs = std::filesystem; std::error_code ec; const fs::path dir = fs::temp_directory_path(ec) / "blockland_mon"; fs::create_directories(dir, ec); gProf = (dir / ("profile_" + std::to_string(port))).string();
    std::string feats = "CalculateNativeWinOcclusion,Translate"; if (gMonProf == 2) feats += ",Vulkan,UseSkiaRenderer";
    std::vector<std::string> a = {exe, "--headless=new", "--remote-debugging-port=" + std::to_string(port), "--remote-allow-origins=*", "--user-data-dir=" + gProf, "--hide-scrollbars",
        "--autoplay-policy=no-user-gesture-required", "--no-first-run", "--no-default-browser-check", "--disable-extensions", "--disable-background-timer-throttling", "--disable-renderer-backgrounding",
        "--disable-backgrounding-occluded-windows", "--disable-ipc-flooding-protection", "--disable-features=" + feats, "--window-size=" + std::to_string(w) + "," + std::to_string(h)};
    if (getenv("BLOCKLAND_MON_MUTE")) a.push_back("--mute-audio");   // por padrao o som do video sai pelo audio do sistema
    // perfis de renderizacao (o jogo troca sozinho para o proximo se a pagina nao gerar quadros): 0 software, 1 SwiftShader, 2 GPU em processo unico, 3 GPU do sistema
    if (gMonProf == 0 || gMonProf == 2) a.push_back("--disable-gpu");
    if (gMonProf == 1) { a.push_back("--use-angle=swiftshader"); a.push_back("--enable-unsafe-swiftshader"); a.push_back("--disable-gpu-compositing"); }
    if (gMonProf == 2) a.push_back("--in-process-gpu");
    if (gMonProf == 3) { a.push_back("--ignore-gpu-blocklist"); a.push_back("--enable-gpu-rasterization"); }
    std::vector<std::string> a2 = a; if (gNoSandbox) a2.push_back("--no-sandbox"); a2.push_back("about:blank");
    gBrowserLog = gLogBase + "logs/monitor-browser.log";
#ifdef _WIN32
    std::string cl; for (auto& x : a2) cl += "\"" + x + "\" "; STARTUPINFOA si{}; si.cb = sizeof si; PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, &cl[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) return false; CloseHandle(pi.hThread); gProc = pi.hProcess; return true;
#else
    std::vector<char*> av; for (auto& x : a2) av.push_back((char*)x.c_str()); av.push_back(nullptr);
    const pid_t pid = fork(); if (pid < 0) return false;
    if (pid == 0) { setpgid(0, 0); int fd = open(gBrowserLog.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); if (fd < 0) fd = open("/dev/null", O_WRONLY); if (fd >= 0) { dup2(fd, 1); dup2(fd, 2); } execv(av[0], av.data()); _exit(127); }
    gPid = pid; return true;
#endif
}
static bool browserAlive() {
#ifdef _WIN32
    return gProc && WaitForSingleObject(gProc, 0) == WAIT_TIMEOUT;
#else
    int st = 0; return gPid > 0 && waitpid(gPid, &st, WNOHANG) == 0;
#endif
}
static void killBrowser() {
#ifdef _WIN32
    if (gProc) { TerminateProcess(gProc, 1); CloseHandle(gProc); gProc = nullptr; }
#else
    if (gPid > 0) { kill(-gPid, SIGKILL); waitpid(gPid, nullptr, 0); gPid = -1; }
#endif
    if (!gProf.empty()) { std::error_code ec; std::filesystem::remove_all(gProf, ec); gProf.clear(); }
}

// ======== entrada (mouse / teclado) -> comandos CDP, enviados pela thread da pagina ========
static std::deque<std::pair<std::string, std::string>> gCmdQ; static std::string gNavIn;   // gNavIn: URL atual da pagina (links clicados), aplicada pelo jogo em applyNav()
static void cmd(const char* m, const std::string& p, bool coalesce = false) {
    if (!gLock) return; SDL_LockMutex(gLock);
    if (coalesce && !gCmdQ.empty() && gCmdQ.back().first == m && gCmdQ.back().second.find("\"mouseMoved\"") != std::string::npos) gCmdQ.back().second = p; else if (gCmdQ.size() < 200) gCmdQ.push_back({m, p});
    SDL_UnlockMutex(gLock);
}
static std::string jsonEsc(const char* t) { std::string o; for (const char* p = t; *p; p++) { const unsigned char c = (unsigned char)*p; if (c == '"' || c == '\\') { o += '\\'; o += (char)c; } else if (c < 32) { char h[8]; snprintf(h, 8, "\\u%04x", c); o += h; } else o += (char)c; } return o; }
static std::string jsonUnesc(const std::string& s) {   // \/ \" \\ \uXXXX (ASCII) -> caractere
    std::string o; for (size_t i = 0; i < s.size(); i++) { if (s[i] != '\\' || i + 1 >= s.size()) { o += s[i]; continue; } const char c = s[++i];
        if (c == 'u' && i + 4 < s.size()) { const int v = (int)strtol(s.substr(i + 1, 4).c_str(), nullptr, 16); if (v < 128) o += (char)v; i += 4; } else o += c; } return o;
}
static int pxOf(float n, int sz) { return (int)(std::max(0.f, std::min(1.f, n)) * (sz - 1)); }
static std::string mouseJson(const char* type, float nx, float ny, int buttons, int clicks, int dy = 0) {
    return std::string("{\"type\":\"") + type + "\",\"x\":" + std::to_string(pxOf(nx, gWantW)) + ",\"y\":" + std::to_string(pxOf(ny, gWantH)) + ",\"button\":\"" + (buttons ? "left" : "none") + "\",\"buttons\":" + std::to_string(buttons) + ",\"clickCount\":" + std::to_string(clicks) + ",\"deltaX\":0,\"deltaY\":" + std::to_string(dy) + "}";
}
static void beTouch(int act, float nx, float ny) {   // 0 = descer, 1 = mover (botao apertado), 2 = soltar
    if (act == 0) cmd("Input.dispatchMouseEvent", mouseJson("mouseMoved", nx, ny, 0, 0), true);
    cmd("Input.dispatchMouseEvent", mouseJson(act == 0 ? "mousePressed" : act == 1 ? "mouseMoved" : "mouseReleased", nx, ny, act == 2 ? 0 : 1, act == 1 ? 0 : 1), act == 1);
}
static void beHover(float nx, float ny) { static Uint32 t = 0; const Uint32 n = SDL_GetTicks(); if (n - t < 30) return; t = n; cmd("Input.dispatchMouseEvent", mouseJson("mouseMoved", nx, ny, 0, 0), true); }
static void beWheel(float nx, float ny, int dy) { cmd("Input.dispatchMouseEvent", mouseJson("mouseWheel", nx, ny, 0, 0, dy)); }
static void keyPress(const std::string& key, const std::string& code, int vk, const std::string& text) {   // text ja escapado; vazio = tecla sem caractere
    const std::string b = "\"key\":\"" + key + "\",\"code\":\"" + code + "\",\"windowsVirtualKeyCode\":" + std::to_string(vk) + ",\"nativeVirtualKeyCode\":" + std::to_string(vk);
    cmd("Input.dispatchKeyEvent", std::string("{\"type\":\"") + (text.empty() ? "rawKeyDown" : "keyDown") + "\"," + b + (text.empty() ? "" : ",\"text\":\"" + text + "\",\"unmodifiedText\":\"" + text + "\"") + "}");
    cmd("Input.dispatchKeyEvent", "{\"type\":\"keyUp\"," + b + "}");
}
static void beText(const char* t) {   // SDL_TEXTINPUT: 1 caractere ASCII vira tecla de verdade (atalhos como k/f do YouTube); o resto (acentos, emoji, IME) vai como texto
    if (!t || !*t) return; if (t[1] == 0 && (unsigned char)t[0] > 32 && (unsigned char)t[0] < 127) { const char c = t[0]; const char u = (char)toupper((unsigned char)c);
        keyPress(jsonEsc(t), isalpha((unsigned char)c) ? std::string("Key") + u : isdigit((unsigned char)c) ? std::string("Digit") + c : std::string(), isalnum((unsigned char)c) ? (int)u : 0, jsonEsc(t)); return; }
    if (t[0] == ' ' && !t[1]) return;   // espaco: vem como tecla (beKey), senao digitaria em dobro
    cmd("Input.insertText", "{\"text\":\"" + jsonEsc(t) + "\"}");
}
static void beKey(int k) {   // codigos SDL: setas/Home/End/PgUp/PgDn = 0x40000000 | scancode
    struct K { int sdl; const char* key; const char* code; int vk; const char* text; };
    static const K T[] = {{8, "Backspace", "Backspace", 8, ""}, {9, "Tab", "Tab", 9, ""}, {13, "Enter", "Enter", 13, "\\r"}, {0x40000058, "Enter", "NumpadEnter", 13, "\\r"}, {32, " ", "Space", 32, " "}, {127, "Delete", "Delete", 46, ""},
        {0x40000050, "ArrowLeft", "ArrowLeft", 37, ""}, {0x40000052, "ArrowUp", "ArrowUp", 38, ""}, {0x4000004F, "ArrowRight", "ArrowRight", 39, ""}, {0x40000051, "ArrowDown", "ArrowDown", 40, ""},
        {0x4000004A, "Home", "Home", 36, ""}, {0x4000004D, "End", "End", 35, ""}, {0x4000004B, "PageUp", "PageUp", 33, ""}, {0x4000004E, "PageDown", "PageDown", 34, ""}};
    for (const K& e : T) if (e.sdl == k) { keyPress(e.key, e.code, e.vk, e.text); return; }
}
static void beScroll(int) {}
static void beBack() { cmd("Runtime.evaluate", "{\"expression\":\"history.back()\"}"); }

// ======== thread da pagina ========
static void setSize(int w, int h) { if (!gLock) return; SDL_LockMutex(gLock); if (w != gWantW || h != gWantH) { gWantW = w; gWantH = h; gSzSeq++; } SDL_UnlockMutex(gLock); }
static std::string rdLog() { std::string s; if (FILE* f = fopen(gBrowserLog.c_str(), "rb")) { char b[500]; size_t n = fread(b, 1, sizeof b - 1, f); fclose(f); b[n] = 0; for (size_t i = 0; i < n; i++) if (b[i] == '\n' || b[i] == '\r') b[i] = ' '; s = b; } return s; }
static bool startBrowser(Ws& ws, int W, int H) {   // abre o navegador e conecta no alvo "page" via WebSocket (diagnostico em logs/latest.log e logs/monitor-browser.log)
    static const std::string exe = findBrowser(); if (exe.empty()) { gStatus = "Instale Edge, Chrome ou Chromium para ver paginas (ou use 'Abrir no sistema')."; mlog("nenhum navegador encontrado (Edge/Chrome/Chromium/Brave); defina BLOCKLAND_BROWSER=/caminho/do/navegador"); return false; }
    for (int attempt = 0; attempt < 2 && !gQuit; attempt++) {
        killBrowser(); const int port = 40000 + (int)(((unsigned)time(nullptr) * 2654435761u + SDL_GetTicks() * 40503u) % 9000u); gStatus = "Iniciando navegador...";
        mlog("iniciando %s (porta %d, perfil %d%s)", exe.c_str(), port, gMonProf, gNoSandbox ? ", --no-sandbox" : ""); if (!launchBrowser(exe, port, W, H)) { gStatus = "Falha ao iniciar o navegador."; mlog("nao foi possivel iniciar o processo"); return false; }
        std::string path; bool died = false; for (Uint32 t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < 9000 && !gQuit && path.empty() && !died;) {
            const std::string j = httpGet(port, "/json/list"); size_t from = 0, p;
            while ((p = j.find("\"webSocketDebuggerUrl\"", from)) != std::string::npos) { std::string u; if (jsonStr(j, "webSocketDebuggerUrl", u, p) && u.find("/devtools/page/") != std::string::npos) { const size_t q = u.find('/', 5); if (q != std::string::npos) path = u.substr(q); break; } from = p + 10; }
            if (path.empty()) { if (!browserAlive()) died = true; else SDL_Delay(200); } }
        if (!path.empty() && ws.open(port, 1000) && ws.upgrade(port, path)) { mlog("conectado ao navegador (DevTools)");
            { const std::string v = httpGet(port, "/json/version"); std::string ua; gUA.clear(); if (jsonStr(v, "User-Agent", ua)) { ua = jsonUnesc(ua); const size_t q = ua.find("HeadlessChrome"); if (q != std::string::npos) ua.replace(q, 14, "Chrome"); gUA = ua; mlog("navegador: %s", ua.c_str()); } }   // mesmo Chrome, sem o \"Headless\" (YouTube/Google servem a pagina normal)
            return true; }
        std::string tail = rdLog(); mlog(died ? "o navegador encerrou sozinho. Saida: %s" : "sem resposta do DevTools em 9 s. Saida: %s", tail.empty() ? "(vazia)" : tail.c_str()); ws.close();
        if (died && !gNoSandbox) { gNoSandbox = true; mlog("tentando de novo com --no-sandbox"); continue; } break; }
    gStatus = "Nao foi possivel controlar o navegador (veja logs/latest.log)."; killBrowser(); return false;
}
struct DecJob { std::array<int, 3> k; std::string j; };
static std::deque<DecJob> gDecQ;
static int worker(void*) {
    Ws ws; int cid = 0; bool up = false, shooting = false, fallback = false, restart = false; int quality = 60, seenSeq = -1, seenSz = -1, fails = 0, shotId = -1, errLogs = 0; bool poll = false, shotPending = false, gotAny = false, firstLogged = false; int badLogged = 0, dbg = 0; Uint32 shootT = 0, lastPoll = 0, lastShare = 0; std::string latest; bool haveLatest = false; Uint32 lastTry = 0, lastShot = 0, lastDec = 0; gName = "Chrome headless (CDP)";
    auto call = [&](const char* m, const std::string& p) { return ws.sendText("{\"id\":" + std::to_string(++cid) + ",\"method\":\"" + m + "\",\"params\":" + p + "}"); };
    while (!gQuit) {
        for (;;) { DecJob j; SDL_LockMutex(gLock); const bool has = !gDecQ.empty(); if (has) { j = std::move(gDecQ.front()); gDecQ.pop_front(); } SDL_UnlockMutex(gLock); if (!has) break;   // quadros dos outros jogadores
            std::vector<uint32_t> px; int w = 0, h = 0; if (jpegDecode((const uint8_t*)j.j.data(), j.j.size(), px, w, h)) setPending(j.k, px.data(), w, h); }
        SDL_LockMutex(gLock); const std::string url = gWantUrl; const std::array<int, 3> key = gWantKey; const bool vis = gVis; const int seq = gSeq, szs = gSzSeq, W = gWantW, H = gWantH; SDL_UnlockMutex(gLock);
        if (url.empty() || !vis) { SDL_LockMutex(gLock); gCmdQ.clear(); SDL_UnlockMutex(gLock); if (up && shooting) { call("Page.stopScreencast", "{}"); shooting = false; call("Runtime.evaluate", "{\"expression\":\"document.querySelectorAll('video,audio').forEach(function(v){try{v.pause()}catch(e){}})\"}"); } if (up) { std::string m; while (ws.next(m, 0)) {} if (ws.closed) { up = false; killBrowser(); } } SDL_Delay(30); continue; }
        if (fallback) {   // sem CDP: captura simples a cada ~3 s (sem clique e sem enviar aos outros jogadores)
            if (seq != seenSeq || SDL_GetTicks() - lastShot > 3000) { seenSeq = seq; std::vector<uint32_t> px; int w = 0, h = 0; if (capture(url, px, w, h)) setPending(key, px.data(), w, h); lastShot = SDL_GetTicks(); } else SDL_Delay(100);
            gName = "Captura Edge/Chrome (simples)"; continue; }
        if (!up) {
            if (lastTry && SDL_GetTicks() - lastTry < 3000) { SDL_Delay(50); continue; } lastTry = SDL_GetTicks();
            if (startBrowser(ws, W, H)) { up = true; shooting = false; poll = false; shotPending = false; firstLogged = false; seenSeq = seenSz = -1; fails = 0; mlog("DevTools aberto; enviando Page.enable/navigate/startScreencast"); call("Page.enable", "{}"); call("Emulation.setFocusEmulationEnabled", "{\"enabled\":true}");
                call("Emulation.setUserAgentOverride", "{\"userAgent\":\"" + jsonEsc(gUA.empty() ? "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36" : gUA.c_str()) + "\"}"); call("Inspector.enable", "{}"); }
            else if (++fails >= 2 && !findBrowser().empty()) { fallback = true; mlog("DevTools indisponivel: usando captura simples a cada 3 s (sem clique e sem enviar aos outros jogadores)"); gStatus = "CDP indisponivel: captura simples (sem clique)."; }
            continue; }
        if (ws.closed) { up = false; killBrowser(); gStatus = "Navegador encerrado; reiniciando..."; continue; }
        if (szs != seenSz) { seenSz = szs; call("Emulation.setDeviceMetricsOverride", "{\"width\":" + std::to_string(W) + ",\"height\":" + std::to_string(H) + ",\"deviceScaleFactor\":1,\"mobile\":false}"); restart = true; }
        if (seq != seenSeq) { seenSeq = seq; call("Page.navigate", "{\"url\":\"" + safeUrl(url) + "\"}"); gStatus = "Carregando pagina..."; }
        if ((!shooting && !poll) || restart) { if (shooting) call("Page.stopScreencast", "{}");
            call("Page.startScreencast", "{\"format\":\"jpeg\",\"quality\":" + std::to_string(quality) + ",\"maxWidth\":" + std::to_string(W) + ",\"maxHeight\":" + std::to_string(H) + ",\"everyNthFrame\":1}"); shooting = true; restart = false; shootT = SDL_GetTicks(); gotAny = false; }
        for (;;) { std::pair<std::string, std::string> q; SDL_LockMutex(gLock); const bool has = !gCmdQ.empty(); if (has) { q = std::move(gCmdQ.front()); gCmdQ.pop_front(); } SDL_UnlockMutex(gLock); if (!has) break; call(q.first.c_str(), q.second); }   // entrada do jogador
        const Uint32 tnow = SDL_GetTicks();
        std::string m; bool first = true;
        auto process = [&](const std::string& b64) {   // quadro (base64 JPEG) -> textura local + outros jogadores
            const std::string jpg = b64dec(b64); const Uint32 now = SDL_GetTicks(); if (jpg.size() < 100) return; lastDec = now;
            std::vector<uint32_t> px; int w = 0, h = 0; if (!jpegDecode((const uint8_t*)jpg.data(), jpg.size(), px, w, h)) { if (badLogged++ < 3) { int sof = -1; for (size_t i = 0; i + 3 < jpg.size() && sof < 0; i++) if ((uint8_t)jpg[i] == 0xFF && (uint8_t)jpg[i + 1] >= 0xC0 && (uint8_t)jpg[i + 1] <= 0xCF && (uint8_t)jpg[i + 1] != 0xC4 && (uint8_t)jpg[i + 1] != 0xC8 && (uint8_t)jpg[i + 1] != 0xCC) sof = (uint8_t)jpg[i + 1];
                mlog("JPEG do navegador nao decodificou (%d bytes, SOF=%02X, inicio=%02X%02X%02X%02X)", (int)jpg.size(), sof, (uint8_t)jpg[0], (uint8_t)jpg[1], (uint8_t)jpg[2], (uint8_t)jpg[3]); } return; }
            gotAny = true;
            if (!firstLogged) { firstLogged = true; mlog("primeiro quadro recebido: %dx%d, %d bytes (%s)", w, h, (int)jpg.size(), poll ? "captura" : "screencast"); }
            setPending(key, px.data(), w, h); gStatus.clear();
            if (netOn && now - lastShare >= 66) { lastShare = now; if (jpg.size() <= 46000) queueJpeg(key, (const uint8_t*)jpg.data(), (int)jpg.size()); else if (quality > 20) { quality -= 10; restart = true; } }   // quadro grande demais p/ UDP: baixa a qualidade
        };
        auto deliver = [&](const std::string& b64) { latest = b64; haveLatest = true; };   // guarda so o quadro mais novo
        while (ws.next(m, first ? 30 : 0)) { first = false; if (dbg < 40 && !gotAny) { dbg++; mlog("CDP <- %.110s", m.c_str()); }
            if (m.size() < 400 && m.find("Inspector.targetCrashed") != std::string::npos) mlog("a aba do navegador travou (targetCrashed)");
            if (m.size() < 600 && (m.find("\"Page.frameNavigated\"") != std::string::npos || m.find("\"Page.navigatedWithinDocument\"") != std::string::npos) && m.find("\"parentId\"") == std::string::npos) {   // link clicado: a URL do monitor acompanha a pagina
                std::string u; if (jsonStr(m, "url", u)) { u = jsonUnesc(u); SDL_LockMutex(gLock); gNavIn = u; SDL_UnlockMutex(gLock); } continue; }
            if (m.find("\"Page.screencastFrame\"") != std::string::npos) { const int sid = jsonInt(m, "sessionId"); if (sid >= 0) call("Page.screencastFrameAck", "{\"sessionId\":" + std::to_string(sid) + "}");
                std::string b; if (jsonStr(m, "data", b)) deliver(b); continue; }
            if (m.compare(0, 6, "{\"id\":") == 0) {   // resposta a um comando
                const int id = jsonInt(m, "id"); const bool err = m.size() < 800 && m.find("\"error\"") != std::string::npos;
                if (shotPending && id == shotId) { shotPending = false; std::string b; if (!err && jsonStr(m, "data", b)) deliver(b); }
                if (err && errLogs++ < 8) mlog("erro do navegador: %s", m.substr(0, 300).c_str()); } }
        if (haveLatest) { const Uint32 nw = SDL_GetTicks(); if (!gotAny || nw - lastDec >= (gOpen ? 12u : 90u)) { process(latest); haveLatest = false; latest.clear(); } }   // pagina aberta: ate ~60 quadros/s; so olhando de longe: ~11/s
        if (!gotAny && !poll && shooting && tnow - shootT > 7000) { poll = true; mlog("o screencast nao enviou quadros; usando captura periodica (Page.captureScreenshot)"); call("Page.stopScreencast", "{}"); }
        if (poll && !gotAny && tnow - shootT > 16000 && gMonProf < 3) { gMonProf++; mlog("nenhum quadro via DevTools em 16 s; reiniciando o navegador com outro perfil de renderizacao (%d)", gMonProf); up = false; killBrowser(); lastTry = 0; gStatus = "Reiniciando navegador (outro modo de render)..."; continue; }
        if (poll && !gotAny && tnow - shootT > 16000) { fallback = true; up = false; killBrowser(); gStatus = "Controle do navegador sem imagem: captura simples."; mlog("nenhum quadro via DevTools em 16 s (todos os perfis); usando captura simples a cada 3 s (sem clique e sem enviar aos outros jogadores)"); continue; }
        if (poll && !shotPending && SDL_GetTicks() - lastPoll > 100) { lastPoll = SDL_GetTicks(); shotId = cid + 1; shotPending = true; call("Page.captureScreenshot", "{\"format\":\"jpeg\",\"quality\":" + std::to_string(quality) + "}"); }
        if (poll && shotPending && SDL_GetTicks() - lastPoll > 4000) shotPending = false;   // resposta perdida
    }
    ws.close(); killBrowser(); return 0;
}
static void ensureWorker() { if (!gThread) { gThread = true; SDL_DetachThread(SDL_CreateThread(worker, "monshot", nullptr)); } }
static void queueDecode(const std::array<int, 3>& k, const std::string& j) {   // thread do jogo: JPEG de outro jogador -> decodificado na thread da pagina
    if (!gLock) return; ensureWorker(); SDL_LockMutex(gLock); gDecQ.push_back({k, j}); while (gDecQ.size() > 2) gDecQ.pop_front(); SDL_UnlockMutex(gLock);
}
