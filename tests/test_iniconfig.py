#!/usr/bin/env python3
"""patchIni.1: receitas .ini do IC2 (shaped/shapeless/furnace/maquina) -> receitas.txt / maquinas.txt. Jar sintetico."""
import io, json, os, sys, tempfile, zipfile
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv'))
import core, iniconfig
fails = 0
def ck(c, m):
    global fails
    if not c: fails += 1; print('FALHOU:', m)
class Ctx:
    def __init__(s): s.g = []; s.s = {}
    def gap(s, k, w, n=1): s.g.append((k, w))
    def stat(s, k, v): s.s[k] = v
d = tempfile.mkdtemp(); jp = os.path.join(d, 't.jar'); out = os.path.join(d, 'out'); os.makedirs(out)
z = zipfile.ZipFile(jp, 'w')
z.writestr('assets/ic2/config/shaped_recipes.ini', '; c\n"PSP|SPS|PSP" P:OreDict:plateLead S:minecraft:stone@* = ic2:resource#reactor_vessel*4\n"CCC|C C" C:ic2:cable#type:copper,insulation:1 = ic2:te#batbox\n"S S|S S" S:OreDict:stickWood = ic2:crop_stick*2\n"AB" A:ic2:re_battery|ic2:wrench B:minecraft:dye@4 = ic2:resource#reactor_vessel @hidden\n')
z.writestr('assets/ic2/config/shapeless_recipes.ini', 'OreDict:itemRubber ic2:cable#type:tin,insulation:0 = ic2:cable#type:tin,insulation:1\nminecraft:redstone*2 OreDict:dustCoal = ic2:crop_stick\n')
z.writestr('assets/ic2/config/furnace.ini', 'ic2:resource#copper_ore = ic2:ingot#copper @xp:0.5\nic2:crushed#iron = minecraft:iron_ingot\n')
z.writestr('assets/ic2/config/macerator.ini', 'OreDict:oreTin = ic2:crushed#tin*2\nminecraft:stone = minecraft:cobblestone\nminecraft:coal = ic2:dust#coal, ic2:dust#nada\n')
z.close()
json.dump({'items': [{'id': i} for i in ('ic2_resource_reactor_vessel', 'ic2_resource_copper_ore', 'ic2_resource_ingot_copper', 'ic2_resource_crushed_iron', 'ic2_resource_crushed_tin', 'ic2_crop_stick', 'ic2_cable_tin_cable_0', 'ic2_cable_tin_cable_1', 'ic2_cable_copper_cable_1', 'ic2_tool_wrench', 'ic2_resource_dust_coal', 'ic2_crafting_rubber', 'ic2_tool_cutter')]}, open(os.path.join(out, 'props.json'), 'w'))
c = Ctx(); R, M = iniconfig.extract(core.Jar(jp), out, ['ic2'], c)
ck('shaped ic2:resource_reactor_vessel 4 PSP|SPS|PSP P=ore:plateLead;S=minecraft:stone' in R, 'shaped com ore e @* ' + str(R))
ck('shaped ic2:crop_stick 2 S S|S S S=ore:stickWood' in R, 'padrao com espaco')
ck(not any('batbox' in r for r in R), 'te#batbox (sem item) nao vira receita')
ck(any(r.startswith('shaped ic2:resource_reactor_vessel 1 AB ') and 'A=ic2:tool_wrench' in r and 'B=minecraft:dye@4' in r for r in R), 'alternativa a|b resolve na 1a existente, @hidden ignorado, meta de vanilla ' + str(R))
ck('shapeless ic2:cable_tin_cable_1 1 ic2:crafting_rubber,ic2:cable_tin_cable_0' in R, 'shapeless com cabo type/insulation')
ck('shapeless ic2:crop_stick 1 minecraft:redstone,minecraft:redstone,ore:dustCoal' in R, 'ingrediente *2 repete')
ck('smelt ic2:resource_copper_ore ic2:resource_ingot_copper 0.5' in R, 'smelt com xp')
ck('smelt ic2:resource_crushed_iron minecraft:iron_ingot 0' in R, 'smelt sem xp')
ck('macerator|ore:oreTin*1|ic2:resource_crushed_tin*2' in M, 'maquina com saida *2')
ck(not any('nada' in m for m in M), 'maquina com saida sem item e descartada inteira')
ck(any('batbox' in w or 'te#batbox' in w for k, w in c.g), 'lacuna registrada')
ck(c.s.get('receitas_ini') == len(R), 'stat')
z2 = os.path.join(d, 'a.jar'); zz = zipfile.ZipFile(z2, 'w'); zz.writestr('assets/ic2/config/shapeless_recipes.ini', 'OreDict:itemRubber OreDict:craftingToolWireCutter OreDict:chestWood OreDict:semAlias = ic2:crop_stick\n'); zz.close()
R3, _ = iniconfig.extract(core.Jar(z2), out, ['ic2'], Ctx())
ck(any('ic2:crafting_rubber' in r and 'ic2:tool_cutter' in r and 'minecraft:chest' in r and 'ore:semAlias' in r for r in R3), 'alias de ore dict ' + str(R3))
# receitas.txt existente (do forge.py) e preservado e sem duplicar
open(os.path.join(out, 'receitas.txt'), 'w').write('shaped minecraft:stick 4 A|A A=minecraft:planks\n' + R[0] + '\n')
R2, _ = iniconfig.extract(core.Jar(jp), out, ['ic2'], Ctx()); t = open(os.path.join(out, 'receitas.txt')).read().splitlines()
ck(t[0].startswith('shaped minecraft:stick') and len(t) == len(set(t)) and len(t) >= len(R) + 1, 'merge sem duplicar ' + str(len(t)))
print('test_iniconfig: %d falha(s)' % fails); sys.exit(1 if fails else 0)
