import bpy,shutil,json
from pathlib import Path
root=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(root/'01_jib/original.glb'))
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.separate(type='LOOSE');bpy.ops.object.mode_set(mode='OBJECT')
groups={k:[] for k in ['01_jib','02_crane_camera','03_rails']}
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 vs=[v.co for v in o.data.vertices]
 low=[min(v[i] for v in vs) for i in range(3)]
 high=[max(v[i] for v in vs) for i in range(3)]
 if high[2]<-0.68:key='03_rails'
 elif low[1]<-0.69 and high[2]<0.49 and low[2]>0.17:key='02_crane_camera'
 else:key='01_jib'
 groups[key].append(o)
for name,objects in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 target=root/'Individual'/name/'original.glb'
 backup=target.with_name('text_attempt.glb')
 if not backup.exists():shutil.copy2(target,backup)
 bpy.ops.export_scene.gltf(filepath=str(target),export_format='GLB',use_selection=True)
 print('EXTRACTED '+name+' '+str(len(objects))+' mesh parts')
(root/'crane_provenance.json').write_text(json.dumps({'source_generation_id':'60af38ff-c0fa-4c2e-97e4-75a17148bd7b','method':'Separate disconnected geometry by spatial region, preserving original UVs and materials','outputs':list(groups)},indent=2))
