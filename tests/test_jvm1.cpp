// jvm estagio 1: parser de .class + leitor de .jar
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
static void splashPoll() {} static void splashNote(const char*) {}
#include "../src/core/png.inc"
#include "../src/modules/jvm/jvm_class.inc"
#include "../src/modules/jvm/jvm_zip.inc"
static int fails = 0;
#define CHK(c) do { if (!(c)) { printf("FALHOU: %s (linha %d)\n", #c, __LINE__); fails++; } } while (0)
int main(int argc, char** argv) {
    const char* jp = argc > 1 ? argv[1] : "/tmp/t1/demo.jar"; if (access(jp, F_OK) != 0) { printf("SKIP: jar de demonstracao ausente (%s; precisa de javac para gerar)\n", jp); return 0; }
    jv::Jar j; CHK(j.open(jp));
    const jv::Jar::Ent* e = j.find("demo/Mod1.class"); CHK(e);
    std::vector<uint8_t> b; CHK(e && j.read(*e, b));
    jv::ClassFile c; CHK(jv::parseClass(b.data(), b.size(), c));
    CHK(c.name == "demo/Mod1"); CHK(c.super == "java/lang/Object");
    CHK(c.ann.size() == 1 && c.ann[0].type == "Ldemo/Mod;");
    CHK(c.ann.size() && c.ann[0].vals.size() == 2 && c.ann[0].vals[0].second == "demomod");
    const jv::Mth* soma = nullptr; for (auto& m : c.methods) if (m.name == "soma") soma = &m;
    CHK(soma && soma->nargs == 5 && soma->ret == 'J' && !soma->code.empty());
    printf("classe %s, %zu metodos, %zu cp\n", c.name.c_str(), c.methods.size(), c.cp.size());
    printf(fails ? "falhas: %d\n" : "passou\n", fails); return fails != 0;
}
