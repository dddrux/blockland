// ================= logger: sistema de logs no formato do Minecraft Java =================
// "[HH:MM:SS] [Render thread/INFO]: mensagem" no console E em logs/latest.log (o anterior vira logs/AAAA-MM-DD-N.log).
// printf(...) vira INFO e fprintf(stderr, ...) vira ERROR (AVISO/WARN no inicio vira WARN) -- todo o codigo antigo ja passa por aqui.
// Falhas fatais (SIGSEGV, SIGABRT, excecao nao tratada) geram crash-reports/crash-AAAA-MM-DD_HH.MM.SS-client.txt com as ultimas linhas do log.
// Uso novo: logInfo("fmt", ...), logWarn(...), logError(...), logFatal(...). Erros de OpenGL: logGlErrors("onde").
#include <cstdarg>
#include <csignal>
#include <exception>
#include <sys/stat.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <execinfo.h>
#endif
enum LogLevel { LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };
static FILE* gLogFile = nullptr;
static std::string gLogBase;   // pasta raiz de logs/ e crash-reports/ (vazio = pasta atual). No Android: Documents/Blockland/ (facil de achar no gerenciador de arquivos)
static char gLogRing[48][512]; static int gLogRingN = 0;   // ultimas linhas (vao para o crash report)
static const char* const LOG_NAMES[4] = {"INFO", "WARN", "ERROR", "FATAL"};
static void logPutLine(LogLevel lv, const char* msg) {
    const time_t t = time(nullptr); struct tm tmv; 
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char line[1024]; snprintf(line, sizeof line, "[%02d:%02d:%02d] [Render thread/%s]: %s", tmv.tm_hour, tmv.tm_min, tmv.tm_sec, LOG_NAMES[lv], msg);
    snprintf(gLogRing[gLogRingN++ % 48], sizeof gLogRing[0], "%s", line);
    FILE* o = lv >= LOG_ERROR ? stderr : stdout; fputs(line, o); fputc('\n', o); fflush(o);
    if (gLogFile) { fputs(line, gLogFile); fputc('\n', gLogFile); fflush(gLogFile); }
}
static void logV(LogLevel lv, const char* fmt, va_list ap) {   // quebra em linhas (como o Minecraft: cada linha ganha o prefixo)
    char buf[2048]; vsnprintf(buf, sizeof buf, fmt, ap);
    char* p = buf;
    for (;;) {
        char* nl = strchr(p, '\n'); if (nl) *nl = 0;
        if (*p || !nl) logPutLine(lv, p);
        if (!nl) break; p = nl + 1; if (!*p) break;
    }
}
static void logInfo(const char* fmt, ...) { va_list a; va_start(a, fmt); logV(LOG_INFO, fmt, a); va_end(a); }
static void logWarn(const char* fmt, ...) { va_list a; va_start(a, fmt); logV(LOG_WARN, fmt, a); va_end(a); }
static void logError(const char* fmt, ...) { va_list a; va_start(a, fmt); logV(LOG_ERROR, fmt, a); va_end(a); }
static void logFatal(const char* fmt, ...) { va_list a; va_start(a, fmt); logV(LOG_FATAL, fmt, a); va_end(a); }
static int mcPrintf(const char* fmt, ...) { va_list a; va_start(a, fmt); logV(LOG_INFO, fmt, a); va_end(a); return 0; }
static int mcFprintf(FILE* f, const char* fmt, ...) {
    va_list a; va_start(a, fmt);
    if (f == stderr) { const bool w = !strncmp(fmt, "AVISO", 5) || !strncmp(fmt, "WARN", 4); logV(w ? LOG_WARN : LOG_ERROR, fmt, a); }
    else vfprintf(f, fmt, a);
    va_end(a); return 0;
}
#define printf(...) mcPrintf(__VA_ARGS__)
#define fprintf(...) mcFprintf(__VA_ARGS__)

// patchShader.5: pilhas de matriz/atributos de GL vazando (push sem pop) estouravam (GL 1283) e o mundo sumia (0 chunks, tela lisa). Verifica, avisa 1x por ponto e DESFAZ o vazamento.
#ifndef GL_MODELVIEW_STACK_DEPTH
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#endif
#ifndef GL_PROJECTION_STACK_DEPTH
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#endif
#ifndef GL_ATTRIB_STACK_DEPTH
#define GL_ATTRIB_STACK_DEPTH 0x0BB0
#endif
#ifndef GL_CLIENT_ATTRIB_STACK_DEPTH
#define GL_CLIENT_ATTRIB_STACK_DEPTH 0x0BB1
#endif
#ifndef GL_MATRIX_MODE
#define GL_MATRIX_MODE 0x0BA0
#endif
static void glStackHeal(const char* where) {
#ifndef __ANDROID__
    static int warned = 0; GLint mv = 1, pj = 1, at = 0, ca = 0;
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &mv); glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &pj); glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &at); glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, &ca);
    if (mv <= 1 && pj <= 1 && at <= 0 && ca <= 0) return;
    if (warned < 6) { warned++; logWarn("[GL] pilha vazada em '%s': modelview %d, projecao %d, attrib %d, client %d (corrigido)", where, (int)mv, (int)pj, (int)at, (int)ca); }
    GLint m0 = GL_MODELVIEW; glGetIntegerv(GL_MATRIX_MODE, &m0);
    glMatrixMode(GL_MODELVIEW); for (; mv > 1; mv--) glPopMatrix();
    glMatrixMode(GL_PROJECTION); for (; pj > 1; pj--) glPopMatrix();
    glMatrixMode((GLenum)m0);
    for (; at > 0; at--) glPopAttrib(); for (; ca > 0; ca--) glPopClientAttrib();
    while (glGetError() != GL_NO_ERROR) {}
