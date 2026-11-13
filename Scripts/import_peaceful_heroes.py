import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut'
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
for who in ['Swordsman','Archer']:
    path=base+'/Characters/'+who+'Act1';name='SK_'+who+'Act1'
    mesh=u.load_asset(path+'/'+name)
    if not mesh:
        t=u.AssetImportTask();t.filename=str(root/'SourceArt/Polysplit'/f'{who}Act1.fbx');t.destination_path=path;t.destination_name=name
        t.automated=True;t.save=True
        o=u.FbxImportUI();o.import_mesh=True;o.import_as_skeletal=True;o.import_animations=False;o.import_materials=False;o.import_textures=False
        o.skeleton=u.load_asset(base+'/Characters/'+who+'/SK_'+who).skeleton;o.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH
        o.automated_import_should_detect_type=False;t.options=o
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);mesh=u.load_asset(path+'/'+name)
    assert mesh
    mesh.modify();slots=[]
    for old in mesh.materials:
        mat=u.load_asset(base+'/Materials/MI_Polysplit_Hero_'+('Body' if 'Body' in str(old.material_slot_name) else 'Objects'))
        slots.append(u.SkeletalMaterial(material_interface=mat,material_slot_name=old.material_slot_name,uv_channel_data=old.uv_channel_data))
    mesh.materials=slots;u.EditorAssetLibrary.save_loaded_asset(mesh,False)
