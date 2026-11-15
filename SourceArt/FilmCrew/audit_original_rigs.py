import bpy,json,os
from mathutils import Matrix
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
SRC=r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes'
report=[]
for gender in ['F','M']:
 existing=[o for o in bpy.data.objects if o.type=='ARMATURE' and o.name.startswith('Crew_'+gender)]
 before=set(bpy.data.objects)
 bpy.ops.import_scene.fbx(filepath=SRC+'/BaseCharacters/Base'+('Female' if gender=='F' else 'Male')+'.fbx');bpy.context.view_layer.update()
 imported=set(bpy.data.objects)-before;source=next(o for o in imported if o.type=='ARMATURE')
 for rig in existing:
  diffs=[];maxerr=0
  for b in source.data.bones:
   r=rig.data.bones.get(b.name)
   if not r:diffs.append(b.name+': missing');continue
   err=max(abs(b.matrix_local[i][j]-r.matrix_local[i][j]) for i in range(4) for j in range(4));err=max(err,(b.head_local-r.head_local).length,(b.tail_local-r.tail_local).length);maxerr=max(maxerr,err)
   if err>1e-5:diffs.append(b.name+': rest transform')
   if (b.parent.name if b.parent else None)!=(r.parent.name if r.parent else None):diffs.append(b.name+': parent')
   if b.use_connect!=r.use_connect or b.use_deform!=r.use_deform:diffs.append(b.name+': flags')
  pose=[b.name for b in rig.pose.bones if max(abs(b.matrix_basis[i][j]-(1 if i==j else 0)) for i in range(4) for j in range(4))>1e-6]
  report.append({'rig':rig.name,'source_bones':len(source.data.bones),'current_bones':len(rig.data.bones),'max_rest_difference_source_units':maxerr,'differences':diffs,'posed_bones':pose})
 # Measure the source body, independently of the preview's pose.
 for part in ['TopBody','BottomBody']:
  o=next(o for o in imported if o.name.split('.')[0]==gender+'_'+part);coords=[o.matrix_world@v.co for v in o.data.vertices]
  print('SOURCE',gender,part)
  for z in ([1.2,1.3,1.4,1.5,1.6,1.68] if part=='TopBody' else [.15,.3,.5,.7,.9,1.1,1.2]):
   near=[v for v in coords if abs(v.z-z)<.07 and (abs(v.x)<.29 if part=='TopBody' else True)]
   if near:print(z,[(round(min(v[i] for v in near),3),round(max(v[i] for v in near),3)) for i in range(3)])
 for o in imported:bpy.data.objects.remove(o,do_unlink=True)
json.dump(report,open(OUT+'/rig_audit_v3.json','w'),indent=2)
print('RIG_AUDIT',json.dumps(report))
