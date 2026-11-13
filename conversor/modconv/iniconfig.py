"""patchIni.1: receitas em arquivos .ini (IC2 2.8: assets/<ns>/config/*.ini) -> receitas.txt / maquinas.txt.
O IC2 nao usa recipes/*.json: o craft vem de shaped_recipes.ini / shapeless_recipes.ini, a fundicao de furnace.ini
e as maquinas (macerator, compressor...) de um .ini cada. Formato (comentario do proprio .ini):
  shaped    "AB|CD" A:item B:item = saida[*n]            (item = nome[@meta][|alternativa])
  shapeless item item = saida[*n]
  furnace   entrada = saida [@xp:0.5]
  maquina   entrada[*n] = saida[*n], saida[*n]
Nomes: minecraft:x | ns:item#variante | OreDict:x | Fluid:x. Resolvidos contra o props.json ja gravado (ids ns_...).
Sem equivalente no jogo (TileEntity `te`, fluidos, ferramentas) = lacuna no RELATORIO, nunca receita meio-quebrada."""
import json, os, re

NS = ['ic2']   # namespace do mod em conversao (extract() atualiza)
MACHINES = ('macerator', 'compressor', 'extractor', 'ore_washer', 'thermal_centrifuge', 'blast_furnace',
            'block_cutter', 'metal_former_cutting', 'metal_former_extruding', 'metal_former_rolling')

# patchIni.2: ore dict que nenhum item casa por palavras (o jogo acha por "todas as palavras do camelCase no id"):
# nome do OreDict -> ids (sem o prefixo do mod) conferidos no .lang/props do IC2. So vale se o id existir no props.json do mod.
ORE_ALIAS = {
    'itemRubber': ['crafting_rubber'], 'circuitBasic': ['crafting_circuit'], 'circuitAdvanced': ['crafting_advanced_circuit'],
    'craftingToolForgeHammer': ['tool_forge_hammer'], 'craftingToolWireCutter': ['tool_cutter'], 'materialScrap': ['crafting_scrap'],
    'dustHydratedCoal': ['resource_dust_coal_fuel'], 'ingotPlutonium': ['resource_nuclear_plutonium'],
    'nuggetUranium235': ['resource_nuclear_small_uranium_235'], 'ingotUranium235': ['resource_nuclear_uranium_235'],
}
VANILLA_ALIAS = {'chestWood': 'minecraft:chest', 'treeSapling': 'minecraft:sapling'}   # o jogo resolve `minecraft:x` pelo pack/nativo

def _ids(out_dir):
    try:
        d = json.load(open(os.path.join(out_dir, 'props.json'), encoding='utf-8'))
        return {i['id'] for i in d.get('items', [])}
    except Exception:
        return set()

def _split_amount(s):
    """`nome*4` -> (nome, 4); sem `*n` -> (nome, 1)."""
    m = re.match(r'^(.*?)\*(\d+)$', s)
    return (m.group(1), int(m.group(2))) if m else (s, 1)

def resolve(tok, ids, ns0='ic2'):
    """Um nome do .ini -> (nome para receitas.txt | None, motivo se falhar). Aceita `a|b` (1a alternativa que resolve)."""
    tok = tok.strip()
    if not tok: return None, 'vazio'
    why = ''
    for alt in tok.split('|'):
        alt = alt.strip()
        if alt.lower().startswith('oredict:'):
            on = alt[8:]
            for c in ORE_ALIAS.get(on, []):
                if '%s_%s' % (ns0, c) in ids: return '%s:%s' % (ns0, c), ''
            return VANILLA_ALIAS.get(on, 'ore:' + on), ''
        if alt.lower().startswith('fluid:'): why = 'fluido'; continue
        base, meta = alt, ''
        if '@' in base: base, meta = base.split('@', 1)
        if meta == '*': meta = ''
        ns, _, rest = base.partition(':')
        if not rest: ns, rest = 'minecraft', ns
        if ns == 'minecraft':
            return 'minecraft:%s%s' % (rest, '@' + meta if meta.isdigit() and meta != '0' else ''), ''
        name, _, var = rest.partition('#')
        cands = []
        if var:
            if ':' in var:   # cable#type:copper,insulation:1
                kv = dict(p.split(':', 1) for p in var.split(',') if ':' in p)
                t, ins = kv.get('type', ''), kv.get('insulation', '')
                cands += ['%s_%s_%s_%s_%s' % (ns, name, t, name, ins), '%s_%s_%s_%s' % (ns, name, t, name), '%s_%s_%s_%s' % (ns, name, t, ins), '%s_%s_%s' % (ns, name, t)]
            else:
                cands += ['%s_%s_%s' % (ns, name, var), '%s_resource_%s_%s' % (ns, name, var), '%s_resource_%s' % (ns, var), '%s_%s' % (ns, var)]
        else:
            cands += ['%s_%s' % (ns, name)]
            if meta.isdigit(): cands.insert(0, '%s_%s_%s' % (ns, name, meta))
        for c in cands:
            if c in ids: return '%s:%s' % (ns, c[len(ns) + 1:]), ''
        key = (var if var and ':' not in var else name)   # sufixo unico: ic2:treetap -> ic2_tool_treetap
        hit = [i for i in ids if i.startswith(ns + '_') and i.endswith('_' + key) and '~' not in i]
        if len(hit) == 1: return '%s:%s' % (ns, hit[0][len(ns) + 1:]), ''
        why = why or ('sem item no jogo: %s' % alt)
    return None, why or 'desconhecido'

