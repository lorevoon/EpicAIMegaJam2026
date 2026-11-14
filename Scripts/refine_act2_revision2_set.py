import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_Set';assert les.load_level(path)
for a in aes.get_all_level_actors():
 if a.get_actor_label().startswith('Soundstage floor -'):
  p=a.get_actor_location();p.z=665;p.y=-9250;a.set_actor_location(p,False,False);s=a.get_actor_scale3d();s.y=36;a.set_actor_scale3d(s);a.set_actor_label('Soundstage floor - lowered below raid terrain')
 if a.get_actor_label()=='Green screen - front':
  p=a.get_actor_location();p.y=-11050;a.set_actor_location(p,False,False)
 if a.get_actor_label()=='Equipment power cable - perimeter':
  p=a.get_actor_location();p.y=-11000;p.z=681;a.set_actor_location(p,False,False)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),path);print('RAID_STUDIO_LANE_CLEARED')
