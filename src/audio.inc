// audio.inc - SONS (patchRPI.11). Mixer proprio sobre SDL_OpenAudioDevice (sem SDL_mixer).
// Os .ogg originais do Minecraft 1.12.2 foram convertidos para WAV IMA-ADPCM em assets/sounds/<pasta>/<nome>.wav
// (o jogo nao tem decodificador de OGG); este arquivo le WAV PCM16 e IMA-ADPCM (mono/estereo) e mistura ate 24 vozes.
// Variaveis de ambiente: BLOCKLAND_MUTE=1 desliga o audio. Sem sons no disco o jogo segue mudo (nada falha).
#ifndef SDLCALL
#define SDLCALL
#endif
struct SndBuf { std::vector<int16_t> pcm; int ch = 1, rate = 44100; size_t frames = 0; bool ok = false; };
struct SndVoice { SndBuf* b = nullptr; double pos = 0, step = 1; float vl = 1, vr = 1; bool music = false; };
static const int8_t SND_IMA_IDX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int16_t SND_IMA_STEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static SDL_AudioDeviceID gSndDev = 0; static int gSndRate = 44100; static bool gSndOn = false;
static SndVoice gSndV[24]; static std::map<std::string, SndBuf> gSndCache;
static float gSndMaster = 1.f, gSndMusicVol = .5f;
static float gSndCat[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};   // volumes por categoria = sliders m1..m8 de Opcoes > Musica & Sons (1 Musica, 4 Blocos, 5 Hostis, 6 Amigaveis, 7 Jogadores); m0 = gSndMaster (ver sndSyncVol em screens.inc)
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
static SndBuf* sndGet(const std::string& name) {   // carrega sob demanda: assets/sounds/<name>.wav (cache; falha tambem fica em cache)
    auto it = gSndCache.find(name); if (it != gSndCache.end()) return it->second.ok ? &it->second : nullptr;
    SndBuf& b = gSndCache[name]; FILE* fp = fopen(("assets/sounds/" + name + ".wav").c_str(), "rb"); if (!fp) return nullptr;
    std::vector<uint8_t> f; uint8_t tmp[16384]; size_t n; while ((n = fread(tmp, 1, sizeof tmp, fp)) > 0) f.insert(f.end(), tmp, tmp + n); fclose(fp);
    b.ok = sndParseWav(f, b); if (!b.ok) { logWarn("Som invalido: %s", name.c_str()); return nullptr; }
    return &b;
}
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
    int16_t* o = (int16_t*)stream;
    for (int i = 0; i < n * 2; i++) { const float s = acc[i] * gSndMaster; o[i] = (int16_t)(s > 32767.f ? 32767 : s < -32768.f ? -32768 : s); }
}
static void sndInit() {
    const char* mute = getenv("BLOCKLAND_MUTE"); if (mute && *mute == '1') { logInfo("Audio desligado (BLOCKLAND_MUTE=1)"); return; }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { logWarn("Audio indisponivel: %s", SDL_GetError()); return; }
    SDL_AudioSpec want, have; memset(&want, 0, sizeof want); want.freq = 44100; want.format = AUDIO_S16SYS; want.channels = 2;
#ifdef BL_RPI
    want.samples = 2048;   // Pi 3B: buffer maior evita estalos
#else
    want.samples = 1024;
#endif
    want.callback = sndCallback;
    gSndDev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!gSndDev) { logWarn("Nao foi possivel abrir o dispositivo de audio: %s", SDL_GetError()); return; }
    gSndRate = have.freq > 0 ? have.freq : 44100; gSndOn = true; SDL_PauseAudioDevice(gSndDev, 0);
    logInfo("Audio: %d Hz, 24 vozes", gSndRate);
}
static void sndPlayBuf(SndBuf* b, float vl, float vr, float pitch, bool music) {
    if (!gSndOn || !b || (vl <= .003f && vr <= .003f)) return;
    SDL_LockAudioDevice(gSndDev);
    int slot = -1; double best = -1; for (int i = 0; i < 24; i++) { if (!gSndV[i].b) { slot = i; break; } if (!gSndV[i].music && gSndV[i].b->frames) { const double pr = gSndV[i].pos / (double)gSndV[i].b->frames; if (pr > best) { best = pr; slot = i; } } }   // sem vaga: toma a voz mais adiantada (nunca a musica)
    if (slot >= 0) { SndVoice& v = gSndV[slot]; v.b = nullptr; v.pos = 0; v.step = (double)b->rate / gSndRate * pitch; v.vl = vl; v.vr = vr; v.music = music; v.b = b; }
    SDL_UnlockAudioDevice(gSndDev);
}
static void sndPlay(const char* name, float vol = 1.f, float pitch = 1.f, int cat = 0) { if (gSndOn) { if (cat > 0 && cat < 9) vol *= gSndCat[cat]; sndPlayBuf(sndGet(name), vol, vol, pitch, false); } }   // som de interface (sem posicao)
static void sndPlayAt(const char* name, double x, double y, double z, float vol = 1.f, float pitch = 1.f, int cat = 0) {   // som no mundo: atenuacao linear em 16 blocos (x vol) e pan esquerda/direita
    if (!gSndOn) return; if (cat > 0 && cat < 9) vol *= gSndCat[cat];
    const double dx = x - gSndLX, dy = y - gSndLY, dz = z - gSndLZ, d = sqrt(dx * dx + dy * dy + dz * dz), rng = 16.0 * std::max(1.f, vol);
    if (d >= rng) return; const float g = vol * (float)(1.0 - d / rng) * (float)(1.0 - d / rng * .3);
    float pan = 0.f; if (d > .5) { const float yr = gSndYaw * .0174533f; pan = (float)((dx * cosf(yr) - dz * sinf(yr)) / d); }   // direita do ouvinte = (cos yaw, -sin yaw)
    sndPlayBuf(sndGet(name), pan > 0 ? g * (1.f - pan * .6f) : g, pan < 0 ? g * (1.f + pan * .6f) : g, pitch, false);
}
static std::string sndVar(const char* base, int n) { char b[96]; snprintf(b, sizeof b, "%s%d", base, 1 + rand() % std::max(1, n)); return b; }   // base + numero (1..n)
static float sndRandPitch(float spread = .1f) { return 1.f + ((rand() % 2001) / 1000.f - 1.f) * spread; }
// ---- eventos de som: assets/sounds/events.cfg liga "evento = arquivos ; vol pitch var cat" (facil de trocar; sndEvtAt/sndEvtUi) ----
struct SndEvt { std::vector<std::string> f; float vol = 1.f, pitch = 1.f, var = .1f; int cat = 0; };
static std::map<std::string, SndEvt> gSndEvts; static bool gSndEvtOk = false;
static bool sndFileOk(const std::string& n) { FILE* fp = fopen(("assets/sounds/" + n + ".wav").c_str(), "rb"); if (fp) fclose(fp); return fp != nullptr; }
static void sndEvtLine(const std::string& ln) {
    if (ln.empty() || ln[0] == '#') return; const size_t eq = ln.find('='); if (eq == std::string::npos) return;
    std::string key = ln.substr(0, eq); while (!key.empty() && key.back() == ' ') key.pop_back(); SndEvt e; std::istringstream ss(ln.substr(eq + 1)); std::string t;
    while (ss >> t) {
        if (t == ";" || t == "|") continue; const size_t q = t.find('=');
        if (q != std::string::npos) { const float v = (float)atof(t.c_str() + q + 1), k = t.compare(0, q, "vol") == 0 ? 0.f : t.compare(0, q, "pitch") == 0 ? 1.f : t.compare(0, q, "var") == 0 ? 2.f : t.compare(0, q, "cat") == 0 ? 3.f : -1.f;
            if (k == 0.f) e.vol = v; else if (k == 1.f) e.pitch = v; else if (k == 2.f) e.var = v; else if (k == 3.f) e.cat = (int)v; continue; }
        if (t.back() == '#') { const std::string b = t.substr(0, t.size() - 1); for (int i = 1; i < 10 && sndFileOk(b + std::to_string(i)); i++) e.f.push_back(b + std::to_string(i)); }
        else if (sndFileOk(t)) e.f.push_back(t); else logWarn("events.cfg: som nao encontrado (.wav): %s", t.c_str());
    }
    gSndEvts[key] = e;
}
static void sndEvtLoad() {
    gSndEvtOk = true; const char* path = "assets/sounds/events.cfg"; FILE* fp = fopen(path, "rb");
    if (!fp) { const char* d = R"CFG(# Blockland - eventos de som (este arquivo e recriado se for apagado).
# Formato:  evento = arquivo1 arquivo2 ... ; vol=1 pitch=1 var=0.1 cat=4
#  - arquivo = caminho dentro de assets/sounds SEM extensao (le .wav). "nome#" = nome1, nome2, nome3... (sorteia um).
#  - Para usar outro som: ponha o arquivo em assets/sounds (se for .ogg/.mp3 rode tools/ogg2wav.sh) e troque o nome na linha.
#  - vol = volume, pitch = tom, var = variacao aleatoria do tom, cat = slider de Opcoes (0 so o principal, 1 musica, 4 blocos, 5 hostis, 6 amigaveis, 7 jogadores).
ui.click = random/click ; vol=0.8 cat=0
block.break = step/stone# ; vol=1 pitch=0.8 var=0.1 cat=4
block.break.grass = dig/grass# ; vol=1 pitch=1 var=0.1 cat=4
block.place = step/stone# ; vol=1 pitch=0.8 var=0.1 cat=4
block.place.grass = dig/grass# ; vol=1 pitch=0.9 var=0.1 cat=4
item.pickup = random/pop ; vol=0.3 pitch=1 var=0.5 cat=7
item.drop = random/pop ; vol=0.3 pitch=0.7 var=0.1 cat=7
egg.throw = random/bow ; vol=0.5 pitch=0.5 var=0.2 cat=7
egg.hit = random/pop ; vol=0.6 pitch=1 var=0.1 cat=7
)CFG"; fp = fopen(path, "wb"); if (fp) { fputs(d, fp); fclose(fp); } std::istringstream is(d); std::string l; while (std::getline(is, l)) sndEvtLine(l); return; }
    std::string all; char tmp[4096]; size_t n; while ((n = fread(tmp, 1, sizeof tmp, fp)) > 0) all.append(tmp, n); fclose(fp);
    std::istringstream is(all); std::string l; while (std::getline(is, l)) { while (!l.empty() && (l.back() == '\r' || l.back() == ' ')) l.pop_back(); sndEvtLine(l); }
}
static const SndEvt* sndEvtGet(const char* ev) { if (!gSndEvtOk) sndEvtLoad(); auto it = gSndEvts.find(ev); return it != gSndEvts.end() && !it->second.f.empty() ? &it->second : nullptr; }
static void sndEvtAt(const char* ev, double x, double y, double z, float vm = 1.f) { const SndEvt* e = sndEvtGet(ev); if (e && gSndOn) sndPlayAt(e->f[rand() % e->f.size()].c_str(), x, y, z, e->vol * vm, e->pitch * sndRandPitch(e->var), e->cat); }
static void sndEvtUi(const char* ev, float vm = 1.f) { const SndEvt* e = sndEvtGet(ev); if (e && gSndOn) sndPlay(e->f[rand() % e->f.size()].c_str(), e->vol * vm, e->pitch * sndRandPitch(e->var), e->cat); }
// ---- sons dos mobs: say = fala/idle, hurt = dano, death = morte (arquivos de 1.12.2: mob/<dir>/say1.., hurt1.., death) ----
struct MobSnd { const char* dir; int nSay, nHurt, nDeath; bool idle; };   // nHurt 0 = usa say; nDeath 0 = usa hurt; nDeath 1 = arquivo "death"
static const MobSnd* mobSndFor(int type) {   // indices = enum de mobs.inc (M_PIG, M_CHICKEN, M_COW, M_SHEEP, M_VILLAGER, M_PIGMAN, M_ZOMBIE, M_SKELETON, M_CREEPER)
    static const MobSnd T[9] = {{"pig", 3, 0, 1, true}, {"chicken", 3, 2, 0, true}, {"cow", 4, 3, 0, true}, {nullptr, 0, 0, 0, false}, {nullptr, 0, 0, 0, false}, {nullptr, 0, 0, 0, false},
                                {"zombie", 3, 2, 1, true}, {"skeleton", 3, 4, 1, true}, {"creeper", 4, 0, 1, false}};
    return type >= 0 && type < 9 && T[type].dir ? &T[type] : nullptr;
}
static void sndMob(int type, double x, double y, double z, int kind) {   // kind: 0 = say, 1 = hurt, 2 = death
    const MobSnd* m = mobSndFor(type); if (!m || !gSndOn) return; std::string base = std::string("mob/") + m->dir + "/"; std::string n;
    if (kind == 2 && m->nDeath == 1) n = base + "death";
    else if (kind >= 1 && m->nHurt > 0) n = sndVar((base + "hurt").c_str(), m->nHurt);
    else n = sndVar((base + "say").c_str(), m->nSay);
    sndPlayAt(n.c_str(), x, y, z, 1.f, kind == 0 ? sndRandPitch(.1f) : sndRandPitch(.15f), type == 6 || type == 7 || type == 8 ? 5 : 6);   // hostis (zumbi/esqueleto/creeper) = m5; animais = m6
}
// ---- por tick de jogo (20 Hz): ouvinte, passos e musica ----
static void sndMusicStop() {   // para a faixa e libera a memoria (ao voltar para o titulo)
    if (!gSndOn) return; SDL_LockAudioDevice(gSndDev); for (SndVoice& v : gSndV) if (v.music) v.b = nullptr; SDL_UnlockAudioDevice(gSndDev);
    if (!gSndMusicName.empty()) { gSndCache.erase(gSndMusicName); gSndMusicName.clear(); } gSndMusicT = 100;
}
static bool sndMusicStart(int t) {   // toca calm1/calm2 agora (t = 0: sorteia, nunca repetindo a ultima); false se sem audio/arquivo
    if (!gSndOn) return false; sndMusicStop();
    if (t < 1 || t > 2) t = gSndMusicLast == 1 ? 2 : gSndMusicLast == 2 ? 1 : 1 + rand() % 2;
    const std::string n = std::string("music/game/calm") + (char)('0' + t); SndBuf* b = sndGet(n); if (!b) return false;
    gSndMusicLast = t; gSndMusicName = n; const float v = gSndCat[1] * gSndMusicVol; sndPlayBuf(b, v, v, 1.f, true); return true;
}
static bool sndMusicPlaying() { for (const SndVoice& v : gSndV) if (v.music && v.b) return true; return false; }
static void sndTick(double x, double y, double z, float yaw, float distWalked, bool walking) {
    if (!gSndOn) return; gSndLX = x; gSndLY = y; gSndLZ = z; gSndYaw = yaw;
    static float last = 0; float dd = distWalked - last; last = distWalked; if (dd < 0.f || dd > 5.f) dd = 0.f;
    // passos: a cada 1 unidade de distWalked (~1,67 blocos), como o nextStepDistance do Java; so no chao e sem agachar
    if (walking) { gSndStepAcc += dd; if (gSndStepAcc > 1.f) { gSndStepAcc = fmodf(gSndStepAcc, 1.f); const std::string n = sndVar("step/stone", 3); sndPlay(n.c_str(), .35f, sndRandPitch(.08f)); } }
    else gSndStepAcc = std::min(gSndStepAcc, 1.f);
    // musica (MusicTicker do Java: 1a faixa ~5 s apos entrar; depois de cada faixa espera 10-20 min; slider Musica = 0 nao toca)
    { const float mv = gSndCat[1] * gSndMusicVol; for (SndVoice& v : gSndV) if (v.music && v.b && v.vl != mv) { SDL_LockAudioDevice(gSndDev); v.vl = v.vr = mv; SDL_UnlockAudioDevice(gSndDev); } }   // slider ao vivo
    if (!gSndMusicName.empty()) { if (!sndMusicPlaying()) { gSndCache.erase(gSndMusicName); gSndMusicName.clear(); gSndMusicT = 12000 + rand() % 12000; } }
    else if (--gSndMusicT <= 0) { gSndMusicT = 200; if (gSndCat[1] > .01f && sndMusicStart(0)) gSndMusicT = 6000; }
}
