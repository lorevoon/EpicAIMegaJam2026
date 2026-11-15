import bpy,math
from mathutils import Matrix
from math import exp
OUT=r'C:\Unity Projects\EpicAIMegaJam2026\SourceArt\FilmCrew'
def interp(z,keys):
 if z<=keys[0][0]:return keys[0][1]
 for (a,x),(b,y) in zip(keys,keys[1:]):
  if z<=b:return x+(y-x)*(z-a)/(b-a)
 return keys[-1][1]
rigs=[o for o in bpy.data.objects if o.type=='ARMATURE' and o.name.startswith('Crew_')]
for rig in rigs:
 name=rig.name.removesuffix('_Rig');female='_F0' in name
 before=set(bpy.data.objects)
 with bpy.data.libraries.load(OUT+'/FilmCrew_Outfits_v2.blend',link=False) as (a,b):b.objects=[name+'_TShirt_Thin']
 o=b.objects[0];bpy.context.collection.objects.link(o)
 old=[x for x in before if x.name.startswith(name+'_TShirt')]
 for x in old:bpy.data.objects.remove(x,do_unlink=True)
 o.name=name+'_TShirt_Fitted';o.data=o.data.copy();o.parent=rig;o.matrix_world=Matrix.Translation(rig.matrix_world.translation)
 for m in o.modifiers:
  if m.type=='ARMATURE':m.object=rig
  elif m.type=='SOLIDIFY':m.thickness=.00065
 o.data.materials.clear();o.data.materials.append(bpy.data.materials['V3_Cotton_Black'])
 for v in o.data.vertices:
  x,y,z=v.co
  torso=max(0,min(1,(.33-abs(x))/.10))
  sx=interp(z,[(1.10,.83),(1.2,.86),(1.32,.78),(1.42,.82),(1.53,.88),(1.65,.91),(1.72,1)])
  sy=interp(z,[(1.10,.80),(1.2,.84),(1.32,.80),(1.42,.93 if female else .87),(1.53,.95 if female else .9),(1.65,.9),(1.72,1)])
  v.co.x=x*(1+(sx-1)*torso);v.co.y=y*(1+(sy-1)*torso)
  if z<1.30:v.co.z+=.045*max(0,1-(z-1.10)/.20)
  if abs(x)>.28:
   t=max(0,min(1,(abs(x)-.28)/.08));v.co.y*=1-.10*t;v.co.z=1.60+(v.co.z-1.60)*(1-.07*t)
  # Shallow, localized fabric creases at the waist and underarm.
  X,Y,Z=v.co
  if Y<-.02:
   ridge=.0025*exp(-((Z-(1.205+.14*X))/.014)**2)*exp(-((X-.12)/.08)**2)
   ridge+=.002*exp(-((Z-(1.255-.18*X))/.015)**2)*exp(-((X+.13)/.07)**2)
   v.co.y-=ridge
 # Library parent dependencies are not used; the audited live skeleton stays intact.
 for dep in set(bpy.data.objects)-before:
  if dep!=o and dep.users==0:bpy.data.objects.remove(dep)
print('TAILORED_5_SHIRTS')
