import unreal as u
from pathlib import Path
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_Real';assert les.load_level(path)
for a in aes.get_all_level_actors():
 if isinstance(a,u.AmbientSound) and a.get_actor_label()=='Starter_Background_Cue':
  c=a.get_component_by_class(u.AudioComponent);c.stop();c.set_editor_property('auto_activate',False);c.set_sound(None);a.set_actor_label('Legacy village ambience - replaced by synchronized wildfire mix')
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),path)
exec(compile((Path(__file__).parent/'validate_act2_stage.py').read_text(),str(Path(__file__).parent/'validate_act2_stage.py'),'exec'))
print('ACT2_AUDIO_FINAL_VALIDATED')
