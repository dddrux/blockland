// camera interpolada, mundo por viewport, mobs, HUD, tela (ate o fim do quadro)
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
        if (ticksThisFrame == maxTk && tickAcc >= 0.05) tickAcc = 0.0;   // não acumula atraso se o frame travou
        const float pt = (float)(tickAcc / 0.05);   // partialTicks
        if (const BoatEnt* rb = boatById(gBoatRide)) {   // camera acompanha o giro do bote (EntityBoat.updatePassenger: yaw += deltaRotation) POR QUADRO, com o yaw interpolado do bote; cabeca presa a 105 graus
            const float bo = boatYawLerp(*rb, pt);
            if (gBoatCamId != gBoatRide) { gBoatCamId = gBoatRide; gBoatCamLast = bo; }
            yaw = wrap180(yaw + wrap180(bo - gBoatCamLast)); gBoatCamLast = bo;
            float d = wrap180(yaw - bo); d = std::max(-105.f, std::min(105.f, d)); yaw = wrap180(bo + d);
        } else gBoatCamId = 0;

        // ===== viewports (split screen): o bloco de desenho abaixo roda 1x por viewport; na 2a passada o contexto do jogador 2 entra no lugar do jogador 1 =====
        int fw = 0, fh = 0; SDL_GetWindowSize(win, &fw, &fh);
        const bool dual = gSplit;   // patchSplit.4: inventario/pausa/placa abrem SO no viewport do dono (o mouse vem em coordenadas locais do viewport); os outros continuam
        const int vpBeg = 0, vpEnd = dual ? gSplitN : 1;
        DbgInfo dbgD; bool dbgHave = false;
        int s2Cam = 0, s2Boat = 0, s2Cart = 0, s2Hand = 0; float s2Eq = 1.f, s2Nm = 0.f, s2Mm = 0.f;
        double vwP1[3] = {viewEye[0], viewEye[1], viewEye[2]};   // olhos do jogador 1 (restaurados apos as passadas dos extras)
        auto swapView = [&]() {   // troca o contexto de VISAO entre jogador 1 e jogador 2 (chamado 2x: entra e sai)
            std::swap(P, SX.P); std::swap(yaw, SX.yaw); std::swap(pitch, SX.pitch); std::swap(sel, SX.sel); std::swap(bodyYaw, SX.bodyYaw); std::swap(prevBodyYaw, SX.prevBodyYaw);
            std::swap(limbSwing, SX.limbSwing); std::swap(limbAmt, SX.limbAmt); std::swap(prevLimbAmt, SX.prevLimbAmt); std::swap(swing, SX.swing); std::swap(swinging, SX.swinging);
            std::swap(mnOn, SX.mnOn); std::swap(mnT, SX.mnT); std::swap(mnNeed, SX.mnNeed); std::swap(mnX, SX.mnX); std::swap(mnY, SX.mnY); std::swap(mnZ, SX.mnZ);
            split2SwapInv(); if (SX.skinTex) std::swap(skinTex, SX.skinTex);
            split2SwapSurv();   // vida, fome, fogo, comer, arco do jogador 2 (HUD e maos dele)
            std::swap(camMode, s2Cam); std::swap(gBoatRide, gBoatX[gS2i]); std::swap(gCartRide, s2Cart); std::swap(handItem, s2Hand); std::swap(equip, s2Eq);
            std::swap(camBobYaw, SX.cbY); std::swap(prevCamBobYaw, SX.pcbY); std::swap(camBobPitch, SX.cbP); std::swap(prevCamBobPitch, SX.pcbP); std::swap(fovMod, SX.fovMod); std::swap(prevFovMod, SX.prevFovMod);   // patchSplit.2: um par por jogador
            for (int i = 0; i < 4; i++) std::swap(gHandBob[i], SX.hb[i]); std::swap(nameT, s2Nm); std::swap(msgT, s2Mm);
        };
        for (int vp = vpBeg; vp < vpEnd; vp++) {
        int vx = 0, vy = 0, w = fw, h = fh; if (dual) split2Rect(vp, fw, fh, vx, vy, w, h);
        gVpX = vx; gVpY = vy;
        const int uiVpN = uiVpNow(); const bool kInv = invOpen, kPause = pauseOpen, kSign = gSignEd, ownSw = dual && gInvOwner == 2;   // patchSplit.4: a UI so aparece no viewport do dono
        if (dual && vp != uiVpN) { invOpen = false; pauseOpen = false; gSignEd = false; }
        if (ownSw) split2SwapInvOwner();   // inventario do extra aberto: os globais guardam o dele; cada passada precisa do inventario do SEU jogador (desfeito no fim)
        if (vp >= 1) { gS2i = vp - 1; swapView(); held = inv[sel].id; handItem = held; equip = 1.f; }
        // ----- estado interpolado para renderização (igual ao Java: prev + (cur - prev) * partialTicks) -----
        double bcx = 0, bcy = 0, bcz = 0; if (const BoatEnt* rbc = gBoatRide ? boatById(gBoatRide) : nullptr) { const double q = 1.0 - pt; bcx = q * ((rbc->px - rbc->x) - (P.px - P.x)); bcy = q * ((rbc->py - rbc->y) - (P.py - P.y)); bcz = q * ((rbc->pz - rbc->z) - (P.pz - P.z));
            if (!gBoatPaxSelf) { float ph = 0; for (int i = 0; i < 2; i++) { const int k = 1 - i; ph += rbc->paddleAmp[k] * sinf(rbc->paddlePos[k] + .3926991f * rbc->paddleAmp[k] * (pt - 1.f)); }   // patchBoat5.3: vai-e-vem aplicado na BASE (camera e corpo juntos; antes so a camera andava e o torso entrava/saia da tela)
                const double yb = boatYawLerp(*rbc, pt) * .0174533, avg = .5 * ph, off = (avg > 0 ? .20 : .07) * avg; bcx += -sin(yb) * off; bcz += -cos(yb) * off; } }   // patchBoat5.2: camera atrasada no bote: ancora a posicao interpolada no PROPRIO bote (o jogador e atualizado fora de fase com ele => 1 tick de atraso)
        const double rX = P.px + (P.x - P.px) * pt + bcx, rY = P.py + (P.y - P.py) * pt + bcy, rZ = P.pz + (P.z - P.pz) * pt + bcz;
        const float eyeH = gSleepT > 0 ? .2f : P.sneak ? 1.54f : 1.62f;   // patchProp.14: deitado, o olho fica rente ao colchao    // 1.8.9: agachar baixa o olho em 0.08
        double bEx = 0, bEy = 0, bEz = 0; if (gBoatRide && !camMode) { if (const BoatEnt* eb = boatById(gBoatRide)) { boatEyeDelta(*eb, pt, bEx, bEy, bEz);
        } }   // patchBoat4.2: olho segue o corpo no balanco do bote
        const double eyeX = rX + bEx, eyeY = rY + eyeH + bEy, eyeZ = rZ + bEz;
        viewEye[0] = eyeX; viewEye[1] = eyeY; viewEye[2] = eyeZ; if (vp == 0) { vwP1[0] = eyeX; vwP1[1] = eyeY; vwP1[2] = eyeZ; }
        float renderBodyYaw = prevBodyYaw + (bodyYaw - prevBodyYaw) * pt;
        if (vp == 0 && grab && gSleepT <= 0) {   // patchSleep.4: deitado a camera nao gira (era isto que tremia: o mouse girava aqui e o tick puxava de volta). patchFPS.2: mouse lido o mais tarde possivel (movimento que chegou depois do inicio do quadro entra neste mesmo quadro = menos input lag)
            SDL_PumpEvents(); SDL_Event me[64]; const int nm = SDL_PeepEvents(me, 64, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
            const float msL = std::max(.1f, gsf("q1", .5f) * 2.f), mvL = gsi("q2", 0) ? -1.f : 1.f;
            for (int i = 0; i < nm; i++) { if (me[i].motion.which == 0x7F000001u) continue;
                yaw -= me[i].motion.xrel * .1f * gZoom * msL; if (yaw > 180.f) yaw -= 360.f; if (yaw < -180.f) yaw += 360.f;
                pitch = std::max(-89.f, std::min(89.f, pitch - me[i].motion.yrel * .1f * gZoom * msL * mvL)); }
        }
        float renderYaw = yaw, renderPitch = pitch; gYawNow = yaw;
        const float renderLimbAmt = std::min(1.f, prevLimbAmt + (limbAmt - prevLimbAmt) * pt);
        const float renderLimb = limbSwing - limbAmt * (1.f - pt);
        const float fovHand = prevFovMod + (fovMod - prevFovMod) * pt;

        if (vp >= 1) {   // mira do jogador extra (o raio usa os olhos/angulos dele; o jogador 1 ja foi calculado no topo do quadro)
            eye[0] = (float)eyeX; eye[1] = (float)eyeY; eye[2] = (float)eyeZ; const float c2 = cosf(yaw * .0174533f), s2 = sinf(yaw * .0174533f), cp2 = cosf(pitch * .0174533f);
            dir[0] = -s2 * cp2; dir[1] = sinf(pitch * .0174533f); dir[2] = -c2 * cp2; has = ray(eye, dir, hh, pp); }
        gHeldSel = sel; gPlayerYaw = yaw; gPlayerPitch = pitch;
        if (vp == vpBeg) {   // simulacao do mundo: so 1x por quadro (nao por viewport)
        for (Slot& ts : throwQ) throwItem(viewEye, yaw, pitch, ts.id, ts.n, ts.nbt);
        throwQ.clear();
        RAD = rdChunks();   // menu Video > Renderizacao
#if defined(__ANDROID__) || defined(BL_RPI)
#ifdef BL_RPI
        RAD = std::max(2, RAD - gAdaptCut);   // patchRpi.9: ajuste automatico tambem no Pi (minimo 2 chunks)
#else
        RAD = std::max(3, RAD - gAdaptCut);
#endif
          // ajuste automatico de FPS (perf.inc)
#endif
        {   // orcamento ADAPTATIVO: usa a folga do quadro (alvo ~55 fps) em vez de 3 ms fixos => chunks carregam bem mais rapido quando ha folga
            // a espera da GPU (swap) e tempo ocioso da CPU: quando a GPU e o gargalo, carregar chunks ali nao custa FPS
            #ifdef __ANDROID__
            const double sBudget = std::max(1.5, std::min(4.0, 16.0 - gPfOtherMs));   // celular: no maximo 4 ms/quadro gerando/malhando chunks (antes ate 14 ms = picos de FPS)
#else
            const double sBudget = std::max(2.5, std::min(RAD > 10 ? 8.0 : 11.0, 16.0 - gPfOtherMs));   // patchPerf.1: teto menor com chunks altos (antes 14 ms: um quadro de carga + desenho passava de 16 ms e o FPS oscilava); o carregamento so fica um pouco mais gradual
#endif
            gStreamBudgetMs = sBudget;
            { static const bool lodEnv = getenv("BLOCKLAND_NOLOD") == nullptr; const float F = fogEndD() / 16.f;   // patchLOD.1: passo 2 a 92% da nevoa, passo 4 a 99% (patchLOD.7: LOD mais sutil, antes 80%/93%; patchLOD.6: antes 55%/80%, reduzia blocos perto demais)
              gLodOn = false;   /* patchLOD.5: LOD desligado */ gLodR2 = std::max(4, (int)(F * .92f)); gLodR4 = std::max(gLodR2 + 2, (int)(F * .99f)); }   // alvo fixo ~60 fps (antes 0,9x o quadro: o orcamento crescia com o lag e prendia o jogo em ~40 fps)
            const int sFar = gPerf ? 2 : (RAD <= 6 ? (1 << 20) : std::max(6, (int)(fogEndD() / 16.f * .90f)))   /* patchFps.6: LOD de cubos distantes mais cedo: malha "longe" (sem modelos, so faces expostas) a partir de 80% da nevoa (antes 92%, e so com RAD>8) */;   // modo desempenho: a partir de 2 chunks a malha dispensa plantas e faces enterradas; normal: raio grande dispensa plantas longe da camera (a nevoa as esconde)
            { static int tpx = 0x7fffffff, tpz = 0, trad = 0; static Uint32 tt = 0; const int ccx = fdiv((int)floorf((float)P.x), CW), ccz = fdiv((int)floorf((float)P.z), CW), tn = (int)SDL_GetTicks();
              if (ccx != tpx || ccz != tpz || trad != RAD || tn - (int)tt > 2000) { tpx = ccx; tpz = ccz; trad = RAD; tt = (Uint32)tn; for (int q = 0; q < 3; q++) { const bool ok = gSplit && q < gSplitN - 1 && gSX[q].init; gTrimX2[q] = ok ? fdiv((int)floor(gSX[q].P.x), CW) : 0x7fffffff; gTrimZ2[q] = ok ? fdiv((int)floor(gSX[q].P.z), CW) : 0; } gpuTrim(ccx, ccz); }
              gSimR = simRadFor(RAD); gSimCx = netOn ? 0x7fffffff : ccx; gSimCz = ccz; for (int q = 0; q < 3; q++) { gSimOx[q] = gTrimX2[q]; gSimOz[q] = gTrimZ2[q]; }   // patchChunks.4
              { static int uf = 0; if (++uf >= (oldUnload() ? 6 : 2)) { uf = 0; chunkUnload(ccx, ccz, oldUnload() ? 3 : 8); } } }   // patchChunks.4: a cada 2 quadros, ate 8 (antes 6 quadros / 3)   // patchUnload.1: a cada 6 quadros, ate 3 chunks
            if (gPregenArg > 0 && !pregenGuest()) { pregenStart(std::min(64, gPregenArg), P); gPregenArg = 0; }
            pregenTick();   // patchPregen.1: /pregen em segundo plano (orcamento por quadro)
            #if defined(__ANDROID__) || defined(BL_RPI)
            int sMaxGen = 3, sMaxMesh = 3; double sBud = sBudget;   // celular: poucos chunks por quadro (carrega um pouco mais devagar, sem engasgar)
#else
            int sMaxGen = RAD > 10 ? 10 : 20, sMaxMesh = RAD > 10 ? 6 : 12; double sBud = sBudget;   // patchPerf.1: menos malhas por quadro = menos uploads de VBO juntos no mesmo quadro (picos); sem perda de qualidade
#endif
#if defined(__ANDROID__) || defined(BL_RPI)
            {   // celular/Pi (patchRpiOpt.1): no maximo 1 chunk gerado e 1 malha por quadro (um chunk custa dezenas de ms na CPU do celular); se o quadro ja esta lento, so carrega a cada 3 quadros
                static int slowSkip = 0; const bool near0 = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)) != nullptr && W.size() > 30;
                sMaxGen = 1; sMaxMesh = 1; sBud = std::min(sBud, 6.0);
                if (near0 && gPfFrameMs > 22.0 && (++slowSkip % 3) != 0) { sMaxGen = 0; sMaxMesh = 0; }
            }
#endif
#ifdef BL_RPI
            if (gTpHold <= 0) { static unsigned rpiFr = 0; if (rpiFr++ & 1) sMaxGen = 0; else sMaxMesh = 0; sBud = std::min(sBud, 8.0); }
            {   // patchRpi.3: bloco quebrado/colocado = chunk sujo ao lado do jogador: malha ja neste quadro (antes esperava quadro alternado / pulo de quadros lentos)
                const int ucx = fdiv((int)floorf((float)P.x), CW), ucz = fdiv((int)floorf((float)P.z), CW); bool urg = false;
                { Chunk* uc = (abs(gUrgCx - ucx) <= 1 && abs(gUrgCz - ucz) <= 1) ? getC(gUrgCx, gUrgCz) : nullptr; urg = uc && uc->lc && (uc->dirty || uc->wdirty); }   // patchBreak.2: urgente = SO o chunk do bloco trocado; os vizinhos (luz) seguem o ritmo normal (1 por quadro, pulando quadros lentos) em vez de 9 malhas seguidas
                if (urg) { sMaxMesh = std::max(sMaxMesh, 1); sBud = std::max(sBud, 12.0); }
            }   // patchRpiOpt.2: gerar e malhar em quadros alternados (nunca os dois no mesmo quadro)
#endif
            if (gTpHold <= 0 && gPfOtherMs > 33.0) { sMaxGen = std::min(sMaxGen, 3); sMaxMesh = std::min(sMaxMesh, 2); }   // patchPerf.4: quadro anterior >33 ms (sem stream/swap) = so poucos chunks agora; carga gradual evita picos em cadeia
            if (gTpHold > 0) sBud = std::max(sBud, 14.0);   // carregando terreno do /tp: orcamento maior, mas sempre limitado por quadro
            {   // porta invisivel enquanto o jogador esta dentro dela (pes ou olhos na celula)
                const double hx = viewEye[0], hz = viewEye[2]; const int dxp = (int)floor(hx), dzp = (int)floor(hz), yy = (int)floor(viewEye[1]); int hy = -1000;
                if (getB(dxp, yy, dzp) == XB_DOOR) { const int ly = (getM(dxp, yy, dzp) & 8) ? yy - 1 : yy, dm = getM(dxp, ly, dzp); if (!((dm >> 2) & 1)) { const double fx = hx - dxp, fz = hz - dzp, T = .1875 + .12; const int kk = ((dm & 3) + 1) & 3; if (kk == 0 ? fx < T : kk == 1 ? fz < T : kk == 2 ? fx > 1 - T : fz > 1 - T) hy = ly; } }   // patchPorta.3: so some quando a CAMERA esta dentro da lamina (3px + margem do plano proximo) de porta FECHADA; antes sumia inteira com o jogador em qualquer parte da celula (porta 'invisivel' na entrada)
                if (hy != gDoorHide[1] || (hy > -1000 && (dxp != gDoorHide[0] || dzp != gDoorHide[2]))) {
                    const int ox = gDoorHide[0], oz = gDoorHide[2], oy = gDoorHide[1]; gDoorHide[0] = dxp; gDoorHide[1] = hy; gDoorHide[2] = dzp;
                    if (oy > -1000) dirtyC(fdiv(ox, CW), fdiv(oz, CW)); if (hy > -1000) dirtyC(fdiv(dxp, CW), fdiv(dzp, CW));
                }
            }
            stream(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW), sMaxGen, sMaxMesh, sBud, -sinf(yaw * .0174533f), -cosf(yaw * .0174533f), sFar, true);
            if (gSplit) for (int xk = 0; xk < gSplitN - 1; xk++) if (gSX[xk].init) {   // extras longe do 1: carrega e malha o mundo em volta deles tambem (metade do orcamento)
                const SplitP& X = gSX[xk]; const int q2x = fdiv((int)floor(X.P.x), CW), q2z = fdiv((int)floor(X.P.z), CW), p1x = fdiv((int)floorf((float)P.x), CW), p1z = fdiv((int)floorf((float)P.z), CW);
                if (abs(q2x - p1x) > 2 || abs(q2z - p1z) > 2) stream(q2x, q2z, std::max(1, sMaxGen / 2), std::max(1, sMaxMesh / 2), sBud * .5, -sinf(X.yaw * .017453f), -cosf(X.yaw * .017453f), sFar, true); }
        }
        if (gTpHold > 0) { Chunk* hc = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)); if ((hc && hc->lc && !hc->dirty) || ++gTpWait > 900) { gTpHold = 0; gKick = true; } }
        if (gNetTp.on) { if (gNetTp.dim >= 0 && gNetTp.dim != gDim) dimSwitch(gNetTp.dim); P.x = P.px = gNetTp.x; P.y = P.py = gNetTp.y; P.z = P.pz = gNetTp.z; P.mx = P.my = P.mz = 0; gFall = 0.f; gTpHold = 1; gTpWait = 0; gNetTp.on = false; }   // /tp ou /puxar vindo do host
        if (gNetReseed) { Chunk* pc = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW));   // seed trocada pelo host: sobe o jogador para a superfície nova
            if (pc) { const int lx = (int)floorf((float)P.x) - fdiv((int)floorf((float)P.x), CW) * CW, lz = (int)floorf((float)P.z) - fdiv((int)floorf((float)P.z), CW) * CW;
                int y = CH - 1; while (y > 0 && (pc->b[idx(lx, y, lz)] == AIR || isWater(pc->b[idx(lx, y, lz)]))) y--;
                P.y = y + 1.0; P.py = P.y; P.my = 0; gNetReseed = false; } }   // orçamento ~3 ms/quadro: sem travadas ao gerar chunks
        PROBE("antes-fluidos");
        { PerfScope perfSim(PS_SIM); simulateWater(dt, fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)); tickLeaves(dt); }
        updateParts(dt);
        }   // fim: so 1x por quadro

        const bool headWater = isWater(getB((int)floor(eyeX), (int)floor(eyeY), (int)floor(eyeZ)));
        const bool headLava = isLava(getB((int)floor(eyeX), (int)floor(eyeY), (int)floor(eyeZ)));   // cabeca na lava: tela laranja, visao de ~2 blocos
        if (dual) rdViewport(vx, vy, w, h); else blViewport(w, h);
        {   // ciclo dia/noite: cor do zênite, do horizonte/neblina e brilho da luz do céu deste quadro
            const float ly = (renderYaw + (camMode == 2 ? 180.f : 0.f)) * .0174533f, lpi = (camMode == 2 ? -renderPitch : renderPitch) * .0174533f;
            gAmbJava = gsi("amb", 0) == 0; gAmbRD = RAD; gAmbY = (float)eyeY;   // patchAmb.1: amb=1 em options.cfg volta ao ceu antigo
            { static float tS = .8f; const float tg = ambBiomeTemp(biomeAt((float)eyeX, (float)eyeZ)); tS += (tg - tS) * std::min(1.f, dt * 1.2f); gAmbTemp = tS; }   // 1.12 troca seco; aqui ~1 s de transicao
            updateSky(pt, -sinf(ly) * cosf(lpi), skyTop, skyC, skyTopD, skyHorD); wxSkyApply(skyTopD, skyHorD, !gAmbJava); bloodSkyHook(skyTopD, skyHorD);   // patchBlood.1
            for (int q = 0; q < 3; q++) uwD[q] = uwC[q] * gSky.sunB;
            if (gAmbJava) { const float f1 = (float)RAD / 32.f, fb = gSky.sunB * (1.f - f1) + f1; uwD[0] = .02f * fb; uwD[1] = .02f * fb; uwD[2] = .2f * fb; }
            if (gDim == 3) bloodUwHook(uwD);   // patchBlood.1: debaixo do sangue   // patchAmb.2: updateFogColor na agua = (.02,.02,.2) x brilho (no Java: luz do olho; aqui o brilho do sol)   // debaixo d'água: escurece com o brilho do sol
        }
        static const float lavaFogC[4] = {.60f, .10f, 0.f, 1.f};
        static const float netherFogC[4] = {.20f, .03f, .03f, 1.f}, endFogC[4] = {.094f, .075f, .094f, 1.f};   // End: 0xA080A0 * 0.15 (WorldProviderEnd.getFogColor)
        const float* sky = headLava ? lavaFogC : headWater ? uwD : gDim == 2 ? endFogC : gDim == 1 ? netherFogC : skyHorD;
        static float fogMix[4]; const float* skyF = sky;   // patchSky.7: no nascer/por a nevoa puxa a cor da faixa do sol (antes: cor do horizonte neutra => terreno escuro contra ceu laranja)
        if (sky == skyHorD && gSky.hasSunrise && !gAmbJava) { const float kf = .45f * std::min(1.f, gSky.sunrise[3]); for (int q = 0; q < 3; q++) fogMix[q] = skyHorD[q] + (gSky.sunrise[q] - skyHorD[q]) * kf; fogMix[3] = skyHorD[3]; skyF = fogMix; }
        glStackHeal("inicio do quadro");
        glClearColor(sky[0], sky[1], sky[2], 1);
        glClearStencil(0); if (dual) { rdEnable(GL_SCISSOR_TEST); rdScissor(vx, vy, w, h); }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); if (dual) rdDisable(GL_SCISSOR_TEST);
        float fov = (30.f + 80.f * gsf("fov", .5f)) * fovHand;   // slider Campo de Visao (30..110, padrao 70); correr aumenta
        if (headWater) fov = fov * 60.f / 70.f;     // dentro d'água o Java reduz o FOV
        { const bool zk = k[SDL_SCANCODE_C] && grab && !chatOpen && !invOpen && !pauseOpen && !gDead; const float zt = zk ? gZoomTgt : 1.f; gZoom += (zt - gZoom) * .35f; if (fabsf(gZoom - zt) < .003f) gZoom = zt; fov = std::max(4.f, fov * gZoom); }   // patchZoom.1: segurar C = zoom 4x suave
        lastFov = fov;
        float n = (RAD > 8 && !headWater) ? .09f : .05f, t = n * tanf(fov * .5f * .0174533f), r = t * w / (float)h;   // patchShader.2: render alto = near maior (menos z-fighting longe)
        rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdFrustum(-r, r, -t, t, n, 700);
        rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
        applyViewBob(P.distWalked, P.prevDistWalked, camBobYaw, prevCamBobYaw, camBobPitch, prevCamBobPitch, pt);
        // Câmera (EntityRenderer.orientCamera do 1.8.9):
        //  1ª pessoa: nos olhos.
        //  F5 (modo 1): ATRÁS do jogador, a 4 blocos ao longo de -olhar.
        //  F5 duplo (modo 2): NA FRENTE do jogador, olhando de volta (yaw+180, pitch invertido).
        //  A distância é limitada por 8 raycasts (±0.1) do pivô dos olhos até a posição desejada,
        //  na MESMA direção em que a câmera vai ser posicionada.
        static bool gSpecInside = false;   // patchSpect.10: espectador com a camera dentro de bloco solido
        double cam[3] = {eyeX, eyeY, eyeZ};
        float camYaw = renderYaw, camPitch = renderPitch;
        if (camMode) {
            const float radYaw = renderYaw * .01745329252f, radPitch = renderPitch * .01745329252f;
            const float lookX = -sinf(radYaw) * cosf(radPitch), lookY = sinf(radPitch), lookZ = -cosf(radYaw) * cosf(radPitch);
            // voo de elitra: o corpo deita ao redor dos pes, entao o pivo da camera passa a ser o centro do corpo deitado (nao os olhos), um pouco mais longe
            double pvX = eyeX, pvY = eyeY, pvZ = eyeZ; float camDist = thirdPersonDistance;
            if (P.elytra && !P.onGround && isElytraItem(armor[1].id) && gGm != 3) {
                const float ft = (float)P.elyTicks + pt, f1 = std::min(1.f, ft * ft / 100.f);
                pvX = rX + lookX * .9f * f1; pvY = rY + .9f * (1.f - f1) + lookY * .9f * f1 + .55f * f1; pvZ = rZ + lookZ * .9f * f1;
                camDist = thirdPersonDistance + 1.5f * f1;
            }
            const float camSign = camMode == 1 ? -1.f : 1.f;   // direção do pivô até a câmera
            const float toX = camSign * lookX, toY = camSign * lookY, toZ = camSign * lookZ;
            float d3 = camDist;
            for (int i = 0; i < (gGm == 3 ? 0 : 8); ++i) {   // patchSpect.10: espectador atravessa blocos: a camera F5 nao e empurrada pra perto do jogador
                const float ox8 = ((i & 1) * 2 - 1) * .1f, oy8 = (((i >> 1) & 1) * 2 - 1) * .1f, oz8 = (((i >> 2) & 1) * 2 - 1) * .1f;
                float ro[3] = {(float)pvX + ox8, (float)pvY + oy8, (float)pvZ + oz8};
                float rd[3] = {toX, toY, toZ};
                // escada (placa fina) nao bloqueia a camera: antes, ao subir/descer a escada a celula do proprio jogador contava como parede (distancia 0 = camera colada)
                float tHit = -1.f, t0 = 0.f;
                for (int sk = 0; sk < 6; ++sk) {
                    const float so[3] = {ro[0] + rd[0] * t0, ro[1] + rd[1] * t0, ro[2] + rd[2] * t0};
                    const float th = raySolidDistance(so, rd, camDist - t0);
                    if (th < 0.f) break;
                    const float hit = t0 + th;
                    const float px = so[0] + rd[0] * (th + .01f), py = so[1] + rd[1] * (th + .01f), pz = so[2] + rd[2] * (th + .01f);
                    const bool ladCell = getB((int)floorf(th <= 0.f ? so[0] : px), (int)floorf(th <= 0.f ? so[1] : py), (int)floorf(th <= 0.f ? so[2] : pz)) == XB_LADDER;
                    if (!ladCell) { tHit = hit; break; }
                    t0 = hit + (th <= 0.f ? .5f : .5f);   // pula a celula da escada e continua
                    if (t0 >= camDist) break;
                }
                if (tHit >= 0.f) {   // distância do ponto de impacto até o pivô (não até o canto deslocado)
                    float hx = ro[0] + rd[0] * tHit - (float)pvX, hy = ro[1] + rd[1] * tHit - (float)pvY, hz = ro[2] + rd[2] * tHit - (float)pvZ;
                    d3 = std::min(d3, sqrtf(hx * hx + hy * hy + hz * hz));
                }
            }
            cam[0] = pvX + toX * d3; cam[1] = pvY + toY * d3; cam[2] = pvZ + toZ * d3;
            if (camMode == 2) { camYaw = renderYaw + 180.f; camPitch = -renderPitch; }
        }
        const double shEc[3] = {rX, rY + 1.0, rZ};
        {
#ifdef BL_VULKAN
          if (BL_VKON) {  if (gShadowMasterClick) { gShadowMasterClick = false; gVkFancyWater = gsi("shm", 0) != 0; say(gVkFancyWater ? "Agua realista: ligada (sombras ainda nao existem no Vulkan)" : "Agua realista: desligada"); } gSt["shm"] = gVkFancyWater ? 1 : 0; }
          else
#endif
          { if (gShadowMasterClick) { gShadowMasterClick = false; const bool want = gsi("shm", 0) != 0; if (want != gShadowsOn) shadowToggle(say); if (!want) { gSt["shd.sel"] = 0; gShaderPackOn = false; } gSt["sh0"] = gShadowsOn ? 1 : 0; } gSt["shm"] = (gShadowsOn || gShaderPackOn) ? 1 : 0; } }   // mostra ON se QUALQUER efeito (sombra ou pacote de agua/sol) estiver ligado
   // patchVk.7: botao unico "Sombras realistas" (Video): sombras + agua + shader; desligar tambem solta o pacote de shaders
        if (!gPerf && gsi("an.0", 1) != 0) waterAnimTick();   // patchOpt.2: agua animada (Animacoes > Agua animada)
   // modo desempenho: sem agua animada (upload de tile por quadro)
        gDifficulty = std::max(0, std::min(3, gsi("d40", 2)));   // patchDif.1
        capeSyncSel();   // patchCapa.1: capa escolhida nas Configuracoes (pronta, PNG ou GIF de assets/capes)
        { const int am = gsi("armsel", 0); static int amLast = -1; const bool want = am == 2;   // patchSlim.1: Bracos (Auto|Normais|Finos) nas Configuracoes; Auto = detecta pela skin
          if (am != amLast) { amLast = am; slimAuto = am == 0; if (am == 0) { slim = areaClear(50, 16, 2, 4) && areaClear(54, 20, 2, 12) && areaClear(42, 48, 2, 4) && areaClear(46, 52, 2, 12); buildModel(); } gSkinNetDirty = true; }
          if (am != 0 && slim != want) { slim = want; buildModel(); gSkinNetDirty = true; } }
        { if (gGodClick) { gGodClick = false; if ((gsi("sh2", 0) != 0) != gGodOn) godToggle(say); } gSt["sh2"] = gGodOn ? 1 : 0;   // patchOpt.4: God rays / Lens flare = opcoes (Shaders), sem F9/F10
          if (gFlClick) { gFlClick = false; if ((gsi("sh3", 0) != 0) != gFlareOn) flareToggle(say); } gSt["sh3"] = gFlareOn ? 1 : 0; }
        { if (gShadowClick) { gShadowClick = false; if ((gsi("sh0", 0) != 0) != gShadowsOn) shadowToggle(say); } gSt["sh0"] = gShadowsOn ? 1 : 0; if (gShQClick) { gShQClick = false; gShQ = std::max(0, std::min(2, gsi("sh1", 2))); } }   // patchOpt.1
        if (gPerfShDrop) { gPerfShDrop = false; if (gShadowsOn && gShQ > 0) { gShQ--; gSt["sh1"] = gShQ; logInfo("[PERF] FPS baixo com sombras: qualidade das sombras -> %d (ajuste automatico)", gShQ); } }   // patchPerf.7
