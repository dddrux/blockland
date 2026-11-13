"""patchMod.4: tradutor de acoes do Forge 1.12.2 -> Blockland.
1) Varre os .class do jar (so a tabela de constantes, sem decompilar) e classifica cada chamada net/minecraft* e net/minecraftforge* pela TABELA abaixo.
2) Traduz o que e declarativo: receitas (assets/<ns>/recipes), drops (loot_tables), sons (sounds.json) para arquivos de texto simples.
Saidas em mods/<mod>/: receitas.txt, drops.txt, sons.txt, forge_acoes.json. O que nao tem equivalente vai para o RELATORIO e para db/unknown.json."""
import json, os, re, struct
try:
    from . import forgeapi, srgnames, dropscode
except ImportError:
    import forgeapi, srgnames, dropscode
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

# patchForge.1: quando nenhuma linha da TABLE casa (ex.: jar de release com nomes SRG func_/field_), classifica pelo PACOTE do dono da chamada. (prefixo, status, o que o Blockland faz). Maior prefixo primeiro.
CATS = [
    ('net/minecraftforge/fml/relauncher/', 'ignorado', 'SideOnly/Side: o Blockland roda cliente e servidor no mesmo processo'),
    ('net/minecraftforge/fml/common/registry/', 'convertido', 'registro de blocos/itens/entidades: vem dos arquivos do jar'),
    ('net/minecraftforge/registries/', 'convertido', 'registro de blocos/itens: vem dos arquivos do jar'),
    ('net/minecraftforge/fml/common/network/', 'sem equivalente', 'pacotes de rede do mod nao sao traduzidos'),
    ('net/minecraftforge/common/network/', 'sem equivalente', 'pacotes de rede do mod nao sao traduzidos'),
    ('net/minecraftforge/fml/client/', 'ignorado', 'tela/cliente do Forge (lista de mods, config)'),
    ('net/minecraftforge/client/model/', 'aproximado', 'modelos: JSON/OBJ viram caixas (OBJ ainda 1 caixa)'),
    ('net/minecraftforge/client/', 'ignorado', 'renderizacao/cliente do Forge'),
    ('net/minecraftforge/common/config/', 'ignorado', 'configuracao do mod nao e lida'),
    ('net/minecraftforge/common/capabilities/', 'sem equivalente', 'capabilities nao existem no Blockland'),
    ('net/minecraftforge/fluids/', 'sem equivalente', 'fluidos de mod (tanques, baldes) nao existem'),
    ('net/minecraftforge/items/', 'sem equivalente', 'IItemHandler: so Container (beh) guarda itens'),
    ('net/minecraftforge/oredict/', 'aproximado', 'ore:<nome> em receitas.txt'),
    ('net/minecraftforge/energy/', 'sem equivalente', 'energia (Forge Energy) nao existe'),
    ('net/minecraftforge/event/terraingen/', 'sem equivalente', 'geracao de mundo por evento nao e traduzida'),
    ('net/minecraftforge/event/', 'sem equivalente', 'eventos do Forge nao sao disparados (ver forge_eventos.json)'),
    ('net/minecraftforge/fml/common/event/', 'ignorado', 'ciclo de vida do Forge: o Blockland carrega o mod pela pasta mods/'),
    ('net/minecraftforge/fml/common/', 'ignorado', 'infraestrutura de mod do Forge (Loader, Mod, ModContainer)'),
    ('net/minecraftforge/', 'ignorado', 'utilitario do Forge sem efeito sozinho'),
    ('cpw/mods/', 'ignorado', 'infraestrutura de mod do FML'),
    ('net/minecraft/client/', 'ignorado', 'cliente/render vanilla: o Blockland desenha pelos modelos convertidos'),
    ('net/minecraft/tileentity/', 'sem equivalente', 'TileEntity por codigo nao e traduzida'),
    ('net/minecraft/entity/', 'sem equivalente', 'entidades/mobs por codigo nao sao traduzidos'),
    ('net/minecraft/world/gen/', 'sem equivalente', 'geracao de mundo por codigo nao e traduzida'),
    ('net/minecraft/world/', 'sem equivalente', 'acao no mundo por codigo nao e traduzida'),
    ('net/minecraft/inventory/', 'sem equivalente', 'inventario/GUI de mod nao existem'),
    ('net/minecraft/network/', 'sem equivalente', 'pacotes de rede nao sao traduzidos'),
    ('net/minecraft/potion/', 'sem equivalente', 'efeitos de pocao de mod nao existem'),
    ('net/minecraft/enchantment/', 'sem equivalente', 'encantamentos de mod nao existem'),
    ('net/minecraft/block/', 'aproximado', 'bloco: forma/textura vem dos modelos; comportamento por codigo nao'),
    ('net/minecraft/item/', 'aproximado', 'item: nome/textura vem dos arquivos; comportamento por codigo nao'),
    ('net/minecraft/', 'ignorado', 'tipo/utilitario vanilla (util, nbt, init, text): sem efeito sozinho'),
]
def classify_cat(key):
    for pre, s, a in CATS:
        if key.startswith(pre): return s, a, 'por pacote'
    return None
