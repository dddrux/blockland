// ---------- Discord Rich Presence (patchDiscord.1): IPC puro (sem SDK), thread propria, nunca trava o jogo ----------
// Config em discord.cfg: client_id=<ID do app no Discord Developer Portal>  large_image=<chave do asset, padrao "blockland">
// A imagem (blockland.png) precisa ser enviada em Developer Portal > Rich Presence > Art Assets com o nome "blockland".
// Sem client_id (ou BLOCKLAND_DISCORD_ID) o recurso fica desligado. No Android nao e usado.
#if !defined(__ANDROID__)
#include <mutex>
#ifdef _WIN32
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#endif
namespace dsc {
static std::mutex mu; static std::string gDetails, gState; static bool gDirty = false, gStop = false, gOn = false;
static std::thread th; static std::string cid, img = "blockland"; static long long tStart = 0;
#ifdef _WIN32
static HANDLE h = INVALID_HANDLE_VALUE;
static bool conn() { for (int i = 0; i < 10; i++) { std::string p = "\\\\?\\pipe\\discord-ipc-" + std::to_string(i);
        h = CreateFileA(p.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr); if (h != INVALID_HANDLE_VALUE) return true; } return false; }
static void disc() { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); h = INVALID_HANDLE_VALUE; }
static bool wr(const void* b, size_t n) { DWORD w = 0; return WriteFile(h, b, (DWORD)n, &w, nullptr) && w == n; }
static bool rd(void* b, size_t n, int ms) { DWORD a = 0; for (int t = 0; t <= ms; t += 10) { if (!PeekNamedPipe(h, nullptr, 0, nullptr, &a, nullptr)) return false; if (a >= n) break; Sleep(10); }
    if (a < n) return false; DWORD r = 0; return ReadFile(h, b, (DWORD)n, &r, nullptr) && r == n; }
#else
static int fd = -1;
static bool conn() {
    std::vector<std::string> bases; for (const char* e : { "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP" }) if (const char* v = getenv(e)) bases.push_back(v); bases.push_back("/tmp");
    std::vector<std::string> dirs; for (auto& b : bases) { dirs.push_back(b + "/"); dirs.push_back(b + "/app/com.discordapp.Discord/"); dirs.push_back(b + "/snap.discord/"); dirs.push_back(b + "/.flatpak/com.discordapp.Discord/xdg-run/"); }
    for (auto& d : dirs) for (int i = 0; i < 10; i++) {
        int s = socket(AF_UNIX, SOCK_STREAM, 0); if (s < 0) return false; sockaddr_un a{}; a.sun_family = AF_UNIX; std::string p = d + "discord-ipc-" + std::to_string(i);
        if (p.size() >= sizeof(a.sun_path)) { close(s); continue; } strcpy(a.sun_path, p.c_str());
        if (connect(s, (sockaddr*)&a, sizeof a) == 0) { fd = s; return true; } close(s); }
    return false; }
static void disc() { if (fd >= 0) close(fd); fd = -1; }
static bool wr(const void* b, size_t n) { const char* p = (const char*)b; while (n) { ssize_t w = send(fd, p, n, MSG_NOSIGNAL); if (w <= 0) return false; p += w; n -= (size_t)w; } return true; }
static bool rd(void* b, size_t n, int ms) { char* p = (char*)b; while (n) { pollfd pf{ fd, POLLIN, 0 }; if (poll(&pf, 1, ms) <= 0) return false; ssize_t r = recv(fd, p, n, 0); if (r <= 0) return false; p += r; n -= (size_t)r; } return true; }
#endif
static bool frame(int op, const std::string& js) { int32_t hd[2] = { op, (int32_t)js.size() }; return wr(hd, 8) && wr(js.data(), js.size()); }
static bool drain(int ms) { int32_t hd[2]; if (!rd(hd, 8, ms)) return true; if (hd[1] < 0 || hd[1] > (1 << 20)) return false; std::string b((size_t)hd[1], 0); return hd[1] == 0 || rd(&b[0], b.size(), 500); }
static std::string esc(const std::string& s) { std::string o; for (unsigned char c : s) { if (c == '"' || c == '\\') { o += '\\'; o += (char)c; } else if (c < 32) o += ' '; else o += (char)c; } return o; }
static bool send(const std::string& d, const std::string& s) {
    static int n = 0; std::string a = "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"" + std::to_string(++n) + "\",\"args\":{\"pid\":" +
#ifdef _WIN32
        std::to_string((int)GetCurrentProcessId())
#else
        std::to_string((int)getpid())
#endif
        + ",\"activity\":{\"details\":\"" + esc(d) + "\",\"state\":\"" + esc(s) + "\",\"timestamps\":{\"start\":" + std::to_string(tStart) +
        "},\"assets\":{\"large_image\":\"" + esc(img) + "\",\"large_text\":\"Blockland " BLOCKLAND_VERSION "\"}}}}";
    return frame(1, a) && drain(300); }
static void run() {
    bool ok = false; std::string ld, ls; auto last = std::chrono::steady_clock::now() - std::chrono::seconds(60);
    while (true) {
        std::string d, s; bool dirty, stop; { std::lock_guard<std::mutex> g(mu); d = gDetails; s = gState; dirty = gDirty; gDirty = false; stop = gStop; }
        if (stop) break;
        auto now = std::chrono::steady_clock::now();
        if (!ok) { if (now - last > std::chrono::seconds(15)) { last = now;
                if (conn()) { if (frame(0, "{\"v\":1,\"client_id\":\"" + cid + "\"}") && drain(1500)) { ok = true; ld.clear(); } else disc(); } } }
        if (ok && (dirty || d != ld || s != ls) && !d.empty()) { if (send(d, s)) { ld = d; ls = s; } else { ok = false; disc(); } }
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
    if (ok) { frame(1, "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"bye\",\"args\":{\"pid\":0}}"); drain(100); } disc(); }
static void init() {
    if (const char* e = getenv("BLOCKLAND_DISCORD_ID")) cid = e;
    FILE* f = fopen("discord.cfg", "r"); if (f) { char ln[256]; while (fgets(ln, sizeof ln, f)) { std::string l = ln; while (!l.empty() && (l.back() == '\n' || l.back() == '\r' || l.back() == ' ')) l.pop_back();
            if (l.rfind("client_id=", 0) == 0 && cid.empty()) cid = l.substr(10); else if (l.rfind("large_image=", 0) == 0 && l.size() > 12) img = l.substr(12); } fclose(f); }
    for (char c : cid) if (c < '0' || c > '9') { cid.clear(); break; }
    if (cid.empty()) return; tStart = (long long)time(nullptr); gOn = true; th = std::thread(run); }
static void set(const std::string& d, const std::string& s) { if (!gOn) return; std::lock_guard<std::mutex> g(mu); if (d != gDetails || s != gState) { gDetails = d; gState = s; gDirty = true; } }
static void shutdown() { if (!gOn) return; { std::lock_guard<std::mutex> g(mu); gStop = true; } if (th.joinable()) th.join(); gOn = false; }
}
#else
namespace dsc { static void init() {} static void set(const std::string&, const std::string&) {} static void shutdown() {} }
#endif
