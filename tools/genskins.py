# Gera skins padrao originais (64x64, layout Minecraft). Uso: python3 genskins.py <pasta>
import sys, random
from PIL import Image
out = sys.argv[1]
PARTS = {'head': (0,0,8,8,8), 'body': (16,16,8,12,4), 'rarm': (40,16,4,12,4), 'rleg': (0,16,4,12,4), 'larm': (32,48,4,12,4), 'lleg': (16,48,4,12,4)}
OVER = {'head': (32,0), 'body': (16,32), 'rarm': (40,32), 'rleg': (0,32), 'larm': (48,48), 'lleg': (0,48)}
K = {'top':1.1,'bottom':.7,'right':.88,'left':.88,'front':1.0,'back':.94}
def faces(u,v,w,h,d): return {'top':(u+d,v,w,d),'bottom':(u+d+w,v,w,d),'right':(u,v+d,d,h),'front':(u+d,v+d,w,h),'left':(u+d+w,v+d,d,h),'back':(u+2*d+w,v+d,w,h)}
def sh(c,k): return tuple(max(0,min(255,int(x*k))) for x in c[:3])+(255,)
def paint(px, part, fn, over=False):
    u,v,w,h,d = PARTS[part]
    if over: u,v = OVER[part]
    for f,(fx,fy,fw,fh) in faces(u,v,w,h,d).items():
        for j in range(fh):
            for i in range(fw):
                c = fn(i,j,fw,fh,f)
                if c is None: continue
                px[fx+i,fy+j] = c if len(c)==4 and c[3]<255 else sh(c,K[f])
def make(name, skin, hair, eye, shirt, pants, shoes, trim, cloth, hat, seed):
    r = random.Random(seed); im = Image.new('RGBA',(64,64),(0,0,0,0)); px = im.load()
    def n(c,a=6): d=r.randint(-a,a); return tuple(max(0,min(255,x+d)) for x in c[:3])+(255,)
    def head(i,j,w,h,f):
        if f=='top': return n(hair)
        if f=='bottom': return n(skin)
        if f in('right','left'): return n(hair) if j<3 or i<2 else n(skin)
        if f=='back': return n(hair) if j<6 else n(skin)
        if j<2 or (j==2 and i in(0,7)): return n(hair)
        if j==4 and i in(1,6): return (255,255,255,255)
        if j==4 and i in(2,5): return eye
        if j==6 and 2<=i<=5: return (150,80,70,255)
        return n(skin)
    paint(px,'head',head)
    def hatf(i,j,w,h,f):
        if hat=='cap': return n(trim) if f=='top' or (f!='bottom' and j<3) else None
        if hat=='helmet': return n(trim) if f=='top' or (f!='bottom' and j<4) else None
        if hat=='wizard': return n(trim) if f=='top' or (f!='bottom' and j<2) else None
        if hat=='band': return n(trim) if f not in('top','bottom') and j==2 else None
        if hat=='visor':
            if f=='front' and 2<=j<=5 and 1<=i<=6: return (90,200,255,170)
            return n((235,235,240)) if f!='bottom' else None
        return None
    paint(px,'head',hatf,True)
    def body(i,j,w,h,f):
        if f=='top': return n(shirt)
        if f=='bottom': return n(pants)
        if cloth=='stripes' and j%3==0: return n(trim)
        if j==0 and f=='front' and 2<=i<=5 and cloth!='robe': return n(skin)
        if j>=h-2: return n(trim)
        return n(shirt)
    paint(px,'body',body)
    def jk(i,j,w,h,f):
        if cloth=='robe' and j>h//2 and f not in('top','bottom'): return n(sh(shirt,.85))
        if cloth=='suit' and f=='front' and 3<=j<=6 and 2<=i<=5: return n((60,70,90))
        if cloth=='miner' and f in('front','back') and i in(1,6): return n(trim)
        return None
    paint(px,'body',jk,True)
    for arm in('rarm','larm'):
        def a(i,j,w,h,f):
            if f=='top': return n(shirt)
            if f=='bottom': return n(skin)
            if j<4 or cloth in('robe','suit','ninja'): return n(shirt)
            return n(skin)
        paint(px,arm,a)
        paint(px,arm,lambda i,j,w,h,f: n(trim) if j>=h-2 and f!='top' and cloth in('suit','miner','robe') else None,True)
    for leg in('rleg','lleg'):
        def l(i,j,w,h,f):
            if f=='top': return n(pants)
            if f=='bottom': return n(shoes)
            return n(shoes) if j>=h-3 else n(pants)
        paint(px,leg,l)
    im.save(f'{out}/{name}.png'); return im
SK=[('aventureiro',(232,180,140),(90,60,35),(50,90,160,255),(70,150,70),(60,80,150),(70,70,75),(110,80,50),'plain','none'),
('exploradora',(190,130,95),(25,20,25),(60,40,30,255),(230,130,40),(80,90,70),(60,45,35),(190,90,25),'plain','band'),
('mineiro',(225,170,130),(60,40,30),(40,40,40,255),(150,150,155),(60,60,70),(50,50,55),(245,200,40),'miner','helmet'),
('mago',(240,205,175),(200,200,205),(80,80,180,255),(110,60,160),(80,45,120),(50,35,70),(230,200,60),'robe','wizard'),
('ninja',(210,160,120),(20,20,25),(30,30,30,255),(40,40,48),(35,35,42),(25,25,30),(200,40,40),'ninja','band'),
('astronauta',(230,180,145),(70,50,40),(50,50,50,255),(225,228,235),(210,214,222),(90,95,110),(80,130,200),'suit','visor'),
('pirata',(215,160,120),(45,30,20),(40,60,40,255),(235,235,235),(60,45,40),(35,30,28),(190,35,35),'stripes','cap')]
sheet=Image.new('RGBA',(len(SK)*72,64),(70,70,70,255))
for k,s in enumerate(SK):
    im=make(*s,seed=100+k)
    for (sx,sy,dx,dy,w,h) in [(8,8,16,0,8,8),(40,8,16,0,8,8),(20,20,16,8,8,12),(44,20,8,8,4,12),(36,52,24,8,4,12),(4,20,16,20,4,12),(20,52,20,20,4,12),(20,36,16,8,8,12),(44,36,8,8,4,12),(52,52,24,8,4,12),(4,36,16,20,4,12),(4,52,20,20,4,12)]:
        sheet.alpha_composite(im.crop((sx,sy,sx+w,sy+h)),(k*72+dx+8,dy))
sheet=sheet.resize((sheet.width*3,sheet.height*3),Image.NEAREST); sheet.save(out+'/../sheet.png')
