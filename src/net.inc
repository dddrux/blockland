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
enum { NP_HELLO = 1, NP_WELCOME, NP_STATE, NP_BLOCKS, NP_DROP, NP_DROPSET, NP_BYE, NP_CHAT, NP_KICK, NP_MON, NP_MONF, NP_CHEST, NP_CHESTREQ, NP_BOAT, NP_BOATDEL, NP_BOATHIT, NP_ENDER, NP_ENDERREQ, NP_PVP, NP_DEATH, NP_HURT, NP_ARROW, NP_PRIV, NP_TPTO, NP_GIVE, NP_ROSTER, NP_NICKREQ, NP_SYS, NP_SKIN, NP_PING, NP_PONG, NP_MOBS, NP_MOBHIT, NP_MOBDMG, NP_EGG, NP_EGGHATCH, NP_NBT, NP_CHUNKREQ, NP_CHUNK };
static const int NET_VER = 23, NET_DISC_PORT = 47778, NET_MAXP = 8;   // NET_VER 23: NP_CHUNKREQ/NP_CHUNK (o convidado pede ao host cada chunk do SAVE do host - mapa importado, edicoes de sessoes antigas - em fragmentos de 1200 bytes; sem isso o convidado gerava o terreno so pela seed e via outro mundo). 22: NP_NBT (texto do NBT de uma pilha, em fragmentos de ate 1000 bytes: hash, tamanho, deslocamento) e campo opcional no FIM de NP_CHEST/NP_ENDER/NP_DROP com o hash do NBT de cada pilha. 21: NP_ARROW tipo 3 (flecha do dono cravou: ponto + direcao, a copia remota cola igual). 20: NP_ARROW tipo 2 (flecha cravada recolhida: some nos outros) e flags no byte final do tipo 0 (bit1 recolhivel, bit2 so criativo). 19: NP_EGGHATCH (convidado -> host: pede galinha(s) do ovo). 18: NP_EGG (ovo arremessado, so visual nos outros). 16: NP_MOBS (host -> todos: estado dos mobs), NP_MOBHIT (convidado -> host: golpe em mob), NP_MOBDMG (host -> jogador: dano de mob). 15: NP_PING/NP_PONG (host mede o ping de cada convidado) e NP_ROSTER leva o ping (uint16, ms) depois de cada nick. 14: NP_SKIN (pixels da skin 64x64 em 16 pacotes: dono, indice, 1024 bytes). 13: WELCOME leva o nome final do convidado; NP_ROSTER (host -> todos: id+nick), NP_NICKREQ (/nick de convidado), NP_SYS (erro do host). 12: NP_PRIV/NP_TPTO/NP_GIVE (tell, puxar, give por nick). 11: HELLO leva o nome do jogador e WELCOME o do host
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
struct NetPeer { sockaddr_in a; Uint32 seen; int id; std::deque<NBuf> q; bool synced = false; std::string name; int ping = -1; };
static std::map<int, std::string> gNetNames; static std::map<int, int> gNetPing;   // id -> ping em ms (convidado: vem no NP_ROSTER; host: medido com PING/PONG)
static int netPingOf(int id);   // abaixo   // convidado: id -> nick (lista que o host manda em NP_ROSTER)
static std::string gNetNickOv, gNickPending;   // nick atribuido pelo host nesta sessao (nao vai pro profile.cfg) / /nick pedido e ainda nao confirmado
static std::string gHostPlName;   // nome do host (vem no WELCOME)
struct RState { double x, y, z; float yaw, pitch, swing; int held; bool sneak; uint8_t ely = 0; };   // ely: bit0 = veste elitra, bit1 = planando
// Jogador remoto: simulado a cada tick (20/s) com a MESMA lógica do jogador local (corpo, membros, balanço) e interpolado com partialTicks.
struct RemotePl {
    double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0; float yaw = 0, pyaw = 0, pitch = 0, ppitch = 0;
    float bodyYaw = 0, pBodyYaw = 0, limbSwing = 0, limbAmt = 0, pLimbAmt = 0, ageTicks = 0, swing = 0, lastRecvSwing = 0;
    int gm = 0, bow = -1, held = 0, deathT = -1, hurtT = 0; bool sneak = false, swinging = false, init = false; Uint32 seen = 0; std::deque<RState> q;
    uint8_t ely = 0; float elyTicks = 0; ElySm elySm;
    int dim = 0;   // dimensao em que o jogador remoto esta (0 superficie, 1 Nether, 2 End); quem esta em outra dimensao nao e desenhado nem alvejavel
};
static void mobSyncRecv(NBuf& b); static void mobSyncHitRecv(NBuf& b, int sender); static void mobDmgRecv(NBuf& b);   // mobsync.inc
static void arrowNetRecv(NBuf& b, int sender);   // bow.inc
static void eggNetRecv(NBuf& b, int sender);   // egg.inc
static void eggHatchRecv(NBuf& b, int sender);   // egg.inc
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
static std::string netName() { if (!gNetNickOv.empty()) return gNetNickOv; if (!gPlayerName.empty()) return gPlayerName; return netSock < 0 ? "Jogador" : "Jogador " + std::to_string(netSelf + 1); }
static void netPutName(NBuf& b) { const std::string n = netName().substr(0, 40); b.put<uint8_t>((uint8_t)n.size()); for (char ch : n) b.put<uint8_t>((uint8_t)ch); }
static std::string netGetName(NBuf& b) { const int l = b.get<uint8_t>(); std::string s; for (int i = 0; i < l; i++) s += (char)b.get<uint8_t>(); return profClean(s); }
static std::string netLabel(int id) {   // nome de um jogador pelo id de rede (0 = host)
    if (id == netSelf) return netName(); { auto it = gNetNames.find(id); if (it != gNetNames.end() && !it->second.empty()) return it->second; }
    if (id == 0 && !gHostPlName.empty()) return gHostPlName;
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
static int netPingOf(int id) {   // ping em ms de um jogador; -1 = desconhecido. Host = 0 para si mesmo
    if (id == 0 && netHost) return 0;
    if (netHost) { for (const NetPeer& p : peers) if (p.id == id) return p.ping; return -1; }
    auto it = gNetPing.find(id); return it == gNetPing.end() ? -1 : it->second;
}
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

// ---- NBT de pilha na rede: o texto vai UMA vez em NP_NBT (fragmentos) e os pacotes de bau/drop levam so o hash (u32). Cache hash -> texto nos dois lados.
// Hash desconhecido (pacote perdido) vira "?<hash>": a pilha guarda o marcador, e ao reenviar o hash o outro lado (que tem o texto) reconhece.
static std::map<uint32_t, std::string> gNbtCache;
struct NbtAsm { std::string buf; std::vector<bool> got; };
static std::map<uint32_t, NbtAsm> gNbtAsm;
static uint32_t nbtHash(const std::string& s) {   // 0 = sem NBT
    if (s.empty()) return 0;
    if (s[0] == '?') return (uint32_t)strtoul(s.c_str() + 1, nullptr, 10);
    uint32_t h = 2166136261u; for (unsigned char c : s) { h ^= c; h *= 16777619u; } return h ? h : 1u;
}
static std::string nbtResolve(uint32_t h) {
    if (!h) return std::string();
    auto it = gNbtCache.find(h); return it != gNbtCache.end() ? it->second : "?" + std::to_string(h);
}
static const int NBT_FRAG = 1000;
static std::vector<NBuf> nbtFragments(const std::string& s) {   // NP_NBT: hash, tamanho total, deslocamento, tamanho do pedaco, bytes
    std::vector<NBuf> o; const uint32_t h = nbtHash(s);
    for (size_t off = 0; off < s.size(); off += NBT_FRAG) {
        const size_t len = std::min((size_t)NBT_FRAG, s.size() - off);
        NBuf b = netMk(NP_NBT); b.put<uint32_t>(h); b.put<uint16_t>((uint16_t)s.size()); b.put<uint16_t>((uint16_t)off); b.put<uint16_t>((uint16_t)len);
        for (size_t i = 0; i < len; i++) b.put<uint8_t>((uint8_t)s[off + i]);
        o.push_back(b);
    }
    return o;
}
static void netSendNbt(const std::string& s) {   // manda o texto (se for real e couber) antes do pacote que o referencia
    if (!netOn || s.empty() || s[0] == '?' || s.size() > 60000) return;
    gNbtCache[nbtHash(s)] = s;
    for (const NBuf& b : nbtFragments(s)) netBroadcast(b);
}
static void nbtNetRecv(NBuf& b) {
    const uint32_t h = b.get<uint32_t>(); const int tot = b.get<uint16_t>(), off = b.get<uint16_t>(), len = b.get<uint16_t>();
    if (!h || tot <= 0 || off < 0 || len <= 0 || off + len > tot || b.n - b.r < len || off % NBT_FRAG != 0 || gNbtCache.count(h)) return;
    NbtAsm& a = gNbtAsm[h]; if ((int)a.buf.size() != tot) { a.buf.assign(tot, 0); a.got.assign((tot + NBT_FRAG - 1) / NBT_FRAG, false); }
    for (int i = 0; i < len; i++) a.buf[off + i] = (char)b.get<uint8_t>();
    a.got[off / NBT_FRAG] = true;
    for (bool g : a.got) if (!g) return;
    if (nbtHash(a.buf) == h) gNbtCache[h] = a.buf;   // so aceita se o texto confere com o hash
    gNbtAsm.erase(h);
}
static NBuf netDropPacket(const DropEnt& d) {
    NBuf b = netMk(NP_DROP); b.put<uint32_t>(d.nid);
    b.put<float>((float)d.x); b.put<float>((float)d.y); b.put<float>((float)d.z); b.put<float>((float)d.vx); b.put<float>((float)d.vy); b.put<float>((float)d.vz);
    b.put<int16_t>((int16_t)d.id); b.put<int16_t>((int16_t)d.n); b.put<int16_t>((int16_t)d.delay);
    b.put<uint32_t>(nbtHash(d.nbt));   // campo opcional no fim: hash do NBT da pilha (0 = nenhum)
    return b;
}
static bool netRemotePos(int id, float pt, double* o) {
    auto it = remotes.find(id); if (it == remotes.end() || !it->second.init) return false; const RemotePl& r = it->second;
    o[0] = r.px + (r.x - r.px) * pt; o[1] = r.py + (r.y - r.py) * pt; o[2] = r.pz + (r.z - r.pz) * pt; return true;
}
static void netSendDrop(const DropEnt& d) { if (netOn) { netSendNbt(d.nbt); netBroadcast(netDropPacket(d)); } }
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
// ---- sincronizacao de nick: o host manda a lista (id -> nick) a todos; convidado pede troca com NP_NICKREQ ----
static void netPutStr(NBuf& b, const std::string& t) { const int l = (int)std::min<size_t>(t.size(), 40); b.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)t[i]); }
// ---- times (como o /team do Minecraft): o host decide quem esta em qual time e manda tudo no NP_ROSTER ----
static const char* const TEAM_COLOR[16] = {"black", "dark_blue", "dark_green", "dark_aqua", "dark_red", "dark_purple", "gold", "gray", "dark_gray", "blue", "green", "aqua", "red", "light_purple", "yellow", "white"};
static const unsigned TEAM_RGB[16] = {0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xFFAA00, 0xAAAAAA, 0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF};
struct TeamDef { std::string name; int col; };
static std::vector<TeamDef> gTeams;                     // times existentes (convidado: copia do host)
static std::map<int, std::string> gTeamOfId;            // id de rede -> nome do time (convidado: copia do host)
static std::map<std::string, std::string> gTeamOfNick;  // so host/solo: nick (minusculo) -> time, como o scoreboard (quem troca de nick sai do time)
static std::string teamKey(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
static int teamFind(const std::string& n) { for (size_t i = 0; i < gTeams.size(); i++) if (nickEq(gTeams[i].name, n)) return (int)i; return -1; }
static std::string teamStrOf(int id) { auto it = gTeamOfId.find(id); return it == gTeamOfId.end() ? std::string() : it->second; }
static int teamIndexOf(int id) { const std::string t = teamStrOf(id); return t.empty() ? -1 : teamFind(t); }
static unsigned teamRgbOf(int id) { const int t = teamIndexOf(id); return t < 0 ? 0xFFFFFFu : TEAM_RGB[gTeams[t].col & 15]; }
static bool teamSame(int a, int b) { const int x = teamIndexOf(a); return x >= 0 && x == teamIndexOf(b); }
static void teamRebuild() {   // host/solo: recalcula "id -> time" a partir dos nicks de agora
    gTeamOfId.clear();
    auto put = [&](int id, const std::string& nm) { auto it = gTeamOfNick.find(teamKey(nm)); if (it != gTeamOfNick.end() && teamFind(it->second) >= 0) gTeamOfId[id] = it->second; };
    put(netHost ? 0 : netSelf, netName()); if (netHost) for (const NetPeer& p : peers) put(p.id, p.name.empty() ? "Jogador " + std::to_string(p.id + 1) : p.name);
}
static void netRosterSend() {   // so o host
    if (!netHost || !netOn) return;
    teamRebuild();
    NBuf b = netMk(NP_ROSTER); b.put<uint8_t>((uint8_t)(peers.size() + 1));
    b.put<uint8_t>(0); netPutStr(b, netName()); b.put<uint16_t>(0); netPutStr(b, teamStrOf(0));
    for (const NetPeer& p : peers) { b.put<uint8_t>((uint8_t)p.id); netPutStr(b, p.name.empty() ? "Jogador " + std::to_string(p.id + 1) : p.name); b.put<uint16_t>((uint16_t)std::max(0, std::min(p.ping, 9999))); netPutStr(b, teamStrOf(p.id)); }
    b.put<uint8_t>((uint8_t)gTeams.size()); for (const TeamDef& t : gTeams) { netPutStr(b, t.name); b.put<uint8_t>((uint8_t)(t.col & 15)); }
    netBroadcast(b);
}
static void netSysTo(int id, const std::string& t) { NBuf b = netMk(NP_SYS); b.put<uint8_t>((uint8_t)id); netPutStr(b, t); netBroadcast(b); }
static void netNameRecv(int type, int sender, NBuf& b) {
    if (type == NP_ROSTER) {
        if (netHost || sender != 0) return;
        gNetNames.clear(); gNetPing.clear(); gTeamOfId.clear(); gTeams.clear(); const int n = b.get<uint8_t>();
        for (int i = 0; i < n; i++) { const int id = b.get<uint8_t>(); const std::string nm = netGetName(b); gNetNames[id] = nm; gNetPing[id] = b.get<uint16_t>(); { const std::string tn = netGetName(b); if (!tn.empty()) gTeamOfId[id] = tn; }
            if (id == netSelf && !nm.empty()) { if (!gNickPending.empty() && nickEq(nm, gNickPending)) { gPlayerName = nm; profileSave(); } gNickPending.clear(); gNetNickOv = nm; } }
        { const int nt = b.get<uint8_t>(); for (int i = 0; i < nt && i < 16; i++) { TeamDef d; d.name = netGetName(b); d.col = b.get<uint8_t>() & 15; gTeams.push_back(d); } }
    } else if (type == NP_NICKREQ) {
        if (!netHost) return;
        NetPeer* q = nullptr; for (auto& p : peers) if (p.id == sender) q = &p; if (!q) return;
        const std::string nm = netGetName(b);
        if (!nickValid(nm)) { netSysTo(sender, "Nick inválido: use de 1 a 16 caracteres entre A-Z, a-z, 0-9 e _."); return; }
        if (!nickEq(nm, q->name) && netNickTaken(nm, q)) { netSysTo(sender, "Já existe um jogador com o nick \"" + nm + "\" neste servidor."); return; }
        const std::string old = q->name; q->name = nm; netSay(old + " agora se chama " + nm); netRosterSend();
    } else if (type == NP_SYS) {
        const int target = b.get<uint8_t>(); const std::string t = netGetName(b);
        if (target == netSelf && sender == 0) { gNickPending.clear(); chatAdd(t, 0xFF5555); }
    }
}
// ---- sincronizacao de skin: cada jogador manda os PIXELS da propria skin (64x64 RGBA = 16 KB em 16 pacotes); o host guarda e repassa ----
static const int SKIN_BYTES = 64 * 64 * 4, SKIN_CH = 1024, SKIN_NCH = SKIN_BYTES / SKIN_CH;
struct SkinSlot { std::vector<uint8_t> px, asmb; unsigned got = 0; bool dirty = false; GLuint tex = 0; };
static std::map<int, SkinSlot> gSkins;   // id de rede -> skin recebida (o host tambem guarda para repassar a quem entra depois)
static Uint32 gSkinT = 0;                // ultima vez que mandamos skins (0 = nunca nesta sessao)
static void netSkinSend(int owner, const uint8_t* px, const sockaddr_in* to) {
    for (int i = 0; i < SKIN_NCH; i++) {
        NBuf b = netMk(NP_SKIN); b.put<uint8_t>((uint8_t)owner); b.put<uint8_t>((uint8_t)i);
        for (int k = 0; k < SKIN_CH; k++) b.put<uint8_t>(px[i * SKIN_CH + k]);
        if (to) netTo(*to, b); else netBroadcast(b);
    }
}
static void netSkinSendAll(const sockaddr_in* to) {   // host: a skin dele + a de cada convidado (quem entra agora ainda nao tem nenhuma)
    netSkinSend(netSelf, skin, to);
    for (auto& kv : gSkins) if (kv.second.px.size() == (size_t)SKIN_BYTES && kv.first != netSelf) netSkinSend(kv.first, kv.second.px.data(), to);
}
static void netSkinForget(int id) { auto it = gSkins.find(id); if (it == gSkins.end()) return; if (it->second.tex) glDeleteTextures(1, &it->second.tex); gSkins.erase(it); }
static void netSkinRecv(int sender, NBuf& b) {
    const int owner = b.get<uint8_t>(), idx = b.get<uint8_t>();
    if (idx >= SKIN_NCH || owner == netSelf || (owner != sender && sender != 0) || b.n - b.r < SKIN_CH) return;   // so o dono (ou o host, repassando) manda a skin de alguem
    SkinSlot& sl = gSkins[owner]; if (sl.asmb.empty()) sl.asmb.assign(SKIN_BYTES, 0);
    memcpy(sl.asmb.data() + idx * SKIN_CH, b.d + b.r, SKIN_CH); sl.got |= 1u << idx;
    if (sl.got != (1u << SKIN_NCH) - 1) return;
    sl.got = 0; if (sl.px != sl.asmb) { sl.px = sl.asmb; sl.dirty = true; }   // so recria a textura se a skin mudou
}
// ---- nick do jogador 2 (split): proprio, valido, nunca igual ao do jogador 1 nem ao de quem esta na rede ----
static std::string p2Nick() {
    auto taken = [](const std::string& c) { return nickEq(c, netName()) || (netOn && netNickTaken(c, nullptr)); };
    std::string n = nickValid(gP2Name) ? gP2Name : std::string("Jogador_2");
    return taken(n) ? nickSuggest(n, taken) : n;
}
static std::string cmdNick2Change(const std::string& arg) {   // "" = trocou; senao o erro
    if (!nickValid(arg)) return "Nick inválido: use de 1 a 16 caracteres entre A-Z, a-z, 0-9 e _.";
    if (nickEq(arg, netName()) || (netOn && netNickTaken(arg, nullptr))) return "Já existe um jogador com o nick \"" + arg + "\".";
    gP2Name = arg; profileSave(); return "";
}
static bool netChatOk(int senderId, const std::string& s) {   // host: o convidado so pode falar com o proprio nick (impede forjar "<Outro> msg")
    const std::string L = netLabel(senderId); bool ok = false;
    for (const std::string& pre : {"<" + L + "> ", "* " + L + " ", "[" + L + "] "}) if (s.size() > pre.size() && s.compare(0, pre.size(), pre) == 0) ok = true;   // fala, /me e /say
    if (!ok) return false;
    for (unsigned char c : s) if (c < 32 || c == 127) return false; return true;
}
// ---- /list e /nick (chat_debug.inc) ----
static std::vector<std::string> cmdPlayerList() {   // jogadores online: eu + host/convidados conhecidos (convidado so conhece os nomes que ja chegaram)
    std::vector<std::string> v; v.push_back(netName()); if (gSplit) v.push_back(p2Nick());
    if (netOn) {
        if (!netHost) v.push_back(netLabel(0));
        for (const NetPeer& p : peers) v.push_back(p.name.empty() ? netLabel(p.id) : p.name);
        if (!netHost) for (auto& kv : gNetNames) if (kv.first != 0 && kv.first != netSelf) v.push_back(kv.second);
        if (!netHost) for (auto& kv : remotes) if (kv.first != 0 && kv.first != netSelf) v.push_back(netLabel(kv.first));
    }
    std::vector<std::string> u; for (auto& n : v) { bool dup = false; for (auto& m : u) if (nickEq(m, n)) dup = true; if (!dup) u.push_back(n); }
    return u;
}
static std::string cmdNickChange(const std::string& arg) {   // "" = trocou; "~..." = informacao (pedido enviado); senao a mensagem de erro
    if (!nickValid(arg)) return "Nick inválido: use de 1 a 16 caracteres entre A-Z, a-z, 0-9 e _.";
    if (gSplit && nickEq(arg, p2Nick())) return "Esse nick já é do jogador 2.";
    if (netOn && !netHost) { gNickPending = arg; NBuf b = netMk(NP_NICKREQ); netPutStr(b, arg); netBroadcast(b); return "~Pedindo ao host para trocar seu nick para " + arg + "..."; }
    if (netOn && !nickEq(arg, netName()) && netNickTaken(arg, nullptr)) return "Já existe um jogador com o nick \"" + arg + "\" neste servidor.";
    const std::string old = netName(); gPlayerName = arg; profileSave();
    if (netOn) { netSay(old + " agora se chama " + arg); netRosterSend(); }
    return "";
}
// ---- comandos por nick: /tell /msg /w /r, /tp <nick>, /puxar, /give <nick> (chat_debug.inc) ----
struct NetTpReq { bool on = false; double x = 0, y = 0, z = 0; };
static NetTpReq gNetTp;      // teletransporte pedido pelo host: o loop principal aplica (tem a Phys)
static int gLastTell = -1;   // id de quem falou comigo por ultimo (/r)
static int netFindNick(const std::string& n) {   // id de rede pelo nick (sem diferenciar caixa); -1 se nao existe
    if (nickEq(n, netName())) return netSelf;
    if (!netOn) return -1;
    if (!netHost && nickEq(n, netLabel(0))) return 0;
    for (const NetPeer& p : peers) if (nickEq(p.name.empty() ? netLabel(p.id) : p.name, n)) return p.id;
    if (!netHost) for (auto& kv : gNetNames) if (kv.first != netSelf && nickEq(kv.second, n)) return kv.first;
    if (!netHost) for (auto& kv : remotes) if (kv.first != netSelf && nickEq(netLabel(kv.first), n)) return kv.first;
    return -1;
}
static bool cmdIsPlayer(const std::string& n) { return netFindNick(n) >= 0; }
static bool cmdIsSelf(const std::string& n) { return nickEq(n, netName()); }
static bool cmdPlayerPos(const std::string& n, double* xyz, std::string& err) {   // posicao de OUTRO jogador (remotes e sincronizado em todos os clientes)
    const int id = netFindNick(n);
    if (id < 0) { err = "Jogador não encontrado: " + n; return false; }
    if (id == netSelf) { err = "Esse é você mesmo."; return false; }
    auto it = remotes.find(id);
    if (it == remotes.end() || !it->second.init) { err = netLabel(id) + " ainda não carregou no mundo."; return false; }
    xyz[0] = it->second.x; xyz[1] = it->second.y; xyz[2] = it->second.z; return true;
}
static std::string netPrivSend(const std::string& n, const std::string& msg) {
    const int id = netFindNick(n);
    if (id < 0) return "Jogador não encontrado: " + n;
    if (id == netSelf) return "Você não pode enviar mensagem para si mesmo.";
    NBuf b = netMk(NP_PRIV); b.put<uint8_t>((uint8_t)id); const int l = (int)std::min<size_t>(msg.size(), 250); b.put<uint8_t>((uint8_t)l);
    for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)msg[i]);
    netBroadcast(b); gLastTell = id;
    chatAdd("Você sussurra para " + netLabel(id) + ": " + msg, 0xAAAAAA); return "";
}
static std::string cmdTell(const std::string& n, const std::string& msg) { return netPrivSend(n, msg); }
static std::string cmdReply(const std::string& msg) {
    if (gLastTell < 0 || (gLastTell != 0 && !netOn)) return "Ninguém falou com você ainda (use /tell <nick> <mensagem>).";
    if (netLabel(gLastTell).empty()) return "Jogador não encontrado."; return netPrivSend(netLabel(gLastTell), msg);
}
static std::string cmdSendTp(const std::string& n, double x, double y, double z) {   // so o host move os outros (sem sistema de op)
    const int id = netFindNick(n);
    if (id < 0) return "Jogador não encontrado: " + n;
    if (netOn && !netHost) return "Só o host pode teletransportar outros jogadores.";
    NBuf b = netMk(NP_TPTO); b.put<uint8_t>((uint8_t)id); b.put<double>(x); b.put<double>(y); b.put<double>(z); netBroadcast(b); return "";
}
static std::string cmdSendGive(const std::string& n, int item, int cnt) {
    const int id = netFindNick(n);
    if (id < 0) return "Jogador não encontrado: " + n;
    if (netOn && !netHost) return "Só o host pode dar itens a outros jogadores.";
    NBuf b = netMk(NP_GIVE); b.put<uint8_t>((uint8_t)id); b.put<int32_t>(item); b.put<int32_t>(cnt); netBroadcast(b); return "";
}
static void netCmdRecv(int type, int sender, NBuf& b) {   // NP_PRIV / NP_TPTO / NP_GIVE: so age quem e o alvo; TPTO e GIVE so valem vindos do host (id 0)
    const int target = b.get<uint8_t>(); if (target != netSelf) return;
    if (type == NP_PRIV) {
        const int l = b.get<uint8_t>(); std::string t; for (int i = 0; i < l; i++) t += (char)b.get<uint8_t>();
        gLastTell = sender; chatAdd(netLabel(sender) + " sussurra para você: " + t, 0xAAAAAA);
    } else if (sender != 0) return;
    else if (type == NP_TPTO) {
        gNetTp.x = b.get<double>(); gNetTp.y = b.get<double>(); gNetTp.z = b.get<double>(); gNetTp.on = true;
        chatAdd("Você foi teletransportado por " + netLabel(sender));
    } else if (type == NP_GIVE) {
        const int item = b.get<int32_t>(), cnt = std::max(1, std::min((int)b.get<int32_t>(), 2304));
        if (item > 0) { const int left = addItem(item, cnt); chatAdd("Você recebeu " + std::to_string(cnt - left) + "x " + itemName(item) + " de " + netLabel(sender)); }
    }
}
static NetPeer* netAddPeer(const sockaddr_in& from, Uint32 now, const std::string& name = "") {
    int id = 1; for (bool used = true; used;) { used = false; for (auto& p : peers) if (p.id == id) { used = true; id++; } }
    peers.push_back({from, now, id, {}, false, name, -1});
    netSay((name.empty() ? "Jogador " + std::to_string(id + 1) : name) + " entrou no jogo");
    return &peers.back();
}

