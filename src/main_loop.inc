// main(): esqueleto. O corpo vive em fragmentos por fase (src/loop/*.inc), incluidos AQUI dentro da funcao: init, frame_begin, events, menus_timing, tick, render.
// Funcoes auxiliares: use_items.inc, render/gpu_chunks.inc, render/sky_panorama.inc (incluidos antes, em main.cpp). Patch: patchOrg.4
int main(int argc, char** argv) {
#include "loop/init.inc"
    while (run) {
#include "loop/frame_begin.inc"
#include "loop/events.inc"
#include "loop/menus_timing.inc"
#include "loop/tick.inc"
#include "loop/render.inc"
    }
    logInfo("Stopping!");
    logInfo("Tempo de sessao: %.1f s", SDL_GetTicks() / 1000.0);
    dsc::shutdown(); modulesShutdown();
    netClose();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    logFinalize("fechado pelo jogador");
    fflush(nullptr); _Exit(0);   // patchExit.1: tudo ja foi salvo/fechado acima; pular os destrutores estaticos evita o SIGABRT (free invalido) que derrubava o jogo DEPOIS de fechar (crash-2026-11-06_17.02.13). Mesmo padrao do splash.inc
}
