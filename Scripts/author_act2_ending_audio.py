"""Add the shortened-pause final Juliet line without rebaking other facial/body acting."""
from pathlib import Path
import unreal as u,json,math
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/EndingAudio';OUT.mkdir(parents=True,exist_ok=True)
BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2/AudioFacesV4';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors());juliet=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Juliet')
source=u.load_asset(BASE+'/Act2/ActingRevision4/A_Juliet_ReactionAway');assert source
mesh=juliet.get_editor_property('character_mesh');target=FOLDER+'/A_Juliet_FinalLine'
anim=u.load_asset(target) or eal.duplicate_asset(source.get_path_name(),target);assert anim
manifest=json.loads((ROOT/'SourceArt/Act2/Audio/timing.json').read_text());line=next(l for l in manifest['lines'] if l['name']=='YouPromised')
assert line['start']==33.01 and abs(line['end']-37.86)<.001
keys=[[],[],[]];openings=[]
for f in range(1141):
 t=f/30;tr=u.AnimationLibrary.get_bone_poses_for_time(source,['mouth_joint'],t,False,mesh)[0]
 opening=0
 if line['start']<=t<line['end']:
  g=max(line['mouth_gate'][max(0,f-1):min(1141,f+2)])*min(1,(t-line['start'])/.07,(line['end']-t)/.07)
  pulse=.5-.5*math.cos((t-line['start'])*2*math.pi*3.6);opening=pulse*g
  tr.scale3d=u.Vector(1-.08*opening,max(.12,(1-g)*.75+g*(.18+1.65*opening)),1)
 keys[0].append(tr.translation);keys[1].append(tr.rotation);keys[2].append(tr.scale3d);openings.append(opening)
c=anim.get_editor_property('controller');c.open_bracket('Juliet final line mouth only',False);assert c.set_bone_track_keys('mouth_joint',*keys,False);c.close_bracket(False);assert eal.save_loaded_asset(anim,False)
comp=juliet.get_component_by_class(u.SkeletalMeshComponent);bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones()) if str(comp.get_bone_name(i))!='mouth_joint']
for t in [0,10,11,12,13,26,29,33.2,34,35,37.8,38]:
 old=u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh);new=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,t,False,mesh)
 for bone,a,b in zip(bones,old,new):
  assert (a.translation-b.translation).length()<.01 and (a.scale3d-b.scale3d).length()<.01,(bone,t)
  assert abs(abs(sum(x*y for x,y in zip(a.rotation.to_tuple(),b.rotation.to_tuple())))-1)<.001,(bone,t)
assert max(openings)>.6
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/Audio/Act2_Soundtrack.wav');task.destination_path=FOLDER;task.destination_name='SW_Act2_Soundtrack';task.automated=True;task.replace_existing=True;task.save=True
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=u.load_asset(FOLDER+'/SW_Act2_Soundtrack');assert sound
sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM);sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE);assert eal.save_loaded_asset(sound,False);assert abs(sound.duration-38)<.001
ambient=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue'))
ac=ambient.get_component_by_class(u.AudioComponent);ac.set_sound(sound);ac.set_editor_property('auto_activate',False);ac.set_editor_property('allow_spatialization',False);ac.set_editor_property('is_ui_sound',True)
cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False;juliet.set_editor_property('animation_cues',[cue])
for a in actors:
 if isinstance(a,u.DCPerformer):a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
report={'sound':sound.get_path_name(),'juliet_animation':anim.get_path_name(),'source_animation':source.get_path_name(),'duration':sound.duration,'line':{k:v for k,v in line.items() if k!='mouth_gate'},'cut_source_seconds':[3.45,8.30],'rate':1,'preserved_nonmouth_bones':len(bones),'maximum_mouth_opening':max(openings)}
(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('ACT2_FINAL_LINE_SAVED',json.dumps(report))
