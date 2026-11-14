"""Finish raid staging, ground the three deaths, and author a matching review sequence."""
import unreal as u,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
# Open the copied soundstage's internal partitions for the troop's entrance lane.
assert les.load_level(BASE+'/Levels/L_DC_Act2_Set')
for a in list(aes.get_all_level_actors()):
 if a.get_actor_label() in ['Green screen - backstage return','Green screen - backstage end']:aes.destroy_actor(a)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert u.EditorLoadingAndSavingUtils.save_map(world,BASE+'/Levels/L_DC_Act2_Set')
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast};ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 r=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert r[0];return r[4].z
alignment={}
for who,start,end in [('Mr. Smith',7.2,9.2),('Armoured monster',25.8,27.8),('Romeo',26.3,28.3)]:
 a=by[who];a.evaluate(30);p=a.get_actor_location();head=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');slope=(ground(head.x,head.y)-ground(p.x,p.y))/max(1,math.hypot(head.x-p.x,head.y-p.y));tilt=-math.degrees(math.atan(slope));alignment[who]=tilt
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and w.get_editor_property('time')>=end:
   w.set_editor_property('tilt',tilt);q=w.get_actor_location();q.z=ground(q.x,q.y)+6;w.set_actor_location(q,False,False)
 for t,angle,lift in [(start,0,3),(end,tilt,6)]:
  w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(p.x,p.y,ground(p.x,p.y)+lift),u.Rotator(yaw=a.get_actor_rotation().yaw));w.set_actor_label(who+' death alignment '+str(t));w.set_folder_path('Act2/Blocking');w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_editor_property('tilt',angle);w.set_is_temporarily_hidden_in_editor(True)
for a in cast:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast}
# Fresh sequence avoids invalid legacy fire bindings and retains the earlier cut as history.
name='LS_DC_Act2_OrcRaid';path=FOLDER+'/'+name;seq=u.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name,FOLDER,u.LevelSequence,u.LevelSequenceFactoryNew());seq.set_display_rate(u.FrameRate(30,1));seq.set_playback_start(0);seq.set_playback_end(1140)
for b in list(seq.get_bindings()):b.remove()
for t in list(seq.get_tracks()):seq.remove_track(t)
for a in cast:
 binding=seq.add_possessable(a);binding.set_name(a.get_actor_label());sec=binding.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);ch=sec.get_all_channels()
 for f in range(0,1141,3):
  a.evaluate(f/30);p=a.get_actor_location();r=a.get_actor_rotation()
  for channel,value in zip(ch,[p.x,p.y,p.z,r.roll,r.pitch,r.yaw,1,1,1]):channel.add_key(u.FrameNumber(f),value,interpolation=u.MovieSceneKeyInterpolation.LINEAR)
 cb=seq.add_possessable(a.get_component_by_class(u.SkeletalMeshComponent));cb.set_parent(binding);sec=cb.add_track(u.MovieSceneSkeletalAnimationTrack).add_section();sec.set_range(0,1141);params=sec.params;params.animation=a.get_editor_property('animation_cues')[0].animation;sec.params=params;a.evaluate(0)
cuts=seq.add_track(u.MovieSceneCameraCutTrack)
shots=[(0,6,(-5600,-9300,1530),['Armoured monster','Raid orc 1','Raid orc 4']),(6,10,(-5350,-9400,1550),['Mr. Smith','Armoured monster']),(10,19,(-4400,-9200,1450),['Romeo','Juliet']),(19,29,(-5480,-9700,1580),['Romeo','Armoured monster']),(29,38,(-5300,-9500,1450),['Juliet','Romeo'])]
for i,(start,end,point,names) in enumerate(shots):
 cam=aes.spawn_actor_from_class(u.CineCameraActor,u.Vector(*point));cam.set_actor_label('Raid cut camera '+str(i+1));cam.set_folder_path('Act2/Raid reference cameras');cc=cam.get_cine_camera_component();cc.set_editor_property('current_focal_length',30. if i==0 else 35.);focus=cc.focus_settings;focus.focus_method=u.CameraFocusMethod.DISABLE;cc.focus_settings=focus
 binding=seq.add_possessable(cam);sec=cuts.add_section();sec.set_range(start*30,end*30);bid=u.MovieSceneObjectBindingID();bid.set_editor_property('guid',binding.get_id());sec.set_camera_binding_id(bid)
 sec=binding.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);channels=sec.get_all_channels()
 for f in range(start*30,end*30+1,3):
  target=u.Vector()
  for name in names:
   a=by[name];a.evaluate(f/30);target+=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
  target/=len(names);rot=u.MathLibrary.find_look_at_rotation(u.Vector(*point),target);cam.set_actor_rotation(rot,False)
  for ch,val in zip(channels,[*point,rot.roll,rot.pitch,rot.yaw,1,1,1]):ch.add_key(u.FrameNumber(f),float(val),interpolation=u.MovieSceneKeyInterpolation.LINEAR)
fade=seq.add_track(u.MovieSceneFadeTrack).add_section();fade.set_range(0,1141);ch=fade.get_all_channels()[0]
for f,value in [(0,0.),(1110,0.),(1140,1.)]:ch.add_key(u.FrameNumber(f),value)
for t,label in [(0,'Orc raid'),(6,'Mr. Smith falls'),(10,'Gasp and rush'),(19,'Fatal victory'),(29,'Juliet mourns')]:seq.add_marked_frame(u.MovieSceneMarkedFrame(frame_number=u.FrameNumber(t*30),label=label))
for a in cast:a.evaluate(0)
assert eal.save_loaded_asset(seq,False);assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# A curtain of fire behind the troop, entirely in the real/replay layer.
assert les.load_level(BASE+'/Levels/L_DC_Act2_Real');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for i,(x,y,z) in enumerate([(-7250,-10300,1600),(-6900,-10400,1450),(-7600,-9750,1700)]):
 a=aes.spawn_actor_from_class(u.NiagaraActor,u.Vector(x,y,z));a.set_actor_label('Orc entrance fire curtain '+str(i));a.set_folder_path('Act2/Real fire');a.tags=['DC_Act2RealFire','DC_RealEnvironment'];a.set_actor_scale3d(u.Vector(1.8,1.8,1.8));a.get_component_by_class(u.NiagaraComponent).set_asset(u.load_asset(FOLDER+'/NS_Act2_VillageInferno'))
assert u.EditorLoadingAndSavingUtils.save_map(world,BASE+'/Levels/L_DC_Act2_Real')
(ROOT/'Saved/Act2/raid_finish.json').write_text(json.dumps({'sequence':path,'death_alignment':alignment,'real_fire_count':46},indent=2));print('ACT2_RAID_FINISHED')
