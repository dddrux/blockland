// ================= chat, comandos, F3 e consistência do inventário =================
#include <cstdarg>
#ifdef __linux__
#include <unistd.h>
#endif
struct ChatLine { std::string s; Uint32 t; unsigned col; };
static std::vector<ChatLine> chatLog;
static bool chatOpen = false;
static std::string chatIn, chatSkip;
static size_t chatCur = (size_t)-1; static bool chatSelAll = false;   // cursor (byte; -1 = fim) e "selecionar tudo" (Ctrl+A)
static size_t curPos() { return chatCur > chatIn.size() ? chatIn.size() : chatCur; }
static std::vector<std::string> chatHist; static int chatHistPos = -1, chatScroll = 0;

static void chatAdd(const std::string& s, unsigned col = 0xFFFFFF) {
    logInfo("[CHAT] %s", s.c_str());   // como o Minecraft: o chat tambem vai para o log
    chatLog.push_back({s, SDL_GetTicks(), col});
    if (chatLog.size() > 200) chatLog.erase(chatLog.begin(), chatLog.begin() + 50);
}
static void setGm(int g) { applyGm(g); static const char* const N[4] = {"Sobrevivência", "Criativo", "Aventura", "Espectador"}; chatAdd(std::string("Modo de jogo alterado para ") + N[gGm]); }
static void setGameMode(bool c) { setGm(c ? 1 : 0); }

// ---------- inventário: normaliza todo frame (nada de pilhas vazias, negativas ou maiores que o limite) ----------
static void sanitizeSlot(Slot& s) { if (s.id <= 0 || s.n <= 0) { s = Slot(); return; } s.n = std::min(s.n, maxStack(s.id)); }
static void sanitizeInv() {
    for (Slot& s : inv) sanitizeSlot(s);
    for (Slot& s : craftG) sanitizeSlot(s);
    sanitizeSlot(cur);
    for (int i = 0; i < 4; i++) { sanitizeSlot(armor[i]); if (armor[i].id && !armorSlotOk(i, armor[i].id)) { addItem(armor[i].id, armor[i].n); armor[i] = Slot(); } }
    updateCraft();
}

