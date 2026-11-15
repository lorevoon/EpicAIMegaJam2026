"""Back-to-back vigilance, shoot-run-shoot, and return to Romeo."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/MobileRevision8';OUT=ROOT/'Saved/Act2/MobileRevision8';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')}
def load(p):
 a=u.load_asset(p);assert a,p;return a
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-x for x in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
def mix(a,b,w):return [x+(y-x)*w for x,y in zip(a,b)]
def yawblend(a,b,w):return a+((b-a+180)%360-180)*w

j=cast['Juliet'];source=load(BASE+'/Act2/SmoothRevision7/A_Grounded_Juliet');mesh=j.get_editor_property('character_mesh');comp=j.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];face={i for i,b in enumerate(bones) if any(s in b.lower() for s in ['mouth','eye','brow','jaw','lip'])}
clips={k:load(F+'/A_Archer_'+k) for k in ['Crouch','TurnLeft','TurnRight','RunForward']}
clips['Draw']=load(BASE+'/Act2/BowRevision6/A_Archer_DrawArrow');clips['Release']=load(BASE+'/Animations/Archer/A_Archer_Support_BowRelease')
def pose(anim,t):return list(u.AnimationLibrary.get_bone_poses_for_time(anim,bones,max(0,min(t,anim.get_play_length())),False,mesh))
leftend=pose(clips['TurnLeft'],clips['TurnLeft'].get_play_length())[0].rotation;rightstart=pose(clips['TurnRight'],0)[0].rotation
turnoffset=leftend*u.Quat(-rightstart.x,-rightstart.y,-rightstart.z,rightstart.w)
# All speech and the original kneeling performance keep their original scene times.
timeline=[(0,'SOURCE',0,13.2,0),(13.2,'Crouch',0,1,.25),(14.5,'TurnLeft',0,1,.25),(15.65,'TurnRight',0,1,.25),(16.8,'Crouch',1,1,.3),(18.4,'Crouch',1,0,.3),(19.4,'SOURCE',19.4,22.65,.3),(22.65,'RunForward',0,None,.18),(23.65,'Draw',0,1,.18),(24.9,'Release',0,.6,.1),(25.4,'SOURCE',25.4,26.5,.25),(26.5,'RunForward',0,None,.25),(28.3,'SOURCE',28.3,38,.3)]
def sample(index,t):
 start,key,lo,hi,transition=timeline[index];end=timeline[index+1][0] if index+1<len(timeline) else 38
 if key=='SOURCE':return pose(source,lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
 anim=clips[key];length=anim.get_play_length();phase=(t-start)%length if hi is None else length*(lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
 p=pose(anim,phase);p[0].translation=u.Vector(0,0,p[0].translation.z)
 # A right turn starts from the left turn's terminal facing, then returns to guard.
 if key=='TurnRight':p[0].rotation=turnoffset*p[0].rotation
 return p
data={b:[[],[],[]] for b in bones}
for f in range(1141):
 t=f/30;index=max(i for i,q in enumerate(timeline) if q[0]<=t+.00001);p=sample(index,t);boundary=timeline[index][0];transition=timeline[index][4]
 if index and t-boundary<transition:p=[blend(a,b,smooth((t-boundary)/transition)) for a,b in zip(sample(index-1,boundary),p)]
 original=pose(source,t)
 for i in face:p[i]=original[i]
 for b,tr in zip(bones,p):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
dest=F+'/A_Mobile_Juliet';anim=u.load_asset(dest) or eal.duplicate_asset(source.get_path_name(),dest);c=anim.get_editor_property('controller');c.open_bracket('Vigilant crouch and two running passages',False)
for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
c.close_bracket(False);assert eal.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;j.set_editor_property('animation_cues',[cue])
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True);assert h;return h.to_tuple()[4].z+3
J=(-4890.,-8734.5);ADV=(-4790.,-9110.);TARGET=(-4430.,-9380.)
rows=[]
for f in range(381):
 t=f/10;x,y=J
 if 22.65<t<23.65:x,y=mix(J,ADV,(t-22.65))
 elif 23.65<=t<26.5:x,y=ADV
 elif 26.5<=t<28.3:x,y=mix(ADV,J,(t-26.5)/1.8)
 if t<11.5:yaw=-150
 elif t<13.2:yaw=yawblend(-150,13,smooth((t-11.5)/1.7))
 elif t<17:yaw=13
 elif t<18.4:yaw=yawblend(13,30,smooth((t-17)/1.4))
 elif t<20.4:yaw=yawblend(30,-96,smooth((t-18.4)/2))
 elif t<22.5:yaw=-96
 elif t<23.4:yaw=yawblend(-96,-75,smooth((t-22.5)/.3))
 elif t<23.85:yaw=yawblend(-75,-37,smooth((t-23.4)/.45))
 elif t<25.8:yaw=-37
 elif t<26.5:yaw=yawblend(-37,105,smooth((t-25.8)/.7))
 elif t<28.1:yaw=105
 else:yaw=yawblend(105,30,smooth((t-28.1)/.5))
 rows.append([t,x,y,ground(x,y),yaw])
def route(a,rows):
 for w in list(aes.get_all_level_actors()):
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:aes.destroy_actor(w)
 a.set_actor_location_and_rotation(u.Vector(*rows[0][1:4]),u.Rotator(yaw=rows[0][4]),False,False);a.set_editor_property('linear_movement',True);a.set_editor_property('use_waypoint_height',True)
 for t,x,y,z,yaw in rows[1:]:
  w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,z),u.Rotator(yaw=yaw));w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_actor_label(a.get_actor_label()+f' mobile @ {t:.1f}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
route(j,rows)
# Relocate the second swordsman to meet Juliet's new firing line.
a=cast['Raid orc 2'];targetrows=[];cache=OUT/'target_baseline.json'
if cache.exists():targetbase=json.loads(cache.read_text())
else:
 targetbase=[]
 for f in range(381):
  a.evaluate(f/10);targetbase.append([*a.get_actor_location().to_tuple(),a.get_actor_rotation().yaw])
 cache.write_text(json.dumps(targetbase))
for f in range(381):
 t=f/10;p=u.Vector(*targetbase[f][:3]);w=smooth((t-6)/9);x=p.x+250*w;y=p.y-160*w;yaw=targetbase[f][3] if t<15 else math.degrees(math.atan2(ADV[1]-y,ADV[0]-x));targetrows.append([t,x,y,ground(x,y),yaw])
route(a,targetrows)
for who,a in cast.items():
 if who=='Juliet':row=rows[0];a.set_actor_location_and_rotation(u.Vector(*row[1:4]),u.Rotator(yaw=row[4]),False,False)
 elif who=='Raid orc 2':row=targetrows[0];a.set_actor_location_and_rotation(u.Vector(*row[1:4]),u.Rotator(yaw=row[4]),False,False)
 else:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Reload to discard cached actor routes before fitting the bow and rebuilding arrows.
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)};j=cast['Juliet'];j.evaluate(22.3);v=cast['Raid orc 1'];v.evaluate(22.65);wrist=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_transform('L_wrist_joint');hit=v.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');aim=u.MathLibrary.find_look_at_rotation(wrist.translation,hit);q=wrist.rotation;desired=u.Rotator(pitch=90,yaw=aim.yaw+90).quaternion();grip=u.Transform(rotation=(u.Quat(-q.x,-q.y,-q.z,q.w)*desired).rotator(),scale=u.Vector(.65,.65,.65));grip.translation=-grip.transform_location(u.Vector(0,-36,0));j.set_editor_property('carry_transform',grip)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'staging.json').write_text(json.dumps({'timeline':timeline,'juliet':rows,'second_target':targetrows},indent=2));print('MOBILE8_STAGED')
