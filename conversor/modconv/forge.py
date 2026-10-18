"""patchMod.4: tradutor de acoes do Forge 1.12.2 -> Blockland.
1) Varre os .class do jar (so a tabela de constantes, sem decompilar) e classifica cada chamada net/minecraft* e net/minecraftforge* pela TABELA abaixo.
2) Traduz o que e declarativo: receitas (assets/<ns>/recipes), drops (loot_tables), sons (sounds.json) para arquivos de texto simples.
Saidas em mods/<mod>/: receitas.txt, drops.txt, sons.txt, forge_acoes.json. O que nao tem equivalente vai para o RELATORIO e para db/unknown.json."""
import json, os, re, struct
# (regex sobre "owner.metodo", status, o que o Blockland faz, nota). Primeira que casar vale. status: convertido | aproximado | ignorado | sem equivalente
TABLE = [
    (r'registerBlock|registerItem|register\b|GameRegistry\.|IForgeRegistry|RegistryEvent', 'convertido', 'blocos/itens vem dos blockstates e modelos JSON', 'registro nao precisa de codigo'),
    (r'setRegistryName|setUnlocalizedName|setTranslationKey', 'convertido', 'id e nome vem do arquivo e do .lang', ''),
    (r'setCreativeTab|CreativeTabs', 'convertido', 'cada mod vira uma aba propria no criativo', ''),
    (r'addShapedRecipe|addShapelessRecipe|ShapedRecipes|ShapelessRecipes|ShapedOreRecipe|ShapelessOreRecipe|IRecipe|CraftingManager', 'aproximado', 'receitas JSON viram receitas.txt; receita feita em codigo nao e lida', 'preferir recipes/*.json'),
    (r'addSmelting|FurnaceRecipes', 'aproximado', 'smelting em JSON vira receitas.txt (linha smelt)', ''),
    (r'OreDictionary', 'aproximado', 'ore:<nome> em receitas.txt (jogo resolve pelo nome)', ''),
    (r'getDrops|dropBlockAsItem|getItemDropped|quantityDropped|LootTable|LootEntry', 'aproximado', 'loot_tables JSON viram drops.txt; drop em codigo nao e lido', ''),
    (r'setHardness|setResistance|setHarvestLevel|setHarvestTool', 'aproximado', 'dureza vem do bloco-base mais proximo; valor exato nao e lido do codigo', 'ver forge_acoes.json'),
    (r'setLightLevel|getLightValue', 'aproximado', 'light= no item do props.json quando o modelo traz emissao; senao 0', ''),
    (r'setLightOpacity|setSoundType|SoundType|setSlipperiness|setTickRandomly|setBlockUnbreakable', 'ignorado', 'sem efeito no Blockland', ''),
    (r'onBlockActivated', 'aproximado', 'props: Door, Chair, Bed, Container, SwitchableLamp (campo beh); o resto nao', 'clique direito personalizado nao e traduzido'),
    (r'Chair|Seat|sitOn|startRiding', 'aproximado', 'beh=Chair (sentar)', ''),
    (r'getBoundingBox|getCollisionBoundingBox|addCollisionBoxToList|AxisAlignedBB', 'aproximado', 'colisao e caixa de selecao vem das caixas do modelo', ''),
    (r'getRenderLayer|isOpaqueCube|isFullCube|BlockRenderLayer', 'aproximado', 'translucidez decidida por textura (translucent.txt / alfa por face)', ''),
    (r'IBlockState|BlockStateContainer|PropertyDirection|PropertyBool|PropertyInteger|PropertyEnum|getMetaFromState|getStateFromMeta', 'aproximado', 'so o 1o estado do blockstate vira bloco (variantes giradas via x/y)', 'estados extras: proximo patch'),
    (r'onNeighborChange|neighborChanged|updateTick|randomTick|scheduleUpdate', 'sem equivalente', 'logica de tick/vizinho nao e traduzida', 'redstone do Blockland e fixa'),
    (r'TileEntity|ITickable|IInventory|ISidedInventory|ItemStackHandler|IItemHandler|Capabilit', 'sem equivalente', 'so Container (beh) guarda itens; TileEntity proprio nao', ''),
    (r'IGuiHandler|GuiScreen|GuiContainer|Container\b|Slot\b|NetworkRegistry', 'sem equivalente', 'GUIs de mod nao existem; Container usa o baú do Blockland', ''),
    (r'SimpleNetworkWrapper|IMessage|FMLCommonHandler|NetworkManager', 'sem equivalente', 'pacotes de rede do mod nao sao traduzidos', ''),
    (r'Entity[A-Z]\w*|EntityRegistry|RenderingRegistry|ModelBakery|ModelLoader|IBakedModel|TileEntitySpecialRenderer', 'sem equivalente', 'entidades e renderizadores proprios nao sao traduzidos', 'modelos JSON/CraftStudio sim'),
    (r'World\.(setBlock|getBlock|spawn)|setBlockState|getBlockState|spawnEntity|spawnParticle|playSound', 'sem equivalente', 'acao no mundo por codigo nao e traduzida', ''),
    (r'EnumFacing|BlockPos', 'ignorado', 'tipos auxiliares, sem efeito sozinhos', ''),
    (r'SubscribeEvent|EventHandler|MinecraftForge\.EVENT_BUS|FMLInitializationEvent|FMLPreInitializationEvent|FMLPostInitializationEvent|Mod\b', 'ignorado', 'ciclo de vida do Forge: o Blockland carrega o mod pela pasta mods/', ''),
    (r'Config|Configuration', 'ignorado', 'configuracao do mod nao e lida', ''),
]
_TABLE = [(re.compile(p), s, a, n) for p, s, a, n in TABLE]
def classify(key):
    for r, s, a, n in _TABLE:
        if r.search(key): return s, a, n
    return 'sem equivalente', 'chamada desconhecida', ''
