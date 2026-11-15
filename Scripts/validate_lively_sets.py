import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);report={}
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 assert les.load_level(f'/Game/DirectorsCut/Levels/L_DC_Act{act}_{main}')
 actors=list(aes.get_all_level_actors());crew=[a for a in actors if isinstance(a,u.DCCrewMember)];assert len(crew)==9
 assert len([a for a in crew if a.get_editor_property('director')])==1
 rows=[]
 for a in crew:
  mesh=a.character.skeletal_mesh_asset;anim=a.get_editor_property('activity');assert mesh and anim
  assert mesh.skeleton==anim.get_editor_property('skeleton'),(a.get_actor_label(),'skeleton mismatch')
  assert a.actor_has_tag('DC_SetOnly') and f'L_DC_Act{act}_Set.' in a.get_path_name()
  assert len(a.get_editor_property('lines'))>=2
  if a.get_editor_property('director'):
   thumb=a.get_editor_property('celebration');assert thumb and thumb.get_play_length()>0;assert thumb.get_editor_property('skeleton')==mesh.skeleton
  a.character.play_animation(anim,True);a.character.set_position(1,False)
  c,e=a.get_actor_bounds(False);rows.append(dict(role=a.get_actor_label(),height=e.z*2,animation=anim.get_path_name(),length=anim.get_play_length(),bones=a.character.get_num_bones()))
 assert not any(a.get_actor_label().startswith('H3D - Light') for a in actors)
 refs=[a for a in actors if a.get_actor_label().startswith('Matched reference - ')]
 assert len(refs)>15
 report[str(act)]=dict(crew=rows,reference_components=len(refs),lamps_removed=True)
(R/'Saved/lively_validation.json').write_text(json.dumps(report,indent=2));print('LIVELY_SET_VALIDATED')
