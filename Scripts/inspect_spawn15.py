import unreal as u,json
from pathlib import Path
O=Path(__file__).resolve().parents[1]/'Saved/Act1Revision15';O.mkdir(parents=True,exist_ok=True);u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise');rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 name=a.get_actor_label()
 if any(s in name.lower() for s in ['wall','screen','shell','floor','clapper','spawn','start']):
  c,e=a.get_actor_bounds(False,True);rows.append({'label':name,'class':a.get_class().get_name(),'pos':list(a.get_actor_location().to_tuple()),'centre':list(c.to_tuple()),'extent':list(e.to_tuple()),'rotation':str(a.get_actor_rotation())})
(O/'bounds.json').write_text(json.dumps(rows,indent=2));print('STUDIO_BOUNDS',rows)
