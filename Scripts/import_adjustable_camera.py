import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder='/Game/DirectorsCut/Props/AdjustableCinemaCamera'
original=u.load_asset('/Game/Fab/CINEMA_CAMERA_with_simple_tripod/cinema_camera_with_simple_tripod/StaticMeshes/cinema_camera_with_simple_tripod')
materials={str(s.material_slot_name):s.material_interface for s in original.static_materials}
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
report={}
for part in ['Base','Neck','Head']:
 name='SM_Cinema'+part
 task=u.AssetImportTask();task.filename=str(root/'SourceArt/AdjustableCinemaCamera'/(name+'.fbx'));task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 opts=u.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False;opts.automated_import_should_detect_type=False;opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
 opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;task.options=opts
 u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 mesh=u.load_asset(folder+'/'+name);assert mesh,name
 for i,s in enumerate(mesh.static_materials):
  key=str(s.material_slot_name)
  assert key in materials,key
  mesh.set_material(i,materials[key])
 u.EditorAssetLibrary.save_loaded_asset(mesh)
 b=mesh.get_bounding_box();report[part]={'min':list(b.min.to_tuple()),'max':list(b.max.to_tuple())}
(root/'Saved/camera_parts_import.json').write_text(json.dumps(report,indent=2))
