import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
es=u.get_editor_subsystem(u.EditorActorSubsystem)
actors=es.get_all_level_actors()
cast=[a for a in actors if isinstance(a,u.DCPerformer)]
beats=[a for a in actors if isinstance(a,u.DCBeat)]
assert len(cast)==5 and len(beats)==4
report={'cast':[],'beats':[],'samples':{},'rewind_passed':False}
for a in cast:
 mesh=a.get_editor_property('character_mesh')
 cues=a.get_editor_property('animation_cues')
 assert mesh and cues and all(s.material_interface for s in mesh.materials)
 assert all(c.animation and c.animation.get_editor_property('skeleton')==mesh.skeleton for c in cues)
 report['cast'].append({'name':a.get_actor_label(),'mesh':mesh.get_path_name(),'cues':len(cues),'crew':a.get_editor_property('ruins_shot')})
for b in beats:
 report['beats'].append({'id':b.get_editor_property('beat_id'),'start':b.get_editor_property('window_start'),'end':b.get_editor_property('window_end')})
def sample(t):
 rows=[]
 for a in cast:
  a.evaluate(t);p=a.get_actor_location(); assert all(math.isfinite(x) for x in [p.x,p.y,p.z])
  rows.append([p.x,p.y,p.z])
 return rows
initial=sample(0)
for t in [2,6.2,8,15,21,24,33.5,35,40]:report['samples'][str(t)]=sample(t)
final=sample(0)
report['rewind_max_error_cm']=max(abs(x-y) for a,b in zip(initial,final) for x,y in zip(a,b))
assert report['rewind_max_error_cm']<0.01
report['rewind_passed']=True
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
(root/'Saved/Stage1Audit/validation.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_VALIDATION_PASSED')
