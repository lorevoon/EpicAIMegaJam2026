import unreal as u,json,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/KissRevision12';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(B+'/Levels/L_DC_Act1_Promise');actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};report={}
for who,role,sex in [('Juliet','Archer','Male'),('Romeo','Swordsman','Female')]:
 a=cast[who];mesh=a.get_editor_property('character_mesh');anim=a.get_editor_property('animation_cues')[0].animation;assert anim.get_path_name().startswith(F);source=u.load_asset(F+'/A_'+role+'_'+sex+'Kiss');assert source;previous=u.load_asset(B+'/Animations/PerformanceRevision11/A_Act1_'+who);bones=['waist_joint','chest_joint','neck_joint','head_joint','L_shoulder_joint','R_shoulder_joint','L_thigh_joint','R_thigh_joint']
 for t in [17,17.5,18,19,20]:
  p=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,t,False,mesh);q=u.AnimationLibrary.get_bone_poses_for_time(source,bones,t-16,False,mesh)
  for x,y in zip(p,q):assert abs(sum(v*w for v,w in zip(x.rotation.to_tuple(),y.rotation.to_tuple())))>.9999
 for t in [0,2,4.7,8.3,12,15.5]:
  p=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,t,False,mesh);q=u.AnimationLibrary.get_bone_poses_for_time(previous,bones,t,False,mesh)
  for x,y in zip(p,q):assert (x.translation-y.translation).length()<.01;assert abs(sum(v*w for v,w in zip(x.rotation.to_tuple(),y.rotation.to_tuple())))>.9999
 report[who]={'source':source.get_path_name(),'unmodified_kiss_pose':'passed','earlier_motion_preserved':'passed'}
routes=json.loads((R/'Saved/KissRevision12/original_marks.json').read_text());checked=0
for w in actors:
 if isinstance(w,u.DCWaypoint) and w.get_actor_label() in routes and w.get_editor_property('time')>=16:
  old=routes[w.get_actor_label()];p=w.get_actor_location();assert abs(abs(p.y-old[1])-8)<.01;checked+=1
assert checked>0;report['spacing_added_cm']=16;report['marks_checked']=checked;(R/'Saved/KissRevision12/validation.json').write_text(json.dumps(report,indent=2));print('KISS12_VALIDATED',report)
