"""Bake current acting and continuous camera/zoom into the editable 38s sequence."""
import unreal as u,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast}
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');assert seq
for b in list(seq.get_bindings()):b.remove()
for t in list(seq.get_tracks()):
 if not isinstance(t,u.MovieSceneAudioTrack):seq.remove_track(t)
for a in cast:
 b=seq.add_possessable(a);b.set_name(a.get_actor_label());sec=b.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);channels=sec.get_all_channels()
 for f in range(1141):
  a.evaluate(f/30);p=a.get_actor_location();r=a.get_actor_rotation()
  for ch,v in zip(channels,[p.x,p.y,p.z,r.roll,r.pitch,r.yaw,1,1,1]):ch.add_key(u.FrameNumber(f),v,interpolation=u.MovieSceneKeyInterpolation.LINEAR)
 cb=seq.add_possessable(a.get_component_by_class(u.SkeletalMeshComponent));cb.set_parent(b);sec=cb.add_track(u.MovieSceneSkeletalAnimationTrack).add_section();sec.set_range(0,1141);params=sec.params;params.animation=a.get_editor_property('animation_cues')[0].animation;sec.params=params
def tags(a):return {str(t).split('=',1)[0]:str(t).split('=',1)[1] for t in a.tags if '=' in str(t)}
cams=sorted([a for a in actors if isinstance(a,u.CameraActor) and a.actor_has_tag('DCReferenceShot')],key=lambda a:float(tags(a)['DCStart']))
assert len(cams)==5
subjects=[['Armoured monster','Raid orc 1','Raid orc 4'],['Mr. Smith','Armoured monster'],['Romeo','Juliet'],['Romeo','Armoured monster'],['Juliet','Romeo']]
cuts=seq.add_track(u.MovieSceneCameraCutTrack)
previous_rotation=None;bound=None
for i,cam in enumerate(cams):
 tag=tags(cam);start=float(tag['DCStart']);end=float(tags(cams[i+1])['DCStart']) if i+1<len(cams) else 38.
 if not cam.actor_has_tag('DCContinueShot'):
  bound=seq.add_possessable(cam);cut=cuts.add_section();cut.set_range(round(start*30),1140 if i==3 else round(end*30));bid=u.MovieSceneObjectBindingID();bid.set_editor_property('guid',bound.get_id());cut.set_camera_binding_id(bid)
  sec=bound.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);channels=sec.get_all_channels()
  component=cam.get_component_by_class(u.CameraComponent);cb=seq.add_possessable(component);cb.set_parent(bound)
  track=cb.add_track(u.MovieSceneFloatTrack);prop='CurrentFocalLength' if isinstance(component,u.CineCameraComponent) else 'FieldOfView';track.set_property_name_and_path(prop,prop);lens=track.add_section();lens.set_range(0,1141);zoom_channel=lens.get_all_channels()[0];previous_rotation=None
 pos=cam.get_actor_location()
 for f in range(round(start*30),round(end*30)+(1 if i==4 else 0)):
  time=f/30;target=u.Vector()
  for name in subjects[i]:
   a=by[name];a.evaluate(time);target+=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
  target/=len(subjects[i])
  if cam.actor_has_tag('DCContinueShot') and time<start+1.5:
   previous_target=u.Vector()
   for name in subjects[i-1]:
    a=by[name];a.evaluate(time);previous_target+=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
   previous_target/=len(subjects[i-1]);weight=max(0,min(1,(time-start)/1.5));weight=weight*weight*(3-2*weight);target=previous_target*(1-weight)+target*weight
  rot=u.MathLibrary.find_look_at_rotation(pos,target)
  if previous_rotation is not None:rot=u.MathLibrary.r_interp_to(previous_rotation,rot,1/30,6.)
  previous_rotation=rot
  for ch,v in zip(channels,[pos.x,pos.y,pos.z,rot.roll,rot.pitch,rot.yaw,1,1,1]):ch.add_key(u.FrameNumber(f),float(v),interpolation=u.MovieSceneKeyInterpolation.LINEAR)
  alpha=max(0,min(1,(time-float(tag['DCZoomStart']))/(float(tag['DCZoomEnd'])-float(tag['DCZoomStart']))));alpha=alpha*alpha*(3-2*alpha);fov=float(tag['DCFOV'])+(float(tag['DCEndFOV'])-float(tag['DCFOV']))*alpha
  value=component.filmback.sensor_width/(2*math.tan(math.radians(fov)/2)) if isinstance(component,u.CineCameraComponent) else fov
  zoom_channel.add_key(u.FrameNumber(f),value,interpolation=u.MovieSceneKeyInterpolation.LINEAR)
fade=seq.add_track(u.MovieSceneFadeTrack).add_section();fade.set_range(0,1141);ch=fade.get_all_channels()[0]
for f,v in [(0,0.),(1125,0.),(1140,1.)]:ch.add_key(u.FrameNumber(f),v)
# The newly mixed soundtrack is assigned to the scene by the audio revision.
audio=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue'));sound=audio.get_component_by_class(u.AudioComponent).sound
for track in seq.get_tracks():
 if isinstance(track,u.MovieSceneAudioTrack):
  for section in track.get_sections():section.set_sound(sound);section.set_range(0,1140)
for a in cast:a.evaluate(0)
assert len(cuts.get_sections())==4
assert eal.save_loaded_asset(seq,False);assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
print('ACT2_REVISION4_SEQUENCE_SAVED',sound.get_path_name())
