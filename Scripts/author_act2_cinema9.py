import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Act2/CinemaRevision9';OUT=ROOT/'Saved/Act2/CinemaRevision9';LEVEL=B+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')}
def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
manifest=json.loads((ROOT/'SourceArt/Act2/CinemaRevision9/Audio/timing.json').read_text());reply=next(l for l in manifest['lines'] if l['name']=='NotLeaving')
for who,sourcepath in [('Romeo',B+'/Act2/SmoothRevision7/A_Grounded_Romeo'),('Juliet',B+'/Act2/MobileRevision8/A_Mobile_Juliet')]:
 a=cast[who];source=u.load_asset(sourcepath);mesh=a.get_editor_property('character_mesh');bones=['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint'];data={b:[[],[],[]] for b in bones}
 for f in range(1141):
  t=f/30;ts=t
  if who=='Juliet' and 16.0<t<18.6:ts=16.3700625+(t-reply['start'])*1.12 if reply['start']<=t<=reply['end'] else 18.6
  poses=u.AnimationLibrary.get_bone_poses_for_time(source,bones,ts,False,mesh);shock=smooth((t-9.85)/.2)*(1-smooth((t-14)/1.8))
  for b,tr in zip(bones,poses):
   if b=='eyes_joint':tr.scale3d=u.Vector(tr.scale3d.x, max(tr.scale3d.y,1+.30*shock),tr.scale3d.z) if shock>0 else tr.scale3d
   if 'eyebrow' in b:
    p=tr.translation;p.x+=1.25*shock;tr.translation=p;tr.rotation=tr.rotation*u.Rotator(yaw=(-17 if b.startswith('L') else 17)*shock).quaternion()
   if b=='mouth_joint' and who=='Juliet' and shock>0:tr.scale3d=u.Vector(.95,max(tr.scale3d.y,1.30*shock),1)
   data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Cinema_'+who;anim=u.load_asset(dest) or eal.duplicate_asset(sourcepath,dest);c=anim.get_editor_property('controller');c.open_bracket('Readable shock and urgent reply',False)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);assert eal.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue])
# Return to the side of Romeo's shoulder instead of straddling his torso.
j=cast['Juliet'];cache=OUT/'juliet_waypoints.json'
waypoints=[a for a in actors if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer')==j]
if not cache.exists():cache.write_text(json.dumps({str(w.get_editor_property('time')):[*w.get_actor_location().to_tuple(),w.get_actor_rotation().yaw] for w in waypoints}))
baseline=json.loads(cache.read_text())
for w in waypoints:
 t=float(w.get_editor_property('time'));x,y,z,yaw=baseline[str(t)];weight=smooth((t-26.5)/1.8);p=u.Vector(x-15*weight,y+69.5*weight,z)
 if t>=28.1:yaw=105+(-165)*smooth((t-28.1)/.5)
 w.set_actor_location_and_rotation(p,u.Rotator(yaw=yaw),False,False)
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/CinemaRevision9/Audio/Act2_Soundtrack.wav');task.destination_path=F;task.destination_name='SW_Cinema';task.automated=True;task.replace_existing=True;task.save=True;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=u.load_asset(F+'/SW_Cinema');assert sound;sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM);sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE);eal.save_loaded_asset(sound,False)
a=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue'));a.get_component_by_class(u.AudioComponent).set_sound(sound)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL);print('CINEMA_ACTING_SAVED')
