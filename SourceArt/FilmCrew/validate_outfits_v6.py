import bpy,os,json,struct
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew\v6'
original_scene=bpy.context.window.scene
test=bpy.data.scenes.new('Temporary_Export_Validation');bpy.context.window.scene=test
report=[]
try:
 for filename in sorted(os.listdir(OUT)):
  if not filename.endswith('.fbx'):continue
  before=set(bpy.data.objects)
  bpy.ops.import_scene.fbx(filepath=OUT+'/'+filename);bpy.context.view_layer.update()
  imported=set(bpy.data.objects)-before;rigs=[o for o in imported if o.type=='ARMATURE'];meshes=[o for o in imported if o.type=='MESH']
  pts=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
  bounds=[(min(v[i] for v in pts),max(v[i] for v in pts)) for i in range(3)]
  unweighted=sum(not any(g.weight>0 for g in v.groups) for o in meshes for v in o.data.vertices)
  assert len(rigs)==1 and len(rigs[0].data.bones)==99 and unweighted==0,(filename,len(rigs),unweighted)
  assert 1.95<bounds[2][1]<2.2 and max(abs(bounds[0][0]),abs(bounds[0][1]))<1.2,(filename,bounds)
  shirt=next(o for o in meshes if '_TShirt_Fitted' in o.name)
  assert len(shirt.data.vertices)>500
  glb=OUT+'/'+filename.replace('.fbx','.glb')
  with open(glb,'rb') as f:
   magic,ver,size=struct.unpack('<4sII',f.read(12));n,kind=struct.unpack('<II',f.read(8));data=json.loads(f.read(n))
  assert magic==b'glTF' and ver==2 and size==os.path.getsize(glb) and data.get('skins')
  result={'file':filename,'bones':99,'mesh_count':len(meshes),'unweighted':unweighted,'bounds_m':bounds,'triangles':sum(len(p.vertices)-2 for o in meshes for p in o.data.polygons),'glb_valid':True}
  report.append(result)
  for o in imported:bpy.data.objects.remove(o,do_unlink=True)
finally:
 bpy.context.window.scene=original_scene;bpy.data.scenes.remove(test)
json.dump(report,open(OUT+'/validation.json','w'),indent=2)
print('VALIDATED_V6',json.dumps(report))




