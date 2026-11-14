import unreal as u, json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/StudioRepair';out.mkdir(parents=True,exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
report={}
for name in ['L_DC_Act2_BrokenPromise','L_DC_Act2_Set']:
 les.load_level('/Game/DirectorsCut/Levels/'+name)
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
 rows=[]
 for a in aes.get_all_level_actors():
  if a.get_attach_parent_actor():continue
  p=a.get_actor_location();b,e=a.get_actor_bounds(False)
  if name.endswith('Set') or isinstance(a,u.DCBeat) or (isinstance(a,u.StaticMeshActor) and -7000<p.x<-4500 and -11000<p.y<-8000):
   d=dict(label=a.get_actor_label(),cls=a.get_class().get_name(),pos=list(p.to_tuple()),bounds=[list(b.to_tuple()),list(e.to_tuple())],scale=list(a.get_actor_scale3d().to_tuple()))
   if isinstance(a,u.StaticMeshActor):
    c=a.static_mesh_component;d['mesh']=c.static_mesh.get_path_name() if c.static_mesh else None;d['materials']=[m.get_path_name() if m else None for m in c.get_materials()]
   if isinstance(a,u.DCBeat):d['spot']=str(a.get_editor_property('camera_spot'));d['show']=a.get_editor_property('show_camera_spot')
   rows.append(d)
 report[name]=rows
(out/'inventory.json').write_text(json.dumps(report,indent=2));print('STUDIO_REPAIR_AUDIT_DONE')
