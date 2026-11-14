"""Create the standalone front-end map after compiling DCTitleGameMode."""
import unreal

path = '/Game/DirectorsCut/Levels/L_DC_Title'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.new_level(path)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = unreal.load_class(None, '/Script/EpicAIMegaJam2026.DCTitleGameMode')
assert mode
world.get_world_settings().set_editor_property('default_game_mode', mode)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
unreal.log('TITLE_SCENE_CREATED ' + path)
