import unreal as u,json,traceback
from editor_toolset.toolsets.blueprint import BlueprintTools as B
out={}
try:
 for name,path in [('source','/Game/WindmillValley/Levels/L_Showcase.L_Showcase'),('stage','/Game/DirectorsCut/Levels/L_DC_Act1_Promise.L_DC_Act1_Promise')]:
  w=u.load_object(None,path)
  bp=u.load_object(None,path+':PersistentLevel.'+path.split('.')[-1])
  out[name]=[]
  for g in B.list_graphs(bp):
   for n in B.find_nodes(g):
    if 'Literal' in n.get_class().get_name():
     out[name].append({'node':n.get_name(),'object':str(n.get_node_title())})
except:out['error']=traceback.format_exc()
open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/literals.json','w').write(json.dumps(out,indent=2))
