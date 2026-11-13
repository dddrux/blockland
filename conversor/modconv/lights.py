"""patchF14: luz emitida. No Forge 1.12.2 o bloco chama setLightLevel(float 0..1) no construtor (func_149715_a); a luz NAO esta nos JSON.
Le a constante float da classe do bloco (tabela de constantes) e grava item.light = round(f*15) (o props.bin ja tem o campo). lights.txt: `item|luz|classe`."""
import os, re
import classinfo
OFF = re.compile(r'(_off|_unlit|_closed|_broken|_empty)$')
def apply(jar, ctx, ns, out, items, info, placers=()):
    p2b = {p['item']: p['bloco'] for p in placers}
    names = [(p2b.get(i['id']) or (i['id'][len(ns) + 1:] if i['id'].startswith(ns + '_') else i['id'])) for i in items]
    cl = classinfo.by_name(info, sorted(set(names)), 'block'); L = []
    for it, b in zip(items, names):
        best = max(((info[c]['light'], c) for c in cl.get(b, []) if info[c].get('light')), default=None)
        if best and not it.get('light') and not OFF.search(it['id']): it['light'] = max(1, min(15, round(best[0] * 15))); L.append('%s|%d|%s' % (it['id'], it['light'], info[best[1]]['simple']))
    if L: open(os.path.join(out, 'lights.txt'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    ctx.stat('itens_com_luz', len(L)); return L
