import unreal as u
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary;base='/Game/DirectorsCut'
les.load_level(base+'/Levels/L_DC_Act2_Set')
for a in aes.get_all_level_actors():
 if a.get_actor_label() in ['Grip stand - spare lamp','Studio equipment bay tape']:
  p=a.get_actor_location();p.x=-4660;a.set_actor_location(p,False,False)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),base+'/Levels/L_DC_Act2_Set')
les.load_level(base+'/Levels/L_DC_Act2_Real')
mat=u.load_asset(base+'/Act2/Revision2/M_WildfireAshSky_Final') or eal.duplicate_asset(base+'/Act2/Revision2/M_WildfireAshSky',base+'/Act2/Revision2/M_WildfireAshSky_Final');mat.set_editor_property('is_sky',True);u.MaterialEditingLibrary.recompile_material(mat);assert eal.save_loaded_asset(mat,False)
for a in list(aes.get_all_level_actors()):
 if isinstance(a,u.AtmosphericFog):aes.destroy_actor(a)
 if a.get_actor_label()=='Act2 red-black ash sky':a.static_mesh_component.set_material(0,mat)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),base+'/Levels/L_DC_Act2_Real')
les.load_level(base+'/Levels/L_DC_Act2_BrokenPromise')
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCBeat) and str(a.get_editor_property('beat_id'))=='2.1':a.set_editor_property('camera_spot',a.get_actor_transform().inverse_transform_location(u.Vector(-5400,-9000,1450)))
 if isinstance(a,u.DCWaypoint):
  actor=a.get_editor_property('performer');t=a.get_editor_property('time')
  if actor and actor.get_actor_label()=='Juliet' and t>29:
   p=a.get_actor_location();p.y=-8950-110*min(1,(t-29)/2);a.set_actor_location(p,False,False)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),base+'/Levels/L_DC_Act2_BrokenPromise');print('REVISION2_VISUALS_REFINED')
