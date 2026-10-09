// ---------- LAN / ZeroTier: interfaces, descoberta de servidores, lista salva, hospedar e entrar ----------
// Descoberta (UDP, porta 47778): o HOST responde "BLQ" com "BLA" (mundo, jogadores, modo). O cliente consulta, com a aba Multiplayer aberta:
//  (1) broadcast global + broadcast dirigido de CADA interface, (2) varredura unicast da sub-rede (ate /22) de cada interface e (3) os servidores salvos.
// A varredura unicast acha o host mesmo onde broadcast nao passa (ZeroTier sem broadcast, Wi-Fi com isolamento). Interface "zt*" (Linux) ou
// adaptador "ZeroTier" (Windows) e marcada como ZeroTier. O jogo em si usa a porta UDP 47777 (net.inc) com o mesmo protocolo em todas as plataformas.
#ifdef _WIN32
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#endif
#include <set>
#ifdef _WIN32
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#endif

static void netSockQuirks(int s) {   // Windows: o aviso ICMP "porta fechada" derrubava o recvfrom do UDP (WSAECONNRESET) e atrasava respostas validas
#ifdef _WIN32
    BOOL off = FALSE; DWORD ret = 0; WSAIoctl((SOCKET)s, SIO_UDP_CONNRESET, &off, sizeof off, nullptr, 0, &ret, nullptr, nullptr);
#else
    (void)s;
#endif
}
struct NIf { uint32_t ip = 0, mask = 0; bool zt = false, lo = false; std::string name; };
static std::string ipStr(uint32_t h) { char b[24]; snprintf(b, sizeof b, "%u.%u.%u.%u", (unsigned)(h >> 24), (unsigned)((h >> 16) & 255), (unsigned)((h >> 8) & 255), (unsigned)(h & 255)); return b; }
static bool ipParse(const std::string& s, uint32_t& h) { in_addr a; if (inet_pton(AF_INET, s.c_str(), &a) != 1) return false; h = ntohl(a.s_addr); return true; }
static std::map<std::string, uint32_t> gDns;   // cache de nomes (0 = nao resolveu): evita travar o menu a cada varredura
static bool netResolve(const std::string& host, uint32_t& h) {
    if (ipParse(host, h)) return true;
    auto it = gDns.find(host); if (it != gDns.end()) { h = it->second; return h != 0; }
    netSockInit(); addrinfo hints{}, *res = nullptr; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM; h = 0;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) == 0 && res) { h = ntohl(((sockaddr_in*)res->ai_addr)->sin_addr.s_addr); freeaddrinfo(res); }
    gDns[host] = h; return h != 0;
}
static uint32_t netPrimaryIp() {   // IP da rota padrao (nao envia nada): reserva quando a lista de interfaces falha (ex.: Android sem getifaddrs)
    netSockInit(); uint32_t r = 0;
    for (const char* t : {"8.8.8.8", "10.255.255.255", "192.168.255.255"}) {
        const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); if (s < 0) continue;
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(9); inet_pton(AF_INET, t, &a.sin_addr);
        if (connect(s, (sockaddr*)&a, sizeof a) == 0) { sockaddr_in me{}; socklen_t l = sizeof me; if (getsockname(s, (sockaddr*)&me, &l) == 0) r = ntohl(me.sin_addr.s_addr); }
        netSockClose(s); if (r) break;
    }
    return r;
}
// ZeroTier: Linux "zt*"; Android (VpnService do app ZeroTier One) "tun*"; sem nome (API restrita), cai nos blocos que o ZeroTier Central sorteia por padrao.
static void netClassify(NIf& n) {
    if (n.name.compare(0, 2, "zt") == 0) n.zt = true;
#ifdef __ANDROID__
    if (n.name.compare(0, 3, "tun") == 0) n.zt = true;
#endif
    if (n.name.empty() && !n.lo) {
        const unsigned a = n.ip >> 24, b = (n.ip >> 16) & 255;
        if ((a == 10 && (b == 147 || (b >= 241 && b <= 244))) || (a == 172 && b >= 22 && b <= 30) || (a == 192 && (n.ip >> 8 & 255) >= 191 && (n.ip >> 8 & 255) <= 196 && b == 168)) n.zt = true;
    }
}
#ifndef _WIN32
static void netIfacesIoctl(std::vector<NIf>& v) {   // reserva quando getifaddrs falha (Android recente): SIOCGIFCONF nao depende de netlink
    const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); if (s < 0) return;
    ifreq rq[32]; ifconf ic{}; ic.ifc_len = sizeof rq; ic.ifc_req = rq;
    if (ioctl(s, SIOCGIFCONF, &ic) == 0) for (int i = 0; i < (int)(ic.ifc_len / sizeof(ifreq)); i++) {
        ifreq& r = rq[i]; if (r.ifr_addr.sa_family != AF_INET) continue;
        NIf n; n.ip = ntohl(((sockaddr_in*)&r.ifr_addr)->sin_addr.s_addr); n.name = r.ifr_name;
        ifreq q = r; if (ioctl(s, SIOCGIFFLAGS, &q) == 0) { if (!(q.ifr_flags & IFF_UP)) continue; n.lo = (q.ifr_flags & IFF_LOOPBACK) != 0; }
        q = r; n.mask = ioctl(s, SIOCGIFNETMASK, &q) == 0 ? ntohl(((sockaddr_in*)&q.ifr_addr)->sin_addr.s_addr) : 0xFFFFFF00u;
        netClassify(n); v.push_back(n);
    }
    close(s);
}
#endif
static std::vector<NIf> netIfaces() {
    std::vector<NIf> v; netSockInit();
#ifdef _WIN32
    std::vector<uint32_t> zt;   // IPs dos adaptadores ZeroTier (iphlpapi carregada na hora: sem -liphlpapi)
    typedef ULONG(WINAPI * GAA)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
    if (HMODULE h = LoadLibraryA("iphlpapi.dll")) {
        if (GAA f = (GAA)GetProcAddress(h, "GetAdaptersAddresses")) {
            ULONG len = 16000; std::vector<char> buf(len);
            ULONG r = f(AF_INET, 0, nullptr, (PIP_ADAPTER_ADDRESSES)buf.data(), &len);
            if (r == ERROR_BUFFER_OVERFLOW) { buf.resize(len); r = f(AF_INET, 0, nullptr, (PIP_ADAPTER_ADDRESSES)buf.data(), &len); }
            if (r == NO_ERROR) for (PIP_ADAPTER_ADDRESSES a = (PIP_ADAPTER_ADDRESSES)buf.data(); a; a = a->Next) {
                const bool isZt = (a->Description && wcsstr(a->Description, L"ZeroTier")) || (a->FriendlyName && wcsstr(a->FriendlyName, L"ZeroTier"));
                if (isZt) for (PIP_ADAPTER_UNICAST_ADDRESS u = a->FirstUnicastAddress; u; u = u->Next)
                    if (u->Address.lpSockaddr && u->Address.lpSockaddr->sa_family == AF_INET) zt.push_back(ntohl(((sockaddr_in*)u->Address.lpSockaddr)->sin_addr.s_addr));
            }
        }
        FreeLibrary(h);
    }
    const SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != INVALID_SOCKET) {
        INTERFACE_INFO info[32]; DWORD got = 0;
        if (WSAIoctl(s, SIO_GET_INTERFACE_LIST, nullptr, 0, info, sizeof info, &got, nullptr, nullptr) == 0)
            for (DWORD i = 0; i < got / sizeof(INTERFACE_INFO); i++) {
                if (!(info[i].iiFlags & IFF_UP) || info[i].iiAddress.Address.sa_family != AF_INET) continue;
                NIf n; n.ip = ntohl(info[i].iiAddress.AddressIn.sin_addr.s_addr); n.mask = ntohl(info[i].iiNetmask.AddressIn.sin_addr.s_addr);
                n.lo = (info[i].iiFlags & IFF_LOOPBACK) != 0; n.zt = std::find(zt.begin(), zt.end(), n.ip) != zt.end(); v.push_back(n);
            }
        closesocket(s);
    }
