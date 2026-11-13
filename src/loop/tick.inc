// ticks fixos de 20/s (tickAcc): fisica, mobs, redstone, rede
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
        tickAcc += dt;
        int ticksThisFrame = 0; const int maxTk = gPfOtherMs > 25.0 ? 2 : 5;   // patchPerf.4: quadro lento (>25 ms de CPU) = no max. 2 ticks de recuperacao (5 ticks x ~8 ms engrossavam o quadro seguinte = espiral de lag)
        while (tickAcc >= 0.05 && ticksThisFrame < maxTk) {
            tickAcc -= 0.05; ++ticksThisFrame;
            if (gDead && !creative && !deadFree) { deadFree = true; if (invOpen) setInv(false); if (chatOpen) setChat(false, "", ""); grab = false; relMouse(false); int ox0, oy0, ww, wh; uiRectNow(ox0, oy0, ww, wh); mouseX = ww / 2; mouseY = wh / 2; SDL_WarpMouseInWindow(win, ox0 + mouseX, oy0 + mouseY); }   // solta o mouse para clicar nos botões
            if (gWantRespawn) { survRespawn(P); if (deadFree) { deadFree = false; if (!pauseOpen && !chatOpen && !invOpen) { grab = true; relMouse(true); } } }
            handBobTick(yaw, pitch);
            const double r2t = perfNow();
            PROBE("antes-mobs");
            tickMobs(P); dragonTick(P);
            tickDrops(P.x, P.y, P.z); sndAsyncPump();
            gR2[4] += perfNow() - r2t;
            if (gSplit) split2Tick(P, !gScreen);   // patchSplit.4: a UI do jogador 1 (inventario/pausa/chat/placa) nao congela os outros
            if (gP2ChatReq) { gP2ChatReq = false; if (gSplit && !chatOpen && !invOpen && !pauseOpen) { gChatK = gP2ChatReqK; gChatAs2 = true; gChatPadNext = true; setChat(true, "", ""); } }   // GA_CHAT do controle do jogador 2   // jogador 2 (splitscreen.inc)
            runWorldTick();   // ticks agendados de bloco (blockupdate.inc)
            netherTick(P); if (gNetherTurn) { gNetherTurn = false; yaw = gNetherYaw; pitch = gNetherPitch; }    // fogo queima o jogador; portal do Nether (nether.inc)
            modulesTick(P);
            netSendState(P.x, P.y, P.z, yaw, pitch, inv[sel].id, swinging ? swing : 0.f, P.sneak, (gGm != 3 && isElytraItem(armor[1].id) ? 1 : 0) | (gGm != 3 && isElytraItem(armor[1].id) && P.elytra && !P.onGround ? 2 : 0)); netTick();
            // Entity.onEntityUpdate: guarda o estado anterior
            P.px = P.x; P.py = P.y; P.pz = P.z;
            P.prevDistWalked = P.distWalked;
            { const int mc = gDim == 1 ? 2 : gDim == 2 ? 3 : creative ? 1 : 0; if (mc != gMusicCtx) { gMusicCtx = mc; if (!gSndMusicName.empty()) sndMusicStop(); gSndMusicT = 100; } }   // patchMus.1: troca de dimensao/modo => troca a playlist
            { int sb = getB((int)floor(P.x), (int)floor(P.y - .2), (int)floor(P.z)); if (sb == AIR) sb = getB((int)floor(P.x), (int)floor(P.y - 1.2), (int)floor(P.z)); gSndStepMat = sndMatOf(sb); }   // patchAud.2
            sndWorldAmbient(P); hopperTick(P.x, P.y, P.z); jukeTick(); sndSyncVol(); gSndOnG = P.onGround; sndTick(P.x, P.y + 1.62, P.z, yaw, P.distWalked, !P.sneak && !P.flying && !P.inWater && !P.noclip);   // ouvinte + passos + musica
            prevBodyYaw = bodyYaw; prevLimbAmt = limbAmt;
            prevCamBobYaw = camBobYaw; prevCamBobPitch = camBobPitch;
            prevFovMod = fovMod;
            ageTicks += 1.f;
            if (netOn && netHost && ++gTimeSyncCtr >= 20) { gTimeSyncCtr = 0; netSendTime(false); }   // hora do mundo para todos, 1x/s
            if (gDayCycle) ++gWorldTime;
            wxTick(P.x, P.y, P.z);   // patchWx.1   // relógio do mundo: 1 por tick (20/s), 24000 = 1 dia; para junto com o menu de pausa (dt = 0)
            if (sprintToggleTimer > 0) --sprintToggleTimer;
            P.inWater = inWaterNow(P); P.inLava = inLavaNow(P);
            if (P.inWater && !P.flying) waterPush(P);   // correnteza (Entity.handleWaterMovement)

            // MovementInputFromOptions
            const bool sneakKey = grab && (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT] || tvSneakT || gi1.sneak);
            const bool jumpKey = grab && !gDead && (k[SDL_SCANCODE_SPACE] || tvJump || gi1.jump);
            float moveForward = 0.f, moveStrafe = 0.f;   // strafe > 0 = direita
            if (grab && !gDead) {
                if (k[SDL_SCANCODE_W] || tvF) moveForward += 1.f;
                if (k[SDL_SCANCODE_S] || tvB) moveForward -= 1.f;
                if (k[SDL_SCANCODE_D] || tvR) moveStrafe += 1.f;
                if (k[SDL_SCANCODE_A] || tvL) moveStrafe -= 1.f;
                moveForward = std::max(-1.f, std::min(1.f, moveForward + gi1.fwd)); moveStrafe = std::max(-1.f, std::min(1.f, moveStrafe + gi1.str));   // stick analogico
            }
            if (sneakKey) { moveStrafe *= .3f; moveForward *= .3f; }
            if (gBowOn || gEatOn) { moveStrafe *= .2f; moveForward *= .2f; }   // carregando o arco: 20% da velocidade
            P.sneak = sneakKey;
            // Voo criativo: dois toques rapidos em ESPACO (7 ticks) ligam/desligam; Espaco sobe, Shift desce
            if (flyTimer > 0) --flyTimer;
            if (creative && !lastJumpKey && jumpKey) { if (flyTimer == 0) flyTimer = 7; else { P.flying = !P.flying; flyTimer = 0; } }
            lastJumpKey = jumpKey;
            if (!creative) P.flying = false;
            if (gGm == 3) P.flying = true;
            P.noclip = (gGm == 3);   // espectador atravessa blocos
            if (P.flying) { if (sneakKey) P.my -= .15; if (jumpKey) P.my += .15; }

            // EntityPlayerSP.onLivingUpdate: início/fim do sprint (Ctrl segurado ou toque duplo em W)
            const bool sprintKey = grab && (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL] || tvSprint || gGpSprint);
            const float F08 = .8f;
            const bool canSprintFood = creative || P.flying || gFood > 6;   // Java: fome > 6 (mais de 3 pernis) ou pode voar
            if (P.onGround && !lastSneakKey && !lastFwdOk && moveForward >= F08 && !P.sprint && canSprintFood) {
                if (sprintToggleTimer <= 0 && !sprintKey) sprintToggleTimer = 7; else P.sprint = true;
            }
            if (!P.sprint && moveForward >= F08 && sprintKey && canSprintFood) P.sprint = true;
            if (P.sprint && (moveForward < F08 || P.collH || !canSprintFood)) P.sprint = false;
            lastSneakKey = sneakKey; lastFwdOk = moveForward >= F08;

            // EntityLivingBase.onLivingUpdate
            if (jumpTicks > 0) --jumpTicks;
            if (fabs(P.mx) < .005) P.mx = 0;
            if (fabs(P.my) < .005) P.my = 0;
            if (fabs(P.mz) < .005) P.mz = 0;
            if (jumpKey) {
                if (P.inWater || P.inLava) P.my += .04;
                else if (P.onGround && jumpTicks == 0) {
                    P.my = fxJump();                             // jump() (patchPot.1: Salto)
                    survExh(P.sprint ? .8f : .2f);
                    if (P.sprint) { P.mx += -sy * .2; P.mz += -cy * .2; }   // impulso do pulo correndo
                    jumpTicks = 10;
                }
            } else jumpTicks = 0;
            moveStrafe *= .98f; moveForward *= .98f;
            // escada de mao (EntityLivingBase.moveEntityWithHeading): limita a velocidade, zera a queda; batendo na parede sobe a 0,2
            elytraBefore(P, jumpKey, yaw, pitch);
            const bool onLadder = !P.flying && climbBlk(getB((int)floor(P.x), (int)floor(P.y), (int)floor(P.z)));
            if (onLadder) { P.mx = std::max(-.15, std::min(.15, P.mx)); P.mz = std::max(-.15, std::min(.15, P.mz)); P.my = std::max(P.my, -.15); if (sneakKey && P.my < 0) P.my = 0; gFall = 0.f; }
            const double myPre = P.my;
            if (gTpHold > 0) { P.mx = P.my = P.mz = 0; gFall = 0.f; } else if (propSeatTick(P, sneakKey || jumpKey)) { gFall = 0.f; } else if (propSleepTick(P, sneakKey || jumpKey)) { gFall = 0.f; yaw = wrap180(gSleepYaw + 180.f); pitch = 0.f; camMode = 0; }   /* patchSleep.3: olha para o pe da cama, travado */ else { unstuckPlayer(P); moveEntityWithHeading(P, yaw, moveStrafe, moveForward); }   // /tp: parado ate o terreno de destino carregar
            { static int slimeId = -1; if (slimeId < 0) { auto sl = gPkByFile.find("block/slime_block"); slimeId = sl != gPkByFile.end() ? sl->second : 0; }   // bloco de slime: quica (Block.onLanded) e anula a queda
              if (slimeId && myPre < -.1 && !sneakKey && P.onGround && getB((int)floor(P.x), (int)floor(P.y - .01), (int)floor(P.z)) == slimeId) { P.my = -myPre; P.onGround = false; gFall = 0.f; } }
            if (onLadder && P.collH) P.my = (.2 - .08) * .98;   // 0,2 e a gravidade do mesmo tick
            survTick(P); fxTick(); potCloudsTick(P); { static int scT = 0; if (++scT >= 20) { scT = 0; scChestFlush(); } } mechCactus(P); mechFarmTick(P);
            elytraAfter(P);
            gBowSlot = &inv[sel]; gEnchHeld = &inv[sel]; bowTick(P, grab && !invOpen && !chatOpen && !pauseOpen, inv[sel].id, yaw, pitch); eatTick(P, grab && !invOpen && !chatOpen && !pauseOpen, inv[sel]); rodTick(P, grab && !invOpen && !chatOpen && !pauseOpen, inv[sel], yaw, pitch); arrowsTick(P); eggsTick(P); fwTick(); pearlApply(P); tntTick(P);
            if (P.onGround && P.flying) P.flying = false;

            // EntityPlayer.onLivingUpdate: balanço de câmera (view bobbing)
            {
                float f = (float)sqrt(P.mx * P.mx + P.mz * P.mz);
                float f1 = (float)(atan(-P.my * .2) * 15.0);
                if (f > .1f) f = .1f;
                if (!P.onGround) f = 0.f;
                if (P.onGround) f1 = 0.f;
                camBobYaw += (f - camBobYaw) * .4f;
                camBobPitch += (f1 - camBobPitch) * .8f;
            }

            // EntityLivingBase.onUpdate: corpo (renderYawOffset), cabeça e membros
            {
                const double d0 = P.x - P.px, d1 = P.z - P.pz;
                const float dsq = (float)(d0 * d0 + d1 * d1);
                float f1 = bodyYaw;
                if (dsq > .0025000002f) f1 = (float)(atan2(-d0, -d1) * 57.29577951308232);   // direção do movimento
                if (swinging) f1 = yaw;                                                         // atacando: corpo acompanha a mira
                // func_110146_f
                float f = wrap180(f1 - bodyYaw);
                bodyYaw += f * .3f;
                float h = wrap180(yaw - bodyYaw);
                if (h < -75.f) h = -75.f;
                if (h >= 75.f) h = 75.f;
                bodyYaw = yaw - h;
                if (h * h > 2500.f) bodyYaw += h * .2f;
                if (gSleepT > 0) { bodyYaw += wrap180(gSleepYaw - bodyYaw) * .5f; limbAmt = 0.f; }
                if (gSeated) { bodyYaw += wrap180(gSeatYaw - bodyYaw) * .5f; limbAmt = 0.f; }   // patchProp.6: sentado, o corpo fica virado para a frente da cadeira
                // evita saltos de 360 na interpolação
                while (bodyYaw - prevBodyYaw < -180.f) prevBodyYaw -= 360.f;
                while (bodyYaw - prevBodyYaw >= 180.f) prevBodyYaw += 360.f;

                float f6 = (float)sqrt(d0 * d0 + d1 * d1) * 4.f;
                if (f6 > 1.f) f6 = 1.f;
                limbAmt += (f6 - limbAmt) * .4f;
                limbSwing += limbAmt;
            }

            // EntityRenderer.updateRenderer: FOV do jogador (correr = x1.15)
            {
                const float target = P.sprint ? 1.15f : 1.f;
                fovMod += (target - fovMod) * .5f;
                fovMod = std::max(.1f, std::min(1.5f, fovMod));
            }

            // ----- bote (boat.inc): física dos botes e jogador montado -----
            {
                const bool inp = grab && !gDead;
                const bool bL = inp && (k[SDL_SCANCODE_A] || tvL || gi1.str < -.5f), bR = inp && (k[SDL_SCANCODE_D] || tvR || gi1.str > .5f), bF = inp && (k[SDL_SCANCODE_W] || tvF || gi1.fwd > .5f), bB = inp && (k[SDL_SCANCODE_S] || tvB || gi1.fwd < -.5f);
                if (cartTickAll(P, bF, sneakKey, yaw)) { bodyYaw = yaw; limbAmt = 0.f; gFall = 0.f; }
                if (vehTickAll(P, bL, bR, bF, bB, jumpKey, sneakKey)) { bodyYaw = yaw; limbAmt = 0.f; gFall = 0.f; }   // patchVeic.1: montado em veiculo
                if (vehTickAll(P, bL, bR, bF, bB, jumpKey, sneakKey)) { bodyYaw = yaw; limbAmt = 0.f; gFall = 0.f; }   // patchVeic.1
                static const int CAS = mobPkItem("item/carrot_on_a_stick") ? mobPkItem("item/carrot_on_a_stick") : mobPkItem("item/carrot_on_stick");
                if (horseTick(P, bF, bB, sneakKey, jumpKey, sprintKey, yaw, CAS && inv[sel].id == CAS)) { bodyYaw = yaw; limbAmt = 0.f; gFall = 0.f; }   // patchHorse.1: montado no cavalo
                if (boatTickAll(P, bL, bR, bF, bB, sneakKey)) {
                    const BoatEnt* rb = boatById(gBoatRide);
                    const float bo = rb ? boatYawO(*rb) : yaw;
                    bodyYaw = bo; limbAmt = 0.f; gFall = 0.f;
                    while (bodyYaw - prevBodyYaw < -180.f) prevBodyYaw -= 360.f;
                    while (bodyYaw - prevBodyYaw >= 180.f) prevBodyYaw += 360.f;
                }
            }
        }
