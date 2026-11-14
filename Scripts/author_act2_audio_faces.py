"""Attach scene-clock audio and bake facial acting without changing body animation."""
import unreal as u,json,math,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FOLDER=BASE+'/Act2/AudioFacesV3';FPS=30
OUT=ROOT/'Saved/Act2/AudioFaces';OUT.mkdir(parents=True,exist_ok=True)
manifest=json.loads((ROOT/'SourceArt/Act2/Audio/timing.json').read_text())
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
for rel in ['Levels/L_DC_Act2_BrokenPromise.umap','Act2/LS_DC_Act2_OrcRaid.uasset']:
 dest=OUT/(Path(rel).name+'.before');src=ROOT/'Content/DirectorsCut'/rel
 if not dest.exists():shutil.copy2(src,dest)
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/Audio/Act2_Soundtrack.wav');task.destination_path=FOLDER;task.destination_name='SW_Act2_Soundtrack';task.automated=True;task.replace_existing=True;task.save=True
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=u.load_asset(FOLDER+'/SW_Act2_Soundtrack');assert sound
sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM);sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE);assert eal.save_loaded_asset(sound,False);assert abs(sound.duration-38)<.001
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act2_BrokenPromise')
ambient=next((a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue')),None)
if not ambient:ambient=aes.spawn_actor_from_class(u.AmbientSound,u.Vector())
ambient.set_actor_label('Act 2 - synchronized voices fire and combat');ambient.tags=['DCSceneDialogue'];ambient.set_folder_path('Act2/Audio')
ac=ambient.get_component_by_class(u.AudioComponent);ac.set_sound(sound);ac.set_editor_property('auto_activate',False);ac.set_editor_property('allow_spatialization',False);ac.set_editor_property('is_ui_sound',True);ac.stop()
def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
report={'sound':sound.get_path_name(),'duration':sound.duration,'cast':{},'preview':{}}
bones=['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint']
for who in ['Romeo','Juliet','Mr. Smith']:
 p=cast[who];mesh=p.get_editor_property('character_mesh');source=p.get_editor_property('animation_cues')[0].animation
 path=FOLDER+'/A_Act2_'+who.replace(' ','').replace('.','')+'_Speaking'
 anim=u.load_asset(path) or eal.duplicate_asset(source.get_path_name(),path);assert anim
 poses=[u.AnimationLibrary.get_bone_poses_for_time(source,bones,f/FPS,False,mesh) for f in range(1141)];data={b:[[],[],[]] for b in bones};opens=[];closed=[]
 for f,row in enumerate(poses):
  t=f/FPS;opening=0;gate=0
  for line in manifest['lines']:
   if line['speaker']!=who or not line['start']<=t<line['end']:continue
   g=max(line['mouth_gate'][max(0,f-1):min(1141,f+2)]);g*=min(1,(t-line['start'])/.07,(line['end']-t)/.07)
   pulse=.5-.5*math.cos((t-line['start'])*2*math.pi*(2.6 if line['name']=='Sobbing' else 3.6));opening=max(opening,pulse*g);gate=max(gate,g)
  fear=smooth((t-(9.5 if who!='Mr. Smith' else -.5))/.5)*(1-smooth((t-(28 if who!='Juliet' else 30))/1))
  crying=smooth((t-27.5)/.7) if who=='Juliet' else 0
  dead=smooth((t-(33 if who=='Romeo' else 8.5))/.4) if who!='Juliet' else 0
  shut=max(crying,dead)
  for b,tr in zip(bones,row):
   if b=='mouth_joint':tr.scale3d=u.Vector(1-.08*opening,max(.12,(1-gate)*.75+gate*(.18+1.65*opening)),1)
   if b=='eyes_joint':tr.scale3d=u.Vector(1,max(.055,1-.945*shut),1)
   if b in ['L_eyebrow_joint','R_eyebrow_joint']:
    pos=tr.translation;pos.x-=.65*max(fear,.65*crying);tr.translation=pos
    # Opposing brow tilts plus the lowered brow line create a concerned frown.
    delta=u.Rotator(yaw=(12 if b.startswith('L_') else -12)*max(fear,crying)).quaternion();tr.rotation=tr.rotation*delta
   data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
  opens.append(opening);closed.append(shut)
 controller=anim.get_editor_property('controller');controller.open_bracket('Act 2 dialogue and grief facial rig',False)
 for b,keys in data.items():assert controller.set_bone_track_keys(b,*keys,False),b
 controller.close_bracket(False)
 if u.AnimationLibrary.does_curve_exist(anim,'FaceSmile',u.RawCurveTrackTypes.RCT_FLOAT):
  u.AnimationLibrary.remove_curve(anim,'FaceSmile');u.AnimationLibrary.add_curve(anim,'FaceSmile');u.AnimationLibrary.add_float_curve_keys(anim,'FaceSmile',[0.,38.],[0.,0.])
 assert eal.save_loaded_asset(anim,False)
 cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False;p.set_editor_property('animation_cues',[cue])
 # Exact body preservation: facial tracks are the only changed bone tracks.
 for t in [0,7.6,11,19.6,22.6,25.7,28,30.5,35,38]:
  body=['pelvis_joint','head_joint']
  old=u.AnimationLibrary.get_bone_poses_for_time(source,body,t,False,mesh);new=u.AnimationLibrary.get_bone_poses_for_time(anim,body,t,False,mesh)
  for x,y in zip(old,new):assert (x.translation-y.translation).length()<.01 and (x.scale3d-y.scale3d).length()<.01
 for f in [0,300,600,900,1050,1140]:
  got=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,f/FPS,False,mesh)
  for b,tr in zip(bones,got):assert (tr.scale3d-data[b][2][f]).length()<.015,(who,b,f)
 assert max(opens)>.6
 report['cast'][who]={'animation':path,'source':source.get_path_name(),'max_open':max(opens),'speaking_frames':sum(x>.1 for x in opens),'eyes_closed_at_35':closed[1050]}
 for t in ([17.3,17.45] if who=='Romeo' else [21,35] if who=='Juliet' else [.5]):
  p.evaluate(t);comp=p.get_component_by_class(u.SkeletalMeshComponent);target=comp.get_socket_location('mouth_joint')+u.Vector(0,0,7);pos=target+p.get_actor_forward_vector()*160+u.Vector(0,0,12);rot=u.MathLibrary.find_look_at_rotation(pos,target)
  report['preview'][who+'_'+str(t)]={'target':list(target.to_tuple()),'position':list(pos.to_tuple()),'rotation':{'pitch':rot.pitch,'yaw':rot.yaw,'roll':rot.roll}}
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');assert seq
for binding in seq.get_bindings():
 for track in binding.get_tracks():
  if isinstance(track,u.MovieSceneSkeletalAnimationTrack):
   for section in track.get_sections():
    params=section.params
    for who,row in report['cast'].items():
     if params.animation and params.animation.get_path_name() in [row['source'],row['animation']+'.'+row['animation'].split('/')[-1]]:params.animation=u.load_asset(row['animation']);section.params=params
for track in list(seq.get_tracks()):
 if isinstance(track,u.MovieSceneAudioTrack):seq.remove_track(track)
track=seq.add_track(u.MovieSceneAudioTrack);track.set_display_name('Synchronized voices, wildfire and sword combat');section=track.add_section();section.set_sound(sound);section.set_range(0,1140)
assert eal.save_loaded_asset(seq,False)
for p in cast.values():p.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'report.json').write_text(json.dumps(report,indent=2));print('ACT2_AUDIO_FACES_SAVED',json.dumps(report['cast']))
