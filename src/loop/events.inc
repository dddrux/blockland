// eventos SDL (SDL_PollEvent), HUD touch, gamepad
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
        while (SDL_PollEvent(&e)) {
#ifdef BL_RPI
            if (!((e.type == SDL_MOUSEMOTION && e.motion.which == 0x7F000001u) || ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) && e.button.which == 0x7F000001u))) blScaleEvent(e);   // patchRpi.3: eventos do cursor do controle ja sao locais (antes eram reescalados 2x = cursor desancorado fora de 400x250)
#endif
#ifdef __ANDROID__
            if (e.type == SDL_APP_WILLENTERBACKGROUND || e.type == SDL_APP_DIDENTERFOREGROUND || e.type == SDL_APP_WILLENTERFOREGROUND || (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST || e.window.event == SDL_WINDOWEVENT_RESTORED || e.window.event == SDL_WINDOWEVENT_SHOWN))) { touchReset(); if (e.type == SDL_APP_DIDENTERFOREGROUND || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)) relMouse(grab); }   // patchRpi.3: ao voltar, solta dedos presos e reafirma o modo do mouse (o Android o perde)
#endif
            if (gpEvent(e)) continue;   // conexao/desconexao de controles
            if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {   // patchSplit.4: UI so num viewport: mouse real vira coordenada local (eventos do controle ja vem locais, marcados)
                int ox, oy, ow, oh; const bool sub = uiRectNow(ox, oy, ow, oh); const bool mot = e.type == SDL_MOUSEMOTION;
                if (sub && (mot ? e.motion.which : e.button.which) != 0x7F000001u) {
                    int& ex = mot ? e.motion.x : e.button.x; int& ey = mot ? e.motion.y : e.button.y;
                    ex = std::max(0, std::min(ow - 1, ex - ox)); ey = std::max(0, std::min(oh - 1, ey - oy));
                }
            }
            if (e.type == SDL_DROPFILE) {   // skin arrastada: tratada ANTES da cadeia de menus (antes o ramo gScreen consumia o evento e nada acontecia)
                if (e.drop.file && capeDropFile(e.drop.file)) { say("Capa carregada"); SDL_free(e.drop.file); continue; }   // patchCapa.2: GIF (ou PNG de capa 64x32) vira capa
                if (e.drop.file) { const bool ok = skinDropped(e.drop.file); if (ok) { gSkinDropT = SDL_GetTicks(); gSkinDropOk = true; } else { gSkinDropT = SDL_GetTicks(); gSkinDropOk = false; } say(ok ? "Skin carregada" : "Skin inválida"); SDL_free(e.drop.file); }
                continue;
            }
#ifdef SDL_DROPBEGIN
            if (e.type == SDL_DROPBEGIN) { gSkinDropHover = true; continue; }
            if (e.type == SDL_DROPCOMPLETE) { gSkinDropHover = false; continue; }