#ifdef __ANDROID__   // patchFix.Cape: so ESTA linha e Android (o #ifdef engolia capa, bracos, sombras e agua animada no PC)
        { static bool tried = false; if (!tried && !headWater) { tried = true; if (gsi("shd.mob", 0) == 1 && !gShadowsOn) shadowToggle(say); } }   // celular: religa as sombras se o jogador as deixou ligadas da ultima vez (padrao: desligadas)
#endif
        PROBE("pre-sombras");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        shadowFrame(cam, headWater || headLava || gDim, shEc);
        gEntShN = 0; gEntShOn = gShadowDraw && gSunStr > 0.25f;   // patchShadow.4: sombras de jogador/mobs (capsulas no shader); a lua fraca/noite continua com o blob
        if (gEntShOn) {
            struct EsC { float d2, x, y, z, r, h; }; EsC es[96]; int ne = 0;
            auto addEs = [&](double ex, double ey, double ez, float er, float eh) { const double dx = ex - cam[0], dz = ez - cam[2], d2 = dx * dx + dz * dz; if (d2 > 900.0 || ne >= 96 || eh <= 0.f) return; es[ne++] = {(float)d2, (float)ex, (float)ey, (float)ez, er, eh}; };
            if (gGm != 3 && !(gDead && !creative)) addEs(rX, rY, rZ, .30f, P.sneak ? 1.5f : 1.8f);
            for (const MobEnt& me : MOBS) { if (me.dead) continue; addEs(me.px + (me.x - me.px) * pt, me.py + (me.y - me.py) * pt, me.pz + (me.z - me.pz) * pt, mobW(me) * .5f, mobH(me)); }
            if (netOn) for (int rp = 0; rp < 2; rp++) for (const auto& kv : (rp ? remotesX : remotes)) { const RemotePl& rr = kv.second; if (!rr.init || rr.dim != gDim || rr.gm == 3 || rr.deathT >= 0) continue; addEs(rr.px + (rr.x - rr.px) * pt, rr.py + (rr.y - rr.py) * pt, rr.pz + (rr.z - rr.pz) * pt, .30f, rr.sneak ? 1.5f : 1.8f); }
            std::sort(es, es + ne, [](const EsC& a, const EsC& b) { return a.d2 < b.d2; });
            gEntShN = std::min(ne, 12); for (int ei = 0; ei < gEntShN; ei++) { gEntSh[ei][0] = es[ei].x; gEntSh[ei][1] = es[ei].y; gEntSh[ei][2] = es[ei].z; gEntSh[ei][3] = es[ei].r; gEntSh[ei][4] = es[ei].h; }
        }
        if (!creative && gHurt > 10 && !gDead) { float f = (gHurt - 10 - pt) / 10.f; if (f > 0.f) { f = sinf(f * f * f * f * 3.14159265f); rdRotatef(-gHurtAtYaw, 0, 1, 0); rdRotatef(-f * 14.f, 0, 0, 1); rdRotatef(gHurtAtYaw, 0, 1, 0); } }   // 1.8.9 hurtCameraEffect: a camera inclina para o lado do golpe
        if (gDead && !creative) rdRotatef(40.f - 8000.f / (gDeadT + pt + 200.f), 0, 0, 1);   // 1.8.9 hurtCameraEffect: camera tomba de lado ao morrer
        gBbYaw = camYaw; gBbPitch = camPitch;
        rdRotatef(-camPitch, 1, 0, 0); rdRotatef(-camYaw, 0, 1, 0); godCaptureCam();
        gSpecInside = gGm == 3 && solid(getB((int)floor(cam[0]), (int)floor(cam[1]), (int)floor(cam[2])));   // patchSpect.10
        const double r2a = perfNow();
        PROBE("pos-sombras");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        if (!headWater && !headLava && dimSky() && !diagOff("nosky")) drawSkyFull(skyTopD, skyHorD);   // domo + faixa do nascer/pôr + sol + lua + estrelas
        gR2[0] += perfNow() - r2a;
        rdTranslatef(-(float)cam[0], -(float)cam[1], -(float)cam[2]);
        rdGetMatrixf(GL_MODELVIEW_MATRIX, gViewM); gEntCam[0] = cam[0]; gEntCam[1] = cam[1]; gEntCam[2] = cam[2]; gCamW[0] = (float)cam[0]; gCamW[1] = (float)cam[1]; gCamW[2] = (float)cam[2]; gSkyCamW[0] = gCamW[0]; gSkyCamW[1] = gCamW[1]; gSkyCamW[2] = gCamW[2];
        { const double dEye[3] = {viewEye[0], viewEye[1], viewEye[2]}; dynLightUpdate(dEye, dir, sel, cam[0], cam[1], cam[2]); }   // luz dinamica (tocha na mao / no chao)
        rdEnable(GL_DEPTH_TEST); rdDepthFunc(GL_LEQUAL);
        rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f);   // (0,1 como o MC: com mipmap, 0,02 engordava as folhas ao longe) o HUD desliga o alpha test: religa para folhas/grama
        if (!gPerf && gsi("d8", 1) == 2 && !headWater && !headLava) rdDisable(GL_FOG); else rdEnable(GL_FOG);   // patchOpt.3: Nevoa OFF (modo desempenho: neblina sempre ligada)
        glHint(GL_FOG_HINT, GL_NICEST);
        rdFogi(GL_FOG_MODE, (headWater || headLava) ? (GLenum)0x0800 : GL_LINEAR); rdFogf((GLenum)0x0B62, headLava ? 2.f : headWater ? .1f : 1.f);   // patchJava.2: setupFog do 1.12: agua EXP .1, lava EXP 2.0 (o shader usa gl_Fog.density != 1 como sinal)
        { const float rf = gDim == 0 ? 1.f - .5f * gRain : gDim == 3 ? .55f : 1.f; /* patchFix.2: neblina mais densa na chuva */ rdFogf(GL_FOG_START, (headWater || headLava) ? 0.f : fogStartD() * rf); rdFogf(GL_FOG_END, headLava ? 2.5f : headWater ? 24.f : gDim == 1 ? std::max(24.f, std::min(fogEndD(), 192.f) * .5f) : std::max(24.f, fogEndD() * rf)); if (gDim == 1 && !headWater && !headLava) { float fe = 0; glGetFloatv(GL_FOG_END, &fe); rdFogf(GL_FOG_START, fe * .05f); } }   /* patchNether.1: nevoa do Nether como o MC 1.12 (inicio 5%, fim min(dist,192)/2): o terreno longe funde na cor da nevoa em vez de ficar preto */
        rdFogfv(GL_FOG_COLOR, skyF);
        if (gPerf && !headWater && !headLava) { const float fe = std::max(24.f, RAD * 16.f - 4.f); rdFogf(GL_FOG_START, fe * BL_RPI_FOG_START); rdFogf(GL_FOG_END, fe); }   // patchRpiOpt.8: neblina linear simples estilo Minecraft Alpha (comeca em 25% do alcance, termina na borda; o shader enxuto usa gl_Fog)
        glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D);
        glColor3f(1, 1, 1);
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        int pcx = fdiv((int)floor(cam[0]), CW), pcz = fdiv((int)floor(cam[2]), CW);
        const float fxv = -sy, fzv = -cy;
        struct GhostC { Chunk* c; float t0, x0, z0; }; static std::vector<GhostC> gGhosts;
        struct FadeC { Chunk* c; float f, x0, z0, dy; }; std::vector<FadeC> fadeList;
        const float nowS = SDL_GetTicks() * .001f;
        const float cullT = tanf(fov * .5f * .0174533f) * (w / (float)h) *
            (gPerf ? 1.0f : 1.08f)   // modo desempenho: culling de visao sem folga extra
            , cullK = sqrtf(1.f + cullT * cullT), cullFar = headWater ? 24.f : fogEndD();
        // Cache dos chunks visiveis (circulo de raio RAD, ordenado de perto p/ longe): so e refeito quando a camera troca de chunk ou o conjunto de
        // chunks muda. Antes eram 2 passes x (2R+1)^2 buscas no hash por quadro. NAO ha corte por angulo de visao: girar a camera nunca faz chunk sumir.
        static std::vector<StreamOff> discOffs; static int discR = -1;
        static std::vector<Chunk*> viewC; static std::vector<float> viewX, viewZ; static int viewPcx = 0x7fffffff, viewPcz = 0, viewR = -1; static unsigned viewEp = 0;
        if (discR != RAD) {
            discOffs.clear(); const float rm = RAD + .5f;
            for (int dz = -RAD - 1; dz <= RAD + 1; dz++) for (int dx = -RAD - 1; dx <= RAD + 1; dx++) { const int d2 = dx * dx + dz * dz; if ((float)d2 <= rm * rm) discOffs.push_back({dx, dz, d2, true, false}); }
            std::sort(discOffs.begin(), discOffs.end(), [](const StreamOff& a, const StreamOff& b) { return a.d2 < b.d2; }); discR = RAD;
        }
        PROBE("vis-ini");
        if (viewPcx != pcx || viewPcz != pcz || viewR != RAD || viewEp != gEpoch) {
            std::vector<Chunk*> oldV; std::vector<float> oldX, oldZ; oldV.swap(viewC); oldX.swap(viewX); oldZ.swap(viewZ);
            for (const StreamOff& o : discOffs) if (Chunk* c = getC(pcx + o.dx, pcz + o.dz)) { viewC.push_back(c); viewX.push_back((pcx + o.dx) * CW + 8.f); viewZ.push_back((pcz + o.dz) * CW + 8.f); }
            // patchChunkAnim.2: chunk que sai do alcance afunda (fantasma com a malha que ainda esta na GPU) e, ao voltar, anima de novo (born reiniciado)
            if (viewEp == gEpoch && viewEp != 0xffffffffu) {
                std::unordered_set<Chunk*> nw(viewC.begin(), viewC.end());
                gGhosts.erase(std::remove_if(gGhosts.begin(), gGhosts.end(), [&](const GhostC& g) { return nw.count(g.c) > 0; }), gGhosts.end());
                for (size_t q = 0; q < oldV.size(); q++) if (!nw.count(oldV[q])) {
                    Chunk* c = oldV[q]; c->born = -1.f; c->rborn = -1.f;

                    if (!gPerf && gAdaptCut < 2 && (c->nv > 0 || c->nwv > 0)) gGhosts.push_back({c, nowS, oldX[q] - 8.f, oldZ[q] - 8.f});
                }
            } else gGhosts.clear();
            viewPcx = pcx; viewPcz = pcz; viewR = RAD; viewEp = gEpoch;
        }
        PROBE("vis-fim");
        const float kChM = gPerf ? 0.f : 4.f, kSeM = gPerf ? 0.f : 3.f, kSeH = gPerf ? 16.f : 18.f;   // modo desempenho: culling exato (sem folga) por chunk e por fatia vertical de 16 blocos
        static const bool cullEnv = getenv("BLOCKLAND_NOCULL") == nullptr; gCullOn = cullEnv;   // culling de visao LIGADO por padrao (BLOCKLAND_NOCULL=1 desliga)
        Frustum frs; if (cullEnv) { float Pm[16], Mm[16]; rdGetMatrixf(GL_PROJECTION_MATRIX, Pm); rdGetMatrixf(GL_MODELVIEW_MATRIX, Mm); frustumFrom(frs, Pm, Mm); gFrs = frs; gFrsOn = true; } else gFrsOn = false;
        // patchGPU.1: culling de cavernas (BFS de secoes a partir da camera; conservador). BLOCKLAND_NOCAVE=1 desliga.
        static const bool caveEnv = getenv("BLOCKLAND_NOCAVE") == nullptr; gCaveOn = false;
        if (caveEnv && cullEnv && !gRfPass) {
            struct CvQ { Chunk* c; int cx, cz; int sec; int ent; }; static std::vector<CvQ> q; q.clear(); ++gVisStamp;
            const int ccx = (int)floor(cam[0] / 16.0), ccz = (int)floor(cam[2] / 16.0), csy = std::max(0, std::min(15, (int)floor(cam[1] / 16.0)));
            auto mark = [&](Chunk* c, int sec, int ent) { if (c->visStamp != gVisStamp) { c->visStamp = gVisStamp; c->visMask = 0; memset(c->entered, 0, sizeof c->entered); }
                if (c->entered[sec] & (1 << ent)) return false; c->entered[sec] |= (uint8_t)(1 << ent); c->visMask |= (uint16_t)(1u << sec); return true; };
            if (Chunk* c0 = getC(ccx, ccz)) {
                gCaveOn = true; mark(c0, csy, 6); q.push_back({c0, ccx, ccz, csy, 6});
                static const int DD[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
                for (size_t qi = 0; qi < q.size(); qi++) {
                    const CvQ cur = q[qi]; const uint16_t cn = cur.c->connOk ? cur.c->conn[cur.sec] : (uint16_t)0x7FFF;
                    for (int f = 0; f < 6; f++) {
                        if (cur.ent < 6 && (cur.ent == f || !((cn >> cvPair(cur.ent, f)) & 1))) continue;
                        const int ns = cur.sec + DD[f][1]; if (ns < 0 || ns > 15) continue;
                        const int ncx = cur.cx + DD[f][0], ncz = cur.cz + DD[f][2]; Chunk* nc = (DD[f][0] || DD[f][2]) ? getC(ncx, ncz) : cur.c; if (!nc) continue;
                        if (mark(nc, ns, f ^ 1)) q.push_back({nc, ncx, ncz, ns, f ^ 1});
                    }
                }
            }
        }
        // Desenho de um chunk. Com culling: so as secoes verticais de 16 blocos que tocam o frustum (a malha e ordenada por Y, entao secoes
        // visiveis vizinhas viram UMA chamada de desenho). A GPU deixa de processar vertices de subsolo/ceu que estao fora da tela.
        auto bindDraw = [&](Chunk* c, bool wpass, int cnt, float x0, float z0) {
            const uint32_t* off = wpass ? c->wOff : c->sOff;
            const bool farSkip = BL_FARSKIP && !wpass && !gRfPass && ((x0 + 8.0 - eyeX) * (x0 + 8.0 - eyeX) + (z0 + 8.0 - eyeZ) * (z0 + 8.0 - eyeZ)) > (double)gFarSkipD * gFarSkipD;   // patchFps.3/.4: LOD barato: >72 blocos nao desenha plantas/modelos (2o trecho da malha), so cubos
            // patchBFC.1: malha solida = [0,cubeN) cubos (culling de costas) + [cubeN,cnt) modelos de 2 faces (plantas, paineis; sem culling). Pi: ligado; BLOCKLAND_NOBFC=1 desliga; BLOCKLAND_BFC=1 liga nos outros
#if defined(BL_RPI) && !defined(__ANDROID__) && !defined(BL_VULKAN)
#define BL_BFC 1   // so GL "de verdade" (Pi/PC): os wrappers GLES nativo/Vulkan nao tem glCullFace
            static const bool bfcEnv = getenv("BLOCKLAND_NOBFC") == nullptr;
#elif !defined(__ANDROID__)
#define BL_BFC 1
            static const bool bfcEnv = BL_NOVK && getenv("BLOCKLAND_NOBFC") == nullptr;   // patchGPU.1: culling de costas LIGADO tambem no PC (BLOCKLAND_NOBFC=1 desliga)
#else
            static const bool bfcEnv = false;
#endif
            const bool bfc = bfcEnv && !gSpecInside && !wpass && !gRfPass && c->cubeN > 0 && c->cubeN <= cnt;   // reflexo da agua espelha a cena (inverte a ordem dos vertices): sem culling
            const int cubeCnt = wpass ? cnt : (c->cubeN > 0 && c->cubeN <= cnt ? c->cubeN : cnt);
            const bool secs = cullEnv && c->secOk && off[16] == (uint32_t)cubeCnt;
            const uint16_t vm = (gCaveOn && c->connOk && c->visStamp == gVisStamp) ? c->visMask : (uint16_t)0xFFFF;   // patchGPU.1: secoes alcancaveis pela vista
            if (gPiPhase == 2 && cubeCnt >= cnt) return;   // patchRpiPerf.4: passe dos recortados: este chunk nao tem
            if (useVBO) {
                if (c->gl || c->glw) uploadChunk(c);
                pBindBuffer(GL_ARRAY_BUFFER, wpass ? c->wvbo : c->vbo); bindQuadIB((size_t)cnt / 4);
            }
            auto range = [&](int a, int b) {   // vertices [a, b) da malha
                if (a < 0 || b <= a) return;   // a<0: nenhuma secao pendente (rs=-1); antes desenhava a partir do vertice -1 e embaralhava cor/posicao
                if (useVBO) {
                    const size_t o = (size_t)a * sizeof(V);
                    glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, x))); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, u))); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, r)));
                    quadDrawVBO(o, b - a, true);
                } else {
                    std::vector<V>& mm = wpass ? c->wmesh : c->mesh; const V* v = mm.data();
                    glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r);
                    glDrawArrays(GL_QUADS, a, b - a);
                }
                gPfCalls++; gPfVerts += b - a;
            };
