import os,re,subprocess,sys,tempfile,shutil,collections,json
R=sys.argv[1]
def run(b,mode,txt):
    d=tempfile.mkdtemp(dir=R+'/corr'); p=d+'/a.cir'; open(p,'w').write(txt)
    env=dict(os.environ,HOME=R+{'hsa':'/h-hsa','plain':'/h-plain'}[mode],SPICE_SCRIPTS=R+'/home')
    r=subprocess.run([R+'/ngspice-'+b,'-b',p],cwd=d,env=env,capture_output=True,text=True,timeout=60); shutil.rmtree(d)
    o=r.stdout+r.stderr
    m=re.search(r'model\s+(x1:\S+)',o); i=re.search(r'i\(vd\)\s*=\s*(\S+)',o)
    return i.group(1) if i else None
def deck(models,x,extra=''):
    return f"t\n{extra}.subckt dut d g s b l=1 w=1 nf=1 m=1\nmn d g s b card l={{l}} w={{w}} nf={{nf}} m={{m}}\n{models}\n.ends dut\nvg g 0 0.9\nvd d 0 0.9\nx1 d g 0 0 dut {x}\n.control\nop\nprint i(vd)\nshowmod m.x1.mn : model\n.endc\n.end\n"
lev='level=54 version=4.5'
tab=collections.Counter(); ex={}
def rec(tag,sc,mode,base,fix,desc):
    k=(tag,sc or '-',mode,'load' if base else 'stop','load' if fix else 'stop'); tab[k]+=1; ex.setdefault(k,desc)
    if base and fix and base!=fix: tab[(tag,sc or '-',mode,'DIFF')]+=1; ex.setdefault((tag,sc or '-',mode,'DIFF'),desc+f' {base} {fix}')
EW=[0.42,0.65,1.1,3,3.3,7]
for e in EW:
  for nf in range(1,41):
    wv=e*nf
    for sc,x,xt in ((None,f"l=0.5u w={wv:.12g}u nf={nf}",''),('1u',f"l=0.5 w={wv:.12g} nf={nf}",'.option scale=1u\n')):
      for tag,m in (('single-wtop',f".model card nmos {lev} lmin=1.5e-7 lmax=2.02e-5 wmin=1e-7 wmax={e}e-6"),
                    ('single-wbot',f".model card nmos {lev} lmin=1.5e-7 lmax=2.02e-5 wmin={e}e-6 wmax=1.01e-3"),
                    ('binned-wtop',f".model card.1 nmos {lev} lmin=1.5e-7 lmax=2.02e-5 wmin=1e-7 wmax={e}e-6"),
                    ('binned-wbot',f".model card.1 nmos {lev} lmin=1.5e-7 lmax=2.02e-5 wmin={e}e-6 wmax=1.01e-3")):
        for mode in ('hsa','plain'):
          b=run('base',mode,deck(m,x,xt)); f=run('fix',mode,deck(m,x,xt))
          rec(tag,sc,mode,b,f,x+' e=%g'%e)
EL=[0.15,0.35,1,2.2,3.3,7.7,20.2]
for e in EL:
  for sc,x,xt in ((None,f"l={e}u w=2u",''),('1u',f"l={e} w=2",'.option scale=1u\n')):
    for tag,m in (('single-ltop',f".model card nmos {lev} lmin=1e-7 lmax={e}e-6 wmin=1e-7 wmax=1e-3"),
                  ('single-lbot',f".model card nmos {lev} lmin={e}e-6 lmax=1e-3 wmin=1e-7 wmax=1e-3")):
      for mode in ('hsa','plain'):
        b=run('base',mode,deck(m,x,xt)); f=run('fix',mode,deck(m,x,xt))
        rec(tag,sc,mode,b,f,x)
for k in sorted(tab,key=str): print(k,tab[k],ex.get(k))