_TABLE = [(re.compile(p), s, a, n) for p, s, a, n in TABLE]
def classify(key):
    for r, s, a, n in _TABLE:
        if r.search(key): return s, a, n
    c = classify_cat(key)
    return c if c else ('sem equivalente', 'chamada desconhecida', '')

LAST = {'eventos': {}, 'mods': []}   # preenchido por scan(): eventos @SubscribeEvent usados e @Mod do jar
def event_status(k):
    """classe de evento do Forge -> (status, o que o Blockland faz)."""
    n = k.rsplit('/', 1)[-1]
    if re.search(r'ModelRegistryEvent|ModelBakeEvent|TextureStitchEvent|RegistryEvent', n): return 'convertido', 'modelos, texturas e registro vem dos arquivos do jar'
    if k.startswith('net/minecraftforge/client/') or k.startswith('net/minecraftforge/fml/client/'): return 'ignorado', 'evento de cliente (tela, render, som): sem efeito no Blockland'
    if re.search(r'ConfigChangedEvent|FML\w*Event|ModLifecycle', n): return 'ignorado', 'ciclo de vida/config do Forge'
    if k.startswith('net/minecraftforge/event/terraingen/'): return 'sem equivalente', 'geracao de mundo por evento nao e traduzida'
    return 'sem equivalente', 'o Blockland nao dispara eventos do Forge: a logica do handler nao e traduzida'
def _collect(c, ctx):
    """c = parse_class: guarda handlers @SubscribeEvent e @Mod."""
    for an in c['anots']:
        t = forgeapi.short(an[0])
        if t == 'Mod' and 'modid' in an[1]: LAST['mods'].append(dict({k: v for k, v in an[1].items() if isinstance(v, (str, int, bool))}, classe=c['name']))
    for nm, desc, fl, ans in c['methods']:
        if not any(forgeapi.short(a[0]) == 'SubscribeEvent' for a in ans): continue
        m = re.match(r'\(L([^;]+);\)V', desc)
        if not m: continue
        ek = m.group(1); d = LAST['eventos'].setdefault(ek, dict(handlers=[]))
        d['handlers'].append(c['name'].rsplit('/', 1)[-1] + '.' + nm)
        pr = [a[1].get('priority') for a in ans if forgeapi.short(a[0]) == 'SubscribeEvent']
        if pr and pr[0]: d.setdefault('prioridades', []).append(pr[0])
