# Mapeamento 1.12.2 (+mods) -> Blockland. Retorna (id|"block/nome", meta)
import re, zipfile, io
from PIL import Image
AIR,GRASS,DIRT,STONE,SAND,LOG,LEAVES,PLANKS,COBBLE,WATER,GRAVEL,BRICKS,BEDROCK,COALORE,DIAMONDORE=range(15)
(GOLDORE,IRONORE,GOLDBLK,IRONBLK,DIAMBLK,COALBLK,SANDSTONE,STONEBRICK,MOSSY,CLAY,SNOWBLK,OBSIDIAN,GLOWSTONE,BOOKSHELF,WOOL,REDSTONEORE,LAPISORE)=range(15,32)
XB_TORCH,XB_PLANT,XB_SAPLING,XB_RAIL=900,940,964,970
XB_TRAP,XB_DOOR,XB_BTN,XB_BED=987,989,981,998
XB_CHEST,XB_FENCE,XB_PANE,XB_WALL,XB_STAIR,XB_SLAB,XB_CROP,XB_FARM,XB_FIRE,XB_PORTAL=990,992,993,995,1000,1001,1002,1003,1014,1015
COL=['white','orange','magenta','light_blue','yellow','lime','pink','gray','light_gray','cyan','purple','blue','brown','green','red','black']

# patchWorld.1: nomes de textura = RAW do pack Default-Java-1.12.2 (o jogo guarda PK[].file cru, ex. "block/planks_acacia"); cubo = mesma regra do jogo (pkIsCube + X_top/X_side)
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from pk112py import pk112
PACK = os.environ.get('BL_PACK', os.path.join(os.path.dirname(__file__), '..', '..', 'assets', 'InventivetalentDev_minecraft-assets_1_12_2_textures.zip'))
_z = zipfile.ZipFile(PACK); VALID = set(); _all = {}; REV = {}
for n in _z.namelist():
    if (n.startswith('textures/blocks/') or n.startswith('blocks/') or n.startswith('assets/minecraft/textures/blocks/')) and n.endswith('.png'):
        b = n.split('/')[-1][:-4]
        im = Image.open(io.BytesIO(_z.read(n))).convert('RGBA'); w = im.size[0]; im = im.crop((0, 0, w, w))
        _all[b] = (im.getchannel('A').getextrema()[0], w)
for b, (a_, w) in _all.items():
    REV.setdefault(pk112(b), b)
    if b[-1].isdigit(): continue
    c12 = pk112(b)
    if a_ == 255 or c12.endswith('_leaves') or ('glass' in b and 'pane' not in b and 'top' not in b) or b in ('ice', 'slime'):
        if not b.endswith(('_top', '_bottom', '_side', '_front', '_back', '_end', '_inner', '_outer', '_overlay', '_particle', '_stage', '_side1', '_side2')): VALID.add(b)
for b in list(_all):
    if b.endswith('_top') and b[:-4] + '_side' in _all and b[:-4] not in _all: VALID.add(b[:-4])
for b in ('furnace', 'dispenser', 'dropper'): pass
CORE_N={'stone':STONE,'dirt':DIRT,'cobblestone':COBBLE,'iron_ore':IRONORE,'coal_ore':COALORE,'gold_ore':GOLDORE,'diamond_ore':DIAMONDORE,'redstone_ore':REDSTONEORE,'lapis_ore':LAPISORE,'sand':SAND,'gravel':GRAVEL,'bedrock':BEDROCK,'obsidian':OBSIDIAN,'glowstone':GLOWSTONE,'clay':CLAY,'bricks':BRICKS,'oak_planks':PLANKS,'stone_bricks':STONEBRICK,'mossy_cobblestone':MOSSY,'bookshelf':BOOKSHELF,'white_wool':WOOL,'iron_block':IRONBLK,'gold_block':GOLDBLK,'diamond_block':DIAMBLK,'coal_block':COALBLK,'snow':SNOWBLK,'sandstone':SANDSTONE,'oak_leaves':LEAVES,'oak_log':LOG,'enchanting_table':OBSIDIAN}
REV.setdefault('podzol','dirt_podzol'); REV.setdefault('grass_path','grass_path'); REV.setdefault('coarse_dirt','coarse_dirt')
def has(n): r = REV.get(n, n); return r in VALID
def P(n, m=0):
    if n in CORE_N: return (CORE_N[n], 0 if n != 'oak_log' else m)
    for c in (n, n + '_block'):
        r = REV.get(c, c)
        if r in VALID: return ('block/' + r, m)
    return (STONE, 0)
