#!/usr/bin/env python3
# Gera assets/creative/mc-1.12.2.cfg: abas do criativo na ordem do Minecraft 1.12.2.
# Linha = "textura[,alternativa]|Nome opcional". Com nome => item novo (so inventario) que reusa a textura (escadas, lajes, pocoes...).
# Uso: python3 tools/gen_creative_1122.py [assets/resourcepacks/Ashen_16x.zip]   (valida as texturas contra o pack)
import sys, zipfile, os
C16 = "white orange magenta light_blue yellow lime pink gray light_gray cyan purple blue brown green red black".split()
W6 = "oak spruce birch jungle acacia dark_oak".split()
def T(s): return " ".join(w.capitalize() for w in s.split("_"))
tabs = {}; cur = None
def tab(n): 
    global cur; cur = n; tabs[n] = []
def b(*names):
    for n in names: tabs[cur].append(n)
def blk(*n): b(*["block/" + x for x in n])
def itm(*n): b(*["item/" + x for x in n])
def al(tex, name): tabs[cur].append(f"{tex}|{name}")
def stairs(tex, nm): al(tex, nm + " Stairs")
def slab(tex, nm): al(tex, nm + " Slab")

tab("BUILD")
blk("stone","granite","polished_granite","diorite","polished_diorite","andesite","polished_andesite","grass_block_top","dirt","coarse_dirt","podzol")
blk("cobblestone"); blk(*[w+"_planks" for w in W6]); blk("bedrock","sand","red_sand","gravel","gold_ore","iron_ore","coal_ore")
blk(*[w+"_log" for w in W6]); blk("sponge","wet_sponge","glass","lapis_ore","lapis_block","sandstone","chiseled_sandstone","cut_sandstone")
blk(*[c+"_wool" for c in C16]); blk("gold_block","iron_block")
for tex,nm in [("smooth_stone","Stone"),("sandstone","Sandstone"),("cobblestone","Cobblestone"),("bricks","Bricks"),("stone_bricks","Stone Brick"),("nether_bricks","Nether Brick"),("quartz_block_side","Quartz")]: slab("block/"+tex, nm)
blk("bricks","bookshelf","mossy_cobblestone","obsidian"); stairs("block/oak_planks","Oak Wood")
blk("diamond_ore","diamond_block"); stairs("block/cobblestone","Cobblestone"); blk("redstone_ore","ice","snow","clay","netherrack","soul_sand","glowstone")
blk("stone_bricks","mossy_stone_bricks","cracked_stone_bricks","chiseled_stone_bricks"); stairs("block/bricks","Brick"); stairs("block/stone_bricks","Stone Brick")
blk("mycelium_top","nether_bricks"); stairs("block/nether_bricks","Nether Brick"); blk("end_stone")
for w in W6: slab("block/"+w+"_planks", T(w)+" Wood")
stairs("block/sandstone","Sandstone"); blk("emerald_ore","emerald_block")
for w in W6[1:]: stairs("block/"+w+"_planks", T(w)+" Wood")
blk("nether_quartz_ore","quartz_block_side","chiseled_quartz_block","quartz_pillar"); stairs("block/quartz_block_side","Quartz")
blk(*[c+"_terracotta" for c in C16]); blk("prismarine","prismarine_bricks","dark_prismarine","sea_lantern","hay_block_side","terracotta","coal_block","packed_ice")
blk("red_sandstone","chiseled_red_sandstone","cut_red_sandstone"); stairs("block/red_sandstone","Red Sandstone"); slab("block/red_sandstone","Red Sandstone")
blk("purpur_block","purpur_pillar"); stairs("block/purpur_block","Purpur"); slab("block/purpur_block","Purpur")
blk("end_stone_bricks","magma","nether_wart_block","red_nether_bricks","bone_block_side")
blk(*[c+"_glazed_terracotta" for c in C16]); blk(*[c+"_concrete" for c in C16]); blk(*[c+"_concrete_powder" for c in C16])

