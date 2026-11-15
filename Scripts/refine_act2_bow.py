"""Clear supporting raiders and solve unobstructed, separated camera stations."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';OUT=ROOT/'Saved/Act2/BowRevision6'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};original=json.loads((OUT/'original_routes.json').read_text())
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 result=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);return result.to_tuple()[4].z+3 if result else 1100
for who in ['Raid orc 3','Raid orc 4']:
 a=cast[who]
 for w in list(actors):
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:aes.destroy_actor(w)
 rows=[]
 for f,p in enumerate(original[who]):
  t=f/10;w=max(0,min(1,(t-13)/6));w=w*w*(3-2*w);x=p[0]+650*w;y=p[1];rows.append((t,x,y,ground(x,y),p[3]))
 a.set_actor_location_and_rotation(u.Vector(*rows[0][1:4]),u.Rotator(yaw=rows[0][4]),False,False)
 for i,(t,x,y,z,yaw) in enumerate(rows[1:],1):
  prev=rows[i-1];yaw=math.degrees(math.atan2(y-prev[2],x-prev[1]));w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,z),u.Rotator(yaw=yaw));w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_actor_label(who+f' outer flank {t}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
# Determine bow's local grip rotation from an actual fully drawn wrist pose.
j=cast['Juliet'];j.evaluate(22.35);c=j.get_component_by_class(u.SkeletalMeshComponent);wrist=c.get_socket_transform('L_wrist_joint')
# Bow model long X is vertical, its curved middle points forward along local +Y.
desired=u.Rotator(pitch=90,yaw=-7,roll=0).quaternion();q=wrist.rotation;relative=u.Quat(-q.x,-q.y,-q.z,q.w)*desired
grip=u.Transform(rotation=relative.rotator(),scale=u.Vector(.65,.65,.65));grip.translation=-grip.transform_location(u.Vector(0,-36,0));j.set_editor_property('carry_transform',grip)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Ray-test actual real-village geometry, including camera position clearance.
for name,on in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,name).get_loaded_level(),on,False)
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.DCStartMark,u.DCClapperboard))]
spec=[([0.5,3,5.8],['Armoured monster','Raid orc 1','Raid orc 4'],(-5794,-8212,1865)),([6.5,7.5,9],['Mr. Smith','Armoured monster'],(-6400,-10000,1600)),([11,14,17.5],['Romeo','Juliet'],(-4200,-8000,1450)),([19,22.65,25.2,28],['Romeo','Juliet','Armoured monster','Raid orc 1','Raid orc 2'],(-2800,-9000,1600)),([30,34,37],['Romeo','Juliet'],(-4000,-10600,1400))]
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
