import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=root/'SourceArt/Hyper3D/CraneCamera_v2/DarkerBlack'
folder='/Game/DirectorsCut/Props/Hyper3D_MovieSet/CraneCameraV2'
eal=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
def imp(file,name,mesh=False):
 obj=u.load_asset(folder+'/'+name)
 if obj:return obj
 task=u.AssetImportTask();task.filename=str(file);task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True
 if mesh:
  opts=u.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False;opts.import_animations=False;opts.import_materials=False;opts.import_textures=False
  opts.automated_import_should_detect_type=False;opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
  opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
  task.options=opts
 tools.import_asset_tasks([task]);obj=u.load_asset(folder+'/'+name);assert obj,name
 return obj
mesh=imp(src/'CraneCamera_3p2m.fbx','SM_CraneCamera_Dark',True)
textures={}
for key,file in [('Base','texture_diffuse.png'),('Normal','texture_normal.png'),('ORM','texture_metallic-texture_roughness.png')]:
 tex=imp(src/'textures'/file,'T_CraneCamera_'+key)
 if key=='Normal':
  tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('srgb',False);tex.set_editor_property('flip_green_channel',True)
 if key=='ORM':tex.set_editor_property('srgb',False)
 eal.save_loaded_asset(tex);textures[key]=tex
def material(dark):
 name='M_CraneCamera_'+('DarkBody' if dark else 'Accents')
 mat=u.load_asset(folder+'/'+name) or tools.create_asset(name,folder,u.Material,u.MaterialFactoryNew())
 u.MaterialEditingLibrary.delete_all_material_expressions(mat)
 nodes={}
 for index,(key,tex) in enumerate(textures.items()):
  node=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTextureSample,-600,index*250);node.texture=tex
  if key=='Normal':node.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL
  if key=='ORM':node.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
  nodes[key]=node
 if dark:
  multiply=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionMultiply,-200,0)
  factor=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant,-400,100);factor.set_editor_property('r',.12)
  u.MaterialEditingLibrary.connect_material_expressions(factor,'',multiply,'B')
  u.MaterialEditingLibrary.connect_material_expressions(nodes['Base'],'RGB',multiply,'A')
  u.MaterialEditingLibrary.connect_material_property(multiply,'',u.MaterialProperty.MP_BASE_COLOR)
  for index,(prop,value) in enumerate([(u.MaterialProperty.MP_ROUGHNESS,.82),(u.MaterialProperty.MP_METALLIC,0.),(u.MaterialProperty.MP_SPECULAR,.15)]):
   constant=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant,-200,250+index*100);constant.r=value
   u.MaterialEditingLibrary.connect_material_property(constant,'',prop)
 else:
  u.MaterialEditingLibrary.connect_material_property(nodes['Base'],'RGB',u.MaterialProperty.MP_BASE_COLOR)
  u.MaterialEditingLibrary.connect_material_property(nodes['ORM'],'G',u.MaterialProperty.MP_ROUGHNESS)
  u.MaterialEditingLibrary.connect_material_property(nodes['ORM'],'B',u.MaterialProperty.MP_METALLIC)
 u.MaterialEditingLibrary.connect_material_property(nodes['Normal'],'RGB',u.MaterialProperty.MP_NORMAL)
 mat.set_editor_property('two_sided',True);u.MaterialEditingLibrary.recompile_material(mat);eal.save_loaded_asset(mat)
 return mat
mats={False:material(False),True:material(True)}
slots=mesh.get_editor_property('static_materials');assert len(slots)==2,len(slots)
assigned=[]
for i,slot in enumerate(slots):
 dark='charcoal' in str(slot.material_slot_name).lower()
 mesh.set_material(i,mats[dark]);assigned.append({'slot':str(slot.material_slot_name),'dark':dark})
assert any(x['dark'] for x in assigned) and any(not x['dark'] for x in assigned),assigned
eal.save_loaded_asset(mesh);assert eal.save_directory(folder,only_if_is_dirty=True,recursive=True)
bounds=mesh.get_bounds();height=bounds.box_extent.z*2;assert abs(height-320)<.1,height
u.EditorAssetLibrary.sync_browser_to_objects([mesh.get_path_name()])
report={'asset':mesh.get_path_name(),'height_cm':height,'material_slots':assigned,'saved':True}
(root/'Saved/dark_crane_import_report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
