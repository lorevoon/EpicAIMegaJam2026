import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';OUT=ROOT/'Saved/Act2/StoryRevision10'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);return h.to_tuple()[4].z if h else 1150
# Ray-test real village; candidate bounds independently keep all stations inside the studio.
for name,on in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,name).get_loaded_level(),on,False)

ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.DCStartMark,u.DCClapperboard))]
spec=[([.5,3,5.8],['Armoured monster','Raid orc 1','Raid orc 4','Mr. Smith'],(-5200,-9150,ground(-5200,-9150)+70)),([10.5,13,15],['Romeo','Juliet'],(-6500,-9800,1470)),([16.5,17.8,18.8],['Juliet'],(-4400,-10200,1300)),([19.5,22.5,25.5,28],['Romeo','Armoured monster'],(-4400,-7650,1430)),([30,34,37],['Romeo','Juliet'],(-5900,-7800,1900))]
chosen=[];report=[]
for times,names,wanted in spec:
 samples=[]
 for t in times:
  row=[]
  for who in names:
   a=cast[who];a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);row.extend([c.get_socket_location('head_joint'),c.get_socket_location('pelvis_joint')])
  samples.append(row)
 center=u.Vector()
 for p in samples[len(samples)//2]:center+=p
 center/=len(samples[len(samples)//2]);candidates=[wanted]
 for radius in [1000,1400,1800,2200]:
  for deg in range(0,360,15):
   x=center.x+radius*math.cos(math.radians(deg));y=center.y+radius*math.sin(math.radians(deg))
   for height in [150,350,550]:candidates.append((x,y,ground(x,y)+height))
 candidates.sort(key=lambda p:math.dist(p,wanted));best=None
 for point in candidates:
  if not (-9100<point[0]<-4350 and -10900<point[1]<-7600):continue
  if any(math.dist(point[:2],q[:2])<1300 for q in chosen):continue
  pos=u.Vector(*point);blocked=False
  for row in samples:
   for target in row:
    hit=u.SystemLibrary.line_trace_single(world,pos,target,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
    if hit and (hit.to_tuple()[4]-target).length()>25:blocked=True;break
   if blocked:break
  if not blocked and len(chosen)==0:
   for t,row in zip(times,samples):
    center=u.Vector()
    for head in row[::2]:center+=head
    center/=len(row[::2]);look=math.atan2(center.y-point[1],center.x-point[0]);alpha=max(0,min(1,(t-.5)/5));fov=66-8*(alpha*alpha*(3-2*alpha))
    for who in ['Romeo','Juliet']:
     cast[who].evaluate(t);p=cast[who].get_actor_location();angle=abs((math.degrees(math.atan2(p.y-point[1],p.x-point[0])-look)+180)%360-180)
     if angle<fov/2+9:blocked=True
  if not blocked:best=point;break
 assert best,('No unobstructed camera',names)
 chosen.append(best);report.append({'point':best,'subjects':names,'times':times,'desired':wanted})
report[0]['end']=[-5370,-9020,ground(-5370,-9020)+75]
(OUT/'camera_solution.json').write_text(json.dumps(report,indent=2));print('CLEAR_CAMERA_STATIONS',chosen)
