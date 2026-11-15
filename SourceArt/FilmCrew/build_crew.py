import bpy, bmesh, math, os, json, shutil
from mathutils import Vector, Matrix, Quaternion
from math import sin,cos,pi
OUT=os.path.dirname(os.path.abspath(__file__))
SRC=r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes'
bpy.ops.wm.read_factory_settings(use_empty=True)
def mat(name,color):
 m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'); p.inputs['Base Color'].default_value=(*color,1); p.inputs['Roughness'].default_value=.82
 return m
black=mat('Crew_Cotton_Black',(.003,.004,.006)); capmat=mat('Cap_Charcoal',(.008,.011,.016)); rubber=mat('Headset_Rubber',(.004,.005,.006)); trim=mat('Equipment_Graphite',(.035,.045,.055)); white=mat('Megaphone_Ivory',(.72,.76,.72)); red=mat('Equipment_Red',(.35,.035,.025))
for m in [black,capmat,rubber]:
 next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED').inputs['Specular IOR Level'].default_value=.12
os.makedirs(OUT+'/Textures',exist_ok=True)
texpath=OUT+'/Textures/Hero_Body.png'
shutil.copy2(SRC+'/Materials_Shaders_Textures/BodyPreColors/medievalTexture_bodyColor1.png',texpath)
skin=mat('Original_Hero_Body_Atlas',(.6,.35,.2))
nodes=skin.node_tree.nodes; p=next(n for n in nodes if n.type=='BSDF_PRINCIPLED'); t=nodes.new('ShaderNodeTexImage'); t.image=bpy.data.images.load(texpath); skin.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color']); skin.node_tree.links.new(t.outputs['Alpha'],p.inputs['Alpha'])
def assign(o,m):
 o.data.materials.clear();o.data.materials.append(m)
 for p in o.data.polygons:p.material_index=0
def mesh(name,verts,faces,m):
 d=bpy.data.meshes.new(name);d.from_pydata(verts,[],faces);d.update();o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);assign(o,m);return o
def rigid(o,rig,bone):
 mw=o.matrix_world.copy();o.parent=rig;o.matrix_world=mw
 g=o.vertex_groups.new(name=bone);g.add(list(range(len(o.data.vertices))),1,'REPLACE');mod=o.modifiers.new('OriginalHeroSkeleton','ARMATURE');mod.object=rig
 return o
def ellipsoid(name,loc,scale,m):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,location=loc);o=bpy.context.object;o.name=name;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);assign(o,m);return o
def box(name,loc,scale,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);assign(o,m);return o
def tube(name,pts,radius,m):
 verts=[];faces=[]
 for i,p in enumerate(pts):
  tangent=Vector(pts[min(i+1,len(pts)-1)])-Vector(pts[max(0,i-1)]);tangent.normalize();a=tangent.cross(Vector((0,0,1)))
  if a.length<.01:a=tangent.cross(Vector((0,1,0)))
  a.normalize();b=tangent.cross(a).normalized()
  for j in range(6):verts.append(Vector(p)+radius*(a*cos(j*pi/3)+b*sin(j*pi/3)))
 for i in range(len(pts)-1):
  for j in range(6):faces.append((i*6+j,i*6+(j+1)%6,(i+1)*6+(j+1)%6,(i+1)*6+j))
 return mesh(name,verts,faces,m)
