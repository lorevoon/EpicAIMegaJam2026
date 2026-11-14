"""Persist UE5.8 skeletal mesh usage on the opaque raid material instances."""
import unreal as u,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/OrcColor';out.mkdir(parents=True,exist_ok=True)
usage=getattr(u.MaterialUsage,next(n for n in dir(u.MaterialUsage) if 'SKELETAL' in n));report={}
for kind in ['Body','Objects']:
 name='MI_Act2_OrcOpaque_'+kind;path='/Game/DirectorsCut/Act2/'+name
 source=root/'Content/DirectorsCut/Act2'/f'{name}.uasset';backup=out/f'{name}.before.uasset'
 if not backup.exists():shutil.copy2(source,backup)
 m=u.load_asset(path);assert m
 before=u.MaterialEditingLibrary.has_material_usage(m,usage)
 u.MaterialEditingLibrary.set_material_usage_override(m,usage,True,True)
 u.MaterialEditingLibrary.update_material_instance(m)
 assert u.MaterialEditingLibrary.has_material_usage(m,usage)
 assert u.MaterialEditingLibrary.has_material_usage_override(m,usage)
 assert u.EditorAssetLibrary.save_loaded_asset(m,False)
 report[name]={'before':before,'skeletal_mesh_usage':True,'explicit_usage_override':True,'parent':m.parent.get_path_name(),'palette_preserved':str(m.get_editor_property('vector_parameter_values'))}
(out/'fix.json').write_text(json.dumps(report,indent=2));print('ORC_MATERIAL_USAGE_FIXED')
