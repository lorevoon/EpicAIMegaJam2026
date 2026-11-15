import bpy,os,json,bmesh
from mathutils import Vector
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
DEST=OUT+'/v6';os.makedirs(DEST,exist_ok=True)
records=[]
rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE' and o.name.startswith('Crew_')]
for rig in rigs:
 name=rig.name.removesuffix('_Rig');saved_loc=rig.location.copy();saved_pose=rig.data.pose_position
 rig.location=(0,0,0);rig.data.pose_position='REST';bpy.context.view_layer.update()
 objects=[rig]+[o for o in bpy.context.scene.objects if o.type=='MESH' and any(m.type=='ARMATURE' and m.object==rig for m in o.modifiers)]
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=rig
 missing=sum(sum(not any(g.weight>0 for g in v.groups) for v in o.data.vertices) for o in objects if o.type=='MESH')
 assert missing==0,(name,missing)
 bpy.ops.export_scene.fbx(filepath=DEST+'/'+name+'.fbx',use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,path_mode='COPY',embed_textures=True)
 bpy.ops.export_scene.gltf(filepath=DEST+'/'+name+'.glb',use_selection=True,export_animations=False)
 records.append({'name':name,'bones':len(rig.data.bones),'mesh_objects':len(objects)-1,'unweighted_vertices':missing,'shirt_thickness_m':.00065,'trouser_thickness_m':.0009})
 rig.location=saved_loc;rig.data.pose_position=saved_pose;bpy.context.view_layer.update()
json.dump(records,open(DEST+'/manifest.json','w'),indent=2)

for rig in rigs:rig.data.pose_position='REST'
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/FilmCrew_Outfits_v6.blend')
print('EXPORTED_V6',records)




