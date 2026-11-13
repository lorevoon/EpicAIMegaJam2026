import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Real')
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DirectionalLight):a.set_actor_rotation(u.Rotator(pitch=-35,yaw=145),False)
 if isinstance(a,u.SkyLight):a.get_component_by_class(u.SkyLightComponent).set_intensity(3.)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Real')
mat=u.load_asset('/Game/DirectorsCut/Materials/M_Act1_OutdoorGround');mat.set_editor_property('two_sided',True);u.MaterialEditingLibrary.recompile_material(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for name,visible in [('L_DC_Act1_Set',True),('L_DC_Act1_Real',False)]:
 layer=u.GameplayStatics.get_streaming_level(world,name);u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
for a in list(aes.get_all_level_actors()):
 if isinstance(a,(u.ProceduralFoliageVolume,u.ProceduralFoliageBlockingVolume,u.LightmassImportanceVolume)):aes.destroy_actor(a)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('DUAL_SCENE_FINISHED')
