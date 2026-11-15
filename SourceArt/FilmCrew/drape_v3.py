import bpy,math
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
SRC=r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes'
def drape(name,frames=26):
 rig=bpy.data.objects[name+'_Rig'];shirt=bpy.data.objects[name+'_TShirt_Fitted'];gender='F' if '_F0' in name else 'M';s=bpy.context.scene
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=SRC+'/BaseCharacters/Base'+('Female' if gender=='F' else 'Male')+'.fbx');bpy.context.view_layer.update()
 imported=set(bpy.data.objects)-before;donor=next(o for o in imported if o.type=='ARMATURE')
 donor.animation_data_clear();donor.matrix_world=rig.matrix_world.copy()
 for p in donor.pose.bones:p.matrix_basis=rig.pose.bones[p.name].matrix_basis.copy()
 for o in imported:
  if o.type=='MESH':o.hide_viewport=False
 bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get();vs=[];fs=[]
 for part in ['TopBody','BottomBody']:
  ob=next(o for o in imported if o.name.split('.')[0]==gender+'_'+part);ev=ob.evaluated_get(dg);me=ev.to_mesh();start=len(vs)
  vs.extend(ev.matrix_world@v.co for v in me.vertices);fs.extend(tuple(start+i for i in p.vertices) for p in me.polygons);ev.to_mesh_clear()
 for o in imported:bpy.data.objects.remove(o,do_unlink=True)
 me=bpy.data.meshes.new('Temporary_Original_Body_Collider');me.from_pydata(vs,[],fs);me.update();collider=bpy.data.objects.new('Temporary_Original_Body_Collider',me);s.collection.objects.link(collider);collider.hide_render=True;collider.modifiers.new('Original body collision','COLLISION');collider.collision.thickness_outer=.0015;collider.collision.thickness_inner=.001
 # Evaluate the current shirt without its sub-millimetre thickness.
 solid=[m for m in shirt.modifiers if m.type=='SOLIDIFY']
 for m in solid:m.show_viewport=False
 bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get();ev=shirt.evaluated_get(dg);simmesh=bpy.data.meshes.new_from_object(ev);simmesh.transform(shirt.matrix_world)
 for m in solid:m.show_viewport=True
 assert len(simmesh.vertices)==len(shirt.data.vertices)
 sim=bpy.data.objects.new('Temporary_Cloth_Drape',simmesh);s.collection.objects.link(sim);sim.data.materials.clear();sim.data.materials.append(shirt.data.materials[0])
 surface=BVHTree.FromPolygons(vs,fs)
 for v in sim.data.vertices:
  hit,n,face,d=surface.find_nearest(v.co)
  if hit is not None and (v.co-hit).dot(n)<.003:v.co=hit+n*.003
 pin=sim.vertex_groups.new(name='NecklinePins')
 for v in shirt.data.vertices:
  if v.co.z>1.66 and abs(v.co.x)<.115:pin.add([v.index],1,'REPLACE')
 cloth=sim.modifiers.new('Cotton gravity drape','CLOTH');c=cloth.settings;c.quality=12;c.mass=.002;c.tension_stiffness=80;c.compression_stiffness=80;c.shear_stiffness=40;c.bending_stiffness=.6;c.air_damping=5;c.vertex_group_mass=pin.name;c.pin_stiffness=1
 cloth.collision_settings.use_collision=True;cloth.collision_settings.distance_min=.002;cloth.collision_settings.collision_quality=5;cloth.collision_settings.use_self_collision=True;cloth.collision_settings.self_distance_min=.002
 cloth.point_cache.frame_start=1;cloth.point_cache.frame_end=frames
 initial=[v.co.copy() for v in sim.data.vertices];s.frame_set(1)
 for frame in range(2,frames+1):s.frame_set(frame);bpy.context.view_layer.update();sim.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh_clear()
 dg=bpy.context.evaluated_depsgraph_get();ev=sim.evaluated_get(dg);result=ev.to_mesh();final=[v.co.copy() for v in result.vertices];ev.to_mesh_clear()
 disp=max((a-b).length for a,b in zip(initial,final));print('DRAPE_DISPLACEMENT',name,disp)
 assert disp<.5,'Unstable cloth simulation'
 # Invert the existing skinning transform; only garment vertex positions change.
 inv=shirt.matrix_world.inverted();trans={}
 for g in shirt.vertex_groups:
  if g.name in rig.pose.bones:
   p=rig.pose.bones[g.name];trans[g.index]=inv@rig.matrix_world@p.matrix@p.bone.matrix_local.inverted()@rig.matrix_world.inverted()@shirt.matrix_world
 for v,q in zip(shirt.data.vertices,final):
  M=Matrix(((0,0,0,0),)*4);total=0
  for g in v.groups:
   if g.group in trans:
    M+=trans[g.group]*g.weight;total+=g.weight
  if total>0:v.co=(M*(1/total)).inverted_safe()@(inv@q)
 bpy.data.objects.remove(sim,do_unlink=True);bpy.data.objects.remove(collider,do_unlink=True);s.frame_set(1);bpy.context.view_layer.update()
 return disp
