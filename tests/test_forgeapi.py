"""patchForge.1: catalogo da API do Forge (conversor/modconv/forgeapi.py) e classificacao do forge.py. Uso: python3 tests/test_forgeapi.py"""
import glob, os, sys
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor'); sys.path.insert(0, R)
from modconv import forge, forgeapi, core
n = f = 0
def ok(c, m):
    global n, f
    n += 1
    if not c: f += 1; print('FALHOU:', m)
A = forgeapi.api()
ok(A.ok and A.forge.startswith('14.23'), 'catalogo carregou (db/forge_api.json.gz)')
ok(len(A.c) > 1400, 'classes do Forge: %d' % len(A.c))
ok(sum(1 for k in A.c if A.is_event(k)) > 250, 'eventos do Forge')
ok(A.has('net/minecraftforge/oredict/OreDictionary', 'registerOre') is True, 'OreDictionary.registerOre existe')
ok(A.has('net/minecraftforge/oredict/OreDictionary', 'naoExiste') is False, 'metodo inexistente -> False')
ok(A.has('java/lang/String', 'x') is None, 'classe fora do Forge -> None')
ok(A.has('net/minecraftforge/client/model/ModelLoader', 'registerItemVariants') == 'mc', 'metodo herdado do Minecraft -> mc')
e = A.event('net/minecraftforge/event/world/BlockEvent$BreakEvent'); ok(e and e['cancelavel'], 'BreakEvent e cancelavel')
ok(A.event('net/minecraftforge/oredict/OreDictionary') is None, 'OreDictionary nao e evento')
ok(A.mc_known('func_175656_a') and not A.mc_known('func_999999999_z'), 'nomes SRG do Minecraft 1.12.2')
ok(A.is_mc('net/minecraft/world/World'), 'classe do Minecraft')
ok(forge.classify('net/minecraft/world/World.func_175656_a')[1] != 'chamada desconhecida', 'SRG classificado por pacote')
ok(forge.classify('net/minecraftforge/fml/relauncher/SideOnly.value')[0] == 'ignorado', 'SideOnly ignorado')
ok(forge.event_status('net/minecraftforge/client/event/RenderGameOverlayEvent')[0] == 'ignorado', 'evento de cliente ignorado')
ok(forge.event_status('net/minecraftforge/event/entity/living/LivingDropsEvent')[0] == 'sem equivalente', 'evento de jogo sem equivalente')
class C:
    def gap(s, *a): pass
    def stat(s, *a): pass
j = sorted(glob.glob(os.path.join(R, '..', 'mods', 'Chisel*.jar')))
if j:
    r = forge.scan(core.Jar(j[0]), C())
    ok(r and all(d['api'] != '?' for d in r.values()), 'scan marca api em toda chamada')
    ok(not any(d['alvo'] == 'chamada desconhecida' for d in r.values()), 'nenhuma chamada desconhecida (Chisel)')
    ok(any(m.get('modid') == 'chisel' for m in forge.LAST['mods']), '@Mod lido do bytecode')
    ok(any(v['status'] == 'convertido' for v in forge.LAST['eventos'].values()), 'ModelRegistryEvent convertido')
print('test_forgeapi: %d verificacoes, %d falhas' % (n, f)); sys.exit(1 if f else 0)
