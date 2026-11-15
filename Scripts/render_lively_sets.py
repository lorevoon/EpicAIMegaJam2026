import unreal as u
from pathlib import Path
R=Path(__file__).resolve().parents[1];O=R/'Saved/LivelySet';O.mkdir(exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 les.load_level(f'/Game/DirectorsCut/Levels/L_DC_Act{act}_{main}')
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
 for a in aes.get_all_level_actors():
  if isinstance(a,u.DCCrewMember):a.call_method('PreviewActivity',(1.2,False))
  if a.get_actor_label() in ['Studio roof','Green screen - front','Green screen - right']:a.set_actor_hidden_in_game(True)
 director=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCCrewMember) and a.get_editor_property('director'))
 dp=director.get_actor_location();close=dp+u.Vector(-270,150,150);look=u.MathLibrary.find_look_at_rotation(close,dp+u.Vector(0,0,105))
 for name,pos,rot,fov in [('overview',(-3600,-11500,2600),(-22,140,0),75),('village',(-5100,-10000,1470) if act==1 else (-5200,-10350,1350),(-8,40,0),75),('director_thumb',close.to_tuple(),(look.pitch,look.yaw,0),45)]:
  if name=='director_thumb':director.call_method('PreviewActivity',(1.6,True))
  camera=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector(*pos),u.Rotator(pitch=rot[0],yaw=rot[1]))
  c=camera.get_component_by_class(u.SceneCaptureComponent2D);rt=u.RenderingLibrary.create_render_target2d(world,1440,900,u.TextureRenderTargetFormat.RTF_RGBA8)
  c.set_editor_property('texture_target',rt);c.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR);c.set_editor_property('fov_angle',fov)
  light=aes.spawn_actor_from_class(u.PointLight,u.Vector(*pos)+u.Vector(0,0,200))
  lc=light.get_component_by_class(u.PointLightComponent);lc.set_editor_property('use_inverse_squared_falloff',False);lc.set_editor_property('intensity',12.);lc.set_editor_property('attenuation_radius',6000.);lc.set_editor_property('cast_shadows',False)
  u.SystemLibrary.execute_console_command(world,'Editor.AsyncAssetCompilationFinishAll')
  c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(O),f'act{act}_{name}.png');aes.destroy_actor(camera);aes.destroy_actor(light)
print('LIVELY_PREVIEWS_DONE')