#else
    ifaddrs* ia = nullptr;
    if (getifaddrs(&ia) == 0) {
        for (ifaddrs* a = ia; a; a = a->ifa_next) {
            if (!a->ifa_addr || a->ifa_addr->sa_family != AF_INET || !a->ifa_netmask || !(a->ifa_flags & IFF_UP)) continue;
            NIf n; n.ip = ntohl(((sockaddr_in*)a->ifa_addr)->sin_addr.s_addr); n.mask = ntohl(((sockaddr_in*)a->ifa_netmask)->sin_addr.s_addr);
            n.lo = (a->ifa_flags & IFF_LOOPBACK) != 0; n.name = a->ifa_name; netClassify(n); v.push_back(n);
        }
        freeifaddrs(ia);
    }
    { bool any = false; for (const NIf& n : v) if (!n.lo && n.ip) any = true; if (!any) netIfacesIoctl(v); }
#endif
    bool any = false; for (const NIf& n : v) if (!n.lo && n.ip) any = true;
    if (!any) { const uint32_t p = netPrimaryIp(); if (p) { NIf n; n.ip = p; n.mask = 0xFFFFFF00u; v.push_back(n); } }
    return v;
}
// meus enderecos (para dizer aos amigos o que digitar): "LAN"/"ZeroTier" + IP
static std::vector<std::pair<std::string, std::string>> netMyAddrs() {
    std::vector<std::pair<std::string, std::string>> r;
    for (const NIf& n : netIfaces()) if (!n.lo && n.ip) r.push_back({n.zt ? "ZeroTier" : "LAN", ipStr(n.ip)});
    return r;
}
static std::string netZtStatus() {   // para a tela Multiplayer (chamado a cada quadro): ZeroTier detectado? cache de 3 s
    static std::string r; static Uint32 t = 0;
    if (!t || SDL_GetTicks() - t > 3000) {
        t = SDL_GetTicks() | 1; std::string s; for (auto& a : netMyAddrs()) if (a.first == "ZeroTier") s += (s.empty() ? "" : ", ") + a.second;
        r = s.empty() ? "ZeroTier: não detectado (abra o ZeroTier e entre na rede, ou use Conexão Direta)" : "ZeroTier: ativo em " + s;
    }
    return r;
}
static std::string netAddrSummary() {   // "192.168.0.5 (LAN), 10.147.17.5 (ZeroTier)"; recalculado no maximo a cada 3 s (e chamado a cada quadro pelo menu)
    static std::string s; static Uint32 t = 0;
    if (!t || SDL_GetTicks() - t > 3000) { t = SDL_GetTicks() | 1; s.clear(); for (auto& a : netMyAddrs()) s += (s.empty() ? "" : ", ") + a.second + " (" + a.first + ")"; if (s.empty()) s = "nenhuma rede detectada"; }
    return s;
}

