import bpy

def interp(z,keys):
 for (a,x),(b,y) in zip(keys,keys[1:]):
  if z<=b:return x+(y-x)*max(0,min(1,(z-a)/(b-a)))
 return keys[-1][1]
for o in bpy.context.scene.objects:
 if '_Trousers_Straight' not in o.name:continue
 for v in o.data.vertices:
  x,y,z=v.co
  leg=max(0,min(1,(.96-z)/.16))
  cx=(1 if x>=0 else -1)*(.15+.006*max(0,(.9-z)/.75))
  hipscale=interp(z,[(.8,.81),(1.06,.81),(1.15,.96),(1.22,1)])
  v.co.x=(x*hipscale)*(1-leg)+(cx+(x-cx)*.78)*leg
  cy=.016+.04*max(0,min(1,(.4-z)/.255))
  v.co.y=cy+(y-cy)*interp(z,[(.14,.88),(.8,.86),(1.05,.89),(1.22,1)])
for r in bpy.context.scene.objects:
 if r.type=='ARMATURE':r.data.pose_position='POSE'
print('Narrowed hips and reduced trouser volume on five variants')
