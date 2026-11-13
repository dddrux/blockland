"""patchConv.6: engenharia reversa de bytecode (sem decompilar). Le o Code de cada metodo dos .class do mod e extrai FATOS de comportamento:
valores passados a setHardness/setResistance/setLightLevel/setHarvestLevel, retorno constante de getLightValue/getLightOpacity/getComparatorInputOverride,
e quais metodos de comportamento a classe sobrescreve (onBlockActivated, updateTick, ...). Saida: mods/<mod>/comportamento.json (por classe)."""
import json, os, struct
# bytes de operando por opcode (JVMS); -1 = tamanho variavel (tableswitch/lookupswitch/wide)
_L = {i: 0 for i in range(256)}
for o in (0x10, 0x12, 0x15, 0x16, 0x17, 0x18, 0x19, 0x36, 0x37, 0x38, 0x39, 0x3a, 0xa9, 0xbc): _L[o] = 1
for o in list(range(0x99, 0xa9)) + [0x11, 0x13, 0x14, 0x84, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xbb, 0xbd, 0xc0, 0xc1, 0xc6, 0xc7]: _L[o] = 2
_L[0xc5] = 3; _L[0xb9] = 4; _L[0xba] = 4; _L[0xc8] = 4; _L[0xc9] = 4; _L[0xaa] = -1; _L[0xab] = -1; _L[0xc4] = -1
INTERESSE = {'onBlockActivated', 'updateTick', 'neighborChanged', 'onNeighborChange', 'randomTick', 'onEntityCollidedWithBlock', 'onEntityWalk', 'getDrops', 'breakBlock', 'onBlockPlacedBy',
             'getStateForPlacement', 'canPlaceBlockAt', 'getWeakPower', 'getStrongPower', 'canProvidePower', 'getLightValue', 'getLightOpacity', 'hasComparatorInputOverride', 'onBlockClicked', 'harvestBlock',
             'onItemRightClick', 'onItemUse', 'onUpdate', 'onUsingTick', 'update', 'getMaxStackSize', 'onItemUseFinish', 'hitEntity', 'onEntityDamaged', 'isOpaqueCube', 'isFullCube', 'getPlayerRelativeBlockHardness'}
SETTERS = {'setHardness': 'dureza', 'setResistance': 'resistencia', 'setLightLevel': 'luz', 'setHarvestLevel': 'nivel_colheita', 'setLightOpacity': 'opacidade', 'setMaxStackSize': 'pilha', 'setMaxDamage': 'durabilidade'}
RETCONST = {'getBlockHardness': 'dureza_metodo', 'func_176195_g': 'dureza_metodo', 'getExplosionResistance': 'resistencia_metodo', 'func_149638_a': 'resistencia_metodo', 'func_180651_a': 'dano_dropado', 'damageDropped': 'dano_dropado', 'quantityDropped': 'qtd_dropada', 'func_149745_a': 'qtd_dropada', 'getLightValue': 'luz_metodo', 'getLightOpacity': 'opacidade_metodo', 'getMaxStackSize': 'pilha_metodo', 'getStrongPower': 'redstone_forte', 'getWeakPower': 'redstone_fraca'}

# patchMec.2: (classe, metodo) -> acao de jogo. Chaves em SRG (jar de release) e em nome legivel (dev). Vale so dentro de onBlockActivated.
ACOES = {('EntityPlayer', 'openGui'): 'gui', ('EntityPlayer', 'func_71007_a'): 'gui', ('EntityPlayer', 'displayGUIChest'): 'gui', ('SeatUtil', 'sitOnBlock'): 'sentar',
         ('World', 'func_180501_a'): 'estado', ('World', 'func_175656_a'): 'estado', ('World', 'setBlockState'): 'estado', ('IBlockState', 'func_177226_a'): 'estado', ('IBlockState', 'withProperty'): 'estado',
         ('IBlockState', 'func_177231_a'): 'estado', ('EntityPlayer', 'func_184185_a'): 'som', ('World', 'func_184134_a'): 'som', ('World', 'playSound'): 'som', ('EntityPlayer', 'playSound'): 'som',
         ('EntityPlayer', 'func_145747_a'): 'mensagem', ('EntityPlayer', 'sendMessage'): 'mensagem', ('ItemStack', 'func_190918_g'): 'consome_item', ('ItemStack', 'shrink'): 'consome_item',
         ('InventoryPlayer', 'func_70441_a'): 'da_item', ('InventoryPlayer', 'addItemStackToInventory'): 'da_item', ('World', 'func_175698_g'): 'remove_bloco', ('World', 'setBlockToAir'): 'remove_bloco',
         ('EntityPlayer', 'func_71024_bL'): 'fome', ('FoodStats', 'func_75122_a'): 'fome', ('FoodStats', 'addStats'): 'fome', ('EntityPlayer', 'func_70691_i'): 'cura', ('EntityLivingBase', 'func_70691_i'): 'cura'}
