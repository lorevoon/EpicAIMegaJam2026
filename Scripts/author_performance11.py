"""Replace requested body motions while preserving current dialogue/face performance."""
import unreal as u,json,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/PerformanceRevision11';O=R/'Saved/PerformanceRevision11';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};report={};tracks={};clips={}
for who,role,sex,intervals in [('Juliet','Archer','Female',[(4,5.6),(7,11.6)]),('Romeo','Swordsman','Male',[(7,11.6)])]:
 a=cast[who];source=u.load_asset(B+'/Animations/Act1Dialogue/A_Act1_'+who+'_Speaking');mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];walk=u.load_asset(F+'/A_'+role+'_'+sex+'Walk');kiss=u.load_asset(F+'/A_'+role+'_'+sex+'Kiss');data={b:[[],[],[]] for b in bones}
 def pose(anim,t):return list(u.AnimationLibrary.get_bone_poses_for_time(anim,bones,max(0,min(t,anim.get_play_length())),False,mesh))
 for f in range(601):
  t=f/30;original=pose(source,t);p=original;w=0
  for start,end in intervals:
   if start<=t<=end+.35:
    phase=(t-start)%walk.get_play_length();p=pose(walk,phase)
    if phase>walk.get_play_length()-.12:p=[blend(x,y,smooth((phase-walk.get_play_length()+.12)/.12)) for x,y in zip(p,pose(walk,0))]
    w=smooth((t-start)/.3)*(1-smooth((t-end)/.35));break
  if t>=16:
   # Use the pair's approach and sustained embrace, ending before their release.
   p=pose(kiss,t-16);w=smooth((t-16)/.35)
  if w:
   # Authored route owns travel; retain vertical weight shifts in the new motion.
   p[0].translation=u.Vector(original[0].translation.x,original[0].translation.y,p[0].translation.z)
   p=[blend(x,y,w) for x,y in zip(original,p)]
   for i,b in enumerate(bones):
    if b in ['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint']:p[i]=original[i]
  if who=='Romeo' and t<2.4:
   legs=pose(walk,t%walk.get_play_length());weight=1-smooth((t-2.1)/.3)
   for i,b in enumerate(bones):
    if b.startswith(('L_thigh','R_thigh','L_knee','R_knee','L_ankle','R_ankle','L_ball','R_ball','L_toe','R_toe')):p[i]=blend(p[i],legs[i],weight)
  for b,tr in zip(bones,p):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Act1_'+who;anim=u.load_asset(dest) or e.duplicate_asset(source.get_path_name(),dest);c=anim.get_editor_property('controller');c.open_bracket('Supplied walk and paired kiss',False)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);assert e.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);tracks[who]=data;clips[who]=anim;report[who]={'walk_intervals':intervals,'kiss_interval':[16,20],'animation':dest}
report['contact']=[]
for t in [15.5,16,16.5,17,17.5,18,19,20]:
 for who in ['Romeo','Juliet']:cast[who].evaluate(t)
 report['contact'].append({'t':t,'actors':{who:{b:list(cast[who].get_component_by_class(u.SkeletalMeshComponent).get_socket_location(b).to_tuple()) for b in ['mouth_joint','head_joint','L_wrist_joint','R_wrist_joint','L_ankle_joint','R_ankle_joint']} for who in ['Romeo','Juliet']}})
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise')
assert les.load_level(B+'/Levels/L_DC_Act2_BrokenPromise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};j=cast['Juliet'];j.evaluate(17.75);v=cast['Raid orc 1'];v.evaluate(18.1);wrist=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_transform('L_wrist_joint');hit=v.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');aim=u.MathLibrary.find_look_at_rotation(wrist.translation,hit);q=wrist.rotation
# FBX geometry: wood grip at (0,0,0), string centre at (0,-39,0).
# Local +Y points toward the target so the string (-Y) faces the archer.
desired=u.Rotator(pitch=90,yaw=aim.yaw-90).quaternion();grip=u.Transform(rotation=(u.Quat(-q.x,-q.y,-q.z,q.w)*desired).rotator(),scale=u.Vector(.65,.65,.65));j.set_editor_property('carry_transform',grip)
report['bow']=[]
for t,target in [(17.75,'Raid orc 1'),(20.15,'Raid orc 2')]:
 j.evaluate(t);v=cast[target];v.evaluate(t);c=j.get_component_by_class(u.SkeletalMeshComponent);prop=next(x for x in j.get_components_by_class(u.StaticMeshComponent) if x.get_name()=='Prop');tf=prop.get_world_transform();wood=tf.transform_location(u.Vector());string=tf.transform_location(u.Vector(0,-39,0));hand=c.get_socket_location('L_wrist_joint');direction=(v.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')-wood).normal();projection=u.MathLibrary.dot_vector_vector(string-wood,direction);assert (wood-hand).length()<.01;assert projection< -10,(t,projection);report['bow'].append({'t':t,'grip_error':(wood-hand).length(),'string_toward_target':projection})
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act2_BrokenPromise');(O/'authoring.json').write_text(json.dumps(report,indent=2));print('PERFORMANCE11_AUTHORED',report['bow'])
