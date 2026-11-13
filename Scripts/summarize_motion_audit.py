import json,math
from pathlib import Path
data=json.loads((Path(__file__).resolve().parents[1]/'Saved/FilmSetAudit/motion_before.json').read_text())
for n,v in data.items():
    print(n,v['cues'],'linear',v['linear'])
    for field in ['p','pelvis','head']:
        d=sorted([(math.dist(b[field],c[field]),(i+1)/60) for i,(b,c) in enumerate(zip(v['samples'],v['samples'][1:]))],reverse=True)
        print(field,d[:8])
    for path,ss in v['local'].items():
        for bone in ['rootSkeleton','pelvis_joint']:
            print(bone,'range',[[min(s[bone]['p'][j] for s in ss),max(s[bone]['p'][j] for s in ss)] for j in range(3)])
    print('opening keys',[(k['time'],k['pos'],k['yaw']) for k in v['keys'] if k['time']<=7.01])
