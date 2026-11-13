#!/usr/bin/env python3
# Gera src/MAPA.md (1 linha por arquivo: linhas + proposito) e src/INDICE.tsv (nome<TAB>arquivo:linha de funcoes/structs/globais).
# Uso: python3 tools/mapa.py   |   consulta: tools/onde.sh <nome>   (grep no indice, em vez de ler arquivos inteiros)
import os, re, sys
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')
files = sorted(os.path.relpath(os.path.join(d, f), R) for d, _, fs in os.walk(R) for f in fs if f.endswith(('.inc', '.cpp')))
why = {}
for ln in open(os.path.join(R, 'main.cpp'), encoding='utf-8', errors='replace'):   # proposito = comentario do #include em main.cpp
    m = re.match(r'#include "([^"]+)"\s*(?://\s*(.*))?', ln)
    if m and m.group(2): why[m.group(1)] = m.group(2).strip()
fn = re.compile(r'^(?:static\s+|inline\s+|constexpr\s+|const\s+)+[\w:<>,\*&\s]*?[\s\*&](\w+)\s*\(')
gl = re.compile(r'^static\s+(?:const\s+|constexpr\s+)?[\w:<>,\*&]+(?:\s+[\w:<>,\*&]+)*?\s+\**(g[A-Z]\w*|[A-Z][A-Z0-9_]{2,})\s*(?:=|;|\[|\{)')
ty = re.compile(r'^(?:struct|enum(?:\s+class)?|namespace|class)\s+(\w+)')
idx, mapa = [], []
for f in files:
    p = os.path.join(R, f); L = open(p, encoding='utf-8', errors='replace').read().split('\n')
    desc = why.get(f, '')
    if not desc:
        for ln in L[:6]:
            if ln.startswith('//'): desc = re.sub(r'^[/\s\-=]+|[\s\-=]+$', '', ln); break
    mapa.append((f, len(L), desc[:110]))
    for i, ln in enumerate(L, 1):
        if ln[:1] in ' \t#/' or not ln: continue
        m = ty.match(ln) or fn.match(ln) or gl.match(ln)
        if m and m.group(1) not in ('if', 'for', 'while', 'return', 'switch'): idx.append((m.group(1), f'{f}:{i}'))
with open(os.path.join(R, 'MAPA.md'), 'w', encoding='utf-8') as o:
    o.write('# MAPA do codigo (gerado por tools/mapa.py - nao editar)\nJogo = 1 unidade de compilacao (main.cpp inclui os .inc na ordem). Achar algo: `tools/onde.sh <nome>` -> arquivo:linha; ler so o trecho (`sed -n X,Yp`).\nModulos novos: `tools/novo_modulo.sh <nome>` + APIs em modules/api.inc (ver modules/README.md).\n\n| arquivo | linhas | o que faz |\n|---|---|---|\n')
    for f, n, d in mapa: o.write(f'| {f} | {n} | {d} |\n')
with open(os.path.join(R, 'INDICE.tsv'), 'w', encoding='utf-8') as o:
    for n, loc in idx: o.write(f'{n}\t{loc}\n')
print(f'{len(mapa)} arquivos, {len(idx)} simbolos')
