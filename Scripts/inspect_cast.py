import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=Path(r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes\BasicHeroes')
result={}
for name in ['M_Mage','M_Rogue','M_Hunter']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(src/(name+'.fbx')),use_anim=False)
    result[name]=[dict(name=o.name,hidden=o.hide_viewport,vertices=len(o.data.vertices)) for o in bpy.context.scene.objects if o.type=='MESH']
(root/'Saved/Stage1Audit/cast_meshes.json').write_text(json.dumps(result,indent=2))
