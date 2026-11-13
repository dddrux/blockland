#include <set>
// ================= mobs com IA básica: porco, galinha, vaca, ovelha =================
// Modelos = ModelPig / ModelChicken / ModelCow / ModelSheep2+1 do Java 1.8.9 (mesmas caixas, pivôs e UVs).
// Medidas em pixels do modelo Java (y para baixo, chão em y=24). Aqui: x_nosso = -x_java, y_nosso = 24 - y_java (caixas viram [-y1,-y0]);
// rotateAngleX = +90° do Java (corpos deitados) = -90° aqui. Texturas vêm do resource pack ativo (64x64 ou 64x32).
// IA (goals do 1.8.9, sem pathfinding A*): Swim, Panic (ao apanhar), Wander, WatchClosest, LookIdle.
//   porco/vaca/ovelha = IA "animal" (Wander 1/120 por tick, raio 10x7, prefere grama);
//   galinha = IA própria: passeios curtos e frequentes, bicadas no chão, plana ao cair (asas), põe ovo a cada 5-10 min, pânico 1.4x.
enum { M_PIG, M_CHICKEN, M_COW, M_SHEEP, M_VILLAGER, M_PIGMAN, M_ZOMBIE, M_SKELETON, M_CREEPER, M_BLAZE, M_SPIDER, M_HUSK, M_CSPIDER, M_MOOSH, M_ENDERMAN, M_SLIME, M_RABBIT, M_OCELOT, M_WOLF, M_STRAY, M_WSKEL, M_ZVILL, M_SILVER, M_ENDERMITE, M_MAGMA, M_SQUID, M_WITCH, M_VINDIC, M_EVOKER, M_ILLUS, M_CATBLK, M_CATRED, M_CATSIA, M_IGOLEM, M_PARROT, M_GHAST, M_SHULKER, M_SNOWGOLEM, M_RWHITE, M_RBLACK, M_RGOLD, M_RSALT, M_RSPLOT, M_RKILLER, M_RTOAST, M_VFARM, M_VLIB, M_VPRIEST, M_VSMITH, M_VBUTCH, M_GUARD, M_ELDER, M_BAT, M_POLAR, M_HORSE, M_HBLACK, M_HWHITE, M_HGRAY, M_DONKEY, M_MULE, M_ZHORSE, M_SHORSE, M_LLAMA, M_LLAMAB, M_LLAMAG, M_LLAMAW, M_VEX, M_WITHER, M_N };
static inline bool isHorse(int t) { return t >= M_HORSE && t <= M_SHORSE; }   // patchMobNew.2: cavalo (4 pelagens), burro, mula, cavalo zumbi e esqueleto (EntityHorse 1.12.2: mesmo modelo)
static inline bool isLlama(int t) { return t >= M_LLAMA && t <= M_LLAMAW; }   // patchMobNew.2: lhama (4 pelagens)
static inline bool isRabbit(int t) { return t == M_RABBIT || (t >= M_RWHITE && t <= M_RTOAST); }   // patchMob112.7: variantes de pelagem do coelho   // patchMob112.5: so modelo/ovo/passeio (ai 0), sem IA propria
static inline bool mobCanBaby(int t) { return t == M_PIG || t == M_CHICKEN || t == M_COW || t == M_SHEEP || t == M_MOOSH || (isRabbit(t) && t != M_RKILLER) || (t >= M_HORSE && t <= M_MULE) || isLlama(t) || t == M_WOLF || t == M_OCELOT || (t >= M_CATBLK && t <= M_CATSIA) || t == M_VILLAGER || (t >= M_VFARM && t <= M_VBUTCH); }   // patchBaby.3: especies com filhote/reproducao (coelho: todas as pelagens, menos o assassino)
static inline int mobKindGrp(int t) { return isRabbit(t) ? M_RABBIT : (t >= M_HORSE && t <= M_HGRAY) ? M_HORSE : isLlama(t) ? M_LLAMA : (t == M_OCELOT || (t >= M_CATBLK && t <= M_CATSIA)) ? M_OCELOT : (t == M_VILLAGER || (t >= M_VFARM && t <= M_VBUTCH)) ? M_VILLAGER : t; }   // P150: pelagens/profissoes/gato-ocelote contam como a mesma especie
static inline bool mobSameKind(int a, int b) { return a == b || (mobCanBaby(a) && mobCanBaby(b) && mobKindGrp(a) == mobKindGrp(b)); }   // coelhos de pelagens diferentes tambem cruzam (o filhote sai da pelagem de quem iniciou); P150: idem p/ cavalo/lhama/gato/aldeao
static inline bool isSlimy(int t) { return t == M_SLIME || t == M_MAGMA; }   // patchMob112.3: cubo de magma = slime com outra textura/modelo
// patchMob112.1: vagante (stray), esqueleto wither e zumbi-aldeao (1.12.2)
static inline bool isArch(int t) { return t == M_SKELETON || t == M_STRAY; }   // atiram flechas, queimam ao sol, fogem do sol
static inline bool isMelee(int t) { return t == M_ZOMBIE || t == M_HUSK || t == M_ZVILL || t == M_WSKEL || t == M_VEX || t == M_VINDIC; }   // corpo a corpo (3 de dano)   // 6..8 = hostis novos (hostile.inc)
// spd = atributo movementSpeed do 1.8.9; panic = multiplicador do EntityAIPanic; ai: 0 = animal, 1 = galinha, 2 = aldeão, 3 = porco-zumbi (neutro; irritado persegue e ataca), 4 = zumbi, 5 = esqueleto, 6 = creeper (>= 4: hostis de hostile.inc)
struct MobDef { const char* nome; const char* tex; const char* wool; float w, h; int hp; float spd, panic; int ai; };
static const MobDef MD[M_N] = {
    {"porco",   "entity/pig/pig_temperate.png|entity/pig/pig.png", "", .9f, .9f, 10, .25f, 1.25f, 0},
    {"galinha", "entity/chicken/chicken_temperate.png|entity/chicken/temperate_chicken.png|entity/chicken.png", "", .4f, .7f, 4, .25f, 1.4f, 1},
    {"vaca",    "entity/cow/cow_temperate.png|entity/cow/cow.png", "", .9f, 1.3f, 10, .2f, 2.0f, 0},
    {"ovelha",  "entity/sheep/sheep.png", "entity/sheep/sheep_wool.png|entity/sheep/sheep_fur.png", .9f, 1.3f, 8, .23f, 1.25f, 0},
    {"aldeao",  "entity/villager/villager.png|entity/villager/type/plains.png|entity/villager.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"porco-zumbi", "entity/zombie_pigman.png|entity/piglin/zombified_piglin.png", "", .6f, 1.95f, 20, .23f, 1.f, 3},
    {"zumbi",   "entity/zombie/zombie.png", "", .6f, 1.95f, 20, .23f, 1.f, 4},
    {"esqueleto", "entity/skeleton/skeleton.png", "", .6f, 1.99f, 20, .25f, 1.f, 5},
    {"creeper", "entity/creeper/creeper.png", "", .6f, 1.7f, 20, .25f, 1.f, 6},
    {"blaze", "entity/blaze.png|entity/blaze/blaze.png", "", .6f, 1.8f, 20, .23f, 1.f, 7},
    {"aranha", "entity/spider/spider.png", "", 1.4f, .9f, 16, .3f, 1.f, 8},
    {"husk", "entity/zombie/husk.png", "", .6f, 1.95f, 20, .23f, 1.f, 4},
    {"aranha-da-caverna", "entity/spider/cave_spider.png", "", .7f, .5f, 12, .3f, 1.f, 8},
    {"vaca-cogumelo", "entity/cow/mooshroom.png", "", .9f, 1.3f, 10, .2f, 2.0f, 0},
    {"enderman", "entity/enderman/enderman.png", "entity/enderman/enderman_eyes.png", .6f, 2.9f, 40, .3f, 1.f, 9},
    {"slime", "entity/slime/slime.png", "entity/slime/slime.png", .51f, .51f, 16, .2f, 1.f, 10},
    {"coelho", "entity/rabbit/brown.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"jaguatirica", "entity/cat/ocelot.png", "", .6f, .7f, 10, .3f, 2.0f, 0},
    {"lobo", "entity/wolf/wolf.png", "", .6f, .85f, 8, .3f, 1.f, 3},
    {"vagante", "entity/skeleton/stray.png", "entity/skeleton/stray_overlay.png", .6f, 1.99f, 20, .25f, 1.f, 5},
    {"esqueleto-wither", "entity/skeleton/wither_skeleton.png", "", .7f, 2.4f, 20, .25f, 1.f, 4},
    {"zumbi-aldeao", "entity/zombie/zombie_villager.png|entity/zombie_villager/zombie_villager.png", "", .6f, 1.95f, 20, .23f, 1.f, 4},
    {"traca", "entity/silverfish.png", "", .4f, .3f, 8, .25f, 1.f, 8},
    {"endermite", "entity/endermite.png", "", .4f, .3f, 8, .25f, 1.f, 8},
    {"cubo-de-magma", "entity/slime/magmacube.png", "", .51f, .51f, 16, .2f, 1.f, 10},
    {"lula", "entity/squid.png", "", .95f, .95f, 10, .2f, 1.5f, 0},
    {"bruxa", "entity/witch.png", "", .6f, 1.95f, 26, .25f, 1.f, 4},
    {"vindicador", "entity/illager/vindicator.png", "", .6f, 1.95f, 24, .25f, 1.f, 4},
    {"evocador", "entity/illager/evoker.png", "", .6f, 1.95f, 24, .25f, 1.f, 4},
    {"ilusionista", "entity/illager/illusionist.png", "", .6f, 1.95f, 32, .25f, 1.f, 4},
    {"gato-preto", "entity/cat/black.png", "", .6f, .7f, 10, .3f, 2.0f, 0},
    {"gato-vermelho", "entity/cat/red.png", "", .6f, .7f, 10, .3f, 2.0f, 0},
    {"gato-siames", "entity/cat/siamese.png", "", .6f, .7f, 10, .3f, 2.0f, 0},
    {"golem-de-ferro", "entity/iron_golem.png", "", 1.4f, 2.7f, 100, .25f, 1.f, 0},
    {"papagaio", "entity/parrot/parrot_red_blue.png", "", .5f, .9f, 6, .2f, 1.5f, 0},
    {"ghast", "entity/ghast/ghast.png", "", 4.f, 4.f, 10, .15f, 1.f, 0},
    {"shulker", "entity/shulker/shulker_purple.png", "", 1.f, 1.f, 30, 0.f, 1.f, 0},
    {"golem-de-neve", "entity/snowman.png", "", .7f, 1.9f, 4, .2f, 1.f, 0},
    {"coelho-branco", "entity/rabbit/white.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-preto", "entity/rabbit/black.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-dourado", "entity/rabbit/gold.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-sal-e-pimenta", "entity/rabbit/salt.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-malhado", "entity/rabbit/white_splotched.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-assassino", "entity/rabbit/caerbannog.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"coelho-toast", "entity/rabbit/toast.png", "", .4f, .5f, 3, .3f, 2.2f, 0},
    {"fazendeiro", "entity/villager/farmer.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"bibliotecario", "entity/villager/librarian.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"clerigo", "entity/villager/priest.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"ferreiro", "entity/villager/smith.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"acougueiro", "entity/villager/butcher.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"guardiao", "entity/guardian.png", "", .85f, .85f, 30, .2f, 1.5f, 0},
    {"guardiao-ancestral", "entity/guardian_elder.png", "", 1.99f, 1.99f, 80, .2f, 1.5f, 0},
    {"morcego", "entity/bat.png", "", .5f, .9f, 6, .2f, 1.5f, 0},   // patchMobNew.1: EntityBat 1.12.2 (6 de vida, .5x.9; voa)
    {"urso-polar", "entity/bear/polarbear.png", "", 1.3f, 1.4f, 30, .3f, 1.f, 3},
    {"cavalo", "entity/horse/horse_brown.png", "", 1.4f, 1.6f, 22, .22f, 1.2f, 0}, {"cavalo-preto", "entity/horse/horse_black.png", "", 1.4f, 1.6f, 22, .22f, 1.2f, 0},   // patchMobNew.2: EntityHorse 1.12.2 (15-30 de vida, 1.4x1.6)
    {"cavalo-branco", "entity/horse/horse_white.png", "", 1.4f, 1.6f, 22, .22f, 1.2f, 0}, {"cavalo-cinza", "entity/horse/horse_gray.png", "", 1.4f, 1.6f, 22, .22f, 1.2f, 0},
    {"burro", "entity/horse/donkey.png", "", 1.4f, 1.6f, 22, .175f, 1.2f, 0}, {"mula", "entity/horse/mule.png", "", 1.4f, 1.6f, 22, .175f, 1.2f, 0},
    {"cavalo-zumbi", "entity/horse/horse_zombie.png", "", 1.4f, 1.6f, 15, .22f, 1.2f, 0}, {"cavalo-esqueleto", "entity/horse/horse_skeleton.png", "", 1.4f, 1.6f, 15, .22f, 1.2f, 0},
    {"lhama", "entity/llama/llama_creamy.png|entity/llama/llama.png", "", .9f, 1.87f, 22, .175f, 1.2f, 0}, {"lhama-marrom", "entity/llama/llama_brown.png", "", .9f, 1.87f, 22, .175f, 1.2f, 0},   // patchMobNew.2: EntityLlama (15-30 de vida, .9x1.87; textura 128x64)
    {"lhama-cinza", "entity/llama/llama_gray.png", "", .9f, 1.87f, 22, .175f, 1.2f, 0}, {"lhama-branca", "entity/llama/llama_white.png", "", .9f, 1.87f, 22, .175f, 1.2f, 0},
    {"vex", "entity/illager/vex.png", "", .4f, .8f, 14, .3f, 1.f, 4},
    {"wither", "entity/wither/wither.png", "", .9f, 3.5f, 300, .35f, 1.f, 4}};   // patchMobNew.6: EntityWither 1.12.2 (300 de vida, .9x3.5, modelo x2; voa)   // patchMobNew.3: EntityVex 1.12.2 (14 de vida, .4x.8; voa, corpo a corpo)   // patchMobNew.1: EntityPolarBear 1.12.2 (30 de vida, 1.3x1.4; neutro: ataca se apanhar, dano 6)   // patchWolf.1: EntityWolf 1.12.2 selvagem (8 de vida, .6x.85; neutro: so ataca irritado, em matilha; dano 2)
       // patchOcelot.1: EntityOcelot 1.12.2 selvagem (10 de vida, .6x.7; foge do jogador a menos de 7 blocos)
       // patchRabbit.1: EntityRabbit 1.12.2 (3 de vida, caixa .4x.5; anda aos pulos)
       // patchSlime.2: EntitySlime 1.12.2 (w/h x tamanho; vida = tamanho^2; 2a textura = casca de gel translucida)   // patchEnd.12: EntityEnderman 1.12.2 (40 de vida, 0,3; olhos = 2a textura)   // patchMob.1: mooshroom = modelo e IA da vaca, textura propria   // EntityVillager 1.8.9: 20 de vida, velocidade 0.5 (passeio a 0.6x)
// kind: 0 fixo, 1 cabeça, 2/3 perna (fase 0 / fase π), 4/5 asa direita/esquerda (galinha). red = cópia vermelha p/ o flash de dano/morte
struct MPart { std::vector<V> m, red; float px, py, pz, rx; bool wool; int kind; };
static std::vector<MPart> MP[M_N];
static GLuint mobTex[M_N][2]; static bool mobReady = false;
struct MobEnt {
    int potPoison = 0, potRegen = 0, potSlow = 0, potSlowAmp = 0, potSpeed = 0, potSpeedAmp = 0; int potPoisonAmp = 0;   /* patchPot.2: efeitos de pocao no mob */
    bool saddled = false;   /* patchHorse.1: cavalo/burro/mula com sela (montavel) */
    bool tame = false; int temper = 0, rideT = 0, rideLim = 0;   /* patchHorse.2: domado; temperamento (+5 por tentativa), ticks montado e limite ate o teste de domar */
    bool baby = false; int babyAge = 0;   /* patchBaby.1: filhote (metade do tamanho, cresce em 24000 ticks, sem drops) */
    int love = 0, breedCd = 0;   /* patchBreed.1: modo amor (ticks restantes, alimentado) e recarga apos reproduzir (6000 ticks = 5 min) */
    bool sheared = false;   /* patchShear.1: ovelha tosquiada (sem la, nao desenha a camada de la, nao solta la ao morrer) */
    bool sunFlee = false;   /* patchMob.6: esqueleto fugindo do sol rumo a sombra */
    int type = 0; double x = 0, y = 0, z = 0; float yaw = 0; int hp = 1, dead = 0;   // dead: 0 vivo; >0 = ticks desde a morte (some aos 21, como o deathTime 20 do Java)
    double px = 0, py = 0, pz = 0, mx = 0, my = 0, mz = 0; float pyaw = 0;           // p* = tick anterior (interpolação); m* = motionX/Y/Z
    float hyaw = 0, hpit = 0, phyaw = 0, phpit = 0;                                  // cabeça relativa ao corpo, em graus (hpit > 0 = olhando para baixo)
    float limb = 0, limbAmt = 0, plimbAmt = 0, fall = 0;
    bool ground = false, collH = false, inWater = false, inLava = false;
    int fuse = 0, atk = 0, burn = 0; bool chase = false;   // hostis: pavio do creeper, recarga de ataque/flecha, queima ao sol, perseguindo
    int talkT = 0; bool hasHome = false; double homeX = 0, homeZ = 0;   // aldeao: conversa (para e olha o jogador) e centro da aldeia
    int rideBoat = 0;   // id do bote em que o animal está sentado (0 = nenhum)
    int wetT = 0; int uid = 0, revId = 0, revT = 0, fire = 0; float aim = 0, paim = 0;   // uid (lazy, ver mobUid); revide: uid do atacante + ticks; fogo: ticks restantes; aim: mira do arco do esqueleto (0..1)
    int endAgg = 0, endStare = 0, endTpCd = 0, endHp = 0, endBlk = 0;   // patchEnd.12 enderman: raiva (ticks), tempo encarado, recarga do teleporte, vida no tick anterior
    int life = 0;   /* patchMobNew.4: vida util em ticks (vex invocado morre ao fim); 0 = eterno */
    int air = 300;   // patch5.78: ar do mob (afogamento, igual ao jogador)
    int ssz = 1, sjump = 20; float sqA = 0, sqF = 0, psqF = 0; bool wasG = false;   // patchSlime.2: tamanho (1,2,4), ticks ate o proximo pulo, esmagamento (squishAmount/Factor) e se estava no chao
    int sayT = -80, hurt = 0, resist = 0, lastDmg = 0, panic = 0, jumpCd = 0, stuck = 0, lookT = 0, idleT = 0, peckT = 0, eggT = 0;
    bool hasT = false; double tx = 0, tz = 0; float idleYaw = 0;                      // alvo do passeio
    float wing = 0, pwing = 0, flap = 0, pflap = 0, flapD = 1.f;                      // galinha: wingRotation, destPos, wingRotDelta
};
static std::vector<MobEnt> MOBS;
static inline float mobW(const MobEnt& e) { return MD[e.type].w * (e.baby ? .5f : 1.f) * (isSlimy(e.type) ? (float)e.ssz : 1.f); }   // patchBaby.1: caixa de colisao do filhote
static inline float mobH(const MobEnt& e) { return MD[e.type].h * (e.baby ? .5f : 1.f) * (isSlimy(e.type) ? (float)e.ssz : 1.f); }
static float gPlayerYaw = 0.f, gPlayerPitch = 0.f;
static bool gHitbox = false;   // /hitbox (equivalente ao F3+B): desenha a caixa de colisão de cada mob
struct Puff { float x, y, z, px, py, pz, vx, vy, vz, s, c; int age, max; uint8_t kind = 0; };   // kind 0 = fumaca; 1 = gota de agua (patchProp.10: cai, azul)           // fumaça da morte (EntityExplodeFX)
static std::vector<Puff> PUFFS;
struct HugeBoom { float x, y, z; int t; };   // patchTnt.2: EntityHugeExplodeFX (so 6 "explosao grande" por tick durante 8 ticks)
static std::vector<HugeBoom> HBOOMS;

static std::vector<V> bx(float x0, float x1, float y0, float y1, float z0, float z1, float u, float v, float w, float h, float d, float g = 0) {
    std::vector<V> o; boxM(o, x0, x1, y0, y1, z0, z1, u, v, w, h, d, g); return o;
}
static void shadeBody(std::vector<V>& m) {   // boxM sombreia pelas faces locais; o corpo é girado: [frente=barriga][costas=dorso][laterais][topo/base=peito/traseira]
    for (size_t i = 0; i < m.size(); i++) { const size_t k = i % 24; const float s = k < 4 ? .55f : k < 8 ? 1.f : k < 16 ? .7f : .85f; m[i].r = m[i].g = m[i].b = s; }
}
static float gMobUs = 1.f;   // patchMob112.6: escala de U da ultima textura (64 / largura); v usa 64 / altura
static bool mobTexLoad(const char* names, GLuint& tex, float& vs, const int fb[3]) {
    std::vector<uint8_t> px; int w = 0, h = 0; bool ok = false;
    std::string all = names; size_t a = 0;
    while (!ok && a <= all.size()) { size_t b = all.find('|', a); if (b == std::string::npos) b = all.size(); ok = packRGBA(all.substr(a, b - a), px, w, h); a = b + 1; }
    if (ok && ((w < 64 && !strstr(names, "parrot")) || (h * 2 != w && h != w && w * 2 != h))) ok = false;   // 2:1, 1:1 ou 1:2 (bruxa 64x128)
    if (!ok) { w = h = 64; px.assign(64 * 64 * 4, 0); for (int i = 0; i < 64 * 64; i++) { px[i * 4] = fb[0]; px[i * 4 + 1] = fb[1]; px[i * 4 + 2] = fb[2]; px[i * 4 + 3] = 255; } }
    if (BL_NOVK && ok && w == 64 && h >= 16 && strstr(names, "slime/slime.png")) {   // patchSlime.9: casca de gel 100% opaca (alpha 255 em x<32,y<16) e olhos/boca copiados do miolo para a face da frente (8,8)
        auto P = [&](int x, int y) { return &px[((size_t)y * w + x) * 4]; };
        auto cp = [&](int sx, int sy, int dx, int dy, int ww, int hh) { for (int j = 0; j < hh; j++) for (int i = 0; i < ww; i++) { const uint8_t* s0 = P(sx + i, sy + j); if (s0[3] > 0) { uint8_t* d0 = P(dx + i, dy + j); memcpy(d0, s0, 3); d0[3] = 255; } } };
        for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) { uint8_t* q = P(x, y); if (q[3] > 0) q[3] = 255; }
        cp(34, 2, 9, 10, 2, 2); cp(34, 6, 13, 10, 2, 2); cp(33, 9, 12, 13, 1, 1); }
    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    vs = 64.f / (float)h; gMobUs = 64.f / (float)w; if (ok && w == 128 && h == 64 && (strstr(names, "ghast/ghast") || strstr(names, "slime/"))) { vs = 2.f; gMobUs = 1.f; }   /* patchTexFix.1: pack 2x (128x64) de ghast/slime/cubo de magma: UVs do modelo sao do vanilla 64x32 */   // boxM normaliza por 64 nos dois eixos; 64x32 precisa de v * 2; 128x128 de u e v * .5
    return ok;
}
static void loadMobs() {   // precisa de contexto GL e do pack já aberto; chamado ao desenhar o 1º mob
    mobReady = true;
    static const int FB[M_N][2][3] = {{{240,160,160},{0,0,0}}, {{245,245,245},{0,0,0}}, {{90,60,40},{0,0,0}}, {{190,190,185},{240,240,240}}, {{130,95,70},{0,0,0}}, {{200,130,130},{0,0,0}}, {{70,130,70},{0,0,0}}, {{200,200,195},{0,0,0}}, {{60,160,50},{0,0,0}}, {{255,255,255},{0,0,0}}, {{60,50,45},{0,0,0}}, {{140,120,80},{0,0,0}}, {{20,60,70},{0,0,0}}, {{80,170,60},{120,200,100}}};
    float vs[M_N][2] = {}, us[M_N][2] = {}; int fromPack = 0;
    for (int t = 0; t < M_N; t++) {
        fromPack += mobTexLoad(MD[t].tex, mobTex[t][0], vs[t][0], FB[t][0]); us[t][0] = gMobUs; us[t][1] = 1.f;
        if (*MD[t].wool) { fromPack += mobTexLoad(MD[t].wool, mobTex[t][1], vs[t][1], FB[t][1]); us[t][1] = gMobUs; }
    }
    auto A = [&](int t, std::vector<V> m, float px, float py, float pz, float rx = 0, bool wool = false, int kind = 0) { MP[t].push_back({std::move(m), {}, px, py, pz, rx, wool, kind}); };
    auto body = [&](std::vector<V> m) { shadeBody(m); return m; };
    auto legK = [](float x, float z) { return x * z > 0 ? 2 : 3; };   // pernas em diagonal andam juntas (ModelQuadruped: leg1/leg4 fase 0, leg2/leg3 fase π)
    // porco: cabeça (0,12,-6) + focinho, corpo (0,13,2), pernas y=6
    { std::vector<V> h = bx(-4,4, -4,4, -8,0, 0,0, 8,8,8), s = bx(-2,2, -3,0, -9,-8, 16,16, 4,3,1); h.insert(h.end(), s.begin(), s.end());
      A(M_PIG, body(bx(-5,5, -6,10, -7,1, 28,8, 10,16,8)), 0,13,2, -90); A(M_PIG, h, 0,12,-6, 0, false, 1);
      for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_PIG, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4), x,6,z, 0, false, legK(x, z)); }
    // galinha: cabeça (0,9,-4) + bico + barbela, corpo (0,8,0), pernas (±..,5,1), asas y=11
    { std::vector<V> h = bx(-2,2, 0,6, -2,1, 0,0, 4,6,3), b = bx(-2,2, 2,4, -4,-2, 14,0, 4,2,2), c = bx(-1,1, 0,2, -3,-1, 14,4, 2,2,2);
      h.insert(h.end(), b.begin(), b.end()); h.insert(h.end(), c.begin(), c.end());
      A(M_CHICKEN, body(bx(-3,3, -4,4, -3,3, 0,9, 6,8,6)), 0,8,0, -90); A(M_CHICKEN, h, 0,9,-4, 0, false, 1);
      A(M_CHICKEN, bx(-2,1, -5,0, -3,0, 26,0, 3,5,3), 2,5,1, 0, false, 2); A(M_CHICKEN, bx(-2,1, -5,0, -3,0, 26,0, 3,5,3), -1,5,1, 0, false, 3);
      A(M_CHICKEN, bx(-1,0, -4,0, -3,3, 24,13, 1,4,6), 4,11,0, 0, false, 4); A(M_CHICKEN, bx(0,1, -4,0, -3,3, 24,13, 1,4,6), -4,11,0, 0, false, 5); }
    // vaca: cabeça (0,20,-8) + chifres, corpo (0,19,2) + úbere, pernas y=12 em (±4, 7/-6)
    { std::vector<V> h = bx(-4,4, -4,4, -6,0, 0,0, 8,8,6), h1 = bx(4,5, 2,5, -4,-3, 22,0, 1,3,1), h2 = bx(-5,-4, 2,5, -4,-3, 22,0, 1,3,1);
      h.insert(h.end(), h1.begin(), h1.end()); h.insert(h.end(), h2.begin(), h2.end());
      std::vector<V> b = bx(-6,6, -8,10, -7,3, 18,4, 12,18,10), u = bx(-2,2, -8,-2, -8,-7, 52,0, 4,6,1); b.insert(b.end(), u.begin(), u.end());
      A(M_COW, body(b), 0,19,2, -90); A(M_COW, h, 0,20,-8, 0, false, 1);
      for (float x : {-4.f, 4.f}) for (float z : {7.f, -6.f}) A(M_COW, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), x,12,z, 0, false, legK(x, z)); }
    for (const MPart& mp : MP[M_COW]) MP[M_MOOSH].push_back(mp);   // patchMob.1: vaca-cogumelo usa as mesmas partes (UV identico no mooshroom.png)
    // ovelha: pele (sheep.png) + lã (sheep_wool.png): cabeça (0,18,-8), corpo (0,19,2), pernas y=12 em (±3, 7/-5)
    A(M_SHEEP, bx(-3,3, -2,4, -6,2, 0,0, 6,6,8), 0,18,-8, 0, false, 1); A(M_SHEEP, body(bx(-4,4, -6,10, -7,-1, 28,8, 8,16,6)), 0,19,2, -90);
    for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_SHEEP, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), x,12,z, 0, false, legK(x, z));
    A(M_SHEEP, bx(-3,3, -2,4, -4,2, 0,0, 6,6,6, .6f), 0,18,-8, 0, true, 1); A(M_SHEEP, body(bx(-4,4, -6,10, -7,-1, 28,8, 8,16,6, 1.75f)), 0,19,2, -90, true);
    for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_SHEEP, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4, .5f), x,12,z, 0, true, legK(x, z));
    // aldeao (ModelVillager 1.8.9, textura 64x64): cabeca (0,0,0) + nariz, corpo + tunica (g .5), bracos cruzados (pivo 0,3,-1, rotX -0.75), pernas (+-2,12,0)
    {   auto mirU = [](std::vector<V> m) { for (int q = 0; q < (int)m.size() / 4; q++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, m[q * 4 + k].u); hi = std::max(hi, m[q * 4 + k].u); }
            for (int k = 0; k < 4; k++) m[q * 4 + k].u = lo + hi - m[q * 4 + k].u; }
            if (m.size() >= 16) for (int k = 0; k < 4; k++) { std::swap(m[8 + k].u, m[12 + k].u); std::swap(m[8 + k].v, m[12 + k].v); } return m; };   // ModelRenderer.mirror
        std::vector<V> h = bx(-4,4, 0,10, -4,4, 0,0, 8,10,8), n = bx(-1,1, -1,3, -6,-4, 24,0, 2,4,2); h.insert(h.end(), n.begin(), n.end());
        A(M_VILLAGER, h, 0,24,0, 0, false, 1);
        { std::vector<V> b = bx(-4,4, -12,0, -3,3, 16,20, 8,12,6), j = bx(-4,4, -18,0, -3,3, 0,38, 8,18,6, .5f); b.insert(b.end(), j.begin(), j.end()); A(M_VILLAGER, b, 0,24,0); }
        { std::vector<V> a1 = bx(4,8, -6,2, -2,2, 44,22, 4,8,4), a2 = mirU(bx(-8,-4, -6,2, -2,2, 44,22, 4,8,4)), a3 = bx(-4,4, -6,-2, -2,2, 40,38, 8,4,4);
          a1.insert(a1.end(), a2.begin(), a2.end()); a1.insert(a1.end(), a3.begin(), a3.end()); A(M_VILLAGER, a1, 0,21,-1, 42.97f); }
        A(M_VILLAGER, bx(-2,2, -12,0, -2,2, 0,22, 4,12,4), 2,12,0, 0, false, 6); A(M_VILLAGER, mirU(bx(-2,2, -12,0, -2,2, 0,22, 4,12,4)), -2,12,0, 0, false, 7); }
    // --- hostis (ModelBiped/ModelCreeper 1.8.9): cabeca (0,24,0), corpo, bracos (0,22,0) rot X, pernas (+-2,12,0); zumbis/piglin: bracos esticados a frente (rot 90) ---
    {   auto mirB = [](std::vector<V> m) { for (int q = 0; q < (int)m.size() / 4; q++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, m[q * 4 + k].u); hi = std::max(hi, m[q * 4 + k].u); }
            for (int k = 0; k < 4; k++) m[q * 4 + k].u = lo + hi - m[q * 4 + k].u; }
            if (m.size() >= 16) for (int k = 0; k < 4; k++) { std::swap(m[8 + k].u, m[12 + k].u); std::swap(m[8 + k].v, m[12 + k].v); } return m; };
        auto biped64 = [&](int t, std::vector<V> head) {   // textura 64x64 (bracos/pernas esquerdos tem UV proprio)
            A(t, head, 0,24,0, 0, false, 1); A(t, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,24,0);
            A(t, bx(4,8, -10,2, -2,2, 40,16, 4,12,4), 0,22,0, 90); A(t, mirB(bx(-8,-4, -10,2, -2,2, 40,16, 4,12,4)), 0,22,0, 90);   /* patchV.5: zumbi 64x32 do 1.12 nao tem UV de braco/perna esquerdos: espelha os direitos */
            A(t, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), 2,12,0, 0, false, 2); A(t, mirB(bx(-2,2, -12,0, -2,2, 0,16, 4,12,4)), -2,12,0, 0, false, 3); };
        biped64(M_ZOMBIE, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8));
        biped64(M_PIGMAN, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8));   // porco-zumbi: mesmo modelo do zumbi (textura 64x64 zombie_pigman do 1.12)
        A(M_SKELETON, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,24,0, 0, false, 1); A(M_SKELETON, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,24,0);
        A(M_SKELETON, bx(4,6, -10,2, -1,1, 40,16, 2,12,2), 0,22,0, 85, false, 8); A(M_SKELETON, mirB(bx(-6,-4, -10,2, -1,1, 40,16, 2,12,2)), 0,22,0, 90, false, 9);   // bracos mirando (arco)
        A(M_SKELETON, bx(-1,1, -12,0, -1,1, 0,16, 2,12,2), 2,12,0, 0, false, 2); A(M_SKELETON, mirB(bx(-1,1, -12,0, -1,1, 0,16, 2,12,2)), -2,12,0, 0, false, 3);
        A(M_CREEPER, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,18,0, 0, false, 1); A(M_CREEPER, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,18,0);
        for (float x : {-2.f, 2.f}) for (float z : {4.f, -4.f}) A(M_CREEPER, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4), x,6,z, 0, false, legK(x, z));
        {   // patchEnd.12: ModelEnderman (64x32): cabeca 8^3 + capuz (g -0.5) em y 38, corpo 8x12x4, bracos/pernas de 2x30x2; olhos = camada 'wool'
            std::vector<V> h = bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), hat = bx(-4,4, 0,8, -4,4, 0,16, 8,8,8, -.5f); h.insert(h.end(), hat.begin(), hat.end());
            A(M_ENDERMAN, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,38,0, 0, false, 1); A(M_ENDERMAN, hat, 0,38,0, 0, false, 14); A(M_ENDERMAN, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8, .05f), 0,38,0, 0, true, 1);
            A(M_ENDERMAN, bx(-4,4, -12,0, -2,2, 32,16, 8,12,4), 0,38,0);
            A(M_ENDERMAN, bx(4,6, -28,2, -1,1, 56,0, 2,30,2), 0,36,0, 0, false, 11); A(M_ENDERMAN, mirB(bx(-6,-4, -28,2, -1,1, 56,0, 2,30,2)), 0,36,0, 0, false, 10);
            A(M_ENDERMAN, bx(-1,1, -30,0, -1,1, 56,0, 2,30,2), 2,30,0, 0, false, 12); A(M_ENDERMAN, mirB(bx(-1,1, -30,0, -1,1, 56,0, 2,30,2)), -2,30,0, 0, false, 13); }
        A(M_BLAZE, bx(-4,4, -4,4, -4,4, 0,0, 8,8,8), 0,24,0, 0, false, 1);   // ModelBlaze: cabeca + 12 varetas (kind 20..31: orbitam, ver drawMob)
        for (int i = 0; i < 12; i++) A(M_BLAZE, bx(0,2, -8,0, 0,2, 0,16, 2,8,2), 0,24,0, 0, false, 20 + i);
        // patchMob.3 aranha (ModelSpider 1.8.9): cabeca, pescoco, corpo e 8 pernas horizontais (sem o angulo Y/Z do original) (VIS)
        A(M_SPIDER, bx(-4,4, -4,4, -8,0, 32,4, 8,8,8), 0,9,-3, 0, false, 1); A(M_SPIDER, bx(-3,3, -3,3, -3,3, 0,0, 6,6,6), 0,9,0); A(M_SPIDER, bx(-5,5, -4,4, -6,6, 0,12, 10,8,12), 0,9,9);
        for (float z : {2.f, 1.f, -1.f, -2.f}) { A(M_SPIDER, bx(-15,1, -1,1, -1,1, 18,0, 16,2,2), -4,9,z); A(M_SPIDER, mirB(bx(-1,15, -1,1, -1,1, 18,0, 16,2,2)), 4,9,z); } }
    { std::vector<V> in = bx(-3,3, 1,7, -3,3, 0,16, 6,6,6), r = bx(-3.25f,-1.25f, 4,6, -3.5f,-1.5f, 32,0, 2,2,2), l = bx(1.25f,3.25f, 4,6, -3.5f,-1.5f, 32,4, 2,2,2), m = bx(0,1, 2,3, -3.5f,-2.5f, 32,8, 1,1,1);   // ModelSlime(16): corpo interno + olhos + boca (patchSlime.2)
      in.insert(in.end(), r.begin(), r.end()); in.insert(in.end(), l.begin(), l.end()); in.insert(in.end(), m.begin(), m.end());
      A(M_SLIME, in, 0,0,0); A(M_SLIME, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,0,0, 0, true); }   // ModelSlime(0): casca externa (gel)
    // patchRabbit.1: ModelRabbit 1.12.2 (textura 64x32; y do Java negado, py = 24 - rotationPointY, rx = -rotateAngleX em graus). Sem orelhas inclinadas (so X no MPart)
    { std::vector<V> h = bx(-2.5f,2.5f, 0,4, -5,0, 32,0, 5,4,5), no = bx(-.5f,.5f, 1.5f,2.5f, -5.5f,-4.5f, 32,9, 1,1,1), er = bx(-2.5f,-.5f, 4,9, -1,0, 52,0, 2,5,1), el = bx(.5f,2.5f, 4,9, -1,0, 58,0, 2,5,1);
      h.insert(h.end(), no.begin(), no.end()); h.insert(h.end(), er.begin(), er.end()); h.insert(h.end(), el.begin(), el.end());
      A(M_RABBIT, bx(-3,3, -3,2, -10,0, 0,0, 6,5,10), 0,5,8, 20); A(M_RABBIT, h, 0,8,-1, 0, false, 1);
      A(M_RABBIT, bx(-1.5f,1.5f, -1.5f,1.5f, 0,2, 52,6, 3,3,2), 0,4,7, 20);
      A(M_RABBIT, bx(-1,1, -4,0, 0,5, 30,15, 2,4,5), 3,6.5f,3.7f, 20); A(M_RABBIT, bx(-1,1, -4,0, 0,5, 16,15, 2,4,5), -3,6.5f,3.7f, 20);
      A(M_RABBIT, bx(-1,1, -6.5f,-5.5f, -3.7f,3.3f, 26,24, 2,1,7), 3,6.5f,3.7f); A(M_RABBIT, bx(-1,1, -6.5f,-5.5f, -3.7f,3.3f, 8,24, 2,1,7), -3,6.5f,3.7f);
      A(M_RABBIT, bx(-1,1, -7,0, -1,1, 8,15, 2,7,2), 3,7,-1, 10, false, 6); A(M_RABBIT, bx(-1,1, -7,0, -1,1, 0,15, 2,7,2), -3,7,-1, 10, false, 7); }
    // patchOcelot.1: ModelOcelot 1.12.2 em pe (textura 64x32): corpo deitado, cauda 2 segmentos, 4 pernas, cabeca com focinho e orelhas
    { std::vector<V> h = bx(-2.5f,2.5f, -2,2, -3,2, 0,0, 5,4,5), no = bx(-1.5f,1.5f, -2,0, -4,-2, 0,24, 3,2,2), e1 = bx(-2,-1, 2,3, 0,2, 0,10, 1,1,2), e2 = bx(1,2, 2,3, 0,2, 6,10, 1,1,2);
      h.insert(h.end(), no.begin(), no.end()); h.insert(h.end(), e1.begin(), e1.end()); h.insert(h.end(), e2.begin(), e2.end());
      auto ob = bx(-2,2, -19,-3, -8,-2, 20,0, 4,16,6); shadeBody(ob);
      A(M_OCELOT, ob, 0,12,-10, -90); A(M_OCELOT, h, 0,9,-9, 0, false, 1);
      A(M_OCELOT, bx(-.5f,.5f, -8,0, 0,1, 0,15, 1,8,1), 0,9,8, -51.6f); A(M_OCELOT, bx(-.5f,.5f, -8,0, 0,1, 4,15, 1,8,1), 0,4,14, -51.6f);
      for (float x : {-1.1f, 1.1f}) A(M_OCELOT, bx(-1,1, -6,0, 1,3, 8,13, 2,6,2), x,6,5, 0, false, legK(x, 1.f));
      for (float x : {-1.2f, 1.2f}) A(M_OCELOT, bx(-1,1, -10,0, 0,2, 40,0, 2,10,2), x,9.9f,-5, 0, false, legK(x, -1.f)); }
    // patchWolf.1: ModelWolf 1.12.2 (selvagem, textura 64x32): cabeca (pivo x -1) com orelhas e focinho, corpo + juba deitados, 4 pernas, cauda a 36 graus
    { std::vector<V> h = bx(-3,3, -3,3, -2,2, 0,0, 6,6,4), e1 = bx(-3,-1, 3,5, 0,1, 16,14, 2,2,1), e2 = bx(1,3, 3,5, 0,1, 16,14, 2,2,1), no = bx(-1.5f,1.5f, -3,0, -5,-1, 0,10, 3,3,4);
      h.insert(h.end(), e1.begin(), e1.end()); h.insert(h.end(), e2.begin(), e2.end()); h.insert(h.end(), no.begin(), no.end());
      auto wb = bx(-4,2, -11,-2, -3,3, 18,14, 6,9,6), wm = bx(-4,4, -3,3, -3,4, 21,0, 8,6,7); shadeBody(wb); shadeBody(wm);
      A(M_WOLF, wb, 0,10,2, -90); A(M_WOLF, wm, -1,10,-3, -90); A(M_WOLF, h, -1,10.5f,-7, 0, false, 1);
      A(M_WOLF, bx(-1,1, -8,0, -1,1, 9,18, 2,8,2), -1,12,8, -36.f);
      for (float x : {-2.5f, .5f}) for (float z : {7.f, -4.f}) A(M_WOLF, bx(-1,1, -8,0, -1,1, 0,18, 2,8,2), x,8,z, 0, false, legK(x, z)); }
    // patchMob112.1: vagante = esqueleto + camada overlay (wool, 5% maior); esqueleto wither = esqueleto x1.2; zumbi-aldeao = aldeao com bracos estendidos
    MP[M_STRAY] = MP[M_SKELETON]; for (const MPart& p : MP[M_SKELETON]) { MPart o = p; o.wool = true; for (V& v : o.m) { v.x *= 1.06f; v.y *= 1.06f; v.z *= 1.06f; } MP[M_STRAY].push_back(o); }
    MP[M_WSKEL] = MP[M_SKELETON]; for (MPart& p : MP[M_WSKEL]) { for (V& v : p.m) { v.x *= 1.2f; v.y *= 1.2f; v.z *= 1.2f; } p.px *= 1.2f; p.py *= 1.2f; p.pz *= 1.2f; }
    for (const MPart& p : MP[M_VILLAGER]) if (p.rx < 40.f) MP[M_ZVILL].push_back(p);   // sem os bracos cruzados (rx 42.97)
    { auto mu = [](std::vector<V> q) { for (int i = 0; i < (int)q.size() / 4; i++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, q[i * 4 + k].u); hi = std::max(hi, q[i * 4 + k].u); }
            for (int k = 0; k < 4; k++) q[i * 4 + k].u = lo + hi - q[i * 4 + k].u; }
        if (q.size() >= 16) for (int k = 0; k < 4; k++) { std::swap(q[8 + k].u, q[12 + k].u); std::swap(q[8 + k].v, q[12 + k].v); } return q; };
      MP[M_ZVILL].push_back({bx(4,8, -10,2, -2,2, 44,22, 4,12,4), {}, 0, 22, 0, 90.f, false, 0}); MP[M_ZVILL].push_back({mu(bx(-8,-4, -10,2, -2,2, 44,22, 4,12,4)), {}, 0, 22, 0, 90.f, false, 0}); }
    // patchMob112.2: ModelSilverfish/ModelEnderMite (64x32): segmentos em fila, z acumulado (z += (sz[i]+sz[i+1])/2), static (sem ondular)
    { static const int SF[7][5] = {{3,2,2,0,0}, {4,3,2,0,4}, {6,4,3,0,9}, {3,3,3,0,16}, {2,2,3,0,22}, {2,1,2,11,0}, {1,1,2,13,4}};
      static const int EM[4][5] = {{4,3,2,0,0}, {6,4,5,0,5}, {3,3,1,0,14}, {1,2,1,0,18}};
      // patchMobFix.2: nossa convencao = y para CIMA a partir do chao: pivo y = sy (Java 24 - sy) e caixa de -sy a 0 (antes flutuava ~1,4 bloco); kind 40+i/60+i = ondula (ModelSilverfish/ModelEndermite.setRotationAngles)
      auto seg = [&](int t, const int (*S)[5], int n, int k0, float* zs) { float z = -3.5f; for (int i = 0; i < n; i++) { const int sx = S[i][0], sy = S[i][1], sz = S[i][2]; zs[i] = z;
            A(t, bx(-sx / 2.f, sx / 2.f, (float)-sy, 0, sz / -2.f, sz / 2.f, (float)S[i][3], (float)S[i][4], (float)sx, (float)sy, (float)sz), 0, (float)sy, z, 0, false, k0 + i);
            if (i < n - 1) z += (sz + S[i + 1][2]) * .5f; } };
      float zf[7], ze[4]; seg(M_SILVER, SF, 7, 40, zf); seg(M_ENDERMITE, EM, 4, 60, ze);
      A(M_SILVER, bx(-5,5, -8,0, -1.5f,1.5f, 20,0, 10,8,3), 0,8,zf[2], 0, false, 47); A(M_SILVER, bx(-3,3, -4,0, -1.5f,1.5f, 20,11, 6,4,3), 0,4,zf[4], 0, false, 48); A(M_SILVER, bx(-3,3, -5,0, -1.f,1.f, 20,18, 6,5,2), 0,5,zf[1], 0, false, 49); }
    // patchMobNew.1: ModelBat (64x64, escala .35 no desenho; kinds 70-74: corpo/asas batem) e ModelPolarBear (128x64, escala 1.2 no desenho; quadrupede)
    { std::vector<V> h = bx(-3,3, -3,3, -3,3, 0,0, 6,6,6), e1 = bx(-4,-1, 2,6, -2,-1, 24,0, 3,4,1), e2 = bx(1,4, 2,6, -2,-1, 24,0, 3,4,1);
      h.insert(h.end(), e1.begin(), e1.end()); h.insert(h.end(), e2.begin(), e2.end());
      A(M_BAT, h, 0,24,0, 0, false, 1);
      std::vector<V> bd = bx(-3,3, -16,-4, -3,3, 0,16, 6,12,6), bd2 = bx(-5,5, -22,-16, 0,1, 0,34, 10,6,1); bd.insert(bd.end(), bd2.begin(), bd2.end());
      A(M_BAT, bd, 0,24,0, -45);
      A(M_BAT, bx(-12,-2, -17,-1, 1.5f,2.5f, 42,0, 10,16,1), 0,24,0, 0, false, 71); A(M_BAT, bx(-8,0, -13,-1, 0,1, 24,16, 8,12,1), -12,23,1.5f, 0, false, 72);
      A(M_BAT, bx(2,12, -17,-1, 1.5f,2.5f, 42,0, 10,16,1), 0,24,0, 0, false, 73); A(M_BAT, bx(0,8, -13,-1, 0,1, 24,16, 8,12,1), 12,23,1.5f, 0, false, 74); }
    { std::vector<V> h = bx(-3.5f,3.5f, -4,3, -3,4, 0,0, 7,7,7), n = bx(-2.5f,2.5f, -4,-1, -6,-3, 0,44, 5,3,3), r1 = bx(-4.5f,-2.5f, 2,4, -1,0, 26,0, 2,2,1), r2 = bx(2.5f,4.5f, 2,4, -1,0, 26,0, 2,2,1);
      h.insert(h.end(), n.begin(), n.end()); h.insert(h.end(), r1.begin(), r1.end()); h.insert(h.end(), r2.begin(), r2.end());
      A(M_POLAR, h, 0,14,-16, 0, false, 1);
      A(M_POLAR, body(bx(-5,9, -1,13, -7,4, 0,19, 14,14,11)), -2,15,12, -90); A(M_POLAR, body(bx(-4,8, 13,25, -7,3, 39,0, 12,12,10)), -2,15,12, -90);
      for (float x : {-3.5f, 3.5f}) { A(M_POLAR, bx(-2,2, -10,0, -2,4, 50,40, 4,10,6), x,10,6, 0, false, legK(x, 6)); A(M_POLAR, bx(-2,2, -10,0, -2,4, 50,22, 4,10,6), x,10,-8, 0, false, legK(x, -8)); } }
    // patchBabyEgg.1: ModelHorse 1.12.2 (textura 128x128, UVs conferidos no PNG do pack; y do Java negado, py = 24 - rotationPointY). Cabeca+pescoco+crina+orelhas = 1 peca kind 80 (inclina -30 e segue a cabeca); cada perna = coxa+canela+casco (pivo y 15). Burro/mula: orelhas longas abertas em V
    for (const int ht : {(int)M_HORSE, (int)M_DONKEY, (int)M_MULE}) {
        const bool mule = ht != M_HORSE;
        std::vector<V> h = bx(-2.5f,2.5f, 5,10, -1.5f,5.5f, 0,0, 5,5,7), um = bx(-2,2, 7,10, -7,-1, 24,18, 4,3,6), lm = bx(-2,2, 5,7, -6.5f,-1.5f, 24,27, 4,2,5), nk = bx(-2.05f,1.95f, -4.2f,9.8f, -2,6, 0,12, 4,14,8), mn = bx(-1,1, -4.5f,11.5f, 5,9, 58,0, 2,16,4);
        for (auto* q : {&um, &lm, &nk, &mn}) h.insert(h.end(), q->begin(), q->end());
        if (!mule) { std::vector<V> e1 = bx(.45f,2.45f, 9,12, 4,5, 0,0, 2,3,1), e2 = bx(-2.45f,-.45f, 9,12, 4,5, 0,0, 2,3,1); h.insert(h.end(), e1.begin(), e1.end()); h.insert(h.end(), e2.begin(), e2.end()); }
        else for (const float sg : {1.f, -1.f}) {   // muleLeftEar/muleRightEar: 2x7x1 em (0,12), giradas 15 graus em Z em torno do pivo da cabeca
            std::vector<V> e = sg > 0 ? bx(0,2, 9,16, 4,5, 0,12, 2,7,1) : bx(-2,0, 9,16, 4,5, 0,12, 2,7,1); const float c = cosf(.2617994f), sn = sinf(.2617994f) * sg;
            for (V& v : e) { const float x = v.x * c + v.y * sn, y = -v.x * sn + v.y * c; v.x = x; v.y = y; } h.insert(h.end(), e.begin(), e.end()); }
        A(ht, h, 0,20,-10, -30, false, 80); A(ht, bx(-5,5, -2,8, -19,5, 0,34, 10,10,24), 0,13,9);
        A(ht, bx(-5,5, -1,0, -3,5, 80,0, 10,1,8), 0,22,2, 0, false, 90); A(ht, bx(-1.5f,1.5f, 0,1, -3,-1, 106,9, 3,1,2), 0,22,2, 0, false, 90); A(ht, bx(-4,4, 0,1, 3,5, 80,9, 8,1,2), 0,22,2, 0, false, 90);   // patchHorse.2: sela (ModelHorse horseSaddleBottom/Front/Back), so desenhada com e.saddled   // corpo sem shadeBody (nao e girado) e UV v=34
        A(ht, bx(-1,1, -1,1, 0,3, 44,0, 2,2,3), 0,21,14, 65); A(ht, bx(-1.5f,1.5f, -2,2, 3,10, 38,7, 3,4,7), 0,21,14, 65); A(ht, bx(-1.5f,1.5f, .5f,4.5f, 9,16, 24,3, 3,4,7), 0,21,14, 80);   // cauda: base (44,0), meio (38,7), ponta (24,3)
        for (float x : {-4.f, 4.f}) { const bool L = x > 0;
            const float bo = L ? -2.5f : -1.5f, bs = L ? -2.f : -1.f, bh = L ? -2.5f : -1.5f, bu = L ? 78.f : 96.f;   // traseira: coxa 4x9x5, canela 3x5x3, casco 4x3x4
            std::vector<V> bl = bx(bo,bo + 4, -7,2, -2.5f,2.5f, bu,29, 4,9,5), s1 = bx(bs,bs + 3, -12,-7, -1.5f,1.5f, bu,43, 3,5,3), s2 = bx(bh,bh + 4, -15.1f,-12.1f, -2,2, bu,51, 4,3,4);
            bl.insert(bl.end(), s1.begin(), s1.end()); bl.insert(bl.end(), s2.begin(), s2.end()); A(ht, bl, x,15,11, 0, false, legK(x, 11));
            const float fo = L ? -1.9f : -1.1f, fh = L ? -2.4f : -1.6f, fu = L ? 44.f : 60.f;   // dianteira: coxa 3x8x4, canela 3x5x3, casco 4x3x4
            std::vector<V> fl = bx(fo,fo + 3, -7,1, -2.1f,1.9f, fu,29, 3,8,4), f1 = bx(fo,fo + 3, -12,-7, -1.6f,1.4f, fu,41, 3,5,3), f2 = bx(fh,fh + 4, -15.1f,-12.1f, -2.1f,1.9f, fu,51, 4,3,4);
            fl.insert(fl.end(), f1.begin(), f1.end()); fl.insert(fl.end(), f2.begin(), f2.end()); A(ht, fl, x,15,-8, 0, false, legK(x, -8)); } }
    for (int t = M_HBLACK; t <= M_SHORSE; t++) if (t != M_DONKEY && t != M_MULE) MP[t] = MP[M_HORSE];   // pelagens, cavalo zumbi e esqueleto: so a textura muda (burro e mula tem o proprio modelo, com orelhas longas)
    // patchMobNew.2: ModelLlama 1.12.2 (128x64): cabeca (0,17,-6) + pescoco + orelhas, corpo girado -90, 4 pernas de 4x14x4 em y 14
    {   std::vector<V> h = bx(-2,2, 10,14, -10,-1, 0,0, 4,4,9), nk = bx(-4,4, -2,16, -6,0, 0,14, 8,18,6), e1 = bx(-4,-1, 16,19, -4,-2, 17,0, 3,3,2), e2 = bx(1,4, 16,19, -4,-2, 17,0, 3,3,2);   // patchBabyEgg.1: orelhas 3x3x2 em (17,0) acima do pescoco (antes 2x2x2 dentro do pescoco = invisiveis)
        for (auto* q : {&nk, &e1, &e2}) h.insert(h.end(), q->begin(), q->end());
        A(M_LLAMA, h, 0,17,-6, 0, false, 1); A(M_LLAMA, body(bx(-6,6, -8,10, -7,3, 29,0, 12,18,10)), 0,19,2, -90);
        for (float x : {-3.5f, 3.5f}) for (float z : {6.f, -5.f}) A(M_LLAMA, bx(-2,2, -14,0, -2,2, 29,29, 4,14,4), x,14,z, 0, false, legK(x, z)); }
    for (int t = M_LLAMAB; t <= M_LLAMAW; t++) MP[t] = MP[M_LLAMA];
    // patchMobNew.6: ModelWither 1.12.2 (64x64, y do Java negado, pivo y 24; o desenho escala x2): 3 cabecas, barra dos ombros, coluna e 3 costelas (VIS)
    {   A(M_WITHER, bx(-4,4, -4,4, -4,4, 0,0, 8,8,8), 0,24,0, 0, false, 1);
        A(M_WITHER, bx(-4,2, -2,4, -4,2, 32,0, 6,6,6), -8,20,0); A(M_WITHER, bx(-4,2, -2,4, -4,2, 32,0, 6,6,6), 10,20,0);
        std::vector<V> b = bx(-10,10, -6.9f,-3.9f, -.5f,2.5f, 0,16, 20,3,3), sp = bx(-2,1, -16.9f,-6.9f, -.5f,2.5f, 0,22, 3,10,3), r1 = bx(-6,6, -14.9f,-13.9f, -.5f,.5f, 24,22, 12,1,1), r2 = bx(-6,6, -12.9f,-11.9f, -.5f,.5f, 24,22, 12,1,1), r3 = bx(-6,6, -10.9f,-9.9f, -.5f,.5f, 24,22, 12,1,1);
        for (auto* q : {&sp, &r1, &r2, &r3}) b.insert(b.end(), q->begin(), q->end());
        A(M_WITHER, body(b), 0,24,0); }
    // patchMobNew.3: ModelVex 1.12.2 (64x64), aproximado em escala de y para cima (VIS): cabeca 5^3, tronco 3x4x2 + saia, bracos 2x4x2, asas planas (kind 71/73 batem como as do morcego)
    {   std::vector<V> b = bx(-1.5f,1.5f, -4,0, -1,1, 0,16, 3,4,2, -.2f), sk = bx(-1.5f,1.5f, -6,-1, -1,1, 0,26, 3,5,2, -.5f); b.insert(b.end(), sk.begin(), sk.end());
        A(M_VEX, bx(-2.5f,2.5f, 0,5, -2.5f,2.5f, 0,0, 5,5,5), 0,9,0, 0, false, 1); A(M_VEX, b, 0,9,0);
        A(M_VEX, bx(-1,1, -3.5f,.5f, -1,1, 23,0, 2,4,2, -.1f), -1.75f,8.7f,0); A(M_VEX, bx(-1,1, -3.5f,.5f, -1,1, 23,6, 2,4,2, -.1f), 1.75f,8.7f,0);
        A(M_VEX, bx(-.05f,0, -8,0, 0,10, 0,32, 0,8,10), -.5f,8.5f,1, 0, false, 71); A(M_VEX, bx(0,.05f, -8,0, 0,10, 0,32, 0,8,10), .5f,8.5f,1, 0, false, 73); }
    // patchMob112.3: ModelMagmaCube (64x32): 8 fatias (y = 24 - Java) + nucleo; sem casca
    for (int i = 0; i < 8; i++) { const int j = i == 2 || i == 3 ? 24 : 0, k = i == 2 ? 10 : i == 3 ? 19 : i; A(M_MAGMA, bx(-4,4, 7.f - i, 8.f - i, -4,4, (float)j, (float)k, 8,1,8), 0,0,0); }
    A(M_MAGMA, bx(-2,2, 2,6, -2,2, 24,40, 4,4,4), 0,0,0);
    // patchMob112.5: illagers = modelo do aldeao (mesmo UV 64x64); bruxa = aldeao + chapeu (64x128); gatos = jaguatirica; lula = corpo 12x16x12 + 8 tentaculos (64x32, estaticos)
    for (int t : {M_VINDIC, M_EVOKER, M_ILLUS}) MP[t] = MP[M_VILLAGER];
    MP[M_WITCH] = MP[M_VILLAGER];
    A(M_WITCH, bx(-5,5, 9,11, -5,5, 0,64, 10,2,10), 0,24,0, 0, false, 1); A(M_WITCH, bx(-3.5f,3.5f, 11,15, -3.5f,3.5f, 0,76, 7,4,7), 0,24,0, 0, false, 1); A(M_WITCH, bx(-2,2, 15,19, -2,2, 0,87, 4,4,4), 0,24,0, 0, false, 1);
    for (int t : {M_CATBLK, M_CATRED, M_CATSIA}) MP[t] = MP[M_OCELOT];
    A(M_SQUID, bx(-6,6, 4,20, -6,6, 0,0, 12,16,12), 0,0,0);
    for (int i = 0; i < 8; i++) { const float an = i * 6.2831853f / 8.f; A(M_SQUID, bx(-1,1, -18,0, -1,1, 48,0, 2,18,2), cosf(an) * 5.f, 5, sinf(an) * 5.f); }
    // patchMob112.6 (modelos 1.12.2 de memoria, VIS): golem de ferro (128x128), papagaio (32x32), ghast (64x32, x4.5), shulker (64x64). ours = -Java em x e y
    { auto G = [&](int t, std::vector<V> q, float px, float py, float pz, int kind = 0) { A(t, std::move(q), px, py, pz, 0, false, kind); };
      G(M_IGOLEM, bx(-4,4, 2,12, -5.5f,2.5f, 0,0, 8,10,8), 0,31,-2, 1); G(M_IGOLEM, bx(-1,1, 1,5, -7.5f,-5.5f, 24,0, 2,4,2), 0,31,-2, 1);
      G(M_IGOLEM, bx(-9,9, -10,2, -6,5, 0,40, 18,12,11), 0,31,0); G(M_IGOLEM, bx(-4.5f,4.5f, -15,-10, -3,3, 0,70, 9,5,6, .5f), 0,31,0);
      G(M_IGOLEM, bx(9,13, -27.5f,2.5f, -3,3, 60,21, 4,30,6), 0,31,0, 3); G(M_IGOLEM, bx(-13,-9, -27.5f,2.5f, -3,3, 60,58, 4,30,6), 0,31,0, 2);
      G(M_IGOLEM, bx(-2.5f,3.5f, -13,3, -3,2, 37,0, 6,16,5), 4,13,0, 2); G(M_IGOLEM, bx(-3,3, -13,3, -3,2, 37,0, 6,16,5), -5,13,0, 3);
      G(M_PARROT, bx(-1.5f,1.5f, -6,0, -1.5f,1.5f, 2,8, 3,6,3), 0,7.5f,-3); G(M_PARROT, bx(-1.5f,1.5f, -4,1, 0,1, 22,1, 3,4,1), 0,2.9f,1.2f);
      G(M_PARROT, bx(.5f,1.5f, -5,0, -1.5f,1.5f, 19,8, 1,5,3), 1.5f,7,-2.8f); G(M_PARROT, bx(-1.5f,-.5f, -5,0, -1.5f,1.5f, 19,8, 1,5,3), -1.5f,7,-2.8f);
      { std::vector<V> h = bx(-1,1, -1.5f,1.5f, -1,1, 2,2, 2,3,2), h2 = bx(-1,1, -.5f,.5f, -2,2, 10,0, 2,1,4), b1 = bx(-.5f,.5f, -1,1, -2.5f,-1.5f, 11,7, 1,2,1), b2 = bx(-.5f,.5f, -2,0, -2.5f,-1.5f, 16,7, 1,2,1);
        h.insert(h.end(), h2.begin(), h2.end()); h.insert(h.end(), b1.begin(), b1.end()); h.insert(h.end(), b2.begin(), b2.end()); G(M_PARROT, h, 0,8.3f,-2.8f, 1); }
      G(M_PARROT, bx(-.5f,.5f, -2,0, -.5f,.5f, 14,18, 1,2,1), 1.5f,2,-1.5f, 2); G(M_PARROT, bx(-.5f,.5f, -2,0, -.5f,.5f, 14,18, 1,2,1), -1.5f,2,-1.5f, 3);
      G(M_GHAST, bx(-8,8, 0,16, -8,8, 0,0, 16,16,16), 0,0,0);
      for (int i = 0; i < 9; i++) { const int len = 8 + (i * 5) % 7; G(M_GHAST, bx(-1,1, -(float)len,0, -1,1, 0,0, 2,len,2), (i % 3 - 1) * 5.f, 1, (i / 3 - 1) * 5.f); }
      for (MPart& p : MP[M_GHAST]) { for (V& v : p.m) { v.x *= 4.5f; v.y *= 4.5f; v.z *= 4.5f; } p.px *= 4.5f; p.py *= 4.5f; p.pz *= 4.5f; }
      G(M_SHULKER, bx(-8,8, 4,16, -8,8, 0,0, 16,12,16), 0,0,0); G(M_SHULKER, bx(-8,8, 0,8, -8,8, 0,28, 16,8,16), 0,0,0); G(M_SHULKER, bx(-3,3, -6,0, -3,3, 0,52, 6,6,6), 0,12,0, 1); }
    for (int t = M_RWHITE; t <= M_RTOAST; t++) MP[t] = MP[M_RABBIT];   // patchMob112.7 (+ assassino e toast, patchMob112.8)
    for (int t = M_VFARM; t <= M_VBUTCH; t++) MP[t] = MP[M_VILLAGER];   // profissoes: so a textura (sem troca)
    // guardiao (ModelGuardian 64x64, Java y absoluto -> ours 24 - y; sem espinhos): cabeca 12x12x16, placas laterais/topo/base, olho e cauda de 3 segmentos
    { auto G = [&](std::vector<V> q) { A(M_GUARD, std::move(q), 0,0,0); };
      G(bx(-6,6, 2,14, -8,8, 0,0, 12,12,16)); G(bx(-8,-6, 2,14, -6,6, 0,28, 2,12,12)); G(bx(6,8, 2,14, -6,6, 0,28, 2,12,12)); G(bx(-6,6, 14,16, -6,6, 16,40, 12,2,12)); G(bx(-6,6, 0,2, -6,6, 16,40, 12,2,12));
      G(bx(-1,1, 7,9, -8.25f,-7.25f, 8,0, 2,2,1)); G(bx(-2,2, 6,10, 7,15, 40,0, 4,4,8)); G(bx(-2,2, 6,10, 15,23, 0,54, 4,4,8)); G(bx(-1.5f,1.5f, 6.5f,9.5f, 23,30, 41,32, 3,3,7)); }
    MP[M_ELDER] = MP[M_GUARD]; for (MPart& p : MP[M_ELDER]) { for (V& v : p.m) { v.x *= 2.35f; v.y *= 2.35f; v.z *= 2.35f; } p.px *= 2.35f; p.py *= 2.35f; p.pz *= 2.35f; }
    // golem de neve (ModelSnowMan 64x64; pivos Java 4/13/24 -> 20/11/0): cabeca, 2 bracos (galhos), tronco e base
    A(M_SNOWGOLEM, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,20,0, 0, false, 1); A(M_SNOWGOLEM, bx(-1,11, -2,0, -1,1, 32,0, 12,2,2), 4,18,0); A(M_SNOWGOLEM, bx(-11,1, -2,0, -1,1, 32,0, 12,2,2), -4,18,0);
    A(M_SNOWGOLEM, bx(-5,5, 0,10, -5,5, 0,16, 10,10,10), 0,11,0); A(M_SNOWGOLEM, bx(-6,6, 0,12, -6,6, 0,36, 12,12,12), 0,0,0);
    MP[M_HUSK] = MP[M_ZOMBIE];   // patchMob.4: husk = zumbi com outra textura; aranha da caverna = aranha a 70% (VIS)
    MP[M_CSPIDER] = MP[M_SPIDER]; for (MPart& p : MP[M_CSPIDER]) { for (V& v : p.m) { v.x *= .7f; v.y *= .7f; v.z *= .7f; } p.px *= .7f; p.py *= .7f; p.pz *= .7f; }
    for (int t = 0; t < M_N; t++) for (MPart& p : MP[t]) {
        for (V& v : p.m) { v.v *= vs[t][p.wool ? 1 : 0]; v.u *= us[t][p.wool ? 1 : 0]; }
    }
    printf("Mobs: %d textura(s) vieram do resource pack\n", fromPack);
}

