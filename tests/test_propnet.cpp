// patchProp.6: cadeira (pose/direcao/altura), rede dos props (NP_PROP) e ids acima de 32767 em uint16
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 150;
static NBuf wireOf(const NBuf& b) { NBuf c = b; c.r = 0; c.get<uint8_t>(); c.get<uint8_t>(); return c; }   // o recv real ja leu tipo e remetente
int main() {
    if (chdir("..") != 0) return 99;
    if (!std::filesystem::exists("mods/decocraft/props.bin")) { printf("SKIP\n"); return 0; }
    propsRegisterTab(); CHECK(gPropTotal >= 3119);
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 4; y++) setB(x, y, z, AIR); }
    int ch = -1, lamp = -1, door = -1, bd = -1; int dpi = 0; for (int i = 0; i < (int)gPropPacks.size(); i++) if (gPropPacks[i].mod == "Decocraft") dpi = i;
    const auto& its = gPropPacks[dpi].items; int PB = 0; for (int i = 0; i < dpi; i++) PB += (int)gPropPacks[i].items.size();
    for (int i = 0; i < (int)its.size(); i++) { if (ch < 0 && its[i].beh == "Chair") ch = i; if (lamp < 0 && its[i].beh == "SwitchableLamp") lamp = i; if (bd < 0 && its[i].beh == "Bed") bd = i;
        if (door < 0 && its[i].beh.rfind("Door:", 0) == 0 && propFromKey(gPropPacks[dpi].mod + "/deco_" + its[i].beh.substr(5))) door = i; }
    CHECK(ch >= 0 && lamp >= 0 && door >= 0 && bd >= 0);
    // ---- cadeira: o jogador olha para a frente (+Z do modelo girado), pes abaixo do tampo, pose sentada ----
    { int nOk = 0, nTot = 0;
      for (int i = 0; i < (int)its.size() && nTot < 60; i++) { if (its[i].beh != "Chair") continue;
        for (int rot = 0; rot < 16; rot += 3) { PropEnt e = propMake(PROP0 + PB + i, 4.5f, AY, 4.5f, rot); double o[3]; float yaw = 0; propSeatFor(e, o, &yaw); nTot++;
            const double th = rot * 22.5 * M_PI / 180, fx = sin(th), fz = cos(th), lx = -sin(yaw * M_PI / 180), lz = -cos(yaw * M_PI / 180);   // olhar do corpo = (-sin yaw, -cos yaw)
            if (fabs(lx - fx) < 1e-3 && fabs(lz - fz) < 1e-3 && o[1] < e.box.y1 && o[1] > e.y - 1) nOk++; } }
      CHECK(nTot > 0 && nOk == nTot); }
    { PropEnt e = propMake(PROP0 + PB + ch, 4.5f, AY, 4.5f, 0); double o[3]; float yaw; propSeatFor(e, o, &yaw); CHECK(fabsf(fabsf(yaw) - 180.f) < 1e-3f);   // giro 0: frente +Z => o corpo gira 180
      PropEnt e4 = propMake(PROP0 + PB + ch, 4.5f, AY, 4.5f, 4); propSeatFor(e4, o, &yaw); CHECK(fabsf(yaw - (-90.f)) < 1e-3f); }
    // ---- sentar, levantar e ocupar assento (rede) ----
    gProps.clear(); gSeated = false; remotes.clear();
    { gProps.push_back(propMake(PROP0 + PB + ch, 4.5f, AY, 4.5f, 0)); propNbAll();
      CHECK(!propSeatTaken(gProps[0]));
      double o[3]; propSeatFor(gProps[0], o);
      RemotePl r; r.init = true; r.sit = true; r.x = o[0]; r.y = o[1]; r.z = o[2]; remotes[5] = r; CHECK(propSeatTaken(gProps[0]));
      const AABB b = gProps[0].box; const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)b.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false;
      CHECK(propUse(eye, d, 5.f) == 1 && !gSeated);   // ocupado por outro jogador: nao senta
      remotes.clear(); CHECK(propUse(eye, d, 5.f) == 1 && gSeated);
      Phys P; P.x = 20; P.y = AY + 5; P.z = 20; CHECK(propSeatTick(P, false) && fabsf(gSeatYaw) > 179.f);
      gSeated = false; }
    // ---- rede: colocar / trocar / remover / sincronizar ao entrar ----
    gProps.clear(); netHost = true;
    { PropEnt a = propMake(PROP0 + PB + ch, 4.5f, AY, 4.5f, 5), l = propMake(PROP0 + PB + lamp, 6.5f, AY, 4.5f, 0), dr = propMake(PROP0 + PB + door, 8.5f, AY, 4.5f, 4); l.on = 0;
      NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_ADD); b.put<uint8_t>(3); propNetEnt(b, a); propNetEnt(b, l); propNetEnt(b, dr);
      NBuf r = wireOf(b); propNetRecv(r, 3); CHECK(gProps.size() == 3);
      CHECK(propFind(a.id, a.x, a.y, a.z, a.rot) >= 0 && propFind(l.id, l.x, l.y, l.z, 0) >= 0);
      const int li = propFind(l.id, l.x, l.y, l.z, 0); CHECK(li >= 0 && gProps[li].on == 0);   // estado ligada/desligada vai junto
      NBuf r2 = wireOf(b); propNetRecv(r2, 3); CHECK(gProps.size() == 3);   // reenvio: sem duplicar
      // lampada liga (op 3 com a mesma chave)
      PropEnt l2 = l; l2.on = 1; NBuf s; s = netMk(NP_PROP); s.put<uint8_t>(PROPNET_SWAP); propNetStr(s, propKey(l.id)); propNetStr(s, propKey(l2.id)); s.put<float>(l.x); s.put<float>(l.y); s.put<float>(l.z); s.put<uint8_t>((uint8_t)l.rot); s.put<uint8_t>(l2.on);
      NBuf r3 = wireOf(s); propNetRecv(r3, 3); CHECK(gProps[propFind(l.id, l.x, l.y, l.z, 0)].on == 1);
      // porta abre (troca de modelo, mesma celula)
      const int openId = propFromKey(gPropPacks[dpi].mod + "/deco_" + its[door].beh.substr(5));
      NBuf t = netMk(NP_PROP); t.put<uint8_t>(PROPNET_SWAP); propNetStr(t, propKey(dr.id)); propNetStr(t, propKey(openId)); t.put<float>(dr.x); t.put<float>(dr.y); t.put<float>(dr.z); t.put<uint8_t>((uint8_t)dr.rot); t.put<uint8_t>(1);
      NBuf r4 = wireOf(t); propNetRecv(r4, 3); CHECK(propFind(dr.id, dr.x, dr.y, dr.z, dr.rot) < 0 && propFind(openId, dr.x, dr.y - propLiftOf(dr.id) + propLiftOf(openId), dr.z, dr.rot) >= 0);
      // sincronizacao de quem entra: todos os props viram pacotes que reconstroem o mesmo estado
      NetPeer peer{}; propNetSyncPeer(peer); CHECK(!peer.q.empty());
      std::vector<PropEnt> keep = gProps; gProps.clear();
      for (NBuf& pk : peer.q) { NBuf q = wireOf(pk); propNetRecv(q, 0); }
      CHECK(gProps.size() == keep.size()); bool same = true; for (const PropEnt& k : keep) if (propFind(k.id, k.x, k.y, k.z, k.rot) < 0) same = false; CHECK(same);
      // remover
      NBuf dl = netMk(NP_PROP); dl.put<uint8_t>(PROPNET_DEL); propNetEnt(dl, a); NBuf r5 = wireOf(dl); propNetRecv(r5, 3); CHECK(gProps.size() == keep.size() - 1 && propFind(a.id, a.x, a.y, a.z, a.rot) < 0);
      // chave de item desconhecida nao quebra
      NBuf bad = netMk(NP_PROP); bad.put<uint8_t>(PROPNET_ADD); bad.put<uint8_t>(1); propNetStr(bad, "naoexiste/deco_1"); bad.put<float>(1); bad.put<float>(2); bad.put<float>(3); bad.put<uint8_t>(0); bad.put<uint8_t>(1);
      NBuf r6 = wireOf(bad); const size_t n0 = gProps.size(); propNetRecv(r6, 3); CHECK(gProps.size() == n0);
      // cama: o host pula a noite pedida por convidado
      const long long wt = gWorldTime; gWorldTime = 14000; NBuf bb = netMk(NP_PROP); bb.put<uint8_t>(PROPNET_BED); NBuf r7 = wireOf(bb); propNetRecv(r7, 3); CHECK(gWorldTime == DAY_TICKS); gWorldTime = wt; }
    netHost = false;
    // ---- ids de props (>= 32768) cabem em uint16 no pacote de estado/drop ----
    { NBuf b; const int id = PROP0 + PB + 3118; b.put<uint16_t>((uint16_t)id); CHECK(b.get<uint16_t>() == id && id > 32767);
      DropEnt dd; dd.id = id; dd.n = 1; dd.nid = 77; NBuf pk = netDropPacket(dd); NBuf q = wireOf(pk); q.get<uint32_t>(); for (int i = 0; i < 6; i++) q.get<float>(); CHECK(q.get<uint16_t>() == id); }
    // ---- translucidez por FACE: moldura opaca de textura translucida nao vai para a passada do vidro ----
    { PropPack& pk = gPropPacks[dpi]; int ti = -1; for (int i = 0; i < (int)pk.tex.size(); i++) if (!pk.tex[i].ref && pk.tex[i].w >= 8 && pk.tex[i].h >= 8) { ti = i; break; }
      CHECK(ti >= 0); if (ti >= 0) { const PropTex pt = pk.tex[ti]; gPropTrans.resize(gPropPacks.size()); gPropTrans[dpi].assign(pk.tex.size(), 0); gPropTrans[dpi][ti] = 1;
        gPropTexA.resize(gPropPacks.size()); std::vector<uint8_t>& ca = gPropTexA[dpi][ti]; ca.assign((size_t)pt.w * pt.h, 0);
        for (int y = 0; y < pt.h; y++) for (int x = 0; x < pt.w; x++) ca[(size_t)y * pt.w + x] = x < pt.w / 2 ? 2 : 1;   // esquerda opaca, direita parcial
        const float W = (float)pk.pageW[pt.page], H = (float)pk.pageH[pt.page];
        auto mk = [&](int x0, int x1) { PropQuad q; q.page = pt.page; q.tex = ti; const float u0 = (pt.x + x0) / W, u1 = (pt.x + x1) / W, v0 = pt.y / H, v1 = (pt.y + pt.h) / H;
            q.uv[0][0] = u0; q.uv[0][1] = v0; q.uv[1][0] = u1; q.uv[1][1] = v0; q.uv[2][0] = u1; q.uv[2][1] = v1; q.uv[3][0] = u0; q.uv[3][1] = v1; return q; };
        PropQuad qo = mk(0, pt.w / 2), qp = mk(pt.w / 2, pt.w), qa = mk(0, pt.w);
        CHECK(!propQuadTrans(dpi, qo)); CHECK(propQuadTrans(dpi, qp)); CHECK(propQuadTrans(dpi, qa)); CHECK(qo.trc == 0 && qp.trc == 1); } }
    // ---- patchProp.8: reenvio de UDP perdido (ADD/DEL entram na fila; op nova na mesma celula descarta a antiga; 3 envios no total) ----
    { const bool on0 = netOn; netOn = true; gPropRep.clear();
      PropEnt e = propMake(PROP0 + PB + ch, 6.5f, AY, 6.5f, 2); propNetAdd(e); CHECK(gPropRep.size() == 1 && gPropRep[0].left == 2);
      propNetDel(e); CHECK(gPropRep.size() == 1);   // DEL substitui o ADD pendente (nao ressuscita)
      PropEnt f = propMake(PROP0 + PB + ch, 9.5f, AY, 9.5f, 0); propNetAdd(f); CHECK(gPropRep.size() == 2);
      propNetRepeatTick(100000); CHECK(gPropRep.size() == 2 && gPropRep[0].left == 1);
      propNetRepeatTick(100200); CHECK(gPropRep.size() == 0);
      netOn = false; propNetAdd(e); CHECK(gPropRep.empty()); netOn = on0; }
    // ---- patchProp.9: luz de prop (lampada ligada emite; desligada nao; remover apaga) ----
    { int lt = -1, lsw = -1; for (int i = 0; i < (int)its.size(); i++) { if (its[i].light > 0 && its[i].beh == "SwitchableLamp" && lsw < 0) lsw = i; if (its[i].light > 0 && its[i].beh.empty() && lt < 0) lt = i; }
      CHECK(lsw >= 0); gProps.clear(); gPropLightV.clear();
      if (lsw >= 0) { PropEnt e = propMake(PROP0 + PB + lsw, 12.5f, AY, 12.5f, 0); e.on = 1; gProps.push_back(e); propNbAll();
        CHECK(gPropLightV.size() == 1 && gPropLightV[0][3] == std::min(15, its[lsw].light) && gPropLightV[0][0] == 12 && gPropLightV[0][2] == 12);
        gProps[0].on = 0; propLightRebuild(); CHECK(gPropLightV.empty());   // desligada
        gProps[0].on = 1; propLightRebuild(); CHECK(gPropLightV.size() == 1); gProps.clear(); propNbAll(); CHECK(gPropLightV.empty()); }
      if (lt >= 0) { gProps.push_back(propMake(PROP0 + PB + lt, 14.5f, AY, 14.5f, 0)); propNbAll(); CHECK(gPropLightV.size() == 1); gProps.clear(); propNbAll(); } }
    // ---- patchProp.10: efeitos (castical = fumaca; torneira/chuveiro so com agua ligada) ----
    { int pe = -1, fa = -1, sh = -1; for (int i = 0; i < (int)its.size(); i++) { if (pe < 0 && its[i].beh == "ParticleEmitter") pe = i; if (fa < 0 && its[i].beh == "Faucet") fa = i; if (sh < 0 && its[i].beh == "ShowerHead") sh = i; }
      CHECK(pe >= 0 && fa >= 0 && sh >= 0); gProps.clear(); PUFFS.clear(); Phys ph; ph.x = 10; ph.y = AY; ph.z = 10;
      if (fa >= 0 && pe >= 0 && sh >= 0) {
        gProps.push_back(propMake(PROP0 + PB + fa, 10.5f, AY, 10.5f, 0)); gProps.push_back(propMake(PROP0 + PB + sh, 12.5f, AY, 12.5f, 0)); CHECK(gProps[0].on == 0 && gProps[1].on == 0);
        for (int k = 0; k < 20; k++) propFxTick(ph); CHECK(PUFFS.empty());   // agua desligada: nada
        gProps[0].on = 1; gProps[1].on = 1; for (int k = 0; k < 20; k++) propFxTick(ph); CHECK(!PUFFS.empty() && PUFFS[0].kind == 1);
        PUFFS.clear(); gProps.clear(); gProps.push_back(propMake(PROP0 + PB + pe, 11.5f, AY, 11.5f, 0)); for (int k = 0; k < 30; k++) propFxTick(ph); CHECK(!PUFFS.empty() && PUFFS[0].kind == 0);
        PUFFS.clear(); gProps[0].x = 200; for (int k = 0; k < 30; k++) propFxTick(ph); CHECK(PUFFS.empty()); }   // longe do jogador: nao gasta
      gProps.clear(); PUFFS.clear(); }
    // ---- patchProp.13: ShowerHandles liga/desliga o ShowerHead vizinho (so os proximos) ----
    { int hd = -1, sh = -1; for (int i = 0; i < (int)its.size(); i++) { if (hd < 0 && its[i].beh == "ShowerHandles") hd = i; if (sh < 0 && its[i].beh == "ShowerHead") sh = i; }
      CHECK(hd >= 0 && sh >= 0);
      if (hd >= 0 && sh >= 0) { gProps.clear(); const bool on0 = netOn; netOn = false;
        gProps.push_back(propMake(PROP0 + PB + hd, 20.5f, AY, 20.5f, 0)); gProps.push_back(propMake(PROP0 + PB + sh, 21.5f, AY + 1, 20.5f, 0)); gProps.push_back(propMake(PROP0 + PB + sh, 40.5f, AY, 40.5f, 0)); propNbAll();
        const AABB b = gProps[0].cb.empty() ? gProps[0].box : gProps[0].cb[0]; const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)gProps[0].box.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false;
        CHECK(gProps[0].on == 0 && gProps[1].on == 0);
        CHECK(propUse(eye, d, 5.f) == 1 && gProps[0].on == 1 && gProps[1].on == 1 && gProps[2].on == 0);   // liga o proximo, nao o distante
        CHECK(propUse(eye, d, 5.f) == 1 && gProps[0].on == 0 && gProps[1].on == 0);
        gProps.clear(); netOn = on0; } }
    // ---- patchProp.15: Decobench pede a mesa de trabalho ----
    { int db = -1; for (int i = 0; i < (int)its.size(); i++) if (db < 0 && its[i].beh == "Decobench") db = i; CHECK(db >= 0);
      if (db >= 0) { gProps.clear(); const bool on0 = netOn; netOn = false; gProps.push_back(propMake(PROP0 + PB + db, 30.5f, AY, 30.5f, 0)); propNbAll(); gPropOpenTable = false;
        const AABB b = gProps[0].cb.empty() ? gProps[0].box : gProps[0].cb[0]; const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)gProps[0].box.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false;
        CHECK(propUse(eye, d, 5.f) == 1 && gPropOpenTable); gPropOpenTable = false; gProps.clear(); netOn = on0; } }
    // ---- patchProp.11b: dormir (so de noite; 5 s; agachar acorda; pula a noite no fim) ----
    { gProps.clear(); gSeated = false; gSleepT = 0; gSleepA = 0; Phys ph; ph.x = 4; ph.y = AY; ph.z = 4; const bool on0 = netOn; netOn = false; const bool h0 = netHost; netHost = true;
      gProps.push_back(propMake(PROP0 + PB + bd, 4.5f, AY, 4.5f, 0)); propNbAll();
      const AABB b = gProps[0].box; const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)b.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false;
      gWorldTime = DAY_TICKS * 3 + 4000; CHECK(propUse(eye, d, 5.f) == 1 && gSleepT == 0);   // de dia: so define o spawn
      gWorldTime = DAY_TICKS * 3 + 13000; CHECK(propUse(eye, d, 5.f) == 1 && gSleepT == 1);
      for (int k = 0; k < 40; k++) propSleepTick(ph, false);
      CHECK(gSleepT > 1 && gSleepA > .9f && fabs(ph.y - b.y1) < 1e-6 && gWorldTime == DAY_TICKS * 3 + 13000);   // deitado, tela escura, ainda noite
      propSleepTick(ph, true); CHECK(gSleepT == 0);   // agachar acorda sem pular a noite
      CHECK(gWorldTime == DAY_TICKS * 3 + 13000);
      for (int k = 0; k < 30; k++) propSleepTick(ph, false); CHECK(gSleepA < .1f);
      CHECK(propUse(eye, d, 5.f) == 1 && gSleepT == 1); for (int k = 0; k < 105; k++) propSleepTick(ph, false);
      CHECK(gSleepT == 0 && gWorldTime == DAY_TICKS * 4);   // dormiu ate o fim: amanheceu
      gWorldTime = DAY_TICKS * 5 + 13000; CHECK(propUse(eye, d, 5.f) == 1 && gSleepT == 1 && fabsf(gSleepYaw) < 1e-3f);   // patchProp.14: deita virado para o giro da cama
      for (int k = 0; k < 10; k++) propSleepTick(ph, false); CHECK(gSleepT > 1);
      gHurtWake = true; propSleepTick(ph, false); CHECK(gSleepT == 0 && !gHurtWake && gWorldTime == DAY_TICKS * 5 + 13000);   // dano acorda sem pular a noite
      gProps.clear(); netOn = on0; netHost = h0; }
    printf("%d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
