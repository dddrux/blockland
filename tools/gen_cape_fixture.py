#!/usr/bin/env python3
# Gera /tmp/cape_t/{anim,trans}.{gif,raw} para tests/test_cape.cpp (decodificador de GIF contra o Pillow).
# anim: 40x30, 4 quadros, atrasos 50/120/300/40 ms; raw = quadros compostos RGBA (como o Pillow decodifica).
# trans: 16x16 com cor transparente; raw = RGBA do quadro 0.
import os, sys
from PIL import Image, ImageDraw
out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/cape_t"
os.makedirs(out, exist_ok=True)
frames = []
for f in range(4):
    im = Image.new("RGB", (40, 30), (20 + f * 40, 60, 200 - f * 40))
    d = ImageDraw.Draw(im)
    d.rectangle([5 + f * 6, 4, 14 + f * 6, 20], fill=(250, 220 - f * 50, 30 * f))
    d.line([0, 29 - f * 5, 39, f * 5], fill=(255, 255, 255))
    frames.append(im.convert("P", palette=Image.ADAPTIVE, colors=32))
frames[0].save(f"{out}/anim.gif", save_all=True, append_images=frames[1:], duration=[50, 120, 300, 40], loop=0)
g = Image.open(f"{out}/anim.gif"); raw = bytearray()
for i in range(g.n_frames):
    g.seek(i); raw += g.convert("RGBA").tobytes()
open(f"{out}/anim.raw", "wb").write(bytes(raw))
t = Image.new("P", (16, 16), 0); t.putpalette([0, 0, 0, 255, 0, 0, 0, 255, 0] + [0] * 759)
ImageDraw.Draw(t).rectangle([4, 4, 11, 11], fill=1); ImageDraw.Draw(t).rectangle([6, 6, 9, 9], fill=2)
t.save(f"{out}/trans.gif", transparency=0)
open(f"{out}/trans.raw", "wb").write(Image.open(f"{out}/trans.gif").convert("RGBA").tobytes())
# plain.png: imagem lisa azul (qualquer capa sem layout: o jogo a distribui na face de fora); layout.png: layout 64x32 do MC (vermelho opaco na face de fora, resto transparente)
Image.new("RGB", (46, 22), (20, 40, 230)).save(f"{out}/plain.png")
lay = Image.new("RGBA", (64, 32), (0, 0, 0, 0)); ImageDraw.Draw(lay).rectangle([1, 1, 11, 17], fill=(255, 0, 0, 255)); lay.save(f"{out}/layout.png")
