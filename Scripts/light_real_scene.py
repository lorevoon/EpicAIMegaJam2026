import unreal as u,json
from pathlib import Path
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Real')
aes=u.get_editor_subsystem(u.EditorActorSubsystem);rows=[]
for a in aes.get_all_level_actors():
 rows.append([a.get_actor_label(),a.get_path_name()])
 if isinstance(a,u.DirectionalLight):a.get_component_by_class(u.DirectionalLightComponent).set_intensity(1000.)
 if isinstance(a,u.PostProcessVolume):
  a.set_editor_property('priority',100.)
  s=a.get_editor_property('settings');s.set_editor_property('auto_exposure_min_brightness',5.);s.set_editor_property('auto_exposure_max_brightness',5.);a.set_editor_property('settings',s)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Real')
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/FilmSetAudit/real_inventory.json').write_text(json.dumps(rows,indent=2))
u.log('REAL_DAYLIGHT_SAVED')
