#!/usr/bin/env python3
"""Uso: python3 conversor/modconv/modconv.py <mod.jar> [mais.jar ...] [--out mods] [--db conversor/modconv/db]
Converte mods Forge 1.12.2 para o formato BLP1 do Blockland. Cada mod novo amplia o banco (db/) e/ou ganha handler em handlers/."""
import os, sys, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import core, handlers, vanilla
def main():
    ap = argparse.ArgumentParser(); ap.add_argument('jars', nargs='+'); ap.add_argument('--out', default='mods')
    ap.add_argument('--vanilla', default=None, help='zip de texturas vanilla 1.12.2 (padrao: assets/*minecraft-assets*textures*.zip)')
    ap.add_argument('--db', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'db')); a = ap.parse_args(); print('vanilla:', 'ok' if vanilla.init(a.vanilla) else 'ausente (refs ficam por nome)')
    for j in a.jars:
        mod, ctx = core.convert(j, a.out, a.db); core.report(os.path.join(a.out, mod, 'RELATORIO.md'), mod, ctx)
        print(mod, ctx.stats, {k: len(v) for k, v in ctx.gaps.items()})
if __name__ == '__main__': main()
