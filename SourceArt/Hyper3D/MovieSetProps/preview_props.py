import bpy, json, math, sys, bmesh
from pathlib import Path
from mathutils import Vector, Matrix
root=Path(__file__).resolve().parent
report=[]
sizes={'01_jib':(2,3.2),'02_crane_camera':(-1,0.55),'03_rails':(-1,4.0),'04_big_box':(-1,1.2),'05_ladder':(2,2.4),'06_light_tall_rectangular':(2,2.6),'07_light_medium_round':(2,1.8),'08_light_short_large':(2,1.3),'09_director_chair':(2,0.95),'10_director_megaphone':(-1,0.5),'11_director_slate':(-1,0.3)}
for src in sorted(root.glob('Individual/*/original.glb')):
    output=src.parent/'preview.png'
    if output.exists() and src.parent.name not in sys.argv: continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(src))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    points=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box]
    lo=Vector(tuple(min(v[i] for v in points) for i in range(3)))
    hi=Vector(tuple(max(v[i] for v in points) for i in range(3)))
    axis,target=sizes[src.parent.name]
    scale=target/(max(hi-lo) if axis<0 else (hi-lo)[axis])
    pivot=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    for o in meshes:
        transform=o.matrix_world.copy()
        for v in o.data.vertices:v.co=(transform@v.co-pivot)*scale
        o.parent=None;o.matrix_world=Matrix.Identity(4)
        o.name=src.parent.name
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:o.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    if len(meshes)>1:
        bpy.ops.object.join()
        meshes=[bpy.context.object]
    if src.parent.name in ['01_jib','02_crane_camera','03_rails']:
        bm=bmesh.new();bm.from_mesh(meshes[0].data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=0.00001)
        bmesh.ops.holes_fill(bm,edges=[e for e in bm.edges if e.is_boundary],sides=100)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(meshes[0].data);bm.free()
    if src.parent.name=='03_rails':
        mesh=meshes[0].data;mesh.materials.clear()
        for title,color in [('RailDarkBrown',(0.055,0.035,0.025,1)),('CrossTieDarkTeal',(0.025,0.07,0.06,1))]:
            mat=bpy.data.materials.new(title);mat.diffuse_color=color;mat.use_nodes=True
            shader=mat.node_tree.nodes.get('Principled BSDF');shader.inputs['Base Color'].default_value=color;shader.inputs['Roughness'].default_value=0.65;shader.inputs['Metallic'].default_value=0.3
            mesh.materials.append(mat)
        width=max(v.co.x for v in mesh.vertices)-min(v.co.x for v in mesh.vertices)
        mesh.update()
        for poly in mesh.polygons:poly.material_index=0 if abs(poly.center.x)>width*0.28 else 1
    bpy.context.scene.unit_settings.system='METRIC'
    bpy.ops.export_scene.gltf(filepath=str(src.parent/(src.parent.name+'.glb')),export_format='GLB',use_selection=True)
    texture_dir=src.parent/'textures';texture_dir.mkdir(exist_ok=True)
    for image in bpy.data.images:
        if image.type=='IMAGE' and image.size[0]>0:
            image.filepath_raw=str(texture_dir/(bpy.path.clean_name(image.name)+'.png'));image.file_format='PNG';image.save()
    bpy.ops.export_scene.fbx(filepath=str(src.parent/(src.parent.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',path_mode='COPY',embed_textures=True,bake_anim=False)
    points=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box]
    bpy.context.view_layer.update()
    points=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box]
    lo=Vector(tuple(min(v[i] for v in points) for i in range(3)))
    hi=Vector(tuple(max(v[i] for v in points) for i in range(3)))
    center=(lo+hi)/2
    extent=max(hi-lo)
    report.append({'name':src.parent.name,'meshes':len(meshes),'vertices':sum(len(o.data.vertices) for o in meshes),'faces':sum(len(o.data.polygons) for o in meshes),'dimensions':list(hi-lo)})
    bpy.ops.object.camera_add(location=center+Vector((1.3,-2,1.1))*extent)
    cam=bpy.context.object
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=extent*1.55
    scene=bpy.context.scene;scene.camera=cam
    for offset,power,size in [((1,-2,3),1100,3),((-2,-1,1),700,2),((0,2,2),900,2)]:
        bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*extent)
        light=bpy.context.object;light.data.energy=power*extent**2;light.data.shape='DISK';light.data.size=size*extent
        light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
    world=bpy.data.worlds.new('Studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(0.23,0.25,0.29,1);world.node_tree.nodes['Background'].inputs[1].default_value=0.5;scene.world=world
    scene.render.engine='CYCLES';scene.cycles.samples=24
    scene.render.resolution_x=800;scene.render.resolution_y=800;scene.render.resolution_percentage=100
    scene.view_settings.view_transform='AgX'
    scene.render.filepath=str(output);bpy.ops.render.render(write_still=True)
    print('PREVIEW_DONE '+src.parent.name,flush=True)
(root/'mesh_report.json').write_text(json.dumps(report,indent=2))
