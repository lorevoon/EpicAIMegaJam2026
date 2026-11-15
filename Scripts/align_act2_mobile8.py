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

j=cast['Juliet'];source=load(BASE+'/Act2/SmoothRevision7/A_Grounded_Juliet');mesh=j.get_editor_property('character_mesh');comp=j.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(0))];face={i for i,b in enumerate(bones) if any(s in b.lower() for s in ['mouth','eye','brow','jaw','lip'])}
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

data=[[],[],[]]
for f in range(1141):
 t=f/30;index=max(i for i,q in enumerate(timeline) if q[0]<=t+.00001);tr=sample(index,t)[0];boundary=timeline[index][0];transition=timeline[index][4]
 if index and t-boundary<transition:tr=blend(sample(index-1,boundary)[0],tr,smooth((t-boundary)/transition))
 data[0].append(tr.translation);data[1].append(tr.rotation);data[2].append(tr.scale3d)
anim=load(F+'/A_Mobile_Juliet');c=anim.get_editor_property('controller');c.open_bracket('Match consecutive crouch turn facings',False);assert c.set_bone_track_keys(bones[0],*data,False);c.close_bracket(False);assert eal.save_loaded_asset(anim,False)
print('TURN_OFFSET',turnoffset.rotator())
