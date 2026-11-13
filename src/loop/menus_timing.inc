// menus/telas, carregamento, autosave, dt, repeticao de acao, animacoes
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
        if (gScreen != SC_PLAY) netUpdate(0.f);   // menu/loading: descoberta, handshake e keepalive (em jogo, netUpdate(dt) roda mais abaixo)
        if (gJoin == NJ_WELCOMED && (gScreen == SC_OTHER || gScreen == SC_TITLE)) { netPrepareGuestWorld(); gMenuDir = "_mp"; leavePages(); gMenuAct = 1; }
        if (gMenuAct == 1) startWorld(); else if (gMenuAct == 2) run = false;
        gMenuAct = 0;
        { static Uint32 pAuto = 0; const Uint32 tn = SDL_GetTicks(); if (gScreen == SC_PLAY && !gCurDir.empty() && tn - pAuto > 5000) { pAuto = tn; const double t0 = perfNow(); savePlayer(); const double dt = perfNow() - t0; if (dt > 4.0) logWarn("[SONDA] savePlayer levou %.1f ms", dt); } }   // autosave do jogador a cada 5 s
        { static Uint32 wAuto = 0; const Uint32 tn = SDL_GetTicks();   // patchSave.1: autosave do MUNDO a cada 5 min (so jogando, sem baú aberto, nao convidado); o 1o intervalo conta a partir da entrada
          if (!wAuto) wAuto = tn;
          if (gScreen == SC_PLAY && !gCurDir.empty() && !gChest && !gGenJob && tn - wAuto > 300000u) { wAuto = tn; saveCurWorld(P.x, P.y, P.z); logInfo("[AUTOSAVE] mundo salvo"); }   /* patchGuest.1 (BUG-022): convidado tambem salva (so inventario/posicao em saves/_guest, leve); antes so gravava ao sair pelo menu e perdia tudo se o jogo fechasse/travasse */ }
        if (gScreen == SC_LOAD) {   // carrega os chunks iniciais (raio 3 gerado e com malha) antes de mostrar o jogo
            int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
            stream(loadCx, loadCz, 12, 10, 40.0);   // patchFast.3: loading nao precisa poupar quadro
            if (useVBO) for (auto& kv : W) if (kv.second->gl || kv.second->glw) uploadChunk(kv.second);   // sobe as malhas durante o loading
            int ok = 0, tot = 0; const int lr = std::min(
2,   // modo desempenho: spawn 5x5 (e nao 7x7): a malha de cada chunk levava ~0,5 s no Pi (loading de 26 s)
 std::max(1, RAD)); const float lrm = (RAD + .5f) * (RAD + .5f);   // so exige o que o stream realmente gera e malha (raio RAD): com RAD baixo o 7x7 fixo nunca fechava (travava em 40%)
            for (int dz = -lr; dz <= lr; dz++) for (int dx = -lr; dx <= lr; dx++) { if ((float)(dx * dx + dz * dz) > lrm) continue; tot++; Chunk* c = getC(loadCx + dx, loadCz + dz); if (c && !c->dirty) ok++; }
            if (tot == 0) { tot = 1; ok = 1; }
            { const int pct = ok * 100 / tot / 10 * 10; if (pct > loadLogPct) { loadLogPct = pct; logInfo("Preparing spawn area: %d%%", pct); } }
            const Uint32 nowL = SDL_GetTicks(); const float el = (nowL - loadT0) / 1000.f;   // barra: 1 s; depois "Pronto!" por 1 s
            if (!loadDoneT && ((ok == tot && el >= .12f) || el >= 25.f)) loadDoneT = nowL;   // 25 s: nunca fica preso no carregamento
            loadProg = std::min((float)ok / tot, std::min(1.f, el * 8.f)); drawLoad(ww, wh, loadProg, loadDoneT != 0, 1.f); SDL_GL_SwapWindow(win); vkBootOk();
            if (loadDoneT && nowL - loadDoneT >= 60) { logInfo("Done (%.3fs)! Para ajuda, digite \"/ajuda\"", (nowL - loadT0) / 1000.0); for (auto& kv : W) kv.second->born = 0.f; finishLoad(); last = SDL_GetTicks(); loadDoneT = 0; fading = true; fadeT0 = last; }
            continue;
        }
        if (gScreen) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); const Uint32 now2 = SDL_GetTicks(); float tIn = 1e9f; capeSyncSel(); gDifficulty = std::max(0, std::min(3, gsi("d40", 2)));   /* patchFix.Cape: capa e dificuldade valem tambem nos menus (previa da tela de skin) */
            if (gScreen == SC_TITLE) { RAD = PANO_RAD; if (!panoOn) { panoBegin(); titleT0 = SDL_GetTicks(); gTitleNoBlack = false; } if (gTitleImgTex) { titleImgDraw(ww, wh); introBgGrab(ww, wh); } else if (panoStage) { panoStep(8.0); if (!bgScreen(gMenuBgTex, ww, wh)) { blViewport(ww, wh); glClearColor(0.f, 0.f, 0.f, 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); } } /* patchIntro.5: enquanto o panorama regenera, mostra o fundo em cache (antes: preto) */ else { drawPanorama(ww, wh, std::min(.1f, (now2 - last) / 1000.f)); blurScreen(ww, wh); blurScreen(ww, wh); introBgGrab(ww, wh); }   // patchMenu.3: fundo bem borrado (estilo MCPE) if (splashActive()) { gTitleNoBlack = true; titleT0 = SDL_GetTicks(); }   // enquanto o splash esta na tela o contador fica parado em 0
                tIn = (SDL_GetTicks() - titleT0) / 1000.f; { static Uint32 bgReadyT = 0; const bool bgReady = (gTitleImgTex || !panoStage || gMenuBgTex) && !splashActive(); if (!bgReady) bgReadyT = 0; else if (!bgReadyT) bgReadyT = SDL_GetTicks();
                  float fa = !bgReady ? 1.f : 1.f - (SDL_GetTicks() - bgReadyT) / (titleQuick ? 150.f : 700.f); if (gMenuBgTex && (panoStage || titleQuick) && !splashActive()) fa = 0.f;   /* volta do jogo: fundo na hora, sem fade preto */ if (fa > 0.f) drawFadeBlack(ww, wh, std::min(1.f, fa)); }   /* preto ate o fundo carregar, depois fade-in */ gUiA = splashActive() ? 0.f : std::max(0.f, std::min(1.f, (tIn - (titleQuick ? .0f : .35f)) / (titleQuick ? .15f : .5f))); if (tIn > 2.f) titleQuick = false; }   // logo e botoes: fade-in 0,35 s DEPOIS do splash acabar   // fundo aparece primeiro; logo e botoes entram depois
            else { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
            drawMenu(ww, wh); iconFilterRestore(); gUiA = 1.f; last = now2; splashOverlay(ww, wh); logGlErrors("Menu"); ctlGrab(ww, wh); SDL_GL_SwapWindow(win); vkBootOk(); { const Uint32 el = SDL_GetTicks() - now2; if (el < 8) SDL_Delay(8 - el); }   // titulo: teto ~120 FPS (vsync off deixava a GPU a 100% e travava o menu)
            continue; }
        Uint32 now = SDL_GetTicks();
        { const Uint32 raw = now - last; static Uint32 lastSlow = 0;   // como o "Can't keep up!" do Minecraft: quadro muito longo (maximo 1 aviso a cada 5 s)
          if (raw > 500 && now - lastSlow > 5000) { lastSlow = now; logWarn("Can't keep up! O quadro demorou %u ms (limite de simulacao: 250 ms) | tela=%d | chunks=%zu | causas: %s", (unsigned)raw, (int)gScreen, W.size(), gPfNotes.empty() ? "(nenhuma registrada)" : gPfNotes.c_str()); } }
        float dt = std::min(.25f, (now - last) / 1000.f);
        last = now;
        dbgFrame(dt);
        if (pauseOpen && !netOn && !netHost && !netConn && !gSplit) dt = 0.f;   // pausa congela a simulação (mundo, ticks, braço); com outro jogador conectado o jogo continua
        if (grab && !invOpen) {   // segurar o botão repete a ação
            Uint32 mb = gpMouseState();
            if (touchHud) mb = touchBreak ? SDL_BUTTON(1) : 0;   // touch: só "segurar parado" quebra
            const bool hold = (mb & SDL_BUTTON(3)) || (creative && (mb & SDL_BUTTON(1)));   // em sobrevivência o botão esquerdo é tratado pela mineração abaixo
            if (hold) { rep += dt; if (rep > .3f) { rep = .2f; act((mb & SDL_BUTTON(1)) ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT, true); } } else rep = 0;
            // sobrevivência: segurar o botão esquerdo minera (tempo conforme dureza e ferramenta)
            bool mining = false;
            mnCool = std::max(0.f, mnCool - dt);
            if (!creative && gGm != 2 && !gDead && !chatOpen && (mb & SDL_BUTTON(1)) && propMineTick(dt, inv[sel].id)) { mining = true; mnSw += dt; if (!swinging || mnSw >= .2f) { swinging = true; swing = 0; mnSw = 0; } }   // patchMec.1: prop com dureza
            if (!mining && !creative && gGm != 2 && !gDead && !chatOpen && (mb & SDL_BUTTON(1)) && has) {   // aventura não quebra blocos
                const int mb2 = getB(hh[0], hh[1], hh[2]);
                if (gEggId && mb2 == gEggId) { if (mnCool <= 0.f) { eggTeleport(hh[0], hh[1], hh[2]); mnCool = .5f; swinging = true; swing = 0; } }
                else if (mb2 != AIR && mb2 != BEDROCK && !(mb2 == XB_PORTAL && !creative) && !isWater(mb2)) {
                    mining = true;
                    if (!mnOn || mnX != hh[0] || mnY != hh[1] || mnZ != hh[2] || mnHeld != inv[sel].id) {
                        mnOn = true; mnX = hh[0]; mnY = hh[1]; mnZ = hh[2]; mnHeld = inv[sel].id; mnT = 0;
                        mnNeed = breakTime(mb2, mnHeld, mnHv);
                    }
                    // braço: como o swingItem() a cada tick do 1.8.9, o balanço reinicia a cada 4 ticks (0,2 s) e só vai até ~metade
                    mnSw += dt;
                    if (!swinging || mnSw >= .2f) { swinging = true; swing = 0; mnSw = 0; }   // sem travar o balanço no meio (só reinicia a cada 0,2 s)
                    for (mnPt += dt; mnPt >= .05f; mnPt -= .05f) spawnHit(hh[0], hh[1], hh[2], pp[0] - hh[0], pp[1] - hh[1], pp[2] - hh[2], mb2);   // partículas na face batida (1/tick)
                    if (mnCool <= 0.f) {
                        mnT += dt * enchMineMul(isWater(getB((int)floor(P.x), (int)floor(P.y + 1.62), (int)floor(P.z))), P.onGround);   // patchEnch.3c
                        if (mnT >= mnNeed) { breakAt(mnX, mnY, mnZ, mnHv); mnT = 0; mnCool = .25f; mnOn = false; }   // mining continua true: o timer do braço não zera entre blocos
                    }
                }
            }
            if (!mining) { mnOn = false; mnT = 0; mnSw = 0; mnPt = 0; }
        } else { mnOn = false; mnT = 0; mnSw = 0; mnPt = 0; }
        if (touchHud && !creative && !touchUse) touchBrkProg = (touchBreak && mnOn && mnNeed > 0) ? std::min(1.f, mnT / mnNeed) : 0.f;   // círculo enche conforme minera
        if (swinging) { swing += dt / .3f; if (swing >= 1) { swing = 0; swinging = false; } }
        int held = inv[sel].id;
        if (gSplit && gInvOwner == 2) { split2SwapInvOwner(); held = inv[sel].id; split2SwapInvOwner(); }   // patchPad.2: com o inventario do J2 aberto, `inv` guarda o DELE; a mao do J1 usa o inventario real do J1
        netUpdate(dt);
        if (held != lastHeld) { lastHeld = held; nameT = 2.f; }
        {   // ItemRenderer.updateEquippedItem (1.8.9): o item antigo desce (0,4/tick), troca ao passar de 0,1 e o novo sobe
            const float tgt = (held == handItem) ? 1.f : 0.f, step = 8.f * dt;
            equip += std::max(-step, std::min(step, tgt - equip));
            if (equip < .1f) handItem = held;
        }
        nameT = std::max(0.f, nameT - dt); msgT = std::max(0.f, msgT - dt);

        // ======================= TICK (20 por segundo, como o Java) =======================
