"""Reload and verify exact arrow-tip contact, held blocking and reversible playback."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/StoryRevision10';BASE='/Game/DirectorsCut'
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
sound=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue')).get_component_by_class(u.AudioComponent).sound;assert '/StoryRevision10/' in sound.get_path_name() and abs(sound.duration-38)<.01
report.update(charge_start_distance_cm=before,charge_end_distance_cm=after,sequence_cuts=5,sound=sound.get_path_name(),status='passed');(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('BOW_VALIDATION_PASSED',report)

j=cast['Juliet'];j.evaluate(17.8);first=j.get_actor_location();j.evaluate(20.2);second=j.get_actor_location();assert (second-first).length()>350
r.evaluate(35);rc=r.get_component_by_class(u.SkeletalMeshComponent);head=rc.get_socket_location('head_joint');pelvis=rc.get_socket_location('pelvis_joint');right=(rc.get_socket_location('R_wrist_joint')-rc.get_socket_location('L_wrist_joint')).normal();j.evaluate(35);jc=j.get_component_by_class(u.SkeletalMeshComponent);side=(jc.get_socket_location('pelvis_joint')-pelvis).dot(right);assert side>50,side
minimum=10000
for f in range(795,850):
 j.evaluate(f/30);p=j.get_actor_location();a=u.Vector(pelvis.x,pelvis.y,p.z);b=u.Vector(head.x,head.y,p.z);v=b-a;alpha=max(0,min(1,(p-a).dot(v)/v.dot(v)));minimum=min(minimum,(p-(a+v*alpha)).length())
assert minimum>55,minimum
cuts=cuts[0].get_sections();ranges=sorted([(c.get_start_frame(),c.get_end_frame()) for c in cuts]);assert ranges==[(0,300),(300,463),(463,570),(570,780),(780,1140)],ranges
solution=json.loads((OUT/'camera_solution.json').read_text());moving=solution[0];pathspacing=min(math.dist([moving['point'][d]+(moving['end'][d]-moving['point'][d])*f/100 for d in range(2)],other['point'][:2]) for f in range(101) for other in solution[1:]);assert pathspacing>=1300,pathspacing
report.update(cut_ranges=ranges,right_side_offset_cm=side,minimum_return_clearance_cm=minimum,moving_camera_spacing_cm=pathspacing,first_shot=17.8,second_shot=20.2)
(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('STORY10_VALIDATED',report)
