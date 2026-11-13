import unreal as u
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCPerformer):a.evaluate(3.4)
u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(-5680,-9040,1510),u.Rotator(pitch=-20,yaw=140))
