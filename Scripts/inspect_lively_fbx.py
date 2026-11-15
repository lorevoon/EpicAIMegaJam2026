import bpy,json
from pathlib import Path
R=Path(__file__).resolve().parents[1]; rows={}
for path in [R/'SourceArt/FilmCrew/v6/Crew_M01_Brown_Crop.fbx',Path('C:/Users/iris6/Downloads/11e2306d-d254-4c6e-aa63-0e7862bfa4c3/base_basic_pbr.fbx')]:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path))
 rows[str(path)]=[dict(name=o.name,type=o.type,dimensions=list(o.dimensions),bones=[b.name for b in o.data.bones] if o.type=='ARMATURE' else [],groups=[v.name for v in o.vertex_groups] if o.type=='MESH' else []) for o in bpy.context.scene.objects]
(R/'Saved/lively_fbx.json').write_text(json.dumps(rows,indent=2))