tab("DECO")
blk(*[w+"_leaves" for w in W6]); blk("cobweb"); b("block/short_grass,block/grass","block/fern","block/dead_bush")
blk(*[w+"_sapling" for w in W6]); blk("dandelion","poppy","blue_orchid","allium","azure_bluet","red_tulip","orange_tulip","white_tulip","pink_tulip","oxeye_daisy","brown_mushroom","red_mushroom")
blk("torch"); al("block/oak_planks","Chest"); b("block/crafting_table_front"); al("block/furnace_front","Furnace"); itm("oak_sign"); b("block/ladder"); al("block/snow","Snow")
blk("cactus_side","jukebox_side"); al("block/oak_planks","Oak Fence"); blk("carved_pumpkin"); al("block/pumpkin_side","Pumpkin"); blk("jack_o_lantern","iron_bars","glass_pane_top","melon_side","vine","lily_pad")
al("block/nether_bricks","Nether Brick Fence"); al("block/enchanting_table_side","Enchantment Table"); blk("end_portal_frame_side","end_stone")
al("block/cobblestone","Cobblestone Wall"); al("block/mossy_cobblestone","Mossy Cobblestone Wall"); al("block/anvil_top","Anvil"); al("block/chipped_anvil_top","Slightly Damaged Anvil"); al("block/damaged_anvil_top","Very Damaged Anvil")
for w in W6[1:]: al("block/"+w+"_planks", T(w)+" Fence")
for c in C16: al("block/"+c+"_wool", T(c)+" Carpet")
blk("sunflower_front","lilac_top","tall_grass_top","large_fern_top","rose_bush_top","peony_top"); blk(*[c+"_stained_glass" for c in C16]); blk(*[c+"_stained_glass_pane_top" for c in C16])
blk("end_rod"); al("block/obsidian","Ender Chest"); al("block/beacon_cage","Beacon"); blk("chorus_plant","chorus_flower"); itm("painting","item_frame","flower_pot","armor_stand"); [al("block/"+c+"_wool", T(c)+" Bed") for c in C16]; [al("block/"+c+"_wool", T(c)+" Banner") for c in C16]
blk("slime_block")
for c in C16: al("block/"+c+"_wool", T(c)+" Shulker Box")

tab("REDSTONE")
blk("dispenser_front","note_block"); al("block/piston_top","Sticky Piston"); blk("piston_top","tnt_side"); al("block/lever","Lever"); al("block/stone","Stone Pressure Plate"); al("block/oak_planks","Oak Pressure Plate")
al("block/gold_block","Light Weighted Pressure Plate"); al("block/iron_block","Heavy Weighted Pressure Plate"); blk("redstone_torch"); al("block/stone","Stone Button"); al("block/oak_planks","Oak Button")
blk("redstone_lamp","redstone_block","tripwire_hook"); al("block/oak_planks","Trapped Chest"); blk("daylight_detector_top","hopper_outside","dropper_front","observer_front"); itm("redstone")
al("block/iron_trapdoor_face","Iron Trapdoor")
for w in W6: al("block/"+w+"_trapdoor,block/"+w+"_planks", T(w)+" Wood Trapdoor")
for w in W6: al("block/"+w+"_planks", T(w)+" Fence Gate")
itm("repeater","comparator","iron_door"); itm(*[w+"_door" for w in W6])

tab("TRANSPORT")
blk("powered_rail","detector_rail","rail","activator_rail"); itm("saddle","minecart","chest_minecart","furnace_minecart","tnt_minecart","hopper_minecart","carrot_on_a_stick")
itm("iron_horse_armor","golden_horse_armor","diamond_horse_armor","elytra"); itm(*[w+"_boat" for w in W6])

tab("MISC")
itm("bucket","water_bucket","lava_bucket","snowball","milk_bucket","ender_pearl","ender_eye","experience_bottle","fire_charge","writable_book","name_tag","lead","firework_rocket","map")
itm("music_disc_13","music_disc_cat","music_disc_blocks","music_disc_chirp","music_disc_far","music_disc_mall","music_disc_mellohi","music_disc_stal","music_disc_strad","music_disc_ward","music_disc_11","music_disc_wait")
EGGS = "bat blaze cave_spider chicken cow creeper donkey elder_guardian enderman endermite evoker ghast guardian horse husk llama magma_cube mooshroom mule ocelot parrot pig polar_bear rabbit sheep shulker silverfish skeleton skeleton_horse slime spider squid stray vex villager vindicator witch wither_skeleton wolf zombie zombie_horse zombified_piglin zombie_villager".split()
for e in EGGS: b("item/%s_spawn_egg" % e + (",item/zombie_pigman_spawn_egg" if e == "zombified_piglin" else ""))

