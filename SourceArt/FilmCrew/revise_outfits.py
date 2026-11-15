"""Run in the live Blender through MCP. Thin connected garments and work sneakers."""
import bpy,bmesh,math,os,json
from mathutils import Vector,Matrix
from mathutils.kdtree import KDTree
from mathutils.bvhtree import BVHTree
from math import sin,cos,pi,exp
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
SRC=r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes'

def material(name,col,rough=.85):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*col,1)
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=rough;p.inputs['Specular IOR Level'].default_value=.18
 return m
cotton=material('Crew_V2_Soft_Black_Cotton',(.009,.012,.017),.93)
twill=material('Crew_V2_Black_Twill',(.014,.018,.024),.88)
seam=material('Crew_V2_Stitching',(.028,.035,.044),.9)
uppermat=material('Crew_V2_Sneaker_Upper',(.009,.012,.016),.78)
solemat=material('Crew_V2_Rubber_Sole',(.025,.03,.036),.9)
lacemat=material('Crew_V2_Laces',(.065,.072,.08),.92)

def make(name,vs,fs,mat):
 me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);me.materials.append(mat)
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
 for p in me.polygons:p.use_smooth=True
 return ob
def activate(o):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
def subdiv(o,n=1):
 activate(o);m=o.modifiers.new('Soft cloth surface','SUBSURF');m.levels=n;bpy.ops.object.modifier_apply(modifier=m.name)
def attach(o,rig,weights=None,bone=None):
 o.parent=rig;o.matrix_world=Matrix.Translation(rig.matrix_world.translation)
 if bone:
  g=o.vertex_groups.new(name=bone);g.add(list(range(len(o.data.vertices))),1,'REPLACE')
 else:
  tree,data=weights;groups={}
  for v in o.data.vertices:
   near=tree.find_n(v.co,4);acc={};total=0
   for co,vi,dist in near:
    w=1/max(dist,.008)**2;total+=w
    for gn,gw in data[vi]:acc[gn]=acc.get(gn,0)+gw*w
   norm=sum(acc.values())
   for gn,w in acc.items():
    if w/norm<.0001:continue
    if gn not in groups:groups[gn]=o.vertex_groups.new(name=gn)
    groups[gn].add([v.index],w/norm,'REPLACE')
 m=o.modifiers.new('Original Hero Rig','ARMATURE');m.object=rig
 return o
def thicken(o,t=.0012):
 m=o.modifiers.new('Fabric thickness mm','SOLIDIFY');m.thickness=t;m.offset=0
def tube(name,pts,r,mat,closed=False):
 vs=[];fs=[];n=len(pts)
 for i,p in enumerate(pts):
  tangent=Vector(pts[(i+1)%n] if closed else pts[min(i+1,n-1)])-Vector(pts[(i-1)%n] if closed else pts[max(0,i-1)])
  tangent.normalize();a=tangent.cross(Vector((0,0,1)))
  if a.length<.01:a=tangent.cross(Vector((0,1,0)))
  a.normalize();b=tangent.cross(a)
  for j in range(6):vs.append(Vector(p)+r*(cos(j*pi/3)*a+sin(j*pi/3)*b))
 for i in range(n if closed else n-1):
  for j in range(6):fs.append((i*6+j,i*6+(j+1)%6,((i+1)%n)*6+(j+1)%6,((i+1)%n)*6+j))
 return make(name,vs,fs,mat)

# Read untouched donor geometry only for fitted shapes and skin-weight transfer.
donors={}
for gender in ['F','M']:
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=SRC+'/BaseCharacters/Base'+('Female' if gender=='F' else 'Male')+'.fbx');bpy.context.view_layer.update()
 imported=set(bpy.data.objects)-before;donors[gender]={}
 for part in ['TopBody','BottomBody']:
  o=next(o for o in imported if o.name.split('.')[0]==gender+'_'+part)
  coords=[o.matrix_world@v.co for v in o.data.vertices];tree=KDTree(len(coords));wg=[]
  for i,v in enumerate(o.data.vertices):tree.insert(coords[i],i);wg.append([(o.vertex_groups[g.group].name,g.weight) for g in v.groups])
  tree.balance();donors[gender][part]=(tree,wg)
  if part=='BottomBody':
   me=o.data.copy();me.transform(o.matrix_world);donors[gender]['lower_mesh']=me
 for o in imported:bpy.data.objects.remove(o,do_unlink=True)

