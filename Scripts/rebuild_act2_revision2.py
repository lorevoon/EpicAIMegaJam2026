"""Author the revised 38-second raid with supplied reactions and continuous in-place locomotion."""
import unreal as u,json,math,warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2/Revision2';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast};assert len(cast)==8
def load(p):
 a=u.load_asset(p);assert a,p;return a
def props(o,**kw):
 for k,v in kw.items():o.set_editor_property(k,v)
def cue(t,a,loop=False,rate=1,offset=0):
 q=u.DCAnimationCue();props(q,start_time=float(t),animation=a,loop=loop,play_rate=float(rate),start_offset=float(offset));return q
S=lambda n:load(BASE+'/Animations/Swordsman/A_Swordsman_'+n)
J=lambda n:load(BASE+'/Animations/Archer/A_Archer_'+n)
O=lambda n:load(BASE+'/Act2/A_Raider_'+n)
V=lambda role,n:load(FOLDER+'/A_'+role+'_'+n)
# Replace routes only for the raiders, plus Juliet's approach to the new holding pose.
for a in actors:
 if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') in [by[n] for n in ['Armoured monster','Juliet','Raid orc 1','Raid orc 2','Raid orc 3','Raid orc 4']]:aes.destroy_actor(a)
groundignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);assert h,(x,y);return h.to_tuple()[4].z+3
def pos(x,y):return u.Vector(x,y,height(x,y))
def route(a,start,points):
 a.set_actor_location(pos(start[0],start[1]),False,False);a.set_actor_rotation(u.Rotator(yaw=start[2]),False);props(a,use_waypoint_height=True,linear_movement=True)
 previous=(0,*start)
 for end in points:
  begin=previous;distance=math.hypot(end[1]-begin[1],end[2]-begin[2]);steps=max(1,math.ceil(distance/55))
  for i in range(1,steps+1):
   f=i/steps;x=begin[1]+(end[1]-begin[1])*f;y=begin[2]+(end[2]-begin[2])*f;yaw=begin[3]+((end[3]-begin[3]+180)%360-180)*f;t=begin[0]+(end[0]-begin[0])*f
   w=aes.spawn_actor_from_class(u.DCWaypoint,pos(x,y),u.Rotator(yaw=yaw));w.set_actor_label(a.get_actor_label()+' v2 @ '+str(round(t,3)));w.set_folder_path('Act2/Blocking');props(w,performer=a,time=float(t));w.set_is_temporarily_hidden_in_editor(True)
  previous=end
orc=by['Armoured monster'];romeo=by['Romeo'];juliet=by['Juliet']
route(orc,(-5750,-10280,90),[(4,-5750,-9600,90),(6.2,-5900,-9085,30),(16.5,-5900,-9085,30),(18.5,-5940,-9300,30),(26,-5940,-9300,30),(28,-5940,-9300,30),(38,-5940,-9300,30)])
for i,(start,end,flank) in enumerate([((-6150,-10270),(-6150,-9500),(-6350,-9400)),((-5450,-10270),(-5450,-9500),(-5200,-9400)),((-6150,-10400),(-6150,-9850),(-6250,-10150)),((-5450,-10400),(-5450,-9850),(-5250,-10150))],1):
 a=by['Raid orc '+str(i)];yaw=math.degrees(math.atan2(flank[1]-end[1],flank[0]-end[0]));route(a,(*start,90),[(6,*end,90),(9,*end,yaw),(16,*flank,yaw),(38,*flank,yaw)])
 props(a,animation_cues=[cue(0,O('Support_Walk'),True,1.15,offset=i*.17),cue(6,O('Support_Idle'),True),cue(9,O('Support_Walk'),True,.85,offset=i*.17),cue(16,O('Support_Idle'),True)])