def _lines(jar, path):
    out = []
    for ln in jar.text(path).splitlines():
        ln = ln.strip()
        if ln and not ln.startswith(';'): out.append(ln)
    return out

def _out(s, ids):
    """lado direito: `saida[@meta][*n] [@flags]` -> (nome, n, motivo)."""
    s = s.strip(); toks = s.split()
    head = toks[0] if toks else ''
    name, n = _split_amount(head)
    r, why = resolve(name, ids, NS[0])
    return r, n, why

def shaped(line, ids):
    m = re.match(r'^"([^"]+)"\s+(.*?)\s+=\s+(.*)$', line)
    if not m: return None, 'formato'
    pat, keys, outp = m.groups()
    res, n, why = _out(outp, ids)
    if not res: return None, why
    kv = []
    for k in re.findall(r'(?:^|\s)(\S):(\S+)', keys):
        r, w = resolve(k[1], ids, NS[0])
        if not r: return None, w
        kv.append('%s=%s' % (k[0], r))
    if not kv: return None, 'sem chaves'
    return 'shaped %s %d %s %s' % (res, n, pat, ';'.join(kv)), ''

def shapeless(line, ids):
    if ' = ' not in line: return None, 'formato'
    left, outp = line.split(' = ', 1)
    res, n, why = _out(outp, ids)
    if not res: return None, why
    ing = []
    for t in left.split():
        t, k = _split_amount(t)
        r, w = resolve(t, ids, NS[0])
        if not r: return None, w
        ing += [r] * k
    if not ing or len(ing) > 9: return None, 'ingredientes'
    return 'shapeless %s %d %s' % (res, n, ','.join(ing)), ''

def furnace(line, ids):
    if ' = ' not in line: return None, 'formato'
    a, b = line.split(' = ', 1)
    xp = re.search(r'@xp:([0-9.]+)', b); xp = xp.group(1) if xp else '0'
    ra, wa = resolve(_split_amount(a.strip())[0], ids, NS[0]); rb, nb, wb = _out(b, ids)
    if not ra or not rb: return None, wa or wb
    return 'smelt %s %s %s' % (ra, rb, xp), ''

def machine(kind, line, ids):
    if ' = ' not in line: return None, 'formato'
    a, b = line.split(' = ', 1)
    na, qa = _split_amount(a.strip().split()[0]); ra, wa = resolve(na, ids, NS[0])
    outs = []
    for part in b.split(','):
        r, n, w = _out(part, ids)
        if not r: return None, w
        outs.append('%s*%d' % (r, n))
    if not ra: return None, wa
    return '%s|%s*%d|%s' % (kind, ra, qa, ','.join(outs)), ''

def extract(jar, out_dir, nss, ctx):
    NS[0] = nss[0] if nss else 'ic2'; ids = _ids(out_dir); R = []; M = []
    for ns in nss:
        base = 'assets/%s/config/' % ns
        for path, fn in ((base + 'shaped_recipes.ini', shaped), (base + 'shapeless_recipes.ini', shapeless), (base + 'furnace.ini', furnace)):
            if not jar.has(path): continue
            for ln in _lines(jar, path):
                r, why = fn(ln, ids)
                if r and r not in R: R.append(r)
                elif not r: ctx.gap('receita .ini sem equivalente', (why or '?')[:60])
        for mk in MACHINES:
            path = base + mk + '.ini'
            if not jar.has(path): continue
            for ln in _lines(jar, path):
                r, why = machine(mk, ln, ids)
                if r and r not in M: M.append(r)
                elif not r: ctx.gap('receita de maquina .ini sem equivalente', (why or '?')[:60])
    if R:
        p = os.path.join(out_dir, 'receitas.txt'); have = []
        if os.path.exists(p): have = [x for x in open(p, encoding='utf-8').read().splitlines() if x]
        new = [x for x in R if x not in have]
        open(p, 'w', encoding='utf-8').write('\n'.join(have + new) + '\n')
    if M: open(os.path.join(out_dir, 'maquinas.txt'), 'w', encoding='utf-8').write('\n'.join(M) + '\n')
    ctx.stat('receitas_ini', len(R)); ctx.stat('maquinas_ini', len(M))
    return R, M
