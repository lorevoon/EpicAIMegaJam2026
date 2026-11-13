import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/WindmillValley/Levels/L_Showcase');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
labels=['BP_Building9','BP_SmallHouse4','BP_Cart_Wheat5','BP_WheatStack_15','BP_Barrels_23','BP_Windmill2','BP_Mailbox5']
props={a.get_actor_label():a.get_actor_transform().export_text() for a in actors if a.get_actor_label() in labels}
trees=[]
for a in actors:
 for c in a.get_components_by_class(u.InstancedStaticMeshComponent):
  mesh=c.static_mesh
  if not mesh or 'tree' not in mesh.get_name().lower():continue
  for i in range(c.get_instance_count()):
   t=c.get_instance_transform(i,True);p=t.translation;d=math.hypot(p.x+6100,p.y+8550)
   if d<3500:trees.append({'actor':a.get_name(),'component':c.get_name(),'mesh':mesh.get_path_name(),'index':i,'distance':d,'transform':t.export_text(),'p':[p.x,p.y,p.z]})
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True)
 return h.to_tuple()[4].z if h else None
samples=[{'x':x,'y':y,'z':ground(x,y)} for x in range(-6500,-4499,250) for y in range(-9250,-7749,250)]
(root/'Saved/FilmSetAudit/original_layout.json').write_text(json.dumps({'props':props,'trees':sorted(trees,key=lambda t:t['distance'])[:35],'ground':samples},indent=2))
u.log('ORIGINAL_LAYOUT_AUDITED')
