// ---------- multiplayer (UDP, LAN / ZeroTier / IP direto; PC Windows, Linux e Android) ----------
// A rede so abre pelo menu: Multiplayer > Hospedar LAN / lista / Conexao Direta, ou pause > "Abrir para LAN" (descoberta e lista em netlan.inc).
// Linha de comando (opcional): --join IP[:porta] | --host (abre o 1o mundo jogado para LAN) | --port N
// Sincroniza: jogadores (posição, mira, item, braço), blocos quebrados/colocados, drops, pickup e chat.
#ifdef _WIN32   // Winsock (headers já incluídos em main.cpp); Linux usa sockets POSIX
typedef int socklen_t;
static void netSockInit() { static bool done = false; if (!done) { WSADATA w; WSAStartup(MAKEWORD(2, 2), &w); done = true; } }
static void netSockNonBlock(int s) { u_long m = 1; ioctlsocket((SOCKET)s, FIONBIO, &m); }
static void netSockClose(int s) { closesocket((SOCKET)s); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
static void netSockInit() {}
static void netSockNonBlock(int s) { fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); }
static void netSockClose(int s) { close(s); }
#endif
#include <deque>
#include <map>
enum { NP_HELLO = 1, NP_WELCOME, NP_STATE, NP_BLOCKS, NP_DROP, NP_DROPSET, NP_BYE, NP_CHAT, NP_KICK, NP_MON, NP_MONF, NP_CHEST, NP_CHESTREQ, NP_BOAT, NP_BOATDEL, NP_BOATHIT, NP_ENDER, NP_ENDERREQ, NP_PVP, NP_DEATH, NP_HURT, NP_ARROW };
static const int NET_VER = 11, NET_DISC_PORT = 47778, NET_MAXP = 8;   // NET_VER: HELLO leva o nome do jogador e WELCOME o do host
enum { NJ_IDLE, NJ_CONNECTING, NJ_WELCOMED, NJ_INGAME, NJ_FAILED };   // estado do convidado: tela "Conectando" -> mundo -> jogando
static int gJoin = NJ_IDLE, gNetPort = 47777, gJoinCreative = 0; static std::string gJoinMsg, gCliJoin;
static bool gGuestReady = false, gHostPending = false, gHostNext = false, gCliHost = false;
static Uint32 gJoinT0 = 0; static int64_t gJoinSeed = 0; static int32_t gJoinTime = 0; static float gJoinPos[3] = {8.5f, 0, 8.5f}; static double gNetPos[3] = {8.5, 0, 8.5};
static void netStop(); static void lanTick(Uint32 now);   // netlan.inc
struct NBuf {
    uint8_t d[1400]; int n = 0, r = 0;
    template <class T> void put(T v) { if (n + (int)sizeof(T) <= (int)sizeof d) { memcpy(d + n, &v, sizeof(T)); n += sizeof(T); } }
    template <class T> T get() { T v{}; if (r + (int)sizeof(T) <= n) { memcpy(&v, d + r, sizeof(T)); r += sizeof(T); } return v; }
};
struct NetPeer { sockaddr_in a; Uint32 seen; int id; std::deque<NBuf> q; bool synced = false; std::string name; };
static std::string gHostPlName;   // nome do host (vem no WELCOME)
struct RState { double x, y, z; float yaw, pitch, swing; int held; bool sneak; uint8_t ely = 0; };   // ely: bit0 = veste elitra, bit1 = planando
// Jogador remoto: simulado a cada tick (20/s) com a MESMA lógica do jogador local (corpo, membros, balanço) e interpolado com partialTicks.
struct RemotePl {
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0; float yaw = 0, pyaw = 0, pitch = 0, ppitch = 0;
    float bodyYaw = 0, pBodyYaw = 0, limbSwing = 0, limbAmt = 0, pLimbAmt = 0, ageTicks = 0, swing = 0, lastRecvSwing = 0;
    int gm = 0, bow = -1, held = 0, deathT = -1, hurtT = 0; bool sneak = false, swinging = false, init = false; Uint32 seen = 0; std::deque<RState> q;
    uint8_t ely = 0; float elyTicks = 0; ElySm elySm;
};
static void arrowNetRecv(NBuf& b, int sender);   // bow.inc
static void pvpReceive(int from, int dmg, float kx, float kz, float kb);   // pvp.inc
static void chestNetRecv(NBuf& b); static void chestNetReq(NBuf& b); static void enderNetRecv(NBuf& b, int sender); static void enderNetReq(int sender);   // chest.inc: conteudo de bau (NP_CHEST) e pedido do convidado (NP_CHESTREQ)
static void monNetRecv(NBuf& b); static void monNetSync(NetPeer& p); static void monFrameRecv(NBuf& b);   // modules/monitor
struct BlockEdit { int x, y, z, id; int meta = 0; };
static bool gNetReseed = false;   // pede ao loop principal para recolocar o jogador acima do terreno novo
static int netSock = -1; static bool netHost = false, netConn = false;
static sockaddr_in netHostAddr; static SDL_Window* netWin = nullptr;
static std::vector<NetPeer> peers; static std::map<int, RemotePl> remotes;
static std::unordered_map<int64_t, int> netEdits, netPend;   // edits conhecidas / edits em chunks ainda não carregados. Valor = id | meta << 16
static inline int netPackEdit(int id, int meta) { return (id & 0xFFFF) | ((meta & 0xFF) << 16); }
static inline int netEditId(int v) { return v & 0xFFFF; }
static inline int netEditMeta(int v) { return (v >> 16) & 0xFF; }
static Uint32 netLastHello = 0, netLastRec = 0, netHostSeen = 0;