def PA(*names):   # primeiro nome que o pack tem (aproximacao fiel quando o bloco nao existe no 1.12.2)
    for n in names:
        if n in CORE_N or has(n) or has(n + '_block'): return P(n)
    return (STONE, 0)
def axis(d): return {0:0,4:1,8:2}.get(d&12,0)
def leaf(n): return (LEAVES,0) if n=='oak_leaves' else P(n)
SPECIES=['oak','spruce','birch','jungle','acacia','dark_oak']
STONES=[(STONE,0),P('granite'),P('polished_granite'),P('diorite'),P('polished_diorite'),P('andesite'),P('polished_andesite')]
def vanilla(n,d):
    if n in('air','structure_void','barrier'):return(AIR,0)
    if n=='stone':return STONES[d] if d<7 else (STONE,0)
    if n=='grass':return(GRASS,0)
    if n=='dirt':return[(DIRT,0),P('coarse_dirt'),P('podzol')][min(d,2)]
    if n=='cobblestone':return(COBBLE,0)
    if n=='planks':return(PLANKS,0)if d==0 else P(SPECIES[min(d,5)]+'_planks')
    if n=='sapling':return(XB_SAPLING+min(d&7,5),0)
    if n=='bedrock':return(BEDROCK,0)
    if n in('water',):return(WATER,0)
    if n=='flowing_water':return(WATER,0)if d==0 else((40,0)if d>=8 else(48-d,0))
    if n in('lava','flowing_lava'):return(48,0)if d==0 else((49,0)if d>=8 else(57-d,0))
    if n=='sand':return(SAND,0)if d==0 else P('red_sand')
    if n=='gravel':return(GRAVEL,0)
    for k,v in(('gold_ore',GOLDORE),('iron_ore',IRONORE),('coal_ore',COALORE),('lapis_ore',LAPISORE),('diamond_ore',DIAMONDORE),('redstone_ore',REDSTONEORE),('lit_redstone_ore',REDSTONEORE)):
        if n==k:return(v,0)
    if n=='emerald_ore':return P('emerald_ore')
    if n=='quartz_ore':return P('nether_quartz_ore')
    if n=='log':return(LOG,axis(d))if d&3==0 else P(SPECIES[d&3]+'_log',axis(d))
    if n=='log2':return P(SPECIES[4+(d&1)]+'_log',axis(d))
    if n=='leaves':return leaf(['oak','spruce','birch','jungle'][d&3]+'_leaves')
    if n=='leaves2':return leaf(['acacia','dark_oak'][d&1]+'_leaves')
    if n=='sponge':return P('wet_sponge'if d else'sponge')
    if n=='glass':return P('glass')
    if n=='lapis_block':return P('lapis_block')
    if n=='sandstone':return(SANDSTONE,0)if d==0 else P(['','chiseled_sandstone','cut_sandstone'][min(d,2)])
    if n=='red_sandstone':return P(['red_sandstone','chiseled_red_sandstone','cut_red_sandstone'][min(d,2)])
    if n=='wool':return(WOOL,0)if d==0 else P(COL[d]+'_wool')
    if n=='gold_block':return(GOLDBLK,0)
    if n=='iron_block':return(IRONBLK,0)
    if n=='brick_block':return(BRICKS,0)
    if n=='tnt':return P('tnt')
    if n=='bookshelf':return(BOOKSHELF,0)
    if n=='mossy_cobblestone':return(MOSSY,0)
    if n=='obsidian':return(OBSIDIAN,0)
    if n=='diamond_block':return(DIAMBLK,0)
    if n=='coal_block':return(COALBLK,0)
    if n=='crafting_table':return P('crafting_table')
    if n=='furnace' or n=='lit_furnace':return P('furnace')
    if n=='farmland':return(XB_FARM,0)
    if n in('wheat','carrots','potatoes'):return(XB_CROP,({'wheat':0,'carrots':1,'potatoes':2}[n]<<3)|min(d,7))
    if n=='beetroots':return(XB_CROP,(3<<3)|(d*7//3))
    if n=='ladder':return(AIR,0)
    if n=='rail':return(XB_RAIL,d&15)
    if n in('golden_rail','detector_rail','activator_rail'):return(XB_RAIL+{'golden_rail':1,'detector_rail':2,'activator_rail':3}[n],d&7)
    if n=='torch' or n=='redstone_torch' or n=='unlit_redstone_torch':
        k={1:0,2:2,3:1,4:3,5:4}.get(d,4);return(XB_TORCH+k,0)
    if n=='snow_layer':return(SNOWBLK,0)if d>=4 else(AIR,0)
    if n=='snow':return(SNOWBLK,0)
    if n=='ice' or n=='frosted_ice':return P('ice')
    if n=='packed_ice':return P('packed_ice')
    if n=='clay':return(CLAY,0)
    if n=='fence':return(XB_FENCE,0)
    if n.endswith('_fence') and n!='nether_brick_fence':return(XB_FENCE,SPECIES.index(n[:-6])<<4 if n[:-6] in SPECIES else 0)
    if n=='nether_brick_fence':return(XB_FENCE,6<<4)
    if n=='cobblestone_wall':return(XB_WALL,d<<4)
    if n=='pumpkin' or n=='lit_pumpkin':return P('pumpkin')
    if n=='netherrack':return P('netherrack')
    if n=='soul_sand':return P('soul_sand')
    if n=='glowstone':return(GLOWSTONE,0)
    if n=='portal':return(XB_PORTAL,0 if d!=2 else 1)
    if n=='stonebrick':return(STONEBRICK,0)if d==0 else P(['','mossy_stone_bricks','cracked_stone_bricks','chiseled_stone_bricks'][d&3])
    if n=='monster_egg':return[(STONE,0),(COBBLE,0),(STONEBRICK,0),P('mossy_stone_bricks'),P('cracked_stone_bricks'),P('chiseled_stone_bricks')][min(d,5)]
    if n=='red_mushroom_block':return P('mushroom_stem'if d in(10,15)else'mushroom_stem'if d==0 else'red_mushroom_block')
    if n=='brown_mushroom_block':return P('mushroom_stem'if d in(10,15)else'mushroom_stem'if d==0 else'brown_mushroom_block')
    if n=='iron_bars':return(XB_PANE,0)
    if n=='glass_pane':return(XB_PANE,1<<4)
    if n=='stained_glass':return P(COL[d]+'_stained_glass')
    if n=='stained_glass_pane':return(XB_PANE,1<<4)
    if n=='melon_block':return P('melon')
    if n=='mycelium':return P('mycelium')
    if n=='nether_brick':return P('nether_bricks')
    if n=='red_nether_brick':return P('red_nether_bricks')
    if n=='end_stone':return P('end_stone')
    if n=='redstone_lamp' or n=='lit_redstone_lamp':return P('redstone_lamp_on'if n[0]=='l'else'redstone_lamp')
    if n=='emerald_block':return P('emerald_block')
    if n=='beacon':return P('glass')
    if n=='quartz_block':return P(['quartz_block','chiseled_quartz_block','quartz_pillar','quartz_pillar','quartz_pillar'][min(d,4)],{3:1,4:2}.get(d,0))
    if n=='hardened_clay':return P('terracotta')
    if n=='stained_hardened_clay':return P(COL[d]+'_terracotta')
    if n.endswith('_glazed_terracotta'):return P(n)
    if n=='concrete':return P(COL[d]+'_concrete')
    if n=='concrete_powder':return P(COL[d]+'_concrete_powder')
    if n=='coal_block':return(COALBLK,0)
    if n=='hay_block':return P('hay_block',axis(d))
    if n=='packed_ice':return P('packed_ice')
    if n=='magma':return P('magma')
    if n=='nether_wart_block':return P('nether_wart_block')
    if n=='bone_block':return P('bone_block',axis(d))
    if n=='sea_lantern':return P('sea_lantern')
    if n=='prismarine':return P(['prismarine','prismarine_bricks','dark_prismarine'][min(d,2)])
    if n=='purpur_block':return P('purpur_block')
    if n=='purpur_pillar':return P('purpur_pillar',axis(d))
    if n=='end_bricks':return P('end_stone_bricks')
    if n=='slime':return(STONE,0)if 'slime_block' not in _all else P('slime_block')
    if n=='redstone_block':return P('redstone_block')
    if n=='observer':return (COBBLE,0)
    if n=='grass_path':return(DIRT,0)
    if n in('mob_spawner','command_block','repeating_command_block','chain_command_block'):return(COBBLE,0)if n=='mob_spawner' else (STONEBRICK,0)
    if n=='enchanting_table':return (OBSIDIAN,0)
    if n in('chest','trapped_chest','ender_chest'):return(XB_CHEST,2)
    if n=='fire':return(XB_FIRE,0)
    if n=='tallgrass':return(XB_PLANT+(1 if d==2 else 0),0)
    if n=='deadbush':return(XB_PLANT,0)
    if n=='yellow_flower':return(XB_PLANT+2,0)
    if n=='red_flower':return(XB_PLANT+3+min(d,8),0)
    if n=='red_mushroom':return(XB_PLANT+3,0)
    if n=='brown_mushroom':return(XB_PLANT+13,0)
    if n=='double_plant':
        t={0:18,1:20,2:14,3:16,4:22,5:22}.get(d&7,14);return(XB_PLANT+t+(1 if d&8 else 0),0)
    if n=='stone_slab':return(XB_SLAB,(1 if d&8 else 0)|([10,7,0,1,9,8,8,10][d&7]<<2))
    if n=='wooden_slab':return(XB_SLAB,(1 if d&8 else 0)|([0,2,3,4,5,6][min(d&7,5)]<<2))
    if n.endswith('_stairs') or n.endswith('_slab'):
        base=n.rsplit('_',1)[0]
        mat={'oak':0,'wooden':0,'stone_brick':8,'brick':9,'stone':10,'cobblestone':1,'spruce':2,'birch':3,'jungle':4,'acacia':5,'dark_oak':6,'sandstone':7,'red_sandstone':7,'nether_brick':8,'quartz':10,'purpur':10,'smooth':10}.get(base,10)
        if n.endswith('_stairs'): return(XB_STAIR,{0:1,1:3,2:2,3:0}[d&3]|((d>>2&1)<<2)|(mat<<3))
        return(XB_SLAB,(1 if d&8 else 0)|(mat<<2))
    if n in('double_stone_slab','double_wooden_slab'):return(XB_SLAB,2|((0 if n=='double_wooden_slab' and d==0 else 10)<<2))
    if n=='double_stone_slab2':return(XB_SLAB,2|(7<<2))
    if n.endswith('_door') or n=='wooden_door':   # patchMap.1: porta = dir | aberta<<2 | cima<<3 | tipo<<5 (a metade de cima herda o d de baixo; veja convert.py)
        ty={'iron_door':0,'wooden_door':1,'spruce_door':2,'birch_door':3,'jungle_door':4,'acacia_door':5,'dark_oak_door':6}.get(n,1)
        return(XB_DOOR,((d+3)&3)|(((d>>2)&1)<<2)|(((d>>3)&1)<<3)|(ty<<5))
    if n in('trapdoor','iron_trapdoor'):   # dir (aberto) | topo<<2 | aberto<<3 | tipo<<4
        return(XB_TRAP,{1:0,2:1,0:2,3:3}[d&3]|(((d>>3)&1)<<2)|(((d>>2)&1)<<3)|((0 if n=='iron_trapdoor' else 1)<<4))
    if n=='bed':   # dir(compasso N,L,S,O) | pes/cabeca<<2 | cor<<3 | 128; cor = branca (a cor mora no tile entity)
        return(XB_BED,{0:2,1:3,2:0,3:1}[d&3]|(((d>>3)&1)<<2)|128)
    if n in('stone_button','wooden_button'):   # face: 0 chao, 1 teto, 2+d lado (N,L,S,O); material no bit 3
        f={5:0,0:1,1:3,2:5,3:4,4:2}.get(d&7,0);return(XB_BTN,f|((1 if n=='wooden_button' else 0)<<3))
    if n=='web':return('block/cobweb',0)
    if n in('skull','banner','standing_banner','wall_banner','flower_pot','cocoa','nether_wart','reeds','waterlily','cactus','web','anvil','end_rod','piston_head','sticky_piston','piston','lever','wooden_button','stone_button','wooden_pressure_plate','stone_pressure_plate','fence_gate','standing_sign','wall_sign','vine','end_portal','end_portal_frame','dragon_egg','cake','tripwire','tripwire_hook','redstone_wire','powered_repeater','unpowered_repeater','daylight_detector','hopper','dispenser','dropper','brewing_stand','cauldron','jukebox','noteblock','carpet','end_gateway'):
        return(AIR,0)
    return None