#else
    (void)where;
#endif
}
static void logGlErrors(const char* where) {   // como o Minecraft: "########## GL ERROR ##########" (no maximo 1x a cada 5 s por codigo)
    static Uint32 last[8] = {0};
    for (int i = 0; i < 8; i++) {
        const unsigned e = glGetError(); if (!e) break;
        const unsigned k = e >= 0x500 && e <= 0x507 ? e - 0x500 : 7; const Uint32 now = SDL_GetTicks();
        if (last[k] && now - last[k] < 5000) continue; last[k] = now;
        const char* n = e == 0x500 ? "Invalid enum" : e == 0x501 ? "Invalid value" : e == 0x502 ? "Invalid operation" : e == 0x503 ? "Stack overflow" : e == 0x504 ? "Stack underflow" : e == 0x505 ? "Out of memory" : e == 0x506 ? "Invalid framebuffer operation" : "Unknown";
        logError("########## GL ERROR ##########"); logError("@ %s", where); logError("%u: %s", e, n);
    }
}

// ---- logs datados: UM arquivo por sessao, com a data/hora em que o jogo FECHOU (logs/AAAA-MM-DD_HH.MM.SS.log) ----
// latest.log e o log vivo (gravado linha a linha). Ao fechar (normal, crash, sinal, excecao, sistema/Android) ele e copiado para o arquivo datado.
// Se o processo for morto sem aviso (kill, bateria, "limpar apps"), logs/.open fica no disco e o PROXIMO inicio gera o arquivo datado
// usando a hora da ultima linha gravada (mtime do latest.log) como data de fechamento. Nunca apaga logs de outras sessoes.
static bool gLogFinal = false; static std::string gLogCopy;   // gLogCopy: arquivo datado desta sessao (atualizado a cada sincronizacao)
static std::string logDatedPath(time_t t, const std::string& reuse) {
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::error_code ec; std::string nm;
    for (int i = 0; i < 1000; i++) {
        char b[64]; if (i) snprintf(b, sizeof b, "%04d-%02d-%02d_%02d.%02d.%02d-%d.log", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, i);
        else snprintf(b, sizeof b, "%04d-%02d-%02d_%02d.%02d.%02d.log", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        nm = gLogBase + "logs/" + b; if (nm == reuse || !std::filesystem::exists(nm, ec)) break;
    }
    return nm;
}
static void logSyncSession(time_t when = 0) {   // copia o log vivo para o arquivo datado da sessao (troca o nome antigo pelo novo)
    if (gLogFile) fflush(gLogFile);
    std::error_code ec; const std::string live = gLogBase + "logs/latest.log"; if (!std::filesystem::exists(live, ec)) return;
    const std::string nm = logDatedPath(when ? when : time(nullptr), gLogCopy);
    std::filesystem::copy_file(live, nm, std::filesystem::copy_options::overwrite_existing, ec); if (ec) return;
    if (!gLogCopy.empty() && gLogCopy != nm) std::filesystem::remove(gLogCopy, ec);
    gLogCopy = nm;
    if (FILE* m = fopen((gLogBase + "logs/.open").c_str(), "wb")) { fputs(nm.c_str(), m); fclose(m); }   // sessao aberta: aponta para a copia atual
}
static void logFinalize(const char* why) {   // fim da sessao (qualquer forma): grava a linha final, gera o arquivo datado e fecha
    if (gLogFinal) return; gLogFinal = true;
    logInfo("Log encerrado (%s)", why);
    logSyncSession();
    std::error_code ec; std::filesystem::remove(gLogBase + "logs/.open", ec);
    if (gLogFile) { fclose(gLogFile); gLogFile = nullptr; }
}
static void logAtExit() { logFinalize("saida do processo"); }
static bool logTailHas(const std::string& path, const char* needle) {
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return false; char b[512] = {0};
    fseek(f, 0, SEEK_END); const long n = ftell(f); fseek(f, n > 400 ? n - 400 : 0, SEEK_SET); const size_t k = fread(b, 1, 400, f); b[k] = 0; fclose(f);
    return strstr(b, needle) != nullptr;
}
static void logCrashReport(const char* what) {
    std::error_code ec; std::filesystem::create_directories(gLogBase + "crash-reports", ec);
    const time_t t = time(nullptr); struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char name[160]; snprintf(name, sizeof name, "%scrash-reports/crash-%04d-%02d-%02d_%02d.%02d.%02d-client.txt", gLogBase.c_str(), tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    FILE* f = fopen(name, "wb"); if (!f) return;
    fprintf(f, "---- Blockland Crash Report ----\nTime: %04d-%02d-%02d %02d:%02d:%02d\nDescription: %s\nVersion: %s\n\n-- Ultimas linhas do log --\n", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, what, BLOCKLAND_VERSION);
    const int n = std::min(gLogRingN, 48);
    for (int i = gLogRingN - n; i < gLogRingN; i++) { fputs(gLogRing[i % 48], f); fputc('\n', f); }
#if defined(__linux__) && !defined(__ANDROID__)
    void* bt[40]; const int nb = backtrace(bt, 40); fputs("\n-- Stacktrace --\n", f); fflush(f); backtrace_symbols_fd(bt, nb, fileno(f));
#endif
    fclose(f); logFatal("Este relatorio de falha foi salvo em: %s", name);
}
static void logSignal(int sg) {
    static bool in = false; if (in) _Exit(3); in = true;
    const char* n = sg == SIGSEGV ? "SIGSEGV (acesso invalido a memoria)" : sg == SIGABRT ? "SIGABRT (abortado)" : sg == SIGFPE ? "SIGFPE (erro aritmetico)" : "SIGILL (instrucao ilegal)";
    logFatal("O jogo falhou! Sinal fatal: %s", n); logCrashReport(n); logFinalize("falha fatal"); _Exit(1);
}
static void logTerminate() {
    const char* what = "excecao C++ nao tratada";
    try { if (auto ep = std::current_exception()) std::rethrow_exception(ep); } catch (const std::exception& e) { logFatal("Excecao nao tratada: %s", e.what()); logCrashReport(e.what()); logFinalize("excecao nao tratada"); _Exit(1); } catch (...) {}
    logFatal("O jogo falhou! %s", what); logCrashReport(what); logFinalize("falha fatal"); _Exit(1);
}
static void logInit() {   // chamar no inicio do main
    std::error_code ec;
#ifdef __ANDROID__
    for (const char* b : {"/storage/emulated/0/Documents/Blockland/", "/sdcard/Documents/Blockland/", "/storage/emulated/0/Blockland/"}) {   // 1a pasta gravavel vence; senao fica na pasta do app
        std::error_code e2; std::filesystem::create_directories(std::string(b) + "logs", e2);
        if (FILE* t = fopen((std::string(b) + "logs/.t").c_str(), "wb")) { fclose(t); remove((std::string(b) + "logs/.t").c_str()); gLogBase = b; break; }
    }
#endif
    std::filesystem::create_directories(gLogBase + "logs", ec);
    {   // sessao anterior terminou sem avisar (kill/bateria/limpar apps)? gera o arquivo datado com a hora da ultima linha gravada
        const std::string live = gLogBase + "logs/latest.log", mk = gLogBase + "logs/.open";
        if (std::filesystem::exists(live, ec)) {
            std::string prevCopy; bool abrupt = false;
            if (FILE* m = fopen(mk.c_str(), "rb")) { char b[512] = {0}; if (fgets(b, sizeof b, m)) prevCopy = b; fclose(m); abrupt = true; }
            else abrupt = !logTailHas(live, "Log encerrado");   // sem marcador .open: so arquiva se o fim do log nao tem a linha de encerramento (logs de versoes antigas)
            if (abrupt) {
                struct stat st; const time_t when = stat(live.c_str(), &st) == 0 ? st.st_mtime : time(nullptr);
                const std::string nm = logDatedPath(when, prevCopy);
                std::filesystem::copy_file(live, nm, std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec && !prevCopy.empty() && prevCopy != nm) std::filesystem::remove(prevCopy, ec);
            }
        }
        std::filesystem::remove(mk, ec);
    }
    gLogFile = fopen((gLogBase + "logs/latest.log").c_str(), "wb");
    signal(SIGSEGV, logSignal); signal(SIGABRT, logSignal); signal(SIGFPE, logSignal); signal(SIGILL, logSignal);
    std::set_terminate(logTerminate); atexit(logAtExit);
    const char* os =
#if defined(_WIN32)
        "Windows";
#elif defined(__APPLE__)
        "macOS";
#else
        "Linux";
#endif
    logInfo("Iniciando %s (SO: %s)", GAME_VERSION_TEXT, os); logInfo("Pasta dos logs: %slogs/latest.log", gLogBase.empty() ? "./" : gLogBase.c_str());
    if (!gLogFile) logWarn("Nao foi possivel criar logs/latest.log (so console)");
    logSyncSession();   // ja deixa um arquivo datado desta sessao (sera renomeado com a hora real do fechamento)
}