def _finish_events():
    A = forgeapi.api()
    for ek, d in LAST['eventos'].items():
        d['status'], d['alvo'] = event_status(ek); i = A.event(ek) if A.ok else None
        if i: d.update(i)
        elif A.ok and not A.is_forge(ek): d['api'] = 'evento de outro mod/Minecraft (nao e do Forge)'
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
def api_state(A, k):
    """a chamada existe? forge = existe no catalogo do Forge | forge? = ausente (mod de outra versao?) | mc = nome SRG do Minecraft 1.12.2 | mc? = SRG que nao existe na 1.12.2 | mc-dev = nome legivel | ? = sem catalogo."""
    if not A.ok: return '?'
    ow, _, nm = k.rpartition('.')
    if nm == '<extends>': return 'forge' if A.is_forge(ow) else ('mc' if A.is_mc(ow) else 'externo')
    if ow.startswith('net/minecraftforge/') or ow.startswith('cpw/mods/'):
        h = A.has(ow, nm)
        if h == 'mc': return 'mc' if (nm.startswith(('func_', 'field_')) and A.mc_known(nm)) or not nm.startswith(('func_', 'field_')) else 'mc?'
        return 'forge' if h else ('mc' if nm.startswith(('func_', 'field_')) and A.mc_known(nm) else 'forge?')
    if ow.startswith('net/minecraft/'):
        if nm.startswith(('func_', 'field_')): return 'mc' if A.mc_known(nm) else 'mc?'
        return 'mc' if A.is_mc(ow) or ow not in A.mc_cls else 'mc-dev'
    return 'externo'
def scan(jar, ctx):
    """-> {chave: {n, status, alvo, nota}}; registra lacunas em ctx."""
    out = {}; classes = 0; LAST['eventos'] = {}; LAST['mods'] = []; A = forgeapi.api()
    def add(k):
        if not (k.startswith('net/minecraft') or k.startswith('net/minecraftforge') or k.startswith('cpw/mods')): return
        d = out.setdefault(k, dict(n=0)); d['n'] += 1
    for nm in jar.names:
        if not nm.endswith('.class'): continue
        r = class_refs(jar.read(nm))
        if not r: continue
        classes += 1; sup, ifs, refs = r
        c = forgeapi.parse_class(jar.read(nm))
        if c: _collect(c, ctx)
        for k in [sup] + ifs: add(k + '.<extends>')
        for k in refs: add(k)
    for k, d in out.items():
        d['status'], d['alvo'], d['nota'] = classify(k)
        d['api'] = api_state(A, k)
        rk = srgnames.readable(k)
        if rk != k: d['nome'] = rk.rpartition('.')[2]   # patchForge.3: nome legivel (MCP) do func_/field_
        if d['api'] in ('forge?', 'mc?'): ctx.gap('Forge: chamada ausente na 1.12.2 (mod de outra versao?)', k.replace('net/minecraftforge/', 'forge/').replace('net/minecraft/', 'mc/'), d['n'])
        if d['status'] in ('sem equivalente', 'aproximado'): ctx.gap('Forge: ' + d['status'], (lambda r, k: r if r == k else '%s [%s]' % (r, k.rpartition('.')[2]))(srgnames.readable(k), k).replace('net/minecraftforge/', 'forge/').replace('net/minecraft/', 'mc/'), d['n'])
    ok, tot = srgnames.coverage(out); ctx.stat('srg_com_nome', '%d/%d' % (ok, tot))
    _finish_events(); ctx.stat('eventos_forge', len(LAST['eventos'])); ctx.stat('classes_java', classes); ctx.stat('chamadas_forge', len(out))
    return out
def _item(x, ns):
    if isinstance(x, list): x = x[0] if x else {}
    if isinstance(x, str) and x in _CONST: x = _CONST[x]   # patchF8: constante de _constants.json
    if isinstance(x, list): x = x[0] if x else {}
    if isinstance(x, str): return x if ':' in x else 'minecraft:' + x
    if x.get('type') == 'forge:ore_dict' or 'ore' in x: return 'ore:' + x.get('ore', '?')
    nm = x.get('item', '?'); nm = nm if ':' in nm else 'minecraft:' + nm
    d = x.get('data', 0); return nm + ('@%d' % d if d else '')
