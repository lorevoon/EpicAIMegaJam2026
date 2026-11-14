import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_Real';assert les.load_level(path)
for a in aes.get_all_level_actors():
 if isinstance(a,u.PostProcessVolume):
  settings=a.get_editor_property('settings');blendables=settings.get_editor_property('weighted_blendables');items=blendables.get_editor_property('array');blendables.set_editor_property('array',[x for x in items if 'Fogging' not in str(x.get_editor_property('object'))]);settings.set_editor_property('weighted_blendables',blendables);a.set_editor_property('settings',settings)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),path);print('ACT2_BLUE_POSTPROCESS_REMOVED')
