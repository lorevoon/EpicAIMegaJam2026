import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for name,visible in [('L_DC_Act1_Set',True),('L_DC_Act1_Real',False)]:
 layer=u.GameplayStatics.get_streaming_level(world,name);assert layer
 u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
 assert layer.get_editor_property('level_transform').translation==u.Vector(0,0,0)
actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)]
assert len(cast)==5
for a in cast:
 w=next(w for w in actors if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and w.get_editor_property('time')==0)
 assert (a.get_actor_location()-w.get_actor_location()).length()<.01,a.get_actor_label()
assert u.load_asset('/Game/DirectorsCut/Environment/SM_Act1_RealGround').get_bounds().box_extent.z<1000
assert u.load_asset('/Game/DirectorsCut/Materials/M_Act1_OutdoorGround').get_editor_property('two_sided')
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('FINAL_DUAL_SCENE_VALIDATED')
