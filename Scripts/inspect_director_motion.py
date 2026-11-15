import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Set');a=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCCrewMember) and a.get_editor_property('director'));out={}
for stem in ['Idle','ThumbsUp']:
 clip=u.load_asset('/Game/DirectorsCut/LivelySet/A_DirectorGame_'+stem);a.character.play_animation(clip,False);rows=[]
 for t in [0,.4,.8,1.2,1.6,2.1,2.8,3.5]:
  a.character.set_position(t,False);rows.append(dict(t=t,raw=[p.export_text() for p in u.AnimationLibrary.get_bone_poses_for_time(clip,['RightArm','RightForeArm','RightHand'],t,False,a.character.skeletal_mesh_asset)],bones={str(a.character.get_bone_name(i)):list((a.character.get_socket_location(a.character.get_bone_name(i))-a.get_actor_location()).to_tuple()) for i in range(a.character.get_num_bones()) if any(s in str(a.character.get_bone_name(i)) for s in ['Hand','ForeArm'])}))
 out[stem]=rows
(R/'Saved/director_motion.json').write_text(json.dumps(out,indent=2))
print('POSE_METHODS',[s for s in dir(a.character) if any(k in s for k in ['tick','refresh','update_animation'])])
