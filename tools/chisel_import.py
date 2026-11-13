#!/usr/bin/env python3
# Converte os assets do Chisel 1.12.2 (zip organizado) em assets/chisel/ para o Blockland.
# Uso: python3 tools/chisel_import.py <pasta_chisel_extraida> [saida=assets/chisel]
# Saida: tiles/<hash>.png (16x16) e chisel.cfg (file<TAB>nome<TAB>top<TAB>side<TAB>bottom<TAB>luz)
import sys, os, re, json, hashlib
from PIL import Image
src = sys.argv[1]; out = sys.argv[2] if len(sys.argv) > 2 else 'assets/chisel'
TEX = os.path.join(src, '02_Texturas_de_blocos'); BS = os.path.join(src, '07_Blockstates')
os.makedirs(out + '/tiles', exist_ok=True)
lang = {}
for ln in open(os.path.join(src, '09_Idiomas_e_guias/lang/en_us.lang'), encoding='utf-8', errors='ignore'):
    if '=' in ln: k, v = ln.rstrip('\n').split('=', 1); lang[k] = v
LIGHT = {'glowstone': 15, 'lavastone': 15, 'magma': 3, 'redstoneLamp': 15}
SKIP = {'auto_chisel', 'carpet'}
cache = {}; tiles = {}
def load(ref):
    if ref in cache: return cache[ref]
    r = None
    m = re.match(r'chisel:blocks/(.+)$', ref or '')
    if m:
        p = os.path.join(TEX, m.group(1) + '.png')
        if os.path.isfile(p):
            im = Image.open(p).convert('RGBA'); w, h = im.size
            if h > w: im = im.crop((0, 0, w, w))          # animacao: 1o quadro
            if im.width > 16: im = im.resize((16, 16), Image.BOX)
            elif im.width < 16: im = im.resize((16, 16), Image.NEAREST)
            r = im
    cache[ref] = r; return r
def tid(im):
    h = hashlib.md5(im.tobytes()).hexdigest()[:10]
    if h not in tiles: im.save(f'{out}/tiles/{h}.png'); tiles[h] = 1
    return h
PT = {'Concrete':'Concreto','Bricks':'Tijolos','Brick':'Tijolo','Block':'Bloco','of':'de','Of':'de','Tiles':'Ladrilhos','Tile':'Ladrilho','Small':'Pequeno','Stone':'Pedra','Red':'Vermelho','Sandstone':'Arenito','Pillar':'Pilar','Wood':'Madeira','Cracked':'Rachado','Planks':'Tábuas','Disordered':'Desordenado','French':'Francês','Circular':'Circular','Blue':'Azul','Light':'Claro','Gray':'Cinza','Coal':'Carvão','Quartz':'Quartzo','Bookshelf':'Estante','Panel':'Painel','Weathered':'Desgastado','Braid':'Trança','Ornate':'Ornamentado','Oak':'Carvalho','Marble':'Mármore','Wide':'Largo','Dent':'Amassado','Layers':'Camadas','Mosaic':'Mosaico','Big':'Grande','Pink':'Rosa','Purple':'Roxo','Encased':'Encaixado','Jellybean':'Jujuba','Prism':'Prisma','Road':'Estrada','Twisted':'Torcido','Slanted':'Inclinado','Black':'Preto','Arrayed':'Enfileirado','Trodden':'Pisado','Ice':'Gelo','Cuts':'Cortes','Zag':'Zigue-zague','Celtic':'Celta','White':'Branco','Yellow':'Amarelo','Brown':'Marrom','Orange':'Laranja','Magenta':'Magenta','Green':'Verde','Lime':'Lima','Cyan':'Ciano','Cobblestone':'Pedregulho','Factory':'Fábrica','Ancient':'Antigo','Arcane':'Arcano','Basalt':'Basalto','Charcoal':'Carvão Vegetal','Coke':'Coque','Glowstone':'Pedra Luminosa','Limestone':'Calcário','Andesite':'Andesito','Moss':'Musgo','Diorite':'Diorito','Endstone':'Pedra do End','Granite':'Granito','Terracotta':'Terracota','Lavastone':'Pedra de Lava','Prismarine':'Prismarinho','Waterstone':'Pedra de Água','Scribbles':'Rabiscos','Temple':'Templo','Wool':'Lã','Dark':'Escuro','Redstone':'Redstone','Acacia':'Acácia','Birch':'Bétula','Jungle':'Selva','Spruce':'Pinheiro','Vertical':'Vertical','Horizontal':'Horizontal','Nether':'Nether','Glass':'Vidro','Gold':'Ouro','Large':'Grande','Raw':'Bruto','Crate':'Caixote','Antiblock':'Antibloco','Powder':'Pó','Iron':'Ferro','Laboratory':'Laboratório','Mossy':'Musgoso','Legacy':'Clássico','Llama':'Lhama','Dirt':'Terra','Obsidian':'Obsidiana','Capped':'Com Tampa','Netherrack':'Netherrack','Plates':'Placas','Plate':'Placa','Diamond':'Diamante','Plain':'Liso','Fancy':'Elegante','Nailed':'Pregado','Disarray':'Desarrumado','Hieroglyphs':'Hieróglifos','Emerald':'Esmeralda','Greek':'Grego','Medium':'Médio','Lapis':'Lápis','Lazuli':'Lazúli','Paper':'Papel','Papers':'Papéis','Wall':'Parede','Scaffold':'Andaime','Uneven':'Irregular','Abandoned':'Abandonado','Smooth':'Liso','Planked':'Em Tábuas','Cloud':'Nuvem','Steel':'Aço','Shiny':'Brilhante','Simple':'Simples','Metal':'Metal','Old':'Velho','Vents':'Respiros','Coin':'Moeda','Lava':'Lava','Rocky':'Rochoso','Skull':'Caveira','Pattern':'Padrão','Streaks':'Listras','Circuit':'Circuito','Column':'Coluna','Rusty':'Enferrujado','Framed':'Emoldurado','Tomes':'Tomos','Stacks':'Pilhas','Space':'Espaço','Meat':'Carne','Cans':'Latas','Rainbow':'Arco-íris','Novice':'Novato','Stand':'Suporte','Blinds':'Persianas','Brownstone':'Arenito Marrom','Valentines':'Dia dos Namorados','Cobble':'Pedregulho','Encrusted':'Incrustado'}
NOUNS = {'Tijolos','Tijolo','Bloco','Ladrilhos','Ladrilho','Pedra','Pilar','Painel','Estante','Concreto','Arenito','Madeira','Tábuas','Carvão','Quartzo','Vidro','Gelo','Mármore','Calcário','Basalto','Granito','Diorito','Andesito','Lã','Terracota','Caixote','Placas','Placa','Parede','Coluna'}
def pt(n):
    out = []
    for part in n.split(': '):
        w = [PT.get(x, x) for x in re.findall(r"[A-Za-z']+", part.replace('Light ', 'Light_').replace('Dark ', 'Dark_'))]
        w = [{'Light_Gray': 'Cinza Claro', 'Light_Blue': 'Azul Claro', 'Dark_Gray': 'Cinza Escuro'}.get(x, x) for x in w] if False else w
        t = part.replace('Light Gray', 'Cinza_Claro').replace('Light Blue', 'Azul_Claro').replace('Dark Oak', 'Carvalho_Escuro').replace('Dark Gray', 'Cinza_Escuro')
        w = [PT.get(x, x).replace('_', ' ') if x not in ('Cinza_Claro', 'Azul_Claro', 'Carvalho_Escuro', 'Cinza_Escuro') else x.replace('_', ' ') for x in re.findall(r"[A-Za-z'_]+", t)]
        if 'de' not in w:
            idx = [i for i, x in enumerate(w) if x in NOUNS]
            if idx and idx[-1] != 0: i = idx[-1]; w = [w[i]] + w[:i] + w[i + 1:]
            if idx and w[0] in NOUNS:   # concordancia com o substantivo (genero e numero)
                fem = w[0].rstrip('s') in ('Pedra', 'Placa', 'Tábua', 'Madeira', 'Lã', 'Parede', 'Coluna', 'Estante', 'Terracota')
                plu = w[0].endswith('s') and w[0] not in ('Mármore',)
                def ag(x):
                    if x in NOUNS or x in ('de',) or ' ' in x or len(x) < 4 or x[-1] not in 'oa': return x
                    if fem and x[-1] == 'o': x = x[:-1] + 'a'
                    return x + 's' if plu else x
                w = [w[0]] + [ag(x) for x in w[1:]]
        out.append(' '.join(w))
    return ': '.join(out)
