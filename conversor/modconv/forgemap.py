"""patchF10: mapa do que o mod REGISTRA no Forge, em forge_mapa.json:
- estados: propriedades de cada blockstate (facing=north,level=0 ou forge_marker) e seus valores -> o jogo sabe que bloco gira/abre/tem niveis;
- tesr: TileEntitySpecialRenderer = bloco desenhado por CODIGO (bau do IronChest); sem modelo JSON, so um handler especifico resolve;
- entidades: classes Entity*/Render* e texturas textures/entity/**, para a fase de mobs;
- itens: contagem por papel (espada, armadura, comida, ...)."""
import os, re, json
def props(d):
    out = {}
    vs = d.get('variants') or {}
    for k, v in vs.items():
        if k in ('normal', '', 'inventory', 'defaults'): continue
        if '=' in k:
            for pv in k.split(','):
                if '=' in pv: a, b = pv.split('=', 1); out.setdefault(a, set()).add(b)
        elif isinstance(v, dict) and not ('model' in v) and all(isinstance(x, (dict, list)) for x in v.values()) and v: out.setdefault(k, set()).update(v.keys())
    for part in d.get('multipart') or []:
        for a, b in (part.get('when') or {}).items():
            if a in ('OR', 'AND'): continue
            out.setdefault(a, set()).update(str(b).split('|'))
    return {a: sorted(b) for a, b in out.items()}
def extract(jar, outdir, nss, ctx=None, info=None):
    info = info or getattr(ctx, 'info', None) or {}
    est = {}
    for ns in nss:
        for f in jar.find(r'^assets/%s/blockstates/[^/]+\.json$' % re.escape(ns)):
            try:
                p = props(jar.json(f))
                if p: est[os.path.basename(f)[:-5]] = p
            except Exception: pass
    tesr = sorted(v['simple'] for v in info.values() if 'tesr' in v['roles'] and '$' not in v['simple'])
    ent = sorted(v['simple'] for v in info.values() if 'entity' in v['roles'] and '$' not in v['simple'])
    tex_ent = [n for n in jar.names if '/textures/entity/' in n and n.endswith('.png')]
    ores = sorted({o for v in info.values() for o in v.get('ores', [])})   # nomes do OreDictionary citados no codigo
    fluidos = sorted(v['simple'] for v in info.values() if 'fluid' in v['roles'] and '$' not in v['simple'])
    comidas = sorted(v['simple'] for v in info.values() if 'food' in v['roles'] and '$' not in v['simple'])
    papeis = {}
    for v in info.values():
        for r in v['roles']: papeis[r] = papeis.get(r, 0) + 1
    if not (est or tesr or ent or ores or fluidos or comidas): return 0
    json.dump(dict(estados=est, tesr=tesr, entidades=ent, oredict=ores, fluidos=fluidos, comidas=comidas, texturas_entidade=len(tex_ent), classes_por_papel=papeis), open(os.path.join(outdir, 'forge_mapa.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1, sort_keys=True)
    if ctx:
        ctx.stat('blocos_com_estados', len(est)); ctx.stat('oredict_nomes', len(ores))
        for x in fluidos: ctx.gap('fluido proprio (sem equivalente)', x)
        for t in tesr: ctx.gap('desenhado por codigo (TESR, sem modelo JSON)', t)
        for e in ent: ctx.gap('entidade propria (sem equivalente)', e)
    return len(est)
