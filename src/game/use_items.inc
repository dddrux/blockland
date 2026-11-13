// usar item no mundo: tesoura, comida de mobs (amor), balde (movido de main_loop.inc em patchOrg.4)
// patchShear.1: botao direito com tesoura numa ovelha com la (adulta, mais perto que o bloco): 1-3 la, gasta 1 de durabilidade, ovelha fica tosquiada
static void mobUseSend(int uid, int act, int item) {   // patchMP.3: convidado -> host
    if (uid <= 0) return; NBuf b = netMk(NP_MOBUSE); b.put<int32_t>(uid); b.put<uint8_t>((uint8_t)act); b.put<int32_t>(netWireId(item)); netBroadcast(b);
}
static bool shearUse(Slot& s, const float* eye, const float* dir) {
    if (!s.id || s.id != shearsId()) return false;
    const bool guestC = netConn && !netHost;   // patchMP.3: convidado manda NP_MOBUSE; o host valida e aplica (la cai pelo NP_DROP)
    int sh = -1; float bt = 2.8f;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || e.baby || e.type != M_SHEEP || e.sheared) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; sh = i; }
    }
    if (sh < 0) return false;
    const float tb = raySolidDistance(eye, dir, 2.8f); if (tb >= 0.f && tb < bt) return false;   // bloco na frente da ovelha
    MobEnt& e = MOBS[sh]; e.sheared = true;
    sndEvtAt(sndBlockEvt(WOOL, false), e.x, e.y + .5, e.z);
    if (guestC) { mobUseSend(e.uid, 2, s.id); itemWear(s, 1); return true; }
    const int n = 1 + rand() % 3;
    for (int i = 0; i < n; i++) spawnDrop(e.x + (frand() - .5) * .4, e.y + mobH(e) * .8, e.z + (frand() - .5) * .4, frand() * .1 - .05, .25, frand() * .1 - .05, WOOL, 1, 10);
    itemWear(s, 1);
    return true;
}
// patchBreed.1: botao direito com a comida certa num animal adulto (vaca/ovelha: trigo; porco: cenoura/batata/beterraba; galinha: sementes) liga o modo amor; consome 1 (menos no criativo)
static bool breedFood(int type, int id) {
    if (id <= 0) return false;
    if (type == M_COW || type == M_SHEEP || type == M_MOOSH) return id == cropItem("item/wheat");
    if (type == M_PIG) return id == cropItem("item/carrot") || id == cropItem("item/potato") || id == cropItem("item/beetroot");
    if (type == M_OCELOT) return id > 0 && (id == cropItem("item/fish_cod_raw") || id == cropItem("item/fish_salmon_raw"));   // patchMobNew.5: peixe cru domestica o ocelote (1/3) em gato
    if (type == M_CHICKEN) return id == cropItem("item/wheat_seeds") || id == cropItem("item/beetroot_seeds");
    if (mobCanBaby(type) && isRabbit(type)) { static const char* const RF[] = {"item/carrot", "item/golden_carrot", "item/carrot_golden", "block/dandelion", "block/flower_dandelion", "item/dandelion"}; for (const char* f : RF) { const int v = cropItem(f); if (v > 0 && v == id) return true; } return false; }   // patchBaby.3: coelho: cenoura, cenoura dourada, dente-de-leao
    return false;
}
static bool breedUse(Slot& s, const float* eye, const float* dir, bool creative) {
    if (!s.id) return false;
    int pk = -1; float bt = 2.8f;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || (!e.baby && (e.love > 0 || e.breedCd > 0)) || !breedFood(e.type, s.id)) continue;   // filhote tambem aceita (cresce mais rapido)
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; pk = i; }
    }
    if (pk < 0) return false;
    const float tb = raySolidDistance(eye, dir, 2.8f); if (tb >= 0.f && tb < bt) return false;
    if (netConn && !netHost) mobUseSend(MOBS[pk].uid, 1, s.id);   // patchMP.3: o host aplica (amor/crescimento); aqui so marca local para nao repetir
    if (MOBS[pk].baby) MOBS[pk].babyAge = std::min(24000, MOBS[pk].babyAge + 2400);   // filhote alimentado: ~10% do tempo de crescimento (como o 1.12)
    else if (MOBS[pk].type == M_OCELOT) { if (rand() % 3 == 0) { MOBS[pk].type = M_CATBLK + rand() % 3; MOBS[pk].hp = MD[MOBS[pk].type].hp; MOBS[pk].hasT = false; } }   // patchMobNew.5: domesticado
    else MOBS[pk].love = 600;
    if (!creative && --s.n <= 0) s = Slot();
    return true;
}
// patchBabyEgg.1: ovo de spawn clicado num mob da MESMA especie (adulto ou filhote) faz nascer um filhote na posicao dele (1.12.2: EntityAgeable.processInitialInteract); consome o ovo (menos no criativo). So especies com modelo de filhote (mobCanBaby)
static bool eggBabyUse(Slot& s, const float* eye, const float* dir, bool creative) {
    const int mt = eggMob(s.id); if (mt < 0 || !mobCanBaby(mt)) return false;
    const float reach = creative ? 6.f : 3.f; int pk = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || !mobSameKind(e.type, mt)) continue;
        const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; pk = i; }
    }
    if (pk < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;   // bloco na frente do mob
    const int pt = MOBS[pk].type; const double px = MOBS[pk].x, py = MOBS[pk].y, pz = MOBS[pk].z; const float pyaw = MOBS[pk].yaw;   // copia: spawnMob faz push_back e invalida a referencia
    if (netOn && !netHost) netSendMobSpawn(pt, px, py, pz, pyaw, true); else spawnMob(pt, px, py, pz, pyaw, true);
    if (!creative && --s.n <= 0) s = Slot();
    return true;
}
static void mobUseRecv(NBuf& b, int sender) {   // host: convidado alimentou (1) ou tosquiou (2) um mob
    const int uid = b.get<int32_t>(), act = b.get<uint8_t>(), item = netUnwireId(b.get<int32_t>()); if (item < 0) return;   // BUG-046c: item por nome
    auto it = remotes.find(sender); if (it == remotes.end() || !it->second.init || it->second.gm == 3) return;
    for (MobEnt& e : MOBS) {
        if (e.uid != uid || e.dead) continue;
        const double dx = e.x - it->second.x, dy = e.y - it->second.y, dz = e.z - it->second.z; if (dx * dx + dy * dy + dz * dz > 100.0) return;   // longe demais = pacote forjado
        if (act == 1 && e.type == M_OCELOT && breedFood(e.type, item)) { if (rand() % 3 == 0) { e.type = M_CATBLK + rand() % 3; e.hp = MD[e.type].hp; e.hasT = false; } }
        else if (act == 1 && breedFood(e.type, item)) { if (e.baby) e.babyAge = std::min(24000, e.babyAge + 2400); else if (e.love <= 0 && e.breedCd <= 0) e.love = 600; }
        else if (act == 2 && e.type == M_SHEEP && !e.baby && !e.sheared && item == shearsId()) {
            e.sheared = true; sndEvtAt(sndBlockEvt(WOOL, false), e.x, e.y + .5, e.z);
            const int n = 1 + rand() % 3; for (int i = 0; i < n; i++) spawnDrop(e.x + (frand() - .5) * .4, e.y + mobH(e) * .8, e.z + (frand() - .5) * .4, frand() * .1 - .05, .25, frand() * .1 - .05, WOOL, 1, 10);
        }
        return;
    }
}
// ---- balde (Item Bucket do 1.8): vazio pega fonte de agua/lava (raio atravessa liquidos); cheio coloca uma fonte na celula da face mirada ----
static bool bucketUse(Slot& s, const float* eye, const float* dir) {
    if (!gBktEmpty || (s.id != gBktEmpty && s.id != gBktWater && s.id != gBktLava && !(gBktMilk && s.id == gBktMilk))) return false;
    if (s.id == gBktMilk) { sndEvtUi("drink"); gPoisonT = 0; if (!creative) s.id = gBktEmpty; return true; }   // patchBucket.2: beber leite remove o veneno e devolve o balde vazio
    RayHit r;
    if (rayCast(eye, dir, gBlockReach, r, false) && getB(r.x, r.y, r.z) == XB_DECOR && (getM(r.x, r.y, r.z) & 3) == 0) {   // patchBucket.3: caldeirao (meta bits 2-3 = nivel 0..3)
        const int cm = getM(r.x, r.y, r.z), lv = (cm >> 2) & 3;
        if (s.id == gBktWater) {
            if (lv < 3) { setBM(r.x, r.y, r.z, XB_DECOR, (cm & 3) | (3 << 2)); netSendBlock(r.x, r.y, r.z, XB_DECOR, (cm & 3) | (3 << 2)); if (!creative) s.id = gBktEmpty; }
            return true;
        }
        if (s.id == gBktEmpty && lv == 3) {
            setBM(r.x, r.y, r.z, XB_DECOR, cm & 3); netSendBlock(r.x, r.y, r.z, XB_DECOR, cm & 3);
            if (!creative) {
                if (s.n <= 1) { s.id = gBktWater; s.n = 1; }
                else { --s.n; if (addItem(gBktWater, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, gBktWater, 1, 10); }
            }
            return true;
        }
    }
    if (s.id == gBktEmpty) {
        if (gBktMilk) {   // patchBucket.1: balde vazio na vaca (adulta) = balde de leite (vale se a vaca estiver mais perto que o bloco)
            int cow = -1; float bt = 2.8f;
            for (int i = 0; i < (int)MOBS.size(); i++) {
                const MobEnt& e = MOBS[i]; if (e.dead || e.baby || (e.type != M_COW && e.type != M_MOOSH)) continue;
                const float hw = mobW(e) * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + mobH(e) + .1f, (float)e.z + hw};
                float t0 = 0.f, t1 = bt; bool ok = true;
                for (int a = 0; a < 3 && ok; a++) {
                    if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
                    float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
                    t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
                }
                if (ok && t0 < bt) { bt = t0; cow = i; }
            }
            if (cow >= 0) {
                const float tb = raySolidDistance(eye, dir, 2.8f);
                if (tb < 0.f || tb >= bt) {
                    if (!creative) {
                        if (s.n <= 1) { s.id = gBktMilk; s.n = 1; }
                        else { --s.n; if (addItem(gBktMilk, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, gBktMilk, 1, 10); }
                    }
                    return true;
                }
            }
        }
        if (!rayCast(eye, dir, gBlockReach, r, true)) return true;
        const int b = getB(r.x, r.y, r.z);
        const int full = b == WATER ? gBktWater : b == LAVA ? gBktLava : 0;   // so FONTES (fluxo nao enche o balde)
        if (!full) return true;
        setB(r.x, r.y, r.z, AIR); netSendBlock(r.x, r.y, r.z, AIR);
        if (!creative) {   // no criativo o balde continua vazio
            if (s.n <= 1) { s.id = full; s.n = 1; }
            else { --s.n; if (addItem(full, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, full, 1, 10); }
        }
        return true;
    }
    if (!rayCast(eye, dir, gBlockReach, r, false)) return true;   // balde cheio mira so em blocos solidos e usa a celula da face
    const int tx = r.x + r.nx, ty = r.y + r.ny, tz = r.z + r.nz;
    if (ty < 0 || ty >= CH) return true;
    const int tb = getB(tx, ty, tz);
    if (solid(tb)) return true;
    if (s.id == gBktWater && gDim == 1) {   // patchBucket.1: Nether (ultra quente): a agua evapora, o balde esvazia e nenhuma fonte e criada
        if (!creative) s.id = gBktEmpty;
        return true;
    }
    setB(tx, ty, tz, s.id == gBktWater ? WATER : LAVA); netSendBlock(tx, ty, tz, s.id == gBktWater ? WATER : LAVA);
    if (!creative) s.id = gBktEmpty;   // no criativo o balde continua cheio
    return true;
}