variants=[('Crew_F01_Auburn_Bun','F',['F_hair_1b','F_hair_1b_Bun'],(.24,.065,.026)),('Crew_M01_Brown_Crop','M',['M_hair_3b'],(.085,.032,.014)),('Crew_F02_Blonde_Bob','F',['F_hair_6b'],(.63,.37,.105)),('Crew_M02_Silver_Swept','M',['M_hair_7b'],(.38,.42,.46)),('Crew_F03_Black_Ponytail','F',['F_hair_13b','F_hair_13b_Ponytail'],(.014,.009,.018))]
records=[];rigs=[]
for idx,(name,gender,hairs,hc) in enumerate(variants):
 before=set(bpy.data.objects)
 bpy.ops.import_scene.fbx(filepath=SRC+'/BaseCharacters/Base'+('Male' if gender=='M' else 'Female')+'.fbx')
 imported=set(bpy.data.objects)-before;rig=next(o for o in imported if o.type=='ARMATURE');rig.name=name+'_Rig';rig.animation_data_clear();rig.data.pose_position='REST'
 base=lambda n:next(o for o in imported if o.name.split('.')[0]==n)
 top=base(gender+'_TopBody');bpy.context.view_layer.update();transform=top.matrix_world.copy()
 keepnames=[gender+'_Head',gender+'_TopBody',gender+'_BottomBody',gender+'_eyes0',gender+'_eyebrows0',gender+'_mouth0']+hairs
 keep=[base(n) for n in keepnames]
 hairmat=mat(name+'_Hair',hc)
 for o in keep:
  o.hide_viewport=False;o.hide_render=False;o.hide_set(False);o.matrix_world=transform;assign(o,hairmat if '_hair_' in o.name else skin);o.animation_data_clear()
  o.name=name+'_'+o.name.split('.')[0]
 # Remove only unselected components from this newly imported copy.
 for o in list(imported):
  if o.type=='MESH' and o not in keep:bpy.data.objects.remove(o,do_unlink=True)
 bpy.context.view_layer.update()
 # Simple cloth topology avoids tracing the source's anatomical contours.
 vv=[];ff=[];N=16
 for z,rx,ry in [(1.08,.245,.165),(1.30,.25,.185),(1.49,.275,.215),(1.70,.30,.18),(1.735,.10,.095)]:
  for j in range(N):
   a=2*pi*j/N;vv.append((rx*cos(a),ry*sin(a),z))
 for k in range(4):
  for j in range(N):ff.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
 # Short sleeves overlap the shoulder panel and remain separate cloth islands.
 for side in [-1,1]:
  start=len(vv)
  for x,z,r in [(.08,1.635,.12),(.335,1.62,.13),(.465,1.60,.115)]:
   for j in range(12):
    a=2*pi*j/12;vv.append((side*x,r*cos(a),z+r*sin(a)))
  for k in range(2):
   for j in range(12):ff.append((start+k*12+j,start+k*12+(j+1)%12,start+(k+1)*12+(j+1)%12,start+(k+1)*12+j))
  ff.append(tuple(start+j for j in range(12)))
 shirt=mesh(name+'_TShirt',vv,ff,black)
 bm=bmesh.new();bm.from_mesh(shirt.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(shirt.data);bm.free()
 from mathutils.kdtree import KDTree
 tree=KDTree(len(top.data.vertices))
 for v in top.data.vertices:tree.insert(transform@v.co,v.index)
 tree.balance()
 groups={g.index:shirt.vertex_groups.new(name=g.name) for g in top.vertex_groups}
 for v in shirt.data.vertices:
  neighbors=tree.find_n(v.co,4);weights={};total=0
  for co,vi,d in neighbors:
   influence=1/max(d,.005)**2;total+=influence
   for g in top.data.vertices[vi].groups:weights[g.group]=weights.get(g.group,0)+g.weight*influence
  for gi,w in weights.items():groups[gi].add([v.index],w/total,'REPLACE')
 shirt.parent=rig;shirt.matrix_world=Matrix.Identity(4);mod=shirt.modifiers.new('OriginalHeroSkeleton','ARMATURE');mod.object=rig
 # Cull only skin concealed by the shirt to prevent shoulder poke-through.
 bm=bmesh.new();bm.from_mesh(top.data);covered=[]
 for f in bm.faces:
  c=transform@f.calc_center_median()
  if abs(c.x)<.43 and not (abs(c.x)<.105 and c.z>1.69):covered.append(f)
 bmesh.ops.delete(bm,geom=covered,context='FACES');bm.to_mesh(top.data);bm.free()
 sol=shirt.modifiers.new('Fabric_Edge_Thickness','SOLIDIFY');sol.thickness=.004
 # Trousers and shoes reuse the exact weighted legs, slightly relaxed for fabric.
 bottom=next(o for o in keep if 'BottomBody' in o.name);assign(bottom,black);bottom.data.materials.append(rubber)
 for p in bottom.data.polygons:
  z=sum(bottom.data.vertices[v].co.y for v in p.vertices)/len(p.vertices)
  if z<15:p.material_index=1
 for v in bottom.data.vertices:
  if v.co.y>15:v.co+=v.normal*1.5
 # Baseball cap: open crown and curved projecting bill, in the original head space.
 vv=[];ff=[];N=16
 for k in range(5):
  theta=(pi/2)*(k/4);r=cos(theta)
  for j in range(N):
   a=2*pi*j/N;vv.append((.145*r*cos(a),.028+.16*r*sin(a),1.945+.132*sin(theta)))
 for k in range(4):
  for j in range(N):ff.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
 cap=mesh(name+'_DarkCap',vv,ff,capmat);rigid(cap,rig,'head_joint')
 vv=[]
 for row in range(2):
  for j in range(9):
   a=-pi/2+pi*j/8;x=.145*sin(a);y=-.065-(.095 if row==0 else .225)*cos(a);z=1.947-.025*cos(a)*row;vv.append((x,y,z))
 bill=mesh(name+'_CapBill',vv,[(j,j+1,10+j,9+j) for j in range(8)],capmat);rigid(bill,rig,'head_joint');so=bill.modifiers.new('BillThickness','SOLIDIFY');so.thickness=.008
 rigid(ellipsoid(name+'_CapButton',(0,.028,2.079),(.016,.016,.009),trim),rig,'head_joint')
 # Full headset, ear cushions, band, and a microphone boom by the mouth.
 for s in [-1,1]:
  rigid(ellipsoid(name+'_EarCushion',(.143*s,.025,1.855),(.031,.055,.072),rubber),rig,'head_joint')
  rigid(ellipsoid(name+'_Earcup',(.169*s,.025,1.855),(.022,.047,.061),trim),rig,'head_joint')
 pts=[(.172*cos(a),.025,1.865+.226*sin(a)) for a in [pi*j/12 for j in range(13)]]
 rigid(tube(name+'_HeadsetBand',pts,.012,rubber),rig,'head_joint')
 rigid(tube(name+'_MicBoom',[(-.18,.01,1.83),(-.19,-.08,1.8),(-.12,-.16,1.79),(-.045,-.165,1.79)],.006,trim),rig,'head_joint')
 rigid(ellipsoid(name+'_MicFoam',(-.045,-.165,1.79),(.027,.013,.013),rubber),rig,'head_joint')
 # A hollow hip-slung megaphone, with handle and a short carry strap.
 center=Vector((.34,-.04,1.04));axis=Vector((.25,-.8,-.55)).normalized();u=axis.cross(Vector((0,0,1))).normalized();v=axis.cross(u);vv=[];ff=[]
 for dist,r in [(-.11,.035),(.13,.115),(.125,.101),(-.07,.024)]:
  for j in range(12):vv.append(center+axis*dist+r*(u*cos(2*pi*j/12)+v*sin(2*pi*j/12)))
 for k in range(3):
  for j in range(12):ff.append((k*12+j,k*12+(j+1)%12,(k+1)*12+(j+1)%12,(k+1)*12+j))
 horn=mesh(name+'_Megaphone',vv,ff,white);horn.data.materials.append(trim)
 for p in horn.data.polygons:
  if p.index>=12:p.material_index=1
 rigid(horn,rig,'pelvis_joint')
 rigid(box(name+'_MegaphoneHandle',(.32,.018,.95),(.035,.045,.12),rubber),rig,'pelvis_joint')
 rigid(tube(name+'_CarryLoop',[(.25,.025,1.23),(.36,.03,1.22),(.39,.01,1.11),(.34,-.01,1.07)],.008,rubber),rig,'pelvis_joint')
 # Collect exactly this variant for portable game-engine exports.
 objects=[o for o in bpy.data.objects if o not in before]
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:
  if o.type in {'MESH','ARMATURE'}:o.select_set(True)
 bpy.context.view_layer.objects.active=rig
 bpy.ops.export_scene.fbx(filepath=OUT+'/'+name+'.fbx',use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,path_mode='COPY',embed_textures=True)
 bpy.ops.export_scene.gltf(filepath=OUT+'/'+name+'.glb',use_selection=True,export_animations=False)
 records.append({'name':name,'source':'Base'+('Male' if gender=='M' else 'Female')+'.fbx','hair_meshes':hairs,'bones':len(rig.data.bones),'meshes':sum(o.type=='MESH' for o in objects)})
 rigs.append(rig)
 # Pose for the overview only. Exports retain the source rest pose.
 rig.data.pose_position='POSE'
 for side,angle in [('L',65),('R',-65)]:
  pb=rig.pose.bones[side+'_shoulder_joint'];axislocal=pb.bone.matrix_local.to_quaternion().inverted() @ (rig.matrix_world.to_quaternion().inverted() @ Vector((0,1,0)))
  pb.rotation_mode='QUATERNION';pb.rotation_quaternion=Quaternion(axislocal,math.radians(angle))
 # Moving the imported root also carries its meshes and original rig.
 root=rig
 while root.parent:root=root.parent
 root.location.x=(idx-2)*1.15
 bpy.context.view_layer.update()
json.dump(records,open(OUT+'/manifest.json','w'),indent=2)
floor=mat('Studio_Ground',(.15,.19,.22));bpy.ops.mesh.primitive_plane_add(size=200);assign(bpy.context.object,floor);bpy.context.object.name='Preview_Ground'
def track(o,pt):o.rotation_euler=(Vector(pt)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(3.5,-11,4.4));cam=bpy.context.object;track(cam,(0,0,1.05));cam.data.type='ORTHO';cam.data.ortho_scale=6.7;bpy.context.scene.camera=cam
for loc,power,size in [((1,-5,7),1800,5),((-5,-2,3.5),1300,4),((2,4,6),2200,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;track(o,(0,0,1))
s=bpy.context.scene;s.world=bpy.data.worlds.new('CrewStudio');s.world.color=(.3,.3,.3)
try:s.render.engine='CYCLES'
except TypeError:pass
s.cycles.samples=32;s.render.resolution_x=1800;s.render.resolution_y=850;s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG';s.render.filepath=OUT+'/FilmCrew_Lineup.png'
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/FilmCrew_5Variants.blend');bpy.ops.render.render(write_still=True)
print('CREW_BUILD_COMPLETE',records)