// ------------------------------- desenho -------------------------------
static void drawMobFire(const MobEnt& e, float pt);   // nether.inc (usa os tiles animados fire_0/fire_1)
static int gMobUidN = 0, gMobAtkUid = 0;   // gMobAtkUid: uid do mob que esta causando o dano (lido por mobHurt para o revide); 0 = jogador/ambiente
static int mobUid(MobEnt& e) { if (!e.uid) e.uid = ++gMobUidN; return e.uid; }
static void drawMob(const MobEnt& e, float pt) {   // origem = pés, olhando para -Z; pt = partialTicks
    if (!mobReady) loadMobs();
    const MobDef& d = MD[e.type];
    auto L = [&](double a, double b) { return (float)(a + (b - a) * pt); };
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .4f);
    rdPushMatrix(); rdTranslatef(L(e.px, e.x), L(e.py, e.y), L(e.pz, e.z)); rdRotatef(L(e.pyaw, e.yaw), 0, 1, 0); if (isRabbit(e.type)) rdTranslatef(0.f, e.baby ? .03f : .07f, 0.f);   /* patchRabbit.2: modelo subia o corpo inclinado e as patas entravam no chao: sobe ~1px */ /* patchBaby.2: filhote = corpo/pernas a 0,5; cabeca cheia e deslocada (ModelQuadruped.childYOffset 8 / childZOffset 4; ModelChicken 5 / 2) */
    // RendererLivingEntity.rotateCorpse: f = sqrt((deathTime + pt - 1) / 20 * 1.6) limitado a 1; gira 90° em Z em torno dos PÉS (o corpo deita no chão)
    if (e.dead > 0) { const float f = std::min(1.f, sqrtf(std::max(0.f, (e.dead - 2 + pt) / 20.f * 1.6f))); rdRotatef(90.f * f, 0, 0, 1); }
    const float hy = L(e.phyaw, e.hyaw), hpt = L(e.phpit, e.hpit), amt = L(e.plimbAmt, e.limbAmt), ls = e.limb - e.limbAmt * (1.f - pt);
    const float fl = d.ai == 1 ? (sinf(L(e.pwing, e.wing)) + 1.f) * L(e.pflap, e.flap) * 57.29578f : 0.f;   // ModelChicken: rotateAngleZ das asas
    const float aimF = isArch(e.type) ? L(e.paim, e.aim) : 0.f;
    auto draw = [&](bool red) {
        for (int w = 0; w < 2; w++) {
            if (w && (!*d.wool || (e.type == M_SHEEP && e.sheared))) break;
            if (!red) glBindTexture(GL_TEXTURE_2D, mobTex[e.type][w]);
            const bool gel = e.type == M_SLIME && w == 1; if (gel) { rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }   // patchSlime.2: casca de gel (LayerSlimeGel) com blend
            const bool eyes = e.type == M_ENDERMAN && w == 1; float eyeSv[3] = {1.f, 1.f, 1.f};
            if (eyes) {   // patchLight.4: olhos do enderman emissivos (LayerEndermanEyes: sem luz do local, blend aditivo, nao escreve profundidade)
                memcpy(eyeSv, gEntDim, sizeof eyeSv); gEntDim[0] = gEntDim[1] = gEntDim[2] = 1.f; rdEnable(GL_BLEND); rdBlendFunc(GL_ONE, GL_ONE); rdDepthMask(GL_FALSE);
                if (gShaderOK && gShadowShader && gShadowAct && uEnt >= 0) pUniform1f(uEntSky, 15.f);
            }
            for (const MPart& p : MP[e.type]) {
                if (p.wool != (w == 1)) continue;
                if (p.kind == 90 && !e.saddled) continue;   // patchHorse.2
                float rx = p.rx, ry = 0, rz = 0, qxAdd = 0.f, qyAdd = 0.f;
                switch (p.kind) {   // ModelQuadruped / ModelChicken.setRotationAngles (rotação X do nosso sistema = -X do Java)
                    case 1: case 14: rx = -(isLlama(e.type) ? std::max(hpt, -12.f) : hpt); ry = hy; break;   // patchBabyEgg.1: lhama nao inclina o pescoco mais que 12 graus para tras (entrava no corpo)
                    case 80: rx = p.rx - hpt; ry = hy; break;   // patchMobNew.2: cabeca do cavalo (inclinacao fixa de 30 graus + olhar)
                    case 2: rx = -57.29578f * cosf(ls * .6662f) * 1.4f * amt; break;
                    case 3: rx = -57.29578f * cosf(ls * .6662f + 3.14159265f) * 1.4f * amt; break;
                    case 6: rx = -57.29578f * cosf(ls * .6662f) * 1.4f * amt * .5f; break;   // ModelVillager: pernas a metade da amplitude
                    case 7: rx = -57.29578f * cosf(ls * .6662f + 3.14159265f) * 1.4f * amt * .5f; break;
                    case 8: rx = 50.f + (p.rx - 50.f) * aimF; break;   // esqueleto: braco do arco (relaxado -> mira)
                    case 9: rx = 15.f + (p.rx - 15.f) * aimF; break;   // esqueleto: braco que puxa a corda
                    case 10: case 11: case 12: case 13: {   // ModelEnderman: balanco de bracos/pernas pela metade, limitado a +-0,4 rad
                        const float k = (p.kind >= 12 ? 1.4f : 1.f) * .5f, ph = (p.kind == 10 || p.kind == 13) ? 0.f : 3.14159265f;
                        rx = std::max(-22.9f, std::min(22.9f, -57.29578f * cosf(ls * .6662f + ph) * k * amt)); break; }
                    case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 60: case 61: case 62: case 63: {   // ModelSilverfish/Endermite: segmentos serpenteiam (Y gira, X desloca); as 3 "asas" seguem os segmentos 2, 4 e 1
                        const bool em = p.kind >= 60; const int i = em ? p.kind - 60 : p.kind >= 47 ? (p.kind == 47 ? 2 : p.kind == 48 ? 4 : 1) : p.kind - 40; const float a = (float)SDL_GetTicks() * .02f * .9f + i * .15f * 3.14159265f, w = (float)(1 + abs(i - 2));
                        ry = cosf(a) * 3.14159265f * (em ? .01f : .05f) * w * 57.29578f; qxAdd = sinf(a) * 3.14159265f * (em ? .1f : .2f) * (float)abs(i - 2); break; }
                    case 71: case 72: case 73: case 74: {   // morcego: asas batem (cos(idade*1.3) * 45 graus); a asa externa acompanha a ponta da interna e dobra 1,5x
                        const float ph = cosf((float)SDL_GetTicks() * .02f * 1.3f) * 45.f, sg = p.kind <= 72 ? 1.f : -1.f, inner = sg * ph;
                        if (p.kind == 71 || p.kind == 73) rz = inner;
                        else { rz = inner * 1.5f; const float rd = inner * .017453292f; if (p.kind == 72) { qxAdd = -12.f * cosf(rd) - p.px; qyAdd = -12.f * sinf(rd); } else { qxAdd = 12.f * cosf(rd) - p.px; qyAdd = 12.f * sinf(rd); } }
                        break; }
                    case 4: rz = fl; break;
                    case 5: rz = -fl; break;
                }
                float qx = p.px + qxAdd, qy = p.py + qyAdd, qz = p.pz;
                if (p.kind >= 20 && p.kind <= 31) {   // patchVis.2: so kinds 20..31 (antes >=20 sobrescrevia asas do morcego 71-74 e segmentos 40-63 com a orbita do Blaze); ModelBlaze.setRotationAngles: 3 aneis de 4 varetas girando em torno do corpo
                    const int i = p.kind - 20; const float age = (float)SDL_GetTicks() * .02f, PI = 3.14159265f; float ang, R, yy;
                    if (i < 4) { ang = age * PI * -.1f + i * PI * .5f; R = 9.f; yy = -2.f + cosf((i * 2 + age) * .25f); }
                    else if (i < 8) { ang = PI * .25f + (i - 4) * PI * .5f + age * PI * .03f; R = 7.f; yy = 2.f + cosf((i * 2 + age) * .25f); }
                    else { ang = .47123889f + (i - 8) * PI * .5f + age * PI * -.05f; R = 5.f; yy = 11.f + cosf((i * 1.5f + age) * .5f); }
                    qx = cosf(ang) * R; qz = sinf(ang) * R; qy = 24.f - yy;
                }
                if (e.type == M_ENDERMAN && e.endAgg > 0 && p.kind == 1) { qy += 5.f; qx += sinf((float)SDL_GetTicks() * .9f) * .6f; }   // irritado: boca aberta e cabeca tremendo
                if (e.baby) {
                    const bool bRb = isRabbit(e.type), bOc = (e.type == M_OCELOT || (e.type >= M_CATBLK && e.type <= M_CATSIA));   // patchBabyFix.1: ModelRabbit (cabeca .56, corpo .4) e ModelOcelot (cabeca .75, corpo .5)
                    if (p.kind == 1 && (bRb || bOc)) { const float hs = bRb ? .56f : .75f, oy = bRb ? 22.f : 10.f, oz = 4.f; const float fy = 24.f - hs * ((24.f - qy) + oy), fz = hs * (qz + oz), fx = hs * qx; rdPushMatrix(); rdScalef(hs, hs, hs); part(red ? p.red : p.m, fx / hs, fy / hs, fz / hs, rx, ry, rz); rdPopMatrix(); }
                    else if (p.kind == 1) { qy -= (e.type == M_CHICKEN ? 5.f : 8.f); qz += (e.type == M_CHICKEN ? 2.f : 4.f); part(red ? p.red : p.m, qx, qy, qz, rx, ry, rz); }
                    else { const float bs = bRb ? .4f : .5f; rdPushMatrix(); rdScalef(bs, bs, bs); part(red ? p.red : p.m, qx, qy, qz, rx, ry, rz); rdPopMatrix(); }
                } else
                part(red ? p.red : p.m, qx, qy, qz, rx, ry, rz);
            }
            if (gel) { rdDisable(GL_BLEND); rdEnable(GL_ALPHA_TEST); }
            if (eyes) { memcpy(gEntDim, eyeSv, sizeof eyeSv); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDisable(GL_BLEND); rdDepthMask(GL_TRUE); if (gShaderOK && gShadowShader && gShadowAct && uEnt >= 0) pUniform1f(uEntSky, gEntSk); }
        }
    };
    if (e.type == M_CREEPER && e.fuse > 0) { float f = std::min(1.f, (e.fuse + pt) / 30.f); const float f2 = 1.f + sinf(f * 100.f) * f * .01f; f *= f; f *= f; rdScalef((1.f + f * .4f) * f2, (1.f + f * .1f) / f2, (1.f + f * .4f) * f2); }   // EntityCreeper: incha ao acender o pavio
    gEntHurt = (e.hurt > 0 || e.dead > 0 || (e.type == M_CREEPER && e.fuse > 0 && ((e.fuse / 3) & 1) == 0)) ? 1.f : 0.f;   // flash vermelho ao apanhar e na morte: tinge a propria textura (a copia vermelha sem textura pintava a caixa inteira dos pes da galinha)
    if (isSlimy(e.type)) { const float sq = (e.psqF + (e.sqF - e.psqF) * pt) / ((float)e.ssz * .5f + 1.f), f1 = 1.f / (sq + 1.f); rdScalef(f1 * e.ssz, (1.f / f1) * e.ssz, f1 * e.ssz); }   // RenderSlime.preRenderCallback: esmaga/estica pelo squish e escala pelo tamanho
    { const float ms = e.type == M_BAT ? .35f : e.type == M_POLAR ? 1.2f : e.type == M_DONKEY ? .87f : e.type == M_MULE ? .92f : e.type == M_WITHER ? 2.f : 1.f; if (ms != 1.f) { rdPushMatrix(); rdScalef(ms, ms, ms); } draw(false); if (ms != 1.f) rdPopMatrix(); } gEntHurt = 0.f;   // patchMobNew.1: morcego .35, urso polar 1.2 (RenderBat/RenderPolarBear)
    if (isArch(e.type) && e.dead == 0) {   // arco na mao direita: mesma cadeia do jogador (braco -> -90 em X -> punho); puxa em 3 estagios (bow_pulling_0..2) nos 20 ticks antes do tiro
        const int t = (aimF > .5f && e.atk < 20) ? 20 - e.atk : -1;
        rdPushMatrix(); rdTranslatef(5 / 16.f, 22 / 16.f, 0); rdRotatef(50.f + (85.f - 50.f) * aimF, 1, 0, 0);
        rdRotatef(-90, 1, 0, 0); rdTranslatef(0, .125f, -.625f); drawHeld(bowPullItemT(IT_BOW, t), false); rdPopMatrix();
    }
    if (e.type == M_ENDERMAN && e.endBlk && e.dead == 0) { rdPushMatrix(); rdTranslatef(0.f, 1.45f, -.5f); rdRotatef(20.f, 0, 1, 0); rdScalef(.9f, .9f, .9f); drawHeld(e.endBlk, false); rdPopMatrix(); }   // bloco carregado (VIS)
    if (e.fire > 0 && e.dead == 0) drawMobFire(e, pt);
    rdPopMatrix(); rdDisable(GL_ALPHA_TEST);
}
static void drawPuffs(float pt) {   // bolinhas cinza que sobem devagar, sempre viradas para a câmera (usa a matriz atual: direita/cima da câmera)
    if (PUFFS.empty()) return;
    float m[16]; rdGetMatrixf(GL_MODELVIEW_MATRIX, m);
    const float rx = m[0], ry = m[4], rz = m[8], ux = m[1], uy = m[5], uz = m[9];
    const GLboolean wasBlend = rdIsEnabled(GL_BLEND);
    rdDisable(GL_TEXTURE_2D); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDepthMask(GL_FALSE);
    for (const Puff& p : PUFFS) {
        const float x = p.px + (p.x - p.px) * pt, y = p.py + (p.y - p.py) * pt, z = p.pz + (p.z - p.pz) * pt, s = .1f * p.s;
        if (p.kind == 2) {   // patchTnt.2: nuvem da explosao (EntityLargeExplodeFX): bola cinza que escurece, com miolo claro e borda macia
            const float t = (p.age + pt) / p.max, g = std::max(.28f, p.c * (1.f - .6f * t)), al = std::max(0.f, 1.f - t * t);
            for (int ring = 0; ring < 3; ring++) {
                const float rs = s * (1.f - ring * .28f), gg = std::min(1.f, g + ring * .13f); glColor4f(gg, gg, gg, al * (ring == 0 ? .55f : .6f));
                glBegin(GL_TRIANGLE_FAN); glVertex3f(x, y, z);
                for (int k = 0; k <= 12; k++) { const float a = k * .5236f, cs = cosf(a) * rs, sn = sinf(a) * rs; glVertex3f(x + rx * cs + ux * sn, y + ry * cs + uy * sn, z + rz * cs + uz * sn); }
                glEnd();
            }
            continue;
        }
        if (p.kind == 3) {   // patchHeart.1: coracao do modo amor (EntityHeartFX): sobe devagar, rosa/vermelho, some no fim
            const float t = (p.age + pt) / p.max, al = std::max(0.f, std::min(1.f, 2.f * (1.f - t))), hs = s * .09f;
            glColor4f(1.f, .12f, .2f, al); glBegin(GL_TRIANGLE_FAN); glVertex3f(x, y, z);
            for (int k = 0; k <= 24; k++) { const float a = k * .2618f, sa = sinf(a), hx = 16.f * sa * sa * sa, hy = 13.f * cosf(a) - 5.f * cosf(2 * a) - 2.f * cosf(3 * a) - cosf(4 * a), cs = hx * hs, sn = (hy + 2.f) * hs; glVertex3f(x + rx * cs + ux * sn, y + ry * cs + uy * sn, z + rz * cs + uz * sn); }
            glEnd(); continue;
        }
        if (p.kind == 1) glColor4f(.3f, .55f, 1.f, std::max(0.f, .75f * (1.f - (p.age + pt) / p.max))); else glColor4f(p.c, p.c, p.c, std::max(0.f, .8f * (1.f - (p.age + pt) / p.max)));
        glBegin(GL_TRIANGLE_FAN); glVertex3f(x, y, z);
        for (int k = 0; k <= 8; k++) { const float t = k * .785398f, cs = cosf(t) * s, sn = sinf(t) * s; glVertex3f(x + rx * cs + ux * sn, y + ry * cs + uy * sn, z + rz * cs + uz * sn); }
        glEnd();
    }
    rdDepthMask(GL_TRUE); if (!wasBlend) rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
static void drawHitboxes(float pt) {   // F3+B: caixa branca = AABB do mob, retângulo vermelho = altura dos olhos
    rdDisable(GL_TEXTURE_2D); rdDisable(GL_ALPHA_TEST); glLineWidth(2.f); glBegin(GL_LINES);
    for (const MobEnt& e : MOBS) {
        const float x = (float)(e.px + (e.x - e.px) * pt), y = (float)(e.py + (e.y - e.py) * pt), z = (float)(e.pz + (e.z - e.pz) * pt);
        const float hw = mobW(e) * .5f, h = mobH(e), ey = y + h * (e.type == M_SHEEP ? .95f : .85f);
        const float X0 = x - hw, X1 = x + hw, Z0 = z - hw, Z1 = z + hw, Y0 = y, Y1 = y + h;
        auto L = [](float a, float b, float c, float d, float f, float g) { glVertex3f(a, b, c); glVertex3f(d, f, g); };
        glColor3f(1.f, 1.f, 1.f);
        L(X0,Y0,Z0,X1,Y0,Z0); L(X1,Y0,Z0,X1,Y0,Z1); L(X1,Y0,Z1,X0,Y0,Z1); L(X0,Y0,Z1,X0,Y0,Z0);
        L(X0,Y1,Z0,X1,Y1,Z0); L(X1,Y1,Z0,X1,Y1,Z1); L(X1,Y1,Z1,X0,Y1,Z1); L(X0,Y1,Z1,X0,Y1,Z0);
        L(X0,Y0,Z0,X0,Y1,Z0); L(X1,Y0,Z0,X1,Y1,Z0); L(X1,Y0,Z1,X1,Y1,Z1); L(X0,Y0,Z1,X0,Y1,Z1);
        glColor3f(1.f, 0.f, 0.f);
        L(X0,ey,Z0,X1,ey,Z0); L(X1,ey,Z0,X1,ey,Z1); L(X1,ey,Z1,X0,ey,Z1); L(X0,ey,Z1,X0,ey,Z0);
    }
    glEnd(); glLineWidth(1.f); glColor4f(1.f, 1.f, 1.f, 1.f); rdEnable(GL_TEXTURE_2D);
}
static void drawMobs(float pt = 1.f) { for (const MobEnt& e : MOBS) {
    if (entFarFrom(e.x, e.z, std::min(gEntDist > 0.0 ? gEntDist : (gPerf ? 48.0 : 80.0), RAD * 16.0))) continue;   // patchRpiOpt.6 / patchFps.5: tambem no PC, a 80 blocos (o MC corta ~64)
    if (entOutView(e.x, e.y + 1.0, e.z, 3.5f)) continue;   // patchRpiCull.1: fora da tela (raio 3,5 cobre ghast/dragao pequeno/pernas)
    entLightAt(e.x, e.y + .7, e.z, gEntDim); const bool es = entShaderOn(); drawMob(e, pt); if (es) skyShaderOff(); } entDimReset(); drawPuffs(pt); if (gHitbox) drawHitboxes(pt); }

// ------------------------------- spawn, itens e dano -------------------------------
static void spawnMob(int type, double x, double y, double z, float yaw, bool baby = false) {
    if (type == M_SPIDER || type == M_CSPIDER) return;   /* patchMob.5: aranha removida do jogo */
    MobEnt e; e.type = type; e.x = e.px = x; e.y = e.py = y; e.z = e.pz = z; e.yaw = e.pyaw = yaw; e.hp = MD[type].hp; e.eggT = 6000 + rand() % 6000; e.baby = baby && mobCanBaby(type); e.babyAge = 0;
    if (type == M_SHULKER) { e.x = e.px = floor(x) + .5; e.z = e.pz = floor(z) + .5; e.yaw = e.pyaw = 0.f; }   // patchTexFix.2: shulker alinhado ao bloco (centro da celula, sem rotacao arbitraria)
    if (isSlimy(type)) { e.ssz = 1 << (rand() % 3); e.hp = e.ssz * e.ssz; e.sjump = 10 + rand() % 20; }   // patchSlime.2: tamanho 1, 2 ou 4 (EntitySlime.initializeSlime / ovo)
    MOBS.push_back(e);
}
static std::vector<std::array<double, 3>> gVexQ;   // patchMobNew.4: evocadores pedem vexes aqui (spawnar dentro do laco invalidaria a referencia do mob)
static void spawnSlime(int size, double x, double y, double z, float yaw, double mx = 0, double mz = 0, int st = M_SLIME) { spawnMob(st, x, y, z, yaw); MobEnt& e = MOBS.back(); e.ssz = std::max(1, std::min(4, size)); e.hp = e.ssz * e.ssz; e.mx = mx; e.mz = mz; }
struct SlimeSp { double x, y, z; int sz; int t = M_SLIME; };   // divisao pendente: criada fora do laco dos mobs (push_back invalidaria referencias)
static std::vector<SlimeSp> gSlimeQ;
static void slimeFlush() { if (gSlimeQ.empty()) return; std::vector<SlimeSp> q; q.swap(gSlimeQ); for (const SlimeSp& s : q) spawnSlime(s.sz, s.x, s.y, s.z, frand() * 360.f, (frand() - .5) * .3, (frand() - .5) * .3, s.t); }
static void spawnMobFront(int type, const Phys& P, float yaw, float side = 0.f) {   // 2 blocos à frente (side = deslocamento lateral), no chão, virado para o jogador
    const float r = yaw * .0174533f;
    const double x = P.x - sin(r) * 2.0 + cos(r) * side, z = P.z - cos(r) * 2.0 - sin(r) * side;
    double y = P.y;
    for (int by = (int)floor(P.y) + 3; by >= 0; by--) if (solid(getB((int)floor(x), by, (int)floor(z)))) { y = by + 1.0; break; }
    spawnMob(type, x, y, z, yaw + 180.f);
}
static int mobPkItem(const char* file) { for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == file) return PK0 + (int)i; return 0; }   // item do pack pelo nome
static int eggMob(int id) {
    { static const int ze = mobPkItem("item/zombie_spawn_egg"), se = mobPkItem("item/skeleton_spawn_egg"), ce = mobPkItem("item/creeper_spawn_egg"), spe = mobPkItem("item/spider_spawn_egg"), hue = mobPkItem("item/husk_spawn_egg"), cse = mobPkItem("item/cave_spider_spawn_egg"), be = mobPkItem("item/blaze_spawn_egg"); if (id) { if (id == ze) return M_ZOMBIE; if (id == se) return M_SKELETON; if (id == ce) return M_CREEPER; if (be && id == be) return M_BLAZE; if (spe && id == spe) return M_SPIDER; if (hue && id == hue) return M_HUSK; if (cse && id == cse) return M_CSPIDER; } }
    { static const char* NM[4] = {"pig", "chicken", "cow", "sheep"}; for (int k = 0; k < 4; k++) { static int ids[4] = {-1, -1, -1, -1}; if (ids[k] < 0) ids[k] = mobPkItem((std::string("item/") + NM[k] + "_spawn_egg").c_str()); if (ids[k] > 0 && id == ids[k]) return k; } }   // patchInv.3: ovos do pack = mesmos mobs
    { static const int me = mobPkItem("item/mooshroom_spawn_egg"); if (me && id == me) return M_MOOSH; }
    { static const int ee = mobPkItem("item/enderman_spawn_egg"); if (ee && id == ee) return M_ENDERMAN; }
    { static const int sle = mobPkItem("item/slime_spawn_egg"), mge = mobPkItem("item/magma_cube_spawn_egg"); if (sle && id == sle) return M_SLIME; if (mge && id == mge) return M_MAGMA; }
    { static const int rbe = mobPkItem("item/rabbit_spawn_egg"); if (rbe && id == rbe) return M_RABBIT; }
    { static const int oce = mobPkItem("item/ocelot_spawn_egg"); if (oce && id == oce) return M_OCELOT; }
    { static const int wfe = mobPkItem("item/wolf_spawn_egg"); if (wfe && id == wfe) return M_WOLF; }
    { static const int bte = mobPkItem("item/bat_spawn_egg"), pbe = mobPkItem("item/polar_bear_spawn_egg"); if (bte && id == bte) return M_BAT; if (pbe && id == pbe) return M_POLAR; }   // patchMobNew.1
    { static const int vxe = mobPkItem("item/vex_spawn_egg"); if (vxe && id == vxe) return M_VEX; }   // patchMobNew.3
    {   static const char* EH[6] = {"item/horse_spawn_egg", "item/donkey_spawn_egg", "item/mule_spawn_egg", "item/zombie_horse_spawn_egg", "item/skeleton_horse_spawn_egg", "item/llama_spawn_egg"};   // patchMobNew.2
        static const int TH[6] = {M_HORSE, M_DONKEY, M_MULE, M_ZHORSE, M_SHORSE, M_LLAMA}; static int hi[6] = {-1, -1, -1, -1, -1, -1};
        for (int k = 0; k < 6; k++) { if (hi[k] < 0) hi[k] = mobPkItem(EH[k]); if (hi[k] > 0 && id == hi[k]) return k == 0 ? M_HORSE + rand() % 4 : k == 5 ? M_LLAMA + rand() % 4 : TH[k]; } }
    { static const int sf = mobPkItem("item/silverfish_spawn_egg"), em = mobPkItem("item/endermite_spawn_egg"); if (id) { if (sf && id == sf) return M_SILVER; if (em && id == em) return M_ENDERMITE; } }
    { static const int gq = mobPkItem("item/guardian_spawn_egg"), eg = mobPkItem("item/elder_guardian_spawn_egg"); if (id) { if (gq && id == gq) return M_GUARD; if (eg && id == eg) return M_ELDER; } }
    { static const char* EN[8] = {"item/squid_spawn_egg", "item/witch_spawn_egg", "item/vindicator_spawn_egg", "item/evoker_spawn_egg", "item/parrot_spawn_egg", "item/ghast_spawn_egg", "item/shulker_spawn_egg", ""}; static const int ET[8] = {M_SQUID, M_WITCH, M_VINDIC, M_EVOKER, M_PARROT, M_GHAST, M_SHULKER, -1};
      static int ei[8] = {-1, -1, -1, -1, -1, -1, -1, 0}; for (int k = 0; k < 7; k++) { if (ei[k] < 0) ei[k] = mobPkItem(EN[k]); if (ei[k] > 0 && id == ei[k]) return ET[k]; } }   // patchMob112.5
    { static const int sy = mobPkItem("item/stray_spawn_egg"), ws = mobPkItem("item/wither_skeleton_spawn_egg"), zv = mobPkItem("item/zombie_villager_spawn_egg"); if (id) { if (sy && id == sy) return M_STRAY; if (ws && id == ws) return M_WSKEL; if (zv && id == zv) return M_ZVILL; } }   // patchMob112.1   // patchWolf.1   // patchOcelot.1   // patchRabbit.1   // patchSlime.2
    static const int vegg = mobPkItem("item/villager_spawn_egg"), pegg = mobPkItem("item/zombified_piglin_spawn_egg"); if (vegg && id == vegg) return M_VILLAGER; if (pegg && id == pegg) return M_PIGMAN; return id >= IT_EGG_PIG && id <= IT_EGG_SHEEP ? id - IT_EGG_PIG : -1; }
