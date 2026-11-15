"""Revision 7: a grounded stand, deliberate sword work and a stationary kneel."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/SmoothRevision7';V6=BASE+'/Act2/BowRevision6';OUT=ROOT/'Saved/Act2/SmoothRevision7';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')};baseline=json.loads((OUT/'baseline.json').read_text())['cast']
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
clips={}
def clip(key,path):clips[key]=load(path)
for role in ['Swordsman','Raider']:
 for name in ['DrawSword','Guard','Dodge']:clip(role+name,F+'/A_'+role+'_'+name)
for role in ['Swordsman','Archer']:
 clip(role+'Idle',BASE+'/Animations/'+role+'/A_'+role+'_Support_Idle')
for name in ['MeleeSlash','MeleeBackhand','MeleeDownward']:
 clip('Swordsman'+name,BASE+'/Animations/Swordsman/A_Swordsman_Support_'+name);clip('Raider'+name,BASE+'/Act2/A_Raider_Support_'+name)
clip('EquipBow',V6+'/A_Archer_EquipBow');clip('DrawArrow',V6+'/A_Archer_DrawArrow');clip('BowReady',BASE+'/Animations/Archer/A_Archer_Bow_Idle');clip('BowAim',BASE+'/Animations/Archer/A_Archer_Support_BowAim');clip('BowRelease',BASE+'/Animations/Archer/A_Archer_Support_BowRelease');clip('RaiderWalk',BASE+'/Act2/A_Raider_Support_Walk')

report=json.loads((OUT/'staging.json').read_text())
for who,performance in report['performances'].items():
 a=cast[who];source=load(baseline[who]['source']);mesh=a.get_editor_property('character_mesh');bone=str(a.get_component_by_class(u.SkeletalMeshComponent).get_bone_name(0));timeline=performance['timeline'];anim=load(performance['asset']);data=[[],[],[]]
 def pose(clip,t):return u.AnimationLibrary.get_bone_poses_for_time(clip,[bone],max(0,min(t,clip.get_play_length())),False,mesh)[0]
 rootref=pose(source,0)
 def sample(index,t):
  start,key,lo,hi,transition=timeline[index];end=timeline[index+1][0] if index+1<len(timeline) else 38
  if key=='SOURCE':return pose(source,lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
  clip=clips[key];length=clip.get_play_length();phase=(t-start)%length if hi is None else length*(lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
  p=pose(clip,phase);p.translation=u.Vector(rootref.translation.x,rootref.translation.y,p.translation.z);return p
 for f in range(1141):
  t=f/30;index=max(i for i,q in enumerate(timeline) if q[0]<=t+.00001);tr=sample(index,t);boundary=timeline[index][0];transition=timeline[index][4]
  if index and t-boundary<transition:tr=blend(sample(index-1,boundary),tr,smooth((t-boundary)/transition))
  data[0].append(tr.translation);data[1].append(tr.rotation);data[2].append(tr.scale3d)
 c=anim.get_editor_property('controller');c.open_bracket('Preserve retargeted body height and orientation',False);assert c.set_bone_track_keys(bone,*data,False);c.close_bracket(False);assert eal.save_loaded_asset(anim,False)
j=cast['Juliet'];j.evaluate(22.3);victim=cast['Raid orc 1'];victim.evaluate(22.65);wrist=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_transform('L_wrist_joint');hit=victim.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');aim=u.MathLibrary.find_look_at_rotation(wrist.translation,hit);desired=u.Rotator(pitch=90,yaw=aim.yaw+90).quaternion();q=wrist.rotation;relative=u.Quat(-q.x,-q.y,-q.z,q.w)*desired;grip=u.Transform(rotation=relative.rotator(),scale=u.Vector(.65,.65,.65));grip.translation=-grip.transform_location(u.Vector(0,-36,0));j.set_editor_property('carry_transform',grip)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
print('ROOT_HEIGHT_AND_BOW_REPAIRED')
