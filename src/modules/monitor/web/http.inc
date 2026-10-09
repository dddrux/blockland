// ---------- PIE Web: cliente HTTP(S) multiplataforma ----------
// Incluido por web/web.inc DENTRO de namespace mod_monitor (so desktop). Sem biblioteca para LINKAR: a criptografia (TLS) vem do SISTEMA.
//   Windows : WinHTTP (winhttp.dll, carregada em tempo de execucao)    Linux/macOS : libcurl (carregada com SDL_LoadObject)
//   Reserva : HTTP/1.1 por sockets proprios (so http://, sem TLS) quando a libcurl nao existe.
// Cookies, redirecionamentos e descompressao (gzip/deflate/br) ficam por conta do sistema. Roda so na thread de rede.
namespace pw {
struct HttpReq { std::string url, method = "GET", body, ctype, referer; size_t maxBytes = 6u << 20; int timeoutS = 25; int lane = 0; };   // lane: 0 paginas/CSS (com cookies), 1 imagens (outra thread, outro handle)
struct HttpRes { int status = 0; std::string url, ctype, body, err; };
static std::string gUA = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36 BlocklandWeb/1.0";

// ---- reserva: http:// por sockets (netSock* vem de net.inc) ----
struct UrlParts { std::string scheme, host, path; int port = 80; };
static bool urlSplit(const std::string& u, UrlParts& p) {
    const size_t a = u.find("://"); if (a == std::string::npos) return false; p.scheme = u.substr(0, a); size_t h = a + 3, e = u.find_first_of("/?#", h); std::string hp = u.substr(h, e == std::string::npos ? std::string::npos : e - h);
    p.path = e == std::string::npos ? "/" : u.substr(e); if (p.path[0] != '/') p.path = "/" + p.path; const size_t hs = p.path.find('#'); if (hs != std::string::npos) p.path.resize(hs);
    p.port = p.scheme == "https" ? 443 : 80; const size_t c = hp.rfind(':'); if (c != std::string::npos && hp.find(']') == std::string::npos) { p.port = atoi(hp.c_str() + c + 1); hp.resize(c); } p.host = hp; return !p.host.empty();
}
static bool rawHttp(const HttpReq& rq, HttpRes& rs, int depth = 0) {
    UrlParts u; if (!urlSplit(rq.url, u)) { rs.err = "URL invalida"; return false; } if (u.scheme != "http") { rs.err = "HTTPS precisa da libcurl (Linux/macOS) ou WinHTTP (Windows)"; return false; }
    netSockInit(); addrinfo hints; memset(&hints, 0, sizeof hints); hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM; addrinfo* ai = nullptr;
    if (getaddrinfo(u.host.c_str(), std::to_string(u.port).c_str(), &hints, &ai) != 0 || !ai) { rs.err = "nao achei o servidor " + u.host; return false; }
    mon_sock_t s = (mon_sock_t)-1; for (addrinfo* a = ai; a; a = a->ai_next) { s = (mon_sock_t)socket(a->ai_family, a->ai_socktype, a->ai_protocol); if ((int)s < 0) continue; if (connect(s, a->ai_addr, (int)a->ai_addrlen) == 0) break; netSockClose((int)s); s = (mon_sock_t)-1; }
    freeaddrinfo(ai); if ((int)s < 0) { rs.err = "conexao recusada"; return false; }
#ifdef _WIN32
    DWORD tv = rq.timeoutS * 1000; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#else
    timeval tv; tv.tv_sec = rq.timeoutS; tv.tv_usec = 0; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#endif
    std::string req = rq.method + " " + u.path + " HTTP/1.1\r\nHost: " + u.host + (u.port != 80 ? ":" + std::to_string(u.port) : "") + "\r\nUser-Agent: " + gUA + "\r\nAccept: */*\r\nAccept-Encoding: identity\r\nConnection: close\r\n";
    if (!rq.referer.empty()) req += "Referer: " + rq.referer + "\r\n"; if (rq.method == "POST") req += "Content-Type: " + (rq.ctype.empty() ? std::string("application/x-www-form-urlencoded") : rq.ctype) + "\r\nContent-Length: " + std::to_string(rq.body.size()) + "\r\n";
    req += "\r\n"; if (rq.method == "POST") req += rq.body;
    size_t o = 0; while (o < req.size()) { const int k = (int)send(s, req.data() + o, (int)(req.size() - o), MON_SEND_FLAGS); if (k <= 0) break; o += (size_t)k; }
    std::string raw; char buf[16384]; for (;;) { const int k = (int)recv(s, buf, sizeof buf, 0); if (k <= 0) break; raw.append(buf, (size_t)k); if (raw.size() > rq.maxBytes + 65536) break; } netSockClose((int)s);
    const size_t he = raw.find("\r\n\r\n"); if (he == std::string::npos || raw.compare(0, 5, "HTTP/") != 0) { rs.err = "resposta invalida"; return false; }
    rs.status = atoi(raw.c_str() + raw.find(' ') + 1); std::string head = raw.substr(0, he), body = raw.substr(he + 4); std::string low = head; for (char& c : low) c = (char)tolower((unsigned char)c);
    auto hdr = [&](const char* n) { const size_t p = low.find(std::string("\r\n") + n + ":"); if (p == std::string::npos) return std::string(); size_t b = p + 3 + strlen(n), e = head.find("\r\n", b); std::string v = head.substr(b, e == std::string::npos ? std::string::npos : e - b); while (!v.empty() && v[0] == ' ') v.erase(0, 1); return v; };
    if ((rs.status == 301 || rs.status == 302 || rs.status == 303 || rs.status == 307 || rs.status == 308) && depth < 8) { std::string loc = hdr("location"); if (!loc.empty()) { HttpReq r2 = rq; if (loc.find("://") == std::string::npos) loc = "http://" + u.host + (loc[0] == '/' ? "" : "/") + loc; r2.url = loc; if (rs.status != 307 && rs.status != 308) { r2.method = "GET"; r2.body.clear(); } return rawHttp(r2, rs, depth + 1); } }
    if (!hdr("transfer-encoding").empty() && hdr("transfer-encoding").find("chunked") != std::string::npos) { std::string d; size_t p = 0; while (p < body.size()) { const size_t e = body.find("\r\n", p); if (e == std::string::npos) break; const long n = strtol(body.c_str() + p, nullptr, 16); if (n <= 0) break; d.append(body, e + 2, (size_t)n); p = e + 2 + (size_t)n + 2; } body.swap(d); }
    rs.ctype = hdr("content-type"); rs.url = rq.url; rs.body.swap(body); if (rs.body.size() > rq.maxBytes) rs.body.resize(rq.maxBytes); return true;
}

#ifdef _WIN32
// ---- Windows: WinHTTP ----
typedef void* WH; typedef WH(WINAPI* fnOpen)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD); typedef WH(WINAPI* fnConnect)(WH, LPCWSTR, WORD, DWORD); typedef WH(WINAPI* fnOpenReq)(WH, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
typedef BOOL(WINAPI* fnSend)(WH, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR); typedef BOOL(WINAPI* fnRecv)(WH, LPVOID); typedef BOOL(WINAPI* fnQH)(WH, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
typedef BOOL(WINAPI* fnRead)(WH, LPVOID, DWORD, LPDWORD); typedef BOOL(WINAPI* fnClose)(WH); typedef BOOL(WINAPI* fnSetOpt)(WH, DWORD, LPVOID, DWORD); typedef BOOL(WINAPI* fnQOpt)(WH, DWORD, LPVOID, LPDWORD); typedef BOOL(WINAPI* fnTimeouts)(WH, int, int, int, int);
struct WinHttpApi { HMODULE m = nullptr; fnOpen Open; fnConnect Connect; fnOpenReq OpenReq; fnSend Send; fnRecv Recv; fnQH QH; fnRead Read; fnClose Close; fnSetOpt SetOpt; fnQOpt QOpt; fnTimeouts Timeouts; WH ses = nullptr; bool ok = false; };
static std::wstring wide(const std::string& s) { if (s.empty()) return L""; const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0); std::wstring w((size_t)n, L'\0'); MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n); return w; }
static std::string narrow(const std::wstring& w) { if (w.empty()) return ""; const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr); std::string s((size_t)n, '\0'); WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr); return s; }
static bool sysFetch(const HttpReq& rq, HttpRes& rs) {
    static WinHttpApi A; static bool tried = false;
    if (!tried) { tried = true; A.m = LoadLibraryA("winhttp.dll"); if (A.m) {
#define WHF(member, type, name) A.member = (type)(void*)GetProcAddress(A.m, name)
        WHF(Open, fnOpen, "WinHttpOpen"); WHF(Connect, fnConnect, "WinHttpConnect"); WHF(OpenReq, fnOpenReq, "WinHttpOpenRequest"); WHF(Send, fnSend, "WinHttpSendRequest"); WHF(Recv, fnRecv, "WinHttpReceiveResponse");
        WHF(QH, fnQH, "WinHttpQueryHeaders"); WHF(Read, fnRead, "WinHttpReadData"); WHF(Close, fnClose, "WinHttpCloseHandle"); WHF(SetOpt, fnSetOpt, "WinHttpSetOption"); WHF(QOpt, fnQOpt, "WinHttpQueryOption");
        A.Timeouts = (fnTimeouts)(void*)GetProcAddress(A.m, "WinHttpSetTimeouts");
        if (A.Open && A.Connect && A.OpenReq && A.Send && A.Recv && A.QH && A.Read && A.Close && A.SetOpt && A.QOpt) { A.ses = A.Open(wide(gUA).c_str(), 0 /*DEFAULT_PROXY*/, nullptr, nullptr, 0);
            if (A.ses) { A.ok = true; DWORD dec = 0x3; A.SetOpt(A.ses, 118 /*DECOMPRESSION*/, &dec, sizeof dec); DWORD prot = 0x00000A80 | 0x00002000; A.SetOpt(A.ses, 84 /*SECURE_PROTOCOLS*/, &prot, sizeof prot); } } } }   // TLS 1.0-1.3 conforme o SO suportar
    if (!A.ok) return false;
    UrlParts u; if (!urlSplit(rq.url, u)) { rs.err = "URL invalida"; return true; } const bool https = u.scheme == "https";
    if (A.Timeouts) A.Timeouts(A.ses, 10000, 10000, 15000, rq.timeoutS * 1000);
    WH con = A.Connect(A.ses, wide(u.host).c_str(), (WORD)u.port, 0); if (!con) { rs.err = "nao achei o servidor " + u.host; return true; }
    WH req = A.OpenReq(con, wide(rq.method).c_str(), wide(u.path).c_str(), nullptr, nullptr, nullptr, https ? 0x00800000 /*SECURE*/ : 0); if (!req) { A.Close(con); rs.err = "falha ao abrir a requisicao"; return true; }
    std::wstring hd = L"Accept: text/html,application/xhtml+xml,*/*;q=0.8\r\nAccept-Language: pt-BR,pt;q=0.9,en;q=0.7\r\n"; if (!rq.referer.empty()) hd += L"Referer: " + wide(rq.referer) + L"\r\n"; if (rq.method == "POST") hd += L"Content-Type: " + wide(rq.ctype.empty() ? "application/x-www-form-urlencoded" : rq.ctype) + L"\r\n";
    const BOOL sent = A.Send(req, hd.c_str(), (DWORD)-1L, rq.body.empty() ? nullptr : (LPVOID)rq.body.data(), (DWORD)rq.body.size(), (DWORD)rq.body.size(), 0);
    if (!sent || !A.Recv(req, nullptr)) { rs.err = "falha na conexao (erro " + std::to_string((int)GetLastError()) + ")"; A.Close(req); A.Close(con); return true; }
    DWORD st = 0, sl = sizeof st; A.QH(req, 19 /*STATUS_CODE*/ | 0x20000000 /*NUMBER*/, nullptr, &st, &sl, nullptr); rs.status = (int)st;
    { wchar_t ct[256]; DWORD cl = sizeof ct; if (A.QH(req, 1 /*CONTENT_TYPE*/, nullptr, ct, &cl, nullptr)) rs.ctype = narrow(ct); }
    { wchar_t ub[2100]; DWORD ul = sizeof ub; rs.url = A.QOpt(req, 34 /*URL*/, ub, &ul) ? narrow(ub) : rq.url; }
    char buf[16384]; DWORD got; while (A.Read(req, buf, sizeof buf, &got) && got > 0) { rs.body.append(buf, got); if (rs.body.size() >= rq.maxBytes) break; }
    A.Close(req); A.Close(con); return true;
}
#else
// ---- Linux / macOS: libcurl carregada em tempo de execucao ----
typedef void CURL; struct curl_slist;
typedef size_t (*CurlWrite)(char*, size_t, size_t, void*);
struct CurlApi { void* lib = nullptr; int (*ginit)(long); CURL* (*init)(); int (*setopt)(CURL*, int, ...); int (*perform)(CURL*); int (*getinfo)(CURL*, int, ...); void (*cleanup)(CURL*); void (*reset)(CURL*); curl_slist* (*sl_append)(curl_slist*, const char*); void (*sl_free)(curl_slist*); const char* (*strerr)(int); CURL* h[2] = {nullptr, nullptr}; bool ok = false; };
static size_t curlWr(char* p, size_t s, size_t n, void* ud) { std::string* o = (std::string*)ud; const size_t k = s * n; if (o->size() + k > (24u << 20)) return 0; o->append(p, k); return k; }
static bool sysFetch(const HttpReq& rq, HttpRes& rs) {
    static CurlApi A; static bool tried = false;
    if (!tried) { tried = true; for (const char* n : {"libcurl.so.4", "libcurl.so", "libcurl.4.dylib", "libcurl.dylib", "libcurl-gnutls.so.4", "libcurl-gnutls.so.3", "libcurl-nss.so.4"}) { A.lib = SDL_LoadObject(n); if (A.lib) break; }
        if (A.lib) { A.ginit = (int (*)(long))SDL_LoadFunction(A.lib, "curl_global_init"); A.init = (CURL * (*)())SDL_LoadFunction(A.lib, "curl_easy_init"); A.setopt = (int (*)(CURL*, int, ...))SDL_LoadFunction(A.lib, "curl_easy_setopt");
            A.perform = (int (*)(CURL*))SDL_LoadFunction(A.lib, "curl_easy_perform"); A.getinfo = (int (*)(CURL*, int, ...))SDL_LoadFunction(A.lib, "curl_easy_getinfo"); A.cleanup = (void (*)(CURL*))SDL_LoadFunction(A.lib, "curl_easy_cleanup");
            A.reset = (void (*)(CURL*))SDL_LoadFunction(A.lib, "curl_easy_reset"); A.sl_append = (curl_slist * (*)(curl_slist*, const char*))SDL_LoadFunction(A.lib, "curl_slist_append"); A.sl_free = (void (*)(curl_slist*))SDL_LoadFunction(A.lib, "curl_slist_free_all"); A.strerr = (const char* (*)(int))SDL_LoadFunction(A.lib, "curl_easy_strerror");
            if (A.init && A.setopt && A.perform && A.getinfo && A.reset && A.sl_append && A.sl_free) { if (A.ginit) A.ginit(3); A.h[0] = A.init(); A.h[1] = A.init(); if (A.h[0] && A.h[1]) A.ok = true; } } }
    if (!A.ok) return false;
    enum { OPT_WRITEDATA = 10001, OPT_URL = 10002, OPT_POSTFIELDS = 10015, OPT_REFERER = 10016, OPT_USERAGENT = 10018, OPT_HTTPHEADER = 10023, OPT_COOKIEFILE = 10031, OPT_POST = 47, OPT_FOLLOW = 52, OPT_POSTFIELDSIZE = 60, OPT_AUTOREFERER = 58, OPT_MAXREDIRS = 68,
        OPT_TIMEOUT = 13, OPT_CONNTIMEOUT = 78, OPT_ENCODING = 10102, OPT_NOSIGNAL = 99, OPT_WRITEFUNC = 20011, OPT_PROTOCOLS = 181, OPT_REDIR_PROTOCOLS = 182, OPT_NOBODY = 44, INFO_URL = 0x100001, INFO_CTYPE = 0x100012, INFO_CODE = 0x200002 };
    CURL* h = A.h[rq.lane ? 1 : 0]; A.reset(h);   // reset mantem cookies e conexoes do handle
 std::string body; curl_slist* hl = A.sl_append(nullptr, "Accept: text/html,application/xhtml+xml,image/*,*/*;q=0.8"); hl = A.sl_append(hl, "Accept-Language: pt-BR,pt;q=0.9,en;q=0.7"); if (rq.method == "POST") hl = A.sl_append(hl, ("Content-Type: " + (rq.ctype.empty() ? std::string("application/x-www-form-urlencoded") : rq.ctype)).c_str());
    A.setopt(h, OPT_URL, rq.url.c_str()); A.setopt(h, OPT_WRITEFUNC, (CurlWrite)curlWr); A.setopt(h, OPT_WRITEDATA, &body); A.setopt(h, OPT_FOLLOW, 1L); A.setopt(h, OPT_MAXREDIRS, 10L); A.setopt(h, OPT_AUTOREFERER, 1L); A.setopt(h, OPT_USERAGENT, gUA.c_str());
    A.setopt(h, OPT_TIMEOUT, (long)rq.timeoutS); A.setopt(h, OPT_CONNTIMEOUT, 12L); A.setopt(h, OPT_ENCODING, ""); A.setopt(h, OPT_NOSIGNAL, 1L); A.setopt(h, OPT_PROTOCOLS, 3L); A.setopt(h, OPT_REDIR_PROTOCOLS, 3L); A.setopt(h, OPT_HTTPHEADER, hl);
    A.setopt(h, OPT_COOKIEFILE, "");   // liga o cookie jar em memoria
    if (!rq.referer.empty()) A.setopt(h, OPT_REFERER, rq.referer.c_str());
    if (rq.method == "POST") { A.setopt(h, OPT_POST, 1L); A.setopt(h, OPT_POSTFIELDSIZE, (long)rq.body.size()); A.setopt(h, OPT_POSTFIELDS, rq.body.c_str()); }
    const int rc = A.perform(h); A.sl_free(hl);
    if (rc != 0 && !(rc == 23 && body.size() >= (24u << 20))) { rs.err = A.strerr ? A.strerr(rc) : "erro de rede " + std::to_string(rc); if (body.empty()) return true; }   // 23 = corpo grande demais (cortado)
    long code = 0; char* eu = nullptr; char* ct = nullptr; A.getinfo(h, INFO_CODE, &code); A.getinfo(h, INFO_URL, &eu); A.getinfo(h, INFO_CTYPE, &ct);
    rs.status = (int)code; rs.url = eu ? eu : rq.url; rs.ctype = ct ? ct : ""; if (body.size() > rq.maxBytes) body.resize(rq.maxBytes); rs.body.swap(body); if (rs.status) rs.err.clear(); return true;
}
#endif
static void sysWarm() { HttpReq r; r.url = "about:blank"; HttpRes x; r.timeoutS = 1; sysFetch(r, x); }   // carrega libcurl/WinHTTP ANTES de criar as threads de rede
static bool httpFetch(const HttpReq& rq, HttpRes& rs) {   // tenta o sistema; sem ele, sockets puros (so http)
    rs = HttpRes(); rs.url = rq.url; if (sysFetch(rq, rs)) return rs.status > 0; return rawHttp(rq, rs) && rs.status > 0;
}
}   // namespace pw
