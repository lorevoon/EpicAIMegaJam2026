import bpy, os, json
from mathutils import Vector
out=os.path.dirname(os.path.abspath(__file__));report=[]
for f in sorted(os.listdir(out)):
 if not f.endswith('.fbx'):continue
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=out+'/'+f);bpy.context.view_layer.update()
 rigs=[o for o in bpy.data.objects if o.type=='ARMATURE'];meshes=[o for o in bpy.data.objects if o.type=='MESH'];coords=[];unweighted=0
 for o in meshes:
  coords.extend(o.matrix_world@v.co for v in o.data.vertices)
  unweighted+=sum(not any(g.weight>0 for g in v.groups) for v in o.data.vertices)
 bounds=[(min(v[i] for v in coords),max(v[i] for v in coords)) for i in range(3)]
 result={'file':f,'rigs':len(rigs),'bones':len(rigs[0].data.bones),'meshes':len(meshes),'unweighted_vertices':unweighted,'bounds_m':bounds,'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)}
 assert len(rigs)==1 and unweighted==0,result
 assert 1.9<bounds[2][1]<2.3 and bounds[2][0]>-.1,result
 report.append(result)
json.dump(report,open(out+'/validation.json','w'),indent=2);print('VALIDATED',report)
