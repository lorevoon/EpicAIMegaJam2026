import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]; out={}
les=u.get_editor_subsystem(u.LevelEditorSubsystem); aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for n in ['L_DC_Act1_Promise','L_DC_Act2_BrokenPromise']:
 les.load_level('/Game/DirectorsCut/Levels/'+n)
 rows=[]
 for a in aes.get_all_level_actors():
  c,e=a.get_actor_bounds(False)
  meshes=[{'mesh':x.static_mesh.get_path_name(),'transform':x.get_world_transform().export_text()} for x in a.get_components_by_class(u.StaticMeshComponent) if x.static_mesh]
  rows.append(dict(label=a.get_actor_label(),path=a.get_path_name(),pos=list(a.get_actor_location().to_tuple()),bounds=[list(c.to_tuple()),list(e.to_tuple())],meshes=meshes,tags=[str(t) for t in a.tags]))
 out[n]=rows
(R/'Saved/lively_inspection.json').write_text(json.dumps(out,indent=2))
