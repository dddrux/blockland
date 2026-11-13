"""patchF2: le os .class do mod (so a tabela de constantes) e descobre o PAPEL de cada classe no Forge 1.12.2, seguindo a heranca dentro do jar.
Papeis: block, container(bloco com TileEntity/BlockContainer), tile, inventory(IInventory/IItemHandler), itemblock (item que COLOCA um bloco), item, armor, sword, tool, bow, food, gui, guihandler, hud (overlay/evento de tela), entity.
Tambem devolve as strings curtas de cada classe (nomes de registro: setRegistryName("bath"), materiais de armadura, nomes de textura)."""
import re, struct
BASE = {   # classe base (vanilla/Forge) -> papel
 'net/minecraft/block/Block': 'block', 'net/minecraft/block/BlockContainer': 'container', 'net/minecraft/block/ITileEntityProvider': 'container',
 'net/minecraft/block/BlockChest': 'container', 'net/minecraft/block/BlockHorizontal': 'block', 'net/minecraft/block/BlockDoor': 'door', 'net/minecraft/block/BlockSlab': 'slab',
 'net/minecraft/block/BlockStairs': 'stairs', 'net/minecraft/block/BlockFence': 'fence', 'net/minecraft/block/BlockPane': 'pane', 'net/minecraft/block/BlockBed': 'bed',
 'net/minecraft/block/BlockFalling': 'falling', 'net/minecraft/block/BlockLiquid': 'fluid', 'net/minecraftforge/fluids/BlockFluidBase': 'fluid', 'net/minecraft/block/BlockCrops': 'crop',
 'net/minecraft/tileentity/TileEntity': 'tile', 'net/minecraft/tileentity/TileEntityChest': 'inventory', 'net/minecraft/tileentity/TileEntityLockable': 'inventory',
 'net/minecraft/inventory/IInventory': 'inventory', 'net/minecraft/inventory/ISidedInventory': 'inventory', 'net/minecraftforge/items/IItemHandler': 'inventory',
 'net/minecraftforge/items/ItemStackHandler': 'inventory', 'net/minecraftforge/items/IItemHandlerModifiable': 'inventory',
 'net/minecraft/item/Item': 'item', 'net/minecraft/item/ItemBlock': 'itemblock', 'net/minecraft/item/ItemArmor': 'armor', 'net/minecraftforge/common/ISpecialArmor': 'armor',
 'net/minecraft/item/ItemSword': 'sword', 'net/minecraft/item/ItemPickaxe': 'tool', 'net/minecraft/item/ItemAxe': 'tool', 'net/minecraft/item/ItemSpade': 'tool', 'net/minecraft/item/ItemHoe': 'tool', 'net/minecraft/item/ItemTool': 'tool',
 'net/minecraft/item/ItemBow': 'bow', 'net/minecraft/item/ItemFood': 'food', 'net/minecraft/item/ItemShield': 'shield',
 'net/minecraft/client/gui/GuiScreen': 'gui', 'net/minecraft/client/gui/inventory/GuiContainer': 'gui', 'net/minecraft/client/gui/Gui': 'gui', 'net/minecraft/inventory/Container': 'guicontainer',
 'net/minecraftforge/fml/common/network/IGuiHandler': 'guihandler', 'net/minecraft/client/renderer/tileentity/TileEntitySpecialRenderer': 'tesr', 'net/minecraft/client/renderer/entity/Render': 'entityrender', 'net/minecraft/entity/Entity': 'entity', 'net/minecraft/entity/EntityLiving': 'entity',
}
USE = {   # classe usada (referenciada) -> papel de "faz coisa de interface" (nao e heranca)
 'net/minecraftforge/client/event/RenderGameOverlayEvent': 'hud', 'net/minecraftforge/client/event/GuiOpenEvent': 'hud', 'net/minecraftforge/client/event/GuiScreenEvent': 'hud',
 'net/minecraft/client/gui/ScaledResolution': 'hud', 'net/minecraft/client/gui/GuiIngame': 'hud', 'net/minecraftforge/client/GuiIngameForge': 'hud',
}
def parse(data):
    """-> dict(this, sup, ifs, strs, refs{owner}) ou None."""
    try:
        if data[:4] != b'\xca\xfe\xba\xbe': return None
        n = struct.unpack('>H', data[8:10])[0]; p = 10; cp = [None] * n; i = 1
        while i < n:
            t = data[p]; p += 1
            if t == 1: ln = struct.unpack('>H', data[p:p + 2])[0]; cp[i] = ('u', data[p + 2:p + 2 + ln].decode('utf-8', 'replace')); p += 2 + ln
            elif t == 3: p += 4
            elif t == 4: cp[i] = ('f', struct.unpack('>f', data[p:p + 4])[0]); p += 4
            elif t in (5, 6): p += 8; i += 1
            elif t in (7, 8, 16, 19, 20): cp[i] = (t, struct.unpack('>H', data[p:p + 2])[0]); p += 2
            elif t == 12: cp[i] = (12, struct.unpack('>HH', data[p:p + 2 * 2])); p += 4
            elif t in (9, 10, 11): cp[i] = (t, struct.unpack('>HH', data[p:p + 4])); p += 4
            elif t in (17, 18): p += 4
            elif t == 15: p += 3
            else: return None
            i += 1
        u = lambda k: cp[k][1] if cp[k] and cp[k][0] == 'u' else ''
        cn = lambda k: u(cp[k][1]) if cp[k] and cp[k][0] == 7 else ''
        p += 2; this = cn(struct.unpack('>H', data[p:p + 2])[0]); p += 2; sup = cn(struct.unpack('>H', data[p:p + 2])[0]); p += 2
        ni = struct.unpack('>H', data[p:p + 2])[0]; p += 2; ifs = [cn(struct.unpack('>H', data[p + 2 * j:p + 2 * j + 2])[0]) for j in range(ni)]
        strs = [u(e[1]) for e in cp if e and e[0] == 8]; owners = {u(e[1]) for e in cp if e and e[0] == 7}
        meth = {u(cp[e[1][1]][1][0]) for e in cp if e and e[0] in (10, 11) and cp[e[1][1]] and cp[e[1][1]][0] == 12}
        floats = sorted({round(e[1], 3) for e in cp if e and e[0] == 'f' and 0 < e[1] <= 1.0})
        return dict(this=this, sup=sup, ifs=ifs, strs=strs, refs=owners, meth=meth, floats=floats)
    except Exception: return None