def class_refs(data):
    """Tabela de constantes de um .class -> (super, interfaces, {owner.metodo})."""
    try:
        if data[:4] != b'\xca\xfe\xba\xbe': return None
        n = struct.unpack('>H', data[8:10])[0]; p = 10; cp = [None] * n; i = 1
        while i < n:
            t = data[p]; p += 1
            if t == 1: ln = struct.unpack('>H', data[p:p + 2])[0]; cp[i] = ('u', data[p + 2:p + 2 + ln].decode('utf-8', 'replace')); p += 2 + ln
            elif t in (3, 4): p += 4
            elif t in (5, 6): p += 8; i += 1
            elif t in (7, 8, 16, 19, 20): cp[i] = (t, struct.unpack('>H', data[p:p + 2])[0]); p += 2
            elif t in (9, 10, 11, 12, 17, 18): cp[i] = (t, struct.unpack('>HH', data[p:p + 4])); p += 4
            elif t == 15: p += 3
            else: return None
            i += 1
        u = lambda k: cp[k][1] if cp[k] and cp[k][0] == 'u' else ''
        cn = lambda k: u(cp[k][1]) if cp[k] and cp[k][0] == 7 else ''
        p += 2; p += 2; sup = cn(struct.unpack('>H', data[p:p + 2])[0]); p += 2
        ni = struct.unpack('>H', data[p:p + 2])[0]; p += 2; ifs = [cn(struct.unpack('>H', data[p + 2 * j:p + 2 * j + 2])[0]) for j in range(ni)]
        refs = set()
        for e in cp:
            if e and e[0] in (9, 10, 11):
                owner = cn(e[1][0]); nat = cp[e[1][1]]
                if nat and nat[0] == 12: refs.add(owner + '.' + u(nat[1][0]))
        return sup, ifs, refs
    except Exception: return None
def scan(jar, ctx):
    """-> {chave: {n, status, alvo, nota}}; registra lacunas em ctx."""
    out = {}; classes = 0
    def add(k):
        if not (k.startswith('net/minecraft') or k.startswith('net/minecraftforge') or k.startswith('cpw/mods')): return
        d = out.setdefault(k, dict(n=0)); d['n'] += 1
    for nm in jar.names:
        if not nm.endswith('.class'): continue
        r = class_refs(jar.read(nm))
        if not r: continue
        classes += 1; sup, ifs, refs = r
        for k in [sup] + ifs: add(k + '.<extends>')
        for k in refs: add(k)
    for k, d in out.items():
        d['status'], d['alvo'], d['nota'] = classify(k)
        if d['status'] in ('sem equivalente', 'aproximado'): ctx.gap('Forge: ' + d['status'], k.replace('net/minecraftforge/', 'forge/').replace('net/minecraft/', 'mc/'), d['n'])
    ctx.stat('classes_java', classes); ctx.stat('chamadas_forge', len(out))
    return out
