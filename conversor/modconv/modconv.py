#!/usr/bin/env python3
"""Uso: python3 conversor/modconv/modconv.py <mod.jar> [mais.jar ...] [--out mods] [--db conversor/modconv/db] [--deps-dir pasta] [--no-deps]
Converte mods Forge 1.12.2 para o formato BLP1 do Blockland. Cada mod novo amplia o banco (db/) e/ou ganha handler em handlers/.
patchConv.4: le as dependencias (mcmod.info, mods.toml, @Mod dependencies=) e converte JUNTO os .jar dependentes que achar ao lado do jar, em --deps-dir, mods/ ou mods_jar/;
o que nao achar vai para dependencias.txt (`falta`) e para o RELATORIO.md, e o jogo avisa no log ([MODS])."""
import os, sys, argparse, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import core, handlers, vanilla, modinfo
def installed(out, dep):
    """dep ja convertido? (pasta com o nome do modid ou mod.txt com modid=)"""
    if os.path.isfile(os.path.join(out, dep, 'props.bin')): return True
    for mt in glob.glob(os.path.join(out, '*', 'mod.txt')):
        try:
            if ('modid=%s' % dep) in open(mt, encoding='utf-8').read().split('\n') and os.path.isfile(os.path.join(os.path.dirname(mt), 'props.bin')): return True
        except Exception: pass
    return False
def scan(dirs):
    """modid -> caminho do .jar, nas pastas dadas"""
    r = {}
    for d in dirs:
        for j in sorted(glob.glob(os.path.join(d, '*.jar'))):
            try: mid = modinfo.modid_of(core.Jar(j))
            except Exception: continue
            if mid: r.setdefault(mid, j)
    return r
def main():
    ap = argparse.ArgumentParser(); ap.add_argument('jars', nargs='+'); ap.add_argument('--out', default='mods')
    ap.add_argument('--vanilla', default=None, help='zip de texturas vanilla 1.12.2 (padrao: assets/*minecraft-assets*textures*.zip)')
    ap.add_argument('--deps-dir', action='append', default=[], help='pasta extra onde procurar .jar de dependencias')
    ap.add_argument('--no-deps', action='store_true', help='nao converter dependencias junto')
    ap.add_argument('--db', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'db')); a = ap.parse_args(); print('vanilla:', ('ok (' + ', '.join(vanilla.used()) + ')') if vanilla.init(a.vanilla) else 'ausente (refs ficam por nome)')
    queue = list(a.jars); done = {os.path.abspath(j) for j in queue}; idx = None; missing = {}
    while queue:
        j = queue.pop(0)
        mod, ctx = core.convert(j, a.out, a.db); st = []
        dp = getattr(ctx, 'deps', {}) or {}
        if dp and idx is None and not a.no_deps: idx = scan([os.path.dirname(os.path.abspath(j))] + a.deps_dir + ['mods', 'mods_jar'])
        for dep, kind in sorted(dp.items()):
            if installed(a.out, dep): st.append((dep, kind, 'ok')); continue
            jp = (idx or {}).get(dep)
            if jp and os.path.abspath(jp) not in done and not a.no_deps:
                done.add(os.path.abspath(jp)); queue.append(jp); st.append((dep, kind, 'convertido junto')); print('  dependencia %s -> convertendo junto: %s' % (dep, os.path.basename(jp)))
            else:
                st.append((dep, kind, 'falta')); ctx.gap('dependencia faltando (coloque o .jar em mods/ e rode tools/instalar_mods.sh)' if kind == 'required' else 'dependencia opcional ausente', dep)
                missing[dep] = kind
        if st: open(os.path.join(a.out, mod, 'dependencias.txt'), 'w', encoding='utf-8').write(''.join('%s %s %s\n' % t for t in st))
        core.report(os.path.join(a.out, mod, 'RELATORIO.md'), mod, ctx)
        print(mod, ctx.stats, {k: len(v) for k, v in ctx.gaps.items()})
    for d, k in sorted(missing.items()):
        if k == 'required': print('AVISO: dependencia obrigatoria faltando: %s (sem ela o mod pode ficar incompleto)' % d)
if __name__ == '__main__': main()
