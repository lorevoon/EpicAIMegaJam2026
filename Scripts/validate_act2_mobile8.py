"""Reload and verify exact arrow-tip contact, held blocking and reversible playback."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/MobileRevision8';BASE='/Game/DirectorsCut'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise');actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};film=json.loads((OUT/'film.json').read_text());report={'arrow_contacts':[],'camera_spacing':film['minimum_camera_spacing_cm']}
assert report['camera_spacing']>=1300
r=cast['Romeo'];r.evaluate(13);p=r.get_actor_location()
for t in [14,16,18,20,25]:r.evaluate(t);assert (r.get_actor_location()-p).length()<1,(t,r.get_actor_location(),p)
k=cast['Armoured monster'];k.evaluate(13);before=(k.get_actor_location()-p).length();k.evaluate(18.8);after=(k.get_actor_location()-p).length();assert before>700 and after<210
for i,row in enumerate(film['arrows'],1):
 a=cast[f'Juliet arrow {i}'];target=cast[row['target']];t=row['impact'];a.evaluate(t);target.evaluate(t);prop=next(c for c in a.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Prop');tip=prop.get_world_transform().transform_location(u.Vector(0,row['tip_offset'],0));head=target.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');error=(tip-head).length();assert error<1,(i,error,tip,head)
 a.evaluate(row['release']+.1);tf=prop.get_world_transform();a.evaluate(38);assert not prop.is_visible();a.evaluate(row['release']+.1);assert prop.is_visible();assert (prop.get_world_transform().translation-tf.translation).length()<.01
 target.evaluate(t+3.2);c=target.get_component_by_class(u.SkeletalMeshComponent);dead=c.get_socket_location('head_joint');target.evaluate(38);assert (c.get_socket_location('head_joint')-dead).length()<.1
 report['arrow_contacts'].append({'target':row['target'],'impact':t,'tip_error_cm':error,'reverse_seek':'passed','death_remains_final':'passed'})
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');cuts=[t for t in seq.get_tracks() if isinstance(t,u.MovieSceneCameraCutTrack)];assert len(cuts)==1 and len(cuts[0].get_sections())==5
names=[b.get_name() for b in seq.get_bindings()];assert 'Juliet arrow 1' in names and 'Juliet arrow 2' in names
sound=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue')).get_component_by_class(u.AudioComponent).sound;assert '/BowRevision6/' in sound.get_path_name() and abs(sound.duration-38)<.01
report.update(charge_start_distance_cm=before,charge_end_distance_cm=after,sequence_cuts=5,sound=sound.get_path_name(),status='passed');(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('BOW_VALIDATION_PASSED',report)

j=cast['Juliet'];j.evaluate(22.35);first=j.get_actor_location();j.evaluate(24.9);second=j.get_actor_location();travel=(second-first).length();assert travel>350,travel
r.evaluate(26.5);j.evaluate(26.5);away=(r.get_actor_location()-j.get_actor_location()).length();j.evaluate(28.6);returned=(r.get_actor_location()-j.get_actor_location()).length();assert away>300 and returned<160,(away,returned)
r.evaluate(13.2);j.evaluate(13.2);dot=r.get_actor_forward_vector().dot(j.get_actor_forward_vector());assert dot<-.99,dot
report.update(shooting_position_change_cm=travel,distance_before_return_cm=away,distance_after_return_cm=returned,back_to_back_dot=dot)
(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('MOBILE_VALIDATED',report)
