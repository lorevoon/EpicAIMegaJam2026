"""Render a direct director-clock sample for visual QA (run with an RHI)."""
import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1];u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise');aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCPerformer):a.evaluate(35)
 if isinstance(a,(u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard)):a.set_actor_hidden_in_game(True)
camera=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-6140,-9550,1460),u.Rotator(pitch=-23,yaw=64));c=camera.get_component_by_class(u.SceneCaptureComponent2D)
rt=u.RenderingLibrary.create_render_target2d(world,1280,720,u.TextureRenderTargetFormat.RTF_RGBA8);c.texture_target=rt;c.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR;c.fov_angle=55;c.capture_every_frame=False
c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(root/'Saved/Act2'),'Ending.png');print('ACT2_ENDING_RENDERED')
