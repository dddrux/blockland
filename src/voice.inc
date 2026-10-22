// voice.inc - VOICE CHAT (patchVoice.1). So SDL2 (captura SDL_OpenAudioDevice) + o UDP do net.inc: igual em Linux, Windows, Android (32/64 bits) e Raspberry Pi.
// Formato: 8000 Hz mono, quadros de 40 ms (320 amostras) em u-law (G.711, 1 byte/amostra) = ~8 KB/s por quem fala. O SDL converte a taxa do microfone sozinho.
// So o jogador do TECLADO (J1) fala/ouve: o split screen nao tem voz (um unico microfone por processo). O host ja repassa todo pacote aos outros convidados.
// Voz proxima: volume cai linear de 8 a 48 blocos + pan esquerda/direita. Mic desligado por padrao; V (PC) ou botao de microfone (Android) liga/desliga.
#include <mutex>
static const int VC_RATE = 8000, VC_FRAME = 320, VC_MAXQ = VC_FRAME * 8;   // 8 quadros (320 ms) no maximo na fila de quem fala
static const float VC_NEAR = 8.f, VC_FAR = 48.f;
static bool gVoiceOn = false;                 // mic ligado (transmitindo quando ha rede)
static float gVoiceVol = 1.f;                 // volume geral das vozes recebidas
static SDL_AudioDeviceID gVcDev = 0;          // captura
static std::mutex gVcMx;                      // protege gVcCap e gVcSpk (thread de audio x thread principal)
static std::vector<int16_t> gVcCap;           // amostras capturadas ainda nao enviadas
struct VcSpk { std::deque<float> q; double frac = 0; bool active = false; float gl = 1.f, gr = 1.f; Uint32 last = 0; uint16_t seq = 0; };
static std::map<int, VcSpk> gVcSpk;           // id de rede -> voz recebida
static Uint32 gVcHang = 0, gVcWarnT = 0; static uint16_t gVcSeq = 0; static bool gVcFail = false, gVcAsked = false;
static void voiceMix(float* acc, int n);      // chamado por sndCallback (audio.inc)