#ifdef BL_BFC
            if (bfc && gPiPhase != 2) { rdEnable(GL_CULL_FACE); rdCullFace(GL_BACK); rdFrontFace(GL_CCW); }
#endif
            if (!secs) { if (gPiPhase != 2) range(0, cubeCnt);
#ifdef BL_BFC
                if (bfc) rdDisable(GL_CULL_FACE);
#endif
                if (cubeCnt < cnt && gPiPhase != 1 && !farSkip) range(cubeCnt, cnt); return; }
            // so as secoes verticais de 16 blocos que tocam o frustum; secoes visiveis vizinhas = uma chamada
            int rs = -1, re = 0;
            for (int sc = 0; sc < (gPiPhase == 2 ? 0 : 16); sc++) {
                const int a = (int)off[sc], b = (int)off[sc + 1]; if (b <= a) continue;
                if (!(vm >> sc & 1) || frustumOutside(frs, x0, sc * 16.f, z0, x0 + 16.f, sc * 16.f + kSeH, z0 + 16.f, kSeM)) { range(rs, re); rs = -1; continue; }
                if (rs >= 0 && re == a) re = b; else { range(rs, re); rs = a; re = b; }
            }
            range(rs, re);
#ifdef BL_BFC
            if (bfc) rdDisable(GL_CULL_FACE);
