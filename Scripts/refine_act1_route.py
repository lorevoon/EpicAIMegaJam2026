"""Ground the relocated blocking in a headless editor, then verify rewind."""
import unreal as u, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
LEVEL='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert les.load_level(LEVEL)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
assert not any('DC_Studio' in [str(t) for t in a.tags] for a in actors), 'Outdoor relocation is obsolete for the studio set.'
for existing in actors:
    if 'DC_Act1_GroundSample' in [str(t) for t in existing.tags]:aes.destroy_actor(existing)
actors=list(aes.get_all_level_actors())
byname={a.get_name():a for a in actors}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
    h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,5000),u.Vector(x,y,-2000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
    assert h[0],(x,y)
    return h[4].z+3
def place(a,x,y,yaw,z=None):
    a.set_actor_location_and_rotation(u.Vector(x,y,ground(x,y) if z is None else z),u.Rotator(yaw=yaw),False,False)
def retime(t):
    return t if t<=14 else 14+(t-14)*41/9 if t<=23 else t+32 if t<40 else 70
# Opening on the path immediately in front of the supplied mill actor origin.
M=(-6100,-8550)
K=(-1503.234575,-8022.357493)
route=[M,(-5500,-8300),(-4900,-8280),(-4300,-8300),(-3700,-8250),(-3100,-8200),(-2500,-8200),(-2000,-8100),K]
lengths=[0.]
for a,b in zip(route,route[1:]):lengths.append(lengths[-1]+math.dist(a,b))
def along(t,female):
    d=max(0,min(1,(t-14)/9))*lengths[-1]
    i=next((i for i in range(len(route)-1) if d<=lengths[i+1]),len(route)-2)
    a,b=route[i:i+2];f=(d-lengths[i])/(lengths[i+1]-lengths[i])
    x=a[0]+(b[0]-a[0])*f;y=a[1]+(b[1]-a[1])*f
    yaw=math.degrees(math.atan2(b[1]-a[1],b[0]-a[0]))
    if female:y+=80
    return x,y,yaw
original=json.loads((ROOT/'Scripts/data/act1_pre_relocation.json').read_text())['actors']
for row in original:
    a=byname[row['actor']['refPath'].split('.')[-1]];label=row['label'];p=row['transform']['location']
    if label.startswith('Lookout '):continue
    x=M[0]+p['y']-1900;y=M[1]-(p['x']-4100);yaw=row['transform']['rotation']['yaw']-90
    z=None
    if ' @ ' in label:
        t=float(label.rsplit(' @ ',1)[1])
        a.set_editor_property('time',retime(t))
        female=label.startswith('Heroine -')
        if label.startswith('Hero -') or female:
            if t<14:
                if female:
                    f=max(0,min(1,(t-10)/2.6));x=M[0]+400*(1-f);y=M[1]+100-20*f;yaw=180
                else:
                    f=min(1,t/4.8);x=M[0]+150*(1-f);y=M[1]-150*(1-f);yaw=135 if t<4.8 else 0
            elif 14<=t<23:x,y,yaw=along(t,female)
            elif t>=23:
                x=K[0]+((17 if t>=34 else 40)*(1 if female else -1));y=K[1]+(3 if female and t>=34 else 0);yaw=180 if female else 0
                z=ground(*K)
    elif label.startswith('1.3') or label.startswith('1.4'):x,y=K
    elif label.startswith('Hero -'):x,y,yaw=M[0]+150,M[1]-150,135
    elif label.startswith('Heroine -'):x,y,yaw=M[0]+400,M[1]+100,180
    elif label=='Camera operator start':x,y,yaw=-5700,-8900,140;z=ground(x,y)+100
    elif label=='Stage 1 clapperboard':x,y=-5800,-8800
    elif 'bench' in label.lower() or label=='Bench leg':z=ground(M[0]+110,M[1])+p['z']-1609.0943813323975
    elif 'bounce' in label:z=ground(x,y)+420
    if label.startswith('Crew -'):
        x=-5400+(p['y']-2200)*.4;y=-8400-(p['x']-4100)*.4;z=None
    place(a,x,y,yaw,z)
hero=byname['DCPerformer_20'];female=byname['DCPerformer_22']
hero.set_editor_property('set_down_transform',u.Transform(location=u.Vector(M[0]+75,M[1],ground(M[0]+110,M[1])+74.5),rotation=u.Rotator(yaw=-90),scale=u.Vector(.35,.35,.35)))
# Dense, terrain-conforming samples keep the feet above the ground between keys.
for performer,is_female in [(hero,False),(female,True)]:
    for i in range(1,164):
        time=14+i*.25;oldtime=14+(time-14)*9/41
        x,y,yaw=along(oldtime,is_female)
        a=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,ground(x,y)),u.Rotator(yaw=yaw))
        a.set_actor_label(performer.get_actor_label()+' ground sample '+str(time));a.tags=['DC_Act1','DC_Act1_GroundSample'];a.set_folder_path('Act1/Blocking')
        a.set_editor_property('performer',performer);a.set_editor_property('time',time)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
kiss_z=ground(*K)
# Fresh load ensures Evaluate builds its cache from the new authored start poses.
les.load_level('/Engine/Maps/Entry');les.load_level(LEVEL)
cast=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)]
def sample(t):
    out={}
    for a in cast:
        a.evaluate(t);v=a.get_actor_location();out[a.get_actor_label()]=[v.x,v.y,v.z]
    return out
initial=sample(0);samples={str(t):sample(t) for t in [8,15,25,40,54,56,66,70]};rewind=sample(0)
error=max(abs(x-y) for name in initial for x,y in zip(initial[name],rewind[name]))
assert error<.01,error
report={'level':LEVEL,'mill_actor':[-6627.616627,-8840.595552,1305.746809],'opening_mark':M,'kiss_mark':[K[0],K[1],kiss_z],'path_cm':lengths[-1],'duration':70,'samples':samples,'rewind_error_cm':error,'animation':'existing walk retained; Mixamo deferred'}
(ROOT/'Saved/Stage1Audit/relocation_validation.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_RELOCATION_VALIDATED')
