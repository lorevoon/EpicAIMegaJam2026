import unreal as u,runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).parent/'review_lively_sets.py'),run_name='__main__')
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(-5250,-10000,1470),u.Rotator(pitch=-8,yaw=35))
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCCrewMember):a.call_method('PreviewActivity',(1.2,False));a.character.set_update_animation_in_editor(True)
print('LIVELY_SET_READY_IN_EDITOR')
