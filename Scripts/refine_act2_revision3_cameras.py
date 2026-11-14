import unreal as u
from pathlib import Path
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';assert les.load_level(level)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCBeat) and str(a.get_editor_property('beat_id')) in ['2.3','2.4']:
  refs=list(a.get_editor_property('anchors'))
  for r in refs:r.set_editor_property('part',u.DCAnchorPart.HEAD)
  a.set_editor_property('anchors',refs)
 if a.get_actor_label() in ['Raid cut camera 4','Raid cut camera 5']:
  a.tags=[str(t) for t in a.tags if not str(t).startswith('DCFOV=')]+['DCFOV=60'];a.get_component_by_class(u.CameraComponent).set_field_of_view(60)
for a in aes.get_all_level_actors():
 if a.get_actor_label()=='Raid cut camera 5':a.set_actor_location(u.Vector(-6400,-9800,1480),False,False)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),level)
p=Path(__file__).parent/'finish_act2_revision3.py';exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p),'__name__':'__main__'})
p=Path(__file__).parent/'validate_act2_revision3.py';exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p),'__name__':'__main__'})
print('ACT2_CLOSE_FRAMING_SAVED')
