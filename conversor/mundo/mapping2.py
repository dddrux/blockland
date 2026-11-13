# patchWorld.1: mapeamento 1.12.2 (+mods) -> Blockland atual (pack Default-Java-1.12.2). resolve(nome_registro, meta) -> (id_ou_"block/raw", meta)
import re
from mapping_base import *
import mapping_base as B
XB_CARPET, XB_LADDER, XB_LILY, XB_VINE, XB_GATE, XB_DECOR, XB_PLATE = 991, 994, 1004, 1005, 988, 1006, 980
FACING = {2: 0, 5: 1, 3: 2, 4: 3}   # facing 1.12 (2 N, 3 S, 4 W, 5 E) -> dir do jogo (N0 L1 S2 O3)
HORIZ = {0: 2, 1: 3, 2: 0, 3: 1}    # portao: S0 O1 N2 L3 (1.12) -> meta do jogo
GATES = {'fence_gate': 0, 'spruce_fence_gate': 1, 'birch_fence_gate': 2, 'jungle_fence_gate': 3, 'acacia_fence_gate': 4, 'dark_oak_fence_gate': 5}
def vanilla2(n, d):
    if n == 'ladder': return (XB_LADDER, FACING.get(d & 7, 0))
    if n in ('chest', 'trapped_chest', 'ender_chest'): return (XB_CHEST, FACING.get(d & 7, 0) | ({'chest': 0, 'trapped_chest': 1, 'ender_chest': 2}[n] << 2))
    if n == 'carpet': return (XB_CARPET, d & 15)
    if n == 'vine': return (XB_VINE, (d & 15) or 16)
    if n == 'waterlily': return (XB_LILY, 0)
    if n in GATES: return (XB_GATE, HORIZ[d & 3] | (((d >> 2) & 1) << 2) | (GATES[n] << 3))
    if n == 'web': return (AIR, 0)
    if n == 'cactus': return PA('cactus')
    if n == 'quartz_ore': return PA('quartz_ore', 'netherrack')
    if n == 'snow_layer': return (SNOWBLK, 0) if d >= 4 else (XB_CARPET, 0)   # camada fina de neve = carpete branco (o jogo nao tem camada de neve)
    if n == 'stone_pressure_plate': return (XB_PLATE, 0)
    if n == 'wooden_pressure_plate': return (XB_PLATE, 1)
    if n == 'light_weighted_pressure_plate': return (XB_PLATE, 2)
    if n == 'heavy_weighted_pressure_plate': return (XB_PLATE, 3)
    if n == 'reeds': return (XB_PLANT + 1, 0)
    if n in ('standing_sign', 'wall_sign', 'skull', 'flower_pot', 'cocoa', 'nether_wart', 'standing_banner', 'wall_banner', 'end_rod', 'anvil', 'lever', 'redstone_wire', 'hopper', 'piston', 'sticky_piston', 'piston_head', 'piston_extension', 'unpowered_repeater', 'powered_repeater', 'unpowered_comparator', 'powered_comparator', 'stone_pressure_plate', 'wooden_pressure_plate', 'light_weighted_pressure_plate', 'heavy_weighted_pressure_plate', 'dispenser', 'dropper', 'tripwire', 'tripwire_hook', 'brewing_stand', 'cauldron', 'jukebox', 'noteblock', 'daylight_detector', 'daylight_detector_inverted', 'trapped_chest', 'end_portal', 'end_portal_frame', 'end_gateway', 'dragon_egg', 'beacon', 'structure_block', 'enchanting_table', 'mob_spawner', 'command_block', 'repeating_command_block', 'chain_command_block', 'melon_stem', 'pumpkin_stem', 'sapling', 'tallgrass', 'yellow_flower', 'red_flower', 'double_plant', 'deadbush', 'brown_mushroom', 'red_mushroom', 'web', 'bed', 'fire', 'torch', 'redstone_torch', 'unlit_redstone_torch'):
        pass
    if n == 'enchanting_table': return PA('enchanting_table_top', 'obsidian')
    if n == 'beacon': return P('glass')
    if n == 'slime': return P('slime')
    if n == 'cobblestone_wall': return (XB_WALL, d << 4)
    if n == 'noteblock': return P('noteblock')
    if n == 'jukebox': return P('jukebox_top') if has('jukebox_top') else (PLANKS, 0)
    if n == 'dragon_egg': return (OBSIDIAN, 0)
    if n in ('end_portal_frame',): return PA('end_stone')
    if n == 'grass_path': return PA('grass_path', 'dirt')
    if n == 'mob_spawner': return (COBBLE, 0)
    if n in ('command_block', 'repeating_command_block', 'chain_command_block'): return (STONEBRICK, 0)
    if n == 'lit_pumpkin': return PA('pumpkin_jack', 'pumpkin')
    if n == 'pumpkin': return P('pumpkin') if has('pumpkin') else P('pumpkin_top')
    if n == 'lit_furnace': return P('furnace')
    if n in ('piston', 'sticky_piston'): return PA('piston_top_normal', 'piston')
    if n in ('dispenser', 'dropper'): return PA(n)
    if n == 'hopper': return (IRONBLK, 0)
    if n == 'cauldron': return (XB_DECOR, 0)
    return None
def vanilla_all(n, d):
    r = vanilla2(n, d)
    if r is not None: return r
    return vanilla(n, d)