static int mobEggItem(int t) {   // inverso de eggMob: ovo do mob (0 = nao tem); usado no pick block (botao do meio) em entidade
    switch (t) {
        case M_PIG: return IT_EGG_PIG; case M_CHICKEN: return IT_EGG_CHICKEN; case M_COW: return IT_EGG_COW; case M_MOOSH: return mobPkItem("item/mooshroom_spawn_egg"); case M_SHEEP: return IT_EGG_SHEEP;
        case M_ZOMBIE: return mobPkItem("item/zombie_spawn_egg"); case M_SKELETON: return mobPkItem("item/skeleton_spawn_egg"); case M_CREEPER: return mobPkItem("item/creeper_spawn_egg"); case M_BLAZE: return mobPkItem("item/blaze_spawn_egg"); case M_SPIDER: return mobPkItem("item/spider_spawn_egg"); case M_HUSK: return mobPkItem("item/husk_spawn_egg"); case M_CSPIDER: return mobPkItem("item/cave_spider_spawn_egg");
        case M_ENDERMAN: return mobPkItem("item/enderman_spawn_egg");
        case M_SLIME: return mobPkItem("item/slime_spawn_egg"); case M_MAGMA: return mobPkItem("item/magma_cube_spawn_egg");
        case M_RABBIT: case M_RWHITE: case M_RBLACK: case M_RGOLD: case M_RSALT: case M_RSPLOT: case M_RKILLER: case M_RTOAST: return mobPkItem("item/rabbit_spawn_egg");
        case M_GUARD: return mobPkItem("item/guardian_spawn_egg"); case M_ELDER: return mobPkItem("item/elder_guardian_spawn_egg"); case M_VFARM: case M_VLIB: case M_VPRIEST: case M_VSMITH: case M_VBUTCH: return mobPkItem("item/villager_spawn_egg");
        case M_OCELOT: return mobPkItem("item/ocelot_spawn_egg");
        case M_WOLF: return mobPkItem("item/wolf_spawn_egg");
        case M_BAT: return mobPkItem("item/bat_spawn_egg"); case M_POLAR: return mobPkItem("item/polar_bear_spawn_egg");
        case M_HORSE: case M_HBLACK: case M_HWHITE: case M_HGRAY: return mobPkItem("item/horse_spawn_egg"); case M_DONKEY: return mobPkItem("item/donkey_spawn_egg"); case M_MULE: return mobPkItem("item/mule_spawn_egg");
        case M_VEX: return mobPkItem("item/vex_spawn_egg"); case M_ZHORSE: return mobPkItem("item/zombie_horse_spawn_egg"); case M_SHORSE: return mobPkItem("item/skeleton_horse_spawn_egg"); case M_LLAMA: case M_LLAMAB: case M_LLAMAG: case M_LLAMAW: return mobPkItem("item/llama_spawn_egg");   // patchMobNew.2
        case M_SILVER: return mobPkItem("item/silverfish_spawn_egg"); case M_ENDERMITE: return mobPkItem("item/endermite_spawn_egg");
        case M_PARROT: return mobPkItem("item/parrot_spawn_egg"); case M_GHAST: return mobPkItem("item/ghast_spawn_egg"); case M_SHULKER: return mobPkItem("item/shulker_spawn_egg");
        case M_SQUID: return mobPkItem("item/squid_spawn_egg"); case M_WITCH: return mobPkItem("item/witch_spawn_egg"); case M_VINDIC: return mobPkItem("item/vindicator_spawn_egg"); case M_EVOKER: return mobPkItem("item/evoker_spawn_egg");
        case M_STRAY: return mobPkItem("item/stray_spawn_egg"); case M_WSKEL: return mobPkItem("item/wither_skeleton_spawn_egg"); case M_ZVILL: return mobPkItem("item/zombie_villager_spawn_egg");
        case M_VILLAGER: return mobPkItem("item/villager_spawn_egg"); case M_PIGMAN: return mobPkItem("item/zombified_piglin_spawn_egg");
    }
    return 0;
}
static bool mobInteract(const float* eye, const float* dir, float reach) {   // clique direito num aldeao: ele para e olha para o jogador
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || MD[e.type].ai != 2) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;
    MOBS[best].talkT = 100; MOBS[best].hasT = false; return true;
}
// ---- patchHorse.1: sela e montaria de cavalo/burro/mula (so 1 jogador local; sem domar, bau nem armadura) ----
static void spawnHearts(double x, double y, double z, int n);
static int gRideUid = 0; static float gRideFwd = 0.f, gRideYaw = 0.f; static bool gRideJump = false;   // mob montado (uid, 0 = nenhum) e comandos do jogador
static inline bool isEquine(int t) { return t == M_HORSE || (t >= M_HBLACK && t <= M_SHORSE); }
static inline bool isSaddleable(int t) { return isEquine(t) || t == M_PIG; }   // patchPig.1: porco tambem leva sela (nao doma; so anda com cenoura no palito)
static MobEnt* rideMob() { if (!gRideUid) return nullptr; for (MobEnt& m : MOBS) if (m.uid == gRideUid) return &m; return nullptr; }
static bool horseInteract(const float* eye, const float* dir, float reach, Slot& held, bool creative) {   // clique direito: sela (item na mao) ou monta (sela posta)
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || !isSaddleable(e.type)) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;
    MobEnt& e = MOBS[best]; static const int SAD = mobPkItem("item/saddle");
    static const int FOOD[6] = {mobPkItem("item/sugar"), mobPkItem("item/wheat"), mobPkItem("item/apple"), mobPkItem("item/golden_apple"), mobPkItem("item/golden_carrot"), mobPkItem("item/carrot")};
    if (e.baby) return false;
    if (e.type == M_PIG) {   // porco: sela direto (sem domar); montar so com sela
        if (!e.saddled && SAD && held.id == SAD) { e.saddled = true; e.tame = true; if (!creative && --held.n <= 0) held = Slot(); return true; }
        if (e.saddled && !gRideUid) { gRideUid = mobUid(e); e.hasT = false; e.panic = 0; return true; }
        return false; }
    if (held.id && !e.tame) for (int k = 0; k < 6; k++) if (FOOD[k] && held.id == FOOD[k]) {   // comida aumenta o temperamento (mais chance de domar)
        e.temper = std::min(100, e.temper + (k == 3 || k == 4 ? 5 : 3)); spawnHearts(e.x, e.y + mobH(e) * .6, e.z, 3); if (!creative && --held.n <= 0) held = Slot(); return true; }
    if (e.tame && !e.saddled && SAD && held.id == SAD) { e.saddled = true; if (!creative && --held.n <= 0) held = Slot(); return true; }
    if (!gRideUid) { gRideUid = mobUid(e); e.hasT = false; e.panic = 0; e.rideT = 0; e.rideLim = 40 + rand() % 60; if (!e.tame) e.temper = std::min(100, e.temper + 5); return true; }   // monta (selvagem: tenta domar)
    return false;
}
static bool horseTick(Phys& P, bool fwdK, bool backK, bool sneak, bool jumpK, bool sprint, float yaw, bool carrotStick = false) {   // chamado no tick do jogador; true = montado (a posicao do jogador segue o cavalo)
    if (!gRideUid) return false;
    MobEnt* m = rideMob(); bool toss = false;
    if (m && !m->dead && !m->tame && ++m->rideT >= m->rideLim) {   // selvagem: teste de domar (rand(100) < temperamento); senao derruba o jogador
        if (rand() % 100 < m->temper) { m->tame = true; spawnHearts(m->x, m->y + mobH(*m) * .6, m->z, 7); chatAdd("Cavalo domado!", 0x55FF55); }
        else toss = true; }
    if (!m || m->dead || sneak || toss) {   // desmontar (agachar), ser derrubado ou o cavalo morreu: desce ao lado, sobre chao livre
        if (m && !m->dead) { const double r = (m->yaw + 90.f) * .017453292519943295; double px = m->x, py = m->y, pz = m->z; bool ok = false;
            for (int s = 1; s >= -1 && !ok; s -= 2) { const double cx = m->x - sin(r) * 1.1 * s, cz = m->z - cos(r) * 1.1 * s; const int bx = (int)floor(cx), bz = (int)floor(cz), by = (int)floor(m->y + .05);
                if (!solid(getB(bx, by, bz)) && !solid(getB(bx, by + 1, bz))) { px = cx; pz = cz; ok = true; } }
            if (!ok) py = m->y + mobH(*m) + .1;
            P.x = px; P.y = py; P.z = pz;
            if (toss) { m->panic = 60; m->hasT = false; m->my = .3; m->rideT = 0; P.my = .3; P.mx = -sin(r) * .25 * (rand() % 2 ? 1 : -1); P.mz = -cos(r) * .25; P.onGround = false; } }
        gRideUid = 0; gRideFwd = 0.f; gRideJump = false; if (!toss) P.mx = P.my = P.mz = 0; return false;
    }
    const bool pig = m->type == M_PIG, ctl = m->saddled && (!pig || carrotStick);   // porco: so obedece com a cenoura no palito na mao
    gRideYaw = yaw; gRideJump = jumpK && ctl && !pig; gRideFwd = ctl ? (fwdK ? 1.f : 0.f) * (sprint ? 1.35f : 1.f) - (backK ? .5f : 0.f) : 0.f;   // sem sela nao da para guiar
    P.x = m->x; P.y = m->y + mobH(*m) * .75 - (pig ? .35 : .6); P.z = m->z; P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; P.flying = false; P.inWater = false; P.sneak = false;
    return true;
}
struct VilSpawn { double x, y, z; }; static std::vector<VilSpawn> gVilQ;   // aldeoes pedidos pela geracao de aldeias (criados no tick)
static std::vector<VilSpawn> gVilCenters;   // patchVil.1: centros das vilas ja geradas (reposicao natural de aldeoes)
static void vilQueueVillagers(double x, double y, double z) { static std::set<std::pair<long long, long long>> done; if (!done.insert({(long long)floor(x), (long long)floor(z)}).second) return; gVilCenters.push_back({x, y, z}); for (int i = 0; i < 3; i++) gVilQ.push_back({x + (i - 1) * 1.2, y, z + 2.5}); }
static void vilSpawnPending() {
    std::vector<VilSpawn> keep;
    for (const VilSpawn& s : gVilQ) {
        const int bx = (int)floor(s.x), bz = (int)floor(s.z);
        Chunk* ch = getC(fdiv(bx, CW), fdiv(bz, CW));
        if (!ch || !ch->lc) { keep.push_back(s); continue; }   // chunk ainda nao pronto: tenta de novo no proximo tick (antes o pedido era descartado e o aldeao nascia na terra)
        int c = 0; for (const MobEnt& m : MOBS) if (m.type == M_VILLAGER && fabs(m.x - s.x) < 24 && fabs(m.z - s.z) < 24) c++;
        if (c >= 3) continue;
        // chao livre mais proximo da altura pedida: bloco solido embaixo e 2 celulas livres (nao solidas, sem liquido) em cima; evita telhado e interior de parede
        int fy = -1, bestD = 1 << 30;
        for (int by = std::min(CH - 3, (int)floor(s.y) + 4); by >= std::max(1, (int)floor(s.y) - 6); by--) {
            if (!solid(getB(bx, by - 1, bz)) || solid(getB(bx, by, bz)) || solid(getB(bx, by + 1, bz)) || isLiquid(getB(bx, by, bz))) continue;
            const int dd = abs(by - (int)floor(s.y)); if (dd < bestD) { bestD = dd; fy = by; }
        }
        if (fy < 0) continue;   // sem lugar livre naquela coluna: nao nasce dentro de bloco
        spawnMob(M_VILLAGER, s.x, (double)fy, s.z, frand() * 360.f); MOBS.back().hasHome = true; MOBS.back().homeX = s.x; MOBS.back().homeZ = s.z - 2.5;
    }
    gVilQ.swap(keep);
}
static void vilNaturalTick(const Phys& P) {   // patchVil.1: a cada 30 s, vila ate 96 blocos do jogador com menos de 3 aldeoes por perto ganha 1 (o teto de 3 e o filtro de chao ficam em vilSpawnPending)
    static int t = 0; if (++t < 600) return; t = 0;
    if (!gVilQ.empty()) return;
    for (const VilSpawn& c : gVilCenters) {
        const double dx = c.x - P.x, dz = c.z - P.z; if (dx * dx + dz * dz > 96.0 * 96.0) continue;
        int n = 0; for (const MobEnt& m : MOBS) if (!m.dead && m.type == M_VILLAGER && fabs(m.x - c.x) < 24 && fabs(m.z - c.z) < 24) n++;
        if (n < 3) gVilQ.push_back({c.x + (rand() % 5 - 2), c.y, c.z + 2.5});
    }
}
static float potMobMul(const MobEnt& e) { float m = 1.f; if (e.potSpeed > 0) m *= 1.f + .2f * (e.potSpeedAmp + 1); if (e.potSlow > 0) m *= std::max(0.f, 1.f - .15f * (e.potSlowAmp + 1)); return m; }   // patchPot.3
static int mobWeaponDmg(int id) {   // dano base do 1.8.9 (4 + material nas espadas, 3 machados, 2 picaretas, 1 pás); sem arma = 1
    static const int S[5] = {4, 5, 6, 4, 7}, AX[5] = {3, 4, 5, 3, 6}, PK[5] = {2, 3, 4, 2, 5}, SH[5] = {1, 2, 3, 1, 4};
    if (id == IT_WSWORD) return S[0]; if (id >= IT_SSWORD && id <= IT_DSWORD) return S[id - IT_SSWORD + 1];
    if (id >= IT_WAXE && id < IT_WAXE + 5) return AX[id - IT_WAXE]; if (id >= IT_WSHOVEL && id < IT_WSHOVEL + 5) return SH[id - IT_WSHOVEL];
    if (id == IT_WPICK) return PK[0]; if (id == IT_SPICK) return PK[1]; if (id >= IT_IPICK && id <= IT_DPICK) return PK[id - IT_IPICK + 2];
    return 1;
}
static int gLootLv = 0;   // patchEnch.4: pilhagem da arma que matou (ligado so em mobAttack)
static void mobDrops(const MobEnt& e) {
    if (e.saddled) { static const int SAD = mobPkItem("item/saddle"); if (SAD) spawnDrop(e.x, e.y + .5, e.z, 0, .1, 0, SAD, 1, 10); }   // patchHorse.1: a sela cai
    if (e.baby) return;   // patchBaby.1: filhote nao solta nada   // tabelas do 1.8.9: porco 1-3 costeleta; galinha 0-2 penas + 1 frango; vaca 0-2 couro + 1-3 carne; ovelha 1 lã + 1-2 carneiro
    const bool burning = e.fire > 0;   // patchFix.4 (BUG-018): morreu queimando (fogo/lava) = carne ja assada (EntityLivingBase.dropFewItems: isBurning)
    auto D = [&](int id, int n) { if (!id) return; if (burning) { if (id == IT_PORK) id = IT_CPORK; else if (id == IT_BEEF) id = IT_STEAK; else if (id == IT_CHICKEN) id = IT_CCHICKEN; else if (id == IT_MUTTON) id = IT_CMUTTON; } for (int i = 0; i < n; i++) spawnDrop(e.x + (frand() - .5) * .4, e.y + mobH(e) * .5, e.z + (frand() - .5) * .4, frand() * .2 - .1, .2, frand() * .2 - .1, id, 1, 10); };
    auto R = [&](int lo, int hi) { return lo + rand() % (hi - lo + 1) + (hi > lo && gLootLv > 0 ? rand() % (gLootLv + 1) : 0); };
    switch (e.type) {
        case M_PIG: D(IT_PORK, R(1, 3)); break;
        case M_CHICKEN: D(IT_FEATHER, R(0, 2)); D(IT_CHICKEN, 1); break;
        case M_COW: case M_MOOSH: D(IT_LEATHER, R(0, 2)); D(IT_BEEF, R(1, 3)); break;
        case M_RABBIT: case M_RWHITE: case M_RBLACK: case M_RGOLD: case M_RSALT: case M_RSPLOT: case M_RKILLER: case M_RTOAST: { auto P2 = [](const char* a, const char* b) { const int x = mobPkItem(a); return x ? x : mobPkItem(b); };   // patchRabbit.1: 0-1 couro, 0-1 coelho cru (assado se morreu queimando), 10% pe de coelho
            D(P2("item/rabbit_hide", "item/rabbit_hide"), R(0, 1)); if (rand() % 2) D(burning ? P2("item/cooked_rabbit", "item/rabbit_cooked") : P2("item/rabbit", "item/rabbit_raw"), 1); if (rand() % 10 == 0) D(P2("item/rabbit_foot", "item/rabbit_foot"), 1); break; }
        case M_POLAR: D(mobPkItem("item/fish_cod_raw"), R(0, 2)); if (rand() % 4 == 0) D(mobPkItem("item/fish_salmon_raw"), 1); break;   // patchMobNew.1: 0-2 bacalhau cru, 25% salmao cru
        case M_SHEEP: if (!e.sheared) D(WOOL, 1); D(IT_MUTTON, R(1, 2)); break;
        case M_ZOMBIE: D(mobPkItem("item/rotten_flesh"), R(0, 2)); break;
        case M_SKELETON: D(IT_BONE, R(0, 2)); D(IT_ARROW, R(0, 2)); break;
        case M_CREEPER: D(mobPkItem("item/gunpowder"), R(0, 2)); break;
        case M_BLAZE: D(mobPkItem("item/blaze_rod"), R(0, 1)); break;
        case M_ENDERMAN: D(mobPkItem("item/ender_pearl"), R(0, 1)); break;   // patchEnd.12
        case M_SLIME: if (e.ssz == 1) { const int sb = mobPkItem("item/slime_ball") ? mobPkItem("item/slime_ball") : mobPkItem("item/slimeball"); D(sb, R(0, 2)); } break;   // patchSlime.2: so o pequeno solta bola de slime
        case M_SPIDER: case M_CSPIDER: D(IT_STRING, R(0, 2)); break;
        case M_HUSK: case M_ZVILL: D(mobPkItem("item/rotten_flesh"), R(0, 2)); break;
        case M_MAGMA: if (e.ssz > 1 && rand() % 4 == 0) D(mobPkItem("item/magma_cream"), 1); break;
        case M_IGOLEM: D(mobPkItem("item/iron_ingot"), R(3, 5)); break;
        case M_GUARD: case M_ELDER: D(mobPkItem("item/prismarine_shard"), R(0, 2)); if (rand() % 3 == 0) D(mobPkItem("item/prismarine_crystals"), 1); break;
        case M_SNOWGOLEM: D(mobPkItem("item/snowball"), R(0, 15)); break;
        case M_PARROT: D(IT_FEATHER, R(1, 2)); break;
        case M_HORSE: case M_HBLACK: case M_HWHITE: case M_HGRAY: case M_DONKEY: case M_MULE: case M_LLAMA: case M_LLAMAB: case M_LLAMAG: case M_LLAMAW: D(IT_LEATHER, R(0, 2)); break;   // patchMobNew.2: 0-2 couro
        case M_ZHORSE: D(mobPkItem("item/rotten_flesh"), R(0, 2)); break;   // patchMobNew.2
        case M_SHORSE: D(IT_BONE, R(0, 2)); break;   // patchMobNew.2
        case M_WITHER: { static const int ns = mobPkItem("item/nether_star"); if (ns) D(ns, 1); break; }   // patchMobNew.6
        case M_GHAST: D(mobPkItem("item/ghast_tear"), R(0, 1)); D(mobPkItem("item/gunpowder"), R(0, 2)); break;
        case M_SHULKER: if (rand() % 2) D(mobPkItem("item/shulker_shell"), 1); break;
        case M_SQUID: D(mobPkItem("item/dye_powder_black"), R(1, 3)); break;
        case M_WITCH: D(mobPkItem("item/glowstone_dust"), R(0, 2)); D(mobPkItem("item/redstone"), R(0, 2)); D(mobPkItem("item/sugar"), R(0, 2)); D(IT_STICK, R(0, 2)); break;
        case M_VINDIC: case M_EVOKER: case M_ILLUS: if (rand() % 3 == 0) D(mobPkItem("item/emerald"), 1); break;
        case M_STRAY: D(IT_BONE, R(0, 2)); D(IT_ARROW, R(0, 2)); break;
        case M_WSKEL: D(IT_BONE, R(0, 2)); D(mobPkItem("item/coal"), R(0, 1)); break;
        case M_PIGMAN: { static const int rf = mobPkItem("item/rotten_flesh"), gn = mobPkItem("item/gold_nugget"); if (rf) D(rf, R(0, 1)); if (gn) D(gn, R(0, 1)); break; }
    }
}
// EntityLivingBase.attackEntityFrom (1.8.9): hurtResistantTime = 20 ticks após levar dano; na 1ª metade (resist > 10) só passa a diferença
// se o novo golpe for MAIOR que o último (lastDamage); na 2ª metade o golpe é integral. Golpe bloqueado = sem dano e sem knockback.
// Depois: knockBack(0.4 + 0.5 por nível extra, ex.: correndo), flash vermelho de 10 ticks e Panic de 100 ticks.
static bool mobSyncGuestHit(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra, float lx, float lz);   // mobsync.inc: convidado manda o golpe ao host
static bool mobHurt(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra = 0.f, float lx = 0.f, float lz = 0.f) {   // kx,kz = atacante -> alvo (posicao); lx,lz = direcao do olhar (parte do sprint)
    if (e.dead || dmg <= 0) return false;
    if (mobSyncGuestHit(e, dmg, kx, kz, attacked, kbExtra, lx, lz)) return true;   // convidado em rede: quem decide o dano e o host
    if (e.resist > 10) { if (dmg <= e.lastDmg) return false; e.hp -= dmg - e.lastDmg; e.lastDmg = dmg; }
    else { e.hp -= dmg; e.lastDmg = dmg; e.resist = 20; }
    e.hurt = 10; sndMob(e.type, e.x, e.y + .8, e.z, e.hp <= 0 ? 2 : 1);   // som de dano / morte (audio.inc)
    if (gMobAtkUid && gMobAtkUid != mobUid(e) && (e.type == M_ZOMBIE || isArch(e.type) || e.type == M_ZVILL || e.type == M_WSKEL)) { e.revId = gMobAtkUid; e.revT = 400; }   // EntityAIHurtByTarget: revida em quem me feriu (20 s)
    if (attacked) {
        const float l = sqrtf(kx * kx + kz * kz);
        e.mx /= 2; e.mz /= 2; e.my = std::min(.4, e.my / 2 + .4);
        if (l > 1e-4f) { e.mx += kx / l * .4; e.mz += kz / l * .4; if (kbExtra > 0.f) { const float ll = sqrtf(lx * lx + lz * lz); const float ux = ll > 1e-4f ? lx / ll : kx / l, uz = ll > 1e-4f ? lz / ll : kz / l; e.mx += ux * kbExtra * .22; e.mz += uz * kbExtra * .22; e.my = std::min(e.my + .05, .42); } /* patchMob.1: correndo, empurrao extra < metade do vanilla (.5), vertical <= .42 (antes os bichos voavam) */ }
        if (MD[e.type].ai == 3) { e.panic = 600; for (MobEnt& o : MOBS) if (&o != &e && !o.dead && o.type == e.type && fabs(o.x - e.x) < 32 && fabs(o.z - e.z) < 32) o.panic = std::max(o.panic, 600); }
        else if (MD[e.type].ai < 3) { e.panic = 100; e.hasT = false; }   // hostis novos (ai >= 4) nao fogem
        if (MD[e.type].ai == 3) e.hasT = false;
    }
    if (e.hp <= 0 && isSlimy(e.type) && e.ssz > 1 && !e.dead) { const int n = 2 + rand() % 3; for (int j = 0; j < n; j++) gSlimeQ.push_back({e.x + (j % 2 - .5) * e.ssz / 4.0, e.y + .5, e.z + (j / 2 - .5) * e.ssz / 4.0, e.ssz / 2, e.type}); }   // patchSlime.2: divide em 2-4 slimes menores
    if (e.hp <= 0) { e.dead = 1; e.hasT = false; e.rideBoat = 0; mobDrops(e); survAddXp(MD[e.type].ai >= 4 || MD[e.type].ai < 3 ? 5 : 1 + rand() % 3); }
    return true;
}
// crit = caindo no ar (fallDistance > 0, fora d'água): dano x1.5; sprint = knockback extra. Sem instakill: o criativo causa o dano normal da arma.
static int mobPickType(const float* eye, const float* dir, float reach) {   // pick block em entidade: tipo do mob mirado (alcance reach, sem bloco na frente) ou -1
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return -1;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return -1;
    return MOBS[best].type;
}
static void mobHitFx(double x, double y, double z, bool crit);   // egg.inc (BUG-057)
static int enchMeleeBonus(int held, int mobType) {   // dano extra da arma: afiacao 0,5*n+0,5; julgamento (morto-vivo) 2,5*n; ruina (artropode) 2,5*n
    int b = 0; const int sh = enchHeldLv(held, 16), sm = enchHeldLv(held, 17), ba = enchHeldLv(held, 18);
    if (sh) b += (int)lroundf(.5f * sh + .5f);
    const bool und = mobType == M_ZOMBIE || mobType == M_SKELETON || mobType == M_HUSK || mobType == M_STRAY || mobType == M_WSKEL || mobType == M_ZVILL || mobType == M_PIGMAN;
    const bool art = mobType == M_SPIDER || mobType == M_CSPIDER || mobType == M_SILVER || mobType == M_ENDERMITE;
    if (sm && und) b += (int)lroundf(2.5f * sm); if (ba && art) b += (int)lroundf(2.5f * ba);
    return b;
}
static bool mobAttack(const float* eye, const float* dir, int held, bool crit = false, bool sprint = false, float reach = 3.f) {   // clique esquerdo: acerta o mob mais próximo na mira (alcance 3), se não houver bloco na frente
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};   // caixa + 0.1 em todos os lados
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;   // bloco na frente do mob
    int dmg = fxMelee(mobWeaponDmg(held)) + enchMeleeBonus(held, MOBS[best].type); if (crit) dmg = (int)(dmg * 1.5f);   // patchEnch.3: afiacao/julgamento/ruina   // 1.8.9: i *= 1.5 em float, depois o dano vira inteiro de vida (meio coração = 1)
    { const MobEnt& t = MOBS[best]; float px = (float)t.x - eye[0], pz = (float)t.z - eye[2]; if (px * px + pz * pz < 1e-6f) { px = dir[0]; pz = dir[2]; }   // 1.8.9: o recuo base vai do atacante para o alvo
      gLootLv = enchHeldLv(held, 21); mobHurt(MOBS[best], dmg, px, pz, true, (sprint ? 1.f : 0.f) + (float)enchHeldLv(held, 19), dir[0], dir[2]); if (const int fa = enchHeldLv(held, 20)) MOBS[best].fire = std::max(MOBS[best].fire, 80 * fa); gLootLv = 0; mobHitFx(t.x, t.y + mobH(t) * .6, t.z, crit); }   // BUG-057: particulas de golpe
    return true;
}

