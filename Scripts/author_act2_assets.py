"""Author stage-two mourning and clock-driven stylized fire. Run with Blender."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'SourceArt/Act2';OUT.mkdir(parents=True,exist_ok=True)
helpers=(ROOT/'Scripts/author_act1_poses.py').read_text().split('manifest=[]')[0]
exec(helpers,globals());OUT=ROOT/'SourceArt/Act2'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceArt/Polysplit/ArcherAct1.fbx'),use_anim=False)
arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');scene=bpy.context.scene;scene.render.fps=30
arm.animation_data_clear();arm.animation_data_create();arm.animation_data.action=bpy.data.actions.new('Juliet_Mourning')
scene.frame_start=1;scene.frame_end=271
for f in range(1,272):
 scene.frame_set(f);t=(f-1)/30;w=smooth(t/1.7)
 pose(arm,'PromiseLook',0,True)
 # Fold both legs underneath her, keeping the final kneel grounded.
 for side in ['L','R']:
  rotate_world(arm,side+'_thigh_joint',(1,0,0),-15*w)
  rotate_world(arm,side+'_knee_joint',(1,0,0),-115*w)
  rotate_world(arm,side+'_ankle_joint',(1,0,0),35*w)
 pelvis=arm.pose.bones['pelvis_joint'];m=pelvis.matrix.copy();m.translation.y-=55*w;pelvis.matrix=m;update()
 sob=(math.sin(t*8)+.35*math.sin(t*17))*w
 rotate_world(arm,'waist_joint',(1,0,0),(65+35*smooth((t-2)/2))*w+sob)
 rotate_world(arm,'chest_joint',(1,0,0),(20+10*smooth((t-2)/2))*w+sob*1.2)
 rotate_world(arm,'head_joint',(1,0,0),20*w+sob*1.4)
 for side,sgn in [('L',1),('R',-1)]:
  rest=arm.pose.bones[side+'_wrist_joint'].head.copy()
  arm_ik(arm,side,rest.lerp(Vector((sgn*17,30,66)),w),(sgn*42,65,30))
 arm.pose.bones['eyes_joint'].scale.y=1-.93*w
 arm.pose.bones['mouth_joint'].scale.y=1+.45*w*(.6+.4*math.sin(t*8))
 for bone in arm.pose.bones:
  bone.rotation_mode='QUATERNION'
  for prop in ['location','rotation_quaternion','scale']:bone.keyframe_insert(prop,frame=f,group=bone.name)
bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(OUT/'A_Juliet_Mourning.fbx'),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,axis_forward='-Z',axis_up='Y')
print('MOURNING_POINTS',{n:list(arm.pose.bones[n].head) for n in ['head_joint','L_knee_joint','L_ankle_joint','L_wrist_joint']})
# Fire is a skeletal effect so the same director clock drives ignition, replay and reverse scrubbing.
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.render.fps=30;scene.frame_start=1;scene.frame_end=1141
bpy.ops.object.armature_add();rig=bpy.context.object;rig.name='FireRig'
bpy.ops.object.mode_set(mode='EDIT');root=rig.data.edit_bones[0];root.name='root';root.head=(0,0,0);root.tail=(0,0,10)
for i in range(18):
 b=rig.data.edit_bones.new('fx_'+str(i));b.head=(0,0,0);b.tail=(0,0,10);b.parent=root
bpy.ops.object.mode_set(mode='OBJECT')
materials=[]
for name,color in [('FlameOrange',(1,.12,.006,1)),('FlameGold',(1,.55,.015,1)),('Smoke',(.12,.10,.09,1))]:
 m=bpy.data.materials.new(name);m.diffuse_color=color;materials.append(m)
for i in range(18):
 if i<12:
  x=(i%4-1.5)*24;y=(i//4-1)*20;h=95+(i*37%90);r=15+(i%3)*4
  verts=[]
  for z,rad,offset in [(0,r,0),(h*.45,r*.65,7),(h*.78,r*.3,-5),(h,0,12)]:
   for j in range(6):verts.append((x+offset+rad*math.cos(j*math.tau/6),y+rad*math.sin(j*math.tau/6),z))
  faces=[(k*6+j,k*6+(j+1)%6,(k+1)*6+(j+1)%6,(k+1)*6+j) for k in range(3) for j in range(6)]
  mesh=bpy.data.meshes.new('Flame');mesh.from_pydata(verts,[],faces);obj=bpy.data.objects.new('Flame',mesh);bpy.context.collection.objects.link(obj);obj.data.materials.append(materials[i%2])
 else:
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=35);obj=bpy.context.object;obj.data.materials.append(materials[2])
 group=obj.vertex_groups.new(name='fx_'+str(i));group.add(list(range(len(obj.data.vertices))),1,'REPLACE');mod=obj.modifiers.new('Rig','ARMATURE');mod.object=rig;obj.parent=rig
rig.animation_data_create();rig.animation_data.action=bpy.data.actions.new('Fire_38s')
for f in range(1,1142):
 t=(f-1)/30;scene.frame_set(f)
 for i in range(18):
  bone=rig.pose.bones['fx_'+str(i)];ignite=smooth((t-.6-i*.04)/1.8)
  if i<12:
   bone.scale=(ignite*(.9+.1*math.sin(t*8+i)),ignite,ignite*(.85+.2*math.sin(t*9+i*2)))
   bone.location=(math.sin(t*6+i)*7*ignite,0,0)
  else:
   phase=((t*.17+(i-12)/6)%1);sz=ignite*(.5+phase*2.5)
   bone.scale=(sz,sz,sz);bone.location=(math.sin(i*7)*40+phase*65,math.cos(i*5)*25,110+phase*420)
  for prop in ['location','scale']:bone.keyframe_insert(prop,frame=f,group=bone.name)
bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(OUT/'SK_Act2_Fire.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,axis_forward='-Z',axis_up='Y')
print('ACT2_AUTHORED')