// ---- lista de servidores: salvos (servers.cfg, "nome|host:porta") + descobertos na rede ----
struct SrvSaved { std::string name, host; int port = 47777; };
struct LanEnt { uint32_t ip = 0; int port = 47777, players = 0, maxp = 0, mode = 0; std::string name, kind; Uint32 seen = 0; };
struct SrvRow { std::string name, l1, l2, cnt, host; unsigned c1 = 0xA0A0A0, c2 = 0x808080; bool ok = false, saved = false; int idx = -1, port = 47777; };
static std::vector<SrvSaved> gSrvSaved; static std::vector<LanEnt> gLan; static bool gScanForce = false, gSrvLoaded = false; static Uint32 gMultiVis = 0;
static bool srvParse(const std::string& in, std::string& host, int& port) {   // "host", "host:porta"
    std::string s = in; while (!s.empty() && (s.back() == ' ' || s.back() == '\r' || s.back() == '\n')) s.pop_back(); size_t b = 0; while (b < s.size() && s[b] == ' ') b++; s = s.substr(b);
    port = 47777; const size_t c = s.rfind(':');
    if (c != std::string::npos) { const int p = atoi(s.c_str() + c + 1); if (p < 1 || p > 65535) return false; port = p; s = s.substr(0, c); }
    host = s; return !host.empty();
}
static void srvSave() { FILE* f = fopen("servers.cfg", "wb"); if (!f) return; for (const SrvSaved& s : gSrvSaved) fprintf(f, "%s|%s:%d\n", s.name.c_str(), s.host.c_str(), s.port); fclose(f); }
static void srvEnsure() {
    if (gSrvLoaded) return; gSrvLoaded = true; FILE* f = fopen("servers.cfg", "rb"); if (!f) return; char ln[300];
    while (fgets(ln, sizeof ln, f)) {
        std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back(); const size_t q = s.rfind('|'); if (q == std::string::npos) continue;
        SrvSaved e; e.name = s.substr(0, q); if (srvParse(s.substr(q + 1), e.host, e.port)) { if (e.name.empty()) e.name = e.host; gSrvSaved.push_back(e); }
    }
    fclose(f);
}
static void srvAdd(int edit, std::string name, const std::string& addr) {
    srvEnsure(); SrvSaved e; if (!srvParse(addr, e.host, e.port)) return;
    for (char& c : name) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    while (!name.empty() && name.back() == ' ') name.pop_back(); e.name = name.empty() ? e.host : name;
    if (edit >= 0 && edit < (int)gSrvSaved.size()) gSrvSaved[edit] = e; else gSrvSaved.push_back(e);
    srvSave();
}
static void srvDel(int i) { srvEnsure(); if (i >= 0 && i < (int)gSrvSaved.size()) { gSrvSaved.erase(gSrvSaved.begin() + i); srvSave(); } }
static std::vector<SrvRow> srvRows() {
    srvEnsure(); std::vector<SrvRow> v; std::set<uint64_t> used;
    auto modeS = [](int m) { return std::string(m ? "Criativo" : "Sobrevivência"); };
    for (size_t i = 0; i < gSrvSaved.size(); i++) {
        const SrvSaved& s = gSrvSaved[i]; SrvRow r; r.name = s.name; r.host = s.host; r.port = s.port; r.saved = true; r.idx = (int)i;
        uint32_t ip = 0; if (!ipParse(s.host, ip)) { auto it = gDns.find(s.host); if (it != gDns.end()) ip = it->second; }
        const std::string ad = s.host + ":" + std::to_string(s.port); const LanEnt* hit = nullptr;
        if (ip) for (const LanEnt& l : gLan) if (l.ip == ip && l.port == s.port) { hit = &l; used.insert(((uint64_t)l.ip << 16) | (uint32_t)l.port); }
        if (hit) { r.ok = true; r.l1 = hit->name; r.c1 = 0x55FFFF; r.l2 = ad + "  " + hit->kind + "  " + modeS(hit->mode); r.cnt = std::to_string(hit->players) + "/" + std::to_string(hit->maxp); }
        else { r.l1 = "Sem resposta (o servidor pode estar fechado)"; r.c1 = 0xAA0000; r.l2 = ad; }
        v.push_back(r);
    }
    for (const LanEnt& l : gLan) {
        if (used.count(((uint64_t)l.ip << 16) | (uint32_t)l.port)) continue;
        SrvRow r; r.name = l.name; r.host = ipStr(l.ip); r.port = l.port; r.ok = true; r.l1 = l.kind + "  " + r.host + ":" + std::to_string(l.port); r.c1 = l.kind == "ZeroTier" ? 0x55FFFF : 0x55FF55;
        r.l2 = modeS(l.mode); r.c2 = 0xAAAAAA; r.cnt = std::to_string(l.players) + "/" + std::to_string(l.maxp); v.push_back(r);
    }
    return v;
}

