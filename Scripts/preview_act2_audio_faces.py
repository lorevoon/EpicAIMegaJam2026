"""Render temporary face close-ups and validate the saved soundtrack bindings."""
import unreal as u,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/AudioFaces';BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
audio=[a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue')];assert len(audio)==1
ac=audio[0].get_component_by_class(u.AudioComponent);assert not ac.get_editor_property('auto_activate');assert ac.sound.duration==38
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');tracks=[t for t in seq.get_tracks() if isinstance(t,u.MovieSceneAudioTrack)];assert len(tracks)==1;assert tracks[0].get_sections()[0].get_sound()==ac.sound
speaking=[s.params.animation.get_name() for b in seq.get_bindings() for t in b.get_tracks() if isinstance(t,u.MovieSceneSkeletalAnimationTrack) for s in t.get_sections()];assert sum('Speaking' in n for n in speaking)==3,speaking
cam=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector());c=cam.get_component_by_class(u.SceneCaptureComponent2D);rt=u.RenderingLibrary.create_render_target2d(world,800,700,u.TextureRenderTargetFormat.RTF_RGBA8);c.texture_target=rt;c.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR;c.fov_angle=32
light=aes.spawn_actor_from_class(u.PointLight,u.Vector());lc=light.get_component_by_class(u.PointLightComponent);lc.set_editor_property('use_inverse_squared_falloff',False);lc.set_intensity(5);lc.set_editor_property('attenuation_radius',1500);lc.set_cast_shadows(False)
for who,t,label in [('Romeo',17.13,'romeo_open'),('Romeo',17.28,'romeo_closed'),('Juliet',21,'juliet_terrified'),('Juliet',35,'juliet_crying')]:
 for a in cast.values():a.evaluate(t)
 a=cast[who];mesh=a.get_component_by_class(u.SkeletalMeshComponent);target=mesh.get_socket_location('mouth_joint')+u.Vector(0,0,7);pos=target+a.get_actor_forward_vector()*160+u.Vector(0,0,12)
 if t==35:pos=target+u.Vector(120,-80,90)
 cam.set_actor_location_and_rotation(pos,u.MathLibrary.find_look_at_rotation(pos,target),False,False);light.set_actor_location(pos+u.Vector(0,0,60),False,False);c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(OUT),label+'.png')
for a in cast.values():a.evaluate(0)
aes.destroy_actor(cam);aes.destroy_actor(light)
print('ACT2_AUDIO_BINDINGS_AND_FACE_PREVIEWS_PASSED',speaking)
