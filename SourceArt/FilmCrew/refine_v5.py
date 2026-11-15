import bpy,bmesh,math
from mathutils import Vector
from mathutils.kdtree import KDTree
from math import sin,cos,pi,exp
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
for o in [o for o in bpy.context.scene.objects if '_Trousers_Straight' in o.name]:
 names=[g.name for g in o.vertex_groups];old=o.data;kd=KDTree(len(old.vertices))
 for v in old.vertices:kd.insert(v.co,v.index)
 kd.balance();oldweights=[[(g.group,g.weight) for g in v.groups] for v in old.vertices]
 vs=[];fs=[];tops=[]
 def add(p):vs.append(p);return len(vs)-1
 for side in [1,-1]:
  rings=[]
  for z,rx,ry in [(.145,.109,.105),(.16,.111,.107),(.19,.119,.115),(.23,.111,.107),(.28,.116,.112),(.36,.121,.116),(.46,.125,.12),(.56,.13,.125),(.68,.136,.132),(.8,.143,.145),(.9,.148,.157),(.99,.15,.17)]:
   ring=[];cx=side*(.15+.006*max(0,(.9-z)/.75));cy=.016+.04*max(0,(.4-z)/.255)
   for j in range(24):
    a=2*pi*j/24;inner=max(0,-side*cos(a));zz=z
    if z==.99:zz-=.055*inner**3
    f=.0035*exp(-((z-.19)/.045)**2)*cos(2*a+side*.5)
    f+=.002*exp(-((z-.56)/.07)**2)*max(0,-sin(a))
    ring.append(add((cx+(rx+f)*cos(a),cy+(ry+f)*sin(a),zz)))
   if rings:
    for j in range(24):fs.append((rings[-1][j],rings[-1][(j+1)%24],ring[(j+1)%24],ring[j]))
   rings.append(ring)
  tops.append(rings[-1])
 right,left=tops
 # Sew the inner arcs into a continuous rounded crotch saddle.
 ra=[right[j] for j in [10,11,12,13,14]];la=[left[j] for j in [2,1,0,23,22]]
 for j in range(4):fs.append((ra[j],la[j],la[j+1],ra[j+1]))
 # Outer boundary of the saddle becomes a single continuous pelvis shell.
 boundary=[right[j%24] for j in range(14,35)]+[left[j%24] for j in range(2,23)]
 # Endpoint bridge edges are part of this loop; preserve their narrow front/back center spacing.
 prev=boundary
 for z,rx,ry in [(1.06,.29,.176),(1.15,.265,.166),(1.22,.237,.155),(1.24,.235,.154)]:
  ring=[]
  for idx in boundary:
   x,y,_=vs[idx];a=math.atan2((y-.016)/.17,x/.30)
   ring.append(add((rx*cos(a),.016+ry*sin(a),z)))
  for j in range(len(prev)):fs.append((prev[j],prev[(j+1)%len(prev)],ring[(j+1)%len(prev)],ring[j]))
  prev=ring
 me=bpy.data.meshes.new(o.name+'_Retopologized');me.from_pydata(vs,[],fs);me.update();bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();o.data=me
 o.vertex_groups.clear()
 for name in names:o.vertex_groups.new(name=name)
 for mat in old.materials:me.materials.append(mat)
 for v in me.vertices:
  _,i,_=kd.find(v.co)
  for g,w in oldweights[i]:o.vertex_groups[g].add([v.index],w,'REPLACE')
 for p in me.polygons:p.use_smooth=True
 # Subdivide the continuous garment shell before skinning.
 flags=[(m,m.show_viewport) for m in o.modifiers]
 for m,f in flags:m.show_viewport=False
 bpy.context.view_layer.objects.active=o;m=o.modifiers.new('Continuous cloth surface','SUBSURF');m.levels=2
 bpy.ops.object.modifier_apply(modifier=m.name)
 for m,f in flags:m.show_viewport=f
for o in list(bpy.context.scene.objects):
 if '_TailoringSeam' in o.name:bpy.data.objects.remove(o,do_unlink=True)
for r in bpy.context.scene.objects:
 if r.type=='ARMATURE':r.data.pose_position='POSE'
print('Rebuilt continuous trouser topology on five original rigs')

