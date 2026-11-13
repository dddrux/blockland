"""patchForge.3: nomes legiveis SRG (srgnames.py), correcao damageDropped (bytecode.py) e drops por bytecode (dropscode.py). Uso: python3 tests/test_forgedrops.py
Usa os jars reais de mods/ quando existirem (pula o que faltar)."""
import glob, os, sys, tempfile
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'); sys.path.insert(0, os.path.join(R, 'conversor'))
from modconv import srgnames, dropscode, bytecode, core
n = f = 0
def ok(c, m):
    global n, f
    n += 1
    if not c: f += 1; print('FALHOU:', m)

# --- srgnames ---
ok(srgnames.readable('net/minecraft/item/ItemStack.func_77973_b') == 'net/minecraft/item/ItemStack.getItem', 'ItemStack.getItem')
ok(srgnames.readable('net/minecraft/world/World.field_72995_K').endswith('.isRemote'), 'World.isRemote (campo)')
ok(srgnames.readable('net/minecraft/block/Block.func_999999_x') == 'net/minecraft/block/Block.func_999999_x', 'SRG desconhecido fica como esta')
ok(srgnames.readable('net/minecraft/block/Block.getItemDropped') == 'net/minecraft/block/Block.getItemDropped', 'nome legivel nao muda')
ok(srgnames.method_name('func_180651_a', '(Lnet/minecraft/block/state/IBlockState;)I') == 'damageDropped', 'damageDropped com descritor certo')
ok(srgnames.method_name('func_180651_a', '(Ljava/lang/String;)V') is None, 'descritor diferente nao rotula (gate)')
ok(srgnames.vanilla_item('field_151044_h') == 'coal' and srgnames.vanilla_item('field_000') is None, 'itens vanilla conhecidos')
ok(srgnames.coverage(['a/B.func_77973_b', 'a/B.func_1_x', 'a/B.nome'])[1] == 2, 'cobertura conta so SRG')
# --- bug: func_180651_a e damageDropped (dado do item), nao resistencia ---
ok(bytecode.RETCONST.get('func_180651_a') == 'dano_dropado', 'func_180651_a = dano_dropado')
ok(bytecode.RETCONST.get('func_149638_a') == 'resistencia_metodo', 'getExplosionResistance(Entity) = func_149638_a')
# --- classify() com tokens sinteticos ---
reg = {'Mod.GEM': ('gem', 'Item'), 'Mod.ORE': ('ore', 'Block')}
c = dropscode.classify
ok(c(['gs:Mod.GEM', 'RET'], reg, 'm') == ('item', 'm:gem'), 'campo de item -> m:gem')
ok(c(['gs:Mod.ORE', 'st:Item.func_150898_a', 'RET'], reg, 'm') == ('item', 'm:ore'), 'getItemFromBlock(campo) -> m:ore')
ok(c(['gs:Mod.ORE', 'sp:ItemStack.<init>', 'vi:ItemStack.func_77973_b', 'RET'], reg, 'm') == ('item', 'm:ore'), 'new ItemStack(bloco).getItem()')
ok(c(['RET'], reg, 'm')[0] == 'nenhum', 'null -> nenhum')
ok(c(['gs:Items.field_190931_a', 'RET'], reg, 'm')[0] == 'nenhum', 'Items.AIR -> nenhum')
ok(c(['gs:Items.field_151044_h', 'RET'], reg, 'm') == ('item', 'minecraft:coal'), 'Items.COAL')
ok(c(['gs:Items.field_999_x', 'RET'], reg, 'm')[0] == 'vanilla?', 'campo vanilla sem nome: nao inventa')
ok(c(['gf:X.p', 'if:IBlockState.func_177229_b', 'gs:Mod.GEM', 'RET', 'gs:Mod.ORE', 'RET'], reg, 'm')[0] == 'estado', 'depende de estado: so relata')
ok(c(['gs:Mod.NAOEXISTE', 'RET'], reg, 'm')[0] == 'desconhecido', 'campo sem registro')
# --- jars reais ---
def jar(pat):
    g = glob.glob(os.path.join(R, 'mods', pat)); return core.Jar(g[0]) if g else None
j = jar('ironfurnaces*.jar')
if j:
    L, rep = dropscode.scan(j, 'ironfurnaces')
    ok('iron_furnace_active -> ironfurnaces:iron_furnace_idle 1-1 por codigo' in L, 'ironfurnaces: forno aceso solta o apagado')
    ok(len(L) == 10, 'ironfurnaces: 10 regras (%d)' % len(L))
j = jar('furniture*.jar')
if j:
    L, rep = dropscode.scan(j, 'cfm')
    ok('bath_top -> cfm:bath_bottom 1-1 por codigo' in L, 'cfm: parte de cima da banheira solta a de baixo')
    ok('codigo proprio' in rep, 'cfm: caixa de presente/caixote escrevem o proprio drop (sem regra estatica)')
    ok(not any(x.startswith('present ') for x in L), 'cfm: present sem regra estatica')
j = jar('BiomesOPlenty*.jar')
if j:
    L, rep = dropscode.scan(j, 'biomesoplenty')
    ok(any(x.startswith('crystal -> biomesoplenty:crystal_shard') for x in L), 'bop: cristal solta cacos')
    ok(rep.get('estado'), 'bop: drops dependentes de estado ficam no relatorio')
# --- pipeline completo: drops.txt + forge_acoes.json com nome ---
j = jar('ironfurnaces*.jar')
if j:
    import json, subprocess
    with tempfile.TemporaryDirectory() as t:
        r = subprocess.run([sys.executable, os.path.join(R, 'conversor', 'modconv', 'modconv.py'), j.path, '--out', t, '--no-deps'], capture_output=True, text=True)
        ok(r.returncode == 0, 'modconv roda: ' + r.stderr[-200:])
        dp = os.path.join(t, 'ironfurnaces', 'drops.txt'); ok(os.path.exists(dp) and 'por codigo' in open(dp).read(), 'drops.txt tem drops por codigo')
        fa = json.load(open(os.path.join(t, 'ironfurnaces', 'forge_acoes.json')))
        ok(any(v.get('nome') == 'getItem' for v in fa.values()), 'forge_acoes.json traz nome legivel')
print('test_forgedrops: %d verificacoes, %d falhas' % (n, f)); sys.exit(1 if f else 0)
