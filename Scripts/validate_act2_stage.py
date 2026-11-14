"""Inspect saved Stage 2, sample both directions, and assert timeline and death holds."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(level)
actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};beats=sorted([a for a in actors if isinstance(a,u.DCBeat)],key=lambda b:b.get_editor_property('window_start'));d=[a for a in actors if isinstance(a,u.DCZombieDirector)]
assert len(d)==1 and d[0].get_editor_property('take_duration')==38
assert len(beats)==5
assert [b.get_editor_property('beat_id') for b in beats]==['2.1','2.2','2.3','2.4','2.5']
for b in beats:
 assert b.get_editor_property('pose_ready_time')+b.get_editor_property('required_hold')<=b.get_editor_property('window_end')
 assert len(b.get_editor_property('anchors'))==(3 if str(b.get_editor_property('beat_id'))=='2.1' else 2)
for a in cast.values():
 assert a.get_editor_property('character_mesh').skeleton
 for cue in a.get_editor_property('animation_cues'):
  assert cue.animation.get_editor_property('skeleton')==a.get_editor_property('character_mesh').skeleton
  assert cue.animation.get_play_length()>0
samples={}
for t in [0,3.5,9.5,11.5,18,23,28.5,34,35,38,35,9.5,0]:
 rows={}
 for name,a in cast.items():
  a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);center,extent=a.get_actor_bounds(False)
  rows[name]={'position':list(a.get_actor_location().to_tuple()),'head':list(c.get_socket_location(a.get_editor_property('head_bone')).to_tuple()),'extent':list(extent.to_tuple()),'wrists':[list(c.get_socket_location(b).to_tuple()) for b in ['L_wrist_joint','R_wrist_joint']]}
 key=str(t)
 if key in samples:
  for n in rows:assert all(abs(x-y)<.01 for x,y in zip(rows[n]['head'],samples[key][n]['head'])),(t,n)
 samples[key]=rows
# Death holds remain fixed even after arbitrary seeks.
for who,first in [('Mr. Smith',9.5),('Romeo',34),('Armoured monster',28.5)]:
 assert max(abs(x-y) for x,y in zip(samples[str(first)][who]['head'],samples['38'][who]['head']))<1,(who,'death moved')
seq=u.load_asset('/Game/DirectorsCut/Act2/LS_DC_Act2_OrcRaid');assert seq and seq.get_playback_end()==1140
assert set(cast)=={'Mr. Smith','Romeo','Juliet','Armoured monster','Raid orc 1','Raid orc 2','Raid orc 3','Raid orc 4'}
continuity=json.loads((root/'Saved/Act2/act1_continuity.json').read_text())['cast']
for who in ['Romeo','Juliet']:
 assert max(abs(x-y) for x,y in zip(samples['0'][who]['position'],continuity[who]['position']))<.01,(who,'Act 1 position mismatch')
assert cast['Romeo'].get_editor_property('character_mesh').get_name()=='SK_Act2_RomeoArmed'
assert not any(a.get_actor_label() in ['Romeo Blade','Romeo Grip','Romeo Guard'] for a in actors)
for a in cast.values():
 if 'orc' in a.get_actor_label().lower() or a.get_actor_label()=='Armoured monster':
  assert a.get_editor_property('character_mesh').get_name()=='SK_Act2_OrcComplete'
  for slot in a.get_editor_property('character_mesh').materials:
   overrides=slot.material_interface.get_editor_property('base_property_overrides')
   assert overrides.get_editor_property('override_blend_mode') and overrides.get_editor_property('blend_mode')==u.BlendMode.BLEND_OPAQUE
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
studio=u.GameplayStatics.get_streaming_level(world,'L_DC_Act2_Set');real=u.GameplayStatics.get_streaming_level(world,'L_DC_Act2_Real')
assert studio and real
assert studio.get_editor_property('initially_visible') and not real.get_editor_property('initially_visible')
for layer in [studio,real]:
 assert layer.get_editor_property('initially_loaded')
 assert layer.get_editor_property('level_transform').translation==u.Vector()
assert str(d[0].get_editor_property('studio_environment_level'))=='L_DC_Act2_Set'
assert str(d[0].get_editor_property('real_environment_level'))=='L_DC_Act2_Real'
assert not any('Fire' in b.get_name() for b in seq.get_bindings())
before={a:a.get_actor_transform() for a in cast.values()}
for outdoor in [True,False]:
 u.EditorLevelUtils.set_level_visibility(studio.get_loaded_level(),not outdoor,False)
 u.EditorLevelUtils.set_level_visibility(real.get_loaded_level(),outdoor,False)
 for a,tf in before.items():assert a.get_actor_transform()==tf
fire=[a for a in aes.get_all_level_actors() if 'DC_Act2RealFire' in [str(t) for t in a.tags]]
assert len(fire)==46,len(fire)
assert all('/L_DC_Act2_Real.' in a.get_path_name() for a in fire)
assert all(a.get_component_by_class(u.NiagaraComponent).get_asset().get_name().startswith('NS_Act2_VillageInferno') for a in fire)
player,sequence_actor=u.LevelSequencePlayer.create_level_sequence_player(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),seq,u.MovieSceneSequencePlaybackSettings())
params=u.MovieSceneSequencePlaybackParams();params.position_type=u.MovieScenePositionType.FRAME;params.frame=u.FrameTime(frame_number=u.FrameNumber(1050));params.update_method=u.UpdatePositionMethod.JUMP;player.set_playback_position(params)
sequence_errors={}
for name,a in cast.items():
 c=a.get_component_by_class(u.SkeletalMeshComponent);p=c.get_socket_location(a.get_editor_property('head_bone'));expected=samples['35'][name]['head'];sequence_errors[name]=max(abs(x-y) for x,y in zip(p.to_tuple(),expected))
assert max(sequence_errors.values())<1,sequence_errors
player.stop()
report={'level':level,'seconds':38,'beats':5,'orcs':5,'act1_position_continuity':'passed','opaque_orc_materials':'passed','source_sword_bind':'passed','sequence_matches_director':sequence_errors,'studio_to_real_alignment':'passed','real_only_fire_systems':len(fire),'removed_prop_fire':True,'reverse_scrub':'passed','death_holds':'passed','samples':samples}
(root/'Saved/Act2/validation.json').write_text(json.dumps(report,indent=2));print('ACT2_VALIDATION_PASSED')