#endif
            if (gKbCap >= 0 && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)) {   // patchKeys.1: Opcoes > Controles esperando a nova tecla (Esc cancela)
                if (e.type == SDL_KEYDOWN && !e.key.repeat) { if (e.key.keysym.scancode != SDL_SCANCODE_ESCAPE) kbSet(gKbCap, (int)e.key.keysym.scancode); gKbCap = -1; }
                continue;
            }
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) e.key.keysym.sym = keymapSym(e.key.keysym.sym);   // BUG-015
            if (e.type == SDL_KEYDOWN && !e.key.repeat && (e.key.keysym.sym == SDLK_F11 || (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)))) {   // tela cheia: F11 ou Alt+Enter (vale em qualquer tela/menu)
                const bool fs = (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                if (SDL_SetWindowFullscreen(win, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) logInfo(fs ? "Modo janela" : "Tela cheia ligada"); else logError("Falha ao alternar tela cheia: %s", SDL_GetError());
                continue;
            }
#ifdef __ANDROID__
            if (e.type == SDL_APP_WILLENTERBACKGROUND && !gScreen) saveCurWorld(P.x, P.y, P.z);   // Android pode matar o app em segundo plano
            if (e.type == SDL_APP_WILLENTERBACKGROUND) logSyncSession();   // atualiza o log datado antes de o sistema poder matar o app
            if (e.type == SDL_APP_TERMINATING) logFinalize("encerrado pelo sistema");
#endif
            if (e.type != SDL_QUIT && modulesEvent(e)) continue;   // patchMod.1: gancho onEvent dos modulos (src/modules/api.inc)
            if (e.type == SDL_QUIT) { if (!gScreen || (netSock >= 0 && !netHost && !gCurDir.empty() && !gGenJob)) saveCurWorld(P.x, P.y, P.z); run = false; }   // patchGuest.1 (BUG-022): convidado fechando a janela com o menu de pausa aberto tambem grava inventario/posicao
            else if (monIsOpen()) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); monEvent(e, ww, wh); if (!monIsOpen()) { grab = true; relMouse(true); if (touchHud) touchReset(); } }   // Monitor Touch aberto: consome a entrada
            else if (gScreen) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (!splashActive()) { menuEvent(e, ww, wh); } }   // overlay do splash bloqueia o menu (como no Minecraft)
            else if (pauseOpen && !gPage.empty()) { int ww, wh; uiWinSize(&ww, &wh); if (e.type == SDL_MOUSEMOTION) { mouseX = e.motion.x; mouseY = e.motion.y; } pageEvent(e, ww, wh); }   // Opções dentro do menu de pausa
            else if (touchHud && grab && !invOpen && !chatOpen && [&] { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); return touchEvent(e, ww, wh); }()) {}   // HUD touch consumiu o evento
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && grab && !gCtl.on) setPause(true);   // trocar de janela abre o menu de pausa (como o Java)
            else if (e.type == SDL_DROPFILE) { if (skinDropped(e.drop.file)) say("Skin carregada"); else say("Skin inválida"); SDL_free(e.drop.file); }
            else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_F3) { if (gGmSw) { gGmSw = false; if (GM_ORDER[gGmSelI] != gGm) setGm(GM_ORDER[gGmSelI]); } else if (!gF3Used) { dbg = !dbg; if (!dbg) msgT = 0; } }
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
#ifdef __ANDROID__
                if (k == SDLK_AC_BACK) k = SDLK_ESCAPE;   // botao Voltar = Esc
