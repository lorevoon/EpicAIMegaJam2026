import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];O=R/'Saved/LivelySet/Revision';O.mkdir(exist_ok=True);B='/Game/DirectorsCut';F=B+'/LivelySet';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);report={}
tex=u.load_asset(B+'/Materials/T_Polysplit_RGBMask');tex.set_editor_property('never_stream',True)
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 les.load_level(f'{B}/Levels/L_DC_Act{act}_{main}');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();crew=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCCrewMember)];assert len(crew)==9
 assert not any(a.get_editor_property('crew_role') in ['Sound mixer','Boom operator'] for a in crew)
 director=next(a for a in crew if a.get_editor_property('director'));assert 'BreathingIdle' in director.get_editor_property('activity').get_name()
 for a in crew:a.call_method('PreviewActivity',(1.2,False))
 subjects=[('crew',next(a for a in crew if a.get_editor_property('crew_role')=='Producer')),('director',director)]
 if act==1:
  romeo=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo');romeo.evaluate(5);subjects.append(('romeo',romeo))
  anim=romeo.get_editor_property('animation_cues')[0].animation;report['romeo_clip']=anim.get_path_name();assert 'A_RomeoHappy_' in anim.get_name(),anim.get_path_name()
  # Report current rig and expression for visual review.
  comp=romeo.get_component_by_class(u.SkeletalMeshComponent);report['romeo_smile']=comp.get_anim_instance().get_curve_value('FaceSmile')
 for name,a in subjects:
  c=a.get_component_by_class(u.SkeletalMeshComponent);bone='Head' if name=='director' else 'head_joint';target=c.get_socket_location(bone);pos=target+a.get_actor_forward_vector()*120+u.Vector(0,0,10)
  if name!='director':
   eye=c.get_socket_location('eyes_joint');delta=eye-target;delta.z=0
   if delta.length()>0.1:pos=target+delta*(120/delta.length())+u.Vector(0,0,5)
   else:pos=target-a.get_actor_forward_vector()*120+u.Vector(0,0,5)
  report[f'{act}_{name}_camera']={'head':list(target.to_tuple()),'camera':list(pos.to_tuple())}
  cam=aes.spawn_actor_from_class(u.SceneCapture2D,pos,u.MathLibrary.find_look_at_rotation(pos,target));cap=cam.get_component_by_class(u.SceneCaptureComponent2D);rt=u.RenderingLibrary.create_render_target2d(world,1000,800,u.TextureRenderTargetFormat.RTF_RGBA8);cap.texture_target=rt;cap.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR;cap.fov_angle=40
  for other in aes.get_all_level_actors():
   if not isinstance(other,(u.Light,u.SceneCapture2D)):other.set_actor_hidden_in_game(other!=a)
  pp=cap.get_editor_property('post_process_settings');pp.set_editor_property('override_auto_exposure_min_brightness',True);pp.set_editor_property('override_auto_exposure_max_brightness',True);pp.set_editor_property('auto_exposure_min_brightness',0.);pp.set_editor_property('auto_exposure_max_brightness',0.);cap.set_editor_property('post_process_settings',pp)
  light=aes.spawn_actor_from_class(u.PointLight,pos+u.Vector(0,0,80));lc=light.get_component_by_class(u.PointLightComponent);lc.set_editor_property('use_inverse_squared_falloff',False);lc.set_editor_property('intensity',10.);lc.set_editor_property('attenuation_radius',3000.);lc.set_editor_property('cast_shadows',False)
  u.SystemLibrary.execute_console_command(world,'Editor.AsyncAssetCompilationFinishAll');cap.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(O),f'act{act}_{name}.png');aes.destroy_actor(cam);aes.destroy_actor(light)
 report[str(act)]={'crew':len(crew),'idle':director.get_editor_property('activity').get_name()}
(O/'validation.json').write_text(json.dumps(report,indent=2));print('REVISION_VISUALS_VALIDATED',report)

