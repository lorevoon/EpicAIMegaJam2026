import bpy,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1]; O=R/'SourceArt/LivelySet';O.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='C:/Users/iris6/Downloads/Neutral Idle.fbx')
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
rig.animation_data_clear()
for o in list(bpy.context.scene.objects):
 if o!=rig:bpy.data.objects.remove(o,do_unlink=True)
bpy.ops.import_scene.fbx(filepath='C:/Users/iris6/Downloads/11e2306d-d254-4c6e-aa63-0e7862bfa4c3/base_basic_pbr.fbx')
mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.context.view_layer.objects.active=mesh
dec=mesh.modifiers.new('Game-ready director','DECIMATE');dec.ratio=min(1,45000/len(mesh.data.polygons))
bpy.ops.object.modifier_apply(modifier=dec.name)
coords=[mesh.matrix_world@Vector(c) for c in mesh.bound_box]
bottom=min(c.z for c in coords);top=max(c.z for c in coords)
print('DIRECTOR_BOUNDS',bottom,top,'RIG',[(b.name,list(rig.matrix_world@b.head_local)) for b in rig.data.bones if any(s in b.name for s in ['Head','Foot','Hips'])])
# Fit the supplied T-pose to the neutral Mixamo rig, retaining the supplied texture.
feet=min((rig.matrix_world@b.head_local).z for b in rig.data.bones if 'ToeBase' in b.name)
head=max((rig.matrix_world@b.tail_local).z for b in rig.data.bones if 'Head' in b.name)
scale=(head-feet)/(top-bottom)
mesh.scale*=scale;mesh.location.z+=feet-bottom*scale
bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True);bpy.context.view_layer.objects.active=mesh
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
assert len(mesh.vertex_groups)>10
bpy.ops.export_scene.fbx(filepath=str(O/'SK_Director.fbx'),use_selection=True,add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='COPY',embed_textures=True)
(O/'rig_report.json').write_text(json.dumps(dict(vertex_groups=len(mesh.vertex_groups),vertices=len(mesh.data.vertices),scale=scale)))