def analyse(jar):
    """-> {classe: dict(roles=[...], strs=[nomes curtos], simple='BlockBath')}"""
    raw = {}
    for nm in jar.names:
        if nm.endswith('.class'):
            c = parse(jar.read(nm))
            if c and c['this']: raw[c['this']] = c
    memo = {}
    def roles(c, d=0):
        if c in memo: return memo[c]
        memo[c] = set(); r = set()
        if c in BASE: r.add(BASE[c])
        k = raw.get(c)
        if k and d < 12:
            for b in [k['sup']] + k['ifs']: r |= roles(b, d + 1)
        if k:
            for o in k['refs']:
                if o in USE: r.add(USE[o])
        memo[c] = r; return r
    out = {}
    for c, k in raw.items():
        r = roles(c)
        strs = sorted({s for s in k['strs'] if re.fullmatch(r'[a-z0-9_]{2,40}', s)})
        out[c] = dict(light=(max(k['floats']) if k['floats'] and (k['meth'] & {'func_149715_a', 'setLightLevel'}) else 0), roles=sorted(r), strs=strs, ores=sorted({x for x in k['strs'] if re.fullmatch(r'(ingot|ore|dust|gem|plank|log|block|nugget|plate|gear|stick|rod|crystal|dye|slab|stair|wool|glass)[A-Z][A-Za-z0-9]*', x)}), paths=sorted({x for x in k['strs'] if re.search(r'textures/gui/|\.png$', x)}), simple=c.rsplit('/', 1)[-1], sup=k['sup'], refs=sorted(o for o in k['refs'] if o in raw and o != c))
    return out
def snake(camel): return re.sub(r'(?<!^)(?=[A-Z])', '_', camel).lower()
def by_name(info, names, role):
    """liga nomes de blockstate/item a classes com o papel: casa por string de registro ou pelo nome da classe (BlockBath -> bath)."""
    res = {}
    cls = {c: v for c, v in info.items() if role in v['roles']}
    for nm in names:
        for c, v in cls.items():
            base = re.sub(r'^(Block|Item|TileEntity)', '', v['simple']); sn = snake(base)
            if nm in v['strs'] or (sn and (nm == sn or nm.startswith(sn + '_'))):
                res.setdefault(nm, []).append(c)
    return res
def stores_items(info, cls):
    """True se a classe do bloco usa (direto ou 1 nivel abaixo) uma classe com papel inventory = tem armazenamento de itens de verdade."""
    seen = set(); todo = [cls]
    for _ in range(2):
        nxt = []
        for c in todo:
            v = info.get(c)
            if not v or c in seen: continue
            seen.add(c)
            if 'inventory' in v['roles'] and 'block' not in v['roles'] and 'container' not in v['roles']: return True
            nxt += v['refs']
        todo = nxt
    return False