static NBuf netMk(int type) { NBuf b; b.put<uint8_t>((uint8_t)type); b.put<uint8_t>((uint8_t)netSelf); return b; }
static void netTo(const sockaddr_in& a, const NBuf& b) { sendto(netSock, (const char*)b.d, b.n, 0, (const sockaddr*)&a, sizeof a); }
static void netBroadcast(const NBuf& b) {
    if (netSock < 0) return;
    if (netHost) { for (auto& p : peers) netTo(p.a, b); } else if (netConn) netTo(netHostAddr, b);
}
static std::string netName() { if (!gPlayerName.empty()) return gPlayerName; return netSock < 0 ? "Jogador" : "Jogador " + std::to_string(netSelf + 1); }
static void netPutName(NBuf& b) { const std::string n = netName().substr(0, 40); b.put<uint8_t>((uint8_t)n.size()); for (char ch : n) b.put<uint8_t>((uint8_t)ch); }
static std::string netGetName(NBuf& b) { const int l = b.get<uint8_t>(); std::string s; for (int i = 0; i < l; i++) s += (char)b.get<uint8_t>(); return profClean(s); }
static std::string netLabel(int id) {   // nome de um jogador pelo id de rede (0 = host)
    if (id == netSelf) return netName(); if (id == 0 && !gHostPlName.empty()) return gHostPlName;
    for (const NetPeer& p : peers) if (p.id == id && !p.name.empty()) return p.name;
    return "Jogador " + std::to_string(id + 1);
}
// ---- botes (boat.inc): autoridade = quem monta (ou o host se ninguém); os outros recebem NP_BOAT e interpolam ----
static NBuf boatNetPacket(const BoatEnt& e) {
    NBuf b = netMk(NP_BOAT); b.put<int32_t>(e.id); b.put<uint8_t>((uint8_t)e.type);
    b.put<double>(e.x); b.put<double>(e.y); b.put<double>(e.z); b.put<float>(e.yaw); b.put<float>(e.damage);
    b.put<uint8_t>((uint8_t)std::max(0, std::min(255, e.timeSinceHit))); b.put<uint8_t>((uint8_t)((e.paddle[0] ? 1 : 0) | (e.paddle[1] ? 2 : 0))); b.put<uint8_t>((uint8_t)e.status);
    b.put<int8_t>((int8_t)e.fwdDir); b.put<int8_t>((int8_t)e.rider);
    return b;
}
static void boatNetSend(const BoatEnt& e) { if (netOn) netBroadcast(boatNetPacket(e)); }
static void boatNetRecv(NBuf& b) {
    const int id = b.get<int32_t>(), ty = b.get<uint8_t>(); BNState s{};
    s.x = b.get<double>(); s.y = b.get<double>(); s.z = b.get<double>(); s.yaw = b.get<float>(); s.damage = b.get<float>();
    s.hit = b.get<uint8_t>(); s.flags = b.get<uint8_t>(); s.status = b.get<uint8_t>(); s.fwd = b.get<int8_t>(); s.rider = b.get<int8_t>();
    boatNetApply(id, ty, s);
}
static void boatNetDel(int id) { if (!netOn) return; NBuf b = netMk(NP_BOATDEL); b.put<int32_t>(id); netBroadcast(b); }
static void boatNetHit(int id, float dmg, bool creative) { if (!netOn) return; NBuf b = netMk(NP_BOATHIT); b.put<int32_t>(id); b.put<float>(dmg); b.put<uint8_t>(creative ? 1 : 0); netBroadcast(b); }
static bool boatNetAlive(int id) { if (!netHost || id == netSelf) return true; for (const NetPeer& p : peers) if (p.id == id) return true; return false; }   // so o host confere
static void netSendChat(const std::string& s) {
    if (!netOn) return;
    NBuf b = netMk(NP_CHAT); const int l = (int)std::min<size_t>(s.size(), 250); b.put<uint8_t>((uint8_t)l);
    for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)s[i]);
    netBroadcast(b);
}
static void netSay(const std::string& s) { chatAdd(s, 0xFFFF55); if (netHost) netSendChat(s); }
static void resetWorldData();
// ids do pack dependem do resource pack de cada máquina: na rede o bloco vai como hash do nome do arquivo (bit 30 ligado)
static int32_t netWireId(int id) { return isPk(id) ? (int32_t)(0x40000000u | netFnvPk(PK[id - PK0].file)) : id; }
static int netUnwireId(int32_t v) { if (!(v & 0x40000000)) return v; auto it = gPkByHash.find((uint32_t)v & 0x3FFFFFFFu); return it == gPkByHash.end() ? -1 : it->second; }
static void netPutEdit(NBuf& b, const BlockEdit& e) { b.put<int32_t>(e.x); b.put<int32_t>(e.y); b.put<int32_t>(e.z); b.put<int32_t>(netWireId(e.id)); b.put<uint8_t>((uint8_t)e.meta); }
static void netSendBlock(int x, int y, int z, int id, int meta) {   // (valor padrao de meta = 0 na declaracao antecipada em models.inc)
    y += gDim << 10;   // a dimensao viaja no bit 10 de y (0 = superficie, 1 = Nether)
    netEdits[k3(x, y, z)] = netPackEdit(id, meta);
    if (!netOn) return;
    NBuf b = netMk(NP_BLOCKS); b.put<uint8_t>(1); netPutEdit(b, {x, y, z, id, meta}); netBroadcast(b);
}
static NBuf netDropPacket(const DropEnt& d) {
    NBuf b = netMk(NP_DROP); b.put<uint32_t>(d.nid);
    b.put<float>((float)d.x); b.put<float>((float)d.y); b.put<float>((float)d.z); b.put<float>((float)d.vx); b.put<float>((float)d.vy); b.put<float>((float)d.vz);
    b.put<int16_t>((int16_t)d.id); b.put<int16_t>((int16_t)d.n); b.put<int16_t>((int16_t)d.delay);
    return b;
}
static bool netRemotePos(int id, float pt, double* o) {
    auto it = remotes.find(id); if (it == remotes.end() || !it->second.init) return false; const RemotePl& r = it->second;
    o[0] = r.px + (r.x - r.px) * pt; o[1] = r.py + (r.y - r.py) * pt; o[2] = r.pz + (r.z - r.pz) * pt; return true;
}
static void netSendDrop(const DropEnt& d) { if (netOn) netBroadcast(netDropPacket(d)); }
static void netSendDropSet(uint32_t nid, int n, int picker) {
    if (!netOn) return;
    NBuf b = netMk(NP_DROPSET); b.put<uint32_t>(nid); b.put<int16_t>((int16_t)n); b.put<uint8_t>((uint8_t)picker); netBroadcast(b);
}
static void netApplyEdit(int x, int y, int z, int id, int meta = 0) {
    netEdits[k3(x, y, z)] = netPackEdit(id, meta);
    if (((y >> 10) & 3) != gDim || !getC(fdiv(x, CW), fdiv(z, CW))) { netPend[k3(x, y, z)] = netPackEdit(id, meta); return; }   // outra dimensao ou chunk ainda nao carregado: fica pendente
    y &= 1023;
    const int old = getB(x, y, z); if (old == id && getM(x, y, z) == meta) return;
    if (id == AIR && old) spawnBreak(x, y, z, old);
    if (isLogB(old) || isLeafB(old)) { keepLeaves.erase(k3(x, y, z)); }
    gNoBlockHooks = true; setBM(x, y, z, id, meta); gNoBlockHooks = false;   // a cascata (queda de tocha etc.) ja veio como edicoes de quem editou
    if (isLeafB(id)) keepLeaves.insert(k3(x, y, z));
    if (isLogB(old) || isLeafB(old)) leafNeighbors(x, y, z);
}
static void netSyncPeer(NetPeer& p) {   // chamado quando o convidado ja carregou o mundo (HELLO "pronto"): edicoes de blocos + drops existentes
    std::vector<BlockEdit> v;
    for (auto& kv : netEdits) { int x, y, z; unk3(kv.first, x, y, z); v.push_back({x, y, z, netEditId(kv.second), netEditMeta(kv.second)}); }
    for (size_t i = 0; i < v.size(); i += 80) {
        NBuf b = netMk(NP_BLOCKS); const size_t c = std::min<size_t>(80, v.size() - i); b.put<uint8_t>((uint8_t)c);
        for (size_t k = 0; k < c; k++) netPutEdit(b, v[i + k]);
        p.q.push_back(b);
    }
    for (const DropEnt& d : DROPS) p.q.push_back(netDropPacket(d));
    for (const BoatEnt& e : BOATS) if (!e.dead && e.rider < 0) p.q.push_back(boatNetPacket(e));   // botes existentes
    monNetSync(p);   // URLs dos monitores
}
static void netKick(const sockaddr_in& to, const char* msg) {
    NBuf k = netMk(NP_KICK); const int l = (int)strlen(msg); k.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) k.put<uint8_t>((uint8_t)msg[i]); netTo(to, k);
}
static bool netNickTaken(const std::string& n, const NetPeer* except, const std::string* hostNm = nullptr) {   // host + convidados, sem diferenciar maiuscula
    if (nickEq(n, hostNm ? *hostNm : netName())) return true;
    for (const NetPeer& q : peers) if (&q != except && nickEq(q.name, n)) return true;
    return false;
}
// ---- /list e /nick (chat_debug.inc) ----
static std::vector<std::string> cmdPlayerList() {   // jogadores online: eu + host/convidados conhecidos (convidado so conhece os nomes que ja chegaram)
    std::vector<std::string> v; v.push_back(netName());
    if (netOn) {
        if (!netHost) v.push_back(netLabel(0));
        for (const NetPeer& p : peers) v.push_back(p.name.empty() ? netLabel(p.id) : p.name);
        if (!netHost) for (auto& kv : remotes) if (kv.first != 0 && kv.first != netSelf) v.push_back(netLabel(kv.first));
    }
    std::vector<std::string> u; for (auto& n : v) { bool dup = false; for (auto& m : u) if (nickEq(m, n)) dup = true; if (!dup) u.push_back(n); }
    return u;
}
static std::string cmdNickChange(const std::string& arg) {   // "" = trocou; senao a mensagem de erro
    if (netOn) return "Não dá para trocar o nick durante uma partida em rede (saia e troque no launcher).";
    if (!nickValid(arg)) return "Nick inválido: use de 1 a 16 caracteres entre A-Z, a-z, 0-9 e _.";
    gPlayerName = arg; profileSave(); return "";
}
static NetPeer* netAddPeer(const sockaddr_in& from, Uint32 now, const std::string& name = "") {
    int id = 1; for (bool used = true; used;) { used = false; for (auto& p : peers) if (p.id == id) { used = true; id++; } }
    peers.push_back({from, now, id, {}, false, name});
    netSay((name.empty() ? "Jogador " + std::to_string(id + 1) : name) + " entrou no jogo");
    return &peers.back();
}
static void netPoll(Uint32 now) {
    for (int guard = 0; guard < 512; guard++) {
        NBuf b; sockaddr_in from{}; socklen_t fl = sizeof from;
        const int n = (int)recvfrom(netSock, (char*)b.d, sizeof b.d, 0, (sockaddr*)&from, &fl);
        if (n < 0) break;
        if (n < 2) continue;
        b.n = n;
        const int type = b.get<uint8_t>(), sender = b.get<uint8_t>();
        if (netHost) {
            NetPeer* p = nullptr;
            for (auto& q : peers) if (q.a.sin_addr.s_addr == from.sin_addr.s_addr && q.a.sin_port == from.sin_port) p = &q;
            if (type == NP_HELLO) {
                const int ver = b.get<uint8_t>(), ready = b.get<uint8_t>();
                if (ver != NET_VER) { netKick(from, "Versão do jogo diferente da do host."); continue; }
                const std::string pname0 = nickSanitize(netGetName(b)); std::string pname = pname0;
                if (!pname.empty()) {   // convidado fantasma (caiu e voltou por outra porta) com o mesmo nick: libera o nick em vez de recusar quem voltou
                    for (size_t i = 0; i < peers.size();) {
                        NetPeer& q = peers[i];
                        const bool same = q.a.sin_addr.s_addr == from.sin_addr.s_addr && q.a.sin_port == from.sin_port;
                        if (!same && nickEq(q.name, pname) && now - q.seen > 2000) { remotes.erase(q.id); peers.erase(peers.begin() + i); } else i++;
                    }
                    p = nullptr; for (auto& q : peers) if (q.a.sin_addr.s_addr == from.sin_addr.s_addr && q.a.sin_port == from.sin_port) p = &q;
                }
                if (pname.empty() || netNickTaken(pname, p)) {   // sem nick valido ou ja em uso
                    if (pname.empty() || nickIsAuto(pname)) pname = nickSuggest("Jogador", [&](const std::string& c) { return netNickTaken(c, p); });   // nome padrao: troca sozinho
                    else {   // nick escolhido pelo jogador: recusa e sugere variacao (como o oficial recusa "ja logado")
                        const std::string alt = nickSuggest(pname, [&](const std::string& c) { return netNickTaken(c, p); });
                        netKick(from, ("Já existe um jogador com o nick \"" + pname + "\" neste servidor. Tente \"" + alt + "\".").c_str()); continue;
                    }
                }
                if (!p) { if ((int)peers.size() >= NET_MAXP - 1) { netKick(from, "Servidor cheio."); continue; } p = netAddPeer(from, now, pname); }
                p->name = pname;
                p->seen = now; if (ready && !p->synced) { p->synced = true; netSyncPeer(*p); }
                NBuf w = netMk(NP_WELCOME); w.put<uint8_t>((uint8_t)p->id); w.put<int64_t>(WORLD_SEED); w.put<int32_t>((int32_t)gWorldTime);
                w.put<uint8_t>(creative ? 1 : 0); w.put<float>((float)gNetPos[0]); w.put<float>((float)gNetPos[1]); w.put<float>((float)gNetPos[2]); netPutName(w); netTo(from, w);
                continue;
            }
            if (!p) continue;
            p->seen = now;
            for (auto& q : peers) if (&q != p) sendto(netSock, (const char*)b.d, n, 0, (sockaddr*)&q.a, sizeof q.a);   // repassa aos outros
            if (type == NP_BYE) {
                remotes.erase(sender); netSay(netLabel(sender) + " saiu do jogo");
                for (size_t i = 0; i < peers.size(); i++) if (peers[i].id == sender) { peers.erase(peers.begin() + i); break; }
                continue;
            }
        } else {
            if (type == NP_KICK) {
                const int l = b.get<uint8_t>(); std::string m; for (int i = 0; i < l; i++) m += (char)b.get<uint8_t>();
                if (gJoin == NJ_CONNECTING) { gJoin = NJ_FAILED; gJoinMsg = m; }
                continue;
            }
            if (type == NP_WELCOME) {   // o mundo (seed, modo, hora, posicao) e criado pelo loop principal depois deste aviso; HELLO repetido e ignorado
                if (gJoin != NJ_CONNECTING) continue;
                netSelf = b.get<uint8_t>(); gJoinSeed = b.get<int64_t>(); gJoinTime = b.get<int32_t>(); gJoinCreative = b.get<uint8_t>();
                for (int i = 0; i < 3; i++) gJoinPos[i] = b.get<float>();
                gHostPlName = netGetName(b);
                netConn = true; netHostSeen = now; gJoin = NJ_WELCOMED;
                continue;
            }
            if (!netConn) continue;
            netHostSeen = now;
            if (type == NP_BYE) {
                remotes.erase(sender);
                if (sender == 0) { netConn = false; netOn = false; chatAdd("O host saiu do jogo.", 0xFF5555); }
                continue;
            }
        }
        if (sender == netSelf && type != NP_CHAT) continue;
        switch (type) {
        case NP_STATE: {
            RemotePl& r = remotes[sender];
            RState st; st.x = b.get<double>(); st.y = b.get<double>(); st.z = b.get<double>();
            st.yaw = b.get<float>(); st.pitch = b.get<float>(); st.swing = b.get<float>(); st.held = b.get<int16_t>(); st.sneak = b.get<uint8_t>() != 0;
            { const int gmb = b.get<uint8_t>(); r.gm = gmb & 3; if (!netHost && (gmb & 4)) gHard = true; }
            { const int bw = b.get<uint8_t>(); r.bow = bw ? bw - 1 : -1; }
            st.ely = b.r < b.n ? b.get<uint8_t>() : 0;   // elitra (compativel com versoes sem o campo)   // ticks de carga do arco (0 = nao carrega)   // modo do jogador remoto (espectador some) e hardcore do host
            if (!r.init) { r.init = true; r.x = r.px = st.x; r.y = r.py = st.y; r.z = r.pz = st.z; r.yaw = r.pyaw = st.yaw; r.pitch = r.ppitch = st.pitch; r.bodyYaw = r.pBodyYaw = st.yaw; }
            if (st.swing > 0.f && (!r.swinging || st.swing < r.lastRecvSwing)) { r.swinging = true; r.swing = st.swing; }   // início ou reinício do golpe
            r.lastRecvSwing = st.swing;
            r.q.push_back(st); r.seen = now;
            break; }
        case NP_BLOCKS: {
            int c = b.get<uint8_t>();
            while (c-- > 0) { const int x = b.get<int32_t>(), y = b.get<int32_t>(), z = b.get<int32_t>(), wid = b.get<int32_t>(), meta = b.get<uint8_t>(); const int id = netUnwireId(wid); if (id >= 0) netApplyEdit(x, y, z, id, meta); }
            break; }
        case NP_DROP: {
            const uint32_t nid = b.get<uint32_t>(); bool has = false;
            for (const DropEnt& d : DROPS) if (d.nid == nid) { has = true; break; }
            const double x = b.get<float>(), y = b.get<float>(), z = b.get<float>(), vx = b.get<float>(), vy = b.get<float>(), vz = b.get<float>();
            const int id = b.get<int16_t>(), cnt = b.get<int16_t>(), dl = b.get<int16_t>();
            if (!has) spawnDrop(x, y, z, vx, vy, vz, id, cnt, dl, nid);
            break; }
        case NP_DROPSET: {
            const uint32_t nid = b.get<uint32_t>(); const int cnt = b.get<int16_t>(), picker = b.get<uint8_t>();
            for (size_t i = 0; i < DROPS.size(); i++) if (DROPS[i].nid == nid) {
                if (cnt <= 0) { if (picker != 255 && picker != netSelf) PICKS.push_back({DROPS[i].x, DROPS[i].y, DROPS[i].z, DROPS[i].id, 0, picker});   // anima a coleta de outro jogador
                    DROPS[i] = DROPS.back(); DROPS.pop_back(); } else DROPS[i].n = cnt;
                break;
            }
            break; }
        case NP_ARROW: arrowNetRecv(b, sender); break;
        case NP_PVP: { const int tid = b.get<uint8_t>(), dmg = b.get<int16_t>(); const float kx = b.get<float>(), kz = b.get<float>(), kb = b.get<float>(); if (tid == netSelf) pvpReceive(sender, dmg, kx, kz, kb); break; }   // o flash vermelho so aparece quando a VITIMA confirma o dano (NP_HURT)
        case NP_HURT: { b.get<uint8_t>(); auto it = remotes.find(sender); if (it != remotes.end() && it->second.deathT < 0) it->second.hurtT = 10; break; }   // vitima levou dano de verdade (nao criativo, nao bloqueado pela invulnerabilidade)

        case NP_DEATH: {
            const int fl = b.get<uint8_t>(), l = b.get<uint8_t>(); std::string s; for (int i = 0; i < l; i++) s += (char)b.get<uint8_t>();
            RemotePl& r = remotes[sender];
            if (fl) { if (r.deathT < 0) r.deathT = 0; if (!s.empty()) chatAdd(profClean(s), 0xFFFFFF); }
            else { r.deathT = -1; r.init = false; r.q.clear(); }   // renasceu: o proximo estado reposiciona
            break; }
        case NP_BOAT: boatNetRecv(b); break;
        case NP_BOATDEL: { const int id = b.get<int32_t>(); if (BoatEnt* e = boatById(id)) if (!e->dead) { boatEject(*e); e->dead = true; } break; }
        case NP_BOATHIT: { const int id = b.get<int32_t>(); const float dmg = b.get<float>(); const bool cr = b.get<uint8_t>() != 0; if (BoatEnt* e = boatById(id)) if (!e->dead && boatAuth(*e)) boatApplyHit(*e, dmg, cr); break; }
        case NP_CHEST: chestNetRecv(b); break;
        case NP_CHESTREQ: chestNetReq(b); break;
        case NP_ENDER: enderNetRecv(b, sender); break;
        case NP_ENDERREQ: enderNetReq(sender); break;
        case NP_MON: monNetRecv(b); break;
        case NP_MONF: monFrameRecv(b); break;   // fragmento JPEG da pagina de um monitor
        case NP_CHAT: {
            const int l = b.get<uint8_t>(); std::string s; for (int i = 0; i < l; i++) s += (char)b.get<uint8_t>();
            chatAdd(s); break; }
        }
    }
}
static void netTick() {   // 1x por tick do jogo (20/s): consome 1 estado de cada remoto e roda a lógica de EntityLivingBase.onUpdate
    for (auto& kv : remotes) {
        RemotePl& r = kv.second; if (!r.init) continue;
        r.px = r.x; r.py = r.y; r.pz = r.z; r.pyaw = r.yaw; r.ppitch = r.pitch; r.pBodyYaw = r.bodyYaw; r.pLimbAmt = r.limbAmt;
        while (r.q.size() > 2) r.q.pop_front();   // buffer curto: sem acumular atraso
        if (!r.q.empty()) {
            const RState s = r.q.front(); r.q.pop_front();
            r.x = s.x; r.y = s.y; r.z = s.z; r.yaw = r.pyaw + wrap180(s.yaw - r.pyaw); r.pitch = s.pitch; r.held = s.held; r.sneak = s.sneak; r.ely = s.ely;
        }
        if (r.ely & 2) r.elyTicks += 1.f; else r.elyTicks = 0.f;
        r.ageTicks++; if (r.hurtT > 0) r.hurtT--; if (r.deathT >= 0 && r.deathT < 100) r.deathT++;
        const double d0 = r.x - r.px, d1 = r.z - r.pz; const float dsq = (float)(d0 * d0 + d1 * d1);
        float f1 = r.bodyYaw;
        if (dsq > .0025000002f) f1 = (float)(atan2(-d0, -d1) * 57.29577951308232);
        if (r.swinging) f1 = r.yaw;
        r.bodyYaw += wrap180(f1 - r.bodyYaw) * .3f;
        float h = wrap180(r.yaw - r.bodyYaw); if (h < -75.f) h = -75.f; if (h >= 75.f) h = 75.f;
        r.bodyYaw = r.yaw - h; if (h * h > 2500.f) r.bodyYaw += h * .2f;
        while (r.bodyYaw - r.pBodyYaw < -180.f) r.pBodyYaw -= 360.f;
        while (r.bodyYaw - r.pBodyYaw >= 180.f) r.pBodyYaw += 360.f;
        float f6 = (float)sqrt(d0 * d0 + d1 * d1) * 4.f; if (f6 > 1.f) f6 = 1.f;
        r.limbAmt += (f6 - r.limbAmt) * .4f; r.limbSwing += r.limbAmt;
    }
}
static void netSendState(double x, double y, double z, float yaw, float pitch, int held, float swing, bool sneak, int ely = 0) {   // 1x por tick
    gNetPos[0] = x; gNetPos[1] = y; gNetPos[2] = z;   // o host informa a posicao a quem entra
    if (!netOn) return;
    NBuf b = netMk(NP_STATE); b.put<double>(x); b.put<double>(y); b.put<double>(z); b.put<float>(yaw); b.put<float>(pitch); b.put<float>(swing);
    b.put<int16_t>((int16_t)held); b.put<uint8_t>(sneak ? 1 : 0); b.put<uint8_t>((uint8_t)(gGm | (gHard ? 4 : 0))); b.put<uint8_t>(gBowOn && gBowT >= 0 ? (uint8_t)(std::min(gBowT, 254) + 1) : 0); b.put<uint8_t>((uint8_t)ely); netBroadcast(b);
}
static void netUpdate(float dt) {
    const Uint32 now = SDL_GetTicks();
    lanTick(now);   // descoberta de servidores (aba Multiplayer) e resposta do host
    if (netSock < 0) return;
    netPoll(now);
    if (!netHost && (gJoin == NJ_CONNECTING || gJoin == NJ_WELCOMED || gJoin == NJ_INGAME) && now - netLastHello > (gGuestReady ? 1000u : 500u)) {   // HELLO ate o WELCOME; depois mantem a conexao viva durante o loading e pede a sincronizacao ("pronto")
        netLastHello = now; NBuf h = netMk(NP_HELLO); h.put<uint8_t>(NET_VER); h.put<uint8_t>(gGuestReady ? 1 : 0); netPutName(h); netTo(netHostAddr, h);
    }
    if (gJoin == NJ_CONNECTING && now - gJoinT0 > 7000) { gJoin = NJ_FAILED; gJoinMsg = "Sem resposta do servidor. Confira o IP, a porta (47777 UDP) e o firewall."; }
    if (netHost) {
        for (size_t i = 0; i < peers.size();) {   // timeout de quem sumiu
            if (now - peers[i].seen < 6000) { ++i; continue; }
            const int id = peers[i].id; const std::string who = netLabel(id); peers.erase(peers.begin() + i); remotes.erase(id);
            NBuf by; by.put<uint8_t>(NP_BYE); by.put<uint8_t>((uint8_t)id); netBroadcast(by);
            netSay(who + " perdeu a conexão");
        }
        for (auto& p : peers) for (int k = 0; k < 8 && !p.q.empty(); k++) { netTo(p.a, p.q.front()); p.q.pop_front(); }
    } else if (netConn && now - netHostSeen > 8000) { netConn = false; chatAdd("Conexão com o host perdida.", 0xFF5555); }
    netOn = netHost ? !peers.empty() : netConn;
    if (now - netLastRec > 1000) {   // aplica edições que chegaram antes do chunk existir
        netLastRec = now;
        for (auto it = netPend.begin(); it != netPend.end();) {
            int bx, by, bz; unk3(it->first, bx, by, bz);
            if (((by >> 10) & 3) == gDim && getC(fdiv(bx, CW), fdiv(bz, CW))) { const int id = netEditId(it->second), meta = netEditMeta(it->second); it = netPend.erase(it); netApplyEdit(bx, by, bz, id, meta); } else ++it;
        }
    }
    for (auto it = remotes.begin(); it != remotes.end();) {
        RemotePl& r = it->second;
        if (r.seen && now - r.seen > 8000) { it = remotes.erase(it); continue; }
        if (r.swinging) { r.swing += dt / .3f; if (r.swing >= 1.f) { r.swing = 0; r.swinging = false; } }   // balanço contínuo por frame, igual ao local
        ++it;
    }
}
static void netDraw(float pt) {
    for (auto& kv : remotes) {
        const RemotePl& r = kv.second; if (!r.init || (r.gm == 3 && gGm != 3)) continue;
        const float bodyY = r.pBodyYaw + (r.bodyYaw - r.pBodyYaw) * pt, headY = r.pyaw + (r.yaw - r.pyaw) * pt;
        Pose p; p.headYaw = wrap180(headY - bodyY); p.headPitch = r.ppitch + (r.pitch - r.ppitch) * pt;
        p.limb = r.limbSwing - r.limbAmt * (1.f - pt); p.amp = std::min(1.f, r.pLimbAmt + (r.limbAmt - r.pLimbAmt) * pt);
        p.atk = r.swinging ? r.swing : 0.f; p.held = r.held; p.bow = r.bow >= 0 ? (float)r.bow : -1.f; p.age = r.ageTicks + pt; p.sneak = r.sneak; p.hurt = (r.hurtT > 0 || r.deathT >= 0) ? 1.f : 0.f;
        p.elySm = const_cast<ElySm*>(&r.elySm); p.elyWorn = (r.ely & 1) && r.gm != 3; p.elyFly = p.elyWorn && (r.ely & 2);
        if (p.elyFly) { p.elyTicks = r.elyTicks + pt; p.elyMot[0] = (float)(r.x - r.px); p.elyMot[1] = (float)(r.y - r.py); p.elyMot[2] = (float)(r.z - r.pz);
            const float yr = (r.pyaw + (r.yaw - r.pyaw) * pt) * .0174533f, pr = (r.ppitch + (r.pitch - r.ppitch) * pt) * .0174533f;
            p.elyLook[0] = -sinf(yr) * cosf(pr); p.elyLook[1] = sinf(pr); p.elyLook[2] = -cosf(yr) * cosf(pr); }
        if (const BoatEnt* rb = boatOfRider(kv.first)) {   // remoto montado: preso ao assento do bote (balanço/inclinação do casco), sentado, remando no ritmo dos remos
            const float by = boatYawLerp(*rb, pt);
            Pose q; q.headYaw = std::max(-105.f, std::min(105.f, wrap180(headY - by))); q.headPitch = p.headPitch; q.sit = true; q.held = r.held; q.age = r.ageTicks + pt; q.hurt = r.hurtT > 0 ? 1.f : 0.f;
            q.atk = r.swinging ? r.swing : 0.f;
            boatRowPose(q, *rb, pt);
            glPushMatrix(); boatRiderBegin(*rb, pt); glRotatef(by, 0, 1, 0);
            entLightAt(rb->x, rb->y + 1.0, rb->z, gEntDim); drawPlayer(q); entDimReset();
            glPopMatrix(); continue;
        }
        if (r.deathT >= 20) continue;   // 1.8.9: o corpo some 20 ticks depois da morte
        glPushMatrix(); glTranslatef((float)(r.px + (r.x - r.px) * pt), (float)(r.py + (r.y - r.py) * pt), (float)(r.pz + (r.z - r.pz) * pt)); glRotatef(bodyY, 0, 1, 0);
        entLightAt(r.x, r.y + 1.0, r.z, gEntDim);
        if (r.deathT >= 0) {   // RendererLivingEntity.rotateCorpse: tomba ate 90 graus (sqrt((t-1)/20*1.6)) e fica avermelhado
            float f = sqrtf(std::max(0.f, (r.deathT + pt - 1.f) / 20.f * 1.6f)); if (f > 1.f) f = 1.f;
            glRotatef(-f * 90.f, 0, 0, 1); gEntDim[1] *= .4f; gEntDim[2] *= .4f;
        }
        drawPlayer(p);
        entDimReset();
        glPopMatrix();
    }
}
static const char* netInit(int argc, char** argv, SDL_Window* win) {   // a rede NAO abre sozinha: so por --join/--host ou pelo menu
    const char* skin = "skin.png"; bool skinSet = false; netWin = win;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if (a == "--join" && i + 1 < argc) gCliJoin = argv[++i];
        else if (a == "--host") gCliHost = true;
        else if (a == "--port" && i + 1 < argc) gNetPort = atoi(argv[++i]);
        else if (a.rfind("--", 0) != 0 && !skinSet) { skin = argv[i]; skinSet = true; }
    }
    return skin;
}
static void netClose() { netStop(); }
