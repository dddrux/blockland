#include <array>
#include <chrono>
#include <atomic>
#include <memory>
#include <set>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
// audio.inc - SONS (patchRPI.11). Mixer proprio sobre SDL_OpenAudioDevice (sem SDL_mixer).
// patchAud.OGG: os sons sao os .ogg originais do Minecraft 1.12.2 em assets/sounds/<pasta>/<nome>.ogg (vorbis.inc decodifica);
// WAV PCM16/IMA-ADPCM continua aceito como reserva. Mistura ate 24 vozes.
// Variaveis de ambiente: BLOCKLAND_MUTE=1 desliga o audio. Sem sons no disco o jogo segue mudo (nada falha).
#ifndef SDLCALL
#define SDLCALL
#endif
struct SndBuf { std::vector<int16_t> pcm; int ch = 1, rate = 44100; size_t frames = 0; bool ok = false; };
struct SndVoice { SndBuf* b = nullptr; double pos = 0, step = 1; float vl = 1, vr = 1; bool music = false; };
static const int8_t SND_IMA_IDX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int16_t SND_IMA_STEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static std::string gSndDevName; static bool gSndMuted = false;   // patchAud.1: dispositivo escolhido ("" = padrao) e mudo
static SDL_AudioDeviceID gSndDev = 0; static int gSndRate = 44100; static bool gSndOn = false;
static SndVoice gSndV[24]; static std::map<std::string, SndBuf> gSndCache;
static float gSndMaster = 1.f, gSndMusicVol = .5f;
static float gSndCat[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};   // volumes por categoria = sliders m1..m8 de Opcoes > Musica & Sons (1 Musica, 4 Blocos, 5 Hostis, 6 Amigaveis, 7 Jogadores); m0 = gSndMaster (ver sndSyncVol em screens.inc)
static bool gSndOnG = true;   // patchAud.4: jogador no chao (passo so toca no chao; ao pousar toca 1 passo)
static int gSndStepMat = 0;   // patchAud.2: material do bloco sob os pes (0 pedra,1 grama,2 cascalho,3 areia,4 madeira,5 la,6 neve,7 vidro); main_loop ajusta
static const char* const SND_BRK[8] = {"block.break", "block.break.grass", "block.break.gravel", "block.break.sand", "block.break.wood", "block.break.cloth", "block.break.snow", "block.break.glass"};
static const char* const SND_PLC[8] = {"block.place", "block.place.grass", "block.place.gravel", "block.place.sand", "block.place.wood", "block.place.cloth", "block.place.snow", "block.place.glass"};
static const char* const SND_STP[8] = {"step.stone", "step.grass", "step.gravel", "step.sand", "step.wood", "step.cloth", "step.snow", "step.glass"};
static int gSndMusicLast = -1;   // ultima faixa (1/2): a proxima e sempre a outra
static double gSndLX = 0, gSndLY = 0, gSndLZ = 0; static float gSndYaw = 0;   // ouvinte (cabeca do jogador)
static std::string gSndMusicName; static int gSndMusicT = 100; static float gSndStepAcc = 0;   // musica: ticks ate a proxima faixa; passos

