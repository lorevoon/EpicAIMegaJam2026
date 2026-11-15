"""Fit paired kiss with joint rotations, preserving bone lengths and planted feet."""
import unreal as u,math,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/PerformanceRevision11';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
def inverse(q):return u.Quat(-q.x,-q.y,-q.z,q.w)
def compose(local,parent):
 tr=u.Transform();tr.translation=parent.transform_location(local.translation);tr.rotation=parent.rotation*local.rotation;tr.scale3d=local.scale3d*parent.scale3d;return tr
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
chains=['waist_joint','chest_joint','neck_joint','head_joint'];state={};report=[]
for who in ['Romeo','Juliet']:
 a=cast[who];c=a.get_component_by_class(u.SkeletalMeshComponent);mesh=a.get_editor_property('character_mesh');anim=u.load_asset(F+'/A_Act1_'+who);bones=[str(c.get_bone_name(i)) for i in range(c.get_num_bones())];parents={b:str(c.get_parent_bone(b)) for b in bones};poses=[list(u.AnimationLibrary.get_bone_poses_for_time(anim,bones,f/30,False,mesh)) for f in range(601)];state[who]={'a':a,'c':c,'anim':anim,'bones':bones,'parents':parents,'poses':poses}
for f in range(481,601):
 t=f/30;points={}
 for who,s in state.items():s['a'].evaluate(t);points[who]=s['c'].get_socket_location('mouth_joint')
 shared=(points['Romeo']+points['Juliet'])*.5;weight=smooth((t-16.65)/1.05);errors={}
 for who,s in state.items():
  p=s['poses'][f];local=dict(zip(s['bones'],p));original={b:local[b].rotation for b in chains};base=s['c'].get_world_transform();target=points[who]+(shared-points[who])*weight
  def fk():
   ws={}
   for b in s['bones']:ws[b]=compose(local[b],ws.get(s['parents'][b],base))
   return ws
  for iteration in range(24):
   for b in reversed(chains):
    ws=fk();pivot=ws[b].translation;v=(ws['mouth_joint'].translation-pivot).normal();goal=(target-pivot).normal();dot=max(-1,min(1,v.dot(goal)));axis=v.cross(goal)
    if axis.length()<.00001:continue
    axis=axis.normal();angle=min(math.acos(dot),math.radians(3));delta=u.Quat(axis.x*math.sin(angle/2),axis.y*math.sin(angle/2),axis.z*math.sin(angle/2),math.cos(angle/2));parent=ws.get(s['parents'][b],base);q=inverse(parent.rotation)*delta*ws[b].rotation
    difference=inverse(original[b])*q
    if difference.w<0:difference=u.Quat(-difference.x,-difference.y,-difference.z,-difference.w)
    total=2*math.acos(max(-1,min(1,difference.w)));limit=math.radians(25 if b in ['waist_joint','chest_joint'] else 18)
    if total>limit:
     length=math.sqrt(difference.x**2+difference.y**2+difference.z**2);sn=math.sin(limit/2)/max(length,.00001);difference=u.Quat(difference.x*sn,difference.y*sn,difference.z*sn,math.cos(limit/2));q=original[b]*difference
    local[b].rotation=q
   if (fk()['mouth_joint'].translation-target).length()<.15:break
  errors[who]=(fk()['mouth_joint'].translation-target).length()
 if f%15==0:report.append({'t':t,'error_cm':errors})
for who,s in state.items():
 c=s['anim'].get_editor_property('controller');c.open_bracket('Paired kiss contact without stretching bones',False)
 for b in chains:
  i=s['bones'].index(b);rows=[p[i] for p in s['poses']];assert c.set_bone_track_keys(b,[p.translation for p in rows],[p.rotation for p in rows],[p.scale3d for p in rows],False)
 c.close_bracket(False);assert e.save_loaded_asset(s['anim'],False);s['a'].evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise');(R/'Saved/PerformanceRevision11/kiss_fit.json').write_text(json.dumps(report,indent=2));print('PAIRED_KISS_FIT',report)
