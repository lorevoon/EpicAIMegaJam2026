"""Author actual arrow trajectories, separated film cameras and the editable cut."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/BowRevision6';OUT=ROOT/'Saved/Act2/StoryRevision10';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')}
for a in list(actors):
 if a.actor_has_tag('DCBowArrow'):
  for w in list(aes.get_all_level_actors()):
   if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:aes.destroy_actor(w)
  aes.destroy_actor(a)
arrows=[];evidence=[];j=cast['Juliet'];arrowmesh=u.load_asset(F+'/SM_ArrowBasic');assert arrowmesh
for index,(release,impact,who) in enumerate([(17.8,18.1,'Raid orc 1'),(20.2,20.5,'Raid orc 2')],1):
 j.evaluate(release);start=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('L_wrist_joint');victim=cast[who];victim.evaluate(impact);hit=victim.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');direction=(hit-start).normal();tip=38.9398
 # Mesh origin is its centre: at impact its TIP meets the victim, then the shaft passes through.
 origin=start-direction*10;contact=hit-direction*tip;exitpoint=hit+direction*140;rot=u.MathLibrary.find_look_at_rotation(origin,hit)
 a=aes.spawn_actor_from_class(u.DCPerformer,origin,rot);a.set_actor_label(f'Juliet arrow {index}');a.tags=['DCBowArrow'];a.set_folder_path('Act2/Arrows');a.set_editor_property('prop_only',True);a.set_editor_property('carried_mesh',arrowmesh);a.set_editor_property('carry_transform',u.Transform(rotation=u.Rotator(yaw=-90),scale=u.Vector(1.5,1,1.5)));a.set_editor_property('prop_visible_from',release-.65);a.set_editor_property('prop_visible_until',impact+.17);a.set_editor_property('linear_movement',True);a.set_editor_property('use_waypoint_height',True)
 for step in range(20):
  t=release-.65+step*.65/20;j.evaluate(t);hand=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('L_wrist_joint');d=(hit-hand).normal();p=hand-d*10;r=u.MathLibrary.find_look_at_rotation(p,hit);w=aes.spawn_actor_from_class(u.DCWaypoint,p,u.Rotator(yaw=r.yaw));w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_editor_property('tilt',r.pitch);w.set_actor_label(f'Arrow {index} nocked {step}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
 # Explicit pre-release key prevents interpolation from scene start; pitch follows the ballistic line.
 for t,p in [(release,origin),(impact,contact),(impact+.17,exitpoint),(38,exitpoint)]:
  w=aes.spawn_actor_from_class(u.DCWaypoint,p,u.Rotator(yaw=rot.yaw));w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_editor_property('tilt',rot.pitch);w.set_actor_label(f'Arrow {index} @ {t}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
 arrows.append(a);evidence.append({'release':release,'impact':impact,'target':who,'start':list(origin.to_tuple()),'head':list(hit.to_tuple()),'centre_at_impact':list(contact.to_tuple()),'direction':list(direction.to_tuple()),'tip_offset':tip})
beats={str(a.get_editor_property('beat_id')):a for a in actors if isinstance(a,u.DCBeat)}
ref=u.DCAnchorRef();ref.set_editor_property('performer',cast['Juliet']);ref.set_editor_property('part',u.DCAnchorPart.HEAD);beats['2.3'].set_editor_property('anchors',[ref])
beats['2.4'].set_editor_property('window_end',26);beats['2.5'].set_editor_property('window_start',26);beats['2.5'].set_editor_property('display_name',"Romeo's death");beats['2.5'].set_editor_property('caption',"Juliet reaches Romeo's right side as he falls, then kneels beside him.")
timing=json.loads((ROOT/'SourceArt/Act2/StoryRevision10/Audio/timing.json').read_text());windows=[(max(0,l['start']-.1),min(38,l['end']+.1)) for l in timing['lines']]+[(26,38)]
# Five non-overlapping camera stations, at least 13 metres apart, including the final shot.
shots=[(0,10,'2.1',(-4950,-8900,1200),66,58,4,8,['Armoured monster','Raid orc 1','Raid orc 4','Mr. Smith']),
 (10,463/30,'2.2',(-6500,-9800,1470),20,26,12,15.3,['Romeo','Juliet']),
 (463/30,19,'2.3',(-4700,-10800,1250),20,26,17.6,18.9,['Juliet']),
 (19,26,'2.4',(-4400,-7650,1430),20,32,23.3,26,['Romeo','Armoured monster']),
 (26,38,'2.5',(-5900,-7800,1600),38,20,29.5,36.8,['Romeo','Juliet'])]
solution=json.loads((OUT/'camera_solution.json').read_text())
shots=[tuple(list(s[:3])+[tuple(solution[i]['point'])]+list(s[4:])) for i,s in enumerate(shots)]
for a in actors:
 if isinstance(a,u.CameraActor) and a.actor_has_tag('DCReferenceShot'):a.tags=[]
cameras=[]
for i,(start,end,beat,point,fov,endfov,zstart,zend,names) in enumerate(shots,1):
 cam=next((a for a in actors if a.get_actor_label()==f'Raid cut camera {i}'),None) or aes.spawn_actor_from_class(u.CameraActor,u.Vector(*point));cam.set_actor_label(f'Raid cut camera {i}');cam.set_actor_location(u.Vector(*point),False,False);cam.get_component_by_class(u.CameraComponent).set_field_of_view(fov);tags=['DCReferenceShot',f'DCStart={start}',f'DCBeat={beat}',f'DCFOV={fov}',f'DCEndFOV={endfov}',f'DCZoomStart={zstart}',f'DCZoomEnd={zend}']
 if i==1:tags += [f'DCDialogue={s}:{e}' for s,e in windows]
 if i==1:
  end=solution[0]['end'];tags += [f'DCEndX={end[0]}',f'DCEndY={end[1]}',f'DCEndZ={end[2]}','DCMoveStart=3.5','DCMoveEnd=7.5','DCFocusActor=RaidSmithFocus','DCFocusStart=4','DCFocusEnd=7.5']
 if i==4:tags+=['DCSlowStart=25.0','DCSlowEnd=25.9','DCSlowRate=0.65']
 cam.tags=tags;cameras.append(cam);b=beats[beat];b.set_editor_property('camera_spot',b.get_actor_transform().inverse_transform_location(u.Vector(*point)))
spacing=min(math.dist(a[3][:2],b[3][:2]) for i,a in enumerate(shots) for b in shots[i+1:]);assert spacing>=1300,spacing
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Reload so newly authored arrow keys become the same deterministic keys used at runtime.
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());allcast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in allcast}
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');assert seq
for b in list(seq.get_bindings()):b.remove()
for t in list(seq.get_tracks()):seq.remove_track(t)
def transform_track(binding,rows):
 section=binding.add_track(u.MovieScene3DTransformTrack).add_section();section.set_range(0,1141);channels=section.get_all_channels()
 for f,tf in rows:
  p=tf.translation;r=tf.rotation.rotator();s=tf.scale3d
  for ch,v in zip(channels,[p.x,p.y,p.z,r.roll,r.pitch,r.yaw,s.x,s.y,s.z]):ch.add_key(u.FrameNumber(f),float(v),interpolation=u.MovieSceneKeyInterpolation.LINEAR)
for a in allcast:
 b=seq.add_possessable(a);b.set_name(a.get_actor_label());rows=[];prop_rows=[];isprop=a.get_editor_property('prop_only');hasprop=bool(a.get_editor_property('carried_mesh'));prop=next((c for c in a.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Prop'),None)
 for f in range(1141):
  a.evaluate(f/30);rows.append((f,a.get_actor_transform()))
  if hasprop:prop_rows.append((f,prop.get_relative_transform()))
 transform_track(b,rows)
 if not isprop:
  cb=seq.add_possessable(a.get_component_by_class(u.SkeletalMeshComponent));cb.set_parent(b);section=cb.add_track(u.MovieSceneSkeletalAnimationTrack).add_section();section.set_range(0,1141);params=section.params;params.animation=a.get_editor_property('animation_cues')[0].animation;section.params=params
 if hasprop:
  pb=seq.add_possessable(prop);pb.set_parent(b);transform_track(pb,prop_rows);track=pb.add_track(u.MovieSceneBoolTrack);track.set_property_name_and_path('bVisible','bVisible');section=track.add_section();section.set_range(0,1141);ch=section.get_all_channels()[0];ch.set_default(False)
  for time,value in [(0,False),(a.get_editor_property('prop_visible_from'),True),(a.get_editor_property('prop_visible_until'),False)]:ch.add_key(u.FrameNumber(round(time*30)),value)
cuts=seq.add_track(u.MovieSceneCameraCutTrack)
for i,(start,end,beat,point,fov,endfov,zstart,zend,names) in enumerate(shots,1):
 cam=next(a for a in actors if a.get_actor_label()==f'Raid cut camera {i}');b=seq.add_possessable(cam);cut=cuts.add_section();cut.set_range(round(start*30),round(end*30));bid=u.MovieSceneObjectBindingID();bid.set_editor_property('guid',b.get_id());cut.set_camera_binding_id(bid);rows=[];component=cam.get_component_by_class(u.CameraComponent);cb=seq.add_possessable(component);cb.set_parent(b);track=cb.add_track(u.MovieSceneFloatTrack);prop='CurrentFocalLength' if isinstance(component,u.CineCameraComponent) else 'FieldOfView';track.set_property_name_and_path(prop,prop);section=track.add_section();section.set_range(0,1141);ch=section.get_all_channels()[0];previous=None
 for f in range(round(start*30),round(end*30)+1):
  t=f/30;target=u.Vector()
  for name in names:
   a=by[name];a.evaluate(t);target+=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
  target/=len(names);position=u.Vector(*point)
  if i==1:
   weight=max(0,min(1,(t-4)/3.5));weight=weight*weight*(3-2*weight);smith=by['Mr. Smith'];smith.evaluate(t);target=target*(1-weight)+smith.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')*weight
   move=max(0,min(1,(t-3.5)/4));move=move*move*(3-2*move);position=position*(1-move)+u.Vector(*solution[0]['end'])*move
  rot=u.MathLibrary.find_look_at_rotation(position,target)
  if previous:rot=u.MathLibrary.r_interp_to(previous,rot,1/30,6.)
  previous=rot;rows.append((f,u.Transform(location=position,rotation=rot)));alpha=max(0,min(1,(t-zstart)/(zend-zstart)));alpha=alpha*alpha*(3-2*alpha);lens=fov+(endfov-fov)*alpha;value=component.filmback.sensor_width/(2*math.tan(math.radians(lens)/2)) if isinstance(component,u.CineCameraComponent) else lens;ch.add_key(u.FrameNumber(f),value,interpolation=u.MovieSceneKeyInterpolation.LINEAR)
 transform_track(b,rows)
sound=u.load_asset(BASE+'/Act2/StoryRevision10/SW_Story');section=seq.add_track(u.MovieSceneAudioTrack).add_section();section.set_sound(sound);section.set_range(0,1140)
fade=seq.add_track(u.MovieSceneFadeTrack).add_section();fade.set_range(0,1141);ch=fade.get_all_channels()[0]
for f,v in [(0,0.),(1125,0.),(1140,1.)]:ch.add_key(u.FrameNumber(f),v)
for a in allcast:a.evaluate(0)
assert eal.save_loaded_asset(seq,False);assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'film.json').write_text(json.dumps({'arrows':evidence,'shots':shots,'minimum_camera_spacing_cm':spacing,'dialogue_windows':windows},indent=2));print('BOW_REFERENCE_FILM_READY',spacing)
