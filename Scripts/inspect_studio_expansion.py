import unreal as u,json
from pathlib import Path
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCWaypoint):continue
 c,e=a.get_actor_bounds(False);p=a.get_actor_location();s=a.get_actor_scale3d()
 rows.append(dict(name=a.get_name(),label=a.get_actor_label(),cls=a.get_class().get_path_name(),pos=[p.x,p.y,p.z],scale=[s.x,s.y,s.z],center=[c.x,c.y,c.z],extent=[e.x,e.y,e.z],parent=str(a.get_attach_parent_actor()),tags=[str(t) for t in a.tags]))
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/FilmSetAudit/expansion_inventory.json').write_text(json.dumps(rows,indent=2))
u.log('INVENTORY_DONE')