static inline int sndClamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
// decodifica WAV (PCM16 ou IMA-ADPCM) para PCM16 intercalado; false se formato nao suportado
static bool sndParseWav(const std::vector<uint8_t>& f, SndBuf& b) {
    auto u16 = [&](size_t o) { return (int)(f[o] | (f[o + 1] << 8)); };
    auto u32 = [&](size_t o) { return (size_t)f[o] | ((size_t)f[o + 1] << 8) | ((size_t)f[o + 2] << 16) | ((size_t)f[o + 3] << 24); };
    if (f.size() < 44 || memcmp(f.data(), "RIFF", 4) || memcmp(f.data() + 8, "WAVE", 4)) return false;
    int fmt = 0, ch = 0, rate = 0, ba = 0, bits = 0, spb = 0; size_t dOff = 0, dLen = 0;
    for (size_t o = 12; o + 8 <= f.size();) {
        const size_t sz = u32(o + 4);
        if (!memcmp(&f[o], "fmt ", 4) && sz >= 16 && o + 8 + 16 <= f.size()) { fmt = u16(o + 8); ch = u16(o + 10); rate = (int)u32(o + 12); ba = u16(o + 20); bits = u16(o + 22); if (fmt == 0x11 && sz >= 20 && o + 28 <= f.size()) spb = u16(o + 26); }
        else if (!memcmp(&f[o], "data", 4)) { dOff = o + 8; dLen = std::min(sz, f.size() - dOff); break; }
        o += 8 + sz + (sz & 1);
    }
    if (!dOff || (ch != 1 && ch != 2) || rate < 4000 || rate > 96000) return false;
    b.ch = ch; b.rate = rate;
    if (fmt == 1 && bits == 16) { b.frames = dLen / (2 * ch); b.pcm.resize(b.frames * ch); for (size_t i = 0; i < b.frames * ch; i++) b.pcm[i] = (int16_t)u16(dOff + i * 2); return b.frames > 0; }
    if (fmt != 0x11 || ba < 4 * ch + 4 || spb < 2) return false;
    const size_t nb = dLen / ba; b.frames = nb * spb; b.pcm.assign(b.frames * ch, 0);
    for (size_t blk = 0; blk < nb; blk++) {
        const uint8_t* p = &f[dOff + blk * ba]; int pred[2] = {0, 0}, idx[2] = {0, 0}; int16_t* out = &b.pcm[blk * spb * ch];
        for (int c = 0; c < ch; c++) { pred[c] = (int16_t)(p[4 * c] | (p[4 * c + 1] << 8)); idx[c] = std::min<int>(p[4 * c + 2], 88); out[c] = (int16_t)pred[c]; }
        for (int g = 0; 4 * ch + (g + 1) * 4 * ch <= ba; g++) for (int c = 0; c < ch; c++) {
            const uint8_t* q = p + 4 * ch + (g * ch + c) * 4;
            for (int j = 0; j < 8; j++) {
                const int s = 1 + g * 8 + j; if (s >= spb) break;
                const int nib = (j & 1) ? (q[j / 2] >> 4) : (q[j / 2] & 15); const int step = SND_IMA_STEP[idx[c]];
                int diff = ((2 * (nib & 7) + 1) * step) >> 3; if (nib & 8) diff = -diff;   // mesma conta do ffmpeg (que gerou os arquivos)
                pred[c] = sndClamp(pred[c] + diff, -32768, 32767); idx[c] = sndClamp(idx[c] + SND_IMA_IDX[nib], 0, 88);
                out[s * ch + c] = (int16_t)pred[c];
            }
        }
    }
    return b.frames > 0;
}
static std::string gPfNotes;   // patchLog.1: causas registradas no quadro (audio decodificado, etc.); impressas junto do [LAG] (perf.inc)
static void perfNote(const char* what, double ms) { (void)ms; if (gPfNotes.size() < 600) { if (!gPfNotes.empty()) gPfNotes += " | "; gPfNotes += what; } }
static void sndSlowNote(const std::string& name, double ms, size_t frames, int rate) {   // decodificar na thread principal > 25 ms = possivel travada
    if (ms < 25.0) return; char t[200]; snprintf(t, sizeof t, "audio decodificado: %s (%.0f ms, %.1f s de som)", name.c_str(), ms, rate ? frames / (double)rate : 0.0);
    logInfo("%s", t); perfNote(t, ms);
}
static bool packGetFile(const std::string& rel, std::vector<uint8_t>& out); static bool packHasFile(const std::string& rel);   // patchRP.1 (mcpack.inc)
// patchSnd.mod1: sons de mod convertido = arquivo solto <mod>/sounds/<ns>/<caminho>.ogg; nome pedido "ns:caminho"
static std::vector<std::string> gSndModDirs;   // pastas de mod (preenchido por modRulesLoad)
static std::string sndLooseFind(const std::string& n) {
    const size_t c = n.find(':'); if (c == std::string::npos) return "";
    for (const auto& d : gSndModDirs) { const std::string f = d + "/sounds/" + n.substr(0, c) + "/" + n.substr(c + 1) + ".ogg"; if (FILE* fp = fopen(f.c_str(), "rb")) { fclose(fp); return f; } }
    return "";
}
static std::string sndResolve(const std::string& n) {
    if (n.find(':') != std::string::npos) return sndLooseFind(n).empty() ? std::string() : n;   // patchSnd.mod1   // patchRP.2: nome pedido -> nome que existe nos packs (dig/* -> step/*, enderchest -> chest)
    auto ex = [](const std::string& s) { return packHasFile("assets/minecraft/sounds/" + s + ".ogg"); };
    if (ex(n)) return n;
    {   // patchSndFix.1: eventos pedem nomes 1.8 que o AudioCraft/Pixel Perfection nao trazem -> equivalente mais proximo que EXISTE
        static const char* const AL[][2] = {{"random/click", "note/hat"}, {"random/wood_click", "note/hat"}, {"random/bow", "random/bowhit1"}, {"random/fuse", "fire/ignite"},
            {"random/classic_hurt", "damage/hit1"}, {"random/classic_hurt1", "damage/hit1"}, {"random/classic_hurt2", "damage/hit2"}, {"random/classic_hurt3", "damage/hit3"},
            {"random/burp", "random/pop"}, {"random/chestopen", "block/chest/open"}, {"random/chestclosed", "block/chest/close"}, {"random/fizz", "liquid/lavapop"}, {"random/orb", "random/pop"}};
        for (const auto& a : AL) if (n == a[0] && ex(a[1])) return a[1];
    }
    std::string d, b = n; while (!b.empty() && isdigit((unsigned char)b.back())) { d.insert(d.begin(), b.back()); b.pop_back(); }
    {   // patchSndFix.2: sons de mobs que o AudioCraft/Pixel Perfection nao trazem -> equivalente mais proximo que EXISTE (nome sem numero -> nome sem numero)
        static const char* const MA[][2] = {
            {"mob/pig/say", "mob/cow/hurt"}, {"mob/pig/death", "mob/cow/hurt"}, {"mob/pig/hurt", "mob/cow/hurt"},
            {"mob/sheep/say", "mob/cow/say"}, {"mob/sheep/hurt", "mob/cow/hurt"}, {"mob/sheep/death", "mob/cow/hurt"},
            {"mob/wolf/growl", "mob/wolf/bark"}, {"mob/wolf/whine", "mob/wolf/hurt"},
            {"mob/guardian/land_idle", "mob/guardian/elder_idle"}, {"mob/guardian/land_hit", "mob/guardian/guardian_hit"}, {"mob/guardian/land_death", "mob/guardian/guardian_death"},
            {"mob/endermen/idle", "mob/endermen/portal"}, {"mob/endermen/hit", "mob/endermen/stare"}, {"mob/endermen/scream", "mob/endermen/stare"},
            {"mob/bat/idle", "mob/phantom/idle"}, {"mob/bat/hurt", "mob/phantom/idle"}, {"mob/bat/death", "mob/phantom/idle"},
            {"mob/snowgolem/ambient", "mob/irongolem/hit"}, {"mob/snowgolem/hurt", "mob/irongolem/hit"}, {"mob/snowgolem/death", "mob/irongolem/death"},
            {"mob/evocation_illager/ambient", "mob/evocation_illager/prepare_attack"}, {"mob/evocation_illager/hurt", "mob/evocation_illager/cast"}, {"mob/evocation_illager/death", "mob/evocation_illager/fangs"},
            {"mob/illusion_illager/ambient", "mob/evocation_illager/prepare_attack"}, {"mob/illusion_illager/hurt", "mob/evocation_illager/cast"}, {"mob/illusion_illager/death", "mob/evocation_illager/fangs"},
            {"mob/vindication/idle", "mob/zombie/say"}, {"mob/vindication/hurt", "mob/zombie/hurt"}, {"mob/vindication/death", "mob/zombie/death"},
            {"mob/villager/idle", "mob/zombie/say"}, {"mob/villager/hit", "mob/zombie/hurt"}, {"mob/villager/death", "mob/zombie/death"},
            {"mob/witch/idle", "mob/zombie/say"}, {"mob/witch/hurt", "mob/zombie/hurt"}, {"mob/witch/death", "mob/zombie/death"},
            {"mob/cat/meow", "mob/wolf/hurt"}, {"mob/cat/hitt", "mob/wolf/hurt"}, {"mob/rabbit/idle", "mob/chicken/say"}, {"mob/rabbit/hurt", "mob/chicken/hurt"}, {"mob/rabbit/death", "mob/chicken/hurt"},
            {"mob/parrot/idle", "mob/chicken/say"}, {"mob/parrot/hurt", "mob/chicken/hurt"}, {"mob/parrot/death", "mob/chicken/hurt"},
            {"mob/squid/ambient", "liquid/swim"}, {"mob/squid/hurt", "liquid/swim"}, {"mob/polarbear/idle", "mob/cow/say"}, {"mob/polarbear/hurt", "mob/cow/hurt"}, {"mob/polarbear/death", "mob/cow/hurt"},
            {"mob/endermite/say", "mob/silverfish/say"}, {"mob/endermite/hit", "mob/silverfish/hit"}, {"mob/endermite/kill", "mob/silverfish/kill"}, {"mob/irongolem/throw", "mob/irongolem/hit"},
            {"mob/shulker/ambient", "mob/vex/idle"}, {"mob/shulker/hurt", "mob/vex/charge"}, {"mob/shulker/death", "mob/vex/charge"}};
        for (const auto& a : MA) if (b == a[0]) { const std::string x = a[1]; if (ex(x + d)) return x + d; if (ex(x + "1")) return x + "1"; if (ex(x)) return x; }
    }
    std::vector<std::string> c; if (b.compare(0, 4, "dig/") == 0) c.push_back("step/" + b.substr(4)); if (b.compare(0, 17, "block/enderchest/") == 0) c.push_back("block/chest/" + b.substr(17));
    for (auto& x : c) { if (ex(x + d)) return x + d; if (ex(x + "1")) return x + "1"; if (ex(x)) return x; }
    return "";
}
static bool sndLoadBytes(const std::string& name, bool& isOgg, std::vector<uint8_t>& f) { isOgg = true; if (name.find(':') != std::string::npos) { const std::string lf = sndLooseFind(name); if (lf.empty()) return false; FILE* fp = fopen(lf.c_str(), "rb"); if (!fp) return false; fseek(fp, 0, SEEK_END); const long L = ftell(fp); fseek(fp, 0, SEEK_SET); f.resize(L > 0 ? L : 0); const bool ok = L > 0 && fread(f.data(), 1, L, fp) == (size_t)L; fclose(fp); return ok; }   // patchSnd.mod1
    const std::string r = sndResolve(name); return !r.empty() && packGetFile("assets/minecraft/sounds/" + r + ".ogg", f); }   // patchRP.2
