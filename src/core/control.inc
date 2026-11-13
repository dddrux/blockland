// patchCtl.1 (HZ-007 parcial): controle por arquivo para IA/script jogar e tirar print. Ativo so com --control DIR (ou BLOCKLAND_CTL=DIR).
// Protocolo: escrever DIR/cmd.txt (atomico: cmd.tmp + mv). 1 comando por linha: start <dir_mundo> | cmd /<comando de chat> | look <yaw> <pitch> | cam <0-2>
// | wait <quadros> | shot <nome> (grava DIR/<nome>.png do quadro seguinte) | state (grava DIR/state.txt) | quit. Ao terminar o lote grava DIR/done.txt.
// Captura = glReadPixels do framebuffer padrao (backend GL; no Vulkan vkglReadPixels devolve preto).
#pragma once
#include <fstream>
struct CtlSt { bool on = false; std::string dir, shot; std::deque<std::string> q; int wait = 0, batch = 0; bool busy = false, shotReq = false; };
static CtlSt gCtl;
static uint32_t ctlCrc(const uint8_t* p, size_t n, uint32_t c = 0) {
    static uint32_t T[256]; static bool init = false;
    if (!init) { for (uint32_t i = 0; i < 256; i++) { uint32_t v = i; for (int k = 0; k < 8; k++) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1; T[i] = v; } init = true; }
    c = ~c; for (size_t i = 0; i < n; i++) c = T[(c ^ p[i]) & 255] ^ (c >> 8); return ~c;
}
static void ctlPngChunk(std::vector<uint8_t>& o, const char* t, const std::vector<uint8_t>& d) {
    auto be = [&](uint32_t v) { for (int s = 24; s >= 0; s -= 8) o.push_back((uint8_t)(v >> s)); };
    be((uint32_t)d.size()); const size_t s0 = o.size(); o.insert(o.end(), t, t + 4); o.insert(o.end(), d.begin(), d.end());
    be(ctlCrc(o.data() + s0, o.size() - s0));
}
static bool ctlWritePng(const std::string& path, const uint8_t* rgb, int w, int h) {   // PNG RGB sem compressao (deflate "stored")
    std::vector<uint8_t> raw; raw.reserve((size_t)(w * 3 + 1) * h);
    for (int y = 0; y < h; y++) { raw.push_back(0); raw.insert(raw.end(), rgb + (size_t)y * w * 3, rgb + (size_t)(y + 1) * w * 3); }
    std::vector<uint8_t> z = {0x78, 0x01}; uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw.size(); i += 65535) {
        const size_t n = std::min<size_t>(65535, raw.size() - i); z.push_back(i + n >= raw.size());
        z.push_back(n & 255); z.push_back(n >> 8); z.push_back(~n & 255); z.push_back((~n >> 8) & 255); z.insert(z.end(), raw.begin() + i, raw.begin() + i + n);
    }
    for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    const uint32_t ad = (b << 16) | a; for (int s = 24; s >= 0; s -= 8) z.push_back((uint8_t)(ad >> s));
    std::vector<uint8_t> o = {0x89, 'P', 'N', 'G', 13, 10, 26, 10}, ih;
    auto be = [&](std::vector<uint8_t>& v, uint32_t x) { for (int s = 24; s >= 0; s -= 8) v.push_back((uint8_t)(x >> s)); };
    be(ih, w); be(ih, h); ih.insert(ih.end(), {8, 2, 0, 0, 0});
    ctlPngChunk(o, "IHDR", ih); ctlPngChunk(o, "IDAT", z); ctlPngChunk(o, "IEND", {});
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return false; const bool ok = fwrite(o.data(), 1, o.size(), f) == o.size(); fclose(f); return ok;
}
static void ctlInit(int argc, char** argv) {
    const char* e = getenv("BLOCKLAND_CTL"); if (e && *e) gCtl.dir = e;
    for (int i = 1; i + 1 < argc; i++) if (!strcmp(argv[i], "--control")) gCtl.dir = argv[i + 1];
    if (gCtl.dir.empty()) return;
    std::error_code ec; fs::create_directories(gCtl.dir, ec); gCtl.on = !ec; if (gCtl.on) logInfo("[CTL] controle por arquivo em %s", gCtl.dir.c_str());
}
static void ctlGrab(int w, int h) {   // chamar logo antes do swap, com o quadro pronto
    if (!gCtl.on || !gCtl.shotReq || w <= 0 || h <= 0) return;
    gCtl.shotReq = false; std::vector<uint8_t> px((size_t)w * h * 4), rgb((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1); glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { const uint8_t* s = &px[((size_t)(h - 1 - y) * w + x) * 4]; uint8_t* d = &rgb[((size_t)y * w + x) * 3]; d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; }
    const bool ok = ctlWritePng(gCtl.dir + "/" + gCtl.shot + ".png", rgb.data(), w, h); logInfo("[CTL] shot %s %dx%d %s", gCtl.shot.c_str(), w, h, ok ? "ok" : "ERRO");
}
// Chamar 1x por quadro no topo do laco. Retorna sem fazer nada se desligado.
static void ctlFrame(Phys& P, float& yaw, float& pitch, int& camMode, bool& run) {
    if (!gCtl.on) return;
    if (gCtl.q.empty() && !gCtl.shotReq && gCtl.wait <= 0) {
        if (gCtl.busy) { gCtl.busy = false; FILE* f = fopen((gCtl.dir + "/done.txt").c_str(), "w"); if (f) { fprintf(f, "%d ok\n", gCtl.batch); fclose(f); } }
        const std::string p = gCtl.dir + "/cmd.txt"; std::ifstream in(p); if (!in) return;
        std::string l; while (std::getline(in, l)) { while (!l.empty() && (l.back() == '\r' || l.back() == ' ')) l.pop_back(); if (!l.empty()) gCtl.q.push_back(l); }
        in.close(); std::error_code ec; fs::remove(p, ec); fs::remove(gCtl.dir + "/done.txt", ec); gCtl.batch++; gCtl.busy = true; return;
    }
    if (gCtl.wait > 0) { gCtl.wait--; return; }
    if (gCtl.shotReq) return;   // espera o quadro ser capturado antes do proximo comando
    while (!gCtl.q.empty()) {
        const std::string l = gCtl.q.front(); gCtl.q.pop_front(); const size_t sp = l.find(' '); const std::string c = l.substr(0, sp), a = sp == std::string::npos ? "" : l.substr(sp + 1);
        if (c == "wait") { gCtl.wait = std::max(0, atoi(a.c_str())); return; }
        else if (c == "shot") { gCtl.shot = a.empty() ? "shot" : a; gCtl.shotReq = true; return; }
        else if (c == "cmd") { if (gScreen == SC_PLAY && !a.empty() && a[0] == '/') execCommand(a, P); }
        else if (c == "look") { float y = yaw, p = pitch; if (sscanf(a.c_str(), "%f %f", &y, &p) >= 1) { yaw = y; pitch = std::max(-90.f, std::min(90.f, p)); } }
        else if (c == "cam") camMode = ((atoi(a.c_str()) % 3) + 3) % 3;
        else if (c == "start") { if (gScreen != SC_PLAY) { if (!a.empty()) gMenuDir = a; leavePages(); gMenuAct = 1; return; } }
        else if (c == "state") { FILE* f = fopen((gCtl.dir + "/state.txt").c_str(), "w"); if (f) { fprintf(f, "screen=%d x=%.2f y=%.2f z=%.2f yaw=%.1f pitch=%.1f cam=%d gm=%d\n", gScreen, P.x, P.y, P.z, yaw, pitch, camMode, (int)gGm); fclose(f); } }
        else if (c == "quit") { run = false; }
    }
}
