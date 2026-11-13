"""patchF5: espadas, ferramentas, arcos e armaduras de mod. Classifica cada item pelo PAPEL da classe no Forge (ItemSword, ItemPickaxe/ItemAxe/ItemSpade/ItemHoe/ItemTool, ItemBow, ItemArmor)
e, na falta dela, pelo nome. Material/nivel vem do nome (wood/stone/iron/gold/diamond + materiais de mod) e do ToolMaterial/ArmorMaterial citados nas strings da classe.
Saida: gear.txt (`id|tipo|nivel|dano|durabilidade|por que`). Armaduras: ver armor.py (texturas de camada); aqui so completa as que a classe diz ser ItemArmor."""
import os, re, json
import classinfo
KIND = [('sword', r'sword|blade|katana|saber|sabre|rapier|cutlass|dagger|knife|machete|claymore|scimitar|longsword|spear|lance|mace|scythe|glaive|halberd|cleaver|espada'),
        ('pickaxe', r'pickaxe|pick(?!le)'), ('axe', r'(^|_)axe(_|$)|hatchet|battleaxe|waraxe'), ('shovel', r'shovel|spade'), ('hoe', r'(^|_)hoe(_|$)'), ('hammer', r'hammer|maul'),
        ('bow', r'(^|_)(bow|crossbow|longbow)(_|$)'), ('shield', r'shield')]
TIERS = [('wood', 0, 59), ('stone', 1, 131), ('iron', 2, 250), ('gold', 0, 32), ('golden', 0, 32), ('diamond', 3, 1561), ('copper', 1, 200), ('bronze', 2, 300), ('steel', 2, 500),
         ('obsidian', 3, 1500), ('emerald', 3, 1400), ('ruby', 3, 1400), ('sapphire', 3, 1400), ('titanium', 3, 1800), ('netherite', 4, 2031), ('nether', 3, 1200), ('silver', 2, 250), ('tin', 1, 180), ('lead', 1, 150)]
BASE = {'sword': 4, 'pickaxe': 2, 'axe': 5, 'shovel': 2.5, 'hoe': 1, 'hammer': 5, 'bow': 0, 'shield': 0}
def kind_of(short):
    for k, rx in KIND:
        if re.search(rx, short): return k
def tier_of(short):
    for w, t, d in TIERS:
        if w in short: return w, t, d
    return '', 1, 250
def extract(jar, outdir, nss, ctx=None, info=None):
    pj = os.path.join(outdir, 'props.json')
    if not os.path.exists(pj): return 0
    items = json.load(open(pj, encoding='utf-8'))['items']; info = info or getattr(ctx, 'info', None) or classinfo.analyse(jar)
    shorts = [i['id'].split('_', 1)[-1] for i in items]
    role = {}
    for r in ('sword', 'tool', 'bow', 'shield', 'armor'):
        for nm, cl in classinfo.by_name(info, shorts, 'item' if False else r).items(): role.setdefault(nm, r)
    L = []
    for it in items:
        if it.get('beh'): continue
        short = it['id'].split('_', 1)[-1]
        if re.search(r'helmet|chestplate|leggings|boots', short): continue   # armadura: armor.py
        k = kind_of(short); why = 'nome'
        if role.get(short) == 'sword': k, why = 'sword', 'classe ItemSword'
        elif role.get(short) == 'bow': k, why = 'bow', 'classe ItemBow'
        elif role.get(short) == 'tool' and k not in ('pickaxe', 'axe', 'shovel', 'hoe'): k, why = k or 'pickaxe', 'classe ItemTool'
        elif role.get(short) and k: why = 'classe+nome'
        if not k or re.search(r'recipe|outline|upgrade|ingot|nugget|dust|gear|plate|rod|block|ore(_|$)|handle|head(_|$)', short) and why == 'nome': continue
        w, t, dur = tier_of(short); dmg = BASE[k] + (t if k in ('sword', 'axe', 'hammer') else 0) if BASE[k] else 0
        L.append('%s|%s|%d|%s|%d|%s' % (it['id'], k, t, dmg, dur, why))
    if L:
        open(os.path.join(outdir, 'gear.txt'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
        if ctx: ctx.stat('espadas_ferramentas', len(L)); ctx.stat('armas_por_classe', sum(1 for x in L if 'classe' in x))
    return len(L)
