"""Armaduras de mod (patchMod.13): item *_helmet/_chestplate/_leggings/_boots -> slot + texturas de camada (layer 1/2) + defesa.
Saida em mods/<mod>/: armor.txt (`id|slot|tex1|tex2|pontos`, tex '-' = usar iron nativa) e armor/<arquivo>.png. O jogo le em props.inc (propLoadArmor)."""
import os, re, io, json
SLOT = [('helmet', 0), ('helm', 0), ('chestplate', 1), ('leggings', 2), ('boots', 3)]
TIER = [(('leather', 'cloth', 'rubber', 'hazmat', 'wood', 'bronze_cloth'), (1, 3, 2, 1)), (('gold', 'golden'), (2, 5, 3, 1)),
        (('diamond', 'nano', 'quantum', 'neptunium', 'obsidian', 'nether', 'crystal', 'titan'), (3, 8, 6, 3))]
def slot_of(iid):
    p = [t for t in re.split(r'[^a-z0-9]+', iid.lower()) if t]
    for t in p:
        for w, s in SLOT:
            if t == w or (len(w) >= 5 and t.endswith(w) and len(t) > len(w)): return s
    return -1
def material(iid, ns):
    s = iid.lower(); s = s[len(ns) + 1:] if s.startswith(ns.lower() + '_') else s
    s = re.sub(r'(helmet|helm|chestplate|leggings|boots|armor|armour)', '_', s); return re.sub(r'_+', '_', s).strip('_')
def points(mat, slot):
    for ws, t in TIER:
        if any(w in mat for w in ws): return t[slot]
    return (2, 6, 5, 2)[slot]
def hints(info):   # patchF5: nomes de material citados em classes ItemArmor/ArmorMaterial (texturas de camada que o nome do item nao revela)
    return sorted({x for v in (info or {}).values() if 'armor' in v['roles'] or 'ArmorMaterial' in v['simple'] or 'Armor' in v['simple'] for x in v['strs']})
def extract(jar, outdir, ns_list, ctx=None, info=None):
    pj = os.path.join(outdir, 'props.json')
    if not os.path.exists(pj): return 0
    items = json.load(open(pj, encoding='utf-8'))['items']
    stems = {}
    for n in jar.names:
        l = n.lower()
        if l.endswith('.png') and '/textures/' in l and re.search(r'armou?r|/models/|layer_', l): stems.setdefault(os.path.basename(l)[:-4], n)
    lines = []; used = {}
    for it in items:
        if it.get('beh'): continue
        sl = slot_of(it['id'])
        if sl < 0: continue
        ns = next((x for x in ns_list if it['id'].lower().startswith(x.lower() + '_')), ns_list[0] if ns_list else '')
        m = material(it['id'], ns); tx = ['-', '-']
        for N in (1, 2):
            cands = [m + s for s in ('_layer_%d' % N, '_%d' % N, '__layer_%d' % N, '_armor_layer_%d' % N, '_armor_%d' % N, 'layer%d' % N)]
            k = next((c for c in cands if c in stems), None)
            if k is None and m:
                fz = sorted([s for s in stems if s.startswith(m) and (s.endswith('_%d' % N) or s.endswith('layer_%d' % N))], key=len); k = fz[0] if fz else None
            if k is None:   # patchF5: dica vinda da classe (ex.: material 'ruby' -> ruby_layer_1)
                for h in hints(info):
                    c2 = next((c for c in (h + '_layer_%d' % N, h + '_%d' % N, h + '__layer_%d' % N) if c in stems), None)
                    if c2 and (h in m or m in h): k = c2; break
            if k: tx[N - 1] = k
        if tx[0] == '-' and tx[1] != '-': tx[0] = tx[1]
        if tx[1] == '-' and tx[0] != '-': tx[1] = tx[0]
        for t in tx:
            if t != '-': used[t] = stems[t]
        lines.append('%s|%d|%s|%s|%d' % (it['id'], sl, tx[0], tx[1], points(m, sl)))
    if not lines: return 0
    os.makedirs(os.path.join(outdir, 'armor'), exist_ok=True)
    for k, n in used.items(): open(os.path.join(outdir, 'armor', k + '.png'), 'wb').write(jar.read(n))
    open(os.path.join(outdir, 'armor.txt'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    if ctx: ctx.stat('armaduras', len(lines)); ctx.stat('armaduras_com_textura', sum(1 for l in lines if l.split('|')[2] != '-'))
    return len(lines)
