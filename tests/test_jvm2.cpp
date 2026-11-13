// jvm estagio 2: interpretador + runtime. Roda demo.T do jar e imprime a saida (comparada com o java real pelo run.sh de teste)
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <ctime>
#include <climits>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_map>
static void splashPoll() {} static void splashNote(const char*) {}
#include "../src/core/png.inc"
#include "../src/modules/jvm/jvm_class.inc"
#include "../src/modules/jvm/jvm_zip.inc"
#include "../src/modules/jvm/jvm_vm.inc"
#include "../src/modules/jvm/jvm_exec.inc"
#include "../src/modules/jvm/jvm_rt.inc"
#include "../src/modules/jvm/jvm_coll.inc"
#include "../src/modules/jvm/jvm_indy.inc"
int main(int argc, char** argv) {
    const char* jp = argc > 1 ? argv[1] : "/tmp/t2/t.jar"; if (access(jp, F_OK) != 0) { printf("SKIP: jar de demonstracao ausente (%s; precisa de javac para gerar)\n", jp); return 0; }
    jv::Jar j; if (!j.open(jp)) { fprintf(stderr, "jar?\n"); return 2; }
    jv::VM vm; vm.jars.push_back(&j); jv::setupRuntime(vm); jv::setupColl(vm);
    jv::V a[1], r; a[0].o = vm.arr('L', 0);
    if (!vm.callStatic(argc > 2 ? argv[2] : "demo/T", "main", "([Ljava/lang/String;)V", a, r)) { fprintf(stderr, "EXC: %s %s\n", vm.exc->c->name.c_str(), vm.exc->s.c_str()); return 1; }
    for (auto& m : vm.missing) fprintf(stderr, "ausente: %s\n", m.c_str());
    return 0;
}
