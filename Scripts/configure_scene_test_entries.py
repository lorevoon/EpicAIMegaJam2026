import unreal as u,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/SceneEntry';out.mkdir(parents=True,exist_ok=True);les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);report={}
for name in ['L_DC_Act1_Set','L_DC_Act1_Real','L_DC_Act2_Set','L_DC_Act2_Real']:
 src=root/'Content/DirectorsCut/Levels'/(name+'.umap');backup=out/(name+'.before.umap')
 if not backup.exists():shutil.copy2(src,backup)
 path='/Game/DirectorsCut/Levels/'+name;assert les.load_level(path);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();ws=world.get_world_settings();old=ws.get_editor_property('default_game_mode');ws.set_editor_property('default_game_mode',u.DCZombieGameMode)
 row={'previous_mode':str(old),'mode':str(ws.get_editor_property('default_game_mode')),'exposure':[]}
 for a in aes.get_all_level_actors():
  if isinstance(a,u.PostProcessVolume):
   s=a.get_editor_property('settings');row['exposure'].append({'actor':a.get_actor_label(),'min':s.auto_exposure_min_brightness,'max':s.auto_exposure_max_brightness,'bias':s.auto_exposure_bias,'method':str(s.auto_exposure_method)})
 assert u.EditorLoadingAndSavingUtils.save_map(world,path);report[name]=row
report['cached_lighting_ev']=u.SystemLibrary.get_console_variable_float_value('r.EyeAdaptation.CachedLightingPreExposure');assert report['cached_lighting_ev']==8
(out/'configuration.json').write_text(json.dumps(report,indent=2));print('SCENE_ENTRY_MAPS_CONFIGURED',json.dumps(report))
