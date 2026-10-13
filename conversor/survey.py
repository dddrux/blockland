import numpy as np, glob, pickle, sys
from nbt import *
def nib(a):
    a=np.frombuffer(a,dtype=np.uint8); o=np.empty(4096,np.uint8); o[0::2]=a&15; o[1::2]=a>>4; return o
res={}
for dim,pat in (('o','/tmp/m/region/*.mca'),('n','/tmp/m/DIM-1/region/*.mca')):
    cnt=np.zeros(65536,np.int64); nch=0
    for f in sorted(glob.glob(pat)):
        for c in chunks(f):
            lv=c['Level']; nch+=1
            for s in lv.get('Sections',[]):
                b=np.frombuffer(s['Blocks'],dtype=np.uint8).astype(np.int32)
                if 'Add' in s: b=b|(nib(s['Add']).astype(np.int32)<<8)
                d=nib(s['Data']).astype(np.int32)
                cnt+=np.bincount(b*16+d,minlength=65536)
    res[dim]=(cnt,nch); print(dim,nch,file=sys.stderr)
pickle.dump(res,open('survey.pkl','wb'))
