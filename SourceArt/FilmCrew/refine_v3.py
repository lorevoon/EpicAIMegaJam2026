"""V3: fitted source-derived fabric; original armature data is read-only."""
import bpy,bmesh,math,os,json
from mathutils import Vector,Matrix
from math import sin,cos,pi,exp
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
SRC=r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes'
# Reuse only generic mesh/material helpers, not V2 garment construction.
helpers=open(OUT+'/revise_outfits.py',encoding='utf-8').read().split('# Read untouched donor geometry')[0]
exec(compile(helpers,'v3_helpers','exec'))
cotton=material('V3_Cotton_Black',(.007,.009,.012),.96)
twill=material('V3_Trousers_Black',(.012,.015,.019),.92)
stitch=material('V3_Seams',(.021,.025,.03),.95)
donors={};imported_all=[]
for gender in ['F','M']:
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=SRC+'/BaseCharacters/Base'+('Female' if gender=='F' else 'Male')+'.fbx');bpy.context.view_layer.update()
 imported=set(bpy.data.objects)-before;imported_all.extend(imported);donors[gender]={}
 for part in ['TopBody','BottomBody']:
  o=next(o for o in imported if o.name.split('.')[0]==gender+'_'+part);donors[gender][part]=o
  coords=[o.matrix_world@v.co for v in o.data.vertices];tree=KDTree(len(coords));weights=[]
  for v,co in zip(o.data.vertices,coords):tree.insert(co,v.index);weights.append([(o.vertex_groups[g.group].name,g.weight) for g in v.groups])
  tree.balance();donors[gender][part+'_weights']=(tree,weights)

def largest_component(bm):
 seen=set();parts=[]
 for v in bm.verts:
  if v in seen:continue
  stack=[v];part=[];seen.add(v)
  while stack:
   q=stack.pop();part.append(q)
   for e in q.link_edges:
    n=e.other_vert(q)
    if n not in seen:seen.add(n);stack.append(n)
  parts.append(part)
 keep=max(parts,key=len);bmesh.ops.delete(bm,geom=[v for p in parts if p is not keep for v in p],context='VERTS')
def interp(z,keys):
 if z<=keys[0][0]:return keys[0][1]
 for (a,x),(b,y) in zip(keys,keys[1:]):
  if z<=b:return x+(y-x)*(z-a)/(b-a)
 return keys[-1][1]
def source_copy(part,name,gender,rig):
 src=donors[gender][part];o=src.copy();o.data=src.data.copy();bpy.context.collection.objects.link(o);o.name=name;o.data.transform(src.matrix_world)
 o.parent=rig;o.matrix_world=Matrix.Translation(rig.matrix_world.translation)
 o.animation_data_clear();o.hide_viewport=False;o.hide_render=False
 for m in o.modifiers:
  if m.type=='ARMATURE':m.object=rig
 return o
def tshirt_v3(name,gender,rig):
 o=source_copy('TopBody',name+'_TShirt_Fitted',gender,rig);o.data.materials.clear();o.data.materials.append(cotton)
 for p in o.data.polygons:p.material_index=0
 bm=bmesh.new();bm.from_mesh(o.data)
 for side in [-1,1]:
  bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,plane_co=(side*.425,0,0),plane_no=(side,0,0),clear_outer=True,clear_inner=False)
 largest_component(bm)
 # Source anatomy is the fitting guide. Ease is millimetres, not inflated tubes.
 for v in bm.verts:
  x,y,z=v.co
  if abs(x)<.23 and z<1.56:
   a=math.atan2(y-.005,x)
   rx=interp(z,[(1.2,.221),(1.32,.196),(1.43,.218),(1.56,.253)])
   ry=interp(z,[(1.2,.177),(1.32,.157),(1.43,.187),(1.56,.187)])
   v.co.x=rx*cos(a);v.co.y=.005+ry*sin(a)
  else:
   v.co+=v.normal*.006
  if z<1.27:v.co.z-=.04*max(0,(1.27-z)/.071)
  if v.is_boundary and z<1.3:
   a=math.atan2(y-.005,x);v.co.x=.221*cos(a);v.co.y=.005+.177*sin(a);v.co.z=1.16
 bm.normal_update()
 # A light relaxation removes muscular ridges without inflating the shoulders.
 movable=[v for v in bm.verts if not v.is_boundary]
 bmesh.ops.smooth_vert(bm,verts=movable,factor=.12,use_axis_x=True,use_axis_y=True,use_axis_z=True)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
 # Keep subdivision ahead of skinning so the garment's weights interpolate.
 for m in o.modifiers:
  if m.type=='ARMATURE':m.show_viewport=False;m.show_render=False
 subdiv(o,1)
 for m in o.modifiers:
  if m.type=='ARMATURE':m.show_viewport=True;m.show_render=True
 # Two shallow tension folds at the waist sides, not periodic waves.
 for v in o.data.vertices:
  x,y,z=v.co
  if y<-.04 and 1.17<z<1.38:
   d=z-(1.20+.15*abs(x));f=.003*exp(-(d/.015)**2)*exp(-((abs(x)-.135)/.075)**2)
   v.co.y-=f
 # Maintain a small, measured clearance over the original upper-arm surface.
 src=donors[gender]['TopBody']
 surface=BVHTree.FromPolygons([src.matrix_world@v.co for v in src.data.vertices],[list(p.vertices) for p in src.data.polygons])
 for v in o.data.vertices:
  if abs(v.co.x)>.20:
   hit,n,face,dist=surface.find_nearest(v.co)
   if hit is not None and (v.co-hit).dot(n)<.009:v.co=hit+n*.009
 for p in o.data.polygons:p.use_smooth=True
 thicken(o,.00065)
 print('SHIRT',name,len(o.data.vertices))
 return o

