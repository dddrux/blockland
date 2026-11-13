"""patchF3: itens que sao o COLOCADOR de um bloco (ItemBlock do Forge) e blocos tecnicos.
No Forge 1.12.2 o que o jogador carrega e o ITEM; o bloco (estado) so existe no mundo. Muitos mods registram blocos auxiliares sem item
(persiana aberta/fechada, parte de cima da cama/banheira, estagios) e itens por cor (ItemBlockColored) que apontam para UM bloco com propriedade.
Saida: placers.txt (`item|bloco|como`), tecnicos.txt (blocos tirados do criativo). Os itens tecnicos saem da lista do criativo (o modelo continua no pacote)."""
import os, re, json
TECH = re.compile(r'(_open|_closed|_on|_off|_lit|_powered|_active|_top|_bottom|_upper|_lower|_head|_foot|_part\d*|_left|_right|_stringless|_pre|_post|_level_?\d+|_stage_?\d+)$')
def _model_of(v):
    v = v[0] if isinstance(v, list) and v else v; return v.get('model') if isinstance(v, dict) else None
def block_models(jar, ns, name):
    """todos os modelos citados pelo blockstate (para ligar item->bloco pelo modelo)."""
    try: d = jar.json('assets/%s/blockstates/%s.json' % (ns, name))
    except Exception: return set()
    out = set(); vs = d.get('variants') or {}
    def walk(x):
        if isinstance(x, list):
            for y in x: walk(y)
        elif isinstance(x, dict):
            if 'model' in x and isinstance(x['model'], str): out.add(x['model'].split(':')[-1])
            for y in x.values():
                if isinstance(y, (dict, list)): walk(y)
    walk(vs); walk(d.get('multipart') or [])
    return out
def analyse(jar, ns, info):
    """-> (placers[{item,bloco,como}], tecnicos[set de nomes de bloco])."""
    bs = sorted(os.path.basename(f)[:-5] for f in jar.find(r'^assets/%s/blockstates/[^/]+\.json$' % re.escape(ns)))
    items = {}
    for f in jar.find(r'^assets/%s/models/item/.+\.json$' % re.escape(ns)):
        rel = f.split('/models/item/', 1)[1][:-5]
        try: items[rel] = (jar.json(f).get('parent') or '').split(':')[-1]
        except Exception: items[rel] = ''
    bmod = {b: block_models(jar, ns, b) for b in bs}
    m2b = {}
    for b, ms in bmod.items():
        for m in ms: m2b.setdefault(m.replace('block/', ''), []).append(b)
    placers = []; targets = set()
    for rel, par in sorted(items.items()):
        base = os.path.basename(rel); hit = None
        if base in bmod: hit = (base, 'mesmo nome')
        elif par.replace('block/', '') in m2b: hit = (m2b[par.replace('block/', '')][0], 'modelo')
        elif base in m2b: hit = (m2b[base][0], 'modelo')
        else:
            pre = [b for b in bs if base.startswith(b + '_') or b.startswith(base + '_')]
            if pre: hit = (pre[0], 'prefixo (ItemBlock por propriedade)')
        if hit: placers.append(dict(item='%s_%s' % (ns, rel.replace('/', '_')), bloco=hit[0], como=hit[1])); targets.add(hit[0])
    tech = set(); grp = {}
    for b in bs:
        if b in targets or b in items: continue
        if TECH.search(b): grp.setdefault(TECH.sub('', b), []).append(b)   # pares aberto/fechado, ligado/desligado, cima/baixo
    PREF = ('_closed', '_off', '_bottom', '_lower', '_left', '_pre')
    for base, bl in grp.items():
        cands = bl + ([base] if base in bs and base not in bl else [])
        if len(cands) < 2: continue
        keep = base if base in bs else next((x for p in PREF for x in bl if x.endswith(p)), sorted(bl)[0])
        tech |= {x for x in cands if x != keep and x not in targets and x not in items}
    return placers, tech
def apply(jar, ctx, ns, out, items, info):
    placers, tech = analyse(jar, ns, info)
    keep = []; hid = []
    for it in items:
        b = it['id'][len(ns) + 1:] if it['id'].startswith(ns + '_') else it['id']
        (hid if b in tech else keep).append(it)
    items[:] = keep
    if placers: open(os.path.join(out, 'placers.txt'), 'w', encoding='utf-8').write('\n'.join('%s|%s|%s' % (p['item'], p['bloco'], p['como']) for p in placers) + '\n')
    if hid: open(os.path.join(out, 'tecnicos.txt'), 'w', encoding='utf-8').write('\n'.join(sorted(i['id'] for i in hid)) + '\n')
    ctx.stat('colocadores', len(placers)); ctx.stat('blocos_tecnicos_ocultos', len(hid))
    return placers, hid
