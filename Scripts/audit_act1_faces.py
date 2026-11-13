import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
for a in actors:
 if not isinstance(a,u.DCPerformer) or a.get_actor_label() not in ['Romeo','Juliet','Mr. Smith']:continue
 c=a.get_component_by_class(u.SkeletalMeshComponent)
 row={'mesh':a.get_editor_property('character_mesh').get_path_name(),'mesh_offset':a.get_editor_property('mesh_offset').export_text(),'bones':[str(c.get_bone_name(i)) for i in range(c.get_num_bones())],'cues':[],'waypoints':[],'samples':{}}
 for cue in a.get_editor_property('animation_cues'):
  row['cues'].append({'time':cue.start_time,'animation':cue.animation.get_path_name() if cue.animation else None,'rate':cue.play_rate,'offset':cue.start_offset,'loop':cue.loop})
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:row['waypoints'].append({'time':w.get_editor_property('time'),'transform':w.get_actor_transform().export_text()})
 for t in [0,4,7,10,12.5,16,17.5,18,19,20,0]:
  a.evaluate(t)
  row['samples'][str(t)]={'actor':a.get_actor_transform().export_text(),'bones':{b:c.get_socket_transform(b,u.RelativeTransformSpace.RTS_WORLD).export_text() for b in row['bones'] if any(k in b.lower() for k in ['head','lip','mouth','jaw','eye','brow'])}}
 out[a.get_actor_label()]=row
(root/'Saved/FilmSetAudit/act1_faces_before.json').write_text(json.dumps(out,indent=2))
u.log('ACT1_FACES_AUDITED')
