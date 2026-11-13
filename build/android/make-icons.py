#!/usr/bin/env python3
# Gera os icones do launcher a partir de ../blockland.png (raiz do projeto)
import os, sys
from PIL import Image
here = os.path.dirname(os.path.abspath(__file__))
src = Image.open(os.path.join(here, '..', 'blockland.png')).convert('RGBA')
w, h = src.size; s = min(w, h)
src = src.crop(((w - s) // 2, (h - s) // 2, (w + s) // 2, (h + s) // 2))
for d, px in {'mdpi': 48, 'hdpi': 72, 'xhdpi': 96, 'xxhdpi': 144, 'xxxhdpi': 192}.items():
    o = os.path.join(here, 'app/src/main/res/mipmap-' + d); os.makedirs(o, exist_ok=True)
    src.resize((px, px), Image.LANCZOS).save(os.path.join(o, 'ic_launcher.png'))
print('icones OK')