// ---- descoberta ----
static int scanSock = -1, discSock = -1; static Uint32 scanT0 = 0;
struct Probe { uint32_t ip; int port; };
static std::vector<Probe> probeQ; static size_t probeI = 0; static std::vector<NIf> ifCache; static std::string gHostName = "Blockland";
static bool gScanWide = false;   // "Atualizar": varre tambem blocos maiores (ate /20 na LAN e /16 no ZeroTier)
static void lanScanForce() { gScanForce = true; gScanWide = true; gDns.clear(); gLan.clear(); }
static std::string lanKind(uint32_t ip) {
    if ((ip >> 24) == 127) return "Este PC";
    for (const NIf& n : ifCache) if (!n.lo && n.mask && (ip & n.mask) == (n.ip & n.mask)) return n.zt ? "ZeroTier" : "LAN";
    return "Internet";
}
static void lanBuildProbes(bool wide) {
    ifCache = netIfaces(); probeQ.clear(); probeI = 0; std::set<uint64_t> seen;
    auto add = [&](uint32_t ip, int port) { if (seen.insert(((uint64_t)ip << 16) | (uint32_t)port).second) probeQ.push_back({ip, port}); };
    add(0x7F000001u, NET_DISC_PORT); add(0xFFFFFFFFu, NET_DISC_PORT);
    for (const NIf& n : ifCache) {
        if (n.lo || !n.ip) continue;
        const uint32_t net = n.ip & n.mask, bc = net | ~n.mask; add(bc, NET_DISC_PORT);
        int pre = 0; for (uint32_t m = n.mask; m & 0x80000000u; m <<= 1) pre++;
        const int lim = wide ? (n.zt ? 16 : 20) : 24;   // 1a passada: /24 do proprio IP (rapida); 2a (so em "Atualizar"): bloco maior, de perto para longe
        for (int pass = 0; pass < 2; pass++) {
            const int use = std::max(pre, pass == 0 ? 24 : lim); if (pass == 1 && use >= 24) break; if (use >= 31) continue;
            const uint32_t um = ~0u << (32 - use), lo = (n.ip & um) + 1, hi = (n.ip | ~um) - 1;
            for (uint32_t a = lo; a <= hi && a >= lo; a++) if (a != n.ip) add(a, NET_DISC_PORT);
        }
    }
    for (const SrvSaved& s : gSrvSaved) { uint32_t h; if (netResolve(s.host, h)) add(h, s.port + 1); }
}
static void lanTick(Uint32 now) {
    if (discSock >= 0) for (int g = 0; g < 16; g++) {   // HOST: responde as consultas
        uint8_t buf[64]; sockaddr_in from{}; socklen_t fl = sizeof from;
        const int n = (int)recvfrom(discSock, (char*)buf, sizeof buf, 0, (sockaddr*)&from, &fl);
        if (n < 0) break;
        if (n < 4 || memcmp(buf, "BLQ", 3) != 0) continue;
        NBuf r; r.put<uint8_t>('B'); r.put<uint8_t>('L'); r.put<uint8_t>('A'); r.put<uint8_t>((uint8_t)NET_VER); r.put<uint16_t>((uint16_t)gNetPort);
        r.put<uint8_t>((uint8_t)std::min<size_t>(255, peers.size() + 1)); r.put<uint8_t>((uint8_t)NET_MAXP); r.put<uint8_t>(creative ? 1 : 0);
        const std::string nm = gHostName.substr(0, 40); r.put<uint8_t>((uint8_t)nm.size()); for (char ch : nm) r.put<uint8_t>((uint8_t)ch);
        sendto(discSock, (const char*)r.d, r.n, 0, (sockaddr*)&from, sizeof from);
    }
    const bool want = gMultiVis && now - gMultiVis < 500 && netSock < 0;   // CLIENTE: so varre com a aba Multiplayer aberta e fora de um jogo em rede
    if (!want) { if (scanSock >= 0) { netSockClose(scanSock); scanSock = -1; } return; }
    if (scanSock < 0) {
        netSockInit(); scanSock = (int)socket(AF_INET, SOCK_DGRAM, 0); if (scanSock < 0) return;
        netSockNonBlock(scanSock); netSockQuirks(scanSock); int one = 1; setsockopt(scanSock, SOL_SOCKET, SO_BROADCAST, (const char*)&one, sizeof one);
        sockaddr_in me{}; me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_ANY); me.sin_port = 0;
        if (bind(scanSock, (sockaddr*)&me, sizeof me) < 0) { netSockClose(scanSock); scanSock = -1; return; }
        scanT0 = 0;
    }
    if (!scanT0 || gScanForce || (probeI >= probeQ.size() && now - scanT0 > 5000)) { gScanForce = false; scanT0 = now | 1; lanBuildProbes(gScanWide); gScanWide = false; }   // varredura grande termina antes de recomecar
    static const uint8_t q[4] = {'B', 'L', 'Q', (uint8_t)NET_VER};
    const int perFrame = probeQ.size() > 2000 ? 400 : 64;
    for (int k = 0; k < perFrame && probeI < probeQ.size(); k++, probeI++) {   // 64 consultas por quadro (/24 = 4 quadros); 400 nas varreduras grandes
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons((uint16_t)probeQ[probeI].port); a.sin_addr.s_addr = htonl(probeQ[probeI].ip);
        sendto(scanSock, (const char*)q, 4, 0, (sockaddr*)&a, sizeof a);
    }
    for (int g = 0; g < 64; g++) {
        NBuf r; sockaddr_in from{}; socklen_t fl = sizeof from;
        const int n = (int)recvfrom(scanSock, (char*)r.d, 96, 0, (sockaddr*)&from, &fl);
        if (n < 0) break;
        if (n < 11 || memcmp(r.d, "BLA", 3) != 0) continue;
        r.n = n; r.r = 3; const int ver = r.get<uint8_t>(); const int port = r.get<uint16_t>(), pl = r.get<uint8_t>(), mx = r.get<uint8_t>(), md = r.get<uint8_t>(), nl = r.get<uint8_t>();
        if (ver != NET_VER) continue;
        std::string nm; for (int i = 0; i < nl; i++) nm += (char)r.get<uint8_t>();
        const uint32_t ip = ntohl(from.sin_addr.s_addr); LanEnt* e = nullptr;
        for (LanEnt& l : gLan) if (l.ip == ip && l.port == port) e = &l;
        if (!e) { gLan.push_back({}); e = &gLan.back(); }
        e->ip = ip; e->port = port; e->players = pl; e->maxp = mx; e->mode = md; e->name = nm.empty() ? "Mundo" : nm; e->kind = lanKind(ip); e->seen = now;
    }
    for (size_t i = 0; i < gLan.size();) { if (now - gLan[i].seen > 12000) gLan.erase(gLan.begin() + i); else ++i; }
}

