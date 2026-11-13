#!/usr/bin/env bash
# Cria tests/test_<nome>.cpp a partir do molde (inclui o jogo com stubs de SDL; CHECK/SECTION prontos). tools/novo_teste.sh <nome>; rodar: tools/t.sh <nome>
cd "$(dirname "$0")/.."; N="${1:-}"; [[ "$N" =~ ^[a-z][a-z0-9_]*$ ]] || { echo "uso: tools/novo_teste.sh <nome>"; exit 1; }
F="tests/test_$N.cpp"; [ -e "$F" ] && { echo "ja existe: $F"; exit 1; }
cat > "$F" <<EOT
// test_$N: o que este teste cobre (1 linha). Rodar: tools/t.sh $N
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
    SECTION("T1 exemplo");
    CHECK(1 + 1 == 2);   // troque por chamadas reais (getB/setB, funcoes do jogo...)
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
EOT
echo "ok: $F  (rodar: tools/t.sh $N)"
