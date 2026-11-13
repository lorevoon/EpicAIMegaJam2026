import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for name in ['L_DC_Act1_Promise','L_DC_Act1_Set','L_DC_Act1_Real']:
 les.load_level('/Game/DirectorsCut/Levels/'+name);actors=list(aes.get_all_level_actors());rows=[]
 for a in actors:
  if a.get_attach_parent_actor():continue
  p=a.get_actor_location();row={'name':a.get_name(),'label':a.get_actor_label(),'class':a.get_class().get_name(),'path':a.get_path_name(),'pos':[p.x,p.y,p.z],'tags':[str(t) for t in a.tags]}
  if isinstance(a,u.DCWaypoint):row.update(time=a.get_editor_property('time'),performer=str(a.get_editor_property('performer')))
  if isinstance(a,u.DCPerformer):
   row['cues']=len(a.get_editor_property('animation_cues'));row['samples']={}
   for t in [0,5,10,15,20,0]:
    a.evaluate(t);v=a.get_actor_location();row['samples'][str(t)]=[v.x,v.y,v.z]
  if isinstance(a,u.StaticMeshActor):row['mesh']=str(a.static_mesh_component.static_mesh)
  rows.append(row)
 out[name]=rows
(root/'Saved/FilmSetAudit/rehearsal_audit.json').write_text(json.dumps(out,indent=2))
u.log('REHEARSAL_AUDITED')