// ---------- comandos ----------
static std::vector<std::string> splitWords(const std::string& s) {
    std::vector<std::string> v; std::string cur;
    for (char c : s) { if (c == ' ') { if (!cur.empty()) v.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) v.push_back(cur);
    return v;
}
static bool parseCoord(const std::string& a, double base, double& out) {   // 12.5 | ~ | ~3
    const char* p = a.c_str(); bool rel = *p == '~'; if (rel) p++;
    double v = 0; if (*p) { char* e; v = strtod(p, &e); if (*e) return false; } else if (!rel) return false;
    out = rel ? base + v : v; return true;
}
static int findItem(const std::string& q0) {
    std::string q = foldAscii(q0.c_str()); for (char& c : q) if (c == '_') c = ' ';
    if (q.empty()) return 0;
    if (q.find_first_not_of("0123456789") == std::string::npos) { int id = atoi(q.c_str()); return id > 0 && *itemName(id) ? id : 0; }
    int part = 0;
    for (int id = 1; id < PK0 + (int)PK.size(); id++) {
        if (id == 256) id = PK0;   // 256..PK0-1 nao existem
        if (!*itemName(id)) continue;
        if (isPk(id) && pkIsFaceOnly(id)) continue;   // faces soltas do pack (cactus_bottom...) nao sao itens
        const std::string n = isPk(id) ? PK[id - PK0].fold : foldAscii(itemName(id));
        if (n == q) return id;
        if (!part && n.find(q) != std::string::npos) part = id;
    }
    return part;
}
static int gTpHold = 0, gTpWait = 0;   // /tp: jogador fica parado ate o chunk de destino ficar pronto (como o "Carregando terreno" do MC)
// ---------- /locate: bioma ou estrutura mais próxima ----------
static int locBiomeId(const std::string& q0) {
    std::string q = foldAscii(q0.c_str()); for (char& c : q) if (c == '_') c = ' ';
    static const char* EN[NBIOME] = {"ocean", "beach", "plains", "forest", "desert", "savanna", "jungle", "swamp", "taiga", "tundra", "mountains"};
    for (int i = 0; i < NBIOME; i++) if (q == foldAscii(BIOME_NAME[i]) || q == EN[i]) return i;
    if (q == "extreme hills" || q == "hills") return B_MOUNTAINS;
    if (q == "ice plains" || q == "snow") return B_TUNDRA;
    return -1;
}
static void locateCmd(const std::vector<std::string>& w, Phys& P) {
    const unsigned ERR = 0xFF5555, OK = 0xFFFFFF;
    if (w.size() < 2) { chatAdd("Uso: /locate <bioma|village|vila>   (biomas: oceano, praia, planície, floresta, deserto, savana, selva, pântano, taiga, tundra, montanhas)", ERR); return; }
    std::string q = w[1]; for (size_t i = 2; i < w.size(); i++) q += " " + w[i];
    const std::string f = foldAscii(q.c_str());
    const int px = (int)floorf(P.x), pz = (int)floorf(P.z); char b[160];
    if (f == "village" || f == "vila") {
        const int rx0 = fdiv(px, 512), rz0 = fdiv(pz, 512); long best = -1; int bx = 0, bz = 0, found = -1;
        for (int ring = 0; ring <= 40; ring++) {
            for (int rz = rz0 - ring; rz <= rz0 + ring; rz++) for (int rx = rx0 - ring; rx <= rx0 + ring; rx++) {
                if (std::max(abs(rx - rx0), abs(rz - rz0)) != ring) continue;
                int sx, sz; if (!vilStart(rx, rz, sx, sz) || !vilGet(rx, rz, true)) continue;   // so posicao+layout (sem gerar terreno)
                const int x = sx * 16 + 2, z = sz * 16 + 2; const long d = (long)(x - px) * (x - px) + (long)(z - pz) * (z - pz);
                if (best < 0 || d < best) { best = d; bx = x; bz = z; if (found < 0) found = ring; }
            }
            if (found >= 0 && ring >= found + 1) break;   // o anel seguinte ainda pode ter uma vila mais perto
        }
        if (best < 0) chatAdd("Nenhuma vila encontrada num raio de ~20000 blocos", ERR);
        else { snprintf(b, sizeof b, "A vila mais próxima está em [%d, ~, %d] (%d blocos de distância)", bx, bz, (int)sqrt((double)best)); chatAdd(b, OK); }
        return;
    }
    const int id = locBiomeId(q);
    if (id < 0) { chatAdd("Bioma/estrutura desconhecido: " + q, ERR); return; }
    if (biomeAt((float)px, (float)pz) == id) { snprintf(b, sizeof b, "Você já está em %s [%d, ~, %d]", BIOME_NAME[id], px, pz); chatAdd(b, OK); return; }
    const int STEP = 32, RMAX = 3200;   // espiral quadrada a cada 32 blocos; para no primeiro anel que contém o bioma, pegando o ponto mais perto
    for (int r = 1; r * STEP <= RMAX; r++) {
        long best = -1; int bx = 0, bz = 0;
        for (int dz = -r; dz <= r; dz++) for (int dx = -r; dx <= r; dx++) {
            if (std::max(abs(dx), abs(dz)) != r) continue;
            const int x = px + dx * STEP, z = pz + dz * STEP;
            if (biomeAt((float)x, (float)z) != id) continue;
            const long d = (long)dx * dx + (long)dz * dz; if (best < 0 || d < best) { best = d; bx = x; bz = z; }
        }
        if (best >= 0) { snprintf(b, sizeof b, "%s mais próximo em [%d, ~, %d] (%d blocos de distância)", BIOME_NAME[id], bx, bz, (int)(sqrt((double)best) * STEP)); chatAdd(b, OK); return; }
    }
    chatAdd(std::string("Nenhum bioma ") + BIOME_NAME[id] + " encontrado num raio de " + std::to_string(RMAX) + " blocos", ERR);
}
static std::vector<std::string> cmdPlayerList(); static std::string cmdNickChange(const std::string& arg); static std::string cmdNick2Change(const std::string& arg); static std::string p2Nick();   // net.inc
static bool split2On(); static bool split2IsNick(const std::string& n); static int split2Give(int id, int n);   // splitscreen.inc: /give @p2 ou <nick do J2>
static std::string chatNick(); static void chatSend(const std::string& m);   // splitscreen.inc: nick/envio do jogador que abriu o chat (J1 ou J2)
static bool cmdIsPlayer(const std::string& n); static bool cmdIsSelf(const std::string& n); static bool cmdPlayerPos(const std::string& n, double* xyz, std::string& err);
static std::string cmdTeam(const std::vector<std::string>& a);   // net.inc
static std::string cmdTell(const std::string& n, const std::string& msg); static std::string cmdReply(const std::string& msg);
static std::string cmdSendTp(const std::string& n, double x, double y, double z); static std::string cmdSendGive(const std::string& n, int item, int cnt);   // net.inc
static void tpSelf(Phys& P, double x, double y, double z) {
    P.x = P.px = x; P.y = P.py = y; P.z = P.pz = z; P.mx = P.my = P.mz = 0;
    gTpHold = 1; gTpWait = 0;   // o carregamento acontece aos poucos nos proximos quadros
}
static std::string joinFrom(const std::vector<std::string>& w, size_t i) { std::string q; for (; i < w.size(); i++) q += (q.empty() ? "" : " ") + w[i]; return q; }
// /tp <nick> (vai ate o jogador) | /tp <alvo> <destino-nick> | /tp <alvo> <x> <y> <z> (mover outros: so o host). true = tratou (inclusive o erro)
static bool tpNick(const std::vector<std::string>& w, Phys& P) {
    const unsigned ERR = 0xFF5555; const size_t n = w.size(); std::string err; double d[3];
    if (n != 2 && n != 3 && n != 5) return false;
    if (n == 2) {
        if (!cmdPlayerPos(w[1], d, err)) { chatAdd(err, ERR); return true; }
        tpSelf(P, d[0], d[1], d[2]); chatAdd("Teletransportado para " + w[1] + " (carregando terreno...)"); return true;
    }
    if (n == 3) { if (cmdIsSelf(w[2])) { d[0] = P.x; d[1] = P.y; d[2] = P.z; } else if (!cmdPlayerPos(w[2], d, err)) { chatAdd(err, ERR); return true; } }
    else if (!(parseCoord(w[2], P.x, d[0]) && parseCoord(w[3], P.y, d[1]) && parseCoord(w[4], P.z, d[2]))) return false;
    if (cmdIsSelf(w[1])) { tpSelf(P, d[0], d[1], d[2]); chatAdd("Teletransportado (carregando terreno...)"); return true; }
    err = cmdSendTp(w[1], d[0], d[1], d[2]);
    chatAdd(err.empty() ? w[1] + " foi teletransportado" : err, err.empty() ? 0xFFFFFF : ERR); return true;
}
static void execCommand(const std::string& line, Phys& P) {
    const unsigned ERR = 0xFF5555, OK = 0xFFFFFF;
    logInfo("Jogador executou o comando: %s", line.c_str());   // como o Minecraft: "<jogador> issued server command"
    std::vector<std::string> w = splitWords(line.substr(1));
    if (w.empty()) { chatAdd("Digite /ajuda para ver os comandos", ERR); return; }
    const std::string c = foldAscii(w[0].c_str());
    if (c == "seed") { chatAdd("Seed: [" + std::to_string(WORLD_SEED) + "]", OK); return; }
    if (c == "gamemode" || c == "gm" || c == "modo") {
        if (w.size() < 2) { chatAdd("Uso: /gamemode <0|1|2|3>   (0 sobrevivência, 1 criativo, 2 aventura, 3 espectador)", ERR); return; }
        const std::string m = foldAscii(w[1].c_str());
        if (m == "0" || m == "s" || m == "survival" || m == "sobrevivencia") setGm(0);
        else if (m == "1" || m == "c" || m == "creative" || m == "criativo") setGm(1);
        else if (m == "2" || m == "a" || m == "adventure" || m == "aventura") setGm(2);
        else if (m == "3" || m == "sp" || m == "spectator" || m == "espectador") setGm(3);
        else chatAdd("Modo de jogo desconhecido: " + w[1], ERR);
    } else if (c == "vsync") {
        bool on = w.size() >= 2 && w[1] == "1";
        SDL_GL_SetSwapInterval(on ? 1 : 0);
        chatAdd(on ? "VSync ligado" : "VSync desligado (FPS destravado)", OK);
    } else if (c == "help" || c == "ajuda" || c == "?") {
        chatAdd("Comandos disponíveis:", 0xFFFF55);
        chatAdd("/locate <bioma|vila>  - acha o bioma ou a vila mais próxima", OK);
        chatAdd("/gamemode <0-3>  - troca o modo (0 sobrev., 1 criativo, 2 aventura, 3 espectador) | F3+F4 alterna", OK);
        chatAdd("/give <item> [qtd]  - dá um item (nome ou id)", OK);
        chatAdd("/dano <n>  - tira n meios-corações | /fome <0-20> | /curar", OK);
        chatAdd("/porco /galinha /vaca /ovelha [n]  - cria mobs (com IA) | /mobs limpar", OK);
        chatAdd("/hitbox  - mostra/esconde as caixas de colisão dos mobs", OK);
        chatAdd("/time set <day|noon|night|midnight|ticks> | add <n> | query | pause | resume  - hora do mundo", OK);
        chatAdd("/tp <x> <y> <z>  - teletransporta (aceita ~ relativo)", OK);
        chatAdd("/clear  - limpa o inventário   /pos  - mostra a posição", OK);
        chatAdd("/tell|/msg|/w <nick> <msg>  - sussurra | /r <msg>  - responde | /puxar <nick>  - traz o jogador (host)", OK);
        chatAdd("/tp <nick> | /tp <alvo> <destino|x y z> (host)   /give <nick|@p2> <item> [qtd] (host)", OK);
        chatAdd("/list  - jogadores online   /nick <nome>  - troca o seu nick   /nick2 <nome>  - nick do jogador 2 (split)", OK);
        chatAdd("/team add <nome> [cor] | join <nome> [nick] | leave | color | remove | list  - times (cor do nome, Tab, nametag)", OK);
        chatAdd("/me <ação>  /say <msg>  /kill  - ação no chat, anúncio e suicídio", OK);
        chatAdd("/seed  - mostra a seed do mundo   /cls  - limpa o chat   /ajuda  - esta lista   /vsync <0|1>  - liga/desliga o VSync", OK);
    } else if (c == "give" || c == "dar") {
        if (w.size() < 2) { chatAdd("Uso: /give <item> [quantidade]", ERR); return; }
        std::string tgt; bool toP2 = false;
        if (w.size() >= 3 && split2IsNick(w[1])) { toP2 = true; w.erase(w.begin() + 1); }   // /give @p2 <item> [qtd] (ou o nick do jogador 2 do split)
        else if (w.size() >= 3 && cmdIsPlayer(w[1])) { tgt = w[1]; w.erase(w.begin() + 1); }   // /give <nick> <item> [qtd]
        int n = 1; size_t last = w.size();
        if (w.size() >= 3 && w.back().find_first_not_of("0123456789") == std::string::npos) { n = atoi(w.back().c_str()); last--; }
        std::string q; for (size_t i = 1; i < last; i++) q += (i > 1 ? " " : "") + w[i];
        const int id = findItem(q);
        if (!id) { chatAdd("Item desconhecido: " + q, ERR); return; }
        n = std::max(1, std::min(n, 2304));
        if (toP2) { const int left = split2Give(id, n); chatAdd("Entregue " + std::to_string(n - left) + "x " + itemName(id) + " ao Jogador 2" + (left ? " (inventário cheio)" : "")); return; }
        if (!tgt.empty() && !cmdIsSelf(tgt)) { const std::string e = cmdSendGive(tgt, id, n); chatAdd(e.empty() ? "Entregue " + std::to_string(n) + "x " + itemName(id) + " a " + tgt : e, e.empty() ? OK : ERR); return; }
        const int left = addItem(id, n);
        chatAdd("Entregue " + std::to_string(n - left) + "x " + itemName(id) + (left ? " (inventário cheio)" : ""));
    } else if (c == "tp" || c == "teleport") {
        double x, y, z;
        if (w.size() == 4 && parseCoord(w[1], P.x, x) && parseCoord(w[2], P.y, y) && parseCoord(w[3], P.z, z)) {
            tpSelf(P, x, y, z);   // o carregamento acontece aos poucos nos proximos quadros (antes: 3x stream() sem limite = jogo travado)
            char b[96]; snprintf(b, sizeof b, "Teletransportado para %.1f %.1f %.1f (carregando terreno...)", x, y, z); chatAdd(b, 0x55FF55);   // verde = sucesso
        } else if (tpNick(w, P)) {
        } else chatAdd("Uso: /tp <x> <y> <z> | /tp <nick> | /tp <alvo> <destino> | /tp <alvo> <x> <y> <z>", ERR);
    } else if (c == "list" || c == "lista" || c == "players") {
        const std::vector<std::string> v = cmdPlayerList(); std::string t;
        for (size_t i = 0; i < v.size(); i++) t += (i ? ", " : "") + v[i];
        chatAdd("Há " + std::to_string(v.size()) + " jogador" + (v.size() == 1 ? "" : "es") + " online: " + t, OK);
    } else if (c == "nick2") {
        if (!gSplit) { chatAdd("O /nick2 só vale com o split screen ligado.", ERR); return; }
        if (w.size() != 2) { chatAdd("Uso: /nick2 <nick do jogador 2>   (1 a 16 caracteres: A-Z a-z 0-9 _)   Atual: " + p2Nick(), ERR); return; }
        const std::string e = cmdNick2Change(w[1]); chatAdd(e.empty() ? "O nick do jogador 2 agora é " + w[1] : e, e.empty() ? OK : ERR);
    } else if (c == "nick") {
        if (w.size() != 2) { chatAdd("Uso: /nick <novo_nick>   (1 a 16 caracteres: A-Z a-z 0-9 _)", ERR); return; }
        const std::string e = cmdNickChange(w[1]);
        if (!e.empty() && e[0] == '~') chatAdd(e.substr(1), OK); else chatAdd(e.empty() ? "Seu nick agora é " + w[1] : e, e.empty() ? OK : ERR);
    } else if (c == "tell" || c == "msg" || c == "w" || c == "whisper" || c == "sussurrar") {
        if (w.size() < 3) { chatAdd("Uso: /tell <nick> <mensagem>", ERR); return; }
        const std::string e = cmdTell(w[1], joinFrom(w, 2)); if (!e.empty()) chatAdd(e, ERR);
    } else if (c == "r" || c == "responder") {
        if (w.size() < 2) { chatAdd("Uso: /r <mensagem>   (responde a quem falou com você por último)", ERR); return; }
        const std::string e = cmdReply(joinFrom(w, 1)); if (!e.empty()) chatAdd(e, ERR);
    } else if (c == "puxar" || c == "pull") {
        if (w.size() != 2) { chatAdd("Uso: /puxar <nick>   (traz o jogador até você; só o host)", ERR); return; }
        if (cmdIsSelf(w[1])) { chatAdd("Esse é você mesmo.", ERR); return; }
        const std::string e = cmdSendTp(w[1], P.x, P.y, P.z); chatAdd(e.empty() ? w[1] + " foi puxado até você" : e, e.empty() ? OK : ERR);
    } else if (c == "clear" || c == "limpar") {
        for (Slot& s : inv) s = Slot();
        for (Slot& s : armor) s = Slot();
        for (Slot& s : craftG) s = Slot();
        cur = Slot(); updateCraft(); chatAdd("Inventário limpo");
    } else if (c == "bote" || c == "boat") {
        const std::string a = w.size() >= 2 ? foldAscii(w[1].c_str()) : "";
        if (a == "limpar" || a == "clear") { BOATS.clear(); gBoatRide = 0; chatAdd("Botes removidos", OK); }
        else {
            int t = BOAT_OAK; for (int i = 0; i < BOAT_N; i++) if (a == BOAT_NAME[i] || (i == BOAT_DARK_OAK && a == "dark")) t = i;
            boatSpawnFront(t, P, gPlayerYaw); chatAdd(std::string("Bote de ") + BOAT_NAME[t] + " criado   (/bote [oak|spruce|birch|jungle|acacia|dark_oak|limpar])", OK);
        }
    } else if (c == "porco" || c == "pig" || c == "galinha" || c == "chicken" || c == "vaca" || c == "cow" || c == "ovelha" || c == "sheep" || c == "aldeao" || c == "villager" || c == "zumbi" || c == "zombie" || c == "esqueleto" || c == "skeleton" || c == "creeper" || c == "pigman" || c == "porcozumbi" || c == "mobs") {
        const int type = (c == "porco" || c == "pig") ? M_PIG : (c == "galinha" || c == "chicken") ? M_CHICKEN : (c == "vaca" || c == "cow") ? M_COW : (c == "aldeao" || c == "villager") ? M_VILLAGER : (c == "zumbi" || c == "zombie") ? M_ZOMBIE : (c == "esqueleto" || c == "skeleton") ? M_SKELETON : c == "creeper" ? M_CREEPER : (c == "pigman" || c == "porcozumbi") ? M_PIGMAN : M_SHEEP;
        const std::string a = w.size() >= 2 ? foldAscii(w[1].c_str()) : "";
        if (a == "limpar" || a == "clear") { MOBS.clear(); chatAdd("Mobs removidos", OK); }
        else if (c == "mobs") chatAdd("Uso: /porco /galinha /vaca /ovelha /aldeao /zumbi /esqueleto /creeper [n]  |  /mobs limpar", ERR);
        else {
            const int n = std::max(1, std::min(20, a.empty() ? 1 : atoi(a.c_str())));
            for (int i = 0; i < n; i++) spawnMobFront(type, P, gPlayerYaw, (i - (n - 1) / 2.f) * 1.5f);
            chatAdd(std::string(MD[type].nome) + (n > 1 ? " x" + std::to_string(n) : "") + " criado", OK);
        }
    } else if (c == "hitbox" || c == "hitboxes") {
        gHitbox = !gHitbox; chatAdd(gHitbox ? "Hitboxes dos mobs: ligadas" : "Hitboxes dos mobs: desligadas", OK);
    } else if (c == "dano" || c == "damage") {
        if (creative) { chatAdd("Só funciona em sobrevivência (/gamemode 0)", ERR); return; }
        const int n = w.size() >= 2 ? std::max(1, atoi(w[1].c_str())) : 2;
        survDamage(n, "Foi morto por comando", true);
    } else if (c == "fome" || c == "hunger") {
        gFood = std::max(0, std::min(20, w.size() >= 2 ? atoi(w[1].c_str()) : 0)); gSat = 0.f;
        chatAdd("Fome definida", OK);
    } else if (c == "curar" || c == "heal") {
        gHP = 20; gFood = 20; gSat = 5.f; chatAdd("Vida e fome restauradas", OK);
    } else if (c == "time" || c == "hora" || c == "tempo") {   // /time set <day|noon|night|midnight|sunrise|sunset|ticks> | add <n> | query | pause | resume
        const std::string a = w.size() >= 2 ? foldAscii(w[1].c_str()) : "query";
        auto show = [&]() { char b[128]; const long long d = gWorldTime / DAY_TICKS + 1; snprintf(b, sizeof b, "Dia %lld, %s (tick %lld de %lld) - lua fase %d/8%s", d, timeString(gWorldTime).c_str(), ((gWorldTime % DAY_TICKS) + DAY_TICKS) % DAY_TICKS, DAY_TICKS, moonPhaseOf(gWorldTime) + 1, gDayCycle ? "" : " - tempo parado"); chatAdd(b, OK); };
        if (a == "query" || a == "ver") show();
        else if (a == "pause" || a == "parar" || a == "stop") { gDayCycle = false; chatAdd("Ciclo de dia e noite parado (/time resume para voltar)", OK); }
        else if (a == "resume" || a == "continuar" || a == "start") { gDayCycle = true; chatAdd("Ciclo de dia e noite ligado", OK); }
        else if ((a == "set" || a == "definir" || a == "add" || a == "somar") && w.size() >= 3) {
            const std::string v = foldAscii(w[2].c_str()); long long t = 0; bool ok = true;
            if (v == "day" || v == "dia") t = 1000; else if (v == "noon" || v == "meiodia") t = 6000; else if (v == "sunset" || v == "poente") t = 12000;
            else if (v == "night" || v == "noite") t = 13000; else if (v == "midnight" || v == "meianoite") t = 18000; else if (v == "sunrise" || v == "amanhecer" || v == "nascer") t = 23000;
            else if (!v.empty() && v.find_first_not_of("0123456789-") == std::string::npos) t = atoll(v.c_str()); else ok = false;
            if (!ok) chatAdd("Hora desconhecida: " + w[2] + "  (day, noon, sunset, night, midnight, sunrise ou ticks)", ERR);
            else { if (a == "set" || a == "definir") { gWorldTime = (gWorldTime / DAY_TICKS) * DAY_TICKS + ((t % DAY_TICKS) + DAY_TICKS) % DAY_TICKS; if (gWorldTime < 0) gWorldTime = 0; } else gWorldTime += t; show(); }
        } else chatAdd("Uso: /time set <day|noon|sunset|night|midnight|sunrise|ticks> | add <ticks> | query | pause | resume", ERR);
    } else if (c == "locate" || c == "localizar") {
        locateCmd(w, P);
    } else if (c == "pos") {
        char b[96]; snprintf(b, sizeof b, "Posição: %.2f %.2f %.2f", P.x, P.y, P.z); chatAdd(b);
    } else if (c == "team" || c == "equipe") {   // /team add|remove|color|join|leave|list (cor do nome, ordem do Tab, nametag atraves de paredes)
        const std::string r = cmdTeam(std::vector<std::string>(w.begin() + 1, w.end()));
        for (size_t i = 0; i < r.size();) { size_t e = r.find('\n', i); if (e == std::string::npos) e = r.size(); const std::string ln = r.substr(i, e - i); i = e + 1; if (ln.empty()) continue; if (ln[0] == '!') chatAdd(ln.substr(1), ERR); else chatAdd(ln, OK); }
    } else if (c == "me") {   // /me <acao>  ->  "* nick acao"
        if (w.size() < 2) { chatAdd("Uso: /me <ação>", ERR); return; }
        const std::string m = "* " + chatNick() + " " + joinFrom(w, 1); chatAdd(m); chatSend(m);
    } else if (c == "say") {   // /say <msg>  ->  "[nick] msg" (anuncio em azul)
        if (w.size() < 2) { chatAdd("Uso: /say <mensagem>", ERR); return; }
        const std::string m = "[" + chatNick() + "] " + joinFrom(w, 1); chatAdd(m, 0x5555FF); chatSend(m);
    } else if (c == "kill" || c == "matar") {
        if (creative) { chatAdd("Só funciona em sobrevivência (/gamemode 0)", ERR); return; }
        survDamage(1000, "Foi morto por comando", true);
    } else if (c == "cls") {
        chatLog.clear();
    } else if (!modulesCommand(c, std::vector<std::string>(w.begin() + 1, w.end()), P)) chatAdd("Comando desconhecido: /" + w[0] + "  (use /ajuda)", ERR);   // modulos tratam o resto
}

// ---------- entrada do chat ----------
static int chatCps(const std::string& s) { int n = 0; for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++; return n; }
static void chatText(const char* t) {
    if (!chatSkip.empty()) { bool same = chatSkip == t || (chatSkip == "t" && !strcmp(t, "T")); chatSkip.clear(); if (same) return; }   // letra que abriu o chat
    if (chatSelAll) { chatIn.clear(); chatCur = 0; chatSelAll = false; }
    size_t cp_ = curPos(); int cps = chatCps(chatIn);   // limite do Java: 256 caracteres (nao bytes); o resto e cortado
    for (const char* p = t; *p; ) {
        const unsigned char c0 = (unsigned char)*p; int len = c0 < 0x80 ? 1 : (c0 >> 5) == 6 ? 2 : (c0 >> 4) == 14 ? 3 : (c0 >> 3) == 30 ? 4 : 1;
        for (int i = 1; i < len; i++) if (!p[i]) { len = i; break; }
        if (c0 >= 32 && c0 != 127 && cps < 256) { chatIn.insert(cp_, p, len); cp_ += len; cps++; }
        p += len;
    }
    chatCur = cp_;
}
// Tab: completa comando (/gam -> /gamemode) ou nick (ultima palavra); varios candidatos => completa o prefixo comum e lista as opcoes (como o Java)
static void chatTab() {
    static const char* const CMDS[] = {"seed", "gamemode", "gm", "help", "ajuda", "give", "tp", "teleport", "list", "nick", "nick2", "tell", "msg", "w", "r", "puxar", "pull", "clear", "time", "locate", "heal", "damage", "hunger", "mobs", "pos", "me", "say", "kill", "team"};
    const bool isCmd = !chatIn.empty() && chatIn[0] == '/' && chatIn.find(' ') == std::string::npos;
    const size_t sp = isCmd ? 0 : chatIn.rfind(' '), st = isCmd ? 1 : (sp == std::string::npos ? 0 : sp + 1);
    const std::string pre = chatIn.substr(st); std::vector<std::string> cand;
    auto low = [](std::string x) { for (char& c : x) c = (char)tolower((unsigned char)c); return x; };
    const std::string lp = low(pre);
    if (isCmd) { for (const char* c : CMDS) if (low(c).compare(0, lp.size(), lp) == 0) cand.push_back(c); }
    else if (!pre.empty() || chatIn.empty() || chatIn.back() != ' ') { for (const std::string& n : cmdPlayerList()) if (low(n).compare(0, lp.size(), lp) == 0) cand.push_back(n); }
    if (cand.empty()) return;
    std::string com = cand[0]; for (const std::string& c : cand) { size_t i = 0; while (i < com.size() && i < c.size() && tolower((unsigned char)com[i]) == tolower((unsigned char)c[i])) i++; com.resize(i); }
    if (cand.size() == 1) com = cand[0] + (isCmd ? " " : "");
    if (com.size() > pre.size() || cand.size() == 1) { chatIn.resize(st); chatIn += com; }
    if (cand.size() > 1) { std::string l; for (size_t i = 0; i < cand.size(); i++) l += (i ? ", " : "") + cand[i]; chatAdd(l); }
}
static int chatKey(SDL_Keycode k, Uint16 mod, Phys& P) {   // devolve 1 para fechar o chat
    chatSkip.clear();
    if (k == SDLK_ESCAPE) return 1;
    if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
        if (!chatIn.empty()) {
            if (chatHist.empty() || chatHist.back() != chatIn) chatHist.push_back(chatIn);
            if (chatHist.size() > 50) chatHist.erase(chatHist.begin());
            if (chatIn[0] == '/') execCommand(chatIn, P); else { const std::string m = "<" + chatNick() + "> " + chatIn; chatAdd(m); chatSend(m); }   // J2 como convidado: o host so aceita o nick do proprio convidado (netChatOk), entao fica so local
        }
        return 1;
    }
    const std::string before = chatIn; const bool ctrl = (mod & KMOD_CTRL) != 0; const size_t cu = curPos();
    auto prevCp = [&](size_t i) { while (i > 0) { i--; if (((unsigned char)chatIn[i] & 0xC0) != 0x80) break; } return i; };
    auto nextCp = [&](size_t i) { if (i < chatIn.size()) { i++; while (i < chatIn.size() && ((unsigned char)chatIn[i] & 0xC0) == 0x80) i++; } return i; };
    auto wordPrev = [&](size_t i) { while (i > 0 && chatIn[i - 1] == ' ') i--; while (i > 0 && chatIn[i - 1] != ' ') i--; return i; };
    auto wordNext = [&](size_t i) { while (i < chatIn.size() && chatIn[i] == ' ') i++; while (i < chatIn.size() && chatIn[i] != ' ') i++; return i; };
    if (k == SDLK_a && ctrl) chatSelAll = !chatIn.empty();
    else if ((k == SDLK_c || k == SDLK_x) && ctrl) {   // copia (ou recorta) o texto digitado
        if (!chatIn.empty()) SDL_SetClipboardText(chatIn.c_str());
        if (k == SDLK_x) { chatIn.clear(); chatCur = (size_t)-1; chatSelAll = false; }
    }
    else if (k == SDLK_BACKSPACE || k == SDLK_DELETE) {
        if (chatSelAll) { chatIn.clear(); chatCur = 0; chatSelAll = false; }
        else if (k == SDLK_BACKSPACE && cu > 0) { const size_t b = ctrl ? wordPrev(cu) : prevCp(cu); chatIn.erase(b, cu - b); chatCur = b; }
        else if (k == SDLK_DELETE && cu < chatIn.size()) { const size_t e = ctrl ? wordNext(cu) : nextCp(cu); chatIn.erase(cu, e - cu); chatCur = cu; }
    }
    else if (k == SDLK_LEFT) { chatSelAll = false; chatCur = ctrl ? wordPrev(cu) : prevCp(cu); }
    else if (k == SDLK_RIGHT) { chatSelAll = false; chatCur = ctrl ? wordNext(cu) : nextCp(cu); }
    else if (k == SDLK_HOME) { chatSelAll = false; chatCur = 0; }
    else if (k == SDLK_END) { chatSelAll = false; chatCur = (size_t)-1; }
    else if (k == SDLK_UP && !chatHist.empty()) { if (chatHistPos < 0) chatHistPos = (int)chatHist.size(); if (chatHistPos > 0) chatIn = chatHist[--chatHistPos]; }
    else if (k == SDLK_DOWN && chatHistPos >= 0) { if (++chatHistPos >= (int)chatHist.size()) { chatHistPos = -1; chatIn.clear(); } else chatIn = chatHist[chatHistPos]; }
    else if (k == SDLK_TAB) chatTab();
    else if (k == SDLK_PAGEUP) chatScroll += 5;
    else if (k == SDLK_PAGEDOWN) chatScroll = std::max(0, chatScroll - 5);
    else if (k == SDLK_v && (mod & KMOD_CTRL)) { if (char* t = SDL_GetClipboardText()) { std::string s = t; SDL_free(t); for (char& c : s) if (c == '\n' || c == '\r' || c == '\t') c = ' '; chatText(s.c_str()); } }
    if (chatIn != before && (k == SDLK_UP || k == SDLK_DOWN || k == SDLK_TAB)) { chatCur = (size_t)-1; chatSelAll = false; }   // historico/Tab: cursor no fim
    return 0;
}

