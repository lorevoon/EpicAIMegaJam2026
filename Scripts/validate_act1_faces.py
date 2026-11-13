"""Validate saved Act 1 facial acting, close walking and deterministic lip contact."""
import unreal as u, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
cast={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet','Mr. Smith']}
comps={n:a.get_component_by_class(u.SkeletalMeshComponent) for n,a in cast.items()}
report={'samples':{},'max_kiss_gap_cm':0,'walk_spacing_cm':[]}
for frame in range(1201):
 t=frame/60
 for a in cast.values():a.evaluate(t)
 if 7<=t<=12:
  p=cast['Romeo'].get_actor_location()-cast['Juliet'].get_actor_location()
  report['walk_spacing_cm'].append((p.x*p.x+p.y*p.y)**.5)
 if t>=17.5:
  gap=(comps['Romeo'].get_socket_location('mouth_joint')-comps['Juliet'].get_socket_location('mouth_joint')).length()
  report['max_kiss_gap_cm']=max(report['max_kiss_gap_cm'],gap)
 if frame in [0,240,420,600,750,960,1050,1080,1140,1200]:
  report['samples'][str(t)]={n:{'mouth':[c.get_socket_location('mouth_joint').x,c.get_socket_location('mouth_joint').y,c.get_socket_location('mouth_joint').z],'eye_scale':c.get_socket_transform('eyes_joint',u.RelativeTransformSpace.RTS_WORLD).scale3d.y,'smile':c.get_anim_instance().get_curve_value('FaceSmile'),'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())]} for n,c in comps.items()}
# Seeking backwards or replaying a recorded time must reproduce the same pose and material.
for t in [18.,7.,20.,4.,0.,18.,10.]:
 for n,a in cast.items():
  a.evaluate(t);c=comps[n];row=report['samples'][str(t)]
  expected=row[n];p=c.get_socket_location('mouth_joint')
  assert (p-u.Vector(*expected['mouth'])).length()<.01,(n,t,'seek pose')
  for i in range(c.get_num_materials()):
   mat=c.get_material(i)
   if 'MI_Act1_FacialExpressions' in mat.get_name():
    assert abs(mat.get_scalar_parameter_value('FaceSmile')-expected['smile'])<.01,(n,t,'material curve')
for a in cast.values():a.evaluate(0)
report['walk_spacing_cm']=[min(report['walk_spacing_cm']),max(report['walk_spacing_cm'])]
(root/'Saved/FilmSetAudit/face_validation.json').write_text(json.dumps(report,indent=2))
assert report['max_kiss_gap_cm']<1,report['max_kiss_gap_cm']
assert all(abs(s-85)<.1 for s in report['walk_spacing_cm'])
assert report['samples']['7.0']['Mr. Smith']['smile']>.95
for who in ['Romeo','Juliet']:
 assert report['samples']['18.0'][who]['eye_scale']<.06
assert all('MI_Act1_FacialExpressions' in ' '.join(row['materials']) for row in report['samples']['0.0'].values())
u.log('ACT1_FACE_VALIDATION_PASSED '+str(report['max_kiss_gap_cm']))