#endif
            if (cubeCnt < cnt && gPiPhase != 1 && !farSkip) {   // 2o trecho: modelos de 2 faces, mesmas secoes
                rs = -1; re = 0;
                for (int sc = 0; sc < 16; sc++) {
                    const int a = (int)c->xOff[sc], b = (int)c->xOff[sc + 1]; if (b <= a) continue;
                    if (!(vm >> sc & 1) || frustumOutside(frs, x0, sc * 16.f, z0, x0 + 16.f, sc * 16.f + kSeH, z0 + 16.f, kSeM)) { range(rs, re); rs = -1; continue; }
                    if (rs >= 0 && re == a) re = b; else { range(rs, re); rs = a; re = b; }
                }
                range(rs, re);
            }
        };
        static const bool chunkEnvOk = getenv("BLOCKLAND_NOCHUNKANIM") == nullptr; const bool chunkAnimOn = !gPerf && gAdaptCut < 2 && chunkEnvOk && gsi("d22", 1) != 0;   // patchRpiOpt.7: modo desempenho sem animacao/fade de chunks
        // patchChunkAnim.1: chunk novo sobe do subsolo (ease-out cubico, 1.1 s) + fade; dy<0 = deslocamento vertical atual (0 = parado)
        auto chunkRise = [&](const Chunk* c) -> float {
            if (true) return 0.f;   // patchChunkAnim.3: sem animacao de subida (so fade)
            if (!chunkAnimOn || gRfPass || c->born < 0.f) return 0.f;
            const float t = (nowS - c->born) / 1.1f; if (t >= 1.f) return 0.f;
            const float u = 1.f - std::max(t, 0.f); return -(u * u * u) * 40.f;
        };
        auto drawChunks = [&](bool wpass) {
            fadeList.clear(); const float cfD = gRfPass ? std::min(cullFar, ANDROID_RF_FAR) : cullFar; const float lim = (cfD + 11.5f) * (cfD + 11.5f);   // reflexo da agua: so ate 64 blocos
            const size_t nView = viewC.size();
#ifdef BL_RPI
            const int phLo = wpass ? 0 : 1, phHi = wpass ? 0 : 2;   // patchRpiPerf.4: todos os opacos (1 troca de programa), depois todos os recortados
#else
            const int phLo = 0, phHi = 0;
#endif
#if !defined(__ANDROID__) && !defined(BL_RPI)
            static const bool a2cEnv = getenv("BLOCKLAND_NOA2C") == nullptr;   // patchGfx.3: alpha-to-coverage (MSAA) so no passe solido: bordas de folhas/plantas/grades suaves e sem pisca a distancia, sem shader
            const bool a2c = BL_NOVK && a2cEnv && gAA > 0 && !wpass;
            if (a2c) rdEnable(0x809E);
#else
            const bool a2c = false;
#endif
            for (int ph = phLo; ph <= phHi; ph++) {
            gPiPhase = ph; if (ph == 1) rpiSolidProg(true); else if (ph == 2) rpiSolidProg(false);
            for (size_t k = 0; k < nView; k++) {
                const size_t i = wpass ? nView - 1 - k : k;   // solidos: perto -> longe (early-z); agua translucida: longe -> perto (mistura correta)
                Chunk* c = viewC[i];
                
                if (gPerf && c->born < 0.f) c->born = nowS - 100.f;   // patchRpiOpt.7: ja "nasceu" ha tempo = sem fade
                if (c->born < 0.f) c->born = nowS;   // fade-in conta desde que o chunk existe (nao desde que entra no campo de visao: senao chunks atras/ao lado ressurgem ao girar)
                const int cnt = wpass ? c->nwv - c->ngv : c->nv;   // agua = parte inicial de wmesh; o vidro vem depois (drawGlass)
                if (cnt <= 0) continue;
                const float vx = viewX[i] - (float)cam[0], vz = viewZ[i] - (float)cam[2];
                if (vx * vx + vz * vz > lim) continue;   // so descarta o que a nevoa ja esconde
                if (cullEnv && frustumOutside(frs, viewX[i] - 8.f, 0.f, viewZ[i] - 8.f, viewX[i] + 8.f, viewC[i]->maxY >= 0 ? std::min((float)CH, viewC[i]->maxY + 3.f) : (float)CH, viewZ[i] + 8.f, kChM)) continue;   // opcional: inteiramente fora da tela (margem 12 blocos)
                if (gRfPass && c->rborn < 0.f) c->rborn = nowS;   // patchRfl.1: o reflexo tem fade-in proprio (comeca quando o chunk aparece no reflexo, 1.6 s)
                float fade = gRfPass ? (nowS - std::max(c->born, c->rborn)) / 1.6f : (nowS - c->born) / (chunkAnimOn ? .7f : 1.2f);
                const float dy = chunkRise(c);
                if (fade < 1.f && false) {   // patchTerra.1: terreno sempre opaco (o fade por blend deixava o chao transparente enquanto o chunk "nascia")
                    fade = fade * fade * (3.f - 2.f * fade);
                    if (wpass) { if (fade < .6f && dy == 0.f) continue; }
                    else if (pBlendColor) { if (gPiPhase != 2) fadeList.push_back({c, fade, viewX[i] - 8.f, viewZ[i] - 8.f, dy}); continue; }
                }
                if (!wpass && gPiPhase != 2) gPfDrawn++;
                if (dy != 0.f) { rdPushMatrix(); rdTranslatef(0.f, dy, 0.f); }
                bindDraw(c, wpass, cnt, viewX[i] - 8.f, viewZ[i] - 8.f);
                if (dy != 0.f) rdPopMatrix();
            }
            }
            if (a2c) rdDisable(0x809E);
            gPiPhase = 0;
            if (chunkAnimOn && !gRfPass && !gGhosts.empty()) {   // chunks que sairam do alcance afundam (ease-in cubico, 1.1 s) e somem
                for (size_t q = 0; q < gGhosts.size();) {
                    GhostC& g = gGhosts[q]; const float t = (nowS - g.t0) / 1.1f;
                    if (t >= 1.f || (g.c->nv <= 0 && g.c->nwv <= 0)) { gGhosts.erase(gGhosts.begin() + q); continue; }
                    const int cnt = wpass ? g.c->nwv - g.c->ngv : g.c->nv; q++;
                    if (cnt <= 0) continue;
                    const float vx = g.x0 + 8.f - (float)cam[0], vz = g.z0 + 8.f - (float)cam[2]; if (vx * vx + vz * vz > lim) continue;
                    rdPushMatrix(); rdTranslatef(0.f, -(t * t * t) * 40.f, 0.f); bindDraw(g.c, wpass, cnt, g.x0, g.z0); rdPopMatrix();
                }
            }
            if (!wpass && !fadeList.empty()) {   // chunks novos: mistura com o fundo (alpha constante crescente)
                rdEnable(GL_BLEND); rdBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                for (auto& fc : fadeList) { pBlendColor(0.f, 0.f, 0.f, fc.f); gPfDrawn++; if (fc.dy != 0.f) { rdPushMatrix(); rdTranslatef(0.f, fc.dy, 0.f); } bindDraw(fc.c, false, fc.c->nv, fc.x0, fc.z0); if (fc.dy != 0.f) rdPopMatrix(); }
                rdDisable(GL_BLEND);
            }
        };
        auto drawGlass = [&]() {   // vidro colorido / gelo / slime: ultimos ngv vertices de wmesh; longe -> perto, blend, sem escrever profundidade
            const float lim = (cullFar + 11.5f) * (cullFar + 11.5f); const size_t nView = viewC.size();
            for (size_t k = 0; k < nView; k++) {
                const size_t i = nView - 1 - k; Chunk* c = viewC[i]; const int cnt = c->ngv; if (cnt <= 0) continue;
                const float vx = viewX[i] - (float)cam[0], vz = viewZ[i] - (float)cam[2]; if (vx * vx + vz * vz > lim) continue;
                if (cullEnv && frustumOutside(frs, viewX[i] - 8.f, 0.f, viewZ[i] - 8.f, viewX[i] + 8.f, viewC[i]->maxY >= 0 ? std::min((float)CH, viewC[i]->maxY + 3.f) : (float)CH, viewZ[i] + 8.f, kChM)) continue;
                const int tot = c->nwv, base = tot - cnt;
                const float gdy = chunkRise(c); if (gdy != 0.f) { rdPushMatrix(); rdTranslatef(0.f, gdy, 0.f); }
                if (useVBO) {
                    if (c->gl || c->glw) uploadChunk(c);
                    pBindBuffer(GL_ARRAY_BUFFER, c->wvbo); bindQuadIB((size_t)tot / 4);
                    const size_t o = (size_t)base * sizeof(V);
                    glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, x))); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, u))); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, r)));
                    quadDrawVBO(o, cnt, true);
                } else {
                    const V* v = c->wmesh.data() + base; glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r);
                    glDrawArrays(GL_QUADS, 0, cnt);
                }
                if (gdy != 0.f) rdPopMatrix();
                gPfCalls++; gPfVerts += cnt;
            }
        };
        gRfOn = false;
        PROBE("pre-reflexo");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        if (!BL_LITE && gShaderPackOn && !headWater && !headLava && !gDim && gShadowShader) {   // reflexao planar da agua (HL2): mundo espelhado em meia resolucao
            float wy = 0.f;
            if (rfFindWater(cam[0], cam[1], cam[2], wy) && cam[1] > wy + 0.05) {
                GLint vp0[4]; rdGetViewport(vp0);
#ifdef __ANDROID__
                const int rdiv = 4;
#else
                const int rdiv = 2;
#endif
                const int rw = std::max(vp0[2] / rdiv, 64), rh = std::max(vp0[3] / rdiv, 64);
                if (rfEnsure(rw, rh)) {
                    gRfSW = vp0[2]; gRfSH = vp0[3]; gRfWy = wy;
                    static int rfFr = 0; bool rfSkip = false;
#ifdef __ANDROID__
                    rfSkip = (rfFr++ % 3) != 0;   // celular: refaz o reflexo a cada 3 quadros (reusa a textura anterior)
#endif
                    if (rfSkip) { pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, gRfTex); pActTex(GL_TEXTURE0); gRfOn = true; } else {
                    glPushAttrib(GL_ALL_ATTRIB_BITS); glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);   // a passada do reflexo nao pode vazar estado para o mundo
                    pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, 0); pActTex(GL_TEXTURE0);   // sem laco de feedback: a textura do reflexo nao fica ligada enquanto e o alvo
                    const bool fgW = rdIsEnabled(GL_FOG), atW = rdIsEnabled(GL_ALPHA_TEST);
                    pBindFB(GL_FRAMEBUFFER, gRfFbo); rdViewport(0, 0, rw, rh);
                    glClearColor(sky[0], sky[1], sky[2], 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    rdMatrixMode(GL_MODELVIEW); rdPushMatrix();
                    float Ms[16]; for (int q = 0; q < 16; q++) Ms[q] = gViewM[q]; Ms[12] = Ms[13] = Ms[14] = 0.f;
                    rdLoadMatrixf(Ms); rdScalef(1.f, -1.f, 1.f); if (!headWater && !headLava && !gDim) drawSkyFull(skyTopD, skyHorD);   // ceu/sol espelhados
                    rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST); rdDepthMask(GL_TRUE); rdDepthFunc(GL_LEQUAL); rdEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
                    if (fgW) rdEnable(GL_FOG); if (atW) { rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f); }
                    rdLoadMatrixf(gViewM); rdTranslatef(0.f, wy, 0.f); rdScalef(1.f, -1.f, 1.f); rdTranslatef(0.f, -wy, 0.f);
                    const double pl[4] = {0.0, 1.0, 0.0, -(double)wy - 0.02}; glClipPlane(GL_CLIP_PLANE0, pl); rdEnable(GL_CLIP_PLANE0);   // so o que esta acima da agua
                    if (cullEnv) { float Pm2[16], Mm2[16]; rdGetMatrixf(GL_PROJECTION_MATRIX, Pm2); rdGetMatrixf(GL_MODELVIEW_MATRIX, Mm2); frustumFrom(frs, Pm2, Mm2); gFrs = frs; }
                    const bool sdw = gShadowDraw; gShadowDraw = false; gRfPass = true;
                    skyShaderOn(0.f, gSkyScale, gSkyRB); drawChunks(false); skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    {   // o proprio personagem aparece no reflexo (tambem em 1a pessoa)
                        Pose p; p.headYaw = wrap180(renderYaw - renderBodyYaw); p.headPitch = renderPitch;
                        p.limb = renderLimb; p.amp = renderLimbAmt; p.atk = swinging ? swing : 0; p.held = held; p.heldEnch = gEnchHeld && enchAny(*gEnchHeld); p.held2 = gGm != 3 ? offh.id : 0; p.bow = gBowOn ? (float)gBowT : -1.f; p.eat = gEatOn ? (float)gEatT : -1.f;
                        p.age = ageTicks + pt; p.sneak = P.sneak && !gBoatRide && !gCartRide && !gVehRide && !gSeated && !gRideUid; p.sit = gBoatRide != 0 || gCartRide != 0 || gVehRide != 0 || gSeated || gRideUid != 0; p.sleep = gSleepT > 0; if (gSeated) p.headYaw = std::max(-105.f, std::min(105.f, p.headYaw)); wornFromInv(p); elytraPoseFill(p, P, pt, renderYaw, renderPitch);
                        if (const BoatEnt* rbr = boatById(gBoatRide)) boatRowPose(p, *rbr, pt, !gBoatPaxSelf);   // patchBoat2.1: reflexo da agua tambem remando
                        rdPushMatrix(); if (const BoatEnt* rbt = boatById(gBoatRide)) { const double sp[3] = {rX, rY, rZ}; boatRiderBegin(*rbt, pt, sp); } else rdTranslatef((float)rX, (float)rY, (float)rZ); rdRotatef(renderBodyYaw, 0, 1, 0); localDeathPose(pt);
                        entLightAt(rX, rY + 1.0, rZ, gEntDim); entShaderOn(); p.hurt = (gHurt > 10 || (gDead && !creative)) ? 1.f : 0.f; p.hide = fxOn(FX_INVIS); drawPlayer(p); if (gFire > 0 && !creative && !gDead) drawFireWH(.6f, 1.8f, renderBodyYaw);   // chamas no proprio modelo (3a pessoa/espelho)
                    skyShaderOff(); entDimReset(); rdPopMatrix();   // patchShader.6: isto estava DENTRO do comentario acima (sem pop = pilha vazava 1 por quadro)
                        if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    }
                    gShadowDraw = sdw; gRfPass = false; rdDisable(GL_CLIP_PLANE0);
                    rdPopMatrix(); pBindFB(GL_FRAMEBUFFER, 0); rdViewport(vp0[0], vp0[1], vp0[2], vp0[3]);
                    glPopClientAttrib(); glPopAttrib(); rdMatrixMode(GL_MODELVIEW); pActTex(GL_TEXTURE0); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    if (cullEnv) { float Pm2[16], Mm2[16]; rdGetMatrixf(GL_PROJECTION_MATRIX, Pm2); rdGetMatrixf(GL_MODELVIEW_MATRIX, Mm2); frustumFrom(frs, Pm2, Mm2); gFrs = frs; }
                    pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, gRfTex); pActTex(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);
                    if (!fgW) rdDisable(GL_FOG); if (!atW) rdDisable(GL_ALPHA_TEST); glClearColor(sky[0], sky[1], sky[2], 1);
                    gRfOn = true; glStackHeal("reflexo da agua");
                    }
                }
            }
        }
        torchWorldUpdate();   // tocha animada (mundo e item no chao)
        netherAnimUpdate();   // fogo e portal animados (nether.inc)
        clockUpdate(gWorldTime, gDim);   // patchClock.1
        compassUpdate(P.x, P.z, yaw);   // agulha da bussola (single e multiplayer: usa so posicao/yaw locais e o spawn fixo)
        PROBE("pre-solido");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        skyShaderOn(0.f, gSkyScale, gSkyRB); { PerfDrawScope perfDs(PS_DRAW_SOLID); if (!diagOff("nochunks")) drawChunks(false); } gShadowDraw = false;
        if (!BOATS.empty() || !CARTS.empty() || !VEHS.empty() || !gSfPos.empty() || !gPAnim.empty()) {   // botes e carrinhos: antes da água (a placa "noWater" esconde a água dentro do casco); estado fixo, sem shader/arrays
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            rdDisable(GL_BLEND); drawBoats(pt); drawCarts(pt); drawVehicles(pt); drawProps(pt, P.x, P.z, 0); signDrawWorld(P.x, P.y, P.z); glBindTexture(GL_TEXTURE_2D, atlasTex); drawSunflowers(pt, P.x, P.z); drawPistonAnims(); drawFalling(pt); drawEndPortals(pt, P.x, P.z);   // bote/carrinho deixam a textura deles ligada: o girassol usa o atlas
            glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);   // drawBoats deixava a textura do bote ligada e o alpha test desligado: a água era desenhada com a textura errada
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        {   // entidades opacas ANTES da agua/vidro/portal (eles nao escrevem profundidade: se viessem depois, cobriam a transparencia com a cor cheia)
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            rdDisable(GL_BLEND); glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D);
            if (!gEntShOn && camMode != 0 && !headWater && gGm != 3 && !(gDead && !creative)) entBlobShadow(rX, rY, rZ, .5f, rX, rY + 1.62, rZ);   // patchSombra.1: sombra sob os pes (sem shader)
            for (const MobEnt& me : MOBS) { if (gEntShOn || me.dead) continue; const double ddx = me.x - rX, ddz = me.z - rZ; if (ddx * ddx + ddz * ddz > 256.0) continue; const double mx = me.px + (me.x - me.px) * pt, my = me.py + (me.y - me.py) * pt, mz = me.pz + (me.z - me.pz) * pt; entBlobShadow(mx, my, mz, mobShadowSize(me.type), rX, rY + 1.62, rZ); }
            if (netOn) for (int rp = 0; rp < 2; rp++) for (const auto& kv : (rp ? remotesX : remotes)) { const RemotePl& r = kv.second; if (gEntShOn || !r.init || r.dim != gDim || r.gm == 3 || r.deathT >= 0) continue; entBlobShadow(r.px + (r.x - r.px) * pt, r.py + (r.y - r.py) * pt, r.pz + (r.z - r.pz) * pt, .5f, rX, rY + 1.62, rZ); }
            if (dual && !headWater) for (int xk = 0; xk < gSplitN - 1; xk++) { const SplitP& o = gSX[xk]; if (gEntShOn || !o.init || o.sv.dead || gGm == 3) continue; entBlobShadow(o.P.px + (o.P.x - o.P.px) * pt, o.P.py + (o.P.y - o.P.py) * pt, o.P.pz + (o.P.z - o.P.pz) * pt, .5f, rX, rY + 1.62, rZ); }   // patchSplit.4: sombra sob os pes dos outros jogadores do split (sem shader)
            torchFootShadow(rX, rY, rZ);
            { int nsh = 0; for (const MobEnt& me : MOBS) { if (me.dead || nsh >= 6) continue; const double ddx = me.x - rX, ddz = me.z - rZ; if (ddx * ddx + ddz * ddz > 400.0) continue; torchFootShadow(me.x, me.y, me.z, std::max(.5f, mobW(me) / .6f), mobH(me)); nsh++; } }   // patchEmber.1: mobs (6 mais proximos, ate 20 blocos)
            if (gDim == 1 && gNetherFx) netherEmbers(rX, rY, rZ, SDL_GetTicks() * .001f);
            if (gsi("d3", 0) != 3) wxDraw(rX, rY, rZ, SDL_GetTicks() * .001f);   // patchWx.1
            const double r2b = perfNow();
            gFireCamYaw = camYaw; if (!diagOff("nodrops")) { drawDrops(pt); drawPickups(pt, rX, rY, rZ); } netDraw(pt); drawMobs(pt); drawDragon(pt); crystalsDraw(pt); arrowsDraw(pt); fishingDraw(pt, P, yaw, pitch); eggsDraw(pt); fwDraw(pt); fireballsDraw(pt); tntDraw(pt);
            gR2[1] += perfNow() - r2b;
        if (dual) for (int xk = 0; xk < gSplitN - 1; xk++) { const SplitP& o = gSX[xk]; if (!o.init) continue;   // os OUTROS jogadores (na passada do extra k, gSX[k] guarda o jogador 1)
            SplitPoseD q = split2PoseOf(o.P, o.yaw, o.pitch, o.prevBodyYaw, o.bodyYaw, o.limbSwing, o.limbAmt, o.prevLimbAmt, o.swinging, o.swing, o.inv[o.sel].id, o.ageT, pt);
            q.boat = gBoatX[xk]; /* patchSplitBoat.1: J2..J4 (na passada do extra k, gBoatX[k] guarda o bote do J1) */ q.fire = o.sv.fire > 0 && !creative && !o.sv.dead && gGm != 3; { const GLuint ct = gGm == 3 ? 0 : (vp > 0 && xk == vp - 1) ? gCapeMine.texAt(SDL_GetTicks()) : split2CapeTex(gSX[xk]); split2DrawBody(q, o.skinTex, false, ct, (float)o.P.my); } }
        const bool fpBoatBody = !camMode && gBoatRide != 0 && gGm != 3;
        if (camMode || fpBoatBody) {   // personagem em 3ª pessoa (ou corpo sem cabeca ao remar em 1a pessoa)
            Pose p; p.noHead = fpBoatBody; p.headYaw = wrap180(renderYaw - renderBodyYaw); p.headPitch = renderPitch;
            p.limb = renderLimb; p.amp = renderLimbAmt; p.atk = swinging ? swing : 0; p.held = held; p.heldEnch = gEnchHeld && enchAny(*gEnchHeld); p.held2 = gGm != 3 ? offh.id : 0; p.bow = gBowOn ? (float)gBowT : -1.f; p.eat = gEatOn ? (float)gEatT : -1.f;
            p.age = ageTicks + pt; p.sneak = P.sneak && !gBoatRide && !gCartRide && !gVehRide && !gSeated && !gRideUid; p.sit = gBoatRide != 0 || gCartRide != 0 || gVehRide != 0 || gSeated || gRideUid != 0; p.sleep = gSleepT > 0; if (gSeated) p.headYaw = std::max(-105.f, std::min(105.f, p.headYaw)); wornFromInv(p); elytraPoseFill(p, P, pt, renderYaw, renderPitch);
            if (const BoatEnt* rb = boatById(gBoatRide)) boatRowPose(p, *rb, pt, !gBoatPaxSelf);   // remar: fase do remo
            rdPushMatrix(); if (const BoatEnt* rbt = boatById(gBoatRide)) { const double sp[3] = {rX, rY, rZ}; boatRiderBegin(*rbt, pt, sp); } else rdTranslatef((float)rX, (float)rY, (float)rZ); rdRotatef(renderBodyYaw, 0, 1, 0); localDeathPose(pt);
            entLightAt(rX, rY + 1.0, rZ, gEntDim);
            entShaderOn();
            p.hurt = (gHurt > 10 || (gDead && !creative)) ? 1.f : 0.f; p.hide = fxOn(FX_INVIS); if (gGm == 3) { rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDepthMask(GL_FALSE); glColor4f(1.f, 1.f, 1.f, .15f); }   /* patchSpect.9: espectador em F5 = fantasma translucido (alfa .15, como o RenderLivingBase do 1.12) */ drawPlayer(p); if (gGm == 3) { glColor4f(1.f, 1.f, 1.f, 1.f); rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); } if (gFire > 0 && !creative && !gDead && gGm != 3) drawFireWH(.6f, 1.8f, renderBodyYaw);   // chamas no proprio modelo (3a pessoa/espelho)
            skyShaderOff();
            entDimReset();
            rdPopMatrix();
        }
            glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        PROBE("pos-solido");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        if (!headWater && !headLava && dimSky()) {   // nuvens ANTES dos translucidos (agua/vidro): senao apareciam por cima do vidro, que nao escreve profundidade
            const GLboolean fgC = rdIsEnabled(GL_FOG);
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            if (gsi("d0", BL_CLOUD_DEF) != 3) drawClouds(cam[0], cam[1], cam[2], nowS);   // patchOpt.1: Nuvens OFF
            if (!fgC) rdDisable(GL_FOG);
            glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        if (!gProps.empty()) {   // patchProp.4: vidro dos props DEPOIS das nuvens (como o vidro de bloco), senao a nuvem aparecia por cima dele
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            drawProps(pt, P.x, P.z, 1);
            glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        skyShaderOn(1.f, gSkyScale, gSkyRB); rdDepthMask(GL_FALSE);
        rdEnable(GL_STENCIL_TEST); glStencilFunc(GL_NOTEQUAL, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);   // água não é desenhada onde a máscara do bote marcou
        PROBE("pre-agua");   // patchPerf.7: localiza travadas de CPU entre antes-fluidos e antes-hud
        { PerfDrawScope perfDw(PS_DRAW_WATER); drawChunks(true); } rdDisable(GL_STENCIL_TEST);
        skyShaderOff(); skyShaderOn(0.f, gSkyScale, gSkyRB); drawGlass(); skyShaderOff(); rdDepthMask(GL_TRUE);   // vidro colorido por cima da agua (shader de bloco, nao o da agua)
        if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
        rdDisable(GL_BLEND);
        glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
        glBindTexture(GL_TEXTURE_2D, atlasTex); rdEnable(GL_TEXTURE_2D);
        { const int pi = gProps.empty() ? -1 : propPick(eye, dir, creative ? 6.f : 3.f); if (pi >= 0 && (!gRay.hit || !has || propPick(eye, dir, (float)gRay.t) == pi)) wireBox(gProps[pi].box); else   // patchProp.2: contorno do prop mirado (so se mais perto que o bloco)
        if (has) {   // contorno da caixa de selecao real; gRay so vale se for a mesma celula mirada (o toque pode ter refeito o raio)
            if (gRay.hit && gRay.x == hh[0] && gRay.y == hh[1] && gRay.z == hh[2]) { if (!wireShape(hh[0], hh[1], hh[2])) wireBox(gRay.box); }
            else wireBox(AABB{(double)hh[0], (double)hh[1], (double)hh[2], hh[0] + 1.0, hh[1] + 1.0, hh[2] + 1.0});
        } }
        if (mnOn && mnNeed > 0.f) { drawCrack(mnX, mnY, mnZ, std::min(9, (int)(mnT / mnNeed * 10.f))); glBindTexture(GL_TEXTURE_2D, atlasTex); }
        drawParts(camYaw, camPitch);
        gNetTagYaw = camYaw; gNetTagPitch = camPitch; gNetTagOn = true; netDrawTags(pt);
        if (dual) for (int xk = 0; xk < gSplitN - 1; xk++) { const SplitP& o = gSX[xk]; if (!o.init || (vp >= 1 && xk == vp - 1 && false)) continue;
            netDrawTagLocal((vp >= 1 && xk == vp - 1) ? netName() : splitNick(xk), o.P.px + (o.P.x - o.P.px) * pt, o.P.py + (o.P.y - o.P.py) * pt, o.P.pz + (o.P.z - o.P.pz) * pt, o.P.sneak, false); }
        gNetTagOn = false;   // nametags depois do mundo opaco (translucidas, sem escrever profundidade)
        modulesDrawWorld(pt);
        if (!gPerf && gsi("d21", 1) != 0 && !headWater && !headLava && !gDim) drawGodRays();   // patchRpiOpt.5: modo desempenho sem god rays
        //   // raios de luz: profundidade copiada ANTES do jogador/mao, igual em 1a e 3a pessoa
        if (!camMode) {         // braço / item em 1ª pessoa
            rdDisable(GL_FOG);
            {   // patchHandFov.1: a mao/item tem projecao PROPRIA com FOV fixo (EntityRenderer.renderHand = getFOVModifier(pt, false) = 70, 60 na agua): o slider de FOV, o aumento ao correr e o zoom (C) nao deformam mais a mao
                const float hfov = headWater ? 60.f : 70.f, hn = (RAD > 8 && !headWater) ? .09f : .05f, ht = hn * tanf(hfov * .5f * .0174533f), hr = ht * w / (float)h;
                rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdLoadIdentity(); rdFrustum(-hr, hr, -ht, ht, hn, 700);
            }
            rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
            applyViewBob(P.distWalked, P.prevDistWalked, camBobYaw, prevCamBobYaw, camBobPitch, prevCamBobPitch, pt);
            entLightAt(eyeX, eyeY, eyeZ, gEntDim);   // a mão também escurece à noite / em cavernas
            if (gGm != 3 && !hideHud) drawFireOverlay();   // jogador em chamas: fogo na tela (patch5.61)
            auto rowHandFp = [&](int side) -> bool {   // patchBoat2.1: 1a pessoa no bote: a mao do lado que esta remando fica SEM item e segue o cabo do remo (mesma fase/formula do boatRowPose)
                const BoatEnt* rbf = boatById(gBoatRide); if (!rbf || gBoatPaxSelf) return false;
                const int k = side > 0 ? 1 : 0; const float a = std::min(1.f, rbf->paddleAmp[k]); if (a < .02f) return false;
                const float rt = rbf->paddlePos[k] + .3926991f * rbf->paddleAmp[k] * (pt - 1.f), sweep = sinf(1.f - rt), deep = -sinf(-rt);   // sweep>0: cabo para a popa (mao vem ao corpo); deep=1: pa funda (cabo sobe)
                rdPushMatrix(); handBobApply(yaw, pitch, pt);
                rdTranslatef(-.06f * side * a, (.05f * deep - .05f) * a, (.2f * sweep - .06f) * a); rdRotatef(sweep * 16.f * a, 1, 0, 0); rdRotatef(-side * (.08f + .1f * sweep) * 57.3f * a, 0, 1, 0);
                drawHand(0, 0.f, equip, side); rdPopMatrix(); return true; };
            const bool fpBody = gBoatRide != 0 && gGm != 3;   // patchBoat3.4: no bote em 1a pessoa aparece o CORPO (bracos remando) em vez das maos soltas
            const bool rowR = !fpBody && gGm != 3 && !hideHud && rowHandFp(1), rowL = !fpBody && gGm != 3 && !hideHud && rowHandFp(-1);
            if (gGm != 3 && !hideHud && gSleepT <= 0 && !rowR && !fpBody && !diagOff("nohand")) { rdPushMatrix(); handBobApply(yaw, pitch, pt); drawHand(bowPullItem(handItem), swinging ? swing : 0, equip); rdPopMatrix(); }
            if (gGm != 3 && !hideHud && gSleepT <= 0 && offh.id && !rowL && !fpBody && !diagOff("nohand")) { rdPushMatrix(); handBobApply(yaw, pitch, pt); drawHand(offh.id, 0.f, 1.f, -1); rdPopMatrix(); }   // mao esquerda (secundaria)
            entDimReset();
            rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW);   // patchHandFov.1: volta a projecao do mundo
        }
        drawNightOverlay();   // só sem shader: escurecimento simples da cena
        glBindTexture(GL_TEXTURE_2D, atlasTex);
        sanitizeInv();
        gNoCross = touchHud;
        
#ifdef BL_RPI
        gInvTouch = true;   // patchRpiOpt.7: Pi usa o layout de inventario mobile
        gInvPad = gpCount() > 0 || gpCurT != 0 && SDL_GetTicks() - gpCurT < 4000;   // patchRpi.3: com controle o inventario usa cursor (pega/solta como no Xbox 360), so o layout e o mobile
#else
        gInvTouch = touchHud;
#endif
        //
        const bool hudOff = (hideHud || gSleepT > 0) && !invOpen && !pauseOpen && !chatOpen;   // patchSleep.4: deitado nao mostra HUD (hotbar, mira, barras)
        { const float bk = (1.f - gsf("v8", 1.f)) * .55f; if (bk > .004f) { rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdPushMatrix(); rdLoadIdentity(); rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_TEXTURE_2D); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0, .02f, bk); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd(); rdEnable(GL_TEXTURE_2D); rdEnable(GL_DEPTH_TEST); rdPopMatrix(); rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW); } }   // patchOpt.1: Brilho abaixo de Claro escurece o mundo (antes do HUD)
        if (gGm != 3 && !P.noclip) { const int hx = (int)floor(eyeX), hy = (int)floor(eyeY), hz = (int)floor(eyeZ), hb = getB(hx, hy, hz);   // patchFix.25 (BUG-067): cabeca dentro de bloco opaco (soterrado): tela escura cheia, como o overlay do MC; antes a face de dentro era cortada e se via o mundo atraves do bloco (xray)
            if (hb != AIR && !isLiquid(hb) && solid(hb) && lightOpacL(hb) >= 15) { rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdPushMatrix(); rdLoadIdentity(); rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_TEXTURE_2D); rdDisable(GL_ALPHA_TEST); rdDisable(GL_BLEND); glColor4f(.04f, .03f, .025f, 1.f); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd(); rdEnable(GL_TEXTURE_2D); rdEnable(GL_DEPTH_TEST); rdPopMatrix(); rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW); glColor4f(1, 1, 1, 1); } }
        const double r2c = perfNow();
        PROBE("antes-hud");
        if (!hudOff && !(pauseOpen && !gPage.empty())) drawHud(w, h, sel, invOpen, mouseX, mouseY, itemName(held), nameT, msg.c_str(), msgT); if (!hudOff && !invOpen && !pauseOpen && !chatOpen) mapDraw(w, h, P, inv[sel].id, yaw);   // BUG-048
        gR2[2] += perfNow() - r2c;
        iconFilterRestore();
        if (touchHud && !invOpen) drawTouch(w, h, hudOff);   // F1: só o botão F1 (fraco)
        if (!hudOff) { drawChat(w, h, !dual || vp == (gChatAs2 ? 1 + gChatK : 0)); if (vp == vpBeg) { drawTabList(w, h); drawGmSwitcher(w, h); } signDrawUI(w, h); gModHudBlocked = invOpen || pauseOpen || chatOpen; modulesDrawHud(w, h); }
        if (!hudOff && gSplit) drawSplitNick(w, h, vp >= 1 ? splitNick(vp - 1) : netName());   // nick do jogador de cada viewport
        if (dual && pauseOpen) { if (gPage.empty()) drawPauseMenu(w, h, mouseX, mouseY); else drawPage(w, h, true); }   // pausa so no viewport do jogador 1 (os outros seguem jogando)
        else if (dual && vp == 0 && gDead && !creative) drawDeathScreen(w, h, mouseX, mouseY);
        if (touchHud && (invOpen || chatOpen || gSleepT > 0)) drawTouchClose(w, h);
        if (dbg && vp == vpBeg) {   // dados do jogador deste viewport; desenhado depois do laco (por cima de tudo)
            DbgInfo d; d.x = rX; d.y = rY; d.z = rZ; d.speed = sqrt(P.mx * P.mx + P.mz * P.mz) * 20.0; d.yaw = yaw; d.pitch = pitch;
            d.hasT = has; d.tx = hh[0]; d.ty = hh[1]; d.tz = hh[2]; d.tid = has ? getB(hh[0], hh[1], hh[2]) : 0;
            d.flying = P.flying; d.sprint = P.sprint; d.sneak = P.sneak; d.ground = P.onGround; d.water = P.inWater; d.camMode = camMode; d.ww = fw; d.wh = fh;
            dbgD = d; dbgHave = true;
        }
        if (vp >= 1) { swapView(); gS2i = 0; viewEye[0] = vwP1[0]; viewEye[1] = vwP1[1]; viewEye[2] = vwP1[2]; }
        if (ownSw) split2SwapInvOwner();
        invOpen = kInv; pauseOpen = kPause; gSignEd = kSign;   // devolve o contexto do jogador 1 (e os olhos dele: antes o raio do J1 saia da posicao do J2)
        }   // fim do laco de viewports
        gVpX = gVpY = 0;
        const int w = fw, h = fh;
        if (dual) {   // divisorias pretas entre os viewports (2, 3 ou 4)
            rdViewport(0, 0, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
            rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_TEXTURE_2D); glColor4f(0, 0, 0, 1);
            glBegin(GL_QUADS);
            for (int dv = 0; dv < gSplitN; dv++) { int rx, ry, rw, rh; split2Rect(dv, w, h, rx, ry, rw, rh); const int ty = h - (ry + rh);   // ry: origem embaixo; ty: origem no topo
                if (rx + rw < w) { glVertex2f((float)(rx + rw - 1), (float)ty); glVertex2f((float)(rx + rw + 1), (float)ty); glVertex2f((float)(rx + rw + 1), (float)(ty + rh)); glVertex2f((float)(rx + rw - 1), (float)(ty + rh)); }
                if (ry > 0) { glVertex2f((float)rx, (float)(ty + rh - 1)); glVertex2f((float)(rx + rw), (float)(ty + rh - 1)); glVertex2f((float)(rx + rw), (float)(ty + rh + 1)); glVertex2f((float)rx, (float)(ty + rh + 1)); } }
            glEnd();
            rdEnable(GL_TEXTURE_2D); rdEnable(GL_DEPTH_TEST);
        }
        if (!dual && pauseOpen) { if (gPage.empty()) drawPauseMenu(w, h, mouseX, mouseY); else drawPage(w, h, true); }
        else if (!dual && gDead && !creative) drawDeathScreen(w, h, mouseX, mouseY);
        if (gCredits) drawCredits(w, h);   // patchEnd.14
        if (dbgHave) drawDebug(dbgD, w, h); else if (gShowFps) drawFpsMini(w, h);   // patchCfg.7
        if (chatOpen && oskActive()) { int ow = w, oh = h; if (gSplit) { int rx, ry, rw, rh; split2Rect(gChatAs2 ? 1 + gChatK : 0, w, h, rx, ry, rw, rh); rdViewport(rx, ry, rw, rh); ow = rw; oh = rh; }   // split: o teclado fica no viewport de quem abriu o chat
          uiOrtho(ow, oh); G = guiScaleFor(ow, oh); OX = OY = 0; oskDraw((ow + G - 1) / G, (oh + G - 1) / G, mouseX / G, mouseY / G); if (gSplit) rdViewport(0, 0, w, h); rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST); }   // teclado virtual do chat (controle)
        if (fading) { const float fa = 1.f - (SDL_GetTicks() - fadeT0) / 250.f; if (fa <= 0.f) fading = false; else drawLoad(w, h, 1.f, true, fa); }   // fade out do loading = fade in do HUD/mundo
        if (gSleepA > 0.f) drawFadeBlack(w, h, gSleepA * .86f);   // patchProp.11b: tela escurece enquanto dorme
        if (quitPhase) {   // fade out do jogo ate o preto; ao fechar salva e volta ao titulo (que entra com fade in)
            const float qa = (SDL_GetTicks() - quitT0) / 120.f; drawFadeBlack(w, h, qa);
            if (qa >= 1.f) { quitPhase = false; saveCurWorld(P.x, P.y, P.z); netStop(); gHostPending = false; setPause(false); resetWorldData(); gCurDir.clear(); panoOn = false; gScreen = SC_TITLE; titleQuick = true; grab = false; relMouse(false); fading = false; }
        }
        if (gpUiWas && gpCount() && (gUiSubOn || SDL_GetTicks() - gpCurT < 4000)) {   // patchPad.1: cursor do controle (pixel art cursor_pad.png), dentro do viewport do jogador do controle
            static GLuint cpTex = 0; static int cpW = 27, cpH = 27; static bool cpTry = false;
            if (!cpTry) { cpTry = true; std::vector<uint8_t> px; int tw = 0, th = 0; if (loadPNG("assets/textures/gui/cursor_pad.png", px, tw, th)) { cpW = tw; cpH = th; glGenTextures(1, &cpTex); glBindTexture(GL_TEXTURE_2D, cpTex); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); } }
            if (cpTex) {
                int ww = w, wh = h; rdViewport(0, 0, ww, wh); rdDisable(GL_SCISSOR_TEST); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, ww, wh, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
                rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, cpTex);
                const float sc = (float)std::max(1, G * 2 / 3), cx0 = (float)(gUiSubOn ? gUiOx : 0) + gpCurX, cy0 = (float)(gUiSubOn ? gUiOy : 0) + gpCurY;   // centro do cursor = ponto do clique
                glColor4f(1, 1, 1, 1); glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(cx0 - cpW * sc / 2, cy0 - cpH * sc / 2); glTexCoord2f(1, 0); glVertex2f(cx0 + cpW * sc / 2, cy0 - cpH * sc / 2); glTexCoord2f(1, 1); glVertex2f(cx0 + cpW * sc / 2, cy0 + cpH * sc / 2); glTexCoord2f(0, 1); glVertex2f(cx0 - cpW * sc / 2, cy0 + cpH * sc / 2); glEnd();
                glColor4f(1, 1, 1, 1);
            }
        }
        splashOverlay(w, h); glStackHeal("fim do quadro"); logGlErrors("Post render");
        ctlGrab(w, h);
        PROBE("antes-swap");
        const Uint64 swT0 = SDL_GetPerformanceCounter();
        if (!gSwapSkip || SDL_GetTicks() - gSwapT >= gSwapGapMs(win)) {   // patchPerf.3: sem foco troca a cada 3 s (host simula ~80% do tempo real em vez de ~60%)
             PerfScope perfSw(PS_SWAP); SDL_GL_SwapWindow(win); vkBootOk(); } else if (!gSoftPace) gSwapSkip = false;