def trousers_v3(name,gender,rig):
 src=donors[gender]['BottomBody'];me=src.data.copy();me.transform(src.matrix_world)
 bm=bmesh.new();bm.from_mesh(me);largest_component(bm)
 bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.000001,plane_co=(0,0,.86),plane_no=(0,0,-1),clear_outer=True,clear_inner=False)
 # Find the two open thigh loops and extrude straight fabric legs from them.
 boundary=[e for e in bm.edges if e.is_boundary and all(v.co.z<.861 for v in e.verts)]
 seen=set();loops=[]
 for e in boundary:
  if e in seen:continue
  edges=[];todo=[e];seen.add(e)
  while todo:
   q=todo.pop();edges.append(q)
   for v in q.verts:
    for ne in v.link_edges:
     if ne in boundary and ne not in seen:seen.add(ne);todo.append(ne)
  verts=list({v for e in edges for v in e.verts});loops.append(verts)
 for verts in loops:
  side=1 if sum(v.co.x for v in verts)>0 else -1
  cx=sum(v.co.x for v in verts)/len(verts);cy=sum(v.co.y for v in verts)/len(verts)
  verts.sort(key=lambda v:math.atan2(v.co.y-cy,v.co.x-cx));angles=[math.atan2(v.co.y-cy,v.co.x-cx) for v in verts]
  prev=verts
  for z in [.82,.74,.64,.58,.55,.52,.49,.43,.34,.25,.20,.182,.165,.155]:
   centerx=side*interp(z,[(.155,.144),(.54,.1535),(.86,.163)])
   centery=interp(z,[(.155,.062),(.54,.005),(.86,.006)])
   rx=interp(z,[(.155,.073),(.54,.081),(.86,.10)])
   ry=interp(z,[(.155,.078),(.54,.089),(.86,.115)])
   ring=[]
   for a in angles:
    # Localized front-knee and cuff breaks, kept to 1-3 mm.
    front=max(0,-sin(a));f=.0025*exp(-((z-(.525+.016*cos(a)))/.014)**2)*front
    f+=.002*exp(-((z-(.185+.008*cos(a)))/.009)**2)
    v=bm.verts.new((centerx+(rx+f)*cos(a),centery+(ry+f)*sin(a),z));ring.append(v)
   for j in range(len(prev)):bm.faces.new((prev[j],prev[(j+1)%len(prev)],ring[(j+1)%len(ring)],ring[j]))
   prev=ring
 # A few millimetres of ease over the retained source pelvis.
 for v in bm.verts:
  if v.co.z>=.86:
   v.co.x*=1.025;v.co.y*=1.025
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
 o=bpy.data.objects.new(name+'_Trousers_Straight',me);bpy.context.collection.objects.link(o);me.materials.clear();me.materials.append(twill)
 for p in me.polygons:p.material_index=0;p.use_smooth=True
 subdiv(o,1);attach(o,rig,donors[gender]['BottomBody_weights']);thicken(o,.0009)
 # Subtle conventional pocket openings and fly, projected onto the actual fabric.
 surface=BVHTree.FromPolygons([v.co for v in o.data.vertices],[list(p.vertices) for p in o.data.polygons])
 paths=[]
 for s in [-1,1]:paths.append([(s*(.145+.06*t),1.14-.13*t) for t in [j/12 for j in range(13)]])
 paths.append([(.018+.015*sin(pi*t),1.145-.17*t) for t in [j/16 for j in range(17)]])
 for i,path in enumerate(paths):
  pts=[]
  for x,z in path:
   hit=surface.ray_cast(Vector((x,-1,z)),Vector((0,1,0)))[0]
   if hit is not None:pts.append((hit.x,hit.y-.0009,hit.z))
  if len(pts)>2:attach(tube(name+'_TailoringSeam'+str(i),pts,.00075,stitch),rig,donors[gender]['BottomBody_weights'])
 print('TROUSERS',name,'loops',len(loops),'vertices',len(me.vertices))
 return o

rigs=[o for o in bpy.data.objects if o.type=='ARMATURE' and o.name.startswith('Crew_')]
for rig in rigs:
 name=rig.name.removesuffix('_Rig');gender='F' if '_F0' in name else 'M'
 for o in list(bpy.data.objects):
  if o.name.startswith(name) and any(t in o.name for t in ['_TShirt','_Trousers','_NeckBinding','_ShirtHem','_PocketSeam','_TailoringSeam']):bpy.data.objects.remove(o,do_unlink=True)
 # Restore the untouched source arm skin up to the sleeve, removing the old gap.
 old=next(o for o in bpy.data.objects if o.name.startswith(name) and '_TopBody' in o.name)
 skin=old.data.materials[0];oldname=old.name;bpy.data.objects.remove(old,do_unlink=True)
 arm=source_copy('TopBody',oldname,gender,rig);arm.data.materials.clear();arm.data.materials.append(skin)
 bm=bmesh.new();bm.from_mesh(arm.data)
 for s in [-1,1]:bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.000001,plane_co=(s*.402,0,0),plane_no=(s,0,0))
 covered=[v for v in bm.verts if abs(v.co.x)<.40199 and not (abs(v.co.x)<.105 and v.co.z>1.69)]
 bmesh.ops.delete(bm,geom=covered,context='VERTS');bm.to_mesh(arm.data);bm.free()
 for p in arm.data.polygons:p.material_index=0
 tshirt_v3(name,gender,rig);trousers_v3(name,gender,rig)
for o in imported_all:bpy.data.objects.remove(o,do_unlink=True)
bpy.context.view_layer.update();bpy.ops.wm.save_as_mainfile(filepath=OUT+'/FilmCrew_Outfits_v3.blend')
print('V3_FITTED_BASE_READY')