# ---- mods: aproximacao por familia (o jogo so tem o pack 1.12.2 + os mods da pasta mods/); escolhe o 1o nome que o pack tem ----
FAM = [   # (substring do nome, candidatos de textura em ordem)
    ('speleothem', None), ('marble', ('stone_diorite_smooth', 'quartz_block', 'diorite')), ('limestone', ('end_stone', 'sandstone', 'stone_andesite')),
    ('slate', ('hardened_clay_stained_gray', 'stone_andesite_smooth', 'cobblestone')), ('basalt', ('concrete_gray', 'hardened_clay_stained_black', 'cobblestone')),
    ('jasper', ('hardened_clay_stained_orange', 'hardened_clay')), ('biome_cobblestone', ('cobblestone',)), ('flesh', ('nether_wart_block',)),
    ('ash_block', ('concrete_powder_gray',)), ('mud', ('dirt_coarse', 'coarse_dirt')), ('hive', ('hardened_clay_stained_yellow',)), ('honey', ('hardened_clay_stained_yellow',)),
    ('glowcelium', ('mycelium_top', 'mycelium')), ('elder_prismarine', ('prismarine_rough', 'prismarine')), ('pavement', ('stonebrick', 'stone_andesite_smooth')),
    ('antiblock', ('stone_andesite_smooth', 'quartz_block')), ('brownstone', ('hardened_clay_stained_brown',)), ('andesite', ('stone_andesite',)), ('diorite', ('stone_diorite',)), ('granite', ('stone_granite',)),
    ('crystal', ('glass',)), ('gem_ore', ('diamond_ore',)), ('prismarine', ('prismarine_rough',)),
]
def mod(n, d):
    ns, s = n.split(':', 1)
    if 'light_source' in s or 'blockfake' in s or 'invis' in s or ns in ('light_blocks', 'invisiblights') or s in ('blood', 'poison'): return (AIR, 0)
    if s == 'resource' and ns == 'ic2': return PA(*(['iron_ore', 'iron_ore', 'iron_ore', 'emerald_ore'][d] if d < 4 else 'stone',))
    if s == 'basicore': return P('iron_ore')
    if s == 'dirt' and ns == 'biomesoplenty': return (DIRT, 0) if d == 0 else P('coarse_dirt') if d == 1 else PA('dirt_podzol', 'coarse_dirt')
    if s == 'grass' and ns == 'biomesoplenty': return (GRASS, 0)
    if 'leaves' in s: return (LEAVES, 0) if ns != 'biomesoplenty' else P(['leaves_oak', 'leaves_birch', 'leaves_jungle', 'leaves_spruce', 'leaves_acacia', 'leaves_big_oak'][(sum(map(ord, s)) + d) % 6])
    if 'rubber_wood' in s or re.match(r'(log|rustic:log)', s) or s.startswith('log'): return P(['log_big_oak', 'log_jungle', 'log_birch', 'log_spruce', 'log_acacia'][d & 3 if (d & 3) < 5 else 0], axis(d))
    if 'planks' in s: return (PLANKS, 0)
    for k, c in FAM:
        if k in s:
            if c is None:
                for k2, v in (('netherrack', ('netherrack',)), ('marble', ('stone_diorite_smooth',)), ('limestone', ('stone_andesite',)), ('basalt', ('concrete_gray',)), ('granite', ('stone_granite',)), ('diorite', ('stone_diorite',)), ('andesite', ('stone_andesite',)), ('slate', ('hardened_clay_stained_gray',))):
                    if k2 in s: return PA(*v)
                return (STONE, 0)
            return PA(*c)
    if s == 'waterlily' or 'waterlily' in s: return (XB_LILY, 0)
    if 'fire' in s: return (XB_FIRE, 0)
    if any(k in s for k in ('thistle', 'lily', 'mushroom', 'shroom', 'bush', 'cloudsbluff', 'herb', 'wildberry', 'cohosh')): return (XB_PLANT + 3 + (sum(map(ord, s)) % 9), 0)
    if any(k in s for k in ('nethermetal', 'metal', 'factory', 'technical', 'futura')): return (IRONBLK, 0)
    if s in ('plant_0', 'plant_1', 'bramble_plant', 'roots', 'ivy', 'willow_vine', 'seaweed', 'double_plant', 'horsetail', 'cohosh', 'ginseng', 'chamomile', 'aloe_vera', 'mushroom', 'coral') or 'roots' in s or 'plant' in s:
        if 'flower' in s or 'lily' in s or 'thistle' in s or 'chamomile' in s: return (XB_PLANT + 3 + (d % 9), 0)
        if s in ('plant_1', 'ivy', 'willow_vine', 'roots', 'seaweed', 'coral'): return (XB_PLANT + 1, 0)
        return (XB_PLANT, 0)
    if s.startswith('flower') or 'flower' in s: return (XB_PLANT + 2 + (d % 12), 0)
    if 'ore' in s: return P('iron_ore')
    if 'glass' in s: return P('glass')
    if 'pane' in s or 'bars' in s: return (XB_PANE, 1 << 4)
    if 'stairs' in s: return (XB_STAIR, 10 << 3)
    if 'slab' in s: return (XB_SLAB, 10 << 2)
    if 'wall' in s or 'fence' in s: return (XB_WALL, 0)
    if any(k in s for k in ('lamp', 'light', 'lantern', 'neon', 'lit')): return (GLOWSTONE, 0)
    if 'concrete' in s: return P('concrete_' + COL[d & 15 if s.endswith('concrete') else 7])
    if 'sand' in s: return (SAND, 0)
    if 'smoker' in s or 'furnace' in s: return P('furnace')
    if 'brick' in s: return (STONEBRICK, 0)
    if 'iron' in s: return (IRONBLK, 0)
    if 'coal' in s: return (COALBLK, 0)
    if 'quartz' in s: return P('quartz_block')
    if 'cobblestone' in s: return (COBBLE, 0)
    return (STONE, 0)
def resolve(name, d):
    n = name[10:] if name.startswith('minecraft:') else name
    if ':' in n and not name.startswith('minecraft:'): return mod(n, d)
    r = vanilla_all(n, d)
    return r if r is not None else (STONE, 0)
