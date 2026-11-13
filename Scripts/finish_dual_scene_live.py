import unreal as u
mat=u.load_asset('/Game/DirectorsCut/Materials/M_Act1_OutdoorGround');mat.set_editor_property('two_sided',True);u.MaterialEditingLibrary.recompile_material(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name()=='L_DC_Act1_Promise':
 for name,visible in [('L_DC_Act1_Set',True),('L_DC_Act1_Real',False)]:
  layer=u.GameplayStatics.get_streaming_level(world,name);u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
 assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('LIVE_GROUND_AND_DEFAULT_VIEW_SAVED')
