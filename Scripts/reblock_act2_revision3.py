"""Continuous raid marches, collision-clear death staging, and six authored cameras."""
import unreal as u,json,math,heapq,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';OUT=ROOT/'Saved/Act2/Revision3';OUT.mkdir(parents=True,exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
backup=OUT/'L_DC_Act2_BrokenPromise.before.umap'
if not backup.exists():shutil.copy2(ROOT/'Content/DirectorsCut/Levels/L_DC_Act2_BrokenPromise.umap',backup)
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.LandscapeProxy))]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-2000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True)
 return h.to_tuple()[4].z if h else None
def clear(x,y):
 z=ground(x,y)
 return z is not None and not u.SystemLibrary.sphere_trace_single(world,u.Vector(x,y,z+85),u.Vector(x,y,z+115),48,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
# Keep the familiar bench nearby, but entirely outside Smith's swept falling body.
for a in actors:
 if a.get_actor_label()=='Mr. Smith - familiar bench':a.set_actor_location(u.Vector(-6150,-8800,ground(-6150,-8800)+46),False,False)
 if a.get_actor_label()=='Bench leg':
  old=a.get_actor_location();x=-6210 if old.x<(-6150 if old.x<-6000 else -5750) else -6090;a.set_actor_location(u.Vector(x,-8800,ground(-6150,-8800)+22),False,False)
# Find corridors clear in BOTH the film set and real village.
blocked=set();heights={};step=100
for real in [False,True]:
 for n,v in [('L_DC_Act2_Set',not real),('L_DC_Act2_Real',real)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,n).get_loaded_level(),v,False)
 for x in range(-6400,-4399,step):
  for y in range(-10700,-7699,step):
   if not clear(x,y):blocked.add((x,y))
   else:heights[(x,y)]=ground(x,y)
free=set(heights)-blocked
def route(start,goal):
 s=min(free,key=lambda p:math.dist(p,start));g=min(free,key=lambda p:math.dist(p,goal));queue=[(0,s)];cost={s:0};prev={}
 while queue:
  _,p=heapq.heappop(queue)
  if p==g:break
  for dx,dy in [(100,0),(-100,0),(0,100),(0,-100),(100,100),(100,-100),(-100,100),(-100,-100)]:
   q=(p[0]+dx,p[1]+dy)
   if q not in free or (dx and dy and ((p[0]+dx,p[1]) not in free or (p[0],p[1]+dy) not in free)):continue
   # Prefer an eastern corridor, safely behind the foreground performance.
   danger=180 if math.dist(q,(-5600,-9000))<450 else 0
   c=cost[p]+math.hypot(dx,dy)+danger
   if c<cost.get(q,1e20):cost[q]=c;prev[q]=p;heapq.heappush(queue,(c+math.dist(q,g),q))
 if g not in cost:g=min(cost,key=lambda p:math.dist(p,goal))
 assert cost[g]>1000,(start,goal,g,cost[g])
 path=[g]
 while path[-1]!=s:path.append(prev[path[-1]])
 return [start]+list(reversed(path))[1:]
paths={}
for i,goal in enumerate([(-4500,-7800),(-4800,-7800),(-5100,-7800),(-5400,-7800)],1):
 a=cast['Raid orc '+str(i)];p=a.get_actor_location();raw=route((p.x,p.y),goal)
 # Corner cutting produces gentle bends; retain only collision-clear rounded corners.
 curve=raw
 for iteration in range(2):
  out=[curve[0]]
  for p,q in zip(curve,curve[1:]):out.extend([(p[0]*.75+q[0]*.25,p[1]*.75+q[1]*.25),(p[0]*.25+q[0]*.75,p[1]*.25+q[1]*.75)])
  out.append(curve[-1]);curve=out
 lengths=[0.]
 for p,q in zip(curve,curve[1:]):lengths.append(lengths[-1]+math.dist(p,q))
 total=lengths[-1];samples=[];k=0
 for f in range(115):
  distance=total*f/114
  while k+1<len(lengths)-1 and lengths[k+1]<distance:k+=1
  p,q=curve[k:k+2];alpha=(distance-lengths[k])/max(.0001,lengths[k+1]-lengths[k]);x=p[0]+(q[0]-p[0])*alpha;y=p[1]+(q[1]-p[1])*alpha
  samples.append((f/3,x,y,math.degrees(math.atan2(q[1]-p[1],q[0]-p[0]))))
 paths[a.get_actor_label()]={'speed_cm_s':total/38,'samples':samples}
