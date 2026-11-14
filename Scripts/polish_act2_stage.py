"""Bake stage-two cue transitions and create an editable 38-second Sequencer review cut."""
import unreal as u,json,math,warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FOLDER=BASE+'/Act2';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)]
# Correct mobility on the new scene's wrist-attached weapon props.
for a in cast:
 if a.get_actor_label() not in ['Romeo','Armoured monster']:continue
 comp=a.get_component_by_class(u.SkeletalMeshComponent)
 for part in ['Blade','Guard','Grip']:
  weapon=next(x for x in actors if x.get_actor_label()==a.get_actor_label()+' '+part)
  weapon.static_mesh_component.set_mobility(u.ComponentMobility.MOVABLE)
  weapon.attach_to_component(comp,'R_wrist_joint',u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.KEEP_WORLD,False)
  length=90 if a.get_actor_label()=='Romeo' else 115
  weapon.set_actor_relative_location(u.Vector(0,0,length*.5 if part=='Blade' else -10 if part=='Grip' else 0),False,False)
at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary

def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
rootcache={}
def sample(cue,t,bones,mesh):
 a=cue.animation;length=a.get_play_length();p=max(0,cue.start_offset+(t-cue.start_time)*cue.play_rate);p=p%length if cue.loop else min(length,p)
 result=u.AnimationLibrary.get_bone_poses_for_time(a,bones,p,False,mesh)
 if a.get_editor_property('force_root_lock'):
  key=a.get_path_name()
  if key not in rootcache:rootcache[key]=u.AnimationLibrary.get_bone_poses_for_time(a,[bones[0]],0,False,mesh)[0]
  result[0]=rootcache[key]
 return result
for a in cast:
 if a.get_actor_label().startswith('Fire'):continue
 mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones())];cues=list(a.get_editor_property('animation_cues'))
 name='A_Act2_'+a.get_actor_label().replace(' ','').replace('.','')+'_Performance';path=FOLDER+'/'+name
 if eal.does_asset_exist(path):anim=u.load_asset(path)
 else:
  factory=u.AnimSequenceFactory();factory.target_skeleton=mesh.skeleton;factory.preview_skeletal_mesh=mesh;anim=at.create_asset(name,FOLDER,u.AnimSequence,factory)
 c=anim.get_editor_property('controller');c.open_bracket('38-second continuous stage performance',False);c.set_frame_rate(u.FrameRate(FPS,1),False);c.set_number_of_frames(u.FrameNumber(1140),False);c.remove_all_bone_tracks(False)
 data={str(b):[[],[],[]] for b in bones}
 for b in bones:c.add_bone_track(b,False)
 for frame in range(1141):
  t=frame/FPS;index=max(i for i,q in enumerate(cues) if q.start_time<=t+.00001);q=cues[index];poses=sample(q,t,bones,mesh)
  if index and t-q.start_time<.18:
   previous=sample(cues[index-1],t,bones,mesh);poses=[blend(x,y,smooth((t-q.start_time)/.18)) for x,y in zip(previous,poses)]
  for b,tr in zip(bones,poses):
   dead=(a.get_actor_label()=='Romeo' and t>25) or (a.get_actor_label()=='Mr. Smith' and t>8.5)
   if str(b)=='eyes_joint' and dead:tr.scale3d=u.Vector(1,.055,1)
   data[str(b)][0].append(tr.translation);data[str(b)][1].append(tr.rotation);data[str(b)][2].append(tr.scale3d)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);eal.save_loaded_asset(anim,False)
 cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue])
 print('BAKED',a.get_actor_label())
# Bake actor motion and the full performance into a matching editable review sequence.
name='LS_DC_Act2_38s';path=FOLDER+'/'+name
seq=u.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name,FOLDER,u.LevelSequence,u.LevelSequenceFactoryNew())
seq.set_display_rate(u.FrameRate(FPS,1));seq.set_playback_start(0);seq.set_playback_end(1140)
for b in list(seq.get_bindings()):b.remove()
for tr in list(seq.get_tracks()):seq.remove_track(tr)
for a in cast:
 binding=seq.add_possessable(a);binding.set_name(a.get_actor_label());track=binding.add_track(u.MovieScene3DTransformTrack);section=track.add_section();section.set_range(0,1141);channels=section.get_all_channels()
 for f in range(0,1141,3):
  a.evaluate(f/FPS);p=a.get_actor_location();r=a.get_actor_rotation();vals=[p.x,p.y,p.z,r.roll,r.pitch,r.yaw,1,1,1]
  for ch,val in zip(channels,vals):ch.add_key(u.FrameNumber(f),float(val),interpolation=u.MovieSceneKeyInterpolation.LINEAR)
 comp=a.get_component_by_class(u.SkeletalMeshComponent);cb=seq.add_possessable(comp);cb.set_parent(binding);track=cb.add_track(u.MovieSceneSkeletalAnimationTrack);section=track.add_section();section.set_range(0,1141)
 params=section.params;params.animation=a.get_editor_property('animation_cues')[0].animation;section.params=params
 a.evaluate(0)
# Five authored camera views, editable by the user in Sequencer.
shots=[(0,5,(-620,-950,390),(-40,10,110)),(5,12,(-420,-650,240),(20,0,80)),(12,22,(-250,-720,260),(50,-180,115)),(22,29,(-280,-650,200),(-10,-180,60)),(29,38,(-430,-510,170),(-150,-220,55))]
origin=u.Vector(-5750,-9000,0)
ground=next(a for a in cast if a.get_actor_label()=='Romeo').get_actor_location().z
cuts=seq.add_track(u.MovieSceneCameraCutTrack)
for i,(start,end,offset,target) in enumerate(shots):
 loc=origin+u.Vector(offset[0],offset[1],ground+offset[2]);look=origin+u.Vector(target[0],target[1],ground+target[2]);rot=u.MathLibrary.find_look_at_rotation(loc,look)
 cam=aes.spawn_actor_from_class(u.CineCameraActor,loc,rot);cam.set_actor_label('Act2 reference camera '+str(i+1));cam.set_folder_path('Act2/Reference cameras')
 cam.get_cine_camera_component().set_editor_property('current_focal_length',35.)
 focus=cam.get_cine_camera_component().focus_settings;focus.focus_method=u.CameraFocusMethod.DISABLE;cam.get_cine_camera_component().focus_settings=focus
 binding=seq.add_possessable(cam);section=cuts.add_section();section.set_range(start*FPS,end*FPS);bid=u.MovieSceneObjectBindingID();bid.set_editor_property('guid',binding.get_id());section.set_camera_binding_id(bid)
# Fade belongs only to the authored review cut; gameplay keeps the final scored pose visible.
fade=seq.add_track(u.MovieSceneFadeTrack).add_section();fade.set_range(0,1141);ch=fade.get_all_channels()[0];ch.add_key(u.FrameNumber(0),0.);ch.add_key(u.FrameNumber(1110),0.);ch.add_key(u.FrameNumber(1140),1.)
for t,label in [(0,'Fire'),(5,'Mr. Smith'),(12,'Duel'),(22,'Romeo falls'),(29,'Juliet mourns')]:seq.add_marked_frame(u.MovieSceneMarkedFrame(frame_number=u.FrameNumber(t*FPS),label=label))
assert eal.save_loaded_asset(seq,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
print('ACT2_POLISH_COMPLETE')

