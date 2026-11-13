"""Render unsaved close-ups for facial and contact review; never save the posed map."""
import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet','Mr. Smith']}
cam=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector(),u.Rotator())
c=cam.get_component_by_class(u.SceneCaptureComponent2D)
rt=u.RenderingLibrary.create_render_target2d(world,1200,900,u.TextureRenderTargetFormat.RTF_RGBA8)
c.set_editor_property('texture_target',rt);c.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR);c.set_editor_property('fov_angle',40.)
pp=c.get_editor_property('post_process_settings');pp.set_editor_property('override_vignette_intensity',True);pp.set_editor_property('vignette_intensity',0);c.set_editor_property('post_process_settings',pp)
light=aes.spawn_actor_from_class(u.PointLight,u.Vector(),u.Rotator())
lc=light.get_component_by_class(u.PointLightComponent);lc.set_editor_property('use_inverse_squared_falloff',False);lc.set_editor_property('intensity',8.);lc.set_editor_property('attenuation_radius',3000.);lc.set_editor_property('cast_shadows',False)
u.SystemLibrary.execute_console_command(world,'Editor.AsyncAssetCompilationFinishAll')
for name,t in [('smith_smile',7),('kiss_contact',18),('kiss_other_side',18),('close_walk',10),('smith_original_mat',7)]:
 for a in cast.values():a.evaluate(t)
 if name.startswith('smith'):
  a=cast['Mr. Smith'];target=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('mouth_joint');pos=target+a.get_actor_forward_vector()*160+u.Vector(0,0,15)
  if name=='smith_original_mat':
   mesh=a.get_component_by_class(u.SkeletalMeshComponent)
   for i in range(mesh.get_num_materials()):
    if 'MI_Act1_FacialExpressions' in mesh.get_material(i).get_name():mesh.set_material(i,u.load_asset('/Game/DirectorsCut/Materials/MI_Polysplit_Hero_Body'))
 else:
  target=sum([cast[n].get_component_by_class(u.SkeletalMeshComponent).get_socket_location('mouth_joint') for n in ['Romeo','Juliet']],u.Vector())*.5
  pos=target+u.Vector(230 if name=='kiss_contact' else -230 if name=='kiss_other_side' else 450,0,15)
 cam.set_actor_location_and_rotation(pos,u.MathLibrary.find_look_at_rotation(pos,target),False,False)
 light.set_actor_location(pos+u.Vector(0,0,80),False,False)
 c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(root/'Saved/FilmSetAudit'),name+'.png')
for a in cast.values():a.evaluate(0)
u.log('ACT1_FACE_PREVIEWS_EXPORTED')