static inline uint8_t vcEnc(int s) {          // PCM16 -> u-law
    const int BIAS = 0x84, CLIP = 32635; int sign = (s >> 8) & 0x80; if (sign) s = -s; if (s > CLIP) s = CLIP; s += BIAS;
    int ex = 7; for (int m = 0x4000; (s & m) == 0 && ex > 0; m >>= 1) ex--;
    return (uint8_t)~(sign | (ex << 4) | ((s >> (ex + 3)) & 0x0F));
}
static inline int vcDec(uint8_t u) {          // u-law -> PCM16
    u = (uint8_t)~u; const int sign = u & 0x80, ex = (u >> 4) & 7, mant = u & 15; int s = (((mant << 3) + 0x84) << ex) - 0x84; return sign ? -s : s;
}
static void SDLCALL vcCapCb(void*, Uint8* stream, int len) {   // thread de audio: so guarda as amostras
    std::lock_guard<std::mutex> lk(gVcMx); const int16_t* p = (const int16_t*)stream; const int n = len / 2;
    if ((int)gVcCap.size() + n > VC_FRAME * 16) gVcCap.clear();   // main thread parada: descarta em vez de crescer
    gVcCap.insert(gVcCap.end(), p, p + n);
}
static void voiceAskPermission() {            // Android 6+: pede RECORD_AUDIO sem Java novo (o manifest precisa declarar a permissao: tools/android-voice/LEIA-ME.txt)
#ifdef __ANDROID__
    if (gVcAsked) return; gVcAsked = true;
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) return;
    jclass cl = env->GetObjectClass(act); jmethodID chk = cl ? env->GetMethodID(cl, "checkSelfPermission", "(Ljava/lang/String;)I") : nullptr, req = cl ? env->GetMethodID(cl, "requestPermissions", "([Ljava/lang/String;I)V") : nullptr;
    if (!chk || !req) { if (env->ExceptionCheck()) env->ExceptionClear(); return; }   // Android < 6: a permissao do manifest basta
    jstring perm = env->NewStringUTF("android.permission.RECORD_AUDIO");
    if (env->CallIntMethod(act, chk, perm) != 0) {
        jobjectArray arr = env->NewObjectArray(1, env->FindClass("java/lang/String"), perm); env->SetObjectArrayElement(arr, 0, perm);
        env->CallVoidMethod(act, req, arr, 4711); gVcFail = false;
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
#endif
}
static bool voiceOpenCap() {
    if (gVcDev) return true;
    voiceAskPermission();
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { logWarn("Voice chat: audio indisponivel: %s", SDL_GetError()); return false; }
    SDL_AudioSpec want, have; memset(&want, 0, sizeof want); want.freq = VC_RATE; want.format = AUDIO_S16SYS; want.channels = 1;
#ifdef BL_RPI
    want.samples = 1024;
#else
    want.samples = 512;
#endif
    want.callback = vcCapCb;
    gVcDev = SDL_OpenAudioDevice(nullptr, 1, &want, &have, 0);   // 0 = o SDL converte formato/taxa/canais para o que pedimos
    if (!gVcDev) { logWarn("Voice chat: sem microfone/permissao: %s", SDL_GetError()); return false; }
    { std::lock_guard<std::mutex> lk(gVcMx); gVcCap.clear(); }
    SDL_PauseAudioDevice(gVcDev, 0); logInfo("Voice chat: microfone aberto (%d Hz, %d canal)", have.freq, (int)have.channels); return true;
}
static void voiceCloseCap() { if (gVcDev) { SDL_CloseAudioDevice(gVcDev); gVcDev = 0; } std::lock_guard<std::mutex> lk(gVcMx); gVcCap.clear(); }
static void voiceSet(bool on, bool say = true) {   // liga/desliga o microfone (V no PC, botao no Android)
    if (on == gVoiceOn) return;
    gVoiceOn = on;
    if (on) { if (!voiceOpenCap()) { gVoiceOn = false; gVcFail = true; if (say) chatAdd("Voice chat: microfone indisponivel (permissao ou dispositivo).", 0xFF5555); return; } }
    else voiceCloseCap();
    if (say) chatAdd(on ? "Voice chat: microfone LIGADO" : "Voice chat: microfone desligado", on ? 0x55FF55 : 0xAAAAAA);
}
static bool voiceTalking() { return gVoiceOn && SDL_GetTicks() - gVcHang < 300; }
static void voiceTick(Uint32 now) {            // 1x por frame (thread principal): envia o que o mic captou e atualiza volume/pan de quem fala
    if (!netOn) { if (gVcDev) voiceCloseCap(); if (gVoiceOn) gVoiceOn = false; }   // saiu da partida: mic desliga
    if (gVoiceOn && gVcDev) {
        for (;;) {
            int16_t fr[VC_FRAME]; { std::lock_guard<std::mutex> lk(gVcMx); if ((int)gVcCap.size() < VC_FRAME) break; memcpy(fr, gVcCap.data(), sizeof fr); gVcCap.erase(gVcCap.begin(), gVcCap.begin() + VC_FRAME); }
            double e = 0; int pk = 0; for (int i = 0; i < VC_FRAME; i++) { e += (double)fr[i] * fr[i]; pk = std::max(pk, std::abs((int)fr[i])); }
            const double rms = sqrt(e / VC_FRAME);
            if (rms > 450.0 || pk > 3500) gVcHang = now;   // porta de ruido: fala = acima do limiar; mantem 300 ms depois (nao corta o fim das palavras)
            if (now - gVcHang > 300) continue;
            NBuf b = netMk(NP_VOICE); b.put<uint16_t>(gVcSeq++);
            for (int i = 0; i < VC_FRAME; i++) { int s = (int)(fr[i] * 2); s = std::max(-32000, std::min(32000, s)); b.put<uint8_t>(vcEnc(s)); }   // ganho x2 (microfones de celular sao baixos)
            netBroadcast(b);
        }
    }
    std::lock_guard<std::mutex> lk(gVcMx);
    for (auto it = gVcSpk.begin(); it != gVcSpk.end();) {
        VcSpk& s = it->second; if (now - s.last > 3000) { it = gVcSpk.erase(it); continue; }
        auto r = remotes.find(it->first); float g = .8f, pan = 0.f;
        if (r != remotes.end()) {
            const double dx = r->second.x - gSndLX, dy = r->second.y - gSndLY, dz = r->second.z - gSndLZ, d = sqrt(dx * dx + dy * dy + dz * dz);
            g = d <= VC_NEAR ? 1.f : d >= VC_FAR ? 0.f : (float)(1.0 - (d - VC_NEAR) / (VC_FAR - VC_NEAR));
            if (d > .5) { const float yr = gSndYaw * .0174533f; pan = (float)((dx * cosf(yr) - dz * sinf(yr)) / d); }
        }
        g *= gVoiceVol; s.gl = g * (pan > 0 ? 1.f - pan * .6f : 1.f); s.gr = g * (pan < 0 ? 1.f + pan * .6f : 1.f); ++it;
    }
}
static void voiceNetRecv(NBuf& b, int sender) {   // NP_VOICE: seq(u16) + ate VC_FRAME bytes u-law
    if (sender == netSelf || sender < 0 || sender >= 1000) return;
    const uint16_t seq = b.get<uint16_t>(); const int n = std::min(VC_FRAME, b.n - b.r); if (n <= 0) return;
    std::lock_guard<std::mutex> lk(gVcMx); VcSpk& s = gVcSpk[sender];
    if (s.last && (int16_t)(seq - s.seq) <= 0 && SDL_GetTicks() - s.last < 1000) return;   // pacote velho/duplicado
    s.seq = seq; s.last = SDL_GetTicks();
    for (int i = 0; i < n; i++) s.q.push_back((float)vcDec(b.d[b.r + i]));
    while ((int)s.q.size() > VC_MAXQ) s.q.pop_front();   // atraso demais: descarta o mais antigo
    if (!s.active && (int)s.q.size() >= VC_FRAME * 2) s.active = true;   // so toca com 80 ms na frente (jitter)
}
static void voiceMix(float* acc, int n) {      // thread de audio: soma as vozes (8 kHz -> taxa do mixer, interpolacao linear)
    if (gSndRate <= 0) return;
    std::unique_lock<std::mutex> lk(gVcMx, std::try_to_lock); if (!lk.owns_lock()) return;   // nunca trava o audio esperando a thread principal
    const double step = (double)VC_RATE / gSndRate;
    for (auto& kv : gVcSpk) {
        VcSpk& s = kv.second; if (!s.active) continue;
        for (int i = 0; i < n; i++) {
            if (s.q.size() < 2) { s.active = false; s.frac = 0; break; }   // acabou: espera encher de novo
            const float v = s.q[0] + (s.q[1] - s.q[0]) * (float)s.frac;
            acc[i * 2] += v * s.gl; acc[i * 2 + 1] += v * s.gr; s.frac += step;
            while (s.frac >= 1.0 && !s.q.empty()) { s.frac -= 1.0; s.q.pop_front(); }
        }
    }
}