def tshirt(name,gender,rig,variation):
 # Circular torso rings with real armholes, extruded into continuous sleeves.
 N=32;vs=[];fs=[]
 rings=[(1.105,.263,.19),(1.113,.263,.19),(1.15,.26,.19),(1.23,.25,.185),(1.32,.245,.185),(1.41,.25,.194),(1.49,.265,.207),(1.59,.272,.193),(1.69,.271,.145),(1.727,.105,.087),(1.733,.104,.087)]
 for k,(z,rx,ry) in enumerate(rings):
  for j in range(N):
   a=2*pi*j/N
   # restrained gravity folds; more gathering at side seams and the lower hem
   fold=(.006*sin(7*a+z*6+variation)+.003*sin(11*a-z*9))*(1 if k<7 else .15)
   hem=.004*sin(3*a+.3*variation)*max(0,1-(z-1.1)/.16)
   zz=z+hem
   if k in [9,10] and sin(a)<0:zz-=.025*(-sin(a))
   rear=.028*max(0,sin(a))*max(0,1-(z-1.1)/.28)
   vs.append(((rx+fold)*cos(a),(ry+fold)*sin(a)+rear,zz))
 def vid(k,j):return k*N+(j%N)
 sidefaces=set([(j%N) for c in [0,16] for j in range(c-3,c+3)])
 for k in range(len(rings)-1):
  for j in range(N):
   if k in [6,7] and j in sidefaces:continue
   fs.append((vid(k,j),vid(k,j+1),vid(k+1,j+1),vid(k+1,j)))
 for center,side in [(0,1),(16,-1)]:
  # Boundary loop around the removed side patch, shared with the torso.
  loop=[vid(6,j) for j in range(center-3,center+4)]
  loop += [vid(7,center+3),vid(8,center+3)]
  loop += [vid(8,j) for j in range(center+2,center-4,-1)]
  loop += [vid(7,center-3)]
  roots=[Vector(vs[i]) for i in loop];prev=loop
  for step,(x,scale,zoff) in enumerate([(.325,.96,-.012),(.4,.93,-.025),(.455,.94,-.032),(.462,.94,-.032)]):
   ring=[]
   for p in roots:
    q=Vector((side*x,p.y*scale,1.59+(p.z-1.59)*scale+zoff));ring.append(len(vs));vs.append(q)
   for j in range(len(loop)):fs.append((prev[j],prev[(j+1)%len(loop)],ring[(j+1)%len(loop)],ring[j]))
   prev=ring
 o=make(name+'_TShirt_Thin',vs,fs,cotton);subdiv(o,2)
 attach(o,rig,donors[gender]['TopBody']);thicken(o,.0012)
 # Narrow collar binding and visible hem stitching, not thick armor rims.
 collar=[(.105*cos(a),.088*sin(a),1.73-.025*max(0,-sin(a))) for a in [2*pi*j/64 for j in range(64)]]
 attach(tube(name+'_NeckBinding',collar,.003,cotton,True),rig,donors[gender]['TopBody'])
 # Smoothly varying skin weights prevent pinched cloth at the axilla.
 for g in list(o.vertex_groups):o.vertex_groups.remove(g)
 groups={n:o.vertex_groups.new(name=n) for n in ['waist_joint','chest_joint','L_shoulder_joint','R_shoulder_joint']}
 for v in o.data.vertices:
  x,y,z=v.co;t=max(0,min(1,(abs(x)-.18)/.19));h=max(0,min(1,(z-1.43)/.12));u=max(0,min(1,(abs(x)-.27)/.06));h=max(h*h*(3-2*h),u*u*(3-2*u));arm=t*t*(3-2*t)*h;sp=max(0,min(1,(z-1.24)/.25));sp=sp*sp*(3-2*sp)
  weights={'waist_joint':(1-arm)*(1-sp),'chest_joint':(1-arm)*sp,('L_shoulder_joint' if x>0 else 'R_shoulder_joint'):arm}
  for n,w in weights.items():
   if w>1e-6:groups[n].add([v.index],w,'REPLACE')
 return o

