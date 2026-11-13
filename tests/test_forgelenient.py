"""patchForge.2: JSON leniente (Gson), condicoes de receita do Forge e tipo de receita pela fabrica. Uso: python3 tests/test_forgelenient.py"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor'))
from modconv import forge, jsonlenient as L
n = f = 0
def ok(c, m):
    global n, f
    n += 1
    if not c: f += 1; print('FALHOU:', m)
ok(L.loads('{ a:b" }') == {'a': 'b"'}, 'valor sem aspas com aspa no fim (BOP double_slab)')
ok(L.loads("{'a': [1, 2.5, true, null,], b = 'x'; // c\n /* z */ c => 3}") == {'a': [1, 2.5, True, None], 'b': 'x', 'c': 3}, 'aspas simples, = e =>, ; e comentarios')
ok(L.loads('# c\n{"a": "\\u00e9\\n"}') == {'a': 'é\n'}, 'comentario # e escapes')
ok(L.loads(")]}'\n[1]") == [1], 'prefixo anti-XSSI')
try: L.loads('{"a":'); ok(False, 'JSON truncado deve dar erro')
except ValueError: ok(True, '')
class C:
    g = []
    def gap(s, a, b=''): s.g.append(a)
ok(forge.cond_ok([{'type': 'forge:false'}]) is False, 'forge:false')
ok(forge.cond_ok([{'type': 'forge:mod_loaded', 'modid': 'minecraft'}]) is True, 'mod_loaded minecraft')
ok(forge.cond_ok({'type': 'forge:mod_loaded', 'modid': 'naoexiste'}) is False, 'mod_loaded ausente')
ok(forge.cond_ok({'type': 'forge:not', 'value': {'type': 'forge:false'}}) is True, 'forge:not')
ok(forge.cond_ok({'type': 'forge:or', 'values': [{'type': 'forge:false'}, {'type': 'forge:mod_loaded', 'modid': 'forge'}]}) is True, 'forge:or')
ok(forge.cond_ok({'type': 'forge:and', 'values': [{'type': 'forge:false'}, {'type': 'forge:mod_loaded', 'modid': 'forge'}]}) is False, 'forge:and')
ok(forge.cond_ok({'type': 'minecraft:item_exists', 'item': 'naoexiste:x'}) is False, 'item_exists de mod ausente')
c = C(); ok(forge.cond_ok({'type': 'x:esquisita'}, c) is True and c.g, 'condicao desconhecida: verdadeira + lacuna')
forge.LOADED.add('cfm'); ok(forge.cond_ok({'type': 'forge:mod_loaded', 'modid': 'CFM'}) is True, 'modid sem diferenciar maiusculas')
fac = {'crafting_shaped_premade': 'com.x.ShapedPremadeFactory', 'cutting_shapeless': 'com.x.CuttingFactoryShapeless', 'forno': 'com.x.FurnaceFactory'}
ok(forge._kind('mod:crafting_shaped_premade', fac) == 'shaped', 'fabrica ShapedPremade -> shaped')
ok(forge._kind('cutting_shapeless', fac) == 'shapeless', 'fabrica ...Shapeless -> shapeless')
ok(forge._kind('forno', fac) == 'smelt', 'fabrica Furnace -> smelt')
ok(forge._kind('ore_shaped', {}) == 'shaped' and forge._kind('desconhecida', {}) is None, 'tipos embutidos e desconhecido')
print('test_forgelenient: %d verificacoes, %d falhas' % (n, f)); sys.exit(1 if f else 0)