_CONST = {}
def _forge_recipe_meta(jar, ns):
    """patchF8: o Forge 1.12.2 le recipes/_factories.json (tipo -> classe Java) e recipes/_constants.json (nomes reutilizaveis de ingrediente)."""
    fac = {}; _CONST.clear()
    try: fac = jar.json('assets/%s/recipes/_factories.json' % ns).get('recipes', {}) if jar.has('assets/%s/recipes/_factories.json' % ns) else {}
    except Exception: pass
    try:
        if jar.has('assets/%s/recipes/_constants.json' % ns):
            c = jar.json('assets/%s/recipes/_constants.json' % ns)
            for e in (c if isinstance(c, list) else [c]):
                for k, v in e.items():
                    if isinstance(v, (dict, list)): _CONST[k] = v
                if 'name' in e and 'ingredient' in e: _CONST[e['name']] = e['ingredient']
    except Exception: pass
    return fac
def _kind(t, fac):
    """tipo de receita -> 'shaped'|'shapeless'|'smelt'|None. Tipo de fabrica propria (cutting_shapeless) herda do nome."""
    t = t.split(':')[-1]
    if t in ('crafting_shaped', 'shaped', 'forge_ore_shaped', 'ore_shaped'): return 'shaped'
    if t in ('crafting_shapeless', 'shapeless', 'forge_ore_shapeless', 'ore_shapeless'): return 'shapeless'
    if t == 'smelting': return 'smelt'
    if t in fac:   # patchForge.2: a fabrica registrada em _factories.json diz o que a receita e (ShapedFactory, ShapelessFactory, ...)
        c = str(fac[t]).rsplit('.', 1)[-1].lower()
        for w, k in (('shapeless', 'shapeless'), ('shaped', 'shaped'), ('smelt', 'smelt'), ('furnace', 'smelt')):
            if w in c: return k
    if t in fac or t.endswith('shapeless') or t.endswith('shaped'): return 'shapeless' if t.endswith('shapeless') else ('shaped' if t.endswith('shaped') else None)
LOADED = {'minecraft', 'forge', 'fml'}   # mods "carregados" para as condicoes das receitas (translate() acrescenta o proprio mod e os ja convertidos)
def cond_ok(c, ctx=None):
    """patchForge.2: condicoes de receita do Forge 1.12.2 (CraftingHelper): forge:mod_loaded, minecraft:item_exists, forge:not/or/and, forge:false. Condicao falsa = o Forge nem carrega a receita."""
    if isinstance(c, list): return all(cond_ok(x, ctx) for x in c)
    t = str(c.get('type', '')).split(':')[-1]
    if t == 'false': return False
    if t == 'mod_loaded': return str(c.get('modid', '')).lower() in LOADED
    if t == 'item_exists':
        nm = str(c.get('item', 'minecraft:air')); return (nm.split(':')[0] if ':' in nm else 'minecraft').lower() in LOADED
    if t == 'not': return not cond_ok(c.get('value', {}), ctx)
    if t == 'or': return any(cond_ok(x, ctx) for x in c.get('values', []))
    if t == 'and': return all(cond_ok(x, ctx) for x in c.get('values', []))
    if ctx: ctx.gap('condicao de receita desconhecida (assumida verdadeira)', c.get('type', '?'))
    return True
