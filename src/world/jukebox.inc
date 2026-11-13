static bool wxSkyOpen(int x, int y, int z);   // weather.inc
// patchJuke.1: toca-discos do 1.12.2. Disco dentro = metadado do bloco (1..12; ja sincroniza e salva como qualquer bloco).
// Clique direito com disco na mao: insere e toca assets/sounds/records/<nome>.ogg; clique direito de novo: ejeta. Quebrar o bloco solta o disco.
static const char* const JUKE_DISCS[12] = {"13", "cat", "blocks", "chirp", "far", "mall", "mellohi", "stal", "strad", "ward", "11", "wait"};
static int gJukeId = 0;                                     // id do bloco (pack): block/jukebox ou block/jukebox_side
static struct { bool on = false; int x = 0, y = 0, z = 0, disc = 0; SndBuf* b = nullptr; std::string name; } gJuke;   // faixa tocando agora (so local)
static void jukeInit() { gJukeId = 0; for (const char* n : {"block/jukebox", "block/jukebox_side", "block/jukebox_top"}) { auto t = gPkByFile.find(n); if (t != gPkByFile.end()) { gJukeId = t->second; break; } } }
static int jukeDiscOf(int id) {   // item -> 1..12 (0 = nao e disco). Aceita music_disc_X e record_X
    if (!isPk(id)) return 0; const std::string& f = PK[id - PK0].file;
    for (const char* pre : {"item/music_disc_", "item/record_"}) { const size_t L = strlen(pre); if (f.compare(0, L, pre) == 0) { const std::string t = f.substr(L); for (int i = 0; i < 12; i++) if (t == JUKE_DISCS[i]) return i + 1; } }
    return 0;
}
static int jukeItemOf(int d) {   // 1..12 -> id do item (0 se o pack nao tem)
    if (d < 1 || d > 12) return 0;
    for (const char* pre : {"item/music_disc_", "item/record_"}) { auto t = gPkByFile.find(std::string(pre) + JUKE_DISCS[d - 1]); if (t != gPkByFile.end()) return t->second; }
    return 0;
}
static void jukeStop() {
    if (!gJuke.on) return;
    if (gSndOn && gJuke.b) { SDL_LockAudioDevice(gSndDev); for (SndVoice& v : gSndV) if (v.b == gJuke.b) v.b = nullptr; SDL_UnlockAudioDevice(gSndDev); gSndCache.erase(gJuke.name); }   // libera o PCM (faixa longa)
    gJuke = decltype(gJuke)(); gJukeBusy = false;
}
static void jukePlay(int x, int y, int z, int d) {
    jukeStop(); if (d < 1 || d > 12 || !gSndOn) return;
    const std::string n = std::string("records/") + JUKE_DISCS[d - 1]; SndBuf* b = sndGet(n); if (!b) return;   // sem .ogg: fica mudo (o disco entra/sai igual)
    gJuke.on = true; gJuke.x = x; gJuke.y = y; gJuke.z = z; gJuke.disc = d; gJuke.b = b; gJuke.name = n; gJukeBusy = true;
    sndPlayAt(n.c_str(), x + .5, y + .5, z + .5, 1.f, 1.f, 1);
}
static void jukeEject(int x, int y, int z) {
    const int d = getM(x, y, z) & 15; if (gJuke.on && gJuke.x == x && gJuke.y == y && gJuke.z == z) jukeStop();
    if (d >= 1 && d <= 12) { const int it = jukeItemOf(d); if (it) spawnDrop(x + .5, y + 1.0, z + .5, 0, .15, 0, it, 1, 10); }
}
static int jukeUse(int x, int y, int z, Slot& s, bool creative) {   // 0 = nao e o toca-discos; 1 = ejetou; 2 = inseriu (consome 1 disco)
    if (!gJukeId || getB(x, y, z) != gJukeId) return 0;
    if (getM(x, y, z) & 15) { jukeEject(x, y, z); setBM(x, y, z, gJukeId, getM(x, y, z) & ~15); netSendBlock(x, y, z, gJukeId, getM(x, y, z)); return 1; }
    const int d = s.id ? jukeDiscOf(s.id) : 0; if (!d) return 0;
    setBM(x, y, z, gJukeId, (getM(x, y, z) & ~15) | d); netSendBlock(x, y, z, gJukeId, getM(x, y, z)); jukePlay(x, y, z, d);
    if (!creative && --s.n <= 0) s = Slot(); return 2;
}
static void jukeNetEdit(int x, int y, int z, int id, int meta) {   // bloco mudou por outro jogador: disco entrou => toca (sem soltar item); saiu => para
    if (!gJukeId || id != gJukeId) { if (gJuke.on && gJuke.x == x && gJuke.y == y && gJuke.z == z) { gJuke.disc = 0; jukeStop(); } return; }
    const int d = meta & 15; if (d >= 1 && d <= 12) jukePlay(x, y, z, d); else if (gJuke.on && gJuke.x == x && gJuke.y == y && gJuke.z == z) jukeStop();
    if (gJuke.on && d >= 1) gJuke.disc = 0;   // disco do outro jogador: quem ejeta/quebra cuida do item (sem duplicar)
}
static void jukeTick() {   // quebrou o bloco / acabou a faixa => para (o disco de um bloco quebrado cai)
    if (!gJuke.on) return;
    if (!gJukeId || getB(gJuke.x, gJuke.y, gJuke.z) != gJukeId) { const int it = jukeItemOf(gJuke.disc); const int x = gJuke.x, y = gJuke.y, z = gJuke.z; jukeStop(); if (it) spawnDrop(x + .5, y + .5, z + .5, 0, .15, 0, it, 1, 10); return; }
    bool alive = false; for (const SndVoice& v : gSndV) if (v.b == gJuke.b) alive = true; if (!alive) jukeStop();
}

