import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as B
import traceback
try:
 l=u.get_editor_subsystem(u.LevelEditorSubsystem).get_current_level()
 bp=u.load_object(None, '/Game/DirectorsCut/Levels/L_DC_Act1_Promise.L_DC_Act1_Promise:PersistentLevel.L_DC_Act1_Promise')
 lines=[str(bp)]
 for g in B.list_graphs(bp):
  lines.append(str(g))
  for n in B.find_nodes(g):
   lines.append(str(n.get_name())+' '+str(n.get_node_title())+' '+str(B._get_node_info(n)))
 open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/blueprint.txt','w').write('\n'.join(lines))
except: open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/blueprint.txt','w').write(traceback.format_exc())