def _pool(d):
    n = struct.unpack('>H', d[8:10])[0]; p = 10; cp = [None] * n; i = 1
    while i < n:
        t = d[p]; p += 1
        if t == 1: l = struct.unpack('>H', d[p:p + 2])[0]; cp[i] = ('u', d[p + 2:p + 2 + l].decode('utf8', 'replace')); p += 2 + l
        elif t in (3, 4): cp[i] = ('i' if t == 3 else 'f', struct.unpack('>i' if t == 3 else '>f', d[p:p + 4])[0]); p += 4
        elif t in (5, 6): cp[i] = ('l' if t == 5 else 'd', struct.unpack('>q' if t == 5 else '>d', d[p:p + 8])[0]); p += 8; i += 1
        elif t in (7, 8, 16, 19, 20): cp[i] = ('r', struct.unpack('>H', d[p:p + 2])[0]); p += 2
        elif t in (9, 10, 11, 12, 17, 18): cp[i] = ('r2', struct.unpack('>HH', d[p:p + 4])); p += 4
        elif t == 15: p += 3
        else: raise ValueError('tag %d' % t)
        i += 1
    return cp, p

SRG = {'func_149711_c': 'setHardness', 'func_149752_b': 'setResistance', 'func_149715_a': 'setLightLevel', 'func_149713_g': 'setLightOpacity', 'func_180639_a': 'onBlockActivated', 'func_180650_b': 'updateTick', 'func_189540_a': 'neighborChanged', 'func_176195_g': 'getBlockHardness'}   # mods vem reofuscados (SRG): traduz os nomes que importam
def _u(cp, i): return cp[i][1] if cp[i] and cp[i][0] == 'u' else ''
def _cls(cp, i): return _u(cp, cp[i][1]) if cp[i] and cp[i][0] == 'r' else ''
def _mname(cp, i):
    e = cp[i]
    if not e or e[0] != 'r2': return ''
    nt = cp[e[1][1]]; return SRG.get(_u(cp, nt[1][0]), _u(cp, nt[1][0])) if nt and nt[0] == 'r2' else ''

def _attrs(d, p, cp):
    n = struct.unpack('>H', d[p:p + 2])[0]; p += 2; out = []
    for _ in range(n):
        nm = _u(cp, struct.unpack('>H', d[p:p + 2])[0]); ln = struct.unpack('>I', d[p + 2:p + 6])[0]; out.append((nm, p + 6, ln)); p += 6 + ln
    return out, p

