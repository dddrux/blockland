// ---------- Android: camada de traducao OpenGL, assets do APK e controles mobile ----------
// O jogo usa OpenGL 1.x (modo imediato, matrizes, luz/neblina fixas). O Android so tem OpenGL ES, entao
// linkamos com o gl4es (libGL.so), que traduz TODAS as chamadas GL 1.x/2.x para GLES2 em tempo de execucao.
// O SDL cria o contexto GLES2; o gl4es roda por cima dele. Nenhuma chamada gl* do jogo foi alterada.
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef BL_VULKAN
extern "C" {
    void initialize_gl4es(void) __attribute__((weak));
    void set_getprocaddress(void* (*)(const char*)) __attribute__((weak));
}
static void* blRawProc(const char* n) { return SDL_GL_GetProcAddress(n); }   // eglGetProcAddress (funcoes GLES reais)
// Extensoes (VBO, shaders...) tem de vir do gl4es, nao do GLES cru, senao pulariam a traducao:
static void* blGetProc(const char* n) {
    static void* h = dlopen("libGL.so", RTLD_NOW | RTLD_NOLOAD);
    if (!h) h = dlopen("libGL.so", RTLD_NOW);
    void* f = h ? dlsym(h, n) : nullptr;
    return f ? f : blRawProc(n);
}
#define SDL_GL_GetProcAddress blGetProc
#define SDL_GL_SwapWindow(w) do { glFlush(); (SDL_GL_SwapWindow)(w); } while (0)   // gl4es agrupa glBegin/glEnd: descarrega antes de trocar o buffer
#endif   // !BL_VULKAN
#define BL_ANDROID_WINFLAGS (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS)

static void androidGLAttrs() {   // contexto GLES2 para o gl4es
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "0");
#ifndef BL_VULKAN
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
}
static void androidGLInit() {    // depois de SDL_GL_CreateContext
#ifndef BL_VULKAN
    if (set_getprocaddress) set_getprocaddress(blRawProc);
    if (initialize_gl4es) initialize_gl4es();
#endif
}

// Copia assets/ de dentro do APK para o armazenamento do app (o jogo le arquivos por caminho relativo) e faz chdir para la.
// bl_manifest.txt (gerado por build-android.sh): linha 1 = carimbo da versao, demais = um caminho relativo por linha.
static void androidPrepare() {
    setenv("LIBGL_ES", "2", 1); setenv("LIBGL_FASTMATH", "1", 1); setenv("LIBGL_NOERROR", "1", 1); setenv("LIBGL_NPOT", "3", 1);
    const char* p = SDL_AndroidGetExternalStoragePath(); if (!p) p = SDL_AndroidGetInternalStoragePath();
    if (!p) return;
    if (chdir(p) != 0) return;
    for (const char* d : {"saves", "logs", "mods"}) mkdir(d, 0755);
    auto readAll = [](const char* n, std::string& o) { SDL_RWops* r = SDL_RWFromFile(n, "rb"); if (!r) return false; o.clear(); char b[65536]; size_t k; while ((k = SDL_RWread(r, b, 1, sizeof b)) > 0) o.append(b, k); SDL_RWclose(r); return true; };
    std::string man; if (!readAll("bl_manifest.txt", man)) return;
    const size_t nl = man.find('\n'); const std::string stamp = man.substr(0, nl);
    std::string old; if (FILE* f = fopen(".bl_stamp", "rb")) { char b[128] = {0}; if (fgets(b, sizeof b, f)) old = b; fclose(f); }
    if (old == stamp && std::filesystem::exists("assets")) return;   // ja extraido nesta versao
    size_t pos = nl == std::string::npos ? man.size() : nl + 1;
    while (pos < man.size()) {
        size_t e = man.find('\n', pos); if (e == std::string::npos) e = man.size();
        std::string rel = man.substr(pos, e - pos); pos = e + 1;
        while (!rel.empty() && (rel.back() == '\r' || rel.back() == ' ')) rel.pop_back();
        if (rel.empty() || (rel == "assets/resourcepacks/pack.cfg" && std::filesystem::exists(rel))) continue;   // preserva a escolha de pack do jogador
        std::error_code ec; std::filesystem::create_directories(std::filesystem::path(rel).parent_path(), ec);
        SDL_RWops* in = SDL_RWFromFile(rel.c_str(), "rb"); if (!in) continue;
        { const Sint64 isz = SDL_RWsize(in); std::error_code e2; if (isz >= 0 && std::filesystem::exists(rel, e2) && (Sint64)std::filesystem::file_size(rel, e2) == isz) { SDL_RWclose(in); continue; } }   // mesmo tamanho = ja extraido: nao recopia (o resource pack de 12 MB so e copiado 1 vez)
        if (FILE* out = fopen(rel.c_str(), "wb")) { static char b[1 << 16]; size_t k; while ((k = SDL_RWread(in, b, 1, sizeof b)) > 0) fwrite(b, 1, k, out); fclose(out); }
        SDL_RWclose(in);
    }
    if (!std::filesystem::exists("options.cfg")) if (FILE* f = fopen("options.cfg", "wb")) { fputs("renderdist=6\nrenderer=opengl\nrenderer_set=0\n", f); fclose(f); }   // distancia menor no celular (muda em Video > Renderizacao)
    if (FILE* f = fopen(".bl_stamp", "wb")) { fputs(stamp.c_str(), f); fclose(f); }
}
