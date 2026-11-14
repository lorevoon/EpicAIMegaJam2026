import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);out={}
for name in ['L_DC_Act1_Promise','L_DC_Act2_Set','L_DC_Act2_Real','L_DC_Act2_BrokenPromise']:
 assert les.load_level('/Game/DirectorsCut/Levels/'+name)
 rows=[]
 for a in aes.get_all_level_actors():
  if a.get_attach_parent_actor():continue
  center,extent=a.get_actor_bounds(False)
  row={'label':a.get_actor_label(),'class':a.get_class().get_name(),'path':a.get_path_name(),'pos':list(a.get_actor_location().to_tuple()),'center':list(center.to_tuple()),'extent':list(extent.to_tuple()),'tags':[str(t) for t in a.tags]}
  rows.append(row)
 out[name]=rows
(root/'Saved/Act2/Revision2/inventory.json').write_text(json.dumps(out,indent=2))
print('REVISION2_INSPECTED')
