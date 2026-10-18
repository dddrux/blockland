// test_audio: decodificador WAV/IMA-ADPCM (audio.inc) contra PCM de referencia gerado pelo ffmpeg + mixer (callback) sem dispositivo
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include "stubs/SDL.h"
static void logWarn(const char*, ...) {} static void logInfo(const char*, ...) {}
#include "../src/vorbis.inc"
#include "../src/audio.inc"
int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("FALHOU: " __VA_ARGS__); printf("\n"); fails++; } } while (0)
int main(int argc, char** argv) {
    int total = 0;
    for (int i = 1; i + 1 < argc; i += 2) {   // pares: arquivo.wav arquivo.raw (s16le intercalado do ffmpeg)
        FILE* fp = fopen(argv[i], "rb"); if (!fp) { printf("FALHOU: abrir %s\n", argv[i]); fails++; continue; }
        std::vector<uint8_t> f; uint8_t t[8192]; size_t n; while ((n = fread(t, 1, sizeof t, fp)) > 0) f.insert(f.end(), t, t + n); fclose(fp);
        SndBuf b; const std::string an = argv[i]; if (an.size() > 4 && an.compare(an.size() - 4, 4, ".ogg") == 0) { int oc = 0, orate = 0; CHECK(vbDecodeOgg(f, b.pcm, oc, orate), "ogg %s", argv[i]); b.ch = oc; b.rate = orate; b.frames = oc ? b.pcm.size() / oc : 0; } else   // patchAud.OGG: .ogg bate com o ffmpeg
        CHECK(sndParseWav(f, b), "parse %s", argv[i]);
        FILE* rf = fopen(argv[i + 1], "rb"); std::vector<int16_t> ref; int16_t s; while (rf && fread(&s, 2, 1, rf) == 1) ref.push_back(s); if (rf) fclose(rf);
        const size_t m = std::min(ref.size(), b.pcm.size()); long maxd = 0; for (size_t k = 0; k < m; k++) maxd = std::max(maxd, (long)labs((long)ref[k] - b.pcm[k]));
        CHECK(m > 1000, "amostras de %s (%zu)", argv[i], m); CHECK(maxd <= 2, "%s: diferenca maxima %ld vs ffmpeg", argv[i], maxd);
        CHECK(b.pcm.size() >= ref.size() && b.pcm.size() - ref.size() < 4096 * b.ch, "tamanho %s: %zu vs %zu", argv[i], b.pcm.size(), ref.size());
        total++;
    }
    // mixer: uma voz mono a 44100 com pitch 1 deve sair identica (vl = vr = 1) e a voz termina no fim do buffer
    gSndOn = true; gSndRate = 44100; gSndMaster = 1.f;
    SndBuf b; b.ch = 1; b.rate = 44100; b.frames = 2000; b.pcm.assign(2000, 0); for (int i = 0; i < 2000; i++) b.pcm[i] = (int16_t)(i * 10 - 10000);
    gSndV[0].b = &b; gSndV[0].pos = 0; gSndV[0].step = 1; gSndV[0].vl = 1; gSndV[0].vr = .5f;
    int16_t out[512 * 2]; sndCallback(nullptr, (Uint8*)out, sizeof out);
    CHECK(out[0] == -10000 && out[1] == -5000, "mixer l/r: %d %d", out[0], out[1]); CHECK(out[2 * 100] == -9000, "mixer amostra 100: %d", out[200]);
    for (int k = 0; k < 4; k++) sndCallback(nullptr, (Uint8*)out, sizeof out); CHECK(gSndV[0].b == nullptr, "voz deveria terminar");
    // saturacao e meio tom: pitch 2 avanca 2 amostras por frame
    gSndV[1].b = &b; gSndV[1].pos = 0; gSndV[1].step = 2; gSndV[1].vl = gSndV[1].vr = 1; gSndV[2].b = &b; gSndV[2].pos = 0; gSndV[2].step = 1; gSndV[2].vl = gSndV[2].vr = 1;
    sndCallback(nullptr, (Uint8*)out, sizeof out); CHECK(out[2 * 10] == (int16_t)((10 * 2 * 10 - 10000) + (10 * 10 - 10000)), "pitch 2 + soma: %d", out[20]);
    {   // eventos (events.cfg): # expande, opcoes, arquivo ausente e novos .wav
        sndEvtLoad(); gSndEvts.clear(); sndEvtLine("block.break.grass = dig/grass# ; vol=.5 pitch=1.2 var=0 cat=4"); sndEvtLine("x.y = random/pop nao/existe ; cat=7"); sndEvtLine("# c = random/pop"); sndEvtLine("lixo");
        const SndEvt* g = sndEvtGet("block.break.grass"); CHECK(g && g->f.size() == 4 && g->vol == .5f && g->pitch == 1.2f && g->cat == 4, "evento grama: %zu", g ? g->f.size() : 0);
        const SndEvt* x = sndEvtGet("x.y"); CHECK(x && x->f.size() == 1 && x->cat == 7, "evento com arquivo ausente"); CHECK(!sndEvtGet("c") && !sndEvtGet("lixo"), "comentario/lixo");
        const char* nw[] = {"random/bow", "random/pop", "random/chestopen", "block/chest/open", "block/enderchest/open", "dig/grass1", "dig/grass4"};
        for (const char* n : nw) { SndBuf* b = sndGet(n); CHECK(b && b->ok, "wav novo invalido: %s", n); }
    }
    {   // patchAud.OGG: todo mob tem say/hurt/death reais; TNT e explosao ligados; .ogg preferido
        for (int t = 0; t < 14; t++) { const MobSnd* m = mobSndFor(t); CHECK(m != nullptr, "mob %d sem tabela", t); if (!m) continue;
            for (int k = 1; k <= m->nSay; k++) CHECK(sndFileOk(std::string("mob/") + m->dir + "/" + m->say + std::to_string(k)), "mob %d say%d", t, k);
            for (int k = 1; k <= m->nHurt; k++) CHECK(sndFileOk(std::string("mob/") + m->dir + "/" + m->hurt + std::to_string(k)), "mob %d hurt%d", t, k);
            if (m->nDeath == -1) CHECK(sndFileOk(std::string("mob/") + m->dir + "/" + m->death), "mob %d death", t);
            for (int k = 1; k <= m->nDeath; k++) CHECK(sndFileOk(std::string("mob/") + m->dir + "/" + m->death + std::to_string(k)), "mob %d death%d", t, k); }
        gSndEvts.clear(); gSndEvtOk = false; sndEvtLoad();
        const SndEvt* fz = sndEvtGet("tnt.fuse"); CHECK(fz && fz->f.size() == 1, "tnt.fuse");
        const SndEvt* ex = sndEvtGet("explode"); CHECK(ex && ex->f.size() == 4 && ex->vol == 4.f, "explode (%zu)", ex ? ex->f.size() : 0);
        SndBuf* ob = sndGet("random/explode1"); CHECK(ob && ob->ok && ob->frames > 1000, "explode1.ogg carrega"); SndBuf* st = sndGet("music/game/calm2"); CHECK(st && st->ok && st->ch == 2, "musica ogg estereo"); gSndCache.erase("music/game/calm2");
    }
    printf("test_audio: %d arquivos, %d falhas\n", total, fails); return fails ? 1 : 0;
}
