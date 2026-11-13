"""Use the existing face atlas's smile artwork, driven by the FaceSmile anim curve."""
import unreal as u,json
from pathlib import Path
base='/Game/DirectorsCut/Materials/'
eal=u.EditorAssetLibrary;mel=u.MaterialEditingLibrary
path=base+'M_Act1_FacialExpressions'
mat=u.load_asset(path) or eal.duplicate_asset(base+'M_PolysplitRecolor',path)
assert mat
node=u.load_object(None,mat.get_path_name()+':MaterialExpressionCustom_0')
inputs=node.get_editor_property('inputs')
sources=dict(zip(mel.get_material_expression_input_names(node),mel.get_inputs_for_material_expression(mat,node)))
assert 'Mask' in sources and 'UV' in sources
source=sources['Mask'];uv=sources['UV']
if isinstance(source,u.MaterialExpressionTextureSample):
 texture=source.get_editor_property('texture')
 offset=mel.create_material_expression(mat,u.MaterialExpressionConstant2Vector,-800,800);offset.r=.28125;offset.g=0
 add=mel.create_material_expression(mat,u.MaterialExpressionAdd,-600,800);mel.connect_material_expressions(uv,'',add,'A');mel.connect_material_expressions(offset,'',add,'B')
 sample=mel.create_material_expression(mat,u.MaterialExpressionTextureSample,-400,800);sample.texture=texture
 mel.connect_material_expressions(add,'',sample,'UVs')
 param=mel.create_material_expression(mat,u.MaterialExpressionScalarParameter,-800,1000);param.set_editor_property('parameter_name','FaceSmile');param.set_editor_property('default_value',0)
 mask=mel.create_material_expression(mat,u.MaterialExpressionCustom,-400,1000);mask.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
 face_inputs=[]
 for n in ['UV','Smile']:
  i=u.CustomInput();i.set_editor_property('input_name',n);face_inputs.append(i)
 mask.set_editor_property('inputs',face_inputs)
 mask.set_editor_property('code','return saturate(Smile) * ((UV.x > .51 && UV.x < .61 && ((UV.y > .586 && UV.y < .64) || (UV.y > .695 && UV.y < .75))) ? 1.0 : 0.0);')
 mel.connect_material_expressions(uv,'',mask,'UV');mel.connect_material_expressions(param,'',mask,'Smile')
 blend=mel.create_material_expression(mat,u.MaterialExpressionLinearInterpolate,-100,800)
 mel.connect_material_expressions(source,'RGBA',blend,'A');mel.connect_material_expressions(sample,'RGBA',blend,'B');mel.connect_material_expressions(mask,'',blend,'Alpha')
 mel.connect_material_expressions(blend,'',node,'Mask')
 # The rig's face cards use atlas alpha; the new mouth's opacity must follow its colour.
 if mat.get_editor_property('blend_mode')==u.BlendMode.BLEND_MASKED:
  alpha=mel.create_material_expression(mat,u.MaterialExpressionComponentMask,100,900);alpha.set_editor_property('a',True);alpha.set_editor_property('r',False);alpha.set_editor_property('g',False);alpha.set_editor_property('b',False)
  assert mel.connect_material_expressions(blend,'',alpha,'');mel.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY_MASK)
else:
 alpha=mel.get_material_property_input_node(mat,u.MaterialProperty.MP_OPACITY_MASK)
 assert mel.connect_material_expressions(source,'',alpha,'')
mel.recompile_material(mat);assert eal.save_loaded_asset(mat,False)
instance=u.load_asset(base+'MI_Act1_FacialExpressions') or eal.duplicate_asset(base+'MI_Polysplit_Hero_Body',base+'MI_Act1_FacialExpressions')
mel.set_material_instance_parent(instance,mat);assert eal.save_loaded_asset(instance,False)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet','Mr. Smith']:
  c=a.get_component_by_class(u.SkeletalMeshComponent)
  for i,slot in enumerate(a.get_editor_property('character_mesh').get_editor_property('materials')):
   if 'Body' in str(slot.material_slot_name):c.set_material(i,instance)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('FACE_SMILE_MATERIAL_AUTHORED')




