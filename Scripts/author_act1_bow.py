"""Replace Smith's shoulder-pat interval with the supplied bow; retain face and later wave."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut';fps=30
p=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Mr. Smith')
mesh=p.get_editor_property('character_mesh');bones=json.loads((root/'SourceArt/Act1Faces/blocking_source.json').read_text())['Mr. Smith']['bones']
source=u.load_asset(base+'/Animations/Act1Faces/A_Act1_MrSmith_FacePerformance');bow=u.load_asset(base+'/Animations/Act1Bow/A_Smith_QuickInformalBow')
path=base+'/Animations/Act1Bow/A_Act1_MrSmith_BowPerformance';anim=u.load_asset(path) or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
start=3.5;end=6.5;length=bow.get_play_length()
before=u.AnimationLibrary.get_bone_poses_for_time(source,bones,start,False,mesh);after=u.AnimationLibrary.get_bone_poses_for_time(source,bones,end,False,mesh)
first=u.AnimationLibrary.get_bone_poses_for_time(bow,bones,0,False,mesh)
face={'eyes_joint','mouth_joint','L_eyebrow_joint','R_eyebrow_joint'}
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 values=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(v*v for v in values));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[v/n for v in values]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
data={b:[[],[],[]] for b in bones}
for f in range(601):
 t=f/fps;poses=u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh)
 if start<=t<=end:
  new=u.AnimationLibrary.get_bone_poses_for_time(bow,bones,min(length,max(0,t-start+.5)),False,mesh)
  for i,b in enumerate(bones):
   if b in face:continue
   tr=new[i]
   if i==0:tr.translation=before[i].translation+(tr.translation-first[i].translation)
   tr=blend(before[i],tr,smooth((t-start)/.22))
   tr=blend(tr,after[i],smooth((t-(end-.25))/.25));poses[i]=tr
 for b,tr in zip(bones,poses):
  data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
c=anim.get_editor_property('controller');c.open_bracket('Replace shoulder pat with supplied informal bow',False)
for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False),b
c.close_bracket(False);assert u.EditorAssetLibrary.save_loaded_asset(anim,False)
print('SMITH_BOW_BAKED',start,end)