// ---- hospedar / entrar / sair ----
static void netStop() {
    if (netSock >= 0) { if (netHost || netConn) netBroadcast(netMk(NP_BYE)); netSockClose(netSock); netSock = -1; }
    if (discSock >= 0) { netSockClose(discSock); discSock = -1; }
    peers.clear(); remotes.clear(); netEdits.clear(); netPend.clear(); netHost = netConn = netOn = false; gJoin = NJ_IDLE; gGuestReady = false; gHostPlName.clear();
}
static bool netStartHost(int port) {
    if (netSock >= 0) return netHost;
    netSockInit(); const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); if (s < 0) { chatAdd("Não foi possível abrir a rede.", 0xFF5555); return false; }
    netSockNonBlock(s); netSockQuirks(s); sockaddr_in me{}; me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_ANY); me.sin_port = htons((uint16_t)port);
    if (bind(s, (sockaddr*)&me, sizeof me) < 0) { netSockClose(s); chatAdd("Porta UDP " + std::to_string(port) + " ocupada (outro jogo aberto para LAN neste PC?).", 0xFF5555); return false; }
    netSock = s; netHost = true; netConn = false; netSelf = 0; gNetPort = port; peers.clear(); remotes.clear(); gJoin = NJ_IDLE;
    gHostName = gCurDir.empty() ? "Blockland" : gCurDir; for (const WInfo& w : gWorlds) if (w.dir == gCurDir) gHostName = w.name;
    discSock = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (discSock >= 0) {
        netSockNonBlock(discSock); netSockQuirks(discSock); me.sin_port = htons((uint16_t)(port + 1));
        if (bind(discSock, (sockaddr*)&me, sizeof me) < 0) { netSockClose(discSock); discSock = -1; chatAdd("Aviso: a porta de descoberta está ocupada; entre por IP.", 0xFFAA00); }
    }
    if (netWin) SDL_SetWindowTitle(netWin, "Blockland - Jogador 1 (host)");
    chatAdd("Mundo aberto para LAN! Porta UDP " + std::to_string(port) + ". Seus amigos entram pela lista Multiplayer ou por IP:", 0x55FF55);
    for (auto& a : netMyAddrs()) chatAdd("  " + a.first + ": " + a.second + ":" + std::to_string(port), a.first == "ZeroTier" ? 0x55FFFF : 0xFFFF55);
