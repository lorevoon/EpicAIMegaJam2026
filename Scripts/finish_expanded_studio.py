import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(level)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in aes.get_all_level_actors():
 label=a.get_actor_label();p=a.get_actor_location();s=a.get_actor_scale3d()
 if label=='Soundstage floor - 56m x 44m':s.y=48;a.set_actor_scale3d(s);a.set_actor_label('Soundstage floor - 56m x 48m')
 if label in ['Green screen - left','Green screen - right']:s.y=48;a.set_actor_scale3d(s)
 if label=='Green screen - rear':p.y=2400;a.set_actor_location(p,False,False)
 if label=='Green screen - front':p.y=-2400;a.set_actor_location(p,False,False)
 if label=='Studio roof':s.y=48.4;a.set_actor_scale3d(s)
 if label=='Studio rear shell':p.y=2440;a.set_actor_location(p,False,False)
 if isinstance(a,u.PostProcessVolume):
  settings=a.get_editor_property('settings');settings.set_editor_property('auto_exposure_min_brightness',1.5);settings.set_editor_property('auto_exposure_max_brightness',1.5);a.set_editor_property('settings',settings)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
exec((root/'Scripts/validate_expanded_studio.py').read_text())
