"""Render current saved performances at fight and ending checkpoints; no asset writes."""
import unreal as u
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/Flow/Preview';OUT.mkdir(parents=True,exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
visibility=[]
for name,on in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:
 level=u.GameplayStatics.get_streaming_level(world,name).get_loaded_level()
 visibility.append((level,level.get_editor_property('is_visible')))
 u.EditorLevelUtils.set_level_visibility(level,on,False)
cam=aes.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-5750,-10100,1231))
c=cam.get_component_by_class(u.SceneCaptureComponent2D)
rt=u.RenderingLibrary.create_render_target2d(world,1280,720,u.TextureRenderTargetFormat.RTF_RGBA8)
c.texture_target=rt;c.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR;c.fov_angle=60;c.capture_every_frame=False
try:
 for t in [18.8,19.6,20.6,21.3,22.6,23.7,24.5,25.55,27,29.5,31.5,35]:
  for a in cast.values():a.evaluate(t)
  names=['Romeo','Armoured monster'] if t<29 else ['Romeo','Juliet']
  target=u.Vector()
  for who in names:target+=cast[who].get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
  target/=2
  pos=u.Vector(-5750,-10100,1231)
  cam.set_actor_location_and_rotation(pos,u.MathLibrary.find_look_at_rotation(pos,target),False,False)
  c.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(OUT),'frame_'+str(t).replace('.','_')+'.png')
finally:
 for a in cast.values():a.evaluate(0)
 aes.destroy_actor(cam)
 for level,visible in visibility:u.EditorLevelUtils.set_level_visibility(level,visible,False)
print('ACT2_FLOW_PREVIEWS_READY')