# Validate final rounded routes against both environments before saving edits.
hits=[]
for real in [False,True]:
 for n,v in [('L_DC_Act2_Set',not real),('L_DC_Act2_Real',real)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,n).get_loaded_level(),v,False)
 for who,row in paths.items():
  for t,x,y,yaw in row['samples']:
   if not clear(x,y):hits.append([real,who,t,x,y])
(OUT/'march_routes.json').write_text(json.dumps({'routes':paths,'collisions':hits},indent=2));assert not hits,hits[:10]
for a in actors:
 if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') in [cast[n] for n in paths]:aes.destroy_actor(a)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act2_BrokenPromise')
walk=u.load_asset(BASE+'/Act2/A_Raider_Support_Walk');assert walk
for who,row in paths.items():
 a=cast[who];samples=row['samples'];a.set_editor_property('linear_movement',True);a.set_editor_property('use_waypoint_height',True)
 for t,x,y,yaw in samples:
  pos=u.Vector(x,y,ground(x,y)+3)
  if t==0:a.set_actor_location_and_rotation(pos,u.Rotator(yaw=yaw),False,False)
  else:
   w=aes.spawn_actor_from_class(u.DCWaypoint,pos,u.Rotator(yaw=yaw));w.set_editor_property('performer',a);w.set_editor_property('time',float(t));w.set_actor_label(who+' uninterrupted march @ '+str(round(t,2)));w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
 mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones())];path=BASE+'/Act2/Revision3/A_'+who.replace(' ','')+'_March';anim=eal.duplicate_asset(walk.get_path_name(),path) if not eal.does_asset_exist(path) else u.load_asset(path)
 if abs(anim.get_play_length()-38)<.001:
  cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False;a.set_editor_property('animation_cues',[cue]);continue
 c=anim.get_editor_property('controller');c.open_bracket('Continuous in-place march',False);c.set_frame_rate(u.FrameRate(30,1),False);c.set_number_of_frames(u.FrameNumber(1140),False);data={str(b):[[],[],[]] for b in bones}
 for f in range(1141):
  pose=u.AnimationLibrary.get_bone_poses_for_time(walk,bones,(f/30*max(.75,row['speed_cm_s']/100))%walk.get_play_length(),False,mesh)
  for b,tr in zip(bones,pose):
   if b==bones[0]:tr.translation=u.Vector(0,0,tr.translation.z)
   data[str(b)][0].append(tr.translation);data[str(b)][1].append(tr.rotation);data[str(b)][2].append(tr.scale3d)
 for b,v in data.items():assert c.set_bone_track_keys(b,*v,False)
 c.close_bracket(False);assert eal.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False;a.set_editor_property('animation_cues',[cue])
# Reuse the existing cameras; add the sixth for the combat reverse angle.
shots=[(0,'2.1',(-5794,-8212,1865),78),(6,'2.2',(-6064,-8257,1421),62),(10,'2.3',(-4550,-9250,1390),62),(18,'2.4',(-5500,-9680,1480),60),(23.5,'2.4',(-6400,-9800,1480),60),(29,'2.5',(-5750,-10100,1231),57)]
for i,(start,beat,point,fov) in enumerate(shots,1):
 cam=next((a for a in actors if a.get_actor_label()=='Raid cut camera '+str(i)),None) or aes.spawn_actor_from_class(u.CameraActor,u.Vector(*point))
 cam.set_actor_label('Raid cut camera '+str(i));cam.set_actor_location(u.Vector(*point),False,False);cam.set_folder_path('Act2/Reference cameras');cam.get_component_by_class(u.CameraComponent).set_field_of_view(fov);tags=['DCReferenceShot','DCStart='+str(start),'DCBeat='+beat,'DCFOV='+str(fov)]
 if i==2:tags+=['DCSlowStart=7.3','DCSlowEnd=8.3','DCSlowRate=0.8']
 if i==5:tags+=['DCSlowStart=25','DCSlowEnd=27','DCSlowRate=0.65']
 cam.tags=tags
for a in actors:
 if isinstance(a,u.DCBeat) and str(a.get_editor_property('beat_id'))=='2.5':a.set_editor_property('display_name','A final goodbye');a.set_editor_property('caption','Romeo dies in Juliet\'s arms as she sobs.');a.set_actor_label('2.5 A final goodbye')
for n,v in [('L_DC_Act2_Set',True),('L_DC_Act2_Real',False)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,n).get_loaded_level(),v,False)
# Reload before evaluating because performer waypoint caches may have been populated by construction.
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL);assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCPerformer):a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),LEVEL)
print('ACT2_REVISION3_BLOCKING_READY',[(n,r['speed_cm_s']) for n,r in paths.items()])
