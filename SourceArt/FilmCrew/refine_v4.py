import bpy,math,json
from math import exp
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE' and o.name.startswith('Crew_')]
before={r.name:{b.name:tuple(c for row in b.matrix_local for c in row) for b in r.data.bones} for r in rigs}
def mix(z,keys):
 for (a,x),(b,y) in zip(keys,keys[1:]):
  if z<=b:return x+(y-x)*max(0,min(1,(z-a)/(b-a)))
 return keys[-1][1]
for rig in rigs:
 name=rig.name.removesuffix('_Rig');cap=bpy.data.objects[name+'_DarkCap'];bill=bpy.data.objects[name+'_CapBill']
 # Seat the entire inner brim edge on the actual polygonal crown rim.
 rim=sorted([v for v in cap.data.vertices if abs(v.co.z-min(q.co.z for q in cap.data.vertices))<.0001 and v.co.y<=.029],key=lambda v:v.co.x)
 assert len(rim)==9
 for i,v in enumerate(rim):bill.data.vertices[i].co=v.co.copy();bill.data.vertices[i].co.y+=.001
 bill.data.vertices[9].co=bill.data.vertices[0].co.copy();bill.data.vertices[17].co=bill.data.vertices[8].co.copy()
 o=bpy.data.objects[name+'_Trousers_Straight']
 for v in o.data.vertices:
  x,y,z=v.co;s=1 if x>=0 else -1
  if z<.94:
   cx=s*mix(z,[(.15,.144),(.54,.1535),(.94,.163)])
   cy=mix(z,[(.15,.062),(.54,.005),(.94,.006)])
   sx=mix(z,[(.15,1.27),(.25,1.34),(.40,1.38),(.55,1.33),(.72,1.26),(.85,1.12),(.94,1)])
   sy=mix(z,[(.15,1.22),(.30,1.28),(.55,1.27),(.75,1.18),(.94,1)])
   dx=(x-cx)*sx;dy=(y-cy)*sy;angle=math.atan2(dy,dx);front=max(0,-math.sin(angle));back=max(0,math.sin(angle))
   fold=.006*exp(-((z-(.52+.022*math.cos(angle)))/.038)**2)*front
   fold-=.003*exp(-((z-.575)/.025)**2)*front
   fold+=.007*exp(-((z-(.205+.014*math.cos(angle+s*.4)))/.021)**2)
   fold-=.0035*exp(-((z-(.244+.012*math.cos(angle)))/.018)**2)
   fold+=.004*exp(-((z-.29)/.029)**2)*back
   v.co.x=cx+dx+math.cos(angle)*fold;v.co.y=cy+dy+math.sin(angle)*fold
   v.co.z=z-.009*max(0,min(1,(.30-z)/.15))
  elif z<1.12:
   v.co.x*=1+.035*exp(-((z-1.01)/.09)**2)
  
 o.data.update()
 rig.data.pose_position='POSE'
assert all(before[r.name][b.name]==tuple(c for row in b.matrix_local for c in row) for r in rigs for b in r.data.bones)
print('Updated five caps and trousers; bone matrices unchanged')