#ifdef _WIN32
    chatAdd("Windows: se ninguém conseguir entrar, permita o jogo no Firewall (redes pública e privada; UDP " + std::to_string(port) + " e " + std::to_string(port + 1) + ").", 0xFFAA00);
#endif
    bool hasZt = false; for (auto& a : netMyAddrs()) if (a.first == "ZeroTier") hasZt = true;
    if (!hasZt) chatAdd("ZeroTier não detectado aqui: para amigos pela internet, entre na mesma rede ZeroTier.", 0xAAAAAA);
    return true;
}
static void netJoinBegin(const std::string& host, int port) {
    netStop(); gJoinMsg = host + ":" + std::to_string(port); uint32_t ip = 0;
    if (!netResolve(host, ip)) { gJoin = NJ_FAILED; gJoinMsg = "Endereço inválido ou sem DNS: " + host; return; }
    netSock = (int)socket(AF_INET, SOCK_DGRAM, 0); if (netSock < 0) { gJoin = NJ_FAILED; gJoinMsg = "Não foi possível abrir a rede."; return; }
    netSockNonBlock(netSock); netSockQuirks(netSock); sockaddr_in me{}; me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_ANY); me.sin_port = 0;
    if (bind(netSock, (sockaddr*)&me, sizeof me) < 0) { netSockClose(netSock); netSock = -1; gJoin = NJ_FAILED; gJoinMsg = "Não foi possível abrir a rede."; return; }
    netHostAddr = {}; netHostAddr.sin_family = AF_INET; netHostAddr.sin_port = htons((uint16_t)port); netHostAddr.sin_addr.s_addr = htonl(ip);
    netHost = false; netConn = false; netOn = false; gGuestReady = false; netLastHello = 0; gJoin = NJ_CONNECTING; gJoinT0 = SDL_GetTicks();
}
static void netPrepareGuestWorld() {   // WELCOME recebido: grava um save temporario com a seed/modo/hora/posicao do host; o loop principal entao chama startWorld
    std::error_code ec; fs::create_directories("saves/_mp", ec); remove("saves/_mp/chunks16.dat");
    SaveInfo s; s.dir = "_mp"; s.name = "Multiplayer"; s.seed = gJoinSeed; s.mode = gJoinCreative; s.played = (long long)time(nullptr); s.wtime = gJoinTime;
    s.px = gJoinPos[0] + 1.0; s.py = gJoinPos[1]; s.pz = gJoinPos[2]; writeCfg(s); gJoin = NJ_INGAME;
}
static void netGuestReady() {   // mundo carregado: pede ao host as edicoes e drops existentes
    if (gJoin != NJ_INGAME) return;
    gGuestReady = true; netLastHello = 0; if (netWin) SDL_SetWindowTitle(netWin, ("Blockland - " + netName()).c_str());
    chatAdd("Conectado ao host como " + netName(), 0x55FF55);
}
static const char* netLanLabel() { static char b[48]; if (netHost) { snprintf(b, sizeof b, "LAN aberta (porta %d)", gNetPort); return b; } return netSock >= 0 ? "Conectado a um host" : "Abrir para LAN"; }
static bool netLanBusy() { return netSock >= 0; }
