import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';assert les.load_level(path)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCBeat) and str(a.get_editor_property('beat_id'))=='2.5':a.set_editor_property('pose_ready_time',31.)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),path);print('LAST_GESTURE_SHOT_TIMED')
