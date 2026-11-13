"""Remove source showcase demo scripting from the two environment-only copies."""
import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
for name in ['L_DC_Act1_Ground','L_DC_Act1_Real']:
 path='/Game/DirectorsCut/Levels/'+name
 assert les.load_level(path)
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
 bp=u.load_object(None,path+'.'+name+':PersistentLevel.'+name)
 if bp:
  graph=u.BlueprintEditorLibrary.find_graph(bp,'EventGraph')
  if graph:u.BlueprintEditorLibrary.remove_graph(bp,graph)
  u.BlueprintEditorLibrary.compile_blueprint(bp)
 assert u.EditorLoadingAndSavingUtils.save_map(world,path)
u.log('ENVIRONMENT_SCRIPTS_CLEANED')

from pathlib import Path
exec((Path(__file__).resolve().parent/"validate_original_act1.py").read_text())
