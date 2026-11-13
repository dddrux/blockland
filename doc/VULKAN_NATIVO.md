# Vulkan nativo (plano, patchVk.2)

Objetivo: backend Vulkan PROPRIO, sem a camada OpenGL->Vulkan (`src/vkgl.inc`), com os mesmos efeitos do OpenGL (sombras, reflexo da agua, god rays, lens flare). Escolha OpenGL/Vulkan ja funciona no PC por 2 executaveis (patchVk.1).

## Auditoria (grep em src/, patchVk.2)
- ~3300 chamadas `gl*` (119 funcoes distintas) em ~100 arquivos; maiores: main_loop.inc 437, player_render.inc 350, menu.inc 178, dragon.inc 163, sky_cycle.inc 153, splash.inc 124, shadows.inc 101, inventory_ui.inc 99.
- Estilo: OpenGL 1.x de funcao fixa (glBegin/glVertex, matrizes glTranslatef/glRotatef, glEnable/glDisable ~520x, glBlendFunc, glAlphaFunc, neblina GL_FOG). Nao ha shader proprio alem de sombras/agua/god rays (GLSL em `shadows.inc`, `godrays.inc`, `sky_cycle.inc`, `world.inc`).
- Pontos sensiveis: glCopyTexSubImage2D (9, reflexo/sombras), stencil (14), glReadPixels/glGet* (~60, viram estado do renderer), FBOs (sombra, reflexo, god rays), glCallList (5).

## Fases (um patch por vez, cada uma compila e roda em OpenGL como hoje)
1. Interface `Renderer` (src/render/renderer.h): estado (blend, depth, alpha, fog, cull, scissor, viewport), matrizes (push/pop/translate/rotate/scale/ortho/persp), textura (criar/atualizar/bind), buffer de vertices do mundo (V = 36 bytes), imediato (`begin/vertex/end` em lote), FBO/render target, pass (clear, colorMask). Backend `RendererGL` = as chamadas atuais, migrando arquivo por arquivo (comecar por world/main_loop; UI e entidades depois). Regra: o resto do jogo NAO chama mais gl* direto.
2. `RendererVK` nativo: instancia/swapchain, 1 pipeline por combinacao de estado (cache), push constants para matriz/neblina/alpha, ring buffer para imediato, texturas com mipmap, descritores por textura. Shaders SPIR-V proprios (nao os de vkgl). Primeiro: mundo + UI + entidades.
3. Efeitos: shadow map (render pass proprio), reflexo/agua, god rays, lens flare, ceu/estrelas, particulas. So entao remover `vkgl.inc` e a macro `BL_VULKAN`.

## Progresso
- patchVk.3 (fase 1, passo 1): `src/render/renderer.inc` com rd* para a pilha de matrizes/projecao (MatrixMode, LoadIdentity, Push/Pop, Translatef, Rotatef, Scalef, Ortho, Frustum, LoadMatrixf); 612 chamadas migradas em todos os .inc (exceto vkgl.inc/glesnative.inc, que implementam gl*). Repassa ao OpenGL/vkgl, comportamento igual.
- patchVk.4 (fase 1, passo 2): estado migrado para rd* em `renderer.inc` (Enable/Disable, BlendFunc, AlphaFunc, DepthMask/Func, ColorMask, Fogf/i/fv, CullFace, FrontFace, Scissor, Viewport); 713 chamadas em 31 arquivos. Comportamento igual (repassa ao GL/vkgl).
- Proximos passos da fase 1: texturas, buffer do mundo (bindDraw), imediato (glBegin/glVertex), FBO/render target.

## O que precisa do usuario
- Headers/SDK Vulkan + glslang no ambiente de build e uma GPU para conferir (VIS) cada fase; sem isso so da para garantir que compila o OpenGL.
- Confirmar a ordem (mundo+UI primeiro, efeitos depois) e se a camada vkgl.inc pode ser mantida ate a fase 3.
- patchVkRT.1: build UNICA no PC. `-DBL_VULKAN` = Vulkan compilado; `gVkOn` (options.cfg renderer=, lido antes da janela) = ativo. libvulkan carregado em runtime (vkgl.inc: ponteiros p_vk*). Fallback: reabre o mesmo exe com renderer=opengl. Indices de chunk ficam 16 bits (QIdx) tambem no GL desta build.
