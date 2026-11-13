"""Adjust 3D material assignments; retain the original texture images and mesh."""
import bpy,json,shutil,struct,numpy as np
from pathlib import Path
root=Path(__file__).resolve().parent
out=root/'DarkerBlack';out.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root/'CraneCamera_3p2m.blend'))
model=next(o for o in bpy.context.scene.objects if o.type=='MESH')
mesh=model.data;original=mesh.materials[0]
dark=original.copy();dark.name='Crane_CharcoalBlack'
shader=next(n for n in dark.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
base=shader.inputs['Base Color'];source=base.links[0].from_socket
texture=source.node.image
mix=dark.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1.;mix.inputs[2].default_value=(.12,.12,.12,1)
dark.node_tree.links.new(source,mix.inputs[1]);dark.node_tree.links.new(mix.outputs[0],base)
base.default_value=(.12,.12,.12,1)
for key,value in [('Metallic',.0),('Roughness',.82),('Specular IOR Level',.15)]:
 for link in list(shader.inputs[key].links):dark.node_tree.links.remove(link)
 shader.inputs[key].default_value=value
mesh.materials.append(dark)
width,height=texture.size
pixels=np.empty(width*height*4,dtype=np.float32);texture.pixels.foreach_get(pixels);pixels=pixels.reshape(height,width,4)
uv=mesh.uv_layers.active.data
counts={'darkened':0,'accent_retained':0}
for face in mesh.polygons:
 samples=[]
 coords=[uv[i].uv.copy() for i in face.loop_indices]
 center=sum(coords[1:],coords[0].copy())/len(coords)
 for point in [center]+[center.lerp(c,.5) for c in coords]:
  samples.append(pixels[min(height-1,int((point.y%1)*height)),min(width-1,int((point.x%1)*width)),:3])
 rgb=np.mean(samples,axis=0);r,g,b=rgb
 red=r>g*1.22 and r>b*1.15
 bright=min(rgb)>.6
 colored=max(rgb)-min(rgb)>.18 and max(rgb)>min(rgb)*1.6
 preserve=red or bright or colored
 face.material_index=0 if preserve else 1
 counts['accent_retained' if preserve else 'darkened']+=1
bpy.ops.object.select_all(action='DESELECT');model.select_set(True);bpy.context.view_layer.objects.active=model
bpy.ops.export_scene.gltf(filepath=str(out/'CraneCamera_3p2m.glb'),export_format='GLB',use_selection=True)
# glTF exporter does not preserve every MixRGB multiplier; write the standard PBR factor explicitly.
glb=out/'CraneCamera_3p2m.glb';data=glb.read_bytes();length=struct.unpack_from('<I',data,12)[0]
doc=json.loads(data[20:20+length])
for mat in doc['materials']:
 if mat.get('name')=='Crane_CharcoalBlack':mat['pbrMetallicRoughness']['baseColorFactor']=[.12,.12,.12,1]
encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4)
tail=data[20+length:]
glb.write_bytes(struct.pack('<4sII',b'glTF',2,20+len(encoded)+len(tail))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+tail)
shutil.copytree(root/'textures',out/'textures',dirs_exist_ok=True)
for image in bpy.data.images:
 if image.type=='IMAGE' and image.size[0]>0:
  target=out/'textures'/(bpy.path.clean_name(image.name)+'.png')
  if target.exists():image.filepath=str(target)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'CraneCamera_3p2m.blend'))
# FBX stores diffuse factor and texture separately; use the direct texture link during export.
dark.node_tree.links.new(source,base)
bpy.ops.export_scene.fbx(filepath=str(out/'CraneCamera_3p2m.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',path_mode='COPY',embed_textures=True,bake_anim=False)
dark.node_tree.links.new(mix.outputs[0],base)
info=json.loads((root/'asset_info.json').read_text());info['material_revision']='Darker charcoal body with original red and bright accents';info['face_assignments']=counts
(out/'asset_info.json').write_text(json.dumps(info,indent=2))
# Use the identical preview setup and lighting as the original, for comparison.
script=(root/'prepare_crane.py').read_text();render=script[script.index('center=Vector'):]
from mathutils import Vector
dimensions=list(model.dimensions);root=out
exec(render)
print('MATERIAL_REVISION '+json.dumps(counts))