def analisar_classe(d):
    if d[:4] != b'\xca\xfe\xba\xbe': return None
    cp, p = _pool(d); p += 6; this = _cls(cp, struct.unpack('>H', d[p - 4:p - 2])[0]); sup = _cls(cp, struct.unpack('>H', d[p - 2:p])[0])
    ni = struct.unpack('>H', d[p:p + 2])[0]; p += 2 + ni * 2
    for _ in range(2):   # campos, depois metodos
        n = struct.unpack('>H', d[p:p + 2])[0]; p += 2; items = []
        for _ in range(n):
            nm = _u(cp, struct.unpack('>H', d[p + 2:p + 4])[0]); nm = SRG.get(nm, nm); at, p = _attrs(d, p + 6, cp); items.append((nm, at))
        meths = items
    fatos = {}; sobre = []; acoes = set(); args = {}   # patchForge.4: args = argumentos constantes das acoes (mensagem/som)
    for nm, at in meths:
        if nm in INTERESSE: sobre.append(nm)
        for an, ap, al in at:
            if an != 'Code': continue
            ml = struct.unpack('>I', d[ap + 4:ap + 8])[0]; code = d[ap + 8:ap + 8 + ml]; i = 0; last = None; strs = []; lastS = None   # patchMec.5b: ultima string ldc (classe da ferramenta de setHarvestLevel)
            while i < len(code):
                op = code[i]; ln = _L[op]
                if op in (0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08): last = op - 3
                elif op in (0x0b, 0x0c, 0x0d): last = float(op - 0x0b)
                elif op == 0x10: last = struct.unpack('>b', code[i + 1:i + 2])[0]
                elif op == 0x11: last = struct.unpack('>h', code[i + 1:i + 3])[0]
                elif op in (0x12, 0x13):
                    k = code[i + 1] if op == 0x12 else struct.unpack('>H', code[i + 1:i + 3])[0]
                    if cp[k] and cp[k][0] in ('i', 'f'): last = round(cp[k][1], 4)
                    elif cp[k] and cp[k][0] == 'r' and nm in ('onBlockActivated', 'func_180639_a'): strs.append(_u(cp, cp[k][1]))
                    elif cp[k] and cp[k][0] == 'r': lastS = _u(cp, cp[k][1])   # patchForge.4: ldc "texto"
                elif op in (0xb6, 0xb7, 0xb8, 0xb9):
                    _ix = struct.unpack('>H', code[i + 1:i + 3])[0]; m = _mname(cp, _ix)
                    if nm in ('onBlockActivated', 'func_180639_a') and cp[_ix] and cp[_ix][0] == 'r2':
                        _a = ACOES.get((_cls(cp, cp[_ix][1][0]).split('/')[-1], m))
                        if _a: acoes.add(_a)
                        _o = _cls(cp, cp[_ix][1][0]).split('/')[-1]   # patchForge.4: new TextComponent*("chave") / new SoundEvent(new ResourceLocation(ns, "nome"))
                        if m == '<init>' and strs and _o.startswith('TextComponent'): args.setdefault('mensagem', set()).add(strs[-1])
                        elif m == '<init>' and strs and _o == 'SoundEvent': args.setdefault('som', set()).add(strs[-1])
                    if m in SETTERS and last is not None and (nm in ('<init>', 'registerBlocks', 'init') or nm.startswith('lambda$')): fatos.setdefault(SETTERS[m], last); (fatos.setdefault('ferramenta', lastS) if m == 'setHarvestLevel' and lastS in ('pickaxe', 'axe', 'shovel') else None)
                elif op in (0xac, 0xae) and nm in RETCONST and last is not None: fatos.setdefault(RETCONST[nm], last)
                if ln == -1:
                    if op == 0xc4: ln = 5 if code[i + 1] == 0x84 else 3
                    else:
                        b = (i + 4) & ~3
                        if op == 0xaa: lo, hi = struct.unpack('>ii', code[b + 4:b + 12]); ln = b + 12 + (hi - lo + 1) * 4 - i - 1
                        else: n = struct.unpack('>i', code[b + 4:b + 8])[0]; ln = b + 8 + n * 8 - i - 1
                i += 1 + ln
    if sobre: fatos['metodos'] = sorted(set(sobre))
    if acoes: fatos['acoes'] = sorted(acoes)
    if args: fatos['acoes_args'] = {k: sorted(v) for k, v in args.items()}
    return this, sup, fatos

def extract(jar_path, out_dir, ctx=None):
    import zipfile
    res = {}
    with zipfile.ZipFile(jar_path) as z:
        for n in z.namelist():
            if not n.endswith('.class'): continue
            try: r = analisar_classe(z.read(n))
            except Exception: continue
            if r and r[2]: res[r[0]] = dict(r[2], super=r[1])
    if res:
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, 'comportamento.json'), 'w', encoding='utf-8') as f: json.dump(res, f, ensure_ascii=False, indent=1, sort_keys=True)
    return res
