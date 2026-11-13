import struct, zlib, gzip, io
def rd(b, p, t):
    if t==1: return b[p], p+1
    if t==2: return struct.unpack_from('>h',b,p)[0], p+2
    if t==3: return struct.unpack_from('>i',b,p)[0], p+4
    if t==4: return struct.unpack_from('>q',b,p)[0], p+8
    if t==5: return struct.unpack_from('>f',b,p)[0], p+4
    if t==6: return struct.unpack_from('>d',b,p)[0], p+8
    if t==7:
        n=struct.unpack_from('>i',b,p)[0]; p+=4; return bytes(b[p:p+n]), p+n
    if t==8:
        n=struct.unpack_from('>H',b,p)[0]; p+=2; return bytes(b[p:p+n]).decode('utf8','replace'), p+n
    if t==9:
        et=b[p]; n=struct.unpack_from('>i',b,p+1)[0]; p+=5; l=[]
        for _ in range(n):
            v,p=rd(b,p,et); l.append(v)
        return l,p
    if t==10:
        d={}
        while True:
            tt=b[p]; p+=1
            if tt==0: return d,p
            n=struct.unpack_from('>H',b,p)[0]; p+=2; k=bytes(b[p:p+n]).decode('utf8','replace'); p+=n
            d[k],p=rd(b,p,tt)
    if t==11:
        n=struct.unpack_from('>i',b,p)[0]; p+=4; return list(struct.unpack_from('>%di'%n,b,p)),p+4*n
    if t==12:
        n=struct.unpack_from('>i',b,p)[0]; p+=4; return list(struct.unpack_from('>%dq'%n,b,p)),p+8*n
def parse(b):
    t=b[0]; n=struct.unpack_from('>H',b,1)[0]; v,_=rd(b,3+n,t); return v
def level(path): return parse(gzip.open(path).read())
def chunks(path):
    d=open(path,'rb').read()
    for i in range(1024):
        o=struct.unpack_from('>I',d,i*4)[0]
        if not o: continue
        off=(o>>8)*4096; ln=struct.unpack_from('>I',d,off)[0]; c=d[off+4]
        raw=d[off+5:off+4+ln]
        raw=zlib.decompress(raw) if c==2 else gzip.decompress(raw)
        yield parse(raw)
