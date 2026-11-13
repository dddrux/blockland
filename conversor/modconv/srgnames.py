"""patchForge.3: nomes legiveis (MCP 1.12.2) para os nomes SRG (func_/field_) que os jars de release usam.
O jar do Forge so traz notch->SRG; o nome legivel vem de duas fontes:
 1) csv do MCP, se voce colocar em conversor/modconv/db/mcp/ (methods.csv e fields.csv, colunas `searge,name,...`): cobre TUDO;
 2) a tabela curada abaixo (so o que e comum nos mods; metodos conferidos pelo DESCRITOR, entao um nome SRG com assinatura diferente nao e rotulado errado).
Uso: readable('net/minecraft/item/ItemStack.func_77973_b') -> 'net/minecraft/item/ItemStack.getItem'; method_name(srg, desc); vanilla_item(field_srg) -> nome de registro (ex.: 'coal')."""
import csv, os

_HERE = os.path.dirname(os.path.abspath(__file__))
_MCP = os.path.join(_HERE, 'db', 'mcp')

# --- metodos: srg -> (nome, sufixo do descritor que precisa casar ou '' = qualquer) ---
METHODS = {
    # Block
    'func_180661_e': ('createBlockState', '()Lnet/minecraft/block/state/BlockStateContainer;'),
    'func_185496_a': ('getBoundingBox', 'Lnet/minecraft/util/math/AxisAlignedBB;'),
    'func_185477_a': ('addCollisionBoxToList', ')V'),
    'func_180639_a': ('onBlockActivated', 'Z'),
    'func_176201_c': ('getMetaFromState', '(Lnet/minecraft/block/state/IBlockState;)I'),
    'func_176203_a': ('getStateFromMeta', '(I)Lnet/minecraft/block/state/IBlockState;'),
    'func_149662_c': ('isOpaqueCube', '(Lnet/minecraft/block/state/IBlockState;)Z'),
    'func_149686_d': ('isFullCube', '(Lnet/minecraft/block/state/IBlockState;)Z'),
    'func_149915_a': ('createNewTileEntity', '(Lnet/minecraft/world/World;I)Lnet/minecraft/tileentity/TileEntity;'),
    'func_180633_a': ('onBlockPlacedBy', ')V'),
    'func_176221_a': ('getActualState', 'Lnet/minecraft/block/state/IBlockState;'),
    'func_180664_k': ('getBlockLayer', '()Lnet/minecraft/util/BlockRenderLayer;'),
    'func_193383_a': ('getBlockFaceShape', 'Lnet/minecraft/block/state/BlockFaceShape;'),
    'func_180642_a': ('getStateForPlacement', 'Lnet/minecraft/block/state/IBlockState;'),
    'func_180660_a': ('getItemDropped', 'Lnet/minecraft/item/Item;'),
    'func_149745_a': ('quantityDropped', '(Ljava/util/Random;)I'),
    'func_149679_a': ('quantityDroppedWithBonus', '(ILjava/util/Random;)I'),
    'func_180651_a': ('damageDropped', '(Lnet/minecraft/block/state/IBlockState;)I'),
    'func_189540_a': ('neighborChanged', ')V'),
    'func_180646_a': ('getSelectedBoundingBox', 'Lnet/minecraft/util/math/AxisAlignedBB;'),
    'func_180650_b': ('updateTick', ')V'),
    'func_180645_a': ('randomTick', ')V'),
    'func_180655_c': ('randomDisplayTick', ')V'),
    'func_176196_c': ('canPlaceBlockAt', '(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;)Z'),
    'func_180641_l': ('getComparatorInputOverride', ')I'),
    'func_180663_b': ('breakBlock', '(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;)V'),   # conferido pelos descritores dos jars (BlockContainer/Block)
    'func_176213_c': ('onBlockAdded', '(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;)V'),
    'func_176208_a': ('onBlockHarvested', ')V'),
    'func_180657_a': ('harvestBlock', ')V'),
    'func_176195_g': ('getBlockHardness', ')F'),
    'func_149638_a': ('getExplosionResistance', '(Lnet/minecraft/entity/Entity;)F'),
    'func_149666_a': ('getSubBlocks', '(Lnet/minecraft/creativetab/CreativeTabs;Lnet/minecraft/util/NonNullList;)V'),
    # Item
    'func_150895_a': ('getSubItems', '(Lnet/minecraft/creativetab/CreativeTabs;Lnet/minecraft/util/NonNullList;)V'),
    'func_77659_a': ('onItemRightClick', 'Lnet/minecraft/util/ActionResult;'),
    'func_77624_a': ('addInformation', ')V'),
    'func_180614_a': ('onItemUse', 'Lnet/minecraft/util/EnumActionResult;'),
    'func_77667_c': ('getUnlocalizedName', '(Lnet/minecraft/item/ItemStack;)Ljava/lang/String;'),
    'func_77653_i': ('getItemStackDisplayName', '(Lnet/minecraft/item/ItemStack;)Ljava/lang/String;'),
    # TileEntity / inventario / container / slot / gui
    'func_145839_a': ('readFromNBT', '(Lnet/minecraft/nbt/NBTTagCompound;)V'),
    'func_189515_b': ('writeToNBT', '(Lnet/minecraft/nbt/NBTTagCompound;)Lnet/minecraft/nbt/NBTTagCompound;'),
    'func_73660_a': ('update', '()V'),
    'func_189518_D_': ('getUpdatePacket', ')Lnet/minecraft/network/play/server/SPacketUpdateTileEntity;'),
    'func_189517_E_': ('getUpdateTag', '()Lnet/minecraft/nbt/NBTTagCompound;'),
    'func_145818_k_': ('hasCustomName', '()Z'),
    'func_70005_c_': ('getName', '()Ljava/lang/String;'),
    'func_70302_i_': ('getSizeInventory', '()I'),
    'func_70297_j_': ('getInventoryStackLimit', '()I'),
    'func_191420_l': ('isEmpty', '()Z'),
    'func_94041_b': ('isItemValidForSlot', '(ILnet/minecraft/item/ItemStack;)Z'),
    'func_174889_b': ('openInventory', '(Lnet/minecraft/entity/player/EntityPlayer;)V'),
    'func_174886_c': ('closeInventory', '(Lnet/minecraft/entity/player/EntityPlayer;)V'),
    'func_174888_l': ('clear', '()V'),
    'func_174876_a': ('createContainer', 'Lnet/minecraft/inventory/Container;'),
    'func_82846_b': ('transferStackInSlot', 'Lnet/minecraft/item/ItemStack;'),
    'func_75145_c': ('canInteractWith', '(Lnet/minecraft/entity/player/EntityPlayer;)Z'),
    'func_75134_a': ('onContainerClosed', '(Lnet/minecraft/entity/player/EntityPlayer;)V'),
    'func_75214_a': ('isItemValid', '(Lnet/minecraft/item/ItemStack;)Z'),
    'func_73863_a': ('drawScreen', '(IIF)V'),
    'func_146976_a': ('drawGuiContainerBackgroundLayer', '(FII)V'),
    'func_146979_b': ('drawGuiContainerForegroundLayer', '(II)V'),
    'func_73866_w_': ('initGui', '()V'),
    'func_146284_a': ('actionPerformed', '(Lnet/minecraft/client/gui/GuiButton;)V'),
    'func_192841_a': ('render', '(Lnet/minecraft/tileentity/TileEntity;DDDFIF)V'),
    # bioma / worldgen / enum
    'func_180627_b': ('getGrassColorAtPos', '(Lnet/minecraft/util/math/BlockPos;)I'),
    'func_180625_c': ('getFoliageColorAtPos', '(Lnet/minecraft/util/math/BlockPos;)I'),
    'func_180624_a': ('decorate', '(Lnet/minecraft/world/World;Ljava/util/Random;Lnet/minecraft/util/math/BlockPos;)V'),
    'func_180709_b': ('generate', '(Lnet/minecraft/world/World;Ljava/util/Random;Lnet/minecraft/util/math/BlockPos;)Z'),
    'func_176610_l': ('getName', '()Ljava/lang/String;'),
    # --- chamadas (sem gate: o nome SRG de metodo e unico no jogo todo) ---
    'func_77973_b': ('getItem', ''), 'func_190916_E': ('getCount', ''), 'func_77952_i': ('getItemDamage', ''), 'func_190926_b': ('isEmpty', ''),
    'func_77946_l': ('copy', ''), 'func_190918_g': ('shrink', ''), 'func_190917_f': ('grow', ''), 'func_77960_j': ('getMetadata', ''),
    'func_77978_p': ('getTagCompound', ''), 'func_77982_d': ('setTagCompound', ''), 'func_77942_o': ('hasTagCompound', ''), 'func_77964_b': ('setItemDamage', ''),
    'func_77969_a': ('isItemEqual', ''), 'func_77976_d': ('getMaxStackSize', ''), 'func_82833_r': ('getDisplayName', ''), 'func_190925_c': ('getOrCreateSubCompound', ''),
    'func_77972_a': ('damageItem', ''),
    'func_177230_c': ('getBlock', ''), 'func_177226_a': ('withProperty', ''), 'func_177229_b': ('getValue', ''), 'func_185904_a': ('getMaterial', ''),
    'func_185918_c': ('getCollisionBoundingBox', ''), 'func_185900_c': ('getBoundingBox', ''), 'func_177231_a': ('cycleProperty', ''),
    'func_180495_p': ('getBlockState', ''), 'func_175625_s': ('getTileEntity', ''), 'func_175656_a': ('setBlockState', ''), 'func_180501_a': ('setBlockState', ''),
    'func_175698_g': ('setBlockToAir', ''), 'func_184133_a': ('playSound', ''), 'func_184148_a': ('playSound', ''), 'func_72838_d': ('spawnEntity', ''),
    'func_175667_e': ('isBlockLoaded', ''),
    'func_177958_n': ('getX', ''), 'func_177956_o': ('getY', ''), 'func_177952_p': ('getZ', ''), 'func_177972_a': ('offset', ''), 'func_177977_b': ('down', ''),
    'func_177984_a': ('up', ''), 'func_177982_a': ('add', ''), 'func_177978_c': ('north', ''), 'func_177968_d': ('south', ''), 'func_177974_f': ('east', ''), 'func_177976_e': ('west', ''),
    'func_176734_d': ('getOpposite', ''), 'func_176736_b': ('getHorizontalIndex', ''), 'func_176746_e': ('rotateY', ''), 'func_176735_f': ('rotateYCCW', ''),
    'func_72317_d': ('offset', ''), 'func_72321_a': ('expand', ''),
    'func_176223_P': ('getDefaultState', ''), 'func_149711_c': ('setHardness', ''), 'func_149752_b': ('setResistance', ''), 'func_149715_a': ('setLightLevel', ''),
    'func_149713_g': ('setLightOpacity', ''), 'func_149647_a': ('setCreativeTab', ''), 'func_149663_c': ('setUnlocalizedName', ''), 'func_149672_a': ('setSoundType', ''),
    'func_149675_a': ('setTickRandomly', ''), 'func_149722_s': ('setBlockUnbreakable', ''), 'func_180632_j': ('setDefaultState', ''),
    'func_150898_a': ('getItemFromBlock', ''), 'func_150899_d': ('getItemById', ''), 'func_77655_b': ('setUnlocalizedName', ''), 'func_77637_a': ('setCreativeTab', ''),
    'func_149634_a': ('getBlockFromItem', ''), 'func_77602_a': ('instance', ''), 'func_151395_a': ('getSmeltingResult', ''),
    'func_77625_d': ('setMaxStackSize', ''), 'func_77656_e': ('setMaxDamage', ''),
    'func_74762_e': ('getInteger', ''), 'func_74768_a': ('setInteger', ''), 'func_74782_a': ('setTag', ''), 'func_74775_l': ('getCompoundTag', ''),
    'func_74779_i': ('getString', ''), 'func_74778_a': ('setString', ''), 'func_74767_n': ('getBoolean', ''), 'func_74757_a': ('setBoolean', ''),
    'func_74764_b': ('hasKey', ''), 'func_150297_b': ('hasKey', ''), 'func_74780_a': ('setDouble', ''), 'func_74769_h': ('getDouble', ''),
    'func_74776_a': ('setFloat', ''), 'func_74760_g': ('getFloat', ''), 'func_74777_a': ('setShort', ''), 'func_74765_d': ('getShort', ''),
    'func_74774_a': ('setByte', ''), 'func_74771_c': ('getByte', ''), 'func_74772_a': ('setLong', ''), 'func_74763_f': ('getLong', ''),
    'func_150295_c': ('getTagList', ''), 'func_74742_a': ('appendTag', ''), 'func_74745_c': ('tagCount', ''), 'func_150305_b': ('getCompoundTagAt', ''),
    'func_184586_b': ('getHeldItem', ''), 'func_145747_a': ('sendMessage', ''), 'func_184185_a': ('playSound', ''), 'func_70691_i': ('heal', ''),
    'func_71410_x': ('getMinecraft', ''), 'func_135052_a': ('format', ''), 'func_78276_b': ('drawString', ''), 'func_78256_a': ('getStringWidth', ''),
    'func_110577_a': ('bindTexture', ''), 'func_73729_b': ('drawTexturedModalRect', ''),
    'func_179094_E': ('pushMatrix', ''), 'func_179121_F': ('popMatrix', ''), 'func_179109_b': ('translate', ''), 'func_179114_b': ('rotate', ''),
    'func_179139_a': ('scale', ''), 'func_179124_c': ('color', ''), 'func_179131_c': ('color', ''), 'func_179147_l': ('enableBlend', ''),
    'func_179084_k': ('disableBlend', ''), 'func_179140_f': ('disableLighting', ''), 'func_179145_e': ('enableLighting', ''),
}
# --- campos: srg -> nome ---
FIELDS = {
    'field_72995_K': 'isRemote', 'field_73012_v': 'rand', 'field_73011_w': 'provider', 'field_71071_by': 'inventory', 'field_71075_bZ': 'capabilities',
    'field_71439_g': 'player', 'field_71441_e': 'world', 'field_190927_a': 'EMPTY', 'field_190931_a': 'AIR',
    'field_176523_O': 'BlockDoor.HALF', 'field_176554_a': 'BlockSlab.HALF', 'field_176299_a': 'BlockLog.LOG_AXIS',
    'field_151576_e': 'Material.ROCK', 'field_151575_d': 'Material.WOOD', 'field_151573_f': 'Material.IRON', 'field_151578_c': 'Material.GROUND',
    'field_151577_b': 'Material.GRASS', 'field_151592_s': 'Material.GLASS', 'field_151580_n': 'Material.CLOTH',
    'field_185848_a': 'SoundType.WOOD', 'field_185851_d': 'SoundType.STONE', 'field_150347_e': 'Blocks.COBBLESTONE',
}
# --- itens vanilla (campo de net/minecraft/init/Items -> nome de registro). So os que tenho certeza: drop errado e pior que drop ausente ---
VANILLA_ITEMS = {
    'field_151044_h': 'coal', 'field_151045_i': 'diamond', 'field_151042_j': 'iron_ingot', 'field_151043_k': 'gold_ingot', 'field_151055_y': 'stick',
    'field_151137_ax': 'redstone', 'field_151166_bC': 'emerald', 'field_151114_aO': 'glowstone_dust', 'field_151123_aH': 'slime_ball',
    'field_151128_bU': 'quartz', 'field_151119_aD': 'clay_ball', 'field_151007_F': 'string', 'field_151008_G': 'feather', 'field_151016_H': 'gunpowder',
    'field_151116_aA': 'leather', 'field_151145_ak': 'flint',
}

