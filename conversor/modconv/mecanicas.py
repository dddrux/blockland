"""patchMec.1: liga os FATOS do bytecode (comportamento.json) a cada item do mod. Saida mods/<mod>/mecanicas.txt: `item|dureza|resistencia|nivel|metodos|classe|acoes` (acoes = o que onBlockActivated faz: gui, sentar, estado, som, mensagem, consome_item, da_item, remove_bloco, fome, cura)
(campos vazios = desconhecido). Dureza/resistencia/nivel vem de setHardness/setResistance/setHarvestLevel da classe do bloco (ou das superclasses);
metodos = sobrescritos que o jogo ainda precisa tratar (onBlockActivated, updateTick, ...). O jogo le a dureza (quebra com tempo) e guarda os metodos."""
import json, os
import classinfo
def _facts(comp, c):
    f = {}; seen = set()
    while c and c in comp and c not in seen:
        seen.add(c); d = comp[c]
        for k, v in d.items():
            if k in ('metodos', 'acoes'): f.setdefault(k, set()).update(v)
            elif k != 'super': f.setdefault(k, v)
        c = d.get('super')
    return f

PARES = (('_closed', '_open'), ('_off', '_on'), ('_unlit', '_lit'), ('_shut', '_open'), ('_down', '_up'))
def parear(out, items, L):
    """patchMec.2: item com acao `estado` (onBlockActivated troca o blockstate) e que tem o par pelo nome (X_closed -> X_open) entra em abertos.txt (`item|variante`)."""
    ids = {i['id'] for i in items}; comBeh = {i['id'] for i in items if i.get('beh')}; ap = os.path.join(out, 'abertos.txt'); have = set()
    if os.path.exists(ap): have = {x.strip() for x in open(ap, encoding='utf-8') if x.strip()}
    done = {h.split('|')[0] for h in have}; novos = []
    for ln in L:
        v = ln.split('|')
        if len(v) < 7 or 'estado' not in v[6].split(',') or v[0] in done or v[0] in comBeh: continue
        for a, b in PARES:
            if v[0].endswith(a) and v[0][:-len(a)] + b in ids: novos.append('%s|%s' % (v[0], v[0][:-len(a)] + b)); break
    if novos: open(ap, 'a' if have else 'w', encoding='utf-8').write('\n'.join(novos) + '\n')
    return novos
def apply(jar, out, ns, ctx):
    cp = os.path.join(out, 'comportamento.json'); pp = os.path.join(out, 'props.json')
    if not (os.path.exists(cp) and os.path.exists(pp)): return []
    comp = json.load(open(cp, encoding='utf-8')); items = json.load(open(pp, encoding='utf-8')).get('items', [])
    p2b = {}
    if os.path.exists(os.path.join(out, 'placers.txt')):
        for ln in open(os.path.join(out, 'placers.txt'), encoding='utf-8'):
            v = ln.strip().split('|')
            if len(v) >= 2: p2b[v[0]] = v[1]
    info = classinfo.analyse(jar)
    names = [(p2b.get(i['id']) or (i['id'][len(ns) + 1:] if i['id'].startswith(ns + '_') else i['id'])) for i in items]
    cl = classinfo.by_name(info, sorted(set(names)), 'block'); L = []; AA = []   # patchForge.4: acoes.txt `item|acao|argumento`
    en = jar.lang(ns); pt = jar.lang(ns, 'pt_br')
    snk = {}   # fallback (patchMec.3): classe Block<Nome> do comportamento.json casa por nome (BlockDiamondFurnace -> diamond_furnace[_*]) mesmo sem o papel `block`
    for c in comp:
        sn = c.split('/')[-1]
        if sn.startswith('Block') and len(sn) > 5: snk[classinfo.snake(sn[5:])] = c
    def porNome(b):
        r = [(len(k), c) for k, c in snk.items() if b == k or b.startswith(k + '_')]
        if r: return [max(r)[1]]
        # patchMec.4: classe de feature (chisel Features$N): as strings da classe sao os nomes dos blocos que o lambda configura
        r = [(len(t), c) for c in comp if c in info and any(k in comp[c] for k in ('dureza', 'resistencia')) for t in info[c]['strs'] if len(t) >= 3 and (b == t or b.startswith(t + '_'))]
        return [max(r)[1]] if r else []
    for it, b in zip(items, names):
        best = None
        for c in (cl.get(b) or porNome(b)):
            f = _facts(comp, c)
            if f and (best is None or len([k for k in f if k != 'acoes_args']) > len([k for k in best[0] if k != 'acoes_args'])): best = (f, c)   # patchForge.4: acoes_args nao decide a classe
        if not best: continue
        f, c = best; mt = sorted(f.get('metodos', ()))
        f.setdefault('dureza', f.get('dureza_metodo')) if f.get('dureza_metodo') is not None else None   # patchMec.6: getBlockHardness constante vale como dureza
        f.setdefault('resistencia', f.get('resistencia_metodo')) if f.get('resistencia_metodo') is not None else None
        g = lambda k: ('' if f.get(k) is None else ('%g' % f[k]))
        for ak, vs in sorted((f.get('acoes_args') or {}).items()):   # mensagem: chave de .lang -> texto (pt-BR, senao en, senao o proprio texto); som: ns:nome
            for v in vs:
                if ak == 'mensagem': v = (pt.get(v) or en.get(v) or v).replace('|', '/').replace('\n', ' ')
                elif ak == 'som' and ':' not in v: v = ns + ':' + v
                if v: AA.append('%s|%s|%s' % (it['id'], ak, v))
        L.append('|'.join([it['id'], g('dureza'), g('resistencia'), g('nivel_colheita'), ','.join(mt), info[c]['simple'], ','.join(sorted(f.get('acoes', ()))), f.get('ferramenta') or '']))   # patchMec.5b: coluna 8 = classe da ferramenta (pickaxe/axe/shovel)
    if L: open(os.path.join(out, 'mecanicas.txt'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    if AA: open(os.path.join(out, 'acoes.txt'), 'w', encoding='utf-8').write('\n'.join(AA) + '\n')
    ctx.stat('acoes_com_argumento', len(AA))
    ctx.stat('abertos_por_mecanica', len(parear(out, items, L)))
    ctx.stat('itens_com_mecanica', len(L)); return L
