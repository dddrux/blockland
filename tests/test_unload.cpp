// patchUnload.1: descarregador de chunks (chunkUnload) + save inclui chunks descarregados. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    RAD = 4; W.clear(); savedC.clear(); savedM.clear(); getCReset(); remotes.clear();
    gen(0, 0); gen(30, 0); gen(31, 0);
    CHECK(getC(0, 0) && getC(30, 0) && getC(31, 0));
    Chunk* far0 = getC(30, 0); const int I = idx(3, 70, 5); far0->b[I] = (BT)OBSIDIAN; far0->m[I] = 9;   // edicao do jogador num chunk que vai sair
    std::vector<BT> ref(far0->b, far0->b + (size_t)CW * CH * CW); std::vector<uint8_t> refm(far0->m, far0->m + (size_t)CW * CH * CW);
    CHECK(chunkUnload(0, 0, 1) == 1);   // limite por chamada
    CHECK(chunkUnload(0, 0, 5) == 1);   // o outro
    CHECK(!getC(30, 0) && !getC(31, 0) && getC(0, 0));   // longe saiu, perto fica
    CHECK(savedC.count(key(30, 0)) && savedM.count(key(30, 0)));
    CHECK(chunkUnload(0, 0, 5) == 0);   // nada mais para descarregar
    // jogador "remoto" perto do chunk 30: nao descarrega
    gen(30, 0); RemotePl r; r.init = true; r.x = 30 * 16 + 8; r.y = 100; r.z = 8; remotes[1] = r;
    CHECK(chunkUnload(0, 0, 5) == 0 && getC(30, 0));
    remotes.clear(); CHECK(chunkUnload(0, 0, 5) == 1 && !getC(30, 0));
    // recarrega de savedC: blocos e meta identicos, incluindo a edicao
    gen(30, 0); Chunk* back = getC(30, 0);
    CHECK(back && back->b[I] == (BT)OBSIDIAN && back->m[I] == 9 && memcmp(back->b, ref.data(), ref.size() * sizeof(BT)) == 0 && memcmp(back->m, refm.data(), refm.size()) == 0);
    // save: W (so o chunk 0 e 30) + savedC (31 descarregado) => 3 chunks no arquivo
    CHECK(chunkUnload(0, 0, 5) == 1);   // 30 sai de novo
    const std::string path = "/tmp/test_unload_chunks.dat"; writeChunkFile(path, W, &savedC, &savedM);
    FILE* f = fopen(path.c_str(), "rb"); CHECK(f != nullptr); int32_t np = 0, n = 0; char mg[4];
    if (f) { fread(mg, 1, 4, f); fread(&np, 4, 1, f); for (int i = 0; i < np; i++) { uint16_t l; fread(&l, 2, 1, f); fseek(f, l, SEEK_CUR); } fread(&n, 4, 1, f); fclose(f); }
    CHECK(n == 3);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
