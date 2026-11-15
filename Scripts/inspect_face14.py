import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];O=R/'Saved/Act1Revision14';B='/Game/DirectorsCut';les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level(B+'/Levels/L_DC_Act1_Promise');a=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo');a.evaluate(.8);c=a.get_component_by_class(u.SkeletalMeshComponent);rows=[]
for i in range(c.get_num_materials()):
 m=c.get_material(i);rows.append({'mat':m.get_path_name(),'parent':str(m.get_editor_property('parent')),'smile':m.get_scalar_parameter_value('FaceSmile')})
mat=u.load_asset(B+'/Materials/M_Act1_FacialExpressions');nodes=[]
for x in u.MaterialEditingLibrary.get_all_referenced_expressions(mat) if hasattr(u.MaterialEditingLibrary,'get_all_referenced_expressions') else []:nodes.append(str(x))
for i in range(12):
 n=u.find_object(None,mat.get_path_name()+':MaterialExpressionCustom_'+str(i))
 if n:rows.append({'node':str(n),'code':n.get_editor_property('code')})
node=u.load_object(None,mat.get_path_name()+':MaterialExpressionCustom_0');rows.append({'inputs':list(u.MaterialEditingLibrary.get_material_expression_input_names(node)),'sources':[str(x) for x in u.MaterialEditingLibrary.get_inputs_for_material_expression(mat,node)]})
for i in range(6):
 n=u.find_object(None,mat.get_path_name()+':MaterialExpressionTextureSample_'+str(i))
 if n:
  texture=n.get_editor_property('texture');task=u.AssetExportTask();task.object=texture;task.filename=str(O/('face_texture_'+str(i)+'.png'));task.exporter=u.TextureExporterPNG();task.automated=True;task.prompt=False;task.replace_identical=True;u.Exporter.run_asset_export_task(task);rows.append({'texture':str(texture)})
(O/'face_material.json').write_text(json.dumps(rows,indent=2));print('FACE_MATERIAL',rows)
