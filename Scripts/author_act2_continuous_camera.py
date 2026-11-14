"""Four angles / five story sections; one continuous fight-to-goodbye camera."""
import unreal as u,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/ActingRevision4';OUT.mkdir(parents=True,exist_ok=True)
BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
timing=json.loads((ROOT/'SourceArt/Act2/Audio/timing.json').read_text())
# Protect all recorded voices, including screams/sobs, with a short lead/trail.
windows=[(max(0,l['start']-.15),min(38,l['end']+.15)) for l in timing['lines']]
windows += [(26.,38.)]
shots=[(1,0,'2.1',(-5794,-8212,1865),78,68,.5,5.5),
       (2,6,'2.2',(-6064,-8257,1421),62,56,6,7.1),
       (3,10,'2.3',(-4550,-8500,1430),62,74,12.8,17.8),
       (4,18,'2.4',(-5750,-10100,1231),50,60,24,28.5),
       (6,29,'2.5',(-5750,-10100,1231),60,38,30.5,36.8)]
for i,start,beat,point,fov,endfov,zstart,zend in shots:
 a=next(a for a in actors if a.get_actor_label()=='Raid cut camera '+str(i))
 a.set_actor_location(u.Vector(*point),False,False);a.get_component_by_class(u.CameraComponent).set_field_of_view(fov)
 tags=['DCReferenceShot',f'DCStart={start}',f'DCBeat={beat}',f'DCFOV={fov}',f'DCEndFOV={endfov}',f'DCZoomStart={zstart}',f'DCZoomEnd={zend}']
 if i==1:tags += [f'DCDialogue={start:.4f}:{end:.4f}' for start,end in windows]
 if i==2:tags += ['DCSlowStart=7.0','DCSlowEnd=7.3','DCSlowRate=0.8']
 if i==4:tags += ['DCSlowStart=25','DCSlowEnd=25.9','DCSlowRate=0.65']
 if i==6:tags += ['DCContinueShot']
 a.tags=tags
for a in actors:
 if a.get_actor_label()=='Raid cut camera 5':
  a.tags=[];a.set_actor_label('Unused combat reverse - retained for editing');a.set_actor_hidden_in_game(True)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'cameras.json').write_text(json.dumps(dict(shots=shots,normal_speed_dialogue=windows),indent=2));print('ACT2_CONTINUOUS_CAMERA_SAVED')
