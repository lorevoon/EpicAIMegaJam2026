import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
t=u.AssetImportTask();t.filename=str(root/'Saved/FilmSetAudit/Act1_RealGround.obj');t.destination_path='/Game/DirectorsCut/Environment';t.destination_name='SM_Act1_RealGround';t.automated=True;t.replace_existing=True;t.save=True
op=u.FbxImportUI();op.import_mesh=True;op.import_materials=False;op.import_textures=False;op.import_as_skeletal=False;op.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;op.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False);op.static_mesh_import_data.set_editor_property('auto_generate_collision',False);t.options=op
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
mesh=u.load_asset('/Game/DirectorsCut/Environment/SM_Act1_RealGround');assert mesh.get_bounds().box_extent.z<1000
assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
(root/'Saved/FilmSetAudit/real_ground_bounds.txt').write_text(str(mesh.get_bounds()))
u.log('GROUND_REIMPORT_SAVED')
