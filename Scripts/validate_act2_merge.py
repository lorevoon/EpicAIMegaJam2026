"""Preserve main's Act1 dressing while retaining direct per-scene testing."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/MergeAct2';out.mkdir(exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);report={}
for name in ['L_DC_Act1_Set','L_DC_Act1_Real']:
 path='/Game/DirectorsCut/Levels/'+name;assert les.load_level(path)
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();settings=world.get_world_settings()
 before=settings.get_editor_property('default_game_mode');wanted=u.DCZombieGameMode.static_class()
 if before!=wanted:
  settings.set_editor_property('default_game_mode',wanted);assert u.EditorLoadingAndSavingUtils.save_map(world,path)
 report[name]={'previous_mode':str(before),'current_mode':str(settings.get_editor_property('default_game_mode'))}
registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
prototype='/Game/DirectorsCut/Environment/SM_Act2_StudioGround'
report['unused_ground_referencers']=[str(p) for p in registry.get_referencers(prototype,u.AssetRegistryDependencyOptions())]
(out/'maps.json').write_text(json.dumps(report,indent=2))
script=root/'Scripts/validate_act2_revision4.py'
exec(compile(script.read_text(),str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
print('ACT2_MERGE_ASSETS_VALIDATED')