static SndBuf* sndGet(const std::string& name) {   // carrega sob demanda: assets/sounds/<name>.wav (cache; falha tambem fica em cache)
    auto it = gSndCache.find(name); if (it != gSndCache.end()) return it->second.ok ? &it->second : nullptr;
    SndBuf& b = gSndCache[name]; bool isOgg = true; std::vector<uint8_t> f; if (!sndLoadBytes(name, isOgg, f)) return nullptr; splashNote((name + ".ogg").c_str());   // patchRP.1
    const auto t0 = std::chrono::steady_clock::now();
    if (isOgg) { int ch = 0, rate = 0; b.ok = vbDecodeOgg(f, b.pcm, ch, rate); if (b.ok) { b.ch = ch; b.rate = rate; b.frames = b.pcm.size() / ch; } }
    else b.ok = sndParseWav(f, b);
    if (!b.ok) { logWarn("Som invalido: %s", name.c_str()); return nullptr; }
    sndSlowNote(name, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(), b.frames, b.rate);   // patchLog.1: decodificacao lenta vai pro log
    return &b;
}
#ifdef BL_RPI
// patchRpiOpt.7: no Pi, decodificar um .ogg novo na thread principal travava 50-170 ms (115 vezes / ~10 s em 15 min de jogo, nos logs).
// Depois do prewarm, sons de evento/mob ainda nao carregados sao decodificados numa thread de fundo; a 1a vez o som e pulado, as proximas tocam.
struct SndAsyncDone { std::string name; SndBuf b; bool ok = false; };
struct SndAsyncQ { std::mutex m; std::condition_variable cv; std::deque<std::string> req; std::vector<SndAsyncDone> done; };
static SndAsyncQ* gSndQ = nullptr; static bool gSndAsync = false;
static bool sndDecodeFile(const std::string& name, SndBuf& b) {   // so le arquivo e decodifica (sem globais): seguro em outra thread
    bool isOgg = true; std::vector<uint8_t> f; if (!sndLoadBytes(name, isOgg, f)) return false;   // patchRP.1
    if (isOgg) { int ch = 0, rate = 0; if (!vbDecodeOgg(f, b.pcm, ch, rate) || ch < 1) return false; b.ch = ch; b.rate = rate; b.frames = b.pcm.size() / ch; return b.frames > 0; }
    return sndParseWav(f, b);
}
static void sndAsyncWorker(SndAsyncQ* q) {
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
    for (;;) {
        std::string name; { std::unique_lock<std::mutex> lk(q->m); q->cv.wait(lk, [&] { return !q->req.empty(); }); name = q->req.front(); q->req.pop_front(); }
        SndAsyncDone d; d.name = name; d.ok = sndDecodeFile(name, d.b);
        { std::lock_guard<std::mutex> lk(q->m); q->done.push_back(std::move(d)); }
    }
}
static void sndAsyncRequest(const std::string& name) {   // thread principal: marca o som como "pendente" (entrada com ok=false) e pede a decodificacao
    if (!gSndQ) { gSndQ = new SndAsyncQ(); std::thread(sndAsyncWorker, gSndQ).detach(); }   // nunca destruido (a thread pode ainda estar rodando no fim do processo)
    gSndCache[name]; { std::lock_guard<std::mutex> lk(gSndQ->m); gSndQ->req.push_back(name); } gSndQ->cv.notify_one();
}
static void sndAsyncPump() {   // 1x por quadro na thread principal: publica os sons prontos no cache (ok so vira true no fim: a thread de audio nunca ve buffer pela metade)
    if (!gSndQ) return; std::vector<SndAsyncDone> got; { std::lock_guard<std::mutex> lk(gSndQ->m); got.swap(gSndQ->done); }
    for (SndAsyncDone& d : got) { SndBuf& dst = gSndCache[d.name]; if (!d.ok) { logWarn("Som invalido/ausente: %s", d.name.c_str()); continue; } dst.pcm = std::move(d.b.pcm); dst.ch = d.b.ch; dst.rate = d.b.rate; dst.frames = d.b.frames; dst.ok = true; }
}
static SndBuf* sndGetBg(const std::string& name) {   // sndGet sem travar: som ainda nao carregado vai para a thread de fundo (retorna nullptr ate ficar pronto)
    if (!gSndAsync) return sndGet(name);
    auto it = gSndCache.find(name); if (it != gSndCache.end()) return it->second.ok ? &it->second : nullptr;
    sndAsyncRequest(name); return nullptr;
}
#else
#define sndGetBg sndGet
static void sndAsyncPump() {}
#endif
static void voiceMix(float* acc, int n);   // voice.inc
static void SDLCALL sndCallback(void*, Uint8* stream, int len) {   // thread de audio: soma as vozes em float e satura para S16 estereo
    const int n = len / 4; static std::vector<float> acc; acc.assign((size_t)n * 2, 0.f);
    for (SndVoice& v : gSndV) {
        SndBuf* b = v.b; if (!b) continue;
        for (int i = 0; i < n; i++) {
            const size_t ip = (size_t)v.pos; if (ip + 1 >= b->frames) { v.b = nullptr; break; }
            const float fr = (float)(v.pos - (double)ip); float l, r;
            if (b->ch == 1) { const float s = b->pcm[ip] + (b->pcm[ip + 1] - b->pcm[ip]) * fr; l = r = s; }
            else { l = b->pcm[ip * 2] + (b->pcm[ip * 2 + 2] - b->pcm[ip * 2]) * fr; r = b->pcm[ip * 2 + 1] + (b->pcm[ip * 2 + 3] - b->pcm[ip * 2 + 1]) * fr; }
            acc[(size_t)i * 2] += l * v.vl; acc[(size_t)i * 2 + 1] += r * v.vr; v.pos += v.step;
        }
    }
    voiceMix(acc.data(), n);   // patchVoice.1: vozes do voice chat
    int16_t* o = (int16_t*)stream;
    for (int i = 0; i < n * 2; i++) { const float s = acc[i] * (gSndMuted ? 0.f : gSndMaster); o[i] = (int16_t)(s > 32767.f ? 32767 : s < -32768.f ? -32768 : s); }
}
static bool sndOpenDev();
static void sndInit() {
    const char* mute = getenv("BLOCKLAND_MUTE"); if (mute && *mute == '1') { logInfo("Audio desligado (BLOCKLAND_MUTE=1)"); return; }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { logWarn("Audio indisponivel: %s", SDL_GetError()); return; }
    if (!sndOpenDev()) return;
    logInfo("Audio: %d Hz, 24 vozes, driver %s", gSndRate, SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "?");
}
static bool sndOpenDev() {   // patchAud.1: abre (ou reabre) o dispositivo gSndDevName
    SDL_AudioSpec want, have; memset(&want, 0, sizeof want); want.freq = 44100; want.format = AUDIO_S16SYS; want.channels = 2;
#ifdef BL_RPI
    want.samples = 2048;   // Pi 3B: buffer maior evita estalos
#else
    want.samples = 1024;
#endif
    want.callback = sndCallback;
    gSndDev = SDL_OpenAudioDevice(gSndDevName.empty() ? nullptr : gSndDevName.c_str(), 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
#ifdef BL_RPI
    if (!gSndDev) {   // patchRpi.5: Pi sem som = padrao indisponivel (HDMI/jack/Pulse): tenta cada dispositivo e depois cada driver (alsa, pulseaudio...) ate abrir um
        const int nd = SDL_GetNumAudioDevices(0);
        for (int i = 0; i < nd && !gSndDev; i++) { const char* dn = SDL_GetAudioDeviceName(i, 0); if (!dn) continue; gSndDev = SDL_OpenAudioDevice(dn, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE); if (gSndDev) logInfo("Audio: dispositivo %s", dn); }
        const int ndrv = SDL_GetNumAudioDrivers();
        for (int d = 0; d < ndrv && !gSndDev; d++) {
            const char* dr = SDL_GetAudioDriver(d); if (!dr || !strcmp(dr, "dummy") || !strcmp(dr, "disk")) continue;
            SDL_AudioQuit(); if (SDL_AudioInit(dr) != 0) continue;
            gSndDev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE); if (gSndDev) logInfo("Audio: driver %s", dr);
        }
    }
    if (gSndDev && have.channels != 2) { logWarn("Audio: %d canais (o mixer so faz estereo): sem som", (int)have.channels); SDL_CloseAudioDevice(gSndDev); gSndDev = 0; }
#endif
    if (!gSndDev) { logWarn("Nao foi possivel abrir o dispositivo de audio: %s", SDL_GetError()); gSndOn = false; return false; }
    gSndRate = have.freq > 0 ? have.freq : 44100; gSndOn = true; SDL_PauseAudioDevice(gSndDev, 0); return true;
}
static void sndStopAll() { if (!gSndDev) return; SDL_LockAudioDevice(gSndDev); for (SndVoice& v : gSndV) { v.b = nullptr; v.pos = 0; } SDL_UnlockAudioDevice(gSndDev); }
static void sndDeviceNext() {   // cicla: padrao -> dispositivo 0 -> 1 ... -> padrao (reabre o audio)
    const int n = SDL_GetNumAudioDevices(0); int cur = -1; for (int i = 0; i < n; i++) if (gSndDevName == SDL_GetAudioDeviceName(i, 0)) cur = i;
    const int nx = cur + 1; gSndDevName = nx >= n ? std::string() : std::string(SDL_GetAudioDeviceName(nx, 0));
    if (gSndDev) { SDL_CloseAudioDevice(gSndDev); gSndDev = 0; } gSndOn = false; sndStopAll(); sndOpenDev();
}
static void sndPlayBuf(SndBuf* b, float vl, float vr, float pitch, bool music) {
    if (!gSndOn || !b || (vl <= .003f && vr <= .003f)) return;
    SDL_LockAudioDevice(gSndDev);
    #ifdef BL_RPI
    const int NV = 12;   // patchRpi.8: no Pi so 12 vozes (cada voz custa uma interpolacao por amostra no callback)
#else
    const int NV = 24;
#endif
    int slot = -1; double best = -1; for (int i = 0; i < NV; i++) { if (!gSndV[i].b) { slot = i; break; } if (!gSndV[i].music && gSndV[i].b->frames) { const double pr = gSndV[i].pos / (double)gSndV[i].b->frames; if (pr > best) { best = pr; slot = i; } } }   // sem vaga: toma a voz mais adiantada (nunca a musica)
    if (slot >= 0) { SndVoice& v = gSndV[slot]; v.b = nullptr; v.pos = 0; v.step = (double)b->rate / gSndRate * pitch; v.vl = vl; v.vr = vr; v.music = music; v.b = b; }
    SDL_UnlockAudioDevice(gSndDev);
}
static void sndPlay(const char* name, float vol = 1.f, float pitch = 1.f, int cat = 0) { if (gSndOn) { if (cat > 0 && cat < 9) vol *= gSndCat[cat]; sndPlayBuf(sndGetBg(name), vol, vol, pitch, false); } }   // som de interface (sem posicao)
static void sndPlayAt(const char* name, double x, double y, double z, float vol = 1.f, float pitch = 1.f, int cat = 0) {   // som no mundo: atenuacao linear em 16 blocos (x vol) e pan esquerda/direita
    if (!gSndOn) return; const float vol0 = vol; if (cat > 0 && cat < 9) vol *= gSndCat[cat];   // patchSnd.1 (BUG-039): vol > 1 so aumenta o alcance (como no MC); ganho nunca passa de 1 (antes estourava/clipava no TNT)
    const double dx = x - gSndLX, dy = y - gSndLY, dz = z - gSndLZ, d = sqrt(dx * dx + dy * dy + dz * dz), rng = 16.0 * std::max(1.f, vol0);
    vol = std::min(1.f, vol0) * (vol0 > 0.f ? vol / vol0 : 0.f);
    if (d >= rng) return; const float g = vol * (float)(1.0 - d / rng) * (float)(1.0 - d / rng * .3);
    float pan = 0.f; if (d > .5) { const float yr = gSndYaw * .0174533f; pan = (float)((dx * cosf(yr) - dz * sinf(yr)) / d); }   // direita do ouvinte = (cos yaw, -sin yaw)
    sndPlayBuf(sndGetBg(name), pan > 0 ? g * (1.f - pan * .6f) : g, pan < 0 ? g * (1.f + pan * .6f) : g, pitch, false);
}
static std::string sndVar(const char* base, int n) { char b[96]; snprintf(b, sizeof b, "%s%d", base, 1 + rand() % std::max(1, n)); return b; }   // base + numero (1..n)
static float sndRandPitch(float spread = .1f) { return 1.f + ((rand() % 2001) / 1000.f - 1.f) * spread; }
// ---- eventos de som: assets/sounds/events.cfg liga "evento = arquivos ; vol pitch var cat" (facil de trocar; sndEvtAt/sndEvtUi) ----
struct SndEvt { std::vector<std::string> f; float vol = 1.f, pitch = 1.f, var = .1f; int cat = 0; };
static std::map<std::string, SndEvt> gSndEvts; static bool gSndEvtOk = false;
static bool sndFileOk(const std::string& n) { return !sndResolve(n).empty(); }   // patchRP.2
static void sndEvtLine(const std::string& ln) {
    if (ln.empty() || ln[0] == '#') return; const size_t eq = ln.find('='); if (eq == std::string::npos) return;
    std::string key = ln.substr(0, eq); while (!key.empty() && key.back() == ' ') key.pop_back(); SndEvt e; std::istringstream ss(ln.substr(eq + 1)); std::string t;
    while (ss >> t) {
        if (t == ";" || t == "|") continue; const size_t q = t.find('=');
        if (q != std::string::npos) { const float v = (float)atof(t.c_str() + q + 1), k = t.compare(0, q, "vol") == 0 ? 0.f : t.compare(0, q, "pitch") == 0 ? 1.f : t.compare(0, q, "var") == 0 ? 2.f : t.compare(0, q, "cat") == 0 ? 3.f : -1.f;
            if (k == 0.f) e.vol = v; else if (k == 1.f) e.pitch = v; else if (k == 2.f) e.var = v; else if (k == 3.f) e.cat = (int)v; continue; }
        if (t.back() == '#') { const std::string b = t.substr(0, t.size() - 1); for (int i = 1; i < 10 && sndFileOk(b + std::to_string(i)); i++) e.f.push_back(b + std::to_string(i)); }
        else if (sndFileOk(t)) e.f.push_back(t); else { static std::set<std::string> seenMiss; static int nMiss = 0; if (seenMiss.insert(t).second && ++nMiss <= 5) logWarn("events.cfg: evento '%s' pede o som '%s' mas assets/sounds/%s.ogg nao existe (copie a pasta sounds completa do 1.12.2)%s", key.c_str(), t.c_str(), t.c_str(), nMiss == 5 ? " [demais sons ausentes omitidos do log]" : ""); }
    }
    gSndEvts[key] = e;
}
static void sndRefresh() { sndStopAll(); gSndCache.clear(); gSndEvts.clear(); gSndEvtOk = false; }   // "Atualizar sons": recarrega events.cfg e os .wav
static void sndEvtLoad() {
    gSndEvtOk = true; const char* path = "assets/sounds/events.cfg"; FILE* fp = fopen(path, "rb");
    if (!fp) { const char* d = R"CFG(# Blockland - eventos de som (este arquivo e recriado se for apagado).
# Formato:  evento = arquivo1 arquivo2 ... ; vol=1 pitch=1 var=0.1 cat=4
#  - arquivo = caminho dentro de assets/sounds SEM extensao (le .ogg, senao .wav). "nome#" = nome1, nome2, nome3... (sorteia um).
#  - Para usar outro som: ponha o arquivo em assets/sounds (.ogg, ou .wav como reserva) e troque o nome na linha.
#  - vol = volume, pitch = tom, var = variacao aleatoria do tom, cat = slider de Opcoes (0 so o principal, 1 musica, 4 blocos, 5 hostis, 6 amigaveis, 7 jogadores).
ui.click = random/click ; vol=0.8 cat=0
item.pickup = random/pop ; vol=0.3 pitch=1 var=0.5 cat=7
item.drop = random/pop ; vol=0.3 pitch=0.7 var=0.1 cat=7
egg.throw = random/bow ; vol=0.5 pitch=0.5 var=0.2 cat=7
egg.hit = random/pop ; vol=0.6 pitch=1 var=0.1 cat=7
pearl.tp = random/pop ; vol=0.8 pitch=0.5 var=0.1 cat=7
tnt.fuse = random/fuse ; vol=1 pitch=1 var=0 cat=4
explode = random/explode# ; vol=4 pitch=0.7 var=0.15 cat=4
block.break = dig/stone# ; vol=1 pitch=0.8 var=0.1 cat=4
block.place = dig/stone# ; vol=1 pitch=0.8 var=0.1 cat=4
step.stone = step/stone# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.grass = dig/grass# ; vol=1 pitch=1 var=0.1 cat=4
block.place.grass = dig/grass# ; vol=1 pitch=0.9 var=0.1 cat=4
step.grass = step/grass# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.gravel = dig/gravel# ; vol=1 pitch=1 var=0.1 cat=4
block.place.gravel = dig/gravel# ; vol=1 pitch=0.9 var=0.1 cat=4
step.gravel = step/gravel# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.sand = dig/sand# ; vol=1 pitch=1 var=0.1 cat=4
block.place.sand = dig/sand# ; vol=1 pitch=0.9 var=0.1 cat=4
step.sand = step/sand# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.wood = dig/wood# ; vol=1 pitch=1 var=0.1 cat=4
block.place.wood = dig/wood# ; vol=1 pitch=0.9 var=0.1 cat=4
step.wood = step/wood# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.cloth = dig/cloth# ; vol=1 pitch=1 var=0.1 cat=4
block.place.cloth = dig/cloth# ; vol=1 pitch=0.9 var=0.1 cat=4
step.cloth = step/cloth# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.snow = dig/snow# ; vol=1 pitch=1 var=0.1 cat=4
block.place.snow = dig/snow# ; vol=1 pitch=0.9 var=0.1 cat=4
step.snow = step/snow# ; vol=0.35 pitch=1 var=0.08 cat=7
block.break.glass = random/glass# ; vol=1 pitch=1 var=0.1 cat=4
block.place.glass = random/glass# ; vol=0.6 pitch=1.3 var=0.08 cat=4
step.glass = random/glass# ; vol=0.22 pitch=1.5 var=0.1 cat=7
player.hurt = random/classic_hurt ; vol=1 pitch=1 var=0.1 cat=7
player.fall.small = random/classic_hurt ; vol=0.8 pitch=1 var=0.1 cat=7
player.fall.big = random/classic_hurt ; vol=1 pitch=1 var=0.1 cat=7
eat = random/eat# ; vol=0.6 pitch=1 var=0.15 cat=7
burp = random/burp ; vol=0.5 pitch=0.9 var=0.1 cat=7
door.open = random/door_open ; vol=1 pitch=1 var=0.1 cat=4
door.close = random/door_close ; vol=1 pitch=1 var=0.1 cat=4
chest.open = random/chestopen ; vol=0.5 pitch=1 var=0.1 cat=4
chest.close = random/chestclosed ; vol=0.5 pitch=1 var=0.1 cat=4
button.click = random/click ; vol=0.3 pitch=0.6 var=0 cat=4
button.click.wood = random/wood_click ; vol=0.3 pitch=0.7 var=0 cat=4
lever.click = random/click ; vol=0.3 pitch=0.6 var=0 cat=4
fire.ignite = fire/ignite ; vol=1 pitch=1 var=0.1 cat=4
fire.extinguish = random/fizz ; vol=0.5 pitch=2.6 var=0.3 cat=4
fire.ambient = fire/fire ; vol=0.5 pitch=1 var=0.3 cat=4
lava.ambient = liquid/lava ; vol=0.4 pitch=1 var=0.2 cat=4
lava.pop = liquid/lavapop ; vol=0.4 pitch=1 var=0.2 cat=4
splash = random/splash ; vol=0.6 pitch=1 var=0.2 cat=7
weather.rain = ambient/weather/rain# ; vol=0.5 pitch=1 var=0 cat=0
weather.thunder = ambient/weather/thunder# ; vol=1 pitch=1 var=0.1 cat=0
ambient.cave = ambient/cave/cave# ; vol=0.7 pitch=1 var=0.1 cat=0
dispenser.launch = random/bow ; vol=0.4 pitch=1.2 var=0.1 cat=4
)CFG"; fp = fopen(path, "wb"); if (fp) { fputs(d, fp); fclose(fp); } std::istringstream is(d); std::string l; while (std::getline(is, l)) sndEvtLine(l); return; }
    std::string all; char tmp[4096]; size_t n; while ((n = fread(tmp, 1, sizeof tmp, fp)) > 0) all.append(tmp, n); fclose(fp);
    std::istringstream is(all); std::string l; while (std::getline(is, l)) { while (!l.empty() && (l.back() == '\r' || l.back() == ' ')) l.pop_back(); sndEvtLine(l); }
}
static const SndEvt* sndEvtGet(const char* ev) { if (!gSndEvtOk) sndEvtLoad(); auto it = gSndEvts.find(ev); return it != gSndEvts.end() && !it->second.f.empty() ? &it->second : nullptr; }
#ifdef BL_RPI
static void sndPrewarm() {   // patchRpiOpt.1: decodifica no loading os sons mais comuns (decodificar .ogg no meio do jogo travava 60-190 ms)
    // patchBreak.3: ANTES so 40 arquivos em ordem aleatoria do mapa => o som de quebrar/colocar/andar de varios materiais era decodificado na 1a vez (micro lag ao quebrar bloco). Agora TODOS os block.*/step.* primeiro, depois os outros comuns.
    if (!gSndOn) return; if (!gSndEvtOk) sndEvtLoad(); int budget = 220;
    static const char* K[] = {"dig", "hurt", "hit", "splash", "eat", "swim", "click", "pop", "grass", "stone", "wood", "gravel", "sand", "random", "item", "drop", "xp", "orb"};
    for (int pass = 0; pass < 2; pass++)
        for (auto& kv : gSndEvts) { bool ok = false;
            if (pass == 0) ok = kv.first.compare(0, 6, "block.") == 0 || kv.first.compare(0, 5, "step.") == 0;
            else for (const char* k : K) if (kv.first.find(k) != std::string::npos) { ok = true; break; }
            if (!ok) continue;
            for (auto& f : kv.second.f) { if (budget <= 0) return; if (!gSndCache.count(f)) { sndGet(f); budget--; } } }
}
#endif
static void sndEvtAt(const char* ev, double x, double y, double z, float vm = 1.f) { const SndEvt* e = sndEvtGet(ev); if (e && gSndOn) sndPlayAt(e->f[rand() % e->f.size()].c_str(), x, y, z, e->vol * vm, e->pitch * sndRandPitch(e->var), e->cat); }
static void sndEvtUi(const char* ev, float vm = 1.f) { const SndEvt* e = sndEvtGet(ev); if (e && gSndOn) sndPlay(e->f[rand() % e->f.size()].c_str(), e->vol * vm, e->pitch * sndRandPitch(e->var), e->cat); }
// ---- sons dos mobs: say = fala/idle, hurt = dano, death = morte (arquivos de 1.12.2: mob/<dir>/say1.., hurt1.., death) ----
struct MobSnd { const char* dir; const char* say; int nSay; const char* hurt; int nHurt; const char* death; int nDeath; bool idle, hostile; };   // nHurt 0 = usa say; nDeath 0 = usa hurt; nDeath -1 = arquivo unico "<death>" sem numero (patchAud.OGG: nomes reais do 1.12.2)
static const MobSnd* mobSndFor(int type) {   // indices = enum de mobs.inc (M_PIG..M_MOOSH)
    static const MobSnd T[54] = {
        {"pig", "say", 3, nullptr, 0, "death", -1, true, false},                       // M_PIG
        {"chicken", "say", 3, "hurt", 2, nullptr, 0, true, false},                     // M_CHICKEN
        {"cow", "say", 4, "hurt", 3, nullptr, 0, true, false},                         // M_COW
        {"sheep", "say", 3, nullptr, 0, nullptr, 0, true, false},                      // M_SHEEP
        {"villager", "idle", 3, "hit", 4, "death", -1, true, false},                   // M_VILLAGER
        {"zombiepig", "zpig", 4, "zpighurt", 2, "zpigdeath", -1, true, true},          // M_PIGMAN
        {"zombie", "say", 3, "hurt", 2, "death", -1, true, true},                      // M_ZOMBIE
        {"skeleton", "say", 3, "hurt", 4, "death", -1, true, true},                    // M_SKELETON
        {"creeper", "say", 4, nullptr, 0, "death", -1, false, true},                   // M_CREEPER (say = dano)
        {"blaze", "breathe", 4, "hit", 4, "death", -1, true, true},                    // M_BLAZE
        {"spider", "say", 4, nullptr, 0, "death", -1, true, true},                     // M_SPIDER
        {"husk", "idle", 3, "hurt", 2, "death", 2, true, true},                        // M_HUSK
        {"spider", "say", 4, nullptr, 0, "death", -1, true, true},                     // M_CSPIDER
        {"cow", "say", 4, "hurt", 3, nullptr, 0, true, false},                       // M_MOOSH
        {"endermen", "idle", 4, "hit", 4, "death", -1, true, true},                   // M_ENDERMAN (patchEnd.12; sem arquivos no pack = mudo)
        {"slime", "big", 4, "big", 4, "big", 4, false, true},                          // M_SLIME (patchSlime.2; sem arquivos no pack = mudo)
        {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false},   // M_RABBIT (patchRabbit.1; sem arquivos no pack = mudo)
        {"cat", "meow", 4, "hitt", 3, nullptr, 0, true, false},   // M_OCELOT (patchOcelot.1; sem arquivos no pack = mudo)
        {"wolf", "growl", 3, "hurt", 3, "death", -1, true, false},   // M_WOLF (patchWolf.1; sem arquivos no pack = mudo)
        {"stray", "idle", 4, "hurt", 4, "death", 2, true, true},   // M_STRAY (patchMob112.1)
        {"wither_skeleton", "idle", 3, "hurt", 4, "death", 2, true, true},   // M_WSKEL
        {"zombie", "say", 3, "hurt", 2, "death", -1, true, true},   // M_ZVILL
        {"silverfish", "say", 4, "hit", 3, "kill", -1, true, true},   // M_SILVER
        {"endermite", "say", 4, "hit", 4, "kill", -1, true, true},   // M_ENDERMITE
        {"magmacube", "big", 4, "big", 4, "big", 4, false, true},   // M_MAGMA
        {"squid", "ambient", 3, "hurt", 4, nullptr, 0, true, false},   // M_SQUID (patchMob112.5; sem arquivo = mudo)
        {"witch", "idle", 3, "hurt", 3, "death", -1, true, true},   // M_WITCH
        {"vindication", "idle", 3, "hurt", 3, "death", -1, true, true},   // M_VINDIC
        {"evocation_illager", "ambient", 3, "hurt", 3, "death", -1, true, true},   // M_EVOKER
        {"illusion_illager", "ambient", 3, "hurt", 3, "death", -1, true, true},   // M_ILLUS
        {"cat", "meow", 4, "hitt", 3, nullptr, 0, true, false}, {"cat", "meow", 4, "hitt", 3, nullptr, 0, true, false}, {"cat", "meow", 4, "hitt", 3, nullptr, 0, true, false},   // gatos
        {"irongolem", "throw", 1, "hit", 4, "death", -1, false, true}, {"parrot", "idle", 5, "hurt", 3, "death", -1, true, false},   // M_IGOLEM, M_PARROT (sem arquivo = mudo)
        {"ghast", "moan", 7, "scream", 5, "death", -1, true, true}, {"shulker", "ambient", 3, "hurt", 3, "death", -1, true, true},   // M_GHAST, M_SHULKER
        {"snowgolem", "ambient", 1, "hurt", 3, "death", -1, true, false},   // M_SNOWGOLEM (sem arquivo = mudo)
        {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false}, {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false}, {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false}, {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false}, {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false},
        {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false}, {"rabbit", "idle", 4, "hurt", 4, "death", 4, true, false},   // coelho assassino/toast
        {"villager", "idle", 3, "hit", 4, "death", -1, true, false}, {"villager", "idle", 3, "hit", 4, "death", -1, true, false}, {"villager", "idle", 3, "hit", 4, "death", -1, true, false}, {"villager", "idle", 3, "hit", 4, "death", -1, true, false}, {"villager", "idle", 3, "hit", 4, "death", -1, true, false},   // profissoes
        {"guardian", "land_idle", 3, "land_hit", 3, "land_death", -1, true, true}, {"guardian", "elder_idle", 3, "elder_hit", 3, "elder_death", -1, true, true},
        {"bat", "idle", 4, "hurt", 4, "death", -1, true, false}, {"polarbear", "idle", 3, "hurt", 3, "death", 3, true, false}};   // M_BAT, M_POLAR (patchMobNew.1; sem arquivos no pack = mudo)   // M_GUARD, M_ELDER
    return type >= 0 && type < 54 ? &T[type] : nullptr;
}
static void sndMob(int type, double x, double y, double z, int kind) {   // kind: 0 = say, 1 = hurt, 2 = death
    const MobSnd* m = mobSndFor(type); if (!m || !gSndOn) return; const std::string base = std::string("mob/") + m->dir + "/"; std::string n;
    if (kind == 2 && m->nDeath == -1) n = base + m->death;
    else if (kind == 2 && m->nDeath > 0) n = sndVar((base + m->death).c_str(), m->nDeath);
    else if (kind >= 1 && m->nHurt > 0) n = sndVar((base + m->hurt).c_str(), m->nHurt);
    else if (kind == 1 && m->idle == false) n = sndVar((base + m->say).c_str(), m->nSay);
    else n = sndVar((base + m->say).c_str(), m->nSay);
    sndPlayAt(n.c_str(), x, y, z, 1.f, kind == 0 ? sndRandPitch(.1f) : sndRandPitch(.15f), m->hostile ? 5 : 6);   // hostis = m5; amigaveis = m6
}
// ---- por tick de jogo (20 Hz): ouvinte, passos e musica ----
static bool gJukeBusy = false;   // patchJuke.2: toca-discos tocando => musica de fundo cala (como no Java)
struct SndMusicJob; static std::shared_ptr<SndMusicJob> gSndJob; static bool gSndJobCancel = false;   // patchPerf.1: faixa em decodificacao (thread)
static void sndMusicStop() {   // para a faixa e libera a memoria (ao voltar para o titulo)
    if (!gSndOn) return; SDL_LockAudioDevice(gSndDev); for (SndVoice& v : gSndV) if (v.music) v.b = nullptr; SDL_UnlockAudioDevice(gSndDev);
    if (gSndJob) gSndJobCancel = true;   // ainda decodificando: descarta o resultado quando terminar
    if (!gSndMusicName.empty() && !gSndJob) { gSndCache.erase(gSndMusicName); gSndMusicName.clear(); } gSndMusicT = 100;
}
static int gMusicCtx = 0;   // patchMus.1: 0 superficie (sobrevivencia), 1 criativo, 2 nether, 3 end (main_loop ajusta a cada tick)
static std::vector<std::string> sndMusicList() {   // faixas do contexto (assets/sounds/music/game/...; as que faltarem sao puladas)
    std::vector<std::string> v;
    if (gMusicCtx == 2) { for (int i = 1; i <= 4; i++) v.push_back("music/game/nether/nether" + std::to_string(i)); return v; }
    if (gMusicCtx == 3) { v.push_back("music/game/end/end"); return v; }
    for (int i = 1; i <= 3; i++) v.push_back("music/game/calm" + std::to_string(i));
    for (int i = 1; i <= 4; i++) v.push_back("music/game/hal" + std::to_string(i));
    for (int i = 1; i <= 2; i++) v.push_back("music/game/nuance" + std::to_string(i));
    for (int i = 1; i <= 3; i++) v.push_back("music/game/piano" + std::to_string(i));
    if (gMusicCtx == 1) for (int i = 1; i <= 6; i++) v.push_back("music/game/creative/creative" + std::to_string(i));
    return v;
}
struct SndMusicJob { std::string name; SndBuf buf; std::atomic<int> st{0}; double ms = 0; };   // st: 1 decodificando, 2 pronto, 3 falhou
static int sndMusicThread(void* p) {   // patchPerf.1: a faixa (minutos de som) era decodificada na thread principal = quadro de 1,5-2,6 s
    auto* sp = (std::shared_ptr<SndMusicJob>*)p; std::shared_ptr<SndMusicJob> j = *sp; delete sp;
#ifdef BL_RPI
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);   // patchRpiOpt.8: decodificar a faixa inteira nao pode roubar CPU da thread de render/audio no Pi
#endif
    const auto t0 = std::chrono::steady_clock::now(); std::vector<uint8_t> f; bool ok = false;
    if (packGetFile("assets/minecraft/sounds/" + j->name + ".ogg", f)) { int ch = 0, rate = 0; ok = vbDecodeOgg(f, j->buf.pcm, ch, rate); if (ok) { j->buf.ch = ch; j->buf.rate = rate; j->buf.frames = j->buf.pcm.size() / ch; j->buf.ok = true; } }
    j->ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); j->st = ok ? 2 : 3; return 0;
}
static bool sndMusicStart(int t) {   // toca uma faixa agora (t = 0: sorteia entre as que existem, sem repetir a ultima; t = N: N-esima da lista); false se sem audio/arquivo
    if (!gSndOn) return false; if (gSndJob) return false;   // ja carregando uma faixa
    sndMusicStop();
    const std::vector<std::string> L = sndMusicList(); std::vector<int> ok; for (int i = 0; i < (int)L.size(); i++) if (sndFileOk(L[i])) ok.push_back(i);
    if (ok.empty()) return false; int k;
    if (t >= 1 && t <= (int)L.size() && sndFileOk(L[t - 1])) k = t - 1;
    else { do k = ok[rand() % ok.size()]; while (ok.size() > 1 && k + 1 == gSndMusicLast); }
    gSndMusicLast = k + 1; gSndMusicName = L[k];
    auto j = std::make_shared<SndMusicJob>(); j->name = L[k]; j->st = 1; gSndJob = j; gSndJobCancel = false;
    SDL_Thread* th = SDL_CreateThread(sndMusicThread, "music", new std::shared_ptr<SndMusicJob>(j));
    if (!th) { gSndJob.reset(); gSndMusicName.clear(); return false; } SDL_DetachThread(th);
    logInfo("Musica: decodificando '%s' em segundo plano", L[k].c_str()); return true;
}
static void sndMusicPoll() {   // chamada por sndTick: quando a thread termina, coloca no cache e toca
    if (!gSndJob || gSndJob->st == 1) return;
    std::shared_ptr<SndMusicJob> j = gSndJob; gSndJob.reset();
    if (gSndJobCancel || j->st != 2) { if (j->st != 2) logWarn("Musica invalida: %s", j->name.c_str()); gSndMusicName.clear(); gSndMusicT = 600; return; }
    SndBuf& b = gSndCache[j->name]; b = std::move(j->buf); b.ok = true;
    logInfo("Musica: %s pronta (%.0f ms em segundo plano, %.0f s)", j->name.c_str(), j->ms, b.rate ? b.frames / (double)b.rate : 0.0);
    const float v = gSndCat[1] * gSndMusicVol; sndPlayBuf(&b, v, v, 1.f, true);
}
static bool sndMusicPlaying() { for (const SndVoice& v : gSndV) if (v.music && v.b) return true; return false; }
static void sndTick(double x, double y, double z, float yaw, float distWalked, bool walking) {
    if (!gSndOn) return; gSndLX = x; gSndLY = y; gSndLZ = z; gSndYaw = yaw;
    static float last = 0; float dd = distWalked - last; last = distWalked; if (dd < 0.f || dd > 5.f) dd = 0.f;
    // passos: a cada 1 unidade de distWalked (~1,67 blocos), como o nextStepDistance do Java; so no chao e sem agachar
    static bool lastG = true;   // patchAud.4: correr+pular: a distancia acumula tambem no ar e o passo sai ao pisar/pousar (antes so contava com o pe no chao => ficava mudo)
    if (walking) { gSndStepAcc += dd; if (gSndOnG && (gSndStepAcc > 1.f || !lastG)) { gSndStepAcc = fmodf(gSndStepAcc, 1.f); sndEvtUi(SND_STP[gSndStepMat & 7]); } }
    else gSndStepAcc = std::min(gSndStepAcc, 1.f);
    lastG = gSndOnG;
    // musica (MusicTicker do Java: 1a faixa ~5 s apos entrar; depois de cada faixa espera 10-20 min; slider Musica = 0 nao toca)
    if (gJukeBusy) { if (!gSndMusicName.empty()) sndMusicStop(); gSndMusicT = std::max(gSndMusicT, 600); }
    { const float mv = gSndCat[1] * gSndMusicVol; for (SndVoice& v : gSndV) if (v.music && v.b && v.vl != mv) { SDL_LockAudioDevice(gSndDev); v.vl = v.vr = mv; SDL_UnlockAudioDevice(gSndDev); } }   // slider ao vivo
    sndMusicPoll();
    if (gSndJob) return;   // carregando a faixa em segundo plano
    if (!gSndMusicName.empty()) { if (!sndMusicPlaying()) { gSndCache.erase(gSndMusicName); gSndMusicName.clear(); gSndMusicT = 12000 + rand() % 12000; } }
    else if (--gSndMusicT <= 0) { gSndMusicT = 200; if (gSndCat[1] > .01f && sndMusicStart(0)) gSndMusicT = 6000; }
}