// ---- v23: chunks do save do host para o convidado (sem isso o convidado so gera pela seed e nao ve o mapa salvo) ----
struct ChRx { std::vector<std::vector<uint8_t>> fr; std::vector<bool> got; int total = 0, have = 0; Uint32 t = 0, t0 = 0; };
static std::unordered_map<int64_t, ChRx> gChRx; static std::unordered_set<int64_t> gChOK;
static void netChunkServe(const sockaddr_in& to, int cx, int cz) {   // host: manda o chunk salvo (ou "nada" = gere pela seed)
    const int64_t k = key(cx, cz); auto sv = savedC.find(k); std::vector<uint8_t> blob;
    auto p32 = [&](uint32_t v) { for (int i = 0; i < 4; i++) blob.push_back((uint8_t)(v >> (8 * i))); };
    auto p16 = [&](int v) { blob.push_back((uint8_t)(v & 255)); blob.push_back((uint8_t)(v >> 8)); };
    if (gDim == 0 && sv != savedC.end()) {
        const size_t N = sv->second.size(); const BT* a = sv->second.data(); std::vector<std::pair<int, int32_t>> r;
        for (size_t i = 0; i < N;) { size_t j = i + 1; while (j < N && a[j] == a[i] && j - i < 65535) j++; r.push_back({(int)(j - i), netWireId(a[i])}); i = j; }
        p32((uint32_t)r.size()); for (auto& q : r) { p16(q.first); p32((uint32_t)q.second); }
        std::vector<std::pair<int, int>> m; auto sm = savedM.find(k);
        if (sm != savedM.end()) { const uint8_t* mm = sm->second.data(); for (size_t i = 0; i < N;) { size_t j = i + 1; while (j < N && mm[j] == mm[i] && j - i < 65535) j++; m.push_back({(int)(j - i), mm[i]}); i = j; } }
        p32((uint32_t)m.size()); for (auto& q : m) { p16(q.first); blob.push_back((uint8_t)q.second); }
    }
    const size_t FR = 1200; const int total = blob.empty() ? 1 : (int)((blob.size() + FR - 1) / FR);
    for (int i = 0; i < total; i++) {
        NBuf b = netMk(NP_CHUNK); b.put<int32_t>(cx); b.put<int32_t>(cz); b.put<uint16_t>((uint16_t)total); b.put<uint16_t>((uint16_t)i);
        const size_t o = (size_t)i * FR, n = blob.size() > o ? std::min(FR, blob.size() - o) : 0; for (size_t q = 0; q < n; q++) b.put<uint8_t>(blob[o + q]);
        netTo(to, b);
    }
}
static void netChunkRecv(NBuf& b) {   // convidado: junta os fragmentos; completo => vira savedC/savedM e o gen() normal usa
    const int cx = b.get<int32_t>(), cz = b.get<int32_t>(), total = b.get<uint16_t>(), idx = b.get<uint16_t>(); const int64_t k = key(cx, cz);
    if (gChOK.count(k) || total < 1 || idx >= total) return;
    auto it = gChRx.find(k); if (it == gChRx.end()) return;   // so aceita o que foi pedido
    ChRx& r = it->second; if (r.total != total) { r.total = total; r.fr.assign(total, {}); r.got.assign(total, false); r.have = 0; }
    if (!r.got[idx]) { r.got[idx] = true; r.fr[idx].assign(b.d + b.r, b.d + b.n); r.have++; }
    if (r.have < total) return;
    std::vector<uint8_t> blob; for (auto& f : r.fr) blob.insert(blob.end(), f.begin(), f.end());
    gChRx.erase(it); gChOK.insert(k);
    if (blob.empty()) return;
    const size_t N = (size_t)CW * CH * CW; size_t p = 0;
    auto g32 = [&]() -> uint32_t { uint32_t v = 0; if (p + 4 <= blob.size()) memcpy(&v, &blob[p], 4); p += 4; return v; };
    const uint32_t nr = g32(); std::vector<BT> d(N, 0); size_t o = 0;
    for (uint32_t i = 0; i < nr && p + 6 <= blob.size() && o < N; i++) { size_t c = blob[p] | (blob[p + 1] << 8); p += 2; int id = netUnwireId((int32_t)g32()); if (id < 0) id = STONE; c = std::min(c, N - o); std::fill(d.begin() + o, d.begin() + o + c, (BT)id); o += c; }
    const uint32_t nm = g32(); std::vector<uint8_t> md(N, 0); o = 0;
    for (uint32_t i = 0; i < nm && p + 3 <= blob.size() && o < N; i++) { size_t c = blob[p] | (blob[p + 1] << 8); const uint8_t v = blob[p + 2]; p += 3; c = std::min(c, N - o); std::fill(md.begin() + o, md.begin() + o + c, v); o += c; }
    savedC[k] = std::move(d); savedM[k] = std::move(md);
}
static void netChunkReq(int cx, int cz) { NBuf b = netMk(NP_CHUNKREQ); b.put<int32_t>(cx); b.put<int32_t>(cz); netBroadcast(b); }
static bool netChunkWait(int cx, int cz) {   // convidado: true = ainda esperando o host (o gen() espera); false = pode gerar
    if (netSock < 0 || netHost || !netConn || gDim != 0) return false;
    const int64_t k = key(cx, cz); if (gChOK.count(k)) return false;
    static bool rb = false; if (!rb) { rb = true; int v = 4 << 20; setsockopt(netSock, SOL_SOCKET, SO_RCVBUF, (const char*)&v, sizeof v); }
    const Uint32 now = SDL_GetTicks(); auto it = gChRx.find(k);
    if (it == gChRx.end()) { if (gChRx.size() >= 4) return true; ChRx& r = gChRx[k]; r.t = r.t0 = now; netChunkReq(cx, cz); return true; }
    if (now - it->second.t0 > 12000) { gChRx.erase(it); gChOK.insert(k); return false; }   // host nao responde: gera pela seed
    if (now - it->second.t > 1500) { it->second.t = now; netChunkReq(cx, cz); }   // reenvia o pedido (fragmento perdido)
    return true;
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
                        if (!same && nickEq(q.name, pname) && now - q.seen > 2000) { remotes.erase(q.id); netSkinForget(q.id); peers.erase(peers.begin() + i); } else i++;
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
                const bool fresh = !p;
                if (!p) { if ((int)peers.size() >= NET_MAXP - 1) { netKick(from, "Servidor cheio."); continue; } p = netAddPeer(from, now, pname); }
                p->name = pname;
                p->seen = now; if (ready && !p->synced) { p->synced = true; netSyncPeer(*p); }
                NBuf w = netMk(NP_WELCOME); w.put<uint8_t>((uint8_t)p->id); w.put<int64_t>(WORLD_SEED); w.put<int32_t>((int32_t)gWorldTime);
                w.put<uint8_t>(creative ? 1 : 0); w.put<float>((float)gNetPos[0]); w.put<float>((float)gNetPos[1]); w.put<float>((float)gNetPos[2]); netPutName(w); netPutStr(w, p->name); netTo(from, w);
                if (fresh) { netRosterSend(); netSkinSendAll(&from); }
                continue;
            }
            if (!p) continue;
            p->seen = now;
            if (type == NP_CHAT) { NBuf pk = b; const int l = pk.get<uint8_t>(); std::string s; for (int i = 0; i < l; i++) s += (char)pk.get<uint8_t>(); if (sender != p->id || !netChatOk(p->id, s)) continue; }   // forjar nick de outro: descarta
            for (auto& q : peers) if (&q != p) sendto(netSock, (const char*)b.d, n, 0, (sockaddr*)&q.a, sizeof q.a);   // repassa aos outros
            if (type == NP_BYE) {
                remotes.erase(sender); netSkinForget(sender); netSay(netLabel(sender) + " saiu do jogo");
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
                gChRx.clear(); gChOK.clear();
                netSelf = b.get<uint8_t>(); gJoinSeed = b.get<int64_t>(); gJoinTime = b.get<int32_t>(); gJoinCreative = b.get<uint8_t>();
                for (int i = 0; i < 3; i++) gJoinPos[i] = b.get<float>();
                gHostPlName = netGetName(b); { const std::string fin = netGetName(b); if (!fin.empty()) gNetNickOv = fin; }   // nome final escolhido pelo host (ex.: Jogador_1)
                netConn = true; netHostSeen = now; gJoin = NJ_WELCOMED;
                continue;
            }
            if (!netConn) continue;
            netHostSeen = now;
            if (type == NP_BYE) {
                remotes.erase(sender); gNetNames.erase(sender); netSkinForget(sender);
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
            st.ely = b.r < b.n ? b.get<uint8_t>() : 0;
            r.dim = b.r < b.n ? (b.get<uint8_t>() & 3) : 0;   // dimensao (campo no fim: compativel com versoes sem ele)   // elitra (compativel com versoes sem o campo)   // ticks de carga do arco (0 = nao carrega)   // modo do jogador remoto (espectador some) e hardcore do host
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
            const int id = b.get<int16_t>(), cnt = b.get<int16_t>(), dl = b.get<int16_t>(); const std::string dn = nbtResolve(b.get<uint32_t>());
            if (!has) spawnDrop(x, y, z, vx, vy, vz, id, cnt, dl, nid, dn);
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
        case NP_EGG: eggNetRecv(b, sender); break;
        case NP_EGGHATCH: eggHatchRecv(b, sender); break;
        case NP_MOBS: mobSyncRecv(b); break;
        case NP_MOBHIT: if (netHost) mobSyncHitRecv(b, sender); break;
        case NP_MOBDMG: mobDmgRecv(b); break;
        case NP_PRIV: case NP_TPTO: case NP_GIVE: netCmdRecv(type, sender, b); break;
        case NP_ROSTER: case NP_NICKREQ: case NP_SYS: netNameRecv(type, sender, b); break;
        case NP_SKIN: netSkinRecv(sender, b); break;
        case NP_PING: if (!netHost && sender == 0) { NBuf pb = netMk(NP_PONG); pb.put<uint32_t>(b.get<uint32_t>()); netTo(netHostAddr, pb); } break;   // convidado devolve o mesmo token
        case NP_PONG: if (netHost) { const Uint32 t = b.get<uint32_t>(), rtt = now - t; for (auto& p : peers) if (p.id == sender) p.ping = p.ping < 0 ? (int)std::min<Uint32>(rtt, 9999u) : (p.ping * 3 + (int)std::min<Uint32>(rtt, 9999u)) / 4; } break;   // suavizado (media movel)
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
        case NP_NBT: nbtNetRecv(b); break;
        case NP_CHEST: chestNetRecv(b); break;
        case NP_CHESTREQ: chestNetReq(b); break;
        case NP_CHUNKREQ: if (netHost) { const int rcx = b.get<int32_t>(), rcz = b.get<int32_t>(); netChunkServe(from, rcx, rcz); } break;
        case NP_CHUNK: if (!netHost) netChunkRecv(b); break;
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
    b.put<int16_t>((int16_t)held); b.put<uint8_t>(sneak ? 1 : 0); b.put<uint8_t>((uint8_t)(gGm | (gHard ? 4 : 0))); b.put<uint8_t>(gBowOn && gBowT >= 0 ? (uint8_t)(std::min(gBowT, 254) + 1) : 0); b.put<uint8_t>((uint8_t)ely); b.put<uint8_t>((uint8_t)gDim); netBroadcast(b);
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
    if (netHost && !peers.empty() && now - gSkinT > 20000) { gSkinT = now; netSkinSendAll(nullptr); }   // reenvia (perda de UDP)
    else if (!netHost && netConn && gJoin == NJ_INGAME && (gSkinT == 0 || now - gSkinT > 20000)) { gSkinT = now ? now : 1; netSkinSend(netSelf, skin, nullptr); }
    if (netHost && !peers.empty()) { static Uint32 pingT = 0; if (now - pingT > 2000) { pingT = now; NBuf pb = netMk(NP_PING); pb.put<uint32_t>(now); netBroadcast(pb); } }
    if (netHost) {
        static Uint32 rosterT = 0; if (now - rosterT > 5000) { rosterT = now; netRosterSend(); }   // reenvia a lista de nicks (perda de UDP / convidado que chegou antes do WELCOME)
        for (size_t i = 0; i < peers.size();) {   // timeout de quem sumiu
            if (now - peers[i].seen < 6000) { ++i; continue; }
            const int id = peers[i].id; const std::string who = netLabel(id); peers.erase(peers.begin() + i); remotes.erase(id); netSkinForget(id);
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
static GLuint netSkinFromPx(const uint8_t* px) {   // textura propria; nao mexe no modelo nem na skin do jogador local
    std::vector<uint8_t> bak(skin, skin + sizeof skin); const GLuint keep = skinTex; skinTex = 0;
    memcpy(skin, px, sizeof skin); uploadSkin(); const GLuint t = skinTex;
    skinTex = keep; memcpy(skin, bak.data(), sizeof skin); return t;
}
static GLuint netSkinTexFor(int id) {   // skin do remoto; sem skin recebida ainda: a padrao (Steve), nunca a do jogador local
    static GLuint def = 0; static bool defTried = false;
    auto it = gSkins.find(id);
    if (it != gSkins.end() && it->second.px.size() == (size_t)SKIN_BYTES) {
        SkinSlot& sl = it->second;
        if (sl.dirty || !sl.tex) { if (sl.tex) glDeleteTextures(1, &sl.tex); sl.tex = netSkinFromPx(sl.px.data()); sl.dirty = false; }
        return sl.tex;
    }
    if (!defTried) { defTried = true;
        std::vector<uint8_t> bak(skin, skin + sizeof skin); const bool bs = slim, ba = slimAuto; const GLuint keep = skinTex;
        if (loadSkin("assets/skins/default.png")) { skinTex = 0; uploadSkin(); def = skinTex; }
        skinTex = keep; memcpy(skin, bak.data(), sizeof skin); slim = bs; slimAuto = ba; uploadSkin(); buildModel(); }
    return def;
}
struct NetSkinSwap { GLuint keep; explicit NetSkinSwap(GLuint t) : keep(skinTex) { if (t) skinTex = t; } ~NetSkinSwap() { skinTex = keep; } };
static void netDraw(float pt) {
    for (auto& kv : remotes) {
        const RemotePl& r = kv.second; if (!r.init || r.dim != gDim || (r.gm == 3 && gGm != 3)) continue;
        NetSkinSwap skinSwap(netSkinTexFor(kv.first));   // cada remoto com a propria skin (restaura ao fim da iteracao)
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
// ---- nametag acima do jogador remoto (como o Minecraft: billboard, fundo preto 25%, texto branco com sombra, 1/60 do bloco por pixel x 1.6) ----
static float gNetTagYaw = 0.f, gNetTagPitch = 0.f; static bool gNetTagOn = false;   // o loop principal liga e passa a camera (so na passada principal)
static bool netTagVisible(const RemotePl& r, double dx, double dy, double dz) {   // some agachado, morto, espectador alheio ou alem de 64 blocos
    if (!r.init || r.sneak || r.deathT >= 0 || (r.gm == 3 && gGm != 3)) return false;
    return dx * dx + dy * dy + dz * dz <= 64.0 * 64.0;
}
struct NetTagState { bool cull, lit, blend; NetTagState() : cull(glIsEnabled(GL_CULL_FACE)), lit(glIsEnabled(GL_LIGHTING)), blend(glIsEnabled(GL_BLEND)) {
        glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); }   // sem escrever profundidade: o texto nao briga com o fundo; paredes ainda escondem
    ~NetTagState() { glColor4f(1, 1, 1, 1); glDepthMask(GL_TRUE); glEnable(GL_TEXTURE_2D); if (cull) glEnable(GL_CULL_FACE); if (lit) glEnable(GL_LIGHTING); if (!blend) glDisable(GL_BLEND); } };
static void netTagOne(const std::string& nm, double x, double y, double z, unsigned rgb = 0xFFFFFF, bool xray = false) {   // 1 nametag no ponto (pes do jogador); rgb = cor do time; xray = mesmo time: tambem aparece atras de paredes (esmaecida, como o Minecraft)
    const float w = textWidth(nm.c_str(), 1.f), s = 0.016666668f * 1.6f, cr = ((rgb >> 16) & 255) / 255.f, cg = ((rgb >> 8) & 255) / 255.f, cb = (rgb & 255) / 255.f;
    glPushMatrix(); glTranslatef((float)x, (float)y + 2.3f, (float)z);   // altura do jogador (1.8) + 0.5
    glRotatef(gNetTagYaw, 0, 1, 0); glRotatef(gNetTagPitch, 1, 0, 0); glScalef(s, -s, s);   // inverso da rotacao da camera: sempre de frente, sem distorcer com FOV
    for (int pass = xray ? 0 : 1; pass < 2; pass++) {   // passada 0: sem teste de profundidade (atras de paredes); passada 1: normal
        if (pass == 0) glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D); glColor4f(0.f, 0.f, 0.f, pass ? .25f : .125f);
        glBegin(GL_QUADS); glVertex3f(-w / 2 - 1, -1, 0); glVertex3f(-w / 2 - 1, 8, 0); glVertex3f(w / 2 + 1, 8, 0); glVertex3f(w / 2 + 1, -1, 0); glEnd();
        text(-w / 2, 0.f, 1.f, nm.c_str(), cr, cg, cb, -1.f, pass ? 1.f : .5f);
        if (pass == 0) glEnable(GL_DEPTH_TEST);
    }
    glPopMatrix();
}
static void netDrawTags(float pt) {
    if (!gNetTagOn) return;
    NetTagState gl;
    for (auto& kv : remotes) {
        const RemotePl& r = kv.second; if (r.dim != gDim) continue;
        const double x = r.px + (r.x - r.px) * pt, y = r.py + (r.y - r.py) * pt, z = r.pz + (r.z - r.pz) * pt;
        if (!netTagVisible(r, x - gCamW[0], y - gCamW[1], z - gCamW[2])) continue;
        netTagOne(netLabel(kv.first), x, y, z, teamRgbOf(kv.first), teamSame(netSelf, kv.first));
    }
}
static void netDrawTagLocal(const std::string& nm, double x, double y, double z, bool sneak, bool dead) {   // split: nick do OUTRO jogador local acima do corpo (mesma regra das remotas)
    if (!gNetTagOn || sneak || dead) return;
    const double dx = x - gCamW[0], dy = y - gCamW[1], dz = z - gCamW[2]; if (dx * dx + dy * dy + dz * dz > 64.0 * 64.0) return;
    NetTagState gl; netTagOne(nm, x, y, z);
}
static int teamColorParse(const std::string& s0) {   // nome em ingles (como o Minecraft), em portugues ou numero 0-15; -1 = invalido
    const std::string s = foldAscii(s0.c_str());
    for (int i = 0; i < 16; i++) if (s == TEAM_COLOR[i]) return i;
    static const struct { const char* n; int c; } PT[] = {{"preto", 0}, {"azul_escuro", 1}, {"verde_escuro", 2}, {"ciano_escuro", 3}, {"vermelho_escuro", 4}, {"roxo", 5}, {"ouro", 6}, {"dourado", 6}, {"cinza", 7}, {"cinza_escuro", 8}, {"azul", 9}, {"verde", 10}, {"ciano", 11}, {"vermelho", 12}, {"rosa", 13}, {"amarelo", 14}, {"branco", 15}};
    for (const auto& p : PT) if (s == p.n) return p.c;
    if (!s.empty() && s.size() <= 2 && s.find_first_not_of("0123456789") == std::string::npos && atoi(s.c_str()) < 16) return atoi(s.c_str());
    return -1;
}
// /team add|remove|color|join|leave|list. Devolve linhas separadas por '\n'; comeco '!' = erro (chat_debug.inc)
static std::string cmdTeam(const std::vector<std::string>& a) {
    const std::string sub = a.empty() ? "list" : foldAscii(a[0].c_str());
    if (sub == "list" || sub == "lista") {
        if (gTeams.empty()) return "Não há times. Use /team add <nome> [cor]";
        std::string r; for (const TeamDef& t : gTeams) { std::string m; for (const auto& kv : gTeamOfId) if (nickEq(kv.second, t.name)) m += (m.empty() ? "" : ", ") + netLabel(kv.first); r += "Time " + t.name + " (" + TEAM_COLOR[t.col & 15] + "): " + (m.empty() ? "sem membros" : m) + "\n"; }
        return r;
    }
    if (netOn && !netHost) return "!Só o host pode criar e alterar times.";
    const std::string usage = "!Uso: /team add <nome> [cor] | remove <nome> | color <nome> <cor> | join <nome> [nick] | leave [nick] | list";
    std::string cores; for (int i = 0; i < 16; i++) cores += (i ? ", " : "") + std::string(TEAM_COLOR[i]);
    auto done = [&](const std::string& m) { teamRebuild(); netRosterSend(); return m; };
    if (sub == "add") {
        if (a.size() < 2 || a.size() > 3) return usage;
        if (!nickValid(a[1])) return "!Nome de time inválido (1 a 16 caracteres: A-Z a-z 0-9 _)";
        if (teamFind(a[1]) >= 0) return "!Já existe um time chamado " + a[1];
        if (gTeams.size() >= 16) return "!Limite de 16 times";
        int col = 15; if (a.size() == 3) { col = teamColorParse(a[2]); if (col < 0) return "!Cor desconhecida: " + a[2] + "  (" + cores + ")"; }
        gTeams.push_back({a[1], col}); return done("Time " + a[1] + " criado");
    }
    if (sub == "remove" || sub == "remover") {
        if (a.size() != 2) return usage; const int t = teamFind(a[1]); if (t < 0) return "!Time inexistente: " + a[1];
        const std::string n = gTeams[t].name; gTeams.erase(gTeams.begin() + t);
        for (auto it = gTeamOfNick.begin(); it != gTeamOfNick.end();) it = nickEq(it->second, n) ? gTeamOfNick.erase(it) : std::next(it);
        return done("Time " + n + " removido");
    }
    if (sub == "color" || sub == "cor") {
        if (a.size() != 3) return usage; const int t = teamFind(a[1]); if (t < 0) return "!Time inexistente: " + a[1];
        const int col = teamColorParse(a[2]); if (col < 0) return "!Cor desconhecida: " + a[2] + "  (" + cores + ")";
        gTeams[t].col = col; return done("Cor do time " + gTeams[t].name + ": " + TEAM_COLOR[col]);
    }
    if (sub == "join" || sub == "entrar" || sub == "leave" || sub == "sair") {
        const bool join = sub == "join" || sub == "entrar"; const size_t need = join ? 2 : 1;
        if (a.size() < need || a.size() > need + 1) return usage;
        int t = -1; if (join) { t = teamFind(a[1]); if (t < 0) return "!Time inexistente: " + a[1]; }
        std::string nick = a.size() == need + 1 ? a[need] : netName(); bool found = false;
        for (const std::string& n : cmdPlayerList()) if (nickEq(n, nick)) { nick = n; found = true; }
        if (!found) return "!Jogador não encontrado: " + nick;
        if (join) { gTeamOfNick[teamKey(nick)] = gTeams[t].name; return done(nick + " entrou no time " + gTeams[t].name); }
        if (!gTeamOfNick.erase(teamKey(nick))) return "!" + nick + " não está em nenhum time";
        return done(nick + " saiu do time");
    }
    return usage;
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
