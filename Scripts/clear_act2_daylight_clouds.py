import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_Real';assert les.load_level(path);n=0
for a in aes.get_all_level_actors():
 c=a.get_component_by_class(u.StaticMeshComponent)
 if c and any('MI_Cloud_Large' in str(c.get_material(i)) for i in range(c.get_num_materials())):
  a.set_actor_hidden_in_game(True);a.set_is_temporarily_hidden_in_editor(True);a.set_actor_enable_collision(False);n+=1
assert n==14,n
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),path);print('ACT2_DAYLIGHT_CLOUDS_HIDDEN',n)