tab("FOOD")
itm("apple","mushroom_stew","bread","porkchop","cooked_porkchop","golden_apple","enchanted_golden_apple","cod","salmon","tropical_fish","pufferfish","cooked_cod","cooked_salmon")
b("item/cake"); itm("cookie","melon_slice","beef","cooked_beef","chicken","cooked_chicken","rotten_flesh","spider_eye","carrot","potato","baked_potato","poisonous_potato","golden_carrot","pumpkin_pie","rabbit","cooked_rabbit","rabbit_stew","mutton","cooked_mutton","chorus_fruit","beetroot","beetroot_soup")

tab("TOOLS")
MT = "wooden stone iron diamond golden".split()
for k in ["iron_shovel","iron_pickaxe","iron_axe","flint_and_steel"]: itm(k)
for m in ["wooden","stone"]: itm(m+"_shovel",m+"_pickaxe",m+"_axe")
for m in ["diamond","golden"]: itm(m+"_shovel",m+"_pickaxe",m+"_axe")
for m in ["wooden","stone","iron","diamond","golden"]: itm(m+"_hoe")
al("item/compass_00","Compass"); itm("fishing_rod"); al("item/clock_00","Clock"); itm("shears")

tab("COMBAT")
itm("bow","arrow","iron_sword","wooden_sword","stone_sword","diamond_sword","golden_sword")
for m in ["leather","chainmail","iron","diamond","golden"]: itm(*[m+"_"+p for p in ["helmet","chestplate","leggings","boots"]])
itm("spectral_arrow","totem_of_undying")

tab("BREW")
itm("ghast_tear","nether_wart","fermented_spider_eye","blaze_powder","magma_cream","brewing_stand","glistering_melon_slice","rabbit_foot","dragon_breath","glass_bottle"); b("item/cauldron")
EFF = ["Night Vision","Leaping","Fire Resistance","Swiftness","Slowness","Water Breathing","Healing","Harming","Poison","Regeneration","Strength","Weakness","Luck"]
al("item/potion","Water Bottle")
for pre,tx in [("Potion of ","potion"),("Splash Potion of ","splash_potion"),("Lingering Potion of ","lingering_potion")]:
    for e in EFF: al("item/"+tx, pre+e)

tab("MATERIALS")
itm("coal","charcoal","diamond","iron_ingot","gold_ingot","stick","bowl","string","feather","gunpowder","wheat_seeds","wheat","flint","brick","clay_ball","sugar_cane","paper","book","slime_ball","egg","glowstone_dust")
itm("ink_sac","red_dye","green_dye","cocoa_beans","lapis_lazuli","purple_dye","cyan_dye","light_gray_dye","gray_dye","pink_dye","lime_dye","yellow_dye","light_blue_dye","magenta_dye","orange_dye","bone_meal")
itm("bone","sugar","leather","blaze_rod","gold_nugget","emerald","nether_star","nether_brick","quartz","prismarine_shard","prismarine_crystals","rabbit_hide","iron_nugget","shulker_shell","melon_seeds","pumpkin_seeds","beetroot_seeds","popped_chorus_fruit")

ICON = dict(BUILD="block/bricks",DECO="block/oak_leaves",REDSTONE="item/redstone",TRANSPORT="block/rail",MISC="item/lava_bucket",FOOD="item/apple",TOOLS="item/iron_axe,item/iron_pickaxe",COMBAT="item/golden_sword",BREW="item/potion",MATERIALS="item/stick")
zp = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "../assets/resourcepacks/Ashen_16x.zip")
have = set()
if os.path.exists(zp):
    for n in zipfile.ZipFile(zp).namelist():
        if "/textures/" in n and n.endswith(".png"): have.add(n.split("/textures/")[1][:-4])
out = ["# Abas do criativo, ordem do Minecraft 1.12.2. Gerado por tools/gen_creative_1122.py", "# '@ABA textura-do-icone'; depois 'textura[,alt][|Nome]' (sem nome = item/bloco direto; com nome = item de inventario que reusa a textura)"]
miss = []; tot = 0
for t, lst in tabs.items():
    out.append("@%s %s" % (t, ICON[t]))
    for e in lst:
        tex = e.split("|")[0].split(","); 
        if have and not any(x in have for x in tex): miss.append((t, e)); continue
        out.append(e); tot += 1
open(os.path.join(os.path.dirname(__file__), "../assets/creative/mc-1.12.2.cfg"), "w").write("\n".join(out) + "\n")
print("entradas:", tot, "| sem textura no pack:", len(miss))
for m in miss: print("  falta", m)
