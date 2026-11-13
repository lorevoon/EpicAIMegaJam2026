"""Import the timed dialogue and bake simple mouth-card rig movement over existing acting."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FPS=30
manifest=json.loads((ROOT/'SourceArt/Act1Dialogue/timing.json').read_text())
aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='L_DC_Act1_Promise'
actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
# A referenced sound actor makes this audio part of the cooked map, rather than a runtime-only path.
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act1Dialogue/Act1_Dialogue.wav');task.destination_path=BASE+'/Audio/Act1';task.destination_name='SW_Act1_Dialogue';task.automated=True;task.replace_existing=True;task.save=True
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound=u.load_asset(BASE+'/Audio/Act1/SW_Act1_Dialogue');assert sound
sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM)
sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE)
assert u.EditorAssetLibrary.save_loaded_asset(sound,False)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
ambient=next((a for a in actors if isinstance(a,u.AmbientSound) and 'DCSceneDialogue' in [str(t) for t in a.tags]),None)
if ambient is None:ambient=aes.spawn_actor_from_class(u.AmbientSound,u.Vector())
ambient.set_actor_label('Act 1 - synchronized dialogue');ambient.tags=['DCSceneDialogue'];ambient.set_folder_path('Act1/Audio')
ac=ambient.get_component_by_class(u.AudioComponent);ac.set_sound(sound);ac.set_editor_property('auto_activate',False);ac.set_editor_property('allow_spatialization',False);ac.set_editor_property('is_ui_sound',True);ac.stop()
report={'lines':[{k:v for k,v in l.items() if k!='mouth_gate'} for l in manifest['lines']],'cast':{}}
for who in ['Romeo','Juliet','Mr. Smith']:
 p=cast[who];mesh=p.get_editor_property('character_mesh');slug=who.replace(' ','').replace('.','')
 source=u.load_asset(BASE+'/Animations/Act1Faces/A_Act1_'+slug+'_FacePerformance');assert source
 path=BASE+'/Animations/Act1Dialogue/A_Act1_'+slug+'_Speaking'
 anim=u.load_asset(path) or u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path);assert anim
 poses=[u.AnimationLibrary.get_bone_poses_for_time(source,['mouth_joint'],f/FPS,False,mesh)[0] for f in range(601)]
 lines=[l for l in manifest['lines'] if l['speaker']==who]
 scales=[];amounts=[]
 for f,tr in enumerate(poses):
  t=f/FPS;opening=0;gate=0
  for l in lines:
   if l['start']<=t<l['end']:
    local=t-l['start'];fade=min(1,local/.07,(l['end']-t)/.07)
    # Three to four clear openings per second; silence inside the line closes the mouth.
    gate=max(l['mouth_gate'][max(0,f-1):min(601,f+2)])*max(0,fade)
    opening=(.5-.5*math.cos(local*2*math.pi*3.6))*gate
    break
  original=tr.scale3d
  # mouth_joint controls the existing face card, whose local Y is vertical.
  scales.append(u.Vector(original.x*(1-.06*opening),original.y*((1-gate)+gate*.30+1.35*opening),original.z))
  amounts.append(opening)
 controller=anim.get_editor_property('controller');controller.open_bracket('Dialogue mouth opening and closing',False)
 assert controller.set_bone_track_keys('mouth_joint',[t.translation for t in poses],[t.rotation for t in poses],scales,False)
 controller.close_bracket(False)
 cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False;p.set_editor_property('animation_cues',[cue])
 assert u.EditorAssetLibrary.save_loaded_asset(anim,False)
 # Verify samples, including that the final kiss retains its original mouth pose.
 for f in [0,60,180,330,440,520,540,600]:
  actual=u.AnimationLibrary.get_bone_poses_for_time(anim,['mouth_joint'],f/FPS,False,mesh)[0]
  assert (actual.scale3d-scales[f]).length()<.015,(who,f,actual.scale3d,scales[f])
 assert max(amounts)>.7
 report['cast'][who]={'animation':path,'max_open':max(amounts),'speaking_frames':sum(v>.05 for v in amounts)}
 p.evaluate(0)
assert abs(sound.duration-20)<.01
assert u.EditorLoadingAndSavingUtils.save_map(world,BASE+'/Levels/L_DC_Act1_Promise')
(ROOT/'Saved/act1_dialogue_report.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_DIALOGUE_SAVED '+json.dumps(report['cast']))
