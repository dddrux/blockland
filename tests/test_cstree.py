"""patchProp.3: hierarquia de nos do .csmodel (csmodel.compose / par_*) e escolha automatica de estrategia."""
import math, sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'conversor', 'modconv')); sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'conversor', 'modconv', 'handlers'))
import csmodel
fails = checks = 0
def CHECK(c, m=''):
    global fails, checks; checks += 1
    if not c: fails += 1; print('  FALHOU', m)
def N(i, tail, pos, q=(1, 0, 0, 0), size=(2, 2, 2), sc=(1, 1, 1)): return dict(id=i, tail=tail, pos=pos, off=(0, 0, 0), sc=sc, q=q, size=size, uv=[0] * 12, name='n%d' % i)
q90 = (math.cos(math.pi / 4), 0, math.sin(math.pi / 4), 0)
# arvore: 0 -> (1 -> 2), 3 ; 0 gira 90 graus em Y
nodes = [N(0, 2, (0, 0, 0), q90), N(1, 1, (1, 0, 0)), N(2, 0, (1, 0, 0)), N(3, 0, (0, 1, 0))]
par = csmodel.par_childcount(nodes); CHECK(par == [-1, 0, 1, 0], par); CHECK(csmodel._valid(par))
w = csmodel.compose(nodes, par)
CHECK(all(abs(a - b) < 1e-6 for a, b in zip(w[2]['pos'], (0, 0, -2))), w[2]['pos'])      # +X local do pai girado 90 em Y -> -Z; dois degraus = -2
CHECK(all(abs(a - b) < 1e-6 for a, b in zip(w[3]['pos'], (0, 1, 0))))
CHECK(abs(w[2]['q'][0] - q90[0]) < 1e-6 and abs(w[2]['q'][2] - q90[2]) < 1e-6)            # rotacao do pai herdada
big = [N(0, 1, (0, 0, 0), sc=(2, 2, 2)), N(1, 0, (1, 0, 0))]
CHECK(abs(csmodel.compose(big, [-1, 0], True)[1]['pos'][0] - 2) < 1e-6 and abs(csmodel.compose(big, [-1, 0], False)[1]['pos'][0] - 1) < 1e-6)   # escala do pai
CHECK(not csmodel._valid([1, 0]) and not csmodel._valid([0]) and csmodel._valid([-1, 0, 0]))   # ciclo / auto-referencia
CHECK(csmodel.par_flat(nodes) == [-1] * 4)
CHECK(csmodel.par_childcount([N(0, 5, (0, 0, 0)), N(1, 0, (0, 0, 0))]) == [-1, 0])   # filhos a mais que nos: nao quebra
import importlib; dc = importlib.import_module('decocraft')
class Ctx:
    def __init__(s): s.st = {}; s.g = {}
    def stat(s, k, v): s.st[k] = v
    def gap(s, k, w, n=1): s.g[k] = n
# modelo onde so a leitura 'childcount' deixa tudo centrado/apoiado: pai em (0,0,0), filhos com offsets opostos relativos
tree = [N(0, 2, (0, 0, 0), size=(16, 16, 16)), N(1, 0, (0.0, 0.0, 0.0), size=(16, 16, 16)), N(2, 0, (0.0, 0.0, 0.0), size=(16, 16, 16))]
ctx = Ctx(); s, inh = dc.pick_strategy(lambda aid: (tree, b''), {1: 1.0}, ctx); CHECK(s in csmodel.PARENTS and 'hierarquia escolhida' in ctx.st, (s, ctx.st))
# patchProp.5a: id = (pai<<16)|proprio; filho = pivo do pai + rot(q, off_pai + pos); tampa assenta no topo da caixa (presente)
gift = [N(-65536, 0, (0, 0, 0), size=(0, 0, 0)), N(3, 0, (0, 0.525, 0), size=(18, 21, 18)), N(196612, 0, (-0.1, 0.362, 0.275), size=(20, 4, 20))]
gift[1]['off'] = (0, -0.262, 0); gift[2]['off'] = (0.1, -0.05, -0.275)
CHECK(csmodel.par_pair(gift) == [-1, 0, 1] and csmodel.has_pair(gift), csmodel.par_pair(gift))   # a raiz tambem e pai
cg = csmodel.compose_pair(gift); topbox = 0.525; lidbot = cg[2]['pos'][1] + cg[2]['off'][1] - 4 * 0.4 / 32
CHECK(abs(lidbot - topbox) < 0.01, lidbot)
print('test_cstree: %d verificacoes, %d falhas' % (checks, fails)); sys.exit(1 if fails else 0)
