"""Ease the paired approach into a quiet held kiss, with Juliet on Male Kiss."""
import unreal as u,json,math,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/KissRevision13';O=R/'Saved/KissRevision13';O.mkdir(parents=True,exist_ok=True);e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
if not (O/'before.umap').exists():shutil.copy2(R/'Content/DirectorsCut/Levels/L_DC_Act1_Promise.umap',O/'before.umap')
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};report={}
for who,role,sex in [('Juliet','Archer','Male'),('Romeo','Swordsman','Female')]:
 a=cast[who];source=u.load_asset(B+'/Animations/KissRevision12/A_Act1_'+who);mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];kiss=u.load_asset(B+'/Animations/KissRevision12/A_'+role+'_'+sex+'Kiss');assert kiss;data={b:[[],[],[]] for b in bones}
 neutral=list(u.AnimationLibrary.get_bone_poses_for_time(kiss,bones,0,False,mesh))
 for f in range(601):
  t=f/30;original=list(u.AnimationLibrary.get_bone_poses_for_time(source,bones,min(t,17.8),False,mesh));p=original
  if t>16:
   weight=smooth((t-16)/1.8);phase=1.45*weight;raw=list(u.AnimationLibrary.get_bone_poses_for_time(kiss,bones,phase,False,mesh));raw[0].translation=u.Vector(original[0].translation.x,original[0].translation.y,raw[0].translation.z)
   for i,b in enumerate(bones):
    if b in ['neck_joint','head_joint']:raw[i].rotation=blend(raw[i],neutral[i],.18*weight).rotation
    if b in ['waist_joint','chest_joint']:raw[i].rotation=blend(raw[i],neutral[i],.10*weight).rotation
   p=[blend(x,y,smooth((t-16)/.4)) for x,y in zip(original,raw)]
   for i,b in enumerate(bones):
    if b in ['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint']:p[i]=original[i]
  for b,tr in zip(bones,p):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Act1_'+who;anim=u.load_asset(dest) or e.duplicate_asset(source.get_path_name(),dest);c=anim.get_editor_property('controller');c.open_bracket('Smooth lean into held kiss',False)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);assert e.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);a.evaluate(17.8);hold=a.get_actor_transform()
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and w.get_editor_property('time')>=17.8:w.set_actor_location_and_rotation(hold.translation,hold.rotation.rotator(),False,False)
 report[who]={'source':kiss.get_path_name(),'hold_from':17.8,'source_hold_time':1.45}
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise')
assert les.load_level(B+'/Levels/L_DC_Act2_BrokenPromise');assert les.load_level(B+'/Levels/L_DC_Act1_Promise');cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
for who in ['Romeo','Juliet']:
 a=cast[who];a.evaluate(17.8);c=a.get_component_by_class(u.SkeletalMeshComponent);names=['head_joint','mouth_joint','eyes_joint','neck_joint','pelvis_joint'];fixed={b:c.get_socket_transform(b) for b in names};error=0
 for f in range(535,601):
  a.evaluate(f/30)
  for b in names:
   tf=c.get_socket_transform(b);error=max(error,(tf.translation-fixed[b].translation).length());assert abs(sum(x*y for x,y in zip(tf.rotation.to_tuple(),fixed[b].rotation.to_tuple())))>.99999
 assert error<.01,(who,error);report[who]['held_world_pose_error_cm']=error
(O/'validation.json').write_text(json.dumps(report,indent=2));print('KISS13_VALIDATED',report)
