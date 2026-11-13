#!/usr/bin/env python3
"""patchF1-F7: forgestate (forge_marker/defaults), placers (pares tecnicos), gear (classificacao por nome). Uso: python3 tests/test_forgeinfo.py"""
import os, sys, re
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv'))
import forgestate as fs, placers, gear
ok = 0
def eq(a, b, m):
    global ok
    assert a == b, '%s: %r != %r' % (m, a, b); ok += 1
# vanilla
eq(fs.pick({'normal': {'model': 'a'}})['model'], 'a', 'normal')
eq(fs.pick({'facing=north': {'model': 'b', 'y': 90}})['y'], 90, 'vanilla 1a chave')
# forge por propriedade (persiana do cfm)
v = fs.pick({'facing': {'north': {'y': 270}, 'east': {'y': 0}}, 'left': {'true': {'model': 'cfm:x_stringless'}, 'false': {'model': 'cfm:x'}}, 'inventory': {'model': 'cfm:x'}})
eq((v['model'], v['y']), ('cfm:x', 270), 'forge propriedades (patchConv.1: estado de repouso left=false)')
# defaults no topo + variante so com texturas (Chisel)
v = fs.pick({'blinds': [{'textures': {'all': 't/blinds'}}]}, {'defaults': {'model': 'cube_all'}})
eq((v['model'], v['textures']['all']), ('cube_all', 't/blinds'), 'defaults do topo')
# so inventory
eq(fs.pick({'inventory': {'model': 'm'}})['model'], 'm', 'so inventory')
eq(fs.pick({}), None, 'vazio')
# pares tecnicos: aberto/fechado -> um fica
class J:
    def __init__(s, n): s.n = n; s.names = n
    def find(s, p): r = re.compile(p); return [x for x in s.n if r.search(x)]
    def json(s, f): return {'variants': {'': {'model': 'ns:' + os.path.basename(f)[:-5]}}}
j = J(['assets/ns/blockstates/blinds_closed.json', 'assets/ns/blockstates/blinds_open.json', 'assets/ns/blockstates/lamp.json'])
pl, tech = placers.analyse(j, 'ns', {})
eq(tech, {'blinds_open'}, 'bloco tecnico')
# espadas/ferramentas por nome
eq(gear.kind_of('ruby_sword'), 'sword', 'sword'); eq(gear.kind_of('steel_pickaxe'), 'pickaxe', 'pickaxe'); eq(gear.kind_of('plank'), None, 'nao arma')
eq(gear.tier_of('diamond_sword')[1], 3, 'tier diamante')
import objmodel, forge, forgemap
eq(objmodel.bbox('v 0 0 0\nv 1 0.5 1'), ([0.0, 0.0, 0.0], [16.0, 8.0, 16.0]), 'obj bbox')
eq(forge._kind('forge:ore_shaped', {}), 'shaped', 'ore_shaped'); eq(forge._kind('cutting_shapeless', {'cutting_shapeless': 'X'}), 'shapeless', 'fabrica')
eq(forgemap.props({'variants': {'facing=north,open=true': {}, 'facing=east,open=false': {}}}), {'facing': ['east', 'north'], 'open': ['false', 'true']}, 'estados vanilla')
eq(forgemap.props({'variants': {'facing': {'north': {}, 'south': {}}}}), {'facing': ['north', 'south']}, 'estados forge')
eq(len(fs.multipart({'multipart': [{'apply': {'model': 'a'}}, {'when': {'open': 'true'}, 'apply': {'model': 'b'}}, {'when': {'open': 'false'}, 'apply': {'model': 'c'}}]})), 2, 'multipart repouso')
eq(fs.when_ok({'OR': [{'a': 'true'}, {'b': 'false'}]}), True, 'multipart OR')
import lights; eq(bool(lights.OFF.search('cfm_lamp_off')), True, 'luz off')
print('forgeinfo: OK (%d checks)' % ok)
