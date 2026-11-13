import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
cam=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector());c=cam.get_component_by_class(u.SceneCaptureComponent2D)
rt=u.RenderingLibrary.create_render_target2d(world,800,700,u.TextureRenderTargetFormat.RTF_RGBA8);c.texture_target=rt;c.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR;c.fov_angle=32
light=aes.spawn_actor_from_class(u.PointLight,u.Vector());lc=light.get_component_by_class(u.PointLightComponent);lc.set_editor_property('use_inverse_squared_falloff',False);lc.set_intensity(8);lc.set_editor_property('attenuation_radius',3000);lc.set_cast_shadows(False)
for who,t,label in [('Romeo',.29,'romeo_open'),('Romeo',.43,'romeo_closed'),('Juliet',10.34,'juliet_open'),('Mr. Smith',2.49,'smith_open')]:
 for p in cast.values():p.evaluate(t)
 p=cast[who];mesh=p.get_component_by_class(u.SkeletalMeshComponent);target=mesh.get_socket_location('mouth_joint')+u.Vector(0,0,8);pos=target+p.get_actor_forward_vector()*150+u.Vector(0,0,8)
 cam.set_actor_location_and_rotation(pos,u.MathLibrary.find_look_at_rotation(pos,target),False,False);light.set_actor_location(pos+u.Vector(0,0,70),False,False)
 c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(root/'Saved'), 'dialogue_'+label+'.png')
for p in cast.values():p.evaluate(0)
aes.destroy_actor(light);aes.destroy_actor(cam)
# Restore the saved start state after the temporary preview actors and poses.
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
print('DIALOGUE_PREVIEWS_COMPLETE')