def title(s): return ' '.join(w.capitalize() for w in re.split(r'[-_]', s))
rows = []; skipped = 0
for f in sorted(os.listdir(BS)):
    mat = f[:-5]
    if mat in SKIP: continue
    try: d = json.loads(re.sub(r',\s*([}\]])', r'\1', open(os.path.join(BS, f), encoding='utf-8').read()))
    except Exception: continue
    dm = d.get('defaults', {}).get('model', 'cube_all'); mname = lang.get(f'tile.chisel.{mat}.name', title(mat))
    for var, v in d.get('variants', {}).items():
        if isinstance(v, list): v = v[0]
        if not isinstance(v, dict) or var in ('normal', 'inventory'): continue
        model = v.get('model', dm); t = v.get('textures', {})
        top = side = bot = None
        if model in ('cube_all', 'chisel:cube_ctm', 'chisel:cube_eldritch', 'chisel:cube_2_layer_sides'):
            top = side = bot = load(t.get('all') or t.get('side'))
        elif model in ('cube_column', 'chisel:column_eldritch', 'chisel:mossy/mossy_column'):
            side = load(t.get('side')); top = bot = load(t.get('end')) or side
        elif model == 'cube_bottom_top':
            top, side, bot = load(t.get('top')), load(t.get('side')), load(t.get('bottom'))
        elif model in ('chisel:lavastone/column_lava', 'chisel:waterstone/column_water'):
            side = load(t.get('side')); top = bot = load(t.get('top'))
        elif model in ('chisel:cube_2_layer', 'chisel:mossy/mossy', 'chisel:mossy/mossy_ctm', 'chisel:cube_2_layer_topshaded'):
            b, o = load(t.get('bot')), load(t.get('top'))
            top = side = bot = (Image.alpha_composite(b, o) if b and o else (b or o))
        elif 'lava' in model or 'water' in model:
            top = side = bot = load(t.get('top'))
        else: skipped += 1; continue
        if not (top and side and bot): skipped += 1; continue
        desc = lang.get(f'tile.chisel.{mat}.{var}.desc.1', title(var))
        rows.append((f'chisel/{mat}/{var}', pt(f'{mname}: {desc}'), tid(top), tid(side), tid(bot), LIGHT.get(mat, 0)))
with open(out + '/chisel.cfg', 'w', encoding='utf-8') as o:
    o.write('# gerado por tools/chisel_import.py: file\tnome\ttop\tside\tbottom\tluz\n')
    for r in rows: o.write('\t'.join(map(str, r)) + '\n')
print(len(rows), 'blocos;', len(tiles), 'tiles unicos;', skipped, 'ignorados')