// ---------- desenho ----------
static void uiOrtho(int w, int h) {
    glViewport(gVpX, gVpY, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void uiEnd() { glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); }
static void wrapText(const std::string& s, int maxCp, std::vector<std::string>& out) {
    const char* p = s.c_str(); std::string cur; int n = 0;
    while (*p) {
        const char* q = p; u8next(p); cur.append(q, p - q);
        if (++n >= maxCp && *p) {   // quebra no ultimo espaco; sem espaco, quebra no limite
            const size_t sp = cur.rfind(' ');
            if (sp != std::string::npos && sp > 0 && sp + 1 < cur.size()) { out.push_back(cur.substr(0, sp)); cur = cur.substr(sp + 1); n = chatCps(cur); }
            else { out.push_back(cur); cur.clear(); n = 0; }
        }
    }
    if (!cur.empty() || out.empty()) out.push_back(cur);
}
static void drawChat(int w, int h) {
    if (chatLog.empty() && !chatOpen) return;
    uiOrtho(w, h);
    const int g = guiScaleFor(w, h), sh = (h + g - 1) / g; const float sc = (float)g;
    struct VL { std::string s; unsigned col; float a; };
    std::vector<VL> vis; const Uint32 now = SDL_GetTicks();
    for (size_t i = chatLog.size() > 80 ? chatLog.size() - 80 : 0; i < chatLog.size(); i++) {
        const float age = (now - chatLog[i].t) / 1000.f;
        const float a = chatOpen ? 1.f : age < 9.f ? 1.f : age < 10.f ? 10.f - age : 0.f;
        if (a <= 0) continue;
        std::vector<std::string> parts; wrapText(chatLog[i].s, 52, parts);
        for (const std::string& p : parts) vis.push_back({p, chatLog[i].col, a});
    }
    const int maxLines = chatOpen ? 10 : 7, total = (int)vis.size();
    chatScroll = std::max(0, std::min(chatScroll, std::max(0, total - maxLines)));
    if (!chatOpen) chatScroll = 0;
    const int shown = std::min(maxLines, total - chatScroll);
    for (int i = 0; i < shown; i++) {
        const VL& v = vis[total - chatScroll - 1 - i];
        const float y = (float)((sh - 40 - 9 * (i + 1)) * g);
        flat(2.f * g, y, 324.f * g, 9.f * g, 0, 0, 0, .5f * v.a);
        text(4.f * g, y + g, sc, v.s.c_str(), ((v.col >> 16) & 255) / 255.f, ((v.col >> 8) & 255) / 255.f, (v.col & 255) / 255.f, sc, v.a);
    }
    if (chatOpen) {
        flat(2.f * g, (float)((sh - 14) * g), (float)(w - 4 * g), 12.f * g, 0, 0, 0, .55f);
        const size_t cu = curPos(); const float maxW = (float)(w - 14 * g), by = (float)((sh - 12) * g);
        size_t st = 0;   // rola o texto para o cursor ficar sempre visivel
        while (st < cu && textWidth(chatIn.substr(st, cu - st).c_str(), sc) > maxW - 6.f * g) { const char* p = chatIn.c_str() + st; u8next(p); st = (size_t)(p - chatIn.c_str()); }
        std::string show = chatIn.substr(st);
        while (textWidth(show.c_str(), sc) > maxW && !show.empty()) { size_t b = show.size() - 1; while (b > 0 && ((unsigned char)show[b] & 0xC0) == 0x80) b--; show.erase(b); }
        const float cx = 4.f * g + (cu > st ? textWidth(chatIn.substr(st, cu - st).c_str(), sc) + sc : 0.f);
        if (chatSelAll && !show.empty()) flat(4.f * g, by, textWidth(show.c_str(), sc) + sc, 10.f * g, .2f, .4f, 1.f, .6f);
        text(4.f * g, by + g, sc, show.c_str(), 1, 1, 1, sc);
        if ((SDL_GetTicks() / 450) % 2) {
            if (cu >= chatIn.size()) text(cx, by + g, sc, "_", 1, 1, 1, sc);   // no fim: sublinhado
            else flat(cx, by, (float)g, 10.f * g, 1, 1, 1, 1);                  // no meio: barra
        }
    }
    uiEnd();
}

// ---------- F3 ----------
struct DbgInfo { double x, y, z, speed; float yaw, pitch; bool hasT, flying, sprint, sneak, ground, water; int tx, ty, tz, tid, camMode, ww, wh; };
static float dbgHist[120]; static int dbgHN = 0;
static double dbgFps = 0, dbgAvg = 0, dbgMin = 0, dbgMax = 0, dbgAcc = 0, dbgWMin = 1e9, dbgWMax = 0; static int dbgFrames = 0;
static void dbgFrame(float dtSec) {
    const double ms = dtSec * 1000.0; dbgHist[dbgHN++ % 120] = (float)ms;
    dbgAcc += dtSec; dbgFrames++; dbgWMin = std::min(dbgWMin, ms); dbgWMax = std::max(dbgWMax, ms);
    if (dbgAcc >= .5) { dbgFps = dbgFrames / dbgAcc; dbgAvg = 1000.0 * dbgAcc / dbgFrames; dbgMin = dbgWMin; dbgMax = dbgWMax; dbgAcc = 0; dbgFrames = 0; dbgWMin = 1e9; dbgWMax = 0; }
}
static bool procMem(double& curMB, double& peakMB) {   // memória do processo (Windows e Linux)
#ifdef _WIN32
    struct PMC { uint32_t cb, pf; size_t peakWS, ws, qpp, qp, qpnp, qnp, pagefile, peakPagefile; };
#ifdef _WIN64
    typedef int (*Fn)(void*, PMC*, uint32_t);
#else
    typedef int (__stdcall *Fn)(void*, PMC*, uint32_t);
#endif
    static Fn fn = nullptr; static bool tried = false;
    if (!tried) { tried = true; if (void* lib = SDL_LoadObject("kernel32.dll")) fn = (Fn)SDL_LoadFunction(lib, "K32GetProcessMemoryInfo"); }
    if (!fn) return false;
    PMC m; memset(&m, 0, sizeof m); m.cb = sizeof m;
    if (!fn((void*)(intptr_t)-1, &m, sizeof m)) return false;
    curMB = m.ws / 1048576.0; peakMB = m.peakWS / 1048576.0; return true;
#elif defined(__linux__)
    FILE* f = fopen("/proc/self/status", "r"); if (!f) return false;
    char ln[256]; long rss = 0, hwm = 0;
    while (fgets(ln, sizeof ln, f)) { sscanf(ln, "VmRSS: %ld", &rss); sscanf(ln, "VmHWM: %ld", &hwm); }
    fclose(f); curMB = rss / 1024.0; peakMB = hwm / 1024.0; return rss > 0;
#else
    (void)curMB; (void)peakMB; return false;
#endif
}
static bool vramInfo(double& freeMB, double& totalMB) {   // NVIDIA (NVX_gpu_memory_info) e AMD (ATI_meminfo)
    static int mode = -1;
    if (mode < 0) { const char* ex = (const char*)glGetString(GL_EXTENSIONS); mode = ex && strstr(ex, "GL_NVX_gpu_memory_info") ? 1 : ex && strstr(ex, "GL_ATI_meminfo") ? 2 : 0; }
    if (!mode) return false;
    GLint v[4] = {0, 0, 0, 0};
    if (mode == 1) { GLint tot = 0, fr = 0; glGetIntegerv(0x9048, &tot); glGetIntegerv(0x9049, &fr); freeMB = fr / 1024.0; totalMB = tot / 1024.0; return tot > 0; }
    glGetIntegerv(0x87FB, v); freeMB = v[0] / 1024.0; totalMB = 0; return v[0] > 0;
}
static void drawDebug(const DbgInfo& d, int w, int h) {
    uiOrtho(w, h);
    const int g = guiScaleFor(w, h); const float sc = (float)g;
    static double memCur = 0, memPeak = 0; static bool memOk = false; static Uint32 memT = 0;
    if (SDL_GetTicks() - memT > 500) { memT = SDL_GetTicks(); memOk = procMem(memCur, memPeak); }
    size_t verts = 0; int meshed = 0;
    for (auto& kv : W) { size_t n = (size_t)kv.second->nv + kv.second->nwv; verts += n; if (n) meshed++; }
    static const char* DIRS[4] = {"Norte (-Z)", "Oeste (-X)", "Sul (+Z)", "Leste (+X)"};
    const int dir = ((int)floorf(d.yaw / 90.f + .5f) % 4 + 4) % 4;
    const int bx = (int)floor(d.x), by = (int)floor(d.y), bz = (int)floor(d.z);
    char b[200]; std::vector<std::string> L, R;
    auto F = [&](std::vector<std::string>& v, const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap); v.push_back(b); };
    F(L, "Blockland v19  (%s)", gGm == 3 ? "espectador" : gGm == 2 ? "aventura" : gGm == 1 ? "criativo" : gHard ? "hardcore" : "sobrevivência");
    F(L, "%.0f fps   %.1f ms/quadro  (mín %.1f, máx %.1f)", dbgFps, dbgAvg, dbgMin, dbgMax);
    F(L, "Chunks: %d carregados, %d com malha", (int)W.size(), meshed);
    F(L, "Malha: %.2f M vértices (%.1f MB)  |  voxels: %.1f MB", verts / 1e6, verts * sizeof(V) / 1048576.0, W.size() * (double)(CW * CH * CW) / 1048576.0);
    L.push_back("");
    F(L, "XYZ: %.3f / %.3f / %.3f", d.x, d.y, d.z);
    F(L, "Bioma: %s", BIOME_NAME[biomeAt((float)d.x, (float)d.z)]);
    F(L, "Bloco: %d %d %d   Chunk: %d %d  (local %d %d %d)", bx, by, bz, fdiv(bx, CW), fdiv(bz, CW), bx - fdiv(bx, CW) * CW, by, bz - fdiv(bz, CW) * CW);
    F(L, "Direção: %s   (yaw %.1f, pitch %.1f)", DIRS[dir], d.yaw, d.pitch);
    F(L, "Velocidade: %.2f blocos/s", d.speed);
    std::string st = d.ground ? "no chão" : "no ar"; if (d.flying) st = "voando"; if (d.water) st = "na água"; if (d.sprint) st += ", correndo"; if (d.sneak) st += ", agachado";
    F(L, "Estado: %s   Câmera: %s", st.c_str(), d.camMode == 0 ? "1ª pessoa" : d.camMode == 1 ? "3ª pessoa (atrás)" : "3ª pessoa (frente)");
    if (d.hasT) F(L, "Mira: %s (id %d) em %d %d %d", *itemName(d.tid) ? itemName(d.tid) : "?", d.tid, d.tx, d.ty, d.tz); else L.push_back("Mira: nenhum bloco");
    // direita
    if (memOk) F(R, "Memória do processo: %.0f MB  (pico %.0f MB)", memCur, memPeak); else R.push_back("Memória do processo: indisponível");
    F(R, "RAM do sistema: %d MB", SDL_GetSystemRAM());
    double vf, vt; if (vramInfo(vf, vt)) { if (vt > 0) F(R, "VRAM: %.0f / %.0f MB livres", vf, vt); else F(R, "VRAM livre: %.0f MB", vf); }
    R.push_back("");
    F(R, "CPU: %d núcleos lógicos", SDL_GetCPUCount());
    const char* gr = (const char*)glGetString(GL_RENDERER); const char* gv = (const char*)glGetString(GL_VENDOR); const char* gl = (const char*)glGetString(GL_VERSION);
    F(R, "GPU: %s", gr ? gr : "?"); F(R, "Fabricante: %s", gv ? gv : "?"); F(R, "OpenGL: %s", gl ? gl : "?");
    SDL_version sv; SDL_GetVersion(&sv);
    F(R, "SDL %d.%d.%d  vídeo: %s", sv.major, sv.minor, sv.patch, SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?");
    F(R, "Janela: %dx%d   (GUI x%d)", d.ww, d.wh, g);
    auto draw = [&](const std::vector<std::string>& v, bool right) {
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i].empty()) continue;
            const float tw = textWidth(v[i].c_str(), sc), x = right ? w - 2.f * g - tw : 2.f * g, y = (2.f + 10.f * i) * g;
            flat(x - g, y - g, tw + 2.f * g, 9.f * g, .31f, .31f, .31f, .56f);
            text(x, y, sc, v[i].c_str(), .88f, .88f, .88f, sc);
        }
    };
    draw(L, false); draw(R, true);
    // gráfico de tempo de quadro (verde < 20 ms, amarelo < 34 ms, vermelho acima)
    const float gx = w - 124.f * g, gy = h - 4.f * g, gh = 40.f * g;
    flat(gx - g, gy - gh - g, 122.f * g, gh + 2.f * g, 0, 0, 0, .45f);
    for (int i = 0; i < 120; i++) {
        const float ms = dbgHist[(dbgHN + i) % 120], hh2 = std::min(1.f, ms / 50.f) * gh;
        flat(gx + i * g, gy - hh2, (float)g, hh2, ms < 20.f ? .3f : 1.f, ms < 34.f ? 1.f : .3f, .3f, .9f);
    }
    flat(gx, gy - gh * (16.7f / 50.f), 120.f * g, (float)g, 1, 1, 1, .5f); flat(gx, gy - gh * (33.3f / 50.f), 120.f * g, (float)g, 1, 1, 1, .5f);
    text(gx - 24.f * g, gy - gh * (16.7f / 50.f) - 3.f * g, sc, "60", 1, 1, 1, 0); text(gx - 24.f * g, gy - gh * (33.3f / 50.f) - 3.f * g, sc, "30", 1, 1, 1, 0);
    uiEnd();
}
