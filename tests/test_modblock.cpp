// test_modblock: o que este teste cobre (1 linha). Rodar: tools/t.sh modblock
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    // jar sintetico (bloco cubo "rb") -> tools/instalar_mods.sh -> jogo registra aba + item colocavel
    const std::string root = std::filesystem::absolute("..").string(), T = "/tmp/blmodblock"; std::filesystem::remove_all(T); std::filesystem::create_directories(T + "/mods");
    const std::string py = "python3 - <<'EOF'\nimport zipfile,json,io\nfrom PIL import Image\nb=io.BytesIO(); Image.new('RGBA',(16,16),(200,30,30,255)).save(b,'PNG')\n"
        "z=zipfile.ZipFile('" + T + "/mods/Ruby-1.12.2.jar','w'); P=lambda n,d:z.writestr('assets/ruby/'+n,json.dumps(d))\n"
        "z.writestr('assets/ruby/textures/blocks/rb.png',b.getvalue()); P('blockstates/rb.json',{'variants':{'normal':{'model':'ruby:rb'}}}); P('models/block/rb.json',{'parent':'block/cube_all','textures':{'all':'ruby:blocks/rb'}})\n"
        "z.writestr('assets/ruby/lang/en_us.lang','tile.rb.name=Ruby Block\\n'); z.close()\nEOF";
    if (system(py.c_str()) != 0) { printf("SKIP (python3/PIL)\n"); return 0; }
    if (system(("cd " + T + " && mkdir -p tools && cp " + root + "/tools/instalar_mods.sh tools/ && ln -s " + root + "/conversor conversor && bash tools/instalar_mods.sh >/dev/null 2>&1").c_str()) != 0) { printf("instalador falhou\n"); return 1; }
    SECTION("T1 instalador gerou o mod");
    CHECK(std::filesystem::exists(T + "/mods/ruby/props.bin")); CHECK(std::filesystem::exists(T + "/mods/ruby/mod.txt"));
    if (chdir(T.c_str()) != 0) return 99;
    SECTION("T2 jogo registra aba e item");
    const size_t tabs0 = TABS.size(); propsRegisterTab();
    CHECK(gPropPacks.size() == 1); CHECK(gPropTotal == 1); CHECK(TABS.size() == tabs0 + 1); CHECK(MODS.size() >= 1 && MODS.back().ids.size() == 1);
    const int id = PROP0; CHECK(isProp(id)); CHECK(std::string(propName(id)) == "Ruby Block");
    SECTION("T3 colocavel (cubo = bloco de grade, nao sprite de item)");
    CHECK(propGridBlock(id)); CHECK(!propModFlat(id));
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
