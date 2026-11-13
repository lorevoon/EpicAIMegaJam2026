import bpy,json,math
from pathlib import Path
from mathutils import Vector,Matrix
root=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(root/'CraneCamera_raw.glb'))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
lo=Vector([min(v[i] for v in points) for i in range(3)]);hi=Vector([max(v[i] for v in points) for i in range(3)])
factor=3.2/(hi.z-lo.z);pivot=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for o in meshes:
 matrix=o.matrix_world.copy()
 for v in o.data.vertices:v.co=(matrix@v.co-pivot)*factor
 o.parent=None;o.matrix_world=Matrix.Identity(4)
bpy.ops.object.select_all(action='DESELECT')
for o in meshes:o.select_set(True)
bpy.context.view_layer.objects.active=meshes[0]
if len(meshes)>1:bpy.ops.object.join()
model=bpy.context.object;model.name='SM_CraneCamera';model.data.name='CraneCamera_Mesh'
bpy.context.view_layer.update()
bpy.context.scene.unit_settings.system='METRIC'
bpy.ops.export_scene.gltf(filepath=str(root/'CraneCamera_3p2m.glb'),export_format='GLB',use_selection=True)
textures=root/'textures';textures.mkdir(exist_ok=True)
for im in bpy.data.images:
 if im.type=='IMAGE' and im.size[0]>0:
  im.filepath_raw=str(textures/(bpy.path.clean_name(im.name)+'.png'));im.file_format='PNG';im.save()
bpy.ops.export_scene.fbx(filepath=str(root/'CraneCamera_3p2m.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',path_mode='COPY',embed_textures=True,bake_anim=False)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CraneCamera_3p2m.blend'))
dimensions=list(model.dimensions);assert abs(dimensions[2]-3.2)<0.001
(root/'asset_info.json').write_text(json.dumps({'name':'CraneCamera','generation_id':'58c524cc-e61f-4ecb-8ffc-52395124b9bd','dimensions_m':dimensions,'height_m':3.2,'pivot':'ground center','mesh_objects':1,'vertices':len(model.data.vertices),'faces':len(model.data.polygons),'static_mesh':True,'rigged':False},indent=2))
center=Vector((0,0,1.6));extent=max(dimensions)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.resolution_x=1200;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
world=bpy.data.worlds.new('Studio');world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.18,.21,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.4;scene.world=world
for offset,power,size in [((1,-2,3),900,3),((-2,-1,1),500,2),((0,2,2),750,2)]:
 bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*extent)
 light=bpy.context.object;light.data.energy=power*extent**2;light.data.shape='DISK';light.data.size=size*extent
 light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=extent*1.3
scene.view_settings.view_transform='AgX'
for label,offset in [('preview',(1.2,-2,0.65)),('preview_back',(-1.2,2,.65))]:
 camera.location=center+Vector(offset)*extent;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(root/(label+'.png'));bpy.ops.render.render(write_still=True)
print('CRANE_READY '+json.dumps(dimensions))
