"""Refresh the editable sequence and verify raid paths and the new supplied poses."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast}
for a in actors:
 if isinstance(a,u.DCClapperboard):a.set_actor_location(u.Vector(-4900,-9900,1200),False,False)
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');assert seq
for b in list(seq.get_bindings()):b.remove()
for t in list(seq.get_tracks()):
 if not isinstance(t,u.MovieSceneAudioTrack):seq.remove_track(t)
for a in cast:
 b=seq.add_possessable(a);b.set_name(a.get_actor_label());sec=b.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);channels=sec.get_all_channels()
 for f in range(0,1141,3):
  a.evaluate(f/30);p=a.get_actor_location();r=a.get_actor_rotation()
  for ch,v in zip(channels,[p.x,p.y,p.z,r.roll,r.pitch,r.yaw,1,1,1]):ch.add_key(u.FrameNumber(f),v,interpolation=u.MovieSceneKeyInterpolation.LINEAR)
 cb=seq.add_possessable(a.get_component_by_class(u.SkeletalMeshComponent));cb.set_parent(b);sec=cb.add_track(u.MovieSceneSkeletalAnimationTrack).add_section();sec.set_range(0,1141);params=sec.params;params.animation=a.get_editor_property('animation_cues')[0].animation;sec.params=params
cuts=seq.add_track(u.MovieSceneCameraCutTrack)
shots=[(0,6,(-5794,-8212,1865),['Armoured monster','Raid orc 1','Raid orc 4']),(6,10,(-6064,-8257,1421),['Mr. Smith','Armoured monster']),(10,18,(-4550,-9250,1390),['Romeo','Juliet']),(18,23.5,(-5500,-9680,1480),['Romeo','Armoured monster']),(23.5,29,(-6400,-9800,1480),['Romeo','Armoured monster']),(29,38,(-5750,-10100,1231),['Juliet','Romeo'])]
for i,(start,end,point,names) in enumerate(shots):
 cam=next(a for a in actors if a.get_actor_label()=='Raid cut camera '+str(i+1));cam.set_actor_location(u.Vector(*point),False,False);b=seq.add_possessable(cam);sec=cuts.add_section();sec.set_range(round(start*30),round(end*30));bid=u.MovieSceneObjectBindingID();bid.set_editor_property('guid',b.get_id());sec.set_camera_binding_id(bid);sec=b.add_track(u.MovieScene3DTransformTrack).add_section();sec.set_range(0,1141);channels=sec.get_all_channels()
 for f in range(round(start*30),round(end*30)+1,3):
  target=u.Vector()
  for name in names:
   a=by[name];a.evaluate(f/30);target+=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint')
  target/=len(names);rot=u.MathLibrary.find_look_at_rotation(u.Vector(*point),target);cam.set_actor_rotation(rot,False)
  for ch,v in zip(channels,[*point,rot.roll,rot.pitch,rot.yaw,1,1,1]):ch.add_key(u.FrameNumber(f),float(v),interpolation=u.MovieSceneKeyInterpolation.LINEAR)
fade=seq.add_track(u.MovieSceneFadeTrack).add_section();fade.set_range(0,1141);ch=fade.get_all_channels()[0]
for f,v in [(0,0.),(1110,0.),(1140,1.)]:ch.add_key(u.FrameNumber(f),v)
for a in cast:a.evaluate(0)
assert eal.save_loaded_asset(seq,False);assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Test clear paths against both the studio practicals and the village, using a body-radius sweep.
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(p):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(p.x,p.y,7000),u.Vector(p.x,p.y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);assert h;return h.to_tuple()[4].z
raiders=[a for a in cast if 'orc' in a.get_actor_label().lower() or a.get_actor_label()=='Armoured monster'];hits=[];motion={}
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.LandscapeProxy))]
for real in [False,True]:
 for name,vis in [('L_DC_Act2_Set',not real),('L_DC_Act2_Real',real)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,name).get_loaded_level(),vis,False)
 for a in raiders:
  previous=None;max_backstep=0;max_jump=0
  for frame in range(181):
   t=frame/30;a.evaluate(t);p=a.get_actor_location();c=a.get_component_by_class(u.SkeletalMeshComponent);body=c.get_socket_location('pelvis_joint')
   if previous:
    travel=p-previous[0];delta=body-previous[1];max_jump=max(max_jump,delta.length());max_backstep=min(max_backstep,delta.dot(travel.normal()) if travel.length()>.01 else 0)
   previous=(p,body)
   if frame%6==0:
    z=height(p);h=u.SystemLibrary.sphere_trace_single(world,u.Vector(p.x,p.y,z+90),u.Vector(p.x,p.y,z+100),45,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
    if h:hits.append({'real':real,'actor':a.get_actor_label(),'time':t,'hit':str(h.to_tuple()[9])})
  motion[a.get_actor_label()]={'largest_frame_step_cm':max_jump,'largest_backstep_cm':max_backstep}
poses={}
for t in [11,20,23,25.5,28.5,30,32,35,38]:
 poses[t]={}
 for a in cast:
  a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);poses[t][a.get_actor_label()]={b:list(c.get_socket_location(b).to_tuple()) for b in ['head_joint','pelvis_joint','R_wrist_joint','L_wrist_joint']}
report={'duration':38,'path_collisions':hits,'entrance_motion':motion,'poses':poses}
(ROOT/'Saved/Act2/Revision3/sequence_validation.json').write_text(json.dumps(report,indent=2));print('REVISION3_SEQUENCE_READY',len(hits),'path contacts',motion)
