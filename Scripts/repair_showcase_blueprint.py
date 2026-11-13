import unreal as u,json
from editor_toolset.toolsets.blueprint import BlueprintTools as B
path='/Game/DirectorsCut/Levels/L_DC_Act1_Promise.L_DC_Act1_Promise'
bp=u.load_object(None,path+':PersistentLevel.L_DC_Act1_Promise')
changes=[]
for g in B.list_graphs(bp):
 for n in B.find_nodes(g):
  if n.get_class().get_name()!='K2Node_Literal' or n.get_node_title()!='Unknown':continue
  for output in B._get_node_info(n).output_pins:
   for target in output.connected_pins:
    call=target.node
    for pin in B._get_node_info(call).input_pins:
     if pin.name=='execute':
      for upstream in pin.connected_pins:
       B.break_pins(upstream,pin.pin_id)
       changes.append({'missing_literal':n.get_name(),'disconnected_call':call.get_node_title()})
u.BlueprintEditorLibrary.compile_blueprint(bp)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if a.get_class().get_name()=='DCPerformer':a.evaluate(0.)
assert u.EditorLoadingAndSavingUtils.save_map(world,path.split('.')[0])
open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/blueprint_repair.json','w').write(json.dumps(changes,indent=2))
u.SystemLibrary.execute_console_command(world,'r.MotionBlurQuality 0')
