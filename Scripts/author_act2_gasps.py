import bpy,math
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
ROOT=Path(__file__).resolve().parents[1];exec((ROOT/'Scripts/author_act1_poses.py').read_text().split('manifest=[]')[0],globals());OUT=ROOT/'SourceArt/Act2'
for role,file in [('Romeo','SwordsmanAct1'),('Juliet','ArcherAct1')]:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceArt/Polysplit'/(file+'.fbx')),use_anim=False);arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');scene=bpy.context.scene;scene.render.fps=30;scene.frame_start=1;scene.frame_end=91
 arm.animation_data_clear();arm.animation_data_create();arm.animation_data.action=bpy.data.actions.new(role+'_Gasp')
 for f in range(1,92):
  t=(f-1)/30;w=smooth(t/.35);scene.frame_set(f);pose(arm,'PromiseLook',0,role=='Juliet');rotate_world(arm,'chest_joint',(1,0,0),-8*w);rotate_world(arm,'head_joint',(1,0,0),-9*w)
  arm_ik(arm,'L',Vector((27,108,6)).lerp(Vector((12,165,24)),w),(45,138,12))
  if role=='Juliet':arm_ik(arm,'R',Vector((-27,108,6)).lerp(Vector((-12,159,27)),w),(-44,138,12))
  for b in arm.pose.bones:
   if 'mouth' in b.name.lower():b.scale.y=1+1.1*w
   b.rotation_mode='QUATERNION'
   for prop in ['location','rotation_quaternion','scale']:b.keyframe_insert(prop,frame=f,group=b.name)
 bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);bpy.context.view_layer.objects.active=arm
 bpy.ops.export_scene.fbx(filepath=str(OUT/('A_'+role+'_Gasp.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,axis_forward='-Z',axis_up='Y')
print('GASP_ANIMATIONS_AUTHORED')