def _item(x, ns):
    if isinstance(x, list): x = x[0] if x else {}
    if isinstance(x, str): return x if ':' in x else 'minecraft:' + x
    if x.get('type') == 'forge:ore_dict' or 'ore' in x: return 'ore:' + x.get('ore', '?')
    nm = x.get('item', '?'); nm = nm if ':' in nm else 'minecraft:' + nm
    d = x.get('data', 0); return nm + ('@%d' % d if d else '')
def recipes(jar, ns, ctx):
    L = []
    for f in jar.find(r'^assets/%s/recipes/[^_][^/]*\.json$' % re.escape(ns)):
        try:
            d = jar.json(f); t = d.get('type', '').split(':')[-1]; r = d.get('result', {})
            res = _item(r, ns) if isinstance(r, dict) else _item(r, ns); n = r.get('count', 1) if isinstance(r, dict) else 1
            if t in ('crafting_shaped', 'shaped', 'forge_ore_shaped'):
                key = ';'.join('%s=%s' % (c, _item(v, ns)) for c, v in d.get('key', {}).items())
                L.append('shaped %s %d %s %s' % (res, n, '|'.join(d.get('pattern', [])), key))
            elif t in ('crafting_shapeless', 'shapeless', 'forge_ore_shapeless'):
                L.append('shapeless %s %d %s' % (res, n, ','.join(_item(v, ns) for v in d.get('ingredients', []))))
            elif t == 'smelting': L.append('smelt %s %s %s' % (_item(d.get('ingredient', {}), ns), res, d.get('experience', 0)))
            else: ctx.gap('receita de tipo desconhecido', t or '?')
        except Exception as ex: ctx.gap('erro ao converter receita', '%s: %s' % (os.path.basename(f), str(ex)[:50]))
    return L
def drops(jar, ns, ctx):
    L = []
    for f in jar.find(r'^assets/%s/loot_tables/blocks/.*\.json$' % re.escape(ns)):
        try:
            d = jar.json(f); blk = os.path.basename(f)[:-5]
            for pool in d.get('pools', []):
                for e in pool.get('entries', []):
                    if e.get('type', 'item') != 'item': ctx.gap('drop de tipo desconhecido', e.get('type', '?')); continue
                    lo = hi = pool.get('rolls', 1) if not isinstance(pool.get('rolls', 1), dict) else pool['rolls'].get('min', 1)
                    for fn in e.get('functions', []):
                        if fn.get('function', '').endswith('set_count'):
                            c = fn.get('count', 1); lo, hi = (c.get('min', 1), c.get('max', 1)) if isinstance(c, dict) else (c, c)
                    conds = [c.get('condition', '?').split(':')[-1] for c in list(pool.get('conditions', [])) + list(e.get('conditions', []))]
                    nm = e.get('name', '?'); nm = nm if ':' in nm else 'minecraft:' + nm
                    L.append('%s -> %s %s-%s%s' % (blk, nm, lo, hi, (' se ' + ','.join(conds)) if conds else ''))
        except Exception as ex: ctx.gap('erro ao converter loot table', '%s: %s' % (os.path.basename(f), str(ex)[:50]))
    return L
def sounds(jar, ns, ctx):
    f = 'assets/%s/sounds.json' % ns; L = []
    if not jar.has(f): return L
    try:
        for ev, d in jar.json(f).items():
            for s in d.get('sounds', []):
                nm = s if isinstance(s, str) else s.get('name', '?'); L.append('%s %s' % (ev, nm if ':' in nm else ns + ':' + nm))
    except Exception as ex: ctx.gap('erro ao converter sounds.json', str(ex)[:50])
    return L
def translate(jar, ctx, out_dir, ns):
    """Roda tudo e grava os .txt na pasta do mod. ns = namespace (ou lista) do mod."""
    acts = scan(jar, ctx); nss = ns if isinstance(ns, list) else [ns]
    R = []; D = []; S = []
    for n in nss: R += recipes(jar, n, ctx); D += drops(jar, n, ctx); S += sounds(jar, n, ctx)
    os.makedirs(out_dir, exist_ok=True)
    for name, L in (('receitas.txt', R), ('drops.txt', D), ('sons.txt', S)):
        if L: open(os.path.join(out_dir, name), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    json.dump(acts, open(os.path.join(out_dir, 'forge_acoes.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1, sort_keys=True)
    ctx.stat('receitas', len(R)); ctx.stat('drops', len(D)); ctx.stat('sons', len(S))
    return acts
