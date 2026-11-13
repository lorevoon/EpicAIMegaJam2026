"""Split a copy of WindmillValley into shared terrain and replay-only dressing.
Never saves the source map. Practical mill actors remain in the persistent action map.
"""
import unreal as u, hashlib, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source='/Game/WindmillValley/Levels/L_Showcase'
base='/Game/DirectorsCut/Levels/'
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
source_file=root/'Content/WindmillValley/Levels/L_Showcase.umap'
before=hashlib.sha256(source_file.read_bytes()).hexdigest()
practical=set(json.loads((root/'Saved/FilmSetAudit/original_layout.json').read_text())['props'])
def terrain(a):
 return isinstance(a,u.LandscapeProxy) or a.get_class().get_name()=='LandscapeSplineActor'
for name,ground_only in [('L_DC_Act1_Ground',True),('L_DC_Act1_Real',False)]:
 assert les.load_level(source)
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
 assert u.EditorLoadingAndSavingUtils.save_map(world,base+name)
 assert les.load_level(base+name)
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
 for a in list(aes.get_all_level_actors()):
  if a.get_attach_parent_actor() or isinstance(a,u.WorldSettings) or a.get_class()==u.Brush:continue
  keep=terrain(a) if ground_only else not (terrain(a) or a.get_actor_label() in practical or isinstance(a,(u.PlayerStart,u.ProceduralFoliageVolume,u.ProceduralFoliageBlockingVolume)))
  if not keep:aes.destroy_actor(a)
  elif ground_only:
   a.tags=list(a.tags)+['DC_OriginalTerrain']
   a.set_folder_path('Act1/Shared original terrain')
 assert u.EditorLoadingAndSavingUtils.save_map(world,base+name)
assert hashlib.sha256(source_file.read_bytes()).hexdigest()==before
u.log('ORIGINAL_ENVIRONMENT_SPLIT')
