import unreal as u,json,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/PerformanceRevision11';O=R/'Saved/PerformanceRevision11';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)};report={}
# Preserve the box-carrying arms while using the supplied male gait on the legs.
a=cast['Romeo'];c=a.get_component_by_class(u.SkeletalMeshComponent);mesh=a.get_editor_property('character_mesh');anim=u.load_asset(F+'/A_Act1_Romeo');walk=u.load_asset(F+'/A_Swordsman_MaleWalk');bones=[str(c.get_bone_name(i)) for i in range(c.get_num_bones())];legs=[b for b in bones if b.startswith(('L_thigh','R_thigh','L_knee','R_knee','L_ankle','R_ankle','L_ball','R_ball','L_toe','R_toe'))];controller=anim.get_editor_property('controller');controller.open_bracket('Male gait while carrying the box',False)
for b in legs:
 poses=[u.AnimationLibrary.get_bone_poses_for_time(anim,[b],f/30,False,mesh)[0] for f in range(601)]
 for f in range(72):
  t=f/30;poses[f]=u.AnimationLibrary.get_bone_poses_for_time(u.load_asset(B+'/Animations/Act1Dialogue/A_Act1_Romeo_Speaking'),[b],t,False,mesh)[0];p=u.AnimationLibrary.get_bone_poses_for_time(walk,[b],t%walk.get_play_length(),False,mesh)[0];w=max(0,min(1,(2.4-t)/.3));w=w*w*(3-2*w);q=poses[f].rotation;r=p.rotation
  if q.x*r.x+q.y*r.y+q.z*r.z+q.w*r.w<0:r=u.Quat(-r.x,-r.y,-r.z,-r.w)
  v=[x+(y-x)*w for x,y in zip(q.to_tuple(),r.to_tuple())];n=math.sqrt(sum(x*x for x in v));poses[f].rotation=u.Quat(*[x/n for x in v])
 assert controller.set_bone_track_keys(b,[p.translation for p in poses],[p.rotation for p in poses],[p.scale3d for p in poses],False)
controller.close_bracket(False);assert e.save_loaded_asset(anim,False)
for who in ['Romeo','Juliet']:
 a=cast[who];mesh=a.get_editor_property('character_mesh');cue=a.get_editor_property('animation_cues')[0];assert '/PerformanceRevision11/' in cue.animation.get_path_name();assert cue.animation.get_editor_property('skeleton')==mesh.skeleton;report[who]={'animation':cue.animation.get_path_name()}
 # Current lips/eyes retain authored dialogue keys exactly; no borrowed source face.
 source=u.load_asset(B+'/Animations/Act1Dialogue/A_Act1_'+who+'_Speaking')
 for t in [4.5,8,10,16.5,18,19.5]:
  for b in ['mouth_joint','eyes_joint']:
   p=u.AnimationLibrary.get_bone_poses_for_time(cue.animation,[b],t,False,mesh)[0];q=u.AnimationLibrary.get_bone_poses_for_time(source,[b],t,False,mesh)[0];assert (p.scale3d-q.scale3d).length()<.001
report['walking_continuity']={}
for who in ['Romeo','Juliet']:
 a=cast[who];c=a.get_component_by_class(u.SkeletalMeshComponent);previous=None;maximum=0.;offset=0.
 for f in range(220,340):
  a.evaluate(f/30);p=c.get_socket_location('pelvis_joint');actor=a.get_actor_location();offset=max(offset,math.hypot(p.x-actor.x,p.y-actor.y))
  if previous is not None:maximum=max(maximum,(p-previous).length())
  previous=p
 report['walking_continuity'][who]={'max_pelvis_step_cm':maximum,'max_offset_cm':offset}
 assert maximum<25,(who,maximum,offset)
report['kiss']=[]
for t in [16,16.5,17,17.5,18,18.5,19,19.5,20]:
 row={'time':t};points={}
 for who in ['Romeo','Juliet']:
  a=cast[who];a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);points[who]=c.get_socket_location('mouth_joint');row[who]=list(points[who].to_tuple())
 row['mouth_distance_cm']=(points['Romeo']-points['Juliet']).length();report['kiss'].append(row)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise');(O/'validation_act1.json').write_text(json.dumps(report,indent=2));print('ACT1_PERFORMANCE_VALIDATED',report)
