import unreal as u,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/SmoothRevision7';BASE='/Game/DirectorsCut'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise');actors=list(aes.get_all_level_actors());report={'cast':{},'studio':[]}
for a in actors:
 if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only'):
  c=a.get_component_by_class(u.SkeletalMeshComponent);row={'source':a.get_editor_property('animation_cues')[0].animation.get_path_name(),'rows':[],'poses':{}}
  for f in range(381):
   a.evaluate(f/10);row['rows'].append([*a.get_actor_location().to_tuple(),a.get_actor_rotation().yaw])
  for t in [0,5,6,10,13,18,26,29,31,35]:
   a.evaluate(t);row['poses'][str(t)]={b:list(a.get_actor_transform().inverse_transform_location(c.get_socket_location(b)).to_tuple()) for b in ['head_joint','pelvis_joint','L_wrist_joint','R_wrist_joint','L_ankle_joint','R_ankle_joint']}
  report['cast'][a.get_actor_label()]=row
 if any(s in a.get_actor_label().lower() for s in ['green screen','studio roof','studio floor']):report['studio'].append({'name':a.get_actor_label(),'position':list(a.get_actor_location().to_tuple()),'bounds':str(a.get_actor_bounds(False))})
if not (OUT/'baseline.json').exists():(OUT/'baseline.json').write_text(json.dumps(report,indent=2))
print('SMOOTH7_INSPECTED',report['studio'])
