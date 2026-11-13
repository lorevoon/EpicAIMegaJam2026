import bpy,bmesh,shutil,json
from pathlib import Path
root=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(root/'06_light_tall_rectangular/original.glb'))
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
for o in bpy.context.selected_objects:
 if o.type=='MESH':
  bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=0.00001);bm.to_mesh(o.data);bm.free()
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.separate(type='LOOSE');bpy.ops.object.mode_set(mode='OBJECT')
groups={k:[] for k in ['06_light_tall_rectangular','07_light_medium_round','08_light_short_large']}
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 vs=[v.co for v in o.data.vertices]
 low=[min(v[i] for v in vs) for i in range(3)];high=[max(v[i] for v in vs) for i in range(3)]
 if high[2]<-0.39:continue
 if high[0]<-0.3:key='07_light_medium_round'
 elif low[1]<-0.7:key='06_light_tall_rectangular'
 else:key='08_light_short_large'
 groups[key].append(o)
for name,objects in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 target=root/'Individual'/name/'original.glb';backup=target.with_name('text_attempt.glb')
 if not backup.exists():shutil.copy2(target,backup)
 bpy.ops.export_scene.gltf(filepath=str(target),export_format='GLB',use_selection=True)
 print('EXTRACTED '+name+' '+str(len(objects))+' mesh parts')
(root/'lights_provenance.json').write_text(json.dumps({'source_generation_id':'2b503bd5-dc28-49ad-b50c-78dafa5986e5','method':'Separate disconnected light geometry by spatial cluster, preserving UVs and materials','outputs':list(groups)},indent=2))
