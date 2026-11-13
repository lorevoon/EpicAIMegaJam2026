"""Export an unsaved cutaway overview for visual review."""
import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
camera=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-3000,-12600,4600),u.Rotator(pitch=-29,yaw=137))
c=camera.get_component_by_class(u.SceneCaptureComponent2D)
rt=u.RenderingLibrary.create_render_target2d(world,1600,1100,u.TextureRenderTargetFormat.RTF_RGBA8)
c.set_editor_property('texture_target',rt)
c.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
c.set_editor_property('fov_angle',75.)
for a in list(aes.get_all_level_actors()):
    if a.get_actor_label() in ['Studio roof','Green screen - front','Green screen - right']:aes.destroy_actor(a)
c.capture_scene()
u.RenderingLibrary.export_render_target(world,rt,str(root/'Saved/FilmSetAudit'),'compact_studio.png')
u.log('COMPACT_PREVIEW_EXPORTED')
