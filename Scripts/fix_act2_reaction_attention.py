"""Keep the supplied surprised body motion while its attention stays on Smith."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/ActingRevision4';out.mkdir(parents=True,exist_ok=True)
level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';folder='/Game/DirectorsCut/Act2/ActingRevision4'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(level);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
def mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b
 return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def inv(q):return (-q[0],-q[1],-q[2],q[3])
def yaw(a):
 a=math.radians(a)/2;return (0,0,math.sin(a),math.cos(a))
def smooth(v):
 v=max(0,min(1,v));return v*v*(3-2*v)
report={}
for name in ['Romeo','Juliet']:
 a=cast[name];comp=a.get_component_by_class(u.SkeletalMeshComponent);mesh=a.get_editor_property('character_mesh');source=u.load_asset('/Game/DirectorsCut/Act2/AudioFacesV3/A_Act2_'+name+'_Speaking')
 cue=u.DCAnimationCue();cue.animation=source;cue.start_time=0;cue.play_rate=1;cue.loop=False;a.set_editor_property('animation_cues',[cue])
 dst=folder+'/A_'+name+'_ReactionAway'
 anim=u.load_asset(dst) or eal.duplicate_asset(source.get_path_name(),dst);assert anim
 bones=['neck_joint','head_joint'];parents={b:str(comp.get_parent_bone(b)) for b in bones};assert parents['head_joint']=='neck_joint',parents
 data={b:[[],[],[]] for b in bones};samples=[]
 for f in range(1141):
  t=f/30;poses=list(u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh))
  if 10<t<13:
   a.evaluate(t);head=comp.get_socket_location('head_joint');mouth=comp.get_socket_location('mouth_joint');target=cast['Mr. Smith'].get_actor_location()
   current=math.degrees(math.atan2(mouth.y-head.y,mouth.x-head.x));desired=math.degrees(math.atan2(target.y-head.y,target.x-head.x));delta=(desired-current+180)%360-180
   weight=min(smooth((t-10)/.2),smooth((13-t)/.2));delta*=weight
   neck_parent=tuple(comp.get_socket_transform(parents['neck_joint'],u.RelativeTransformSpace.RTS_WORLD).rotation.to_tuple())
   neck_world=tuple(comp.get_socket_transform('neck_joint',u.RelativeTransformSpace.RTS_WORLD).rotation.to_tuple())
   head_world=tuple(comp.get_socket_transform('head_joint',u.RelativeTransformSpace.RTS_WORLD).rotation.to_tuple())
   corrected_neck=mul(yaw(delta*.35),neck_world);corrected_head=mul(yaw(delta),head_world)
   poses[0].rotation=u.Quat(*mul(inv(neck_parent),corrected_neck));poses[1].rotation=u.Quat(*mul(inv(corrected_neck),corrected_head))
   if f%3==0:samples.append([t,current,desired,delta])
  for b,tr in zip(bones,poses):
   data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 controller=anim.get_editor_property('controller');controller.open_bracket('Reaction attention on Smith',False)
 for b,keys in data.items():assert controller.set_bone_track_keys(b,*keys,False)
 controller.close_bracket(False);assert eal.save_loaded_asset(anim,False)
 a.evaluate(0);cue=u.DCAnimationCue();cue.animation=anim;cue.start_time=0;cue.play_rate=1;cue.loop=False;a.set_editor_property('animation_cues',[cue]);report[name]={'source':source.get_path_name(),'animation':dst,'samples':samples}
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
# Validate actual evaluated mouth direction in the central reaction window.
camera=u.Vector(-4550,-9250,1390)
for name in ['Romeo','Juliet']:
 a=cast[name];comp=a.get_component_by_class(u.SkeletalMeshComponent);measure=[]
 for t in [10.25,10.5,11,11.5,12,12.5,12.75]:
  a.evaluate(t);head=comp.get_socket_location('head_joint');mouth=comp.get_socket_location('mouth_joint');face=mouth-head;target=cast['Mr. Smith'].get_actor_location()-head;view=camera-head
  dot=(face.x*view.x+face.y*view.y)/math.hypot(face.x,face.y)/math.hypot(view.x,view.y)
  err=abs((math.degrees(math.atan2(face.y,face.x)-math.atan2(target.y,target.x))+180)%360-180)
  measure.append([t,err,dot])
 report[name]['verified_head_to_smith_error_and_camera_dot']=measure
 a.evaluate(0)
(out/'reaction_attention.json').write_text(json.dumps(report,indent=2));print('ACT2_REACTION_ATTENTION_FIXED')
