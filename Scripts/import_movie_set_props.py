"""Import and add the eleven delivered Hyper3D props to the existing Act 1 set."""
import unreal as u,json,shutil,datetime
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'SourceArt/Hyper3D/MovieSetProps/Delivery'
LEVEL='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
BASE='/Game/DirectorsCut/Props/Hyper3D_MovieSet'
TAG='DC_Hyper3D_MovieSet'
eal=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools();aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith(LEVEL+'.'),world.get_path_name()
backup=ROOT/'Saved/MovieSetPropsBackup';backup.mkdir(exist_ok=True)
mapfile=ROOT/'Content/DirectorsCut/Levels/L_DC_Act1_Promise.umap'
shutil.copy2(mapfile,backup/('L_DC_Act1_Promise_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S')+'.umap'))
def transform_values(a):
 p=a.get_actor_location();r=a.get_actor_rotation();s=a.get_actor_scale3d()
 return [p.x,p.y,p.z,r.pitch,r.yaw,r.roll,s.x,s.y,s.z]
before={a.get_path_name():transform_values(a) for a in aes.get_all_level_actors() if TAG not in [str(t) for t in a.tags]}
u.SystemLibrary.execute_console_command(world,'Interchange.FeatureFlags.Import.FBX false')

def import_file(file,folder,name,mesh=False):
 asset=u.load_asset(folder+'/'+name)
 if asset:return asset
 task=u.AssetImportTask();task.filename=str(file);task.destination_path=folder;task.destination_name=name
 task.automated=True;task.replace_existing=False;task.save=True
 if mesh:
  options=u.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=False;options.import_animations=False
  options.import_materials=False;options.import_textures=False;options.automated_import_should_detect_type=False
  options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
  options.static_mesh_import_data.combine_meshes=True
  options.static_mesh_import_data.generate_lightmap_u_vs=True
  options.static_mesh_import_data.auto_generate_collision=False
  task.options=options
 at.import_asset_tasks([task])
 asset=u.load_asset(folder+'/'+name)
 if not asset:
  assets=[u.load_asset(p) for p in task.imported_object_paths]
  asset=next((a for a in assets if isinstance(a,u.StaticMesh if mesh else u.Texture2D)),None)
  assert asset,(file,list(task.imported_object_paths))
 return asset

def mat_for(name,folder,texdir):
 mat=u.load_asset(folder+'/M_'+name)
 if mat:return mat
 mat=at.create_asset('M_'+name,folder,u.Material,u.MaterialFactoryNew())
 for kind,file in [('base','texture_diffuse.png'),('normal','texture_normal.png'),('orm','texture_metallic-texture_roughness.png')]:
  tex=import_file(texdir/file,folder+'/Textures','T_'+name+'_'+kind)
  if kind=='normal':
   tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
   tex.set_editor_property('srgb',False)
   tex.set_editor_property('flip_green_channel',True)
  elif kind=='orm':tex.set_editor_property('srgb',False)
  eal.save_loaded_asset(tex)
  node=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTextureSample,-400,{'base':0,'normal':250,'orm':500}[kind])
  node.texture=tex
  if kind=='base':u.MaterialEditingLibrary.connect_material_property(node,'RGB',u.MaterialProperty.MP_BASE_COLOR)
  elif kind=='normal':
   node.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL
   u.MaterialEditingLibrary.connect_material_property(node,'RGB',u.MaterialProperty.MP_NORMAL)
  else:
   node.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
   u.MaterialEditingLibrary.connect_material_property(node,'G',u.MaterialProperty.MP_ROUGHNESS)
   u.MaterialEditingLibrary.connect_material_property(node,'B',u.MaterialProperty.MP_METALLIC)
 mat.set_editor_property('two_sided',True)
 u.MaterialEditingLibrary.recompile_material(mat);eal.save_loaded_asset(mat)
 return mat

placements={
 '01_jib':(-450,-1280,20,0),
 '02_crane_camera':(-458.5,-1130,212.1,0),
 '03_rails':(-450,-1325,1,0),
 '04_big_box':(-1450,-1810,1,0),
 '05_ladder':(-1750,-1500,1,25),
 '06_light_tall_rectangular':(-1850,-1100,1,30),
 '07_light_medium_round':(1950,-1050,1,150),
 '08_light_short_large':(-700,-1770,1,30),
 '09_director_chair':(-1000,-1880,1,0),
 '10_director_megaphone':(-1475,-1810,79.2,0),
 '11_director_slate':(-1410,-1810,79.2,0),
}
records=[]
for item in json.loads((SOURCE/'asset_manifest.json').read_text()):
 key=item['name'];name=''.join(w.title() for w in key[3:].split('_'));folder=BASE+'/'+name
 mesh=import_file(SOURCE/key/(key+'.fbx'),folder,'SM_'+name,True)
 assert isinstance(mesh,u.StaticMesh)
 if key=='03_rails':
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   mname='M_Rail_'+str(i);mat=u.load_asset(folder+'/'+mname) or at.create_asset(mname,folder,u.Material,u.MaterialFactoryNew())
   u.MaterialEditingLibrary.delete_all_material_expressions(mat)
   color=(.055,.035,.025) if 'brown' in str(slot.material_slot_name).lower() else (.025,.07,.06)
   node=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-200,0);node.constant=u.LinearColor(*color,1)
   u.MaterialEditingLibrary.connect_material_property(node,'',u.MaterialProperty.MP_BASE_COLOR)
   u.MaterialEditingLibrary.recompile_material(mat);eal.save_loaded_asset(mat);mesh.set_material(i,mat)
 else:
  mat=mat_for(name,folder,SOURCE/key/'textures')
  for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,mat)
 eal.save_loaded_asset(mesh)
 label='H3D - '+key[3:].replace('_',' ').title()
 actor=next((a for a in aes.get_all_level_actors() if a.get_actor_label()==label and TAG in [str(t) for t in a.tags]),None)
 if not actor:actor=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector())
 actor.set_actor_label(label);actor.set_folder_path('Act1/Studio/Hyper3D Props');actor.tags=[TAG]
 component=actor.static_mesh_component;component.set_static_mesh(mesh);component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
 x,y,z,yaw=placements[key]
 actor.set_actor_location_and_rotation(u.Vector(x,y,z),u.Rotator(yaw=yaw),False,False)
 scale=1.1695 if key=='02_crane_camera' else 1.
 actor.set_actor_scale3d(u.Vector(scale,scale,scale))
 c,e=actor.get_actor_bounds(False)
 records.append({'key':key,'asset':mesh.get_path_name(),'actor':actor.get_path_name(),'size_cm':[e.x*2,e.y*2,e.z*2],'location':[x,y,z]})
 assert max(e.x,e.y,e.z)>5,(key,'unexpected tiny import',e)
for a in aes.get_all_level_actors():
 if a.get_path_name() in before:assert max(abs(x-y) for x,y in zip(before[a.get_path_name()],transform_values(a)))<0.001,a.get_actor_label()
assert len(records)==11
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
eal.save_directory(BASE,only_if_is_dirty=True,recursive=True)
(ROOT/'Saved/movie_set_import_report.json').write_text(json.dumps({'level':LEVEL,'count':len(records),'existing_actor_transforms_unchanged':True,'assets':records},indent=2))
print('MOVIE_SET_IMPORTED '+json.dumps(records))
