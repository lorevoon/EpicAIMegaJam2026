import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
report={}
for file in root.joinpath('SourceArt/Mixamo/Requested').glob('*.fbx'):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(file))
    report[file.name]=[{'name':a.name,'frames':list(a.frame_range)} for a in bpy.data.actions]
root.joinpath('Saved/supplied_fbx_inspection.json').write_text(json.dumps(report,indent=2))
