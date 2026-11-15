import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';O=R/'Saved/Act1Revision14';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());board=next(a for a in actors if isinstance(a,u.DCClapperboard));mark=next(a for a in actors if isinstance(a,u.DCStartMark));p=mark.get_actor_location();p.z=board.get_actor_location().z;mark.set_actor_location(p,False,False);cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};report={'spawn':list(p.to_tuple())}
for who in ['Romeo','Juliet']:
 a=cast[who];c=a.get_component_by_class(u.SkeletalMeshComponent);rows=[]
 for t in [0,.4,.8,1.2,4,5.6,12,14.8,15.3,15.8,16,17.8,19.5]:
  a.evaluate(t);mouth=u.AnimationLibrary.get_bone_poses_for_time(a.get_editor_property('animation_cues')[0].animation,['mouth_joint'],t,False,a.get_editor_property('character_mesh'))[0];rows.append({'t':t,'position':list(a.get_actor_location().to_tuple()),'mouth_scale':list(mouth.scale3d.to_tuple()),'smile':c.get_anim_instance().get_curve_value('FaceSmile'),'head':list(c.get_socket_location('head_joint').to_tuple())})
 report[who]=rows
 assert (u.Vector(*rows[-1]['head'])-u.Vector(*rows[-2]['head'])).length()<.01
 if who=='Romeo':assert rows[2]['smile']>.8;assert abs(rows[1]['mouth_scale'][1]-rows[2]['mouth_scale'][1])>1
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise');(O/'validation.json').write_text(json.dumps(report,indent=2));print('ACT1_14_VALIDATED')