static void sndWorldAmbient(const Phys& P) {   // patchAud.3: 1x por tick: chiado do fogo/lava por perto (a cada 0,5 s) e respingo ao cair na agua
    static int tk = 0; static bool wasW = false; static int splashCd = 0, lastStep = 0; static bool swimInit = false;
    // patchAud.W1: agua como o Entity.doWaterSplashEffect do 1.8.9: volume proporcional a velocidade (sqrt(.2*vx^2 + vy^2 + .2*vz^2) * .2), nao fixo. Antes: .5 + 1,2*|my| x .6 = alto,
    // e segurar ESPACO na superficie entrava/saia da agua a cada tick (1 respingo cheio por reentrada). Agora: velocidade minima, volume baixo na reentrada e 1 s de intervalo.
    // Nado: 1 som por passo HORIZONTAL (distWalked, como o moveEntity do MC), entao nadar so para cima/segurar espaco nao faz barulho.
    if (!swimInit) { swimInit = true; if (!sndEvtGet("swim")) sndEvtLine("swim = liquid/swim# ; vol=0.5 pitch=1 var=0.4 cat=7"); }
    if (splashCd > 0) splashCd--;
    const float wsp = (float)sqrt(P.mx * P.mx * .2 + P.my * P.my + P.mz * P.mz * .2);
    if (P.inWater && !wasW && !P.noclip && wsp > .1f && splashCd == 0) { splashCd = 20; sndEvtAt("splash", P.x, P.y + .5, P.z, std::min(1.f, wsp * .5f)); }
    wasW = P.inWater;
    if (P.inWater && !P.noclip) { const int st = (int)P.distWalked; if (st != lastStep) { lastStep = st; if (wsp > .05f) sndEvtAt("swim", P.x, P.y + .5, P.z, std::min(1.f, .25f + wsp * 2.5f)); } } else lastStep = (int)P.distWalked;
    if (gDim == 3) { static int bloodT = 200; if (--bloodT <= 0) { bloodT = 160 + rand() % 360; sndEvtAt("ambient.cave", P.x + (rand() % 25 - 12), P.y + 1., P.z + (rand() % 25 - 12)); } }   // patchBlood.2: Rubra: ruidos de caverna a ceu aberto (ambientacao estranha); o trovao/chuva ja vem do clima
    { static int caveT = 600; if (--caveT <= 0) { caveT = 400 + rand() % 800;   // patchAud.5: ruido de caverna (assets/sounds/ambient/cave) abaixo da superficie, sem ceu aberto
          if (gDim == 0 && P.y < 56 && !wxSkyOpen((int)floor(P.x), (int)floor(P.y + 1.6), (int)floor(P.z))) sndEvtAt("ambient.cave", P.x + (rand() % 17 - 8), P.y + 1., P.z + (rand() % 17 - 8)); } }
    if (++tk % 10) return; const int px = (int)floor(P.x), py = (int)floor(P.y), pz = (int)floor(P.z); int fx = 0, fy = 0, fz = 0, lx = 0, ly = 0, lz = 0; bool f = false, l = false;
    for (int y = py - 3; y <= py + 3; y++) for (int z = pz - 5; z <= pz + 5; z++) for (int x = px - 5; x <= px + 5; x++) {
        const int b = getB(x, y, z); if (b == XB_FIRE && (!f || rand() % 4 == 0)) { f = true; fx = x; fy = y; fz = z; } else if (isLava(b) && getB(x, y + 1, z) == AIR && (!l || rand() % 8 == 0)) { l = true; lx = x; ly = y; lz = z; } }
    if (f && rand() % 2 == 0) sndEvtAt("fire.ambient", fx + .5, fy + .5, fz + .5);
    if (l) { if (rand() % 6 == 0) sndEvtAt("lava.pop", lx + .5, ly + 1., lz + .5); else if (rand() % 6 == 0) sndEvtAt("lava.ambient", lx + .5, ly + 1., lz + .5); }
}
