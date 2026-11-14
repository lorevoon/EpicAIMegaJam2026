import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise');actors=list(aes.get_all_level_actors());d=next(a for a in actors if isinstance(a,u.DCZombieDirector));duration=d.get_editor_property('take_duration');rows={}
for a in actors:
 if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet']:
  a.evaluate(duration);rows[a.get_actor_label()]={'position':list(a.get_actor_location().to_tuple()),'rotation':list(a.get_actor_rotation().to_tuple()),'mesh':a.get_editor_property('character_mesh').get_path_name(),'cues':[{'t':q.start_time,'anim':q.animation.get_path_name(),'offset':q.start_offset,'rate':q.play_rate,'loop':q.loop} for q in a.get_editor_property('animation_cues')]}
(root/'Saved/Act2/act1_continuity.json').write_text(json.dumps({'duration':duration,'cast':rows},indent=2));print('ACT1_CONTINUITY',rows)
