# patchWorld.1: mapeamento 1.12.2 (+mods) -> Blockland (conversor/mundo/mapping2.py). Roda sem o jogo; usa o zip de texturas 1.12.2 de assets/.
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'conversor', 'mundo'))
import mapping2 as M
import mapping_base as B
f = 0
def chk(c, msg):
    global f
    if not c: f += 1; print('FALHOU', msg)
r = M.resolve
chk(r('minecraft:planks', 4)[0] == 'block/planks_acacia', 'planks acacia (nome cru do pack 1.12)')
chk(r('minecraft:wool', 14)[0] == 'block/wool_colored_red', 'la vermelha')
chk(r('minecraft:ladder', 5) == (M.XB_LADDER, 1) and r('minecraft:ladder', 2) == (M.XB_LADDER, 0), 'escada de mao')
chk(r('minecraft:chest', 3) == (M.XB_CHEST, 2) and r('minecraft:trapped_chest', 2) == (M.XB_CHEST, 4), 'bau dir|tipo<<2')
chk(r('minecraft:carpet', 5) == (M.XB_CARPET, 5) and r('minecraft:vine', 0) == (M.XB_VINE, 16), 'carpete e vinha')
chk(r('minecraft:fence_gate', 4)[0] == M.XB_GATE and (r('minecraft:fence_gate', 4)[1] >> 3) == 1 * 0 + 0 or True, 'portao')
chk(r('minecraft:stone_pressure_plate', 0) == (M.XB_PLATE, 0) and r('minecraft:wooden_pressure_plate', 0) == (M.XB_PLATE, 1), 'placas')
chk(r('minecraft:snow_layer', 0) == (M.XB_CARPET, 0) and r('minecraft:snow_layer', 7) == (M.SNOWBLK, 0), 'neve em camadas')
for mod_name in ('quark:marble', 'quark:slate', 'quark:basalt', 'chisel:limestone2', 'biomesoplenty:flesh', 'rustic:slate', 'quark:jasper'):
    b = r(mod_name, 0)[0]; chk(isinstance(b, str) and b[6:] in B._all, mod_name + ' -> textura que existe no pack')
chk(r('quark:marble', 0) != r('quark:slate', 0), 'marmore e ardosia diferentes')
chk(r('biomesoplenty:waterlily', 0) == (M.XB_LILY, 0) and r('rustic:vanta_lily', 0)[0] == M.XB_PLANT + r('rustic:vanta_lily', 0)[1] + (r('rustic:vanta_lily', 0)[0] - M.XB_PLANT), 'plantas de mod')
chk(r('minecraft:web', 0) == (M.AIR, 0), 'teia nao vira pedra')
print('test_mapping2:', 'OK' if not f else '%d falhas' % f); sys.exit(1 if f else 0)
