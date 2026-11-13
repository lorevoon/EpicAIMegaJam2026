import bpy,json,sys,bmesh
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(root/(sys.argv[-1] if '--' in sys.argv else '01_jib/original.glb')))
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
for o in bpy.context.selected_objects:
 if o.type=='MESH':
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=0.00001);bm.to_mesh(o.data);bm.free()
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.separate(type='LOOSE');bpy.ops.object.mode_set(mode='OBJECT')
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 vs=[v.co for v in o.data.vertices]
 print(json.dumps({'name':o.name,'verts':len(vs),'min':[round(min(v[i] for v in vs),4) for i in range(3)],'max':[round(max(v[i] for v in vs),4) for i in range(3)]}))
