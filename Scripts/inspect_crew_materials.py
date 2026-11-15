import bpy,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];out={}
for file in (R/'SourceArt/LivelySet').glob('Crew*.fbx'):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
 for m in bpy.data.materials:
  bs=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if m.use_nodes else None
  out[m.name]=dict(color=list(bs.inputs['Base Color'].default_value if bs else m.diffuse_color),images=[n.image.filepath for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image] if m.use_nodes else [])
(R/'SourceArt/LivelySet/materials.json').write_text(json.dumps(out,indent=2))