#if !defined(__ANDROID__) && !defined(BL_RPI)
        // patchPerf.1: janela em 2o plano/oculta (varias instancias no mesmo PC) e estrangulada pelo compositor a 1 Hz (swap = 1000 ms exatos) => host em camera lenta.
        // 3 swaps lentos seguidos com vsync ligado: troca para swap 0 + limitador proprio (60 FPS em foco, 30 sem foco). /vsync 1 manual desativa a deteccao.
        // patchPerf.2: o /vsync 1 (sugerido pela dica) desligava a deteccao e prendia a janela em 1 Hz (swap = 1000 ms). Agora a deteccao vale sempre:
        // 3 swaps lentos => swap 0 + limitador proprio; se mesmo assim o swap trava (>=400 ms x3), pula o SwapWindow (so troca a cada 1 s) e a simulacao segue fluida.
        if (BL_NOVK) { static int slowSw = 0, slowSw0 = 0; const double swMs = (double)(SDL_GetPerformanceCounter() - swT0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
          const double lim = gVsyncUser ? 400.0 : 120.0;   // /vsync 1 manual: so intervem se for claramente estrangulado (nao 60 Hz normal)
          if (!gSoftPace && gVsyncOn) { if (swMs > lim) slowSw++; else if (swMs < 40.0) slowSw = 0;
            if (slowSw >= 3) { slowSw = 0; gSoftHz = 60; SDL_GL_SetSwapInterval(0); gSoftPace = true; gVsyncOn = false; logWarn("[PERF] swap travado (compositor limitando a janela): vsync do driver trocado por limitador de %d FPS (/vsync 1 desfaz)", gSoftHz); } }
          else if (gSoftPace) { if (swMs > 400.0) slowSw0++; else if (swMs < 40.0) { slowSw0 = 0; if (gSwapSkip) { gSwapSkip = false; logInfo("[PERF] swap normalizado: troca de buffers a cada quadro novamente"); } }
            if (slowSw0 >= 3 && !gSwapSkip) { gSwapSkip = true; slowSw0 = 0; logWarn("[PERF] swap continua travado mesmo sem vsync (janela oculta/em 2o plano): SwapWindow so 1x/s, simulacao fluida"); } }
          gSwapT = SDL_GetTicks(); }
#endif
        if (gSoftPace) { static Uint32 siT = 0; const Uint32 tn = SDL_GetTicks(); if (tn - siT > 1000) { siT = tn; SDL_GL_SetSwapInterval(0); } }   // patchWayland.1: tela cheia/redimensionar no Wayland devolve o vsync do driver (swap travando 50 ms, ~30 FPS); reaplica 0 a cada 1 s (o limitador proprio segura o FPS)
        if (gSoftPace) { static Uint64 sp0 = 0; const Uint64 fq = SDL_GetPerformanceFrequency(), per = fq / gSoftHzNow(win); Uint64 nn = SDL_GetPerformanceCounter();
          if (sp0 && nn - sp0 < per) { const Uint32 ms = (Uint32)((per - (nn - sp0)) * 1000 / fq); if (ms > 1) SDL_Delay(ms - 1); while ((nn = SDL_GetPerformanceCounter()) - sp0 < per) {} } sp0 = SDL_GetPerformanceCounter(); }
        { const float fv = gsf("v3", 1.f); if (fv < .999f) { const int cap = std::max(10, (int)floorf(10 + 250 * fv + .5f)); static Uint64 ft0 = 0; const Uint64 fq = SDL_GetPerformanceFrequency(), per = fq / cap, nn = SDL_GetPerformanceCounter(); if (ft0 && nn - ft0 < per) { const Uint32 ms = (Uint32)((per - (nn - ft0)) * 1000 / fq); if (ms > 0) SDL_Delay(ms); } ft0 = SDL_GetPerformanceCounter(); } }   // patchOpt.1: Taxa de FPS Maxima
#ifdef BL_RPI
        { static const bool fin = getenv("BLOCKLAND_GLFINISH") != nullptr; if (fin) { const double r2f = perfNow(); glFinish(); gR2[3] += perfNow() - r2f; } }   // patchRpiOpt.4: sem glFinish (CPU e GPU em paralelo); BLOCKLAND_GLFINISH=1 liga
#endif
        { for (int i = 0; i < 5; i++) { gR2d[i] = gR2[i] - gR2prev[i]; gR2prev[i] = gR2[i]; } }   // patchGfx.2: tempo por etapa deste quadro (PC tambem)
        { static Uint64 p2 = 0; gR2n++; const Uint64 q2 = SDL_GetPerformanceCounter(), f2 = SDL_GetPerformanceFrequency(); if (!p2) p2 = q2;
          if (q2 - p2 > 5 * f2) { const double n = gR2n ? gR2n : 1; logInfo("[PERF2] por quadro: ceu %.2f entidades %.2f hud %.2f glFinish %.2f ticks(mobs+itens) %.2f | quadro %.1f ms", gR2[0] / n, gR2[1] / n, gR2[2] / n, gR2[3] / n, gR2[4] / n, (double)(q2 - p2) * 1000.0 / f2 / n); for (double& v : gR2) v = 0; for (double& v : gR2prev) v = 0; gR2n = 0; p2 = q2; } }
#ifdef BL_RPI
        { static Uint64 t0 = 0; const Uint64 fq = SDL_GetPerformanceFrequency(), per = fq / (gRpiSplit ? 30 : 60); const Uint64 n = SDL_GetPerformanceCounter();   // trava em 60 FPS mesmo se o vsync do driver falhar
          if (t0 && n - t0 < per) { const Uint32 ms = (Uint32)((per - (n - t0)) * 1000 / fq); if (ms > 1) SDL_Delay(ms - 1); } t0 = SDL_GetPerformanceCounter(); }
#endif
        PROBE("fim"); probeReport();
        perfFrameEnd(W.size());   // profiler: [PERF] a cada 5 s e [LAG] em quadros > 45 ms
        if (gPfFrameMs > 45.0 && gPfFrameMs < 1000.0) { static double lastL2 = -1e9; const double nw = perfNow(); if (nw - lastL2 > 400.0) { lastL2 = nw; logWarn("[LAG2] quadro de %.0f ms | ceu %.1f | entidades %.1f | hud %.1f | ticks(mobs+itens) %.1f | chunks no mundo %zu", gPfFrameMs, gR2d[0], gR2d[1], gR2d[2], gR2d[4], W.size()); } }   // patchGfx.2: quem causa o 'resto' das travadas
