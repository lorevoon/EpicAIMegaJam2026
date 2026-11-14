import unreal as u,json
from pathlib import Path
out={};base='/Game/DirectorsCut/'
for path in [base+'Act2/SK_Act2_OrcComplete',base+'Act2/SK_Act2_Raider']:
 m=u.load_asset(path);out[path]=[{'slot':str(s.material_slot_name),'material':s.material_interface.get_path_name() if s.material_interface else None} for s in m.materials]
for path in [base+'Act2/MI_Act2_OrcOpaque_Body',base+'Act2/MI_Act2_OrcOpaque_Objects',base+'Materials/MI_Polysplit_Orc_Body',base+'Materials/MI_Polysplit_Orc_Objects']:
 m=u.load_asset(path);usage=getattr(u.MaterialUsage,next(n for n in dir(u.MaterialUsage) if 'SKELETAL' in n));out[path]={'skeletal_usage':u.MaterialEditingLibrary.has_material_usage(m,usage),'skeletal_usage_override':u.MaterialEditingLibrary.has_material_usage_override(m,usage),'parent':m.parent.get_path_name(),'vectors':str(m.get_editor_property('vector_parameter_values')),'textures':str(m.get_editor_property('texture_parameter_values')),'scalars':str(m.get_editor_property('scalar_parameter_values')),'overrides':str(m.get_editor_property('base_property_overrides'))}
m=u.load_asset(base+'Materials/M_PolysplitRecolor');e=u.load_object(None,m.get_path_name()+':MaterialExpressionCustom_0');out['code']=e.get_editor_property('code');out['inputs']=str(e.get_editor_property('inputs'))
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(base+'Levels/L_DC_Act2_BrokenPromise')
out['cast']=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCPerformer):
  c=a.get_component_by_class(u.SkeletalMeshComponent);out['cast'].append({'name':a.get_actor_label(),'mesh':str(c.skeletal_mesh_asset),'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())],'overrides':str(c.get_editor_property('override_materials'))})
p=Path(__file__).resolve().parents[1]/'Saved/Act2/OrcColor';p.mkdir(parents=True,exist_ok=True);(p/'audit.json').write_text(json.dumps(out,indent=2));print('ORC_COLOR_AUDIT_COMPLETE')