def pants(name,gender,rig):
 me=donors[gender]['lower_mesh'].copy();bm=bmesh.new();bm.from_mesh(me)
 # Keep only the connected legs/pelvis; discard fantasy ties and underwear trim.
 seen=set();parts=[]
 for v in bm.verts:
  if v in seen:continue
  todo=[v];seen.add(v);part=[]
  while todo:
   p=todo.pop();part.append(p)
   for e in p.link_edges:
    q=e.other_vert(p)
    if q not in seen:seen.add(q);todo.append(q)
  parts.append(part)
 largest=max(parts,key=len);remove=[v for part in parts if part is not largest for v in part]
 bmesh.ops.delete(bm,geom=remove,context='VERTS')
 bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,plane_co=(0,0,.155),plane_no=(0,0,-1),clear_outer=True,clear_inner=False)
 # Relax the silhouette from thigh to ankle before subdivision.
 for v in bm.verts:
  x,y,z=v.co
  if z<1.04:
   cx=(1 if x>0 else -1)*(.144+.023*max(0,min(1,(z-.2)/.8)))
   cy=.055-.09*exp(-((z-.55)/.23)**2)
   dx=x-cx;dy=y-cy
   v.co.x=cx+dx*1.15+(1 if dx>0 else -1)*.014
   v.co.y=cy+dy*1.12+(1 if dy>0 else -1)*.016
  else:v.co.x*=1.025;v.co.y*=1.04
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
 o=bpy.data.objects.new(name+'_Trousers',me);bpy.context.collection.objects.link(o);me.materials.clear();me.materials.append(twill)
 for p in me.polygons:p.material_index=0;p.use_smooth=True
 subdiv(o,2)
 # Small diagonal compression folds at knee and ankle; never uniform corrugation.
 for v in o.data.vertices:
  x,y,z=v.co;side=1 if x>0 else -1;cx=side*(.144+.023*max(0,min(1,(z-.2)/.8)));cy=.055-.09*exp(-((z-.55)/.23)**2)
  a=math.atan2(y-cy,x-cx)
  f=.0045*sin(47*z+2.5*a+side)*exp(-((z-.54)/.13)**2)+.006*sin(70*z+2*a)*exp(-((z-.235)/.085)**2)
  if z<.85:
   blend=max(0,min(1,(.85-z)/.15));rx=.072+.032*max(0,min(1,(z-.18)/.67));ry=.079+.035*max(0,min(1,(z-.18)/.67))
   v.co.x=(1-blend)*x+blend*(cx+rx*cos(a))+f*cos(a)
   v.co.y=(1-blend)*y+blend*(cy+ry*sin(a))+f*sin(a)
 attach(o,rig,donors[gender]['BottomBody']);thicken(o,.0015)
 return o