_csv = None
def _load_csv():
    """db/mcp/methods.csv e fields.csv (searge,name,...): cobre todos os nomes. Sem os arquivos, so a tabela curada vale."""
    global _csv
    if _csv is not None: return _csv
    _csv = {'m': {}, 'f': {}}
    for kind, fn in (('m', 'methods.csv'), ('f', 'fields.csv')):
        p = os.path.join(_MCP, fn)
        if not os.path.exists(p): continue
        try:
            with open(p, encoding='utf-8', errors='replace', newline='') as fh:
                for row in csv.reader(fh):
                    if len(row) >= 2 and row[0].startswith(('func_', 'field_')): _csv[kind][row[0]] = row[1]
        except OSError: pass
    return _csv

def have_mcp():
    c = _load_csv(); return bool(c['m'] or c['f'])

def method_name(srg, desc=None):
    """nome legivel do metodo SRG ou None. Com `desc`, a tabela curada exige que o descritor termine como o esperado."""
    c = _load_csv()
    if srg in c['m']: return c['m'][srg]
    e = METHODS.get(srg)
    if not e: return None
    nm, suf = e
    if desc is not None and suf and not (desc.endswith(suf) or desc == suf or suf in desc): return None
    return nm

def field_name(srg):
    c = _load_csv(); return c['f'].get(srg) or FIELDS.get(srg)

def vanilla_item(srg):
    """campo de Items -> nome de registro (minecraft:<nome>) ou None. Com o csv do MCP, NOME_DO_CAMPO vira nome_do_campo."""
    if srg in VANILLA_ITEMS: return VANILLA_ITEMS[srg]
    n = _load_csv()['f'].get(srg)
    return n.lower() if n and n.upper() == n else None

def readable(key):
    """'net/minecraft/item/ItemStack.func_77973_b' -> 'net/minecraft/item/ItemStack.getItem'; sem nome conhecido devolve a chave."""
    ow, _, nm = key.rpartition('.')
    if nm.startswith('func_'): n = method_name(nm)
    elif nm.startswith('field_'): n = field_name(nm)
    else: return key
    return '%s.%s' % (ow, n) if n else key

def coverage(keys):
    """(com_nome, total_srg) entre as chaves dadas."""
    t = ok = 0
    for k in keys:
        nm = k.rpartition('.')[2]
        if not nm.startswith(('func_', 'field_')): continue
        t += 1; ok += readable(k) != k
    return ok, t