def recipes(jar, ns, ctx):
    L = []; fac = _forge_recipe_meta(jar, ns)
    for f in jar.find(r'^assets/%s/recipes/[^_][^/]*\.json$' % re.escape(ns)):
        try:
            d = jar.json(f)
            if not cond_ok(d.get('conditions', []), ctx): ctx.gap('receita ignorada (condicao do Forge falsa: mod ausente)', os.path.basename(f)[:-5]); continue
            t = d.get('type', '').split(':')[-1]; r = d.get('result', {}); kd = _kind(t, fac)
            res = _item(r, ns) if isinstance(r, dict) else _item(r, ns); n = r.get('count', 1) if isinstance(r, dict) else 1
            if kd == 'shaped':
                key = ';'.join('%s=%s' % (c, _item(v, ns)) for c, v in d.get('key', {}).items())
                L.append('shaped %s %d %s %s' % (res, n, '|'.join(d.get('pattern', [])), key))
            elif kd == 'shapeless':
                L.append('shapeless %s %d %s' % (res, n, ','.join(_item(v, ns) for v in d.get('ingredients', []))))
            elif kd == 'smelt': L.append('smelt %s %s %s' % (_item(d.get('ingredient', {}), ns), res, d.get('experience', 0)))
            else: ctx.gap('receita de tipo desconhecido', (t or '?') + (' -> ' + fac[t].rsplit('.', 1)[-1] if t in fac else ''))
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
def extract_sounds(jar, nss, out_dir):
    """patchMod.15: copia assets/<ns>/sounds/**/*.ogg para <mod>/sounds/<ns>/... (o jogo ainda nao toca; ver PENDENCIAS). Devolve quantos copiou."""
    n = 0
    for ns in nss:
        for f in jar.find(r'^assets/%s/sounds/.+\.ogg$' % re.escape(ns)):
            dst = os.path.join(out_dir, 'sounds', ns, f.split('/sounds/', 1)[1])
            os.makedirs(os.path.dirname(dst), exist_ok=True); open(dst, 'wb').write(jar.read(f)); n += 1
    return n
def translate(jar, ctx, out_dir, ns):
    """Roda tudo e grava os .txt na pasta do mod. ns = namespace (ou lista) do mod."""
    acts = scan(jar, ctx); nss = ns if isinstance(ns, list) else [ns]
    LOADED.update(n.lower() for n in nss); LOADED.update(str(m.get('modid', '')).lower() for m in LAST['mods'])
    try: LOADED.update(d.lower() for d in os.listdir(os.path.dirname(os.path.abspath(out_dir))) if os.path.isdir(os.path.join(os.path.dirname(os.path.abspath(out_dir)), d)))   # mods ja convertidos
    except OSError: pass
    R = []; D = []; S = []
    for n in nss: R += recipes(jar, n, ctx); D += drops(jar, n, ctx); S += sounds(jar, n, ctx)
    try:   # patchForge.3: drops escritos em Java (getItemDropped/quantityDropped) -> mesma drops.txt
        dl, rep = dropscode.scan(jar, nss[0]); have = {x.split(' -> ')[0] for x in D}; D += [x for x in dl if x.split(' -> ')[0] not in have]
        for k, v in rep.items():
            if k in ('estado', 'desconhecido', 'vanilla?', 'codigo proprio'): ctx.gap('drop por codigo nao traduzido: ' + k, ', '.join(sorted(set(v))[:12]) + (' ...' if len(set(v)) > 12 else ''), len(set(v)))
        ctx.stat('drops_por_codigo', len(dl))
    except Exception as ex: ctx.gap('erro ao ler drops do bytecode', str(ex)[:60])
    os.makedirs(out_dir, exist_ok=True)
    for name, L in (('receitas.txt', R), ('drops.txt', D), ('sons.txt', S)):
        if L: open(os.path.join(out_dir, name), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    json.dump(acts, open(os.path.join(out_dir, 'forge_acoes.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1, sort_keys=True)
    if LAST['eventos']: json.dump(LAST['eventos'], open(os.path.join(out_dir, 'forge_eventos.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1, sort_keys=True)
    if LAST['mods']: json.dump(LAST['mods'], open(os.path.join(out_dir, 'forge_mod.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    ctx.stat('receitas', len(R)); ctx.stat('drops', len(D)); ctx.stat('sons', len(S))
    ctx.stat('sons_ogg', extract_sounds(jar, nss, out_dir))
    return acts
