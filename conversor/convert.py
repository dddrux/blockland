import numpy as np, glob, os, struct, sys, pickle
from nbt import *
import mapping as M
PK0=1016
names=pickle.load(open('names.pkl','rb'))
pal={}; 
def pid(n):
    if n not in pal: pal[n]=len(pal)
    return PK0+pal[n]
lutB=np.zeros(4096*16,np.uint16); lutM=np.zeros(4096*16,np.uint8)
S=pickle.load(open('survey.pkl','rb')); used=(S['o'][0]+S['n'][0])>0
report={}
for k in np.nonzero(used)[0]:
    i,d=int(k)>>4,int(k)&15
    r=M.resolve(names.get(i,'minecraft:stone') if i in names else 'minecraft:stone',d)
    b,m=r; b=pid(b) if isinstance(b,str) else b
    lutB[k]=b; lutM[k]=m&255
def nib(a):
    a=np.frombuffer(a,dtype=np.uint8); o=np.empty(4096,np.uint8); o[0::2]=a&15; o[1::2]=a>>4; return o
XBF=(M.XB_FENCE,M.XB_PANE,M.XB_WALL)
def solidmask(B):
    return ((B>=1)&(B<=31)&(B!=9)) | (B>=PK0)
def conn(B,Mt):
    for xb in XBF:
        sel=B==xb
        if not sel.any(): continue
        S_=solidmask(B)|sel if xb!=M.XB_FENCE else solidmask(B)|(B==xb)
        mask=np.zeros(B.shape,np.uint8)
        def sh(a,ax,s):  # vizinho em +s no eixo (False fora)
            o=np.zeros_like(a); sl_a=[slice(None)]*3; sl_o=[slice(None)]*3
            if s>0: sl_o[ax]=slice(0,-1); sl_a[ax]=slice(1,None)
            else: sl_o[ax]=slice(1,None); sl_a[ax]=slice(0,-1)
            o[tuple(sl_o)]=a[tuple(sl_a)]; return o
        # eixos: 0=y,1=z,2=x ; bits N(-Z)=1 L(+X)=2 S(+Z)=4 O(-X)=8
        mask|=(sh(S_,1,-1)*1+sh(S_,2,1)*2+sh(S_,1,1)*4+sh(S_,2,-1)*8).astype(np.uint8)
        Mt[sel]=(Mt[sel]&0xF0)|mask[sel]
def conv_chunk(lv):
    B=np.zeros((256,16,16),np.uint16); Mt=np.zeros((256,16,16),np.uint8)
    for s in lv.get('Sections',[]):
        y0=s['Y']*16
        if y0<0 or y0>=256: continue
        b=np.frombuffer(s['Blocks'],dtype=np.uint8).astype(np.int32)
        if 'Add' in s: b=b|(nib(s['Add']).astype(np.int32)<<8)
        d=nib(s['Data']).astype(np.int32)
        b=b.reshape(16,16,16); d=d.reshape(16,16,16)
        dp=(b==175)&((d&8)!=0)   # planta alta: metade de cima herda o tipo da de baixo
        if dp.any():
            dd=np.zeros_like(d); dd[1:]=d[:-1]; d=np.where(dp,(dd&7)|8,d)
        k=b*16+d; B[y0:y0+16]=lutB[k]; Mt[y0:y0+16]=lutM[k]
    conn(B,Mt)
    return B,Mt
def rle16(a):
    a=a.ravel(); ch=np.flatnonzero(np.diff(a))+1; st=np.concatenate(([0],ch)); en=np.concatenate((ch,[a.size]))
    out=[]
    for s,e,v in zip(st.tolist(),en.tolist(),a[st].tolist()):
        n=e-s
        while n>0: r=min(n,65535); out.append((r,v)); n-=r
    return out
def write(path,chs):
    with open(path,'wb') as f:
        f.write(b'BL18'); f.write(struct.pack('<i',len(pal)))
        for n,_ in sorted(pal.items(),key=lambda x:x[1]):
            e=n.encode(); f.write(struct.pack('<H',len(e))+e)
        f.write(struct.pack('<i',len(chs)))
        for cx,cz,B,Mt in chs:
            f.write(struct.pack('<ii',cx,cz))
            r=rle16(B); f.write(struct.pack('<i',len(r)*2)); f.write(np.array([x for p in r for x in p],np.uint16).tobytes())
            m=rle16(Mt); buf=b''.join(struct.pack('<HB',n,v) for n,v in m); f.write(struct.pack('<i',len(buf))+buf)
def run(pat,out):
    chs=[]
    for f in sorted(glob.glob(pat)):
        for c in chunks(f):
            lv=c['Level']; B,Mt=conv_chunk(lv); chs.append((lv['xPos'],lv['zPos'],B,Mt))
    write(out,chs); return len(chs)
if __name__=='__main__':
    dst=sys.argv[1]; os.makedirs(dst,exist_ok=True)
    n1=run('/tmp/m/region/*.mca',dst+'/chunks16.dat'); print('overworld',n1,file=sys.stderr)
    n2=run('/tmp/m/DIM-1/region/*.mca',dst+'/chunks16n.dat'); print('nether',n2,len(pal),file=sys.stderr)
    # paleta e escrita: o nether foi escrito depois; reescreve cabecalho com paleta final nao e necessario (ids ja fixos pelo LUT)
