import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';OUT=ROOT/'Saved/Act2/SmoothRevision7'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
def ground(x,y):return 1150
# Ray-test real village; candidate bounds independently keep all stations inside the studio.
for name,on in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,name).get_loaded_level(),on,False)

ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.DCStartMark,u.DCClapperboard))]
spec=[([0.5,3,5.8],['Armoured monster','Raid orc 1','Raid orc 4'],(-7300,-10200,1950)),([6.5,7.5,9],['Mr. Smith','Armoured monster'],(-6064,-8257,1421)),([11,14,17.5],['Romeo','Juliet'],(-4450,-10100,1450)),([19,22.65,25.2,28],['Romeo','Juliet','Armoured monster','Raid orc 1','Raid orc 2'],(-5936,-9773,1550)),([30,34,37],['Romeo','Juliet'],(-4500,-7800,1430))]
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
  if not (-9100<point[0]<-4400 and -10900<point[1]<-7600):continue
  if any(math.dist(point[:2],q[:2])<1300 for q in chosen):continue
  pos=u.Vector(*point);blocked=False
  for row in samples:
   for target in row:
    hit=u.SystemLibrary.line_trace_single(world,pos,target,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
    if hit and (hit.to_tuple()[4]-target).length()>25:blocked=True;break
   if blocked:break
  if not blocked:best=point;break
 assert best,('No unobstructed camera',names)
 chosen.append(best);report.append({'point':best,'subjects':names,'times':times,'desired':wanted})
(OUT/'camera_solution.json').write_text(json.dumps(report,indent=2));print('CLEAR_CAMERA_STATIONS',chosen)
