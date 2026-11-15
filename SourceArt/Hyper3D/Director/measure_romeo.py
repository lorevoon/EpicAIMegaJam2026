import bpy,json
from mathutils import Vector
bpy.ops.wm.open_mainfile(filepath=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\Hyper3D\Director\Director_RomeoRig_Preparation.blend')
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name!='Cube']
pts=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
for o in meshes:
 p=[o.matrix_world@v.co for v in o.data.vertices]
 print('MEASURE',o.name,[round(min(v[i] for v in p),4) for i in range(3)],[round(max(v[i] for v in p),4) for i in range(3)])
r=bpy.data.objects['rootSkeleton']
print('BONES',[(b.name,[round(c,3) for c in r.matrix_world@b.head_local]) for b in r.data.bones if any(s in b.name.lower() for s in ['head','neck','hand','foot','arm','thigh','calf','hips'])])
s=bpy.context.scene
cam=s.camera
cam.location=(0,-8,1.0)
cam.rotation_euler=(Vector((0,0,1.0))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=2.65
s.render.engine='BLENDER_WORKBENCH'
s.display.shading.light='STUDIO';s.display.shading.color_type='SINGLE';s.display.shading.single_color=(0.65,0.65,0.65)
s.display.shading.background_type='WORLD';s.world.color=(1,1,1)
s.render.resolution_x=1400;s.render.resolution_y=1400;s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG'
s.render.filepath=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\Hyper3D\Director\References\Romeo_Proportions.png'
bpy.ops.render.render(write_still=True)
