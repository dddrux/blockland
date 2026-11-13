// LAN / ZeroTier: lista salva, descoberta (BLQ/BLA) e handshake HELLO/WELCOME por UDP real em loopback. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static Uint32 gFakeT = 1000;
Uint32 SDL_GetTicks() { return gFakeT; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static int udp(int port) {
    const int s = (int)socket(AF_INET, SOCK_DGRAM, 0); netSockNonBlock(s); sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons((uint16_t)port);
    if (port && bind(s, (sockaddr*)&a, sizeof a) < 0) { close(s); return -1; } return s;
}
static void lo(sockaddr_in& a, int port) { a = {}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons((uint16_t)port); }
static int recvWait(int s, uint8_t* b, int n, int ms = 500) { for (int i = 0; i < ms; i++) { int r = (int)recvfrom(s, (char*)b, n, 0, nullptr, nullptr); if (r > 0) return r; usleep(1000); } return -1; }
int main() {
    SECTION("parse e ip");
    std::string h; int p; uint32_t ip;
    CHECK(srvParse("10.147.17.20:47777", h, p) && h == "10.147.17.20" && p == 47777);
    CHECK(srvParse("  amigo.exemplo ", h, p) && h == "amigo.exemplo" && p == 47777);
    CHECK(srvParse("1.2.3.4:5000\r\n", h, p) && p == 5000);
    CHECK(!srvParse("1.2.3.4:99999", h, p) && !srvParse("", h, p));
    CHECK(ipParse("10.147.17.5", ip) && ipStr(ip) == "10.147.17.5" && !ipParse("nao.eh.ip", ip));
    SECTION("lista salva");
    remove("servers.cfg"); gSrvLoaded = false; gSrvSaved.clear();
    srvAdd(-1, "Casa|do Zé", "10.147.17.20:47800"); srvAdd(-1, "", "192.168.0.7");
    CHECK(gSrvSaved.size() == 2 && gSrvSaved[0].name == "Casa do Zé" && gSrvSaved[1].name == "192.168.0.7" && gSrvSaved[1].port == 47777);
    gSrvLoaded = false; gSrvSaved.clear(); srvEnsure();
    CHECK(gSrvSaved.size() == 2 && gSrvSaved[0].host == "10.147.17.20" && gSrvSaved[0].port == 47800);
    srvAdd(0, "Novo", "10.0.0.9:47777"); CHECK(gSrvSaved[0].host == "10.0.0.9"); srvDel(1); CHECK(gSrvSaved.size() == 1);
    gSrvSaved.clear(); srvSave(); remove("servers.cfg");
    SECTION("perfil (nome/skin)");
    CHECK(profClean("  Maria  ") == "Maria" && profClean("a<b>|c\n") == "abc" && profClean("12345678901234567890") == "1234567890123456" && profClean("Zé") == "Zé" && profClean("   ").empty());
    gPlayerName = "João Pedro"; gSkinFile = ""; profileSave(); gPlayerName.clear(); profileLoad(); CHECK(gPlayerName == "Joo_Pedro"); remove("profile.cfg");   // perfil antigo com acento/espaco: vira nick valido (Parte 2)
    gPlayerName = "Maria_9"; profileSave(); gPlayerName.clear(); profileLoad(); CHECK(gPlayerName == "Maria_9"); remove("profile.cfg");
    std::error_code ec; std::filesystem::create_directories("assets/skins", ec); { FILE* f = fopen("assets/skins/zz_teste.PNG", "wb"); fclose(f); } skinScan();
    CHECK(std::find(gSkinList.begin(), gSkinList.end(), "zz_teste.PNG") != gSkinList.end()); remove("assets/skins/zz_teste.PNG");
    gPlayerName.clear(); CHECK(netName() == "Jogador");
    SECTION("classificacao ZeroTier");
    NIf a; a.name = "zt7nnjhqb4"; a.ip = 0x0A931105; netClassify(a); CHECK(a.zt);
    NIf b; b.name = "eth0"; b.ip = 0xC0A80005; netClassify(b); CHECK(!b.zt);
    NIf c; c.ip = 0x0A931105; netClassify(c); CHECK(c.zt);        // sem nome (Android restrito): bloco padrao do ZeroTier
    NIf d; d.ip = 0xC0A80005; netClassify(d); CHECK(!d.zt);
    NIf e; e.name = "lo"; e.lo = true; e.ip = 0x7F000001; netClassify(e); CHECK(!e.zt);
    CHECK(!netIfaces().empty());
    SECTION("descoberta (cliente) contra host falso em loopback");
    {
        const int fake = udp(NET_DISC_PORT);
        if (fake < 0) printf("  (porta ocupada: pulado)\n"); else {
            gMultiVis = gFakeT; gLan.clear(); netSock = -1; gScanForce = true;
            for (int i = 0; i < 6; i++) { gFakeT += 20; gMultiVis = gFakeT; lanTick(gFakeT); }
            uint8_t q[16]; sockaddr_in from{}; socklen_t fl = sizeof from; int n = -1;
            for (int i = 0; i < 300 && n < 0; i++) { n = (int)recvfrom(fake, (char*)q, sizeof q, 0, (sockaddr*)&from, &fl); usleep(1000); }
            CHECK(n == 4 && memcmp(q, "BLQ", 3) == 0 && q[3] == NET_VER);
            NBuf r; r.put<uint8_t>('B'); r.put<uint8_t>('L'); r.put<uint8_t>('A'); r.put<uint8_t>((uint8_t)NET_VER); r.put<uint16_t>(47777); r.put<uint8_t>(2); r.put<uint8_t>(8); r.put<uint8_t>(1);
            const char* nm = "Mundo do Ze"; r.put<uint8_t>((uint8_t)strlen(nm)); for (const char* k = nm; *k; k++) r.put<uint8_t>((uint8_t)*k);
            sendto(fake, (const char*)r.d, r.n, 0, (sockaddr*)&from, fl); usleep(20000);
            for (int i = 0; i < 4; i++) { gFakeT += 20; gMultiVis = gFakeT; lanTick(gFakeT); }
            CHECK(!gLan.empty() && gLan[0].port == 47777 && gLan[0].players == 2 && gLan[0].maxp == 8 && gLan[0].mode == 1 && gLan[0].name == "Mundo do Ze" && gLan[0].kind == "Este PC");
            gSrvSaved.clear(); gSrvSaved.push_back({"Salvo", "127.0.0.1", 47777}); gSrvLoaded = true;
            std::vector<SrvRow> rows = srvRows(); CHECK(rows.size() == 1 && rows[0].ok && rows[0].saved && rows[0].cnt == "2/8");   // salvo casa com o descoberto (sem duplicar)
            gSrvSaved.clear(); rows = srvRows(); CHECK(rows.size() == 1 && !rows[0].saved && rows[0].host == "127.0.0.1");
            gFakeT += 20000; gMultiVis = gFakeT; lanTick(gFakeT); CHECK(gLan.empty());   // some depois de 12 s sem resposta
            gMultiVis = 0; lanTick(gFakeT); CHECK(scanSock < 0);   // aba fechada: varredura para
            close(fake);
        }
    }
    SECTION("varredura grande so em Atualizar");
    {
        lanBuildProbes(false); const size_t narrow = probeQ.size(); lanBuildProbes(true); const size_t wide = probeQ.size();
        CHECK(wide >= narrow);
        bool big = false; for (const NIf& n : ifCache) if (!n.lo && n.ip) { int pre = 0; for (uint32_t m = n.mask; m & 0x80000000u; m <<= 1) pre++; if (pre < 24) big = true; }
        if (big) CHECK(wide > narrow + 200);
    }
    SECTION("host: descoberta + handshake");
    {
        CHECK(netStartHost(47911)); gPlayerName = "Alex"; CHECK(netHost && discSock >= 0 && netSock >= 0);
        creative = true; WORLD_SEED = 424242; gWorldTime = 6000; gNetPos[0] = 12.5; gNetPos[1] = 70; gNetPos[2] = -3.5;
        const int cli = udp(0); sockaddr_in hd, hg; lo(hd, 47912); lo(hg, 47911);
        const uint8_t q[4] = {'B', 'L', 'Q', (uint8_t)NET_VER}; sendto(cli, (const char*)q, 4, 0, (sockaddr*)&hd, sizeof hd);
        for (int i = 0; i < 5; i++) { gFakeT += 20; lanTick(gFakeT); usleep(2000); }
        uint8_t buf[128]; int n = recvWait(cli, buf, sizeof buf);
        CHECK(n >= 11 && memcmp(buf, "BLA", 3) == 0 && buf[3] == NET_VER && (buf[4] | buf[5] << 8) == 47911 && buf[6] == 1 && buf[7] == NET_MAXP && buf[8] == 1);
        NBuf hello; hello.put<uint8_t>(NP_HELLO); hello.put<uint8_t>(255); hello.put<uint8_t>(NET_VER); hello.put<uint8_t>(0); hello.put<uint8_t>(3); hello.put<uint8_t>('Z'); hello.put<uint8_t>('e'); hello.put<uint8_t>('h');
        sendto(cli, (const char*)hello.d, hello.n, 0, (sockaddr*)&hg, sizeof hg);
        for (int i = 0; i < 5; i++) { gFakeT += 20; netPoll(gFakeT); usleep(2000); }
        CHECK(peers.size() == 1);
        n = recvWait(cli, buf, sizeof buf); NBuf w; memcpy(w.d, buf, n); w.n = n;
        CHECK(n > 0 && w.get<uint8_t>() == NP_WELCOME); w.get<uint8_t>();
        CHECK(w.get<uint8_t>() == 1 && w.get<int64_t>() == 424242 && w.get<int32_t>() == 6000 && w.get<uint8_t>() == 1);
        CHECK(w.get<float>() == 12.5f && w.get<float>() == 70.f && w.get<float>() == -3.5f);
        CHECK(netGetName(w) == "Alex");   // nome do host no WELCOME
        CHECK(peers[0].name == "Zeh" && netLabel(1) == "Zeh");
        NBuf bad; bad.put<uint8_t>(NP_HELLO); bad.put<uint8_t>(255); bad.put<uint8_t>(NET_VER + 1); bad.put<uint8_t>(0);
        const int cli2 = udp(0); sendto(cli2, (const char*)bad.d, bad.n, 0, (sockaddr*)&hg, sizeof hg);
        for (int i = 0; i < 5; i++) { gFakeT += 20; netPoll(gFakeT); usleep(2000); }
        n = recvWait(cli2, buf, sizeof buf); CHECK(n > 0 && buf[0] == NP_KICK && peers.size() == 1);
        netStop(); CHECK(netSock < 0 && discSock < 0 && !netHost);
        close(cli); close(cli2);
    }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
