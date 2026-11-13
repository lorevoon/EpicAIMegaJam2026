import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith('/Game/DirectorsCut/Levels/L_DC_Act1_Promise.'),world.get_path_name()
rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 p=a.get_actor_location();c,e=a.get_actor_bounds(False)
 if isinstance(a,(u.StaticMeshActor,u.DCPerformer,u.DCClapperboard,u.DCStartMark)):
  rows.append({'label':a.get_actor_label(),'class':a.get_class().get_name(),'pos':[p.x,p.y,p.z],'extent':[e.x,e.y,e.z]})
(root/'Saved/movie_set_before.json').write_text(json.dumps(rows,indent=2))
print(json.dumps({'world':world.get_path_name(),'actors':rows}))
