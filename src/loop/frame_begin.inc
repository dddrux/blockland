// inicio do quadro: teclado, lambdas de uso de item/quebrar, ajustes por quadro
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
        if (benchTick(perfNow())) run = false;   // HZ-008: --benchmark
        iconFilterRestore();   // patchPerfVk.1: garante o filtro mipmap do atlas antes de desenhar o mundo (icones deixam LINEAR ate o fim do HUD)
        ctlFrame(P, yaw, pitch, camMode, run);
        gBlockReach = creative ? 5.f : 4.5f;   // patchReach.1: alcance de quebrar/colocar igual ao MC
        SDL_Event e;
        int hh[3], pp[3];
        const Uint8* k = keymapState();   // BUG-015
        gTabHeld = (k[SDL_SCANCODE_TAB] && !chatOpen && !invOpen && !pauseOpen) || gTabTouch;
        gTabTouch = false; for (const TPt& q : tpts) if (q.on && q.kind == 3 && q.idx == TA_TAB && !gHideHud) gTabTouch = true;   // botao de toque: vale enquanto o dedo estiver nele   // lista de jogadores (Tab): so enquanto segura
        float cy = cosf(yaw * .0174533f), sy = sinf(yaw * .0174533f);
        float cp = cosf(pitch * .0174533f), sp_ = sinf(pitch * .0174533f);
        float eye[3] = {(float)viewEye[0], (float)viewEye[1], (float)viewEye[2]};
        float dir[3] = {-sy * cp, sp_, -cy * cp};
        for (int k = 0; k < 3; k++) { gAimEye[k] = eye[k]; gAimDir[k] = dir[k]; } gAimOk = true; gModHeldSel = sel;   // patchMec.1 / patchTrash.1: slot ativo para modulos
        bool has = ray(eye, dir, hh, pp);
        auto say = [&](const std::string& s) { chatAdd(s); };
        auto breakAt = [&](int bx, int by, int bz, bool harvest) {
            int b = getB(bx, by, bz);
            if (b == BEDROCK) return;   // BUG-036: rocha matriz nunca quebra (criativo/toque/gamepad passam por aqui)
            gModHeldSel = sel; modulesBlockBreak(bx, by, bz, b);   // patchMod.2
            spawnBreak(bx, by, bz, b); gStBroken++; sndEvtAt(sndBlockEvt(b, false), bx + .5, by + .5, bz + .5);
            if (b == XB_FIRE) fireSmoke(bx, by, bz);   /* patchFire.1: soco apaga o fogo com fumaca */
            setB(bx, by, bz, AIR); netSendBlock(bx, by, bz, AIR);
            if (isLogB(b) || isLeafB(b)) { keepLeaves.erase(k3(bx, by, bz)); leafNeighbors(bx, by, bz); }
            if (!creative && harvest) { gShearBreak = inv[sel].id && inv[sel].id == shearsId(); gDropSilk = enchLv(inv[sel], 33); gDropFort = gDropSilk ? 0 : enchLv(inv[sel], 35); blockDrops(bx, by, bz, b); gShearBreak = false; gDropSilk = gDropFort = 0; survExh(.025f); survOreXp(b); }
            if (!creative && blockHardness(b) > 0.f) toolWear(inv[sel]);   // patchDur.1
        };
        bool openChestReq = false;   // pedido para abrir a tela de um bau (processado junto com a mesa)
        bool openTable = false;   // pedido para abrir a mesa de trabalho (processado depois do laço de eventos)
        auto pickPut = [&](int id, bool make, const std::string& nbt = std::string()) -> bool { return invPickPut(sel, id, make, nbt); };   // pick block: ver invPickPut (inventory_ui.inc)
        auto actMain = [&](int btn, bool rpt = false) {   // quebrar / colocar (rpt = repetição do botão segurado: sem alvo não balança o braço)
            if (gDead) return;
            if (gGm == 3) { if (btn == SDL_BUTTON_LEFT && !rpt) spectTeleport(P); return; }   // espectador não interage; clique esquerdo teleporta para um jogador
            if (btn == SDL_BUTTON_MIDDLE) {   // pick block (como no Java): item ja no hotbar => seleciona; na mochila => traz para o slot atual; criativo => cria; sobrevivencia => nada
                if (rpt) return;
                if (creative) {   // pick em entidade (criativo): ovo de spawn do mob mirado, como no Java (so os mobs que tem ovo no jogo)
                    const int mt = mobPickType(eye, dir, 6.f), egg = mt >= 0 ? mobEggItem(mt) : 0;
                    if (egg) {
                        pickPut(egg, true); return;
                    }
                    { const int bi = boatPick(eye, dir, 6.f), ci = cartPick(eye, dir, 6.f);   // bote / carrinho (sem ovo): item do proprio veiculo, como no Java
                      int it = 0; if (bi >= 0) it = boatItemId(BOATS[bi].type); else if (ci >= 0) it = cartItemId(CARTS[ci].type);
                      if (it) { pickPut(it, true); return; } }
                    { const int pi = gProps.empty() ? -1 : propPick(eye, dir, 6.f); if (pi >= 0 && (!gRay.hit || !has || propPick(eye, dir, (float)gRay.t) == pi)) { pickPut(gProps[pi].id, true); return; } }   // patchProp.2: pick block do prop (so criativo, como o bote)
                }
                if (!has) return;
                const int pb = getB(hh[0], hh[1], hh[2]);
                if (pb == AIR || isLiquid(pb)) return;
                const int pit = pickItemAt(hh[0], hh[1], hh[2]);   // estado (laje/escada/tocha...) vem do item do modelo, nao do generico
                if (pit <= 0) return;
                std::string pnbt; if (pb == XB_CHEST && (SDL_GetModState() & KMOD_CTRL)) pnbt = chestPickNbt(hh[0], hh[1], hh[2]);   // Ctrl + botao do meio: copia o conteudo do bau (BlockEntityTag); sem Ctrl, bau vazio (touch/controle nunca copiam)
                pickPut(pit, creative, pnbt); return;
            }
            if (btn == SDL_BUTTON_LEFT && !rpt && !gVehRide && vehHit(eye, dir, creative, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // patchVeic.1
            if (btn == SDL_BUTTON_LEFT && !rpt && boatHit(eye, dir, inv[sel].id, creative, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // bater no bote   // patchBoat3.3: antes de bater em mob/jogador: o casco do bote quebra mesmo com animal montado
            if (btn == SDL_BUTTON_LEFT && !rpt && pvpAttack(eye, dir, inv[sel].id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { meleeWear(inv[sel]); if (P.sprint) { P.mx *= .6; P.mz *= .6; } P.sprint = false; swinging = true; swing = 0; return; }   // bater em outro jogador
            if (btn == SDL_BUTTON_LEFT && !rpt && mobAttack(eye, dir, inv[sel].id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { meleeWear(inv[sel]); if (P.sprint) { P.mx *= .6; P.mz *= .6; } P.sprint = false; swinging = true; swing = 0; return; }   // bater em mob
            if (btn == SDL_BUTTON_LEFT && !rpt && dragonHit(eye, dir, inv[sel].id, !P.onGround && P.my < 0 && !P.inWater, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // patchEnd.1
            if (btn == SDL_BUTTON_LEFT && !rpt && propHit(eye, dir, creative ? 6.f : 3.f, creative)) { swinging = true; swing = 0; return; }   // patchProp.2: remove prop
            if (btn == SDL_BUTTON_LEFT && !rpt && cartHit(eye, dir, inv[sel].id, creative, creative ? 6.f : 3.f, P)) { swinging = true; swing = 0; return; }   // bater no carrinho
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && !gCartRide && !gVehRide && vehInteract(eye, dir, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // patchVeic.1: montar veiculo
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gVehRide) { Slot& vs = inv[sel]; const int vd = vehDefOfItem(vs.id); if (vd >= 0 && gGm != 2) { if (vehPlace(vd, eye, dir, yaw, 5.f)) { swinging = true; swing = 0; if (!creative && --vs.n <= 0) vs = Slot(); } return; } }   // patchVeic.1: item de veiculo
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && !gCartRide && cartInteract(eye, dir, inv[sel], creative, creative ? 6.f : 3.f, P)) { swinging = true; swing = 0; return; }   // montar / abastecer
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && boatInteract(eye, dir, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // montar
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && !gCartRide && horseInteract(eye, dir, creative ? 6.f : 3.f, inv[sel], creative)) { swinging = true; swing = 0; return; }   // patchHorse.1: selar / montar
            if (btn == SDL_BUTTON_RIGHT && !rpt && mobInteract(eye, dir, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // aldeao: para e olha
            if (btn == SDL_BUTTON_RIGHT && !rpt) { Slot& bs = inv[sel]; const int bt = boatTypeOfItem(bs.id);   // item de bote: coloca na água (alcance 5)
                if (bt >= 0 && gGm != 2) { if (boatPlace(bt, eye, dir, yaw, 5.f)) { swinging = true; swing = 0; if (!creative && --bs.n <= 0) bs = Slot(); } return; } }
            if (btn == SDL_BUTTON_RIGHT && !rpt && rocketUse(P, inv[sel])) { swinging = true; swing = 0; return; }
            if (btn == SDL_BUTTON_RIGHT && !rpt && has && !P.elytra && gGm != 2 && !eggBlockUsable(hh[0], hh[1], hh[2]) && fwLaunch(pp, inv[sel])) { swinging = true; swing = 0; return; }   // BUG-014: foguete no bloco
            if (btn == SDL_BUTTON_RIGHT && !rpt && (inv[sel].id == IT_EGG || potThrowable(inv[sel].id) || (pearlId() > 0 && inv[sel].id == pearlId())) && gGm != 3 && (!has || P.sneak || !eggBlockUsable(hh[0], hh[1], hh[2])) && eggThrow(P, yaw, pitch, inv[sel])) { swinging = true; swing = 0; return; }   // ovo: arremessa (ovo.inc/egg.inc)   // foguete de artificio: impulso em voo de elitra
            if (btn == SDL_BUTTON_RIGHT && survCanEat(inv[sel].id)) return;   // comer: segurar o botao (eatTick)
            if (btn == SDL_BUTTON_RIGHT && !rpt && !P.sneak && propUse(eye, dir, creative ? 6.f : 5.f)) { if (gPropOpenChest) { gPropOpenChest = false; openChestReq = true; } if (gPropOpenTable) { gPropOpenTable = false; gTable = true; openTable = true; } swinging = true; swing = 0; return; }   // patchProp.4: lampada liga/desliga, porta abre/fecha (agachado = empilhar)
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && isProp(inv[sel].id)) { const int pp2 = propPlaceOnProp(inv[sel].id, eye, dir, creative ? 6.f : 5.f, yaw, P.x, P.y, P.z, P.sneak); if (pp2) { if (pp2 == 2) { swinging = true; swing = 0; if (!creative && --inv[sel].n <= 0) inv[sel] = Slot(); } return; } }   // patchProp.3: empilhar prop em cima/ao lado de outro prop
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && signUse(eye, dir, P.sneak)) { swinging = true; swing = 0; return; }   // patchSign.1
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2) { auto gx = gXbItem.find(inv[sel].id); if (gx != gXbItem.end() && gx->second == XB_SIGN) signWatchPlace(eye, dir); }
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && (bucketUse(inv[sel], eye, dir) || bottleUse(inv[sel], eye, dir))) { swinging = true; swing = 0; return; }   // balde
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && shearUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && breedUse(inv[sel], eye, dir, creative)) { swinging = true; swing = 0; return; }   // patchBreed.1: alimentar animal   // patchShear.1: tesoura
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && eggBabyUse(inv[sel], eye, dir, creative)) { swinging = true; swing = 0; return; }   // patchBabyEgg.1: ovo de spawn num mob da mesma especie = filhote
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && lilyUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }   // vitoria-regia: mira na agua (raio atravessa liquidos)
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && chiselUse(inv[sel], eye, dir, P.sneak)) { swinging = true; swing = 0; return; }   // cinzel
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && flintUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }   // isqueiro: fogo / portal
            if (rpt && !has) return;
            if (btn == SDL_BUTTON_LEFT) { swinging = true; swing = 0; }   // só o botão esquerdo balança no ar; colocar bloco balança só se colocar de fato
            if (!has) return;
            if (btn == SDL_BUTTON_LEFT) {
                int b = getB(hh[0], hh[1], hh[2]);
                if (b == BEDROCK) return;
                if (!rpt && b == gEggId && gEggId && gGm != 2) { eggTeleport(hh[0], hh[1], hh[2]); mnCool = .25f; return; }   // patchEnd.1: ovo teleporta ao ser clicado
                if (creative && isSwordId(inv[sel].id)) { mnCool = .25f; return; }   // patchSword.1: criativo + espada na mao NAO quebra bloco (so bate)
                if (creative) breakAt(hh[0], hh[1], hh[2], false);   // sobrevivência: quebra pelo tempo, no bloco de mineração abaixo
            } else if (btn == SDL_BUTTON_RIGHT) {
                if (!P.sneak && !rpt && monUse(hh[0], hh[1], hh[2])) { grab = false; relMouse(false); swinging = true; swing = 0; if (touchHud) touchReset(); return; }   // Monitor Touch: toque/clique no bloco abre o navegador
                if (getB(hh[0], hh[1], hh[2]) == XB_CHEST && !rpt && !(P.sneak && inv[sel].id)) {   // bau: abre a tela (agachado com item na mao coloca bloco, como no Java)
                    const int cu = chestOpenAt(hh[0], hh[1], hh[2]); if (cu) { if (cu == 1) openChestReq = true; swinging = true; swing = 0; return; } }
                if (gFurnId && getB(hh[0], hh[1], hh[2]) == gFurnId && !rpt && !(P.sneak && inv[sel].id)) {   // fornalha: abre a tela (agachado com item na mao coloca bloco)
                    const int fu = furnOpenAt(hh[0], hh[1], hh[2]); if (fu) { if (fu == 1) openChestReq = true; swinging = true; swing = 0; return; } }
                { const int ab = getB(hh[0], hh[1], hh[2]); if (ab >= XB_ANVIL && ab < XB_ANVIL + 4 && !rpt && !(P.sneak && inv[sel].id) && anvOpenAt(hh[0], hh[1], hh[2])) { openChestReq = true; swinging = true; swing = 0; return; } }   // patchAnvil.1: bigorna abre a tela de reparo
                if (!rpt && !(P.sneak && inv[sel].id) && brewHere(hh[0], hh[1], hh[2])) { const int bu = brewOpenAt(hh[0], hh[1], hh[2]); if (bu) { if (bu == 1) openChestReq = true; swinging = true; swing = 0; return; } }   // patchBrew.1: cervejaria
                if (!rpt && !P.sneak && dispOpen(hh[0], hh[1], hh[2])) { openChestReq = true; swinging = true; swing = 0; return; }   // patchLogic.2
                if (!rpt && !P.sneak && noteUse(hh[0], hh[1], hh[2])) { swinging = true; swing = 0; return; }   // patchLogic.1: bloco de notas
                if (!rpt && !(P.sneak && inv[sel].id)) { const int ju = jukeUse(hh[0], hh[1], hh[2], inv[sel], creative); if (ju) { swinging = true; swing = 0; return; } }   // patchJuke.1
                if (getB(hh[0], hh[1], hh[2]) == CTABLE && !P.sneak) { gTable = true; openTable = true; swinging = true; swing = 0; return; }   // como no Java: agachado coloca bloco, em pé abre a mesa
                if (gGm == 2) {   // aventura: só interage (alavanca, portas, botões); não coloca blocos
                    const int hb0 = getB(hh[0], hh[1], hh[2]);
                    if (xbIsLever(hb0) && !P.sneak && !rpt) { xbToggleLever(hh[0], hh[1], hh[2]); swinging = true; swing = 0; return; }
                    if (!P.sneak && !rpt && rsUse(hh[0], hh[1], hh[2])) { swinging = true; swing = 0; return; }
                    return; }
                { Slot& e0 = inv[sel]; const int mt = eggMob(e0.id);   // ovo de spawn
                  if (mt >= 0) { const double jx = rpt ? (rand() % 1000 / 1000.0 - .5) * .9 : 0., jz = rpt ? (rand() % 1000 / 1000.0 - .5) * .9 : 0.;   /* patchEgg.1: segurar o botao direito repete o spawn (a cada 0,2 s) com pequeno espalhamento */
                      if (netOn && !netHost) netSendMobSpawn(mt, pp[0] + .5 + jx, pp[1], pp[2] + .5 + jz, yaw + 180.f); else spawnMob(mt, pp[0] + .5 + jx, pp[1], pp[2] + .5 + jz, yaw + 180.f); swinging = true; swing = 0; if (!creative && --e0.n <= 0) e0 = Slot(); return; } }
                Slot& s = inv[sel];
                if (gEndFrameId && !rpt && getB(hh[0], hh[1], hh[2]) == gEndFrameId && !(getM(hh[0], hh[1], hh[2]) & 4) && isPk(s.id) && PK[s.id - PK0].file == "item/ender_eye") {   // encaixa o olho do Ender na moldura
                    setBM(hh[0], hh[1], hh[2], gEndFrameId, getM(hh[0], hh[1], hh[2]) | 4); netSendBlock(hh[0], hh[1], hh[2], gEndFrameId, getM(hh[0], hh[1], hh[2]));
                    tryOpenEnd(hh[0], hh[1], hh[2]); swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); return; }
                { const int cu = cropUse(s, hh, pp, rpt); if (cu) { swinging = true; swing = 0; if (cu == 2 && !creative && --s.n <= 0) s = Slot(); return; } }   // patchCrop.1: enxada / semente
                { const int ru = railUse(s, hh, pp, rpt); if (ru) { if (ru == 2) { swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); } return; } }   // trilho / carrinho (minecart.inc)
                { const int hb0 = getB(hh[0], hh[1], hh[2]);
                  if (xbIsLever(hb0) && !P.sneak && !rpt) { xbToggleLever(hh[0], hh[1], hh[2]); swinging = true; swing = 0; return; }   // liga/desliga a alavanca
                  if (!P.sneak && !rpt && rsUse(hh[0], hh[1], hh[2])) { swinging = true; swing = 0; return; }   // porta/alcapao/portao/repetidor/comparador
                  if (const int xb = xbItemBlock(s.id)) { { auto gv = gXbVar.find(s.id); gXbPlaceVar = gv != gXbVar.end() ? gv->second : 0; } if (!s.nbt.empty() && netSock >= 0 && !netHost) return;   /* convidado nao guarda conteudo de bau: nao coloca bau cheio (perderia os itens) */ gPlaceNbt = s.nbt; const bool plcd = !rpt && (isRs(xb) ? rsPlace(xb, rsItemVar(s.id), hh, pp, yaw, P.x, P.y, P.z) : xbPlace(xb, hh, pp, yaw, P.x, P.y, P.z)); gPlaceNbt.clear(); if (plcd) { swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); } return; } }
                { static int warnId = -1; if (!rpt && isPk(s.id) && warnId != s.id && !xbItemBlock(s.id) && PK[s.id - PK0].file.find("trapdoor") != std::string::npos) { warnId = s.id; logWarn("[PLACE] item %d '%s' (%s) nao esta ligado a bloco colocavel (gXbItem sem entrada): recarregue o pack/mundo e mande este log", s.id, PK[s.id - PK0].name.c_str(), PK[s.id - PK0].file.c_str()); } }   // patchLog.2: diagnostico do alcapao de ferro (registro e colocacao passam nos testes headless)
                if (isProp(s.id)) { if (!rpt && gGm != 2 && propPlace(s.id, hh[0], hh[1], hh[2], gRay.hit ? gRay.face : 3, yaw, P.x, P.y, P.z, P.sneak && gRay.hit ? gRay.hx : NAN, P.sneak && gRay.hit ? gRay.hz : NAN)) { swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); } return; }   // patchProp.2
                if (!s.id || !isBlk(s.id)) return;
                PlaceCtx pc{hh[0], hh[1], hh[2], 3, .5, .5, .5, yaw, pitch, P.sneak, P.x, P.y, P.z};
                if (gRay.hit && gRay.x == hh[0] && gRay.y == hh[1] && gRay.z == hh[2]) { pc.face = gRay.face; pc.fx = gRay.hx - hh[0]; pc.fy = gRay.hy - hh[1]; pc.fz = gRay.hz - hh[2]; }
                int tp[3]; if (!placeBlock(s.id, pc, tp)) return;
                gStPlaced++; sndEvtAt(sndBlockEvt(s.id, true), tp[0] + .5, tp[1] + .5, tp[2] + .5);   // colocar bloco: som de pedra mais grave (como o Java)
                swinging = true; swing = 0;
                if (isLeafB(s.id)) keepLeaves.insert(k3(tp[0], tp[1], tp[2]));
                if (!creative && --s.n <= 0) s = Slot();
            }
        };
        auto act = [&](int btn, bool rpt = false) {   // botao direito com a mao principal vazia usa o item da mao esquerda (colocar tocha/bloco)
            const bool oh = btn == SDL_BUTTON_RIGHT && gGm != 3 && !inv[sel].id && offh.id;
            if (oh) std::swap(inv[sel], offh);
            actMain(btn, rpt);
            if (oh) std::swap(inv[sel], offh);
        };
        auto uiVpNow = [&]() { return gInvOwner == 2 ? gInvK + 1 : 0; };   // patchSplit.4: viewport dono da UI aberta (0 = jogador 1)
        auto uiRectNow = [&](int& ox, int& oy, int& ow, int& oh) -> bool {   // retangulo (origem no topo) onde inventario/pausa/placa/chat/morte aparecem; true = so um viewport do split
            int W = 0, H = 0; SDL_GetWindowSize(win, &W, &H); ox = oy = 0; ow = W; oh = H;
            if (!gSplit || gScreen || !(pauseOpen || invOpen || gSignEd || chatOpen || (gDead && !creative))) return false;
            int x, y, w, h; split2Rect(uiVpNow(), W, H, x, y, w, h); ox = x; oy = H - (y + h); ow = w; oh = h; return true;
        };
        auto uiWinSize = [&](int* ww, int* wh) { int ox, oy; uiRectNow(ox, oy, *ww, *wh); };
        auto uiSync = [&]() { int ox, oy, ow, oh; gUiSubOn = uiRectNow(ox, oy, ow, oh); gUiOx = ox; gUiOy = oy; gUiOw = ow; gUiOh = oh; };   // para o cursor do controle (gamepad.inc)
        auto setInv = [&](bool on) {
            if (on && gGm == 3) return;   // espectador não tem inventário
            invReset();
            if (!on) { closeInv(); if (gInvOwner == 2) { split2SwapInvOwner(); gInvOwner = 0; } }   // inventario do jogador 2: devolve o conteudo do jogador 1 DEPOIS de guardar o crafting
            invOpen = on; grab = !on; relMouse(grab);
            if (on && gSplit) { int ox, oy, ow, oh; uiRectNow(ox, oy, ow, oh); mouseX = ow / 2; mouseY = oh / 2; SDL_WarpMouseInWindow(win, ox + mouseX, oy + mouseY); }   // cursor no centro do viewport do dono
        };
        auto setPause = [&](bool on) {   // menu de pausa: solta o mouse e congela o jogo (so no modo solo, como o Java); fechar devolve o mouse
            pauseOpen = on; grab = !on; relMouse(grab);
            if (on) { int ox, oy, ww, wh; uiRectNow(ox, oy, ww, wh); mouseX = ww / 2; mouseY = wh / 2; SDL_WarpMouseInWindow(win, ox + mouseX, oy + mouseY); }
        };
        auto setChat = [&](bool on, const char* pre, const char* skip) {   // abre/fecha o chat (solta o mouse enquanto digita)
            if (!on) gChatAs2 = false; gChatPad = on && gChatPadNext; gChatPadNext = false; if (chatOpen != on) netSendTyping(on && !*pre); chatOpen = on; chatIn = on ? pre : ""; chatSkip = on ? skip : ""; chatHistPos = -1; chatScroll = 0; chatCur = (size_t)-1; chatSelAll = false;
            grab = !on; relMouse(grab);
            if (on) SDL_StartTextInput();
#ifdef __ANDROID__
            else SDL_StopTextInput();   // fecha o teclado virtual ao sair do chat
#endif
        };
#ifdef __ANDROID__
        if (gSkinPickWait) { SDL_RWops* rw = SDL_RWFromFile("skin_pick.png", "rb"); if (rw) { SDL_RWclose(rw); gSkinPickWait = false; const bool ok = skinDropped("skin_pick.png"); remove("skin_pick.png"); gSkinDropT = SDL_GetTicks(); gSkinDropOk = ok; say(ok ? "Skin carregada" : "Skin inválida"); } }   // galeria nativa devolveu a imagem
#endif
#ifdef BL_RPI
        gLowF = invOpen ? invFitFactor() : 1.f;   // patchRpiOpt.8: inventario aberto = resolucao interna menor => painel enche a tela (UI continua em escala 1)
#endif
        { static Uint32 dscT = 0; const Uint32 tn = SDL_GetTicks(); if (tn - dscT > 2000) { dscT = tn; if (gScreen) dsc::set("No menu principal", "Blockland"); else dsc::set("Jogando Blockland", gJoin == NJ_INGAME ? "Multiplayer" : creative ? "Modo criativo" : "Modo sobreviv\u00eancia"); } }
        gProbeN = 0; PROBE("inicio");
