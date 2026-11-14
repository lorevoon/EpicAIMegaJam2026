import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);out={}
for name in ['L_DC_Act1_Set','L_DC_Act1_Real','L_DC_Act1_Ground','L_DC_Act2_BrokenPromise']:
 assert les.load_level('/Game/DirectorsCut/Levels/'+name)
 out[name]=[{'label':a.get_actor_label(),'class':a.get_class().get_name(),'name':a.get_name(),'pos':list(a.get_actor_location().to_tuple()),'tags':[str(t) for t in a.tags]} for a in aes.get_all_level_actors() if not a.get_attach_parent_actor()]
for name in ['NS_Fire_Big','NS_Fire_Big_Smoke','BP_Fire']:
 a=u.load_asset('/Game/Vefects/Free_Fire/Shared/Particles/'+name)
 out[name]={'class':a.get_class().get_name(),'doc':str(a)}
(root/'Saved/Act2/dual_inventory.json').write_text(json.dumps(out,indent=2));print('ACT2_INVENTORY_READY')
