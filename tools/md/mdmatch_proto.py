"""Prototype of mdmatch.py (written for docs/PLAN): pair every Martian Dreams function with the closest
Savage Empire function. Expects mdexe/{GAME,MARTIAN}.EXE, seexe/{GAME,SAVAGE}.EXE and se_src/{game,savage}_modules.txt
(Savage Empire SRC/modules.txt and SRC_S/modules.txt). Needs capstone and rapidfuzz. Not yet wired to target.py."""
import struct,re
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
import sys,pickle
from rapidfuzz.distance import Indel
EXTRA=set()
md=Cs(CS_ARCH_X86,CS_MODE_16)
def load(p):
    d=open(p,'rb').read(); last,pages=struct.unpack_from('<HH',d,2); size=(pages-1)*512+(last or 512); h=struct.unpack_from('<H',d,8)[0]*16
    return d[h:size], d[size:], d
def toks(code,base):
    out=[]
    for i in md.disasm(code,base):
        s=i.mnemonic+' '+i.op_str
        if i.mnemonic.startswith(('j','call','lcall','loop')): s=i.mnemonic+' T'
        s=re.sub(r'\b0x[0-9a-f]{3,}\b','N',s)
        out.append(s)
    return out
def split(img,st,en,label):
    """functions in [st,en): starts at push bp/mov bp,sp"""
    fs=set([st])|set(a for a in EXTRA if st<a<en)
    for m in re.finditer(b'\x55\x8b\xec',img[st:en]): fs.add(st+m.start())
    fs=sorted(fs); out=[]
    for k,a in enumerate(fs):
        b=fs[k+1] if k+1<len(fs) else en
        if b>a: out.append(dict(addr=a,size=b-a,seg=label,toks=toks(img[a:b],a)))
    return out
def md_game():
    img,ov,_=load('mdexe/GAME.EXE')
    sig,ovsize,exeinfo,segnum=struct.unpack_from('<4sIII',ov,0)
    t=exeinfo-0x3400
    ents=[struct.unpack_from('<HHHH',img,t+8*i) for i in range(segnum)]
    F=[]; segs=[]
    for i,(seg,mx,fl,mn) in enumerate(ents):
        if fl==1 and mx and i>0:
            segs.append(('R',seg,mx))
            F+=split(img,seg*16,seg*16+mx,'R%04X'%seg)
        if fl==3:
            st=seg*16
            fo,cs,rs,ne=struct.unpack_from('<IHHH',img,st+4)
            code=ov[16+fo:16+fo+cs]
            base=0x100000+fo
            big=bytearray(base)+code  # sparse ok
            segs.append(('O',seg,cs))
            F+=split(bytes(big),base,base+cs,'O%04X'%seg)
    # resident C0M _TEXT seg 0: game code also? (OSI asm linked into _TEXT) skip lib
    return F,segs,ents
def se_mods(path,img):
    mods=[]
    for l in open(path):
        if l.startswith('#') or not l.strip(): continue
        p=l.split()
        if p[0]=='-' : continue
        fr,at,ln=int(p[0],16),int(p[1],16),int(p[2],16)
        if fr==0: continue
        mods.append((fr*16+at,ln,p[3]))
    return mods
def se_game():
    img,_,_=load('seexe/GAME.EXE'); F=[]
    for st,ln,name in se_mods('se_src/game_modules.txt',img):
        F+=split(img,st,st+ln,name)
    return F
def se_savage():
    img,_,_=load('seexe/SAVAGE.EXE'); F=[]
    for st,ln,name in se_mods('se_src/savage_modules.txt',img):
        F+=split(img,st,st+ln,name)
    return F
def md_martian():
    img,_,d=load('mdexe/MARTIAN.EXE')
    import struct
    nrel=struct.unpack_from('<H',d,6)[0]; ro=struct.unpack_from('<H',d,0x18)[0]
    for i in range(nrel):
        o,s=struct.unpack_from('<HH',d,ro+4*i); a=s*16+o
        if a>=3 and img[a-3]==0x9A: EXTRA.add(struct.unpack_from('<H',img,a)[0]*16+struct.unpack_from('<H',img,a-2)[0])
    dg=0x172A
    return split(img,0x4FE4,dg*16,'M')+split(img,0x242,0xEAE,'M0')
def best(A,B):
    vocab={}
    enc=lambda t:[vocab.setdefault(x,len(vocab)) for x in t]
    for f in A+B: f['e']=enc(f['toks'])
    exact={}
    for j,g in enumerate(B): exact.setdefault(tuple(g['e']),j)
    res=[]
    for f in A:
        k=tuple(f['e'])
        if k in exact: res.append((1.0,exact[k])); continue
        bs,bj=0.0,None
        n=len(f['e'])
        for j,g in enumerate(B):
            m=len(g['e'])
            if min(n,m)/max(n,m,1) < bs: continue  # ratio upper bound 2*min/(n+m) ~
            if 2*min(n,m)/(n+m) <= bs: continue
            r=Indel.normalized_similarity(f['e'],g['e'],score_cutoff=bs)
            if r>bs: bs,bj=r,j
        res.append((bs,bj))
    return res
def report(name,A,B,res):
    bk=[(0.95,1.01,'>=0.95 near-identical'),(0.85,0.95,'0.85-0.95 small changes'),(0.5,0.85,'0.5-0.85 recognisable'),(0,0.5,'<0.5 new')]
    tot=sum(f['size'] for f in A)
    print('==',name,len(A),'functions',tot,'bytes')
    for lo,hi,lab in bk:
        sel=[f for f,(r,j) in zip(A,res) if lo<=r<hi]
        print('  %-25s %4d funcs %7d bytes (%4.1f%%)'%(lab,len(sel),sum(f['size'] for f in sel),100*sum(f['size'] for f in sel)/tot))
if __name__=='__main__':
    MG,segs,ents=md_game(); SG=se_game(); SS=se_savage(); MM=md_martian()
    r1=best(MG,SG+SS); report('MD GAME vs SE GAME+SAVAGE',MG,SG+SS,r1)
    r2=best(MM,SS+SG); report('MARTIAN.EXE vs SAVAGE+SE GAME',MM,SS+SG,r2)
    pickle.dump(dict(MG=MG,SG=SG,SS=SS,MM=MM,r1=r1,r2=r2),open('match.pkl','wb'))
