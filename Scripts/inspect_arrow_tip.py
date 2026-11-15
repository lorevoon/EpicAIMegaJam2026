import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'SourceArt/Act2/BowRevision6/ArrowBasic.fbx'))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH')
vs=[list(v.co) for v in o.data.vertices];vs.sort(key=lambda v:v[2])
(root/'Saved/Act2/BowRevision6/arrow_tip.json').write_text(json.dumps({'negative_end':vs[:20],'positive_end':vs[-20:]},indent=2))