continuity=json.loads((ROOT/'Saved/Act2/act1_continuity.json').read_text())['cast']
route(juliet,(-5000,-8790.5,-90),[(6,-5000,-8790.5,-90),(9.5,-4920,-8720,210),(13,-4920,-8720,210),(17,-5370,-8950,210),(29,-5370,-8950,210),(31,-5585,-9090,210),(38,-5585,-9090,210)])
juliet.set_actor_location(u.Vector(*continuity['Juliet']['position']),False,False)
props(romeo,animation_cues=[cue(0,S('PromiseLook'),True),cue(10,V('Swordsman','Surprised'),False,.65),cue(13,S('Support_Run'),True),cue(17.5,S('Support_DrawWeapon')),cue(19,S('Support_MeleeSlash')),cue(21,S('Support_Idle'),True),cue(22,S('Support_BlockHit')),cue(24,S('Support_Idle'),True),cue(25,S('Support_MeleeDownward')),cue(26,S('Support_DeathBackward'),False,1.55),cue(29.2,V('Swordsman','LayingHandGesture'),False,.6)])
props(orc,animation_cues=[cue(0,O('Support_Walk'),True,1.65),cue(6.3,O('Support_MeleeDownward')),cue(8.5,O('Support_Idle'),True),cue(16.5,O('Support_Walk'),True),cue(19,O('Support_BlockHit')),cue(21,O('Support_Idle'),True),cue(22,O('Support_MeleeSlash')),cue(24,O('Support_Idle'),True),cue(25,O('Support_MeleeDownward')),cue(26,load(BASE+'/Act2/A_Raid_OrcDeath'),False,load(BASE+'/Act2/A_Raid_OrcDeath').get_play_length()/2)])
props(juliet,animation_cues=[cue(0,J('PromiseLook'),True),cue(10,V('Archer','Surprised'),False,.65),cue(13,J('Support_Run'),True),cue(17,V('Archer','Terrified'),False,1,3),cue(29,J('Support_Run'),True),cue(31,V('Archer','HoldingDown'),False,1.9/6.5)])
# Retain the existing terrain-aligned corpse slopes, synchronizing both falls.
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCWaypoint):
  who=a.get_editor_property('performer');t=a.get_editor_property('time')
  if who==romeo and 26<=t<29:
   if abs(t-26.3)<.02:a.set_editor_property('time',26.)
   if abs(t-28.3)<.02:a.set_editor_property('time',28.)
  if who==orc and t>=28:a.set_editor_property('tilt',-1.578)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCBeat):
  id=str(a.get_editor_property('beat_id'))
  if id=='2.1':a.set_editor_property('camera_spot',a.get_actor_transform().inverse_transform_location(u.Vector(-6800,-10600,1400)))
  if id=='2.4':a.set_editor_property('caption','Romeo strikes; the orc guards. The orc strikes; Romeo guards. Both strike and fall together.')
  if id=='2.5':a.set_editor_property('caption','Romeo reaches out in his final moments while Juliet kneels over him, holding him down and grieving.')
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)]
def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
rootcache={}
def sample(q,t,bones,mesh):
 anim=q.animation;length=anim.get_play_length();p=max(0,q.start_offset+(t-q.start_time)*q.play_rate);p=p%length if q.loop else min(length,p);poses=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,p,False,mesh);name=anim.get_name()
 if 'Support_Walk' in name or 'Support_Run' in name:
  # The path provides all planar travel; preserve only gait height and root rotation.
  tr=poses[0];tr.translation=u.Vector(0,0,tr.translation.z);poses[0]=tr
 if 'LayingHandGesture' in name:
  key=mesh.get_path_name()
  if key not in rootcache:
   dead=S('Support_DeathBackward');d=u.AnimationLibrary.get_bone_poses_for_time(dead,[bones[0]],dead.get_play_length(),False,mesh)[0];first=u.AnimationLibrary.get_bone_poses_for_time(anim,[bones[0]],0,False,mesh)[0];rootcache[key]=d.translation-first.translation+u.Vector(0,0,10)
  tr=poses[0];tr.translation+=rootcache[key];poses[0]=tr
 return poses
manifest={}
for a in cast:
 if a.get_actor_label()=='Mr. Smith':continue
 mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones())];cues=list(a.get_editor_property('animation_cues'));manifest[a.get_actor_label()]=[{'t':q.start_time,'animation':q.animation.get_path_name(),'rate':q.play_rate,'offset':q.start_offset} for q in cues]
 name='A_V2_'+a.get_actor_label().replace(' ','').replace('.','')+'_Performance';path=FOLDER+'/'+name;anim=u.load_asset(path)
 if not anim:
  factory=u.AnimSequenceFactory();factory.target_skeleton=mesh.skeleton;factory.preview_skeletal_mesh=mesh;anim=at.create_asset(name,FOLDER,u.AnimSequence,factory)
 c=anim.get_editor_property('controller');c.open_bracket('Revised 38-second raid',False);c.set_frame_rate(u.FrameRate(30,1),False);c.set_number_of_frames(u.FrameNumber(1140),False);c.remove_all_bone_tracks(False);data={str(b):[[],[],[]] for b in bones}
 for b in bones:c.add_bone_track(b,False)
 for f in range(1141):
  t=f/30;index=max(i for i,q in enumerate(cues) if q.start_time<=t+.00001);q=cues[index];poses=sample(q,t,bones,mesh);transition=.75 if 'HoldingDown' in q.animation.get_name() else .55 if 'LayingHandGesture' in q.animation.get_name() else .18
  if index and t-q.start_time<transition:poses=[blend(x,y,smooth((t-q.start_time)/transition)) for x,y in zip(sample(cues[index-1],t,bones,mesh),poses)]
  for b,tr in zip(bones,poses):
   if str(b)=='eyes_joint' and ((a.get_actor_label()=='Romeo' and t>33) or (a.get_actor_label()=='Armoured monster' and t>28)):tr.scale3d=u.Vector(1,.055,1)
   data[str(b)][0].append(tr.translation);data[str(b)][1].append(tr.rotation);data[str(b)][2].append(tr.scale3d)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);assert eal.save_loaded_asset(anim,False);a.set_editor_property('animation_cues',[cue(0,anim)]);print('REVISION2_BAKED',a.get_actor_label())
for a in cast:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(ROOT/'Saved/Act2/Revision2/cue_manifest.json').write_text(json.dumps(manifest,indent=2));print('ACT2_REVISION2_PERFORMANCE_READY')