#endif
                if (gSignEd) { signKey((int)k); continue; }
                if (chatOpen) { if (chatKey(k, e.key.keysym.mod, P)) setChat(false, "", ""); continue; }
                if (gCredits) { if (k == SDLK_ESCAPE || k == SDLK_RETURN) creditsSkip(); continue; }   // patchEnd.14
                if (pauseOpen) { if (quitPhase) continue; if (k == SDLK_ESCAPE) setPause(false); else if (k == SDLK_F3) { dbg = !dbg; if (!dbg) msgT = 0; } continue; }   // no menu de pausa só Esc (volta) e F3
                if (!invOpen && grab && (k == SDLK_t || k == SDLK_SLASH)) { setChat(true, k == SDLK_SLASH ? "/" : "", k == SDLK_SLASH ? "/" : "t"); continue; }
                if (k == SDLK_ESCAPE) { if (gDead && !creative) continue; if (gSleepT > 0 && !invOpen) { gHurtWake = true; continue; }   /* patchSleep.3: Esc = sair da cama (GuiSleepMP) */ if (invOpen) setInv(false); else setPause(true); }
                if (k == SDLK_q && grab && !invOpen && !chatOpen && gGm != 3 && inv[sel].id) {   // soltar item (Ctrl+Q = pilha inteira)
                    int nn = (SDL_GetModState() & KMOD_CTRL) ? inv[sel].n : 1;
                    throwItem(viewEye, yaw, pitch, inv[sel].id, nn); inv[sel].n -= nn; if (inv[sel].n <= 0) inv[sel] = Slot();
                }
                if (k == SDLK_f && grab && !invOpen && !chatOpen && gGm != 3) std::swap(inv[sel], offh);   // trocar item entre as maos (como o F do Minecraft)
                if (k == SDLK_e && (grab || invOpen) && !(invOpen && invSearching())) setInv(!invOpen);
                if (invOpen && invSearching()) {
                    gSrchEat = 0; const bool ctl = (SDL_GetModState() & KMOD_CTRL) != 0;
                    if (k == SDLK_BACKSPACE || k == SDLK_DELETE) invSearchBack();
                    else if (ctl && k == SDLK_a) gSrchSel = !searchQ.empty();   // Ctrl+A seleciona tudo (destaque azul); digitar/Backspace substitui/apaga
                    else if (ctl && (k == SDLK_c || k == SDLK_x) && !searchQ.empty()) { SDL_SetClipboardText(searchQ.c_str()); if (k == SDLK_x) { searchQ.clear(); gSrchSel = false; cscroll = 0; } }
                    else if (ctl && k == SDLK_v) { if (char* cb = SDL_GetClipboardText()) { invSearchText(cb); SDL_free(cb); } }
                }
                if (k == SDLK_F4) { if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3]) { if (invOpen) setInv(false); gF3Used = true; if (!gGmSw) { gGmSw = true; gGmSelI = 0; for (int i = 0; i < 4; i++) if (GM_ORDER[i] == gGm) gGmSelI = i; } gGmSelI = (gGmSelI + 1) & 3; } }   // patchGm.1: F4 sozinho nao troca mais o modo; so F3+F4 abre o seletor (cada F4 avanca; soltar F3 aplica, igual ao Java)
                if (k == SDLK_F3) gF3Used = false;   // o debug alterna ao SOLTAR o F3 sem combinacao (SDL_KEYUP abaixo)
                if (k == SDLK_F1) hideHud = !hideHud;
                if (k == SDLK_F5) { if (gSleepT <= 0) camMode = (camMode + 1) % 3; }   // patchSleep.4: sem F5 deitado
                if (k == SDLK_F9 && gSplit && !chatOpen && !invOpen && !pauseOpen && !gSignEd && !gScreen && gInvOwner == 0 && !e.key.repeat) {   // patchSplit.6: teclado/mouse assumem o proximo jogador do split
                    const int kk = split2PossessPick();
                    if (kk >= 0) { SxSel q(kk);
                        std::swap(P, SX.P); std::swap(yaw, SX.yaw); std::swap(pitch, SX.pitch); std::swap(sel, SX.sel); std::swap(bodyYaw, SX.bodyYaw); std::swap(prevBodyYaw, SX.prevBodyYaw);
                        std::swap(limbSwing, SX.limbSwing); std::swap(limbAmt, SX.limbAmt); std::swap(prevLimbAmt, SX.prevLimbAmt); std::swap(swing, SX.swing); std::swap(swinging, SX.swinging);
                        std::swap(mnOn, SX.mnOn); std::swap(mnT, SX.mnT); std::swap(mnNeed, SX.mnNeed); std::swap(mnX, SX.mnX); std::swap(mnY, SX.mnY); std::swap(mnZ, SX.mnZ);
                        split2SwapInv(); if (SX.skinTex) std::swap(skinTex, SX.skinTex);
                        split2SwapSurv(); std::swap(gPvpP, SX.pvpP);   // gPvpP continua apontando para o P global (os valores e que trocaram)
                        std::swap(gBoatRide, gBoatX[kk]); camMode = 0;
                        std::swap(camBobYaw, SX.cbY); std::swap(prevCamBobYaw, SX.pcbY); std::swap(camBobPitch, SX.cbP); std::swap(prevCamBobPitch, SX.pcbP); std::swap(fovMod, SX.fovMod); std::swap(prevFovMod, SX.prevFovMod);
                        for (int hi = 0; hi < 4; hi++) std::swap(gHandBob[hi], SX.hb[hi]);
                        { const int ca = gsi("capesel", 0); gSt["capesel"] = SX.capeSel; SX.capeSel = ca; }   // a capa vai junto com o corpo (capeSyncSel recarrega a do J1)
                        split2PossessPads(kk); split2PossessDone(kk);
                        const int bi = gBodyId[0]; say(("Controlando: " + (bi == 0 ? netName() : xNickK(bi - 1)) + "  (F9 troca)").c_str()); }
                    else say("F9: nenhum outro jogador no split"); }
                if (k == SDLK_v && !chatOpen && !invOpen && !pauseOpen && !gSignEd && !gDead) { if (netOn) voiceSet(!gVoiceOn); else say("Voice chat: so funciona no multiplayer"); }   // V = liga/desliga o microfone (so o jogador do teclado)
                if (k == SDLK_F8) { touchHud = !touchHud; touchReset(); tvSneakT = false; relMouse(grab); say(touchHud ? "HUD touch ligado" : "HUD touch desligado"); }
                if (invOpen && (k == SDLK_q || k == SDLK_f) && !invSearching()) {   // Q / Ctrl+Q / F sobre o slot apontado (survival e criativo, igual ao baú)
                    int w0, h0; uiWinSize(&w0, &h0); const int hs = hitSlot(w0, h0, mouseX, mouseY);
                    if (k == SDLK_q) invDropKey(hs, (SDL_GetModState() & KMOD_CTRL) != 0); else invOffhandKey(hs);
                }
                if (k >= SDLK_1 && k <= SDLK_9) {   // patchInv.1: na aba Pesquisar tambem: sobre um slot manda para a hotbar (como o Java); fora de slot digita o numero na busca
                    int n = k - SDLK_1;
                    int w0, h0; uiWinSize(&w0, &h0);
                    int hs = invOpen ? hitSlot(w0, h0, mouseX, mouseY) : -1;
                    if (hs >= 0) { invNumberKey(n, hs); if (invOpen && invSearching()) gSrchEat = (char)('1' + n); }
                    else if (!invOpen) sel = n;   // com o inventário aberto o Java ignora 1-9 fora de slot
                }
            } else if (e.type == SDL_TEXTINPUT) { if (gSignEd) signText(e.text.text); else if (chatOpen) chatText(e.text.text); else if (invOpen) invSearchText(e.text.text); else if (grab && !strcmp(e.text.text, "/")) setChat(true, "/", "/"); }
            else if (e.type == SDL_MOUSEWHEEL) { if (chatOpen) chatScroll = std::max(0, chatScroll + e.wheel.y * 2); else if (pauseOpen) {} else if (!invOpen && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_C] && grab && !pauseOpen) gZoomTgt = std::max(.05f, std::min(1.f, gZoomTgt * (e.wheel.y > 0 ? .8f : 1.25f)));   /* patchZoom.2: C + roda = ajusta o zoom */ else if (!invOpen) sel = ((sel - e.wheel.y) % 9 + 9) % 9; else invWheel(e.wheel.y); }
            else if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x; mouseY = e.motion.y;
                if (invOpen) { int ww, wh; uiWinSize(&ww, &wh); invMouseMotion(ww, wh, mouseX, mouseY); }
                if (grab && gSleepT <= 0) {   // patchSleep.3: deitado a camera fica travada (como no MC)
                    const float msens = std::max(.1f, gsf("q1", .5f) * 2.f), minv = gsi("q2", 0) ? -1.f : 1.f;   // patchMouse.1: sensibilidade (0-200%, padrao 100%) e inverter mouse (Configuracoes > Controles)
                    yaw -= e.motion.xrel * .1f * gZoom * msens;   // patchZoom.1: sensibilidade cai com o zoom
                    if (yaw > 180.f) yaw -= 360.f;
                    if (yaw < -180.f) yaw += 360.f;
                    pitch = std::max(-89.f, std::min(89.f, pitch - e.motion.yrel * .1f * gZoom * msens * minv));
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (touchHud && (invOpen || chatOpen || gSleepT > 0)) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (touchCloseRect(ww, wh).in((float)e.button.x, (float)e.button.y)) { if (invOpen) setInv(false); else if (chatOpen) setChat(false, "", ""); else gHurtWake = true; continue; } }   // X do modo celular (patchSleep.5: deitado, o X acorda)
                if (gCredits) { creditsSkip(); continue; }   // patchEnd.15: clique/A do gamepad pula os creditos
                if (gDead && !creative && !pauseOpen) {   // tela de morte: Renascer / Menu principal
                    int ww, wh; uiWinSize(&ww, &wh);
                    if (e.button.button == SDL_BUTTON_LEFT && !quitPhase) { const int hb = deathHit(ww, wh, e.button.x, e.button.y); if (hb >= 0) sndEvtUi("ui.click"); if (hb == 0) gWantRespawn = true; else if (hb == 1) { quitPhase = true; quitT0 = SDL_GetTicks(); } }
                    continue;
                }
                if (pauseOpen) {   // só "Voltar para o jogo" funciona; os outros botões são placeholders
                    int ww, wh; uiWinSize(&ww, &wh); if (e.button.button == SDL_BUTTON_LEFT && !quitPhase && pauseHit(ww, wh, e.button.x, e.button.y) != PB_NONE) sndEvtUi("ui.click");
                    if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_BACK && !quitPhase) setPause(false);
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_ACH && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo("?Conquistas"); }
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_STATS && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo("?Estatísticas"); }
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_LAN && !quitPhase) { if (!netLanBusy() && netStartHost(gNetPort)) setPause(false); }
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SKIN2 && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo("peskin"); }   // Esc > skin do jogador 2 (so com split ligado)
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_OPT && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo(peUi() ? "peset" : "opcoes"); }   /* patchBoot.1: pausa no PC abre Opcoes do Java */
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SAVE && !quitPhase) { quitPhase = true; quitT0 = SDL_GetTicks(); }
                    continue;
                }
                if (invOpen) {
                    int ww, wh; uiWinSize(&ww, &wh);
                    invMouseDown(e.button.button, ww, wh, e.button.x, e.button.y, SDL_GetModState() & KMOD_SHIFT, SDL_GetTicks());
                    continue;
                }
                if (chatOpen) { if (oskActive() && e.button.button == SDL_BUTTON_LEFT) { int ww, wh; uiWinSize(&ww, &wh); oskMouse(e.button.x, e.button.y, ww, wh); } continue; }   // chat: clique no teclado virtual
                if (!grab) { grab = true; relMouse(true); continue; }
                act(e.button.button); rep = 0;
            }
            else if (e.type == SDL_MOUSEBUTTONUP) { if (invOpen) invMouseUp(e.button.button, 0, 0, 0, 0); }
        }

        if (touchHud) {   // HUD touch: aplica o que os toques pediram neste quadro
            if (invOpen || chatOpen || !grab) touchReset();
            gTouchUseItem = grab && !gDead && ((!creative && foodValue(inv[sel].id) > 0 && gFood < 20) || (inv[sel].id == IT_BOW && bowHasArrow()));   // item que se usa segurando
            touchFrame(SDL_GetTicks());
            int tMob = -1; float d2[3] = {dir[0], dir[1], dir[2]}; bool haveD2 = false;   // tMob: IA do mob sob o dedo (2 = aldeão), -1 = nenhum
            {   // MCPE (sem controles divididos): quebrar/colocar/bater age sob o dedo, não no centro da tela
                int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
                bool aim = touchAim; float ax = touchBrkX, ay = touchBrkY;
                if (touchPlace) { aim = true; ax = touchPlaceX; ay = touchPlaceY; }
                has = false;
                if (aim && ww > 0 && wh > 0) {
                    const float tn = tanf(lastFov * .5f * .0174533f), nx = (ax / ww * 2 - 1) * tn * ww / (float)wh, ny = (1 - ay / wh * 2) * tn;
                    const float rgt[3] = {cy, 0, -sy}, upv[3] = {sy * sp_, cp, cy * sp_};
                    for (int i = 0; i < 3; i++) d2[i] = dir[i] + rgt[i] * nx + upv[i] * ny;
                    const float l2 = sqrtf(d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2]); for (float& v : d2) v /= l2;
                    has = ray(eye, d2, hh, pp); haveD2 = true; tMob = touchMobAim(eye, d2, creative ? 6.f : 3.f);
                }
            }
            auto actAt = [&](int btn) { float sv[3] = {dir[0], dir[1], dir[2]}; if (haveD2) for (int i = 0; i < 3; i++) dir[i] = d2[i]; act(btn); for (int i = 0; i < 3; i++) dir[i] = sv[i]; };   // age no ponto tocado (mobs/botes/blocos)
            if (touchBreak && tMob >= 0 && tMob != 2) {   // Bedrock: segurar sobre um mob = bater repetidamente (não minera)
                touchBreak = false; touchRing = false; touchBrkProg = 0;
                static Uint32 atkLast = 0; const Uint32 nt = SDL_GetTicks(); if (nt - atkLast > 500) { atkLast = nt; actAt(SDL_BUTTON_LEFT); }
            }
            if (touchUse && gEatOn && gEatT >= 0) { touchRing = true; touchBrkProg = std::max(.02f, gEatT / 32.f); }   // Bedrock: segurar com comida = comer em 1,6 s (eatTick); o anel enche enquanto come
            if (tDX != 0 || tDY != 0) {
                yaw -= tDX * .25f; if (yaw > 180.f) yaw -= 360.f; if (yaw < -180.f) yaw += 360.f;
                pitch = std::max(-89.f, std::min(89.f, pitch - tDY * .25f)); tDX = tDY = 0;
            }
            if (touchSel >= 0) { sel = touchSel; touchSel = -1; }
            if (tAct[TA_LIGHT]) shadowToggle(say);   // botao de luz (mobile) = F7: liga/desliga sombras
            if (tAct[TA_CHAT]) setChat(true, "", "");
            if (tAct[TA_VOICE]) { if (netOn) voiceSet(!gVoiceOn); else chatAdd("Voice chat: so funciona no multiplayer", 0xAAAAAA); }   // botao de microfone (Android)
            if (tAct[TA_F3]) { dbg = !dbg; if (!dbg) msgT = 0; }
            if (tAct[TA_PAUSE]) setPause(true);
            if (tAct[TA_INV]) setInv(true);
            if (tAct[TA_F1]) hideHud = !hideHud;   // F1: oculta o HUD (o botão F1 fica com opacidade baixa)
            if (tAct[TA_F5] && gSleepT <= 0) camMode = (camMode + 1) % 3;   // F5: 1ª pessoa > 3ª atrás > 3ª na frente
            if (tAct[TA_DROP] && gGm != 3 && inv[sel].id) { throwItem(viewEye, yaw, pitch, inv[sel].id, 1); if (--inv[sel].n <= 0) inv[sel] = Slot(); }   // Q
            if (tAct[TA_SWAP] && gGm != 3) std::swap(inv[sel], offh);   // F
            if (tAct[TA_SPRINT]) tvSprintL = !tvSprintL;
            for (bool& a : tAct) a = false;
            if (touchPlace) {   // toque rápido: em mob = bater (aldeão = interagir); senão colocar/interagir. Com comida/arco na mão só segurar usa o item (como o Bedrock)
                touchPlace = false;
                if (grab && !invOpen && !gTouchUseItem) actAt(tMob >= 0 && tMob != 2 ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
            }
        }
        if (openChestReq) { openChestReq = false; if (!invOpen) setInv(true); }
        if (invOpen && gChest && chestFrame(P.x, P.y, P.z)) setInv(false);
        if (openTable) { openTable = false; if (!invOpen) setInv(true); else gTable = false; }   // fora do bloco touchHud: vale também com mouse/teclado
        {   // ---------- controle (gamepad.inc): le o estado; menus = cursor virtual; jogo = entrada do jogador 1 (e do 2 no split) ----------
            static Uint32 gpT = 0; const Uint32 tn = SDL_GetTicks(); const float gdt = gpT ? std::min(.1f, (tn - gpT) / 1000.f) : 0.f; gpT = tn;
            gSplitMode = gsi("splitmode", 0) & 1;
            if (gSkin2Dirty) { gSkin2Dirty = false; for (int xk = 0; xk < 3; xk++) { SxSel q(xk); if (SX.skinTex) { glDeleteTextures(1, &SX.skinTex); SX.skinTex = 0; } if (SX.init) split2LoadSkin(); } }
            { static Uint32 gpScanT = 0; const Uint32 gpN = SDL_GetTicks(); if (gpN - gpScanT > (gpN < 20000 ? 1000u : 120000u)) { gpScanT = gpN; const double t0 = perfNow(); gpScanAll(); const double dt = perfNow() - t0; if (dt > 4.0) logWarn("[SONDA] gpScanAll levou %.1f ms", dt); } }   // patchRpiOpt.8: so varre 1x/s nos 20 s iniciais; depois confia no hot-plug (SDL_CONTROLLERDEVICEADDED) e varre a cada 30 s   // patchPad.9
            gpPoll(); { static const float SN[5] = {.5f, .75f, 1.f, 1.5f, 2.f}; gGpSens = SN[std::max(0, std::min(4, gsi("gpsens", 2)))]; }
            gGpMouse = 0; gi1 = GpIn(); GpPad* pad1 = gpPadFor(1);
            { static bool sgPrev = false;   // patchSign.2: editor da placa solta o mouse (camera/personagem travados) e liga a entrada de texto (antes o texto nao aparecia: SDL_StartTextInput so era chamado no chat)
              if (gSignEd && !sgPrev) { grab = false; relMouse(false); SDL_StartTextInput(); if (touchHud) touchReset(); }
              else if (!gSignEd && sgPrev) { if (!pauseOpen && !invOpen && !chatOpen && !gScreen) { grab = true; relMouse(true); }
#ifdef __ANDROID__
                  SDL_StopTextInput();
#endif
              }
              sgPrev = gSignEd; }
            signTick(nullptr, nullptr);   // patchSignUI.1: abre o editor da placa recem-colocada AQUI (na logica). Antes rodava no desenho: kSign (linha ~1084) era restaurado em ~1574 e apagava o editor no mesmo quadro
            const bool uiMode = gScreen || pauseOpen || invOpen || chatOpen || gSignEd || monIsOpen() || (gDead && !creative) || gCredits;
            gInvOpenNow = invOpen; if (gpCount() && uiMode) { uiSync(); if (!splashActive()) gpUiFrame(win, gdt); } else gpUiLeave();
            if (!uiMode) {
                for (GpPad& q : gPads) if (q.c && gpBtnPressed(&q, GB_START)) { split2PadStart(q.id, P, yaw); pad1 = gpPadFor(1); break; }   // patchSplit.5: START abre/fecha a janela do controle
                for (GpPad& q : gPads) if (q.c && (!gSplit || &q == pad1 || !pad1) && gpPressed(&q, GA_PAUSE)) { setPause(true); break; }   // split: so o controle do jogador 1 pausa (com J1 no teclado, qualquer um)
                if (pad1 && grab && !gDead && !pauseOpen) {
                    gi1 = gpRead(pad1);
                    yaw = wrap180(yaw - gi1.lx * GP_LOOK_DEG * gGpSens * gdt); pitch = std::max(-89.f, std::min(89.f, pitch - gi1.ly * GP_LOOK_DEG * gGpSens * gdt));
                    if (gpPressed(pad1, GA_SPRINT) && gi1.fwd > .5f) gGpSprint = true; else if (gi1.fwd < .3f) gGpSprint = false;
                    const int bA = gGpMap[GA_ATTACK], bU = gGpMap[GA_USE];
                    if (pad1->b[bA] && !pad1->pb[bA]) gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, mouseX, mouseY);   // vira o clique normal (act) no proximo quadro
                    if (!pad1->b[bA] && pad1->pb[bA]) gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, mouseX, mouseY);
                    if (pad1->b[bU] && !pad1->pb[bU]) gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, mouseX, mouseY);
                    if (!pad1->b[bU] && pad1->pb[bU]) gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, mouseX, mouseY);
                    gGpMouse = (gi1.attack ? SDL_BUTTON(1) : 0) | (gi1.use ? SDL_BUTTON(3) : 0);
                    if (gpPressed(pad1, GA_INV)) setInv(true);
                    else if (gpPressed(pad1, GA_CHAT)) { gChatPadNext = true; setChat(true, "", ""); }
                    if (gpPressed(pad1, GA_CAM) && gSleepT <= 0) camMode = (camMode + 1) % 3;
                    if (gpPressed(pad1, GA_NEXT) || gpBtnPressed(pad1, GB_RIGHT)) sel = (sel + 1) % 9;
                    if (gpPressed(pad1, GA_PREV) || gpBtnPressed(pad1, GB_LEFT)) sel = (sel + 8) % 9;
                    if (gpPressed(pad1, GA_DROP) && gGm != 3 && inv[sel].id) { throwItem(viewEye, yaw, pitch, inv[sel].id, 1); if (--inv[sel].n <= 0) inv[sel] = Slot(); }
                    if (gpPressed(pad1, GA_SWAP) && gGm != 3) std::swap(inv[sel], offh);
                    if (gpPressed(pad1, GA_PICK)) actMain(SDL_BUTTON_MIDDLE);   // pick block (controle)
                } else if (!pad1 || !grab) gGpSprint = false;
            }
            if (gSplit) { const int wk = split2Frame(gdt, !gScreen, P); if (wk && !invOpen && gGm != 3) { gInvK = wk - 1; split2SwapInvOwner(); gInvOwner = 2; setInv(true); } }   // jogador 2 pediu o inventario
        }