def sneaker(name,side,rig):
 # Rounded work sneaker: separate sole, shaped upper, padded collar, tongue, laces.
 cx=side*.144;N=32
 def outline(a,rx=.085,ry=.175,cy=-.04):
  # Wider round forefoot, narrower heel, no pointed medieval toe.
  width=rx*(1+.10*max(0,-sin(a))-.12*max(0,sin(a)))
  return cx+width*cos(a),cy+ry*sin(a)
 vs=[];fs=[]
 for z,scale in [(.008,.95),(.014,1.01),(.045,1.01),(.052,.96)]:
  for j in range(N):
   a=2*pi*j/N;x,y=outline(a);vs.append((cx+(x-cx)*scale,-.04+(y+.04)*scale,z))
 for k in range(3):
  for j in range(N):fs.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
 fs.append(tuple(reversed(range(N))));fs.append(tuple(3*N+j for j in range(N)))
 sole=make(name+'_Sole',vs,fs,solemat);subdiv(sole,1);attach(sole,rig,bone=('L' if side>0 else 'R')+'_ankle_joint')
 vs=[];fs=[]
 for k in range(6):
  for j in range(N):
   a=2*pi*j/N
   if k==0:x,y=outline(a,.079,.169);z=.047
   elif k==1:x,y=outline(a,.082,.167);z=.068+.012*(sin(a)+1)/2
   elif k==2:x,y=outline(a,.077,.15,-.031);z=.106+.025*(sin(a)+1)/2
   elif k==3:x,y=outline(a,.065,.112,-.005);z=.145+.017*(sin(a)+1)/2
   else:x,y=outline(a,.057,.067,.045);z=.181 if k==4 else .186
   vs.append((x,y,z))
 for k in range(5):
  for j in range(N):fs.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
 up=make(name+'_Upper',vs,fs,uppermat);subdiv(up,2);attach(up,rig,bone=('L' if side>0 else 'R')+'_ankle_joint');thicken(up,.002)
 surface=BVHTree.FromPolygons([v.co for v in up.data.vertices],[list(p.vertices) for p in up.data.polygons])
 def shoe_surface(x,y,lift):
  hit=surface.ray_cast(Vector((x,y,1)),Vector((0,0,-1)))[0]
  return (x,y,(hit.z if hit else .16)+lift)
 collar=[(*outline(a,.058,.068,.045),.184) for a in [2*pi*j/48 for j in range(48)]]
 attach(tube(name+'_PaddedCollar',collar,.0045,uppermat,True),rig,bone=('L' if side>0 else 'R')+'_ankle_joint')
 # A narrow tongue patch follows the sloping instep.
 pts=[shoe_surface(cx+x,-.11+j*.019,.002) for j in range(6) for x in [-.033,0,.033]]
 tongue=make(name+'_Tongue',pts,[(j*3+i,j*3+i+1,(j+1)*3+i+1,(j+1)*3+i) for j in range(5) for i in range(2)],uppermat);thicken(tongue,.002);attach(tongue,rig,bone=('L' if side>0 else 'R')+'_ankle_joint')
 for i in range(5):
  y=-.10+i*.017;z=.132+i*.009
  for s in [-1,1]:
   pts=[shoe_surface(cx+s*.034,y,.005),shoe_surface(cx,y+.008,.0065),shoe_surface(cx-s*.034,y+.016,.005)]
   attach(tube(name+'_Lace',pts,.0018,lacemat),rig,bone=('L' if side>0 else 'R')+'_ankle_joint')
 # Toe stitch and the sidewall groove distinguish the sole from the upper.
 pts=[(*outline(a,.079,.165),.067) for a in [pi+pi*j/30 for j in range(31)]]
 attach(tube(name+'_ToeStitch',pts,.0008,seam),rig,bone=('L' if side>0 else 'R')+'_ankle_joint')

rigs=sorted([o for o in bpy.data.objects if o.type=='ARMATURE' and o.name.startswith('Crew_')],key=lambda o:o.location.x)
for i,rig in enumerate(rigs):
 name=rig.name.removesuffix('_Rig');gender='F' if '_F0' in name else 'M'
 # Remove only the previously generated clothing that this revision supersedes.
 for o in list(bpy.data.objects):
  if o.name.startswith(name) and ('_TShirt' in o.name or '_BottomBody' in o.name):bpy.data.objects.remove(o,do_unlink=True)
 tshirt(name,gender,rig,i);pants(name,gender,rig)
 for side in [-1,1]:sneaker(name+('_ShoeL' if side>0 else '_ShoeR'),side,rig)
 print('REVISED',name)

bpy.context.view_layer.update()
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/FilmCrew_Outfits_v2.blend')
print('OUTFITS_V2_READY')