// ------------------------------- física e IA (20 ticks/s) -------------------------------
static float mwrap(float a) { while (a > 180.f) a -= 360.f; while (a < -180.f) a += 360.f; return a; }
static float mturn(float cur, float want, float step) { return cur + std::max(-step, std::min(step, mwrap(want - cur))); }
static void moveMob(MobEnt& e, double mx, double my, double mz) {   // Entity.moveEntity (Y, X, Z + degrau de 0.6), com a caixa do mob
    const double hw = mobW(e) * .5;
    AABB bb{e.x - hw, e.y, e.z - hw, e.x + hw, e.y + mobH(e), e.z + hw};
    double x = mx, y = my, z = mz; const double d6 = x, d7 = y, d8 = z;
    std::vector<AABB> list; collBoxes(bb.addCoord(x, y, z), list); boatCollect(bb.addCoord(x, y, z), list, 0);
    const AABB bb0 = bb;
    for (auto& b : list) y = b.calcY(bb, y);
    bb = bb.offset(0, y, 0);
    const bool canStep = e.ground || (d7 != y && d7 < 0);
    for (auto& b : list) x = b.calcX(bb, x);
    bb = bb.offset(x, 0, 0);
    for (auto& b : list) z = b.calcZ(bb, z);
    bb = bb.offset(0, 0, z);
    if (canStep && (d6 != x || d8 != z)) {
        std::vector<AABB> l1; collBoxes(bb0.addCoord(d6, .6, d8), l1); boatCollect(bb0.addCoord(d6, .6, d8), l1, 0);
        AABB a = bb0; const AABB a5 = a.addCoord(d6, 0, d8);
        double dy = .6; for (auto& b : l1) dy = b.calcY(a5, dy);
        a = a.offset(0, dy, 0);
        double ax = d6; for (auto& b : l1) ax = b.calcX(a, ax);
        a = a.offset(ax, 0, 0);
        double az = d8; for (auto& b : l1) az = b.calcZ(a, az);
        a = a.offset(0, 0, az);
        if (ax * ax + az * az > x * x + z * z) {
            x = ax; z = az; y = -dy; bb = a;
            for (auto& b : l1) y = b.calcY(bb, y);
            bb = bb.offset(0, y, 0);
        }
    }
    e.x = (bb.x0 + bb.x1) / 2; e.y = bb.y0; e.z = (bb.z0 + bb.z1) / 2;
    e.collH = d6 != x || d8 != z; e.ground = d7 != y && d7 < 0;
    if (d6 != x) e.mx = 0; if (d8 != z) e.mz = 0; if (d7 != y) e.my = 0;
    if (d7 < -.1 && e.ground && slimeUnder(e.x, e.y, e.z)) { e.my = -d7; e.ground = false; e.fall = 0.f; }   // patchSlime.3: bloco de slime (Block.onLanded): quica e anula a queda
}
static bool mobSafeStep(const MobEnt& e, float yawDeg) {   // ponto logo à frente: tem chão (até 4 blocos abaixo) e não é água (navegador de terra evita água e precipícios)
    const float r = yawDeg * .0174533f;
    const int x = (int)floor(e.x - sin(r) * .7), z = (int)floor(e.z - cos(r) * .7), by = (int)floor(e.y + .001);
    if (!e.inWater && (isWater(getB(x, by, z)) || isWater(getB(x, by - 1, z)))) return false;
    for (int k = 1; k <= 4; k++) if (solid(getB(x, by - k, z))) return true;
    return false;
}
static void pickTarget(MobEnt& e, int rxz, int ry) {   // RandomPositionGenerator.findRandomTarget: 10 sorteios, vence o maior peso (grama = 10, resto ~0.5 como EntityAnimal.getBlockPathWeight)
    float best = -1e9f;
    for (int i = 0; i < 10; i++) {
        const int x = (int)floor(e.x) + rand() % (2 * rxz + 1) - rxz, z = (int)floor(e.z) + rand() % (2 * rxz + 1) - rxz;
        int gy = (int)floor(e.y) + rand() % (2 * ry + 1) - ry;
        for (int k = 0; k < 8 && gy > 1 && !solid(getB(x, gy - 1, z)); k++) gy--;   // desce até o chão
        if (!solid(getB(x, gy - 1, z)) || solid(getB(x, gy, z)) || solid(getB(x, gy + 1, z)) || isWater(getB(x, gy, z))) continue;
        const float w = getB(x, gy - 1, z) == GRASS ? 10.f : .5f;
        if (w > best) { best = w; e.tx = x + .5; e.tz = z + .5; e.hasT = true; e.stuck = 0; }
    }
}
static void hostileAI(MobEnt& e, double ppx, double ppy, double ppz); static void hostileTick(MobEnt& e, Phys& P); static void endSpawn(Phys& P); static void hostileSpawn(Phys& P); static void animalSpawn(Phys& P); static void fireballsTick(Phys& P); static void blazeSpawnTick(Phys& P);   // hostile.inc
static inline bool mobFlies(int t) { return t == M_VEX || t == M_WITHER || t == M_PARROT || t == M_GHAST || t == M_BAT; }   // patchMobFix.1: voam (sem gravidade); papagaio pousa quando sem alvo
static inline bool mobSwims(int t) { return t == M_SQUID || t == M_GUARD || t == M_ELDER; }   // patchMobFix.1: aquaticos: nao andam em terra, nadam em 3D
static double mobGroundY(double x, double y, double z) { int by = (int)floor(y) - 1; for (int i = 0; i < 24 && by > 0; i++, by--) if (solid(getB((int)floor(x), by, (int)floor(z)))) return by + 1.0; return y - 24.0; }
static void tickMob(MobEnt& e, double ppx, double ppy, double ppz) {
    const MobDef& d = MD[e.type];
    e.px = e.x; e.py = e.y; e.pz = e.z; e.pyaw = e.yaw; e.phyaw = e.hyaw; e.phpit = e.hpit; e.plimbAmt = e.limbAmt; e.pwing = e.wing; e.pflap = e.flap;
    if (e.rideBoat) {   // sentado no bote: sem IA nem física, só anima (a posição vem de boatTickAll)
        if (e.hurt > 0) e.hurt--; if (e.resist > 0) e.resist--;
        e.limbAmt = 0.f; e.mx = e.my = e.mz = 0; e.fall = 0.f; e.panic = 0; e.hasT = false; e.ground = true; e.inWater = false; return;
    }
    if (e.hurt > 0) e.hurt--;
    if (e.resist > 0) e.resist--;
    if (e.jumpCd > 0) e.jumpCd--;
    { const double sm = isSlimy(e.type) ? e.ssz : 1.0, hw = d.w * sm * .5, y0 = e.y + .4, y1 = std::max(e.y + .41, e.y + d.h * sm - .4);   // handleWaterMovement: caixa encolhida
      e.inWater = anyLiquid(AABB{e.x - hw + .001, y0, e.z - hw + .001, e.x + hw - .001, y1, e.z + hw - .001});
      e.inLava = anyLava(AABB{e.x - hw + .001, y0, e.z - hw + .001, e.x + hw - .001, y1, e.z + hw - .001});   // patchPigLava.1: lava propria (arrasto 0.5), nao mais a fisica da agua
      if (e.inWater) e.wetT = 15; else if (e.wetT > 0) e.wetT--; }   // patchMob.9: molhado ate 0,75 s depois de sair (pulos na agua)
    const double pdx = ppx - e.x, pdz = ppz - e.z, pdy = ppy + 1.62 - (e.y + d.h * .85), pd2 = pdx * pdx + pdz * pdz;
    float fwd = 0.f; bool jump = false;
    if (!e.dead && gRideUid && e.uid == gRideUid) {   // patchHorse.1: montado: gira com o jogador, anda pelo teclado e salta com espaco
        if (e.saddled && (e.type != M_PIG || gRideFwd != 0.f)) e.yaw = gRideYaw; e.hasT = false; e.panic = 0; e.hyaw = 0.f; e.hpit = 0.f; e.talkT = 0; e.idleT = 0; e.lookT = 0;
        fwd = d.spd * gRideFwd * potMobMul(e); if (e.jumpCd == 0 && gRideJump && e.ground && !e.inWater && !e.inLava) { e.my = .55; e.jumpCd = 10; e.ground = false; }
        if (gRideJump && (e.inWater || e.inLava)) e.my += .04;
    }
    else if (!e.dead) {
        // --- Panic (prioridade 1) > Wander (6): EntityMoveHelper gira o corpo no máx. 30°/tick ---
        float mul = d.ai == 2 ? .6f : 1.f;   // EntityAIWander(0.6) do aldeao
        if (d.ai == 2) { e.panic = 0; if (e.talkT > 0) { e.talkT--; e.hasT = false; e.lookT = std::max(e.lookT, 2); } }   // sem panico; conversando: para e olha o jogador
        if (d.ai >= 4) { e.panic = 0; hostileAI(e, ppx, ppy, ppz); }   // zumbi/esqueleto/creeper: persegue, recua ou acende o pavio
        if ((e.type == M_CATBLK || e.type == M_CATRED || e.type == M_CATSIA) && pd2 > 16.0 && pd2 < 1600.0 && !gDead && e.panic <= 0) { e.tx = e.x + pdx; e.tz = e.z + pdz; e.hasT = true; }   // patchMobNew.5: gato domesticado segue o jogador (3-40 blocos)
        if (e.type == M_OCELOT && pd2 < 49.0 && !creative && !gDead && e.panic <= 0) { const double dd = sqrt(pd2) + 1e-3; e.tx = e.x - pdx / dd * 10.0; e.tz = e.z - pdz / dd * 10.0; e.hasT = true; e.panic = 40; }   // patchOcelot.1: EntityAIAvoidEntity (7 blocos): corre na direcao oposta ao jogador
        if (d.ai == 3 && creative) e.panic = 0;   // mobs hostis ignoram jogador no criativo
        if (d.ai == 3 && e.panic > 0) {   // porco-zumbi irritado: corre ate o jogador e bate (dano 5, a cada 1 s)
            e.panic--; mul = 1.6f; e.tx = ppx; e.tz = ppz; e.hasT = true;
            if (e.talkT > 0) e.talkT--; else if (pd2 < 2.6 && fabs(pdy) < 1.8 && !gDead) { e.talkT = 20; survDamage(e.type == M_WOLF ? 2 : e.type == M_POLAR ? 6 : 5, e.type == M_WOLF ? "Foi morto por Lobo" : e.type == M_POLAR ? "Foi morto por Urso polar" : "Foi morto por Porco-zumbi"); }
        }
        else if (e.panic > 0) { e.panic--; mul = d.panic; if (!e.hasT) pickTarget(e, 5, 4); }
        else if (!e.hasT && e.talkT <= 0 && e.fuse <= 0) {
            if (d.ai == 2) { if (rand() % 120 == 0) { if (e.hasHome && (e.x - e.homeX) * (e.x - e.homeX) + (e.z - e.homeZ) * (e.z - e.homeZ) > 576.0) { e.tx = e.homeX; e.tz = e.homeZ; e.hasT = true; } else pickTarget(e, 10, 7); } }   // MoveTowardsRestriction: nao se afasta > 24 blocos da aldeia
            else if (d.ai == 1) { if (rand() % 60 == 0) pickTarget(e, 6, 3); }   // galinha: passeios curtos e frequentes
            else if (rand() % 120 == 0) pickTarget(e, 10, 7);                // EntityAIWander
        }
        mul *= potMobMul(e);   // patchPot.3: Velocidade/Lentidao de pocao
        if (isSlimy(e.type)) {   // patchSlime.2: EntitySlime: so se move PULANDO (10-30 ticks entre pulos, /3 perseguindo); no chao nao anda
            if (e.chase && e.hasT) e.yaw = mturn(e.yaw, (float)(atan2(-(e.tx - e.x), -(e.tz - e.z)) * 57.29578), 20.f);
            if (!e.chase) e.hasT = false;
            if (e.inWater || e.inLava) { if (frand() < .8f) jump = true; }
            else if (e.ground && --e.sjump <= 0) {
                e.sjump = rand() % 20 + 10; if (e.chase) e.sjump /= 3;
                if (!e.chase) e.yaw = frand() * 360.f;   // passeia em direcao aleatoria a cada pulo
                const double v = std::min(.34, .2 + .035 * e.ssz), r = e.yaw * .017453292519943295;
                e.my = .42; e.mx = -sin(r) * v; e.mz = -cos(r) * v; e.ground = false; e.jumpCd = 10;
            }
        } else if (e.hasT) {
            const double dx = e.tx - e.x, dz = e.tz - e.z, sp = mul * d.spd, tol = std::max(.3, sp * sp * 2.2);   // tol = distância andada por tick (evita passar direto)
            if (dx * dx + dz * dz < tol * tol) e.hasT = false;
            else {
                e.yaw = mturn(e.yaw, (float)(atan2(-dx, -dz) * 57.29578), 30.f);
                if (!mobSafeStep(e, e.yaw)) e.hasT = false;
                else { fwd = (float)sp; if (e.collH && e.ground) jump = true; }   // bateu em degrau/bloco: pula (JumpHelper)
            }
        }
        if (e.baby && ++e.babyAge >= 24000) e.baby = false;   // patchBaby.1: cresce
        if (e.type == M_SHEEP && e.sheared && e.ground && rand() % 1000 == 0 && getB((int)floor(e.x), (int)floor(e.y) - 1, (int)floor(e.z)) == GRASS) e.sheared = false;   // patchShear.1: come grama e a la volta (EntityAIEatGrass simplificado)
        // --- cabeça: bicada (galinha) > WatchClosest (2%/tick, 6 blocos, 40-79 ticks) > LookIdle (2%/tick, 20-39 ticks) ---
        float ty = 0.f, tp = 0.f;
        if (e.lookT > 0 && pd2 > 36.0) e.lookT = 0;
        if (e.peckT > 0) { e.peckT--; tp = (e.peckT % 10 < 5) ? 65.f : 15.f; }
        else if (e.lookT > 0) { e.lookT--; ty = mwrap((float)(atan2(-pdx, -pdz) * 57.29578) - e.yaw); tp = (float)(atan2(-pdy, sqrt(pd2)) * 57.29578); }
        else if (e.idleT > 0) { e.idleT--; ty = mwrap(e.idleYaw - e.yaw); }
        else if (pd2 <= 36.0 && frand() < .02f) e.lookT = 40 + rand() % 40;
        else if (frand() < .02f) { e.idleT = 20 + rand() % 20; e.idleYaw = frand() * 360.f; }
        else if (d.ai == 1 && !e.hasT && rand() % 50 == 0) e.peckT = 20;
        ty = std::max(-75.f, std::min(75.f, ty)); tp = std::max(-40.f, std::min(70.f, tp));
        e.hyaw += std::max(-10.f, std::min(10.f, ty - e.hyaw)); e.hpit += std::max(-40.f, std::min(40.f, tp - e.hpit));
        if (d.ai == 1 && !e.baby && --e.eggT <= 0) { e.eggT = 6000 + rand() % 6000; spawnDrop(e.x, e.y + .3, e.z, 0, .1, 0, IT_EGG, 1, 10); sndPlayAt("mob/chicken/plop", e.x, e.y, e.z, 1.f, sndRandPitch(.1f)); }   // EntityChicken: ovo a cada 5-10 min
        if (mobSwims(e.type)) { if (!e.inWater) { fwd = 0.f; jump = false; e.hasT = false; } else if (e.inWater && rand() % 70 == 0) e.my += (frand() - .5f) * .16; }   // patchMobFix.1: lula/guardiao em terra ficam parados; na agua derivam em 3D
        else if (e.inWater && frand() < .8f) jump = true;   // EntityAISwimming
    }
    if (e.inLava && frand() < .8f) jump = true;   // EntityAISwimming tambem vale na lava: boia em vez de afundar
    if (jump) { if (e.inWater || e.inLava) e.my += .04; else if (e.ground && e.jumpCd == 0) { e.my = .42; e.jumpCd = 10; } }
    // moveEntityWithHeading: aceleração = forward * friction; no chão friction = aiMoveSpeed (= forward), no ar/água 0.02
    if (fabs(e.mx) < .005) e.mx = 0; if (fabs(e.my) < .005) e.my = 0; if (fabs(e.mz) < .005) e.mz = 0;   // patchMob.1: EntityLivingBase.onLivingUpdate zera movimento residual (evita deslizar/tremer)
    if (isRabbit(e.type) && fwd > 0.f && e.ground && !e.inWater && !e.inLava) {   // patchRabbit.1: coelho anda aos pulos (EntityJumpHelper): no chao nao desliza; a cada 10-20 ticks salta (.32 para cima) na direcao do alvo
        if (e.jumpCd == 0) { const double v = std::min(.26, fwd * .5), ry = e.yaw * .017453292519943295; e.my = .32; e.mx = -sin(ry) * v; e.mz = -cos(ry) * v; e.ground = false; e.jumpCd = 10 + rand() % 10; }
        fwd = 0.f;
    }
    const double r = e.yaw * .017453292519943295, ac = mobFlies(e.type) ? fwd * .06 : (mobSwims(e.type) && e.inWater) ? fwd * .08 : (e.ground && !e.inWater && !e.inLava) ? fwd * fwd : fwd * .02;   // patchMobFix.1: voadores/aquaticos aceleram no fluido
    e.mx += -sin(r) * ac; e.mz += -cos(r) * ac;
    moveMob(e, e.mx, e.my, e.mz);
    if (isSlimy(e.type)) { e.psqF = e.sqF; if (e.ground && !e.wasG) e.sqA = -.5f; else if (!e.ground && e.wasG) e.sqA = 1.f; e.wasG = e.ground; e.sqA *= .6f; e.sqF += (e.sqA - e.sqF) * .5f; }   // squishAmount/Factor (EntitySlime.onUpdate)
    if (e.type == M_BLAZE || e.type == M_VEX || e.type == M_WITHER) {   // voa: sem gravidade; paira ~2 blocos acima do alvo (vex: na altura do alvo) e amortece o movimento
        const double wantY = (e.chase || pd2 < 400.0) && !creative ? ppy + (e.type == M_VEX ? .3 : e.type == M_WITHER ? 3.0 : 2.2) : e.y; e.my += std::max(-.5, std::min(.5, wantY - e.y)) * .06; e.my *= .6; e.mx *= .91; e.mz *= .91; e.fall = 0.f;
    } else if (mobFlies(e.type)) {   // patchMobFix.1: papagaio/ghast: sem gravidade; com alvo voa 3 blocos acima do chao, sem alvo pousa (papagaio) ou paira (ghast)
        const double gy = mobGroundY(e.x, e.y, e.z), wantY = e.type == M_GHAST ? std::max(e.y, gy + 2.0) : e.type == M_BAT ? gy + 2.5 + 1.5 * sin((double)SDL_GetTicks() * .0015 + (double)(&e - &MOBS[0])) : (e.hasT ? gy + 3.0 : gy);   // morcego: paira oscilando entre 1 e 4 blocos do chao
        if (e.type == M_PARROT && !e.hasT && e.ground) { e.my -= .08; e.my *= .98; e.mx *= .6; e.mz *= .6; }
        else { e.my += std::max(-.5, std::min(.5, wantY - e.y)) * .06; e.my *= .6; e.mx *= .91; e.mz *= .91; }
        e.fall = 0.f; if (e.collH) e.my += .1;
    } else if (mobSwims(e.type) && e.inWater) { e.mx *= .9; e.my *= .9; e.mz *= .9; if (!anyLiquid(AABB{e.x - .1, e.y + MD[e.type].h + .1, e.z - .1, e.x + .1, e.y + MD[e.type].h + .2, e.z + .1})) e.my -= .01; else e.my += .005; e.fall = 0.f;   // patchMobFix.1: nada, sem afundar nem boiar ate a superficie
    } else if (e.inLava) { e.mx *= .5; e.my *= .5; e.mz *= .5; e.my -= .02; if (e.collH) e.my = .3; e.fall = 0.f;   // Entity.handleLavaMovement: arrasto 0,5
    } else if (e.inWater) { e.mx *= .8; e.my *= .8; e.mz *= .8; e.my -= .02; if (e.collH) e.my = .3; }
    else { const double f4 = e.ground ? groundSlipAt(e.x, e.y, e.z) * .91 : .91; e.my -= .08; e.my *= .98; e.mx *= f4; e.mz *= f4; }
    // dano de queda (EntityLivingBase.fall: > 3 blocos; galinha é imune)
    { const double dy = e.y - e.py;
      if (e.inWater || e.inLava) e.fall = 0.f;
      else if (e.ground) { if (e.fall > 3.f && d.ai != 1 && !slimeUnder(e.x, e.y, e.z)) mobHurt(e, (int)ceil(e.fall - 3.f), 0, 0, false); e.fall = 0.f; }
      else if (dy < 0) e.fall -= (float)dy; }
    if (d.ai == 1) {   // EntityChicken.onLivingUpdate: asas batem no ar e a queda é amortecida (motionY *= 0.6)
        e.flap = std::max(0.f, std::min(1.f, e.flap + (e.ground ? -.3f : 1.2f)));
        if (!e.ground && e.flapD < 1.f) e.flapD = 1.f;
        e.flapD *= .9f; e.wing += e.flapD * 2.f;
        if (!e.ground && e.my < 0) e.my *= .6;
    }
    const double mdx = e.x - e.px, mdz = e.z - e.pz;
    if (!e.dead && fwd <= 0.f && e.type != M_BLAZE && e.type != M_SHULKER && mdx * mdx + mdz * mdz > .0025 * .0025) {   // patchMob.2: EntityLivingBase.updateDistance — empurrado/arrastado, o CORPO gira p/ a direcao do movimento (30%/tick) e a cabeca mantem o olhar
        const float want = (float)(atan2(-mdx, -mdz) * 57.29578), dy = mwrap(want - e.yaw) * .3f;
        e.yaw += dy; e.hyaw = std::max(-75.f, std::min(75.f, e.hyaw - dy));
    }
    float f6 = (float)sqrt(mdx * mdx + mdz * mdz) * 4.f; if (f6 > 1.f) f6 = 1.f;
    e.limbAmt += (f6 - e.limbAmt) * .4f; e.limb += e.limbAmt;
    if (e.hasT && fwd > 0.f) { if (mdx * mdx + mdz * mdz < 1e-4) { if (++e.stuck > 20) { e.hasT = false; e.stuck = 0; } } else e.stuck = 0; }   // preso: desiste do alvo
    else e.stuck = 0;
}
static void spawnPoof(const MobEnt& e) {   // EntityLivingBase.onDeathUpdate (deathTime 20): 20 partículas "explode" espalhadas pelo corpo
    const float w = mobW(e), h = mobH(e);
    auto G = []() { return (frand() + frand() + frand() + frand() - 2.f) * .0346f; };   // ~ gaussiana * 0.02
    for (int i = 0; i < 20; i++) {
        Puff p; p.x = p.px = (float)e.x + (frand() * 2.f - 1.f) * w; p.y = p.py = (float)e.y + frand() * h; p.z = p.pz = (float)e.z + (frand() * 2.f - 1.f) * w;
        p.vx = G() + (frand() * 2.f - 1.f) * .05f; p.vy = G() + (frand() * 2.f - 1.f) * .05f; p.vz = G() + (frand() * 2.f - 1.f) * .05f;
        p.s = frand() * frand() * 6.f + 1.f; p.c = frand() * .3f + .7f; p.max = (int)(16.f / (frand() * .8f + .2f)) + 2; p.age = 0;
        PUFFS.push_back(p);
    }
}
// Entity.applyEntityCollision (1.8.9): afasta os dois pela diferença de posição; força = 0.05 / sqrt(max(|dx|,|dz|)) (no máx. 0.05), só em X/Z.
static void pushPair(double ax, double az, double& amx, double& amz, double bx, double bz, double& bmx, double& bmz) {
    double d0 = bx - ax, d1 = bz - az, d2 = std::max(fabs(d0), fabs(d1));
    if (d2 < .01) return;
    d2 = sqrt(d2); d0 /= d2; d1 /= d2;
    const double d3 = std::min(1.0, 1.0 / d2); d0 *= d3 * .05; d1 *= d3 * .05;
    amx -= d0; amz -= d1; bmx += d0; bmz += d1;
}
static AABB mobBox(const MobEnt& e) { const double hw = mobW(e) * .5; return {e.x - hw, e.y, e.z - hw, e.x + hw, e.y + mobH(e), e.z + hw}; }
static void mobPush(Phys& P) {   /* patchMob.7: soma das forcas de varios mobs empilhados e limitada por tick (canto dos mobs nao arremessa) */
    static std::vector<std::pair<double, double>> v0; v0.resize(MOBS.size()); for (size_t i = 0; i < MOBS.size(); i++) v0[i] = {MOBS[i].mx, MOBS[i].mz}; const double pmx0 = P.mx, pmz0 = P.mz;
    struct Clamp { static void lim(double& vx, double& vz, double x0, double z0) { const double dx = vx - x0, dz = vz - z0, l = sqrt(dx * dx + dz * dz); if (l > .15) { vx = x0 + dx / l * .15; vz = z0 + dz / l * .15; } } };
    // cada entidade empurra as vizinhas (caixa + 0.2 em X/Z); como no Java, cada par é processado 2x por tick (uma por entidade)
    for (size_t i = 0; i < MOBS.size(); i++) {
        MobEnt& a = MOBS[i]; if (a.dead || a.rideBoat) continue;
        if (a.y < -64.0) { a.dead = true; continue; }   // caiu no vazio
        const AABB ba = mobBox(a), ex = {ba.x0 - .2, ba.y0, ba.z0 - .2, ba.x1 + .2, ba.y1, ba.z1 + .2};
        for (size_t j = 0; j < MOBS.size(); j++) {
            if (j == i || MOBS[j].dead || MOBS[j].rideBoat || !ex.intersects(mobBox(MOBS[j]))) continue;
            pushPair(a.x, a.z, a.mx, a.mz, MOBS[j].x, MOBS[j].z, MOBS[j].mx, MOBS[j].mz);
        }
        if (ex.intersects(physBox(P))) pushPair(a.x, a.z, a.mx, a.mz, P.x, P.z, P.mx, P.mz);   // mob -> jogador
    }
    const AABB pb = physBox(P), ex = {pb.x0 - .2, pb.y0, pb.z0 - .2, pb.x1 + .2, pb.y1, pb.z1 + .2};
    for (MobEnt& m : MOBS) if (!m.dead && !m.rideBoat && !(gRideUid && m.uid == gRideUid) && ex.intersects(mobBox(m))) pushPair(P.x, P.z, P.mx, P.mz, m.x, m.z, m.mx, m.mz);   // jogador -> mob
    Clamp::lim(P.mx, P.mz, pmx0, pmz0); for (size_t i = 0; i < MOBS.size() && i < v0.size(); i++) Clamp::lim(MOBS[i].mx, MOBS[i].mz, v0[i].first, v0[i].second);
}
static bool mobTouchesFire(const AABB& r) {
    for (int y = (int)floor(r.y0); y <= (int)floor(r.y1); y++) for (int z = (int)floor(r.z0); z <= (int)floor(r.z1); z++) for (int x = (int)floor(r.x0); x <= (int)floor(r.x1); x++) if (getB(x, y, z) == XB_FIRE) return true;
    return false;
}
static void mobAirTick(MobEnt& e) {   // patch5.78: afogamento de mob (1.8.9): cabeca na agua gasta 1 de ar por tick; a -20 leva 2 de dano e o ar volta a 0; fora da agua enche na hora
    if (e.dead) return;
    const MobDef& d = MD[e.type];
    if (isWater(getB((int)floor(e.x), (int)floor(e.y + d.h * .85), (int)floor(e.z)))) { if (--e.air <= -20) { e.air = 0; mobHurt(e, 2, 0, 0, false); } } else e.air = 300;
}
static void mobFireTick(MobEnt& e) {
    mobAirTick(e);   // fogo/lava incendeiam todo mob (menos porco-zumbi); agua apaga; queimando = 1 de dano por segundo (como o Minecraft)
    if (e.dead || e.type == M_PIGMAN || e.type == M_BLAZE || e.type == M_MAGMA || e.type == M_WSKEL) { e.fire = 0; return; }   // porco-zumbi e blaze: imunes a fogo
    const MobDef& d = MD[e.type]; const double hw = d.w * .5;
    const AABB bb{e.x - hw + .001, e.y + .001, e.z - hw + .001, e.x + hw - .001, e.y + d.h - .001, e.z + hw - .001};
    const int before = e.fire;
    if (anyLava(bb)) { mobHurt(e, 4, 0, 0, false); e.fire = std::max(e.fire, 300); }   // lava: 4 de dano (respeita o tempo de invulnerabilidade) e 15 s de fogo
    else if (mobTouchesFire(bb)) { mobHurt(e, 1, 0, 0, false); e.fire = std::max(e.fire, 160); }   // bloco de fogo: 8 s
    if (e.inWater) e.fire = 0;
    if (e.fire > 0 && before == 0 && MD[e.type].ai < 3) { e.panic = std::max(e.panic, 100); e.hasT = false; }   // animais fogem queimando
    if (e.fire > 0) { e.fire--; if (++e.burn >= 20) { e.burn = 0; if (!e.dead) mobHurt(e, 1, 0, 0, false); } } else e.burn = 0;   // 1 de dano por segundo; contador proprio (o sol renova fire a cada tick e fire % 20 nunca fechava: fogo na tela mas sem dano)
}
static bool gSplit = false;   // split screen ligado (splitscreen.inc / gamepad.inc)
static int gSplitN = 2;   // patchSplit.1: total de jogadores locais no split (2..4); extras = gSX[0..gSplitN-2]
static int gS2i = 0;      // indice do jogador extra "atual" (SX); trocar sempre com SxSel (RAII)
static bool split2MobTick(MobEnt& e, Phys& P1);   // splitscreen.inc
static bool mobSyncGuestTick(); static bool mobRemoteTick(MobEnt& e, Phys& P); static void mobSyncHostTick(Phys& P);   // mobsync.inc
static void propFxTick(const Phys& P);   // propsys.inc
// patchBreed.1: reproducao. Animal adulto alimentado (love=600 ticks) anda ate outro da mesma especie tambem em modo amor (ate 8 blocos); a <1,6 blocos nasce um filhote
// no meio, ambos ganham recarga de 5 min e o jogador ganha 1-7 de XP (como no 1.12). So host/offline.
static void spawnHearts(double x, double y, double z, int n) {   // patchHeart.1: n coracoes em volta do animal (EntityAgeable/EntityAnimal.handleStatusUpdate 18/6)
    for (int k = 0; k < n; k++) { Puff p; p.kind = 3; p.x = p.px = (float)x + (frand() - .5f) * .8f; p.y = p.py = (float)y + .7f + frand() * .5f; p.z = p.pz = (float)z + (frand() - .5f) * .8f;
        p.vx = (frand() - .5f) * .02f; p.vy = .03f + frand() * .02f; p.vz = (frand() - .5f) * .02f; p.s = 5.f; p.c = 1.f; p.max = 16 + rand() % 8; p.age = 0; PUFFS.push_back(p); }
}
static void breedTick() {
    const int n = (int)MOBS.size();
    for (int i = 0; i < n; i++) { MobEnt& e = MOBS[i]; if (e.dead) continue; if (e.love > 0) { e.love--; if (rand() % 12 == 0) spawnHearts(e.x, e.y + (e.baby ? -.3 : 0.), e.z, 1); } if (e.breedCd > 0) e.breedCd--; }
    static int fol = 0;
    if (++fol >= 10) { fol = 0;   // patchBreed.2: filhote parado segue o adulto mais proximo da mesma especie (3 a 10 blocos)
        for (int i = 0; i < n; i++) { MobEnt& b = MOBS[i]; if (b.dead || !b.baby || b.panic > 0) continue;   // patchBaby.2: segue mesmo andando (antes so parado)
            int best = -1; double bd = 324.0; for (int j = 0; j < n; j++) { const MobEnt& a = MOBS[j]; if (a.dead || a.baby || !mobSameKind(a.type, b.type)) continue; const double dx = a.x - b.x, dz = a.z - b.z, d2 = dx * dx + dz * dz; if (fabs(a.y - b.y) < 6.0 && d2 < bd) { bd = d2; best = j; } }
            if (best >= 0 && bd > 9.0) { b.tx = MOBS[best].x; b.tz = MOBS[best].z; b.hasT = true; } } }
    for (int i = 0; i < n; i++) {
        MobEnt& a = MOBS[i]; if (a.dead || a.baby || a.love <= 0) continue;
        int best = -1; double bd = 64.0;
        for (int j = 0; j < n; j++) { if (j == i) continue; const MobEnt& b = MOBS[j]; if (b.dead || b.baby || b.love <= 0 || !mobSameKind(b.type, a.type)) continue;
            const double dx = b.x - a.x, dz = b.z - a.z, dy = b.y - a.y, d2 = dx * dx + dz * dz; if (fabs(dy) < 3.0 && d2 < bd) { bd = d2; best = j; } }
        if (best < 0) continue;
        MobEnt& b = MOBS[best];
        if (bd < 1.6 * 1.6) {
            const double mx = (a.x + b.x) * .5, my = std::max(a.y, b.y), mz = (a.z + b.z) * .5; const int ty = a.type;
            a.love = b.love = 0; a.breedCd = b.breedCd = 6000; a.hasT = b.hasT = false;
            spawnHearts(a.x, a.y, a.z, 7); spawnHearts(b.x, b.y, b.z, 7);   // patchHeart.1
            spawnMob(ty, mx, my, mz, a.yaw, true);   // push_back: referencias a/b ficam invalidas; nao usar depois
            survAddXp(1 + rand() % 7);
            return;   // no maximo 1 nascimento por tick (evita mexer em MOBS durante o laco)
        }
        a.tx = b.x; a.tz = b.z; a.hasT = true;
    }
}
static void tickMobs(Phys& P) {
    propFxTick(P);
    const bool guest = mobSyncGuestTick();   // convidado em rede: nao simula, so interpola o que o host manda
    if (!guest) { hostileSpawn(P); animalSpawn(P); blazeSpawnTick(P); endSpawn(P); }
    fireballsTick(P);
    const double ppx = P.x, ppy = P.y, ppz = P.z;
    if (!guest) { vilNaturalTick(P); if (!gVilQ.empty()) vilSpawnPending(); mobPush(P); }
    for (size_t i = 0; i < HBOOMS.size();) {   // EntityHugeExplodeFX.onUpdate: 6 "largeexplode" por tick em posicao aleatoria (+-4), escala 1 - 0,5*t/8
        HugeBoom& h = HBOOMS[i];
        for (int n = 0; n < 6; n++) {
            Puff p; p.kind = 2; p.x = p.px = h.x + (frand() - frand()) * 4.f; p.y = p.py = h.y + (frand() - frand()) * 4.f; p.z = p.pz = h.z + (frand() - frand()) * 4.f;
            p.vx = p.vy = p.vz = 0.f; p.s = (1.f - h.t * .5f / 8.f) * 14.f; p.c = .82f + frand() * .15f; p.max = 6 + rand() % 4; p.age = 0; PUFFS.push_back(p);
        }
        if (++h.t >= 8) HBOOMS.erase(HBOOMS.begin() + i); else i++;
    }
    for (size_t i = 0; i < PUFFS.size();) {
        Puff& p = PUFFS[i]; p.px = p.x; p.py = p.y; p.pz = p.z;
        if (p.age++ >= p.max) { PUFFS.erase(PUFFS.begin() + i); continue; }
        if (p.kind == 2) { p.x += p.vx; p.y += p.vy; p.z += p.vz; i++; continue; }
        if (p.kind == 3) { p.x += p.vx; p.y += p.vy; p.z += p.vz; i++; continue; }   // patchHeart.1
        if (p.kind == 1) { p.vy -= .03f; p.x += p.vx; p.y += p.vy; p.z += p.vz; if (solid(getB((int)floorf(p.x), (int)floorf(p.y), (int)floorf(p.z))) || isWater(getB((int)floorf(p.x), (int)floorf(p.y), (int)floorf(p.z)))) { PUFFS.erase(PUFFS.begin() + i); continue; } i++; continue; }   // patchProp.16: gota some ao tocar chao/agua (antes atravessava piso e banheira)
        p.vy += .004f; p.x += p.vx; p.y += p.vy; p.z += p.vz; p.vx *= .9f; p.vy *= .9f; p.vz *= .9f; i++;
    }
    if (!guest) slimeFlush();
    for (size_t i = 0; !guest && i < MOBS.size();) {
        MobEnt& e = MOBS[i];
        if (gSndOn && !e.dead) { const MobSnd* ms = mobSndFor(e.type); if (ms && ms->idle && rand() % 1000 < e.sayT++) { e.sayT = -80; sndMob(e.type, e.x, e.y + .8, e.z, 0); } }   // som ocioso (EntityLiving.getTalkInterval: 1/1000 que cresce por tick)
        if (e.y < -30) { MOBS.erase(MOBS.begin() + i); continue; }
        if (!getC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { i++; continue; }   // chunk não carregado: congela
        if (!simNearC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { i++; continue; }   // patchChunks.4: fora do raio de simulacao: congela
        if (gPerf && !netOn && !gSplit) { const double fx0 = e.x - ppx, fz0 = e.z - ppz, R0 = RAD * 16.0; if (fx0 * fx0 + fz0 * fz0 > R0 * R0) { i++; continue; } }   // patchRpiOpt.6: mob alem do raio de render: congela (so single player)
        if (mobRemoteTick(e, P)) mobFireTick(e);   // host: jogador remoto e o alvo mais proximo
        else if (gSplit && split2MobTick(e, P)) mobFireTick(e);   // jogador 2 e o alvo mais proximo: roda no contexto dele
        else { tickMob(e, ppx, ppy, ppz); hostileTick(e, P); mobFireTick(e); }
        if (e.dead && ++e.dead >= 21) { spawnPoof(e); MOBS.erase(MOBS.begin() + i); continue; }
        i++;
    }
    for (const auto& q : gVexQ) for (int k = 0; k < 3; k++) { spawnMob(M_VEX, q[0] + (rand() % 5 - 2) * .6, q[1] + .5, q[2] + (rand() % 5 - 2) * .6, frand() * 360.f); MOBS.back().life = 600 + rand() % 400; }   // 3 vexes, 30-50 s
    gVexQ.clear();
    if (!guest) breedTick();
    mobSyncHostTick(P);   // host: envia os mobs aos convidados
}
