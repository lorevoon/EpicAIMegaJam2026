"""Blender 5: author original-rig Act 1 gestures; export animation-only FBX.

Run: blender --background --factory-startup --python Scripts/author_act1_poses.py
Coordinates below are Polysplit armature space: X lateral, Y up, Z forward, cm.
These are authored skeletal gestures, not motion-capture replacements. Paired contact
must be aligned in the level using the head/wrist positions in manifest.json.
"""
import bpy, math, json
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SourceArt' / 'Act1Poses'
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = Path('C:/Unity Projects/COMP 3329 POC/Assets/PolysplitGames/LowPolyMedievalFantasyHeroes/BasicHeroes')
FPS = 30

def update(): bpy.context.view_layer.update()
def smooth(v):
    v=max(0.,min(1.,v)); return v*v*(3-2*v)
def rotate_world(arm, name, axis, angle):
    bone=arm.pose.bones[name]; pos=bone.head.copy()
    bone.matrix=Matrix.Translation(pos) @ Quaternion(Vector(axis), math.radians(angle)).to_matrix().to_4x4() @ Matrix.Translation(-pos) @ bone.matrix
    update()
def aim(arm, name, child, target):
    bone=arm.pose.bones[name]; pos=bone.head.copy()
    direction=arm.pose.bones[child].head-pos
    desired=Vector(target)-pos
    if direction.length<.0001 or desired.length<.0001: return
    rotation=direction.rotation_difference(desired)
    bone.matrix=Matrix.Translation(pos) @ rotation.to_matrix().to_4x4() @ Matrix.Translation(-pos) @ bone.matrix
    update()
def arm_ik(arm, side, target, hint):
    names=[side+'_'+n+'_joint' for n in ('shoulder','elbow','wrist')]
    sh,el,wr=[arm.pose.bones[n].head.copy() for n in names]
    a=(el-sh).length; b=(wr-el).length
    delta=Vector(target)-sh; distance=min(delta.length,a+b-.05)
    axis=delta.normalized(); target=sh+axis*distance
    along=(a*a-b*b+distance*distance)/(2*distance)
    height=math.sqrt(max(0,a*a-along*along))
    plane=Vector(hint)-sh; plane=(plane-axis*plane.dot(axis)).normalized()
    elbow=sh+axis*along+plane*height
    aim(arm,names[0],names[1],elbow)
    aim(arm,names[1],names[2],target)

def pose(arm, kind, t, female=False):
    for b in arm.pose.bones:
        b.matrix_basis=Matrix.Identity(4)
    update()
    # Natural, relaxed arms, with a slight bend; no T-pose in any authored clip.
    for side,sgn in [('L',1),('R',-1)]:
        arm_ik(arm,side,(sgn*27,108,6),(sgn*42,137,-5))
    breathe=math.sin(t*math.pi*.7)*.5
    rotate_world(arm,'chest_joint',(1,0,0),breathe)
    if kind in ('CarryWalk','CarryHold','SetDown'):
        bend=0
        if kind=='SetDown': bend=26*smooth(t/.75)*(1-smooth((t-1.25)/.75))
        rotate_world(arm,'waist_joint',(1,0,0),bend)
        level=123-bend*.8
        for side,sgn in [('L',1),('R',-1)]:
            arm_ik(arm,side,(sgn*22,level,40+bend*.35),(sgn*42,133-bend*.4,8))
        if kind=='CarryWalk':
            swing=math.sin(t*math.tau)
            for side,sgn in [('L',1),('R',-1)]:
                rotate_world(arm,side+'_thigh_joint',(1,0,0),sgn*swing*17)
                rotate_world(arm,side+'_knee_joint',(1,0,0),-max(0,sgn*swing)*20)
                rotate_world(arm,side+'_ankle_joint',(1,0,0),-sgn*swing*5)
    elif kind=='ShoulderPat':
        reach=smooth(t/.6)*(1-smooth((t-2.5)/.5))
        rest=Vector((-27,108,6)); target=Vector((-25,159+math.sin(t*math.tau*2)*2,48))
        arm_ik(arm,'R',rest.lerp(target,reach),(-48,141,18))
        rotate_world(arm,'head_joint',(0,1,0),-12*reach)
    elif kind=='Wave':
        reach=smooth(t/.5)*(1-smooth((t-2.5)/.5))
        target=Vector((45+math.sin(t*math.tau*1.5)*7,191,19))
        arm_ik(arm,'L',Vector((27,108,6)).lerp(target,reach),(55,154,4))
        rotate_world(arm,'L_wrist_joint',(0,0,1),math.sin(t*math.tau*1.5)*12*reach)
    elif kind in ('PromiseLook','KissLean','ForeheadHold'):
        blend=smooth(t/2) if kind=='KissLean' else (1 if kind=='ForeheadHold' else 0)
        rotate_world(arm,'waist_joint',(1,0,0),(5 if female else 8)*blend)
        rotate_world(arm,'chest_joint',(1,0,0),(3 if female else 4)*blend)
        rotate_world(arm,'head_joint',(1,0,0),(-9 if female else 7)*blend)
        rotate_world(arm,'head_joint',(0,0,1),(4 if female else -4)*blend)
        for side,sgn in [('L',1),('R',-1)]:
            rest=Vector((sgn*27,108,6))
            target=Vector((sgn*(25 if female else 30),148 if female else 132,35))
            arm_ik(arm,side,rest.lerp(target,blend),(sgn*44,134,15))
        if kind=='PromiseLook':
            rotate_world(arm,'head_joint',(1,0,0),(-4 if female else 4)+math.sin(t*1.3)*1.5)
    update()

def render_preview(arm, kind, t, path):
    # Render the actual skinned costume meshes with studio clay for pose review.
    pose(arm,kind,t,'Archer' in path.name)
    scene=bpy.context.scene
    scene.render.engine='BLENDER_EEVEE'
    scene.render.resolution_x=600;scene.render.resolution_y=600;scene.render.resolution_percentage=100
    if not scene.world: scene.world=bpy.data.worlds.new('PoseReviewWorld')
    scene.world.color=(.16,.16,.16)
    camera_data=bpy.data.cameras.new('PoseReviewCamera');camera=bpy.data.objects.new('PoseReviewCamera',camera_data)
    scene.collection.objects.link(camera);scene.camera=camera
    center=arm.matrix_world @ Vector((0,105,0))
    camera.location=center+Vector((3.3,-5.5,2.4))
    camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    camera_data.type='ORTHO';camera_data.ortho_scale=2.8
    for loc,power,size in [((3,-4,6),900,5),((-3,-1,3),500,4)]:
        data=bpy.data.lights.new('PoseReviewLight','AREA');data.energy=power;data.shape='DISK';data.size=size
        obj=bpy.data.objects.new('PoseReviewLight',data);scene.collection.objects.link(obj);obj.location=loc
        obj.rotation_euler=(center-obj.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
    for obj in list(scene.objects):
        if obj.type in ('CAMERA','LIGHT'): bpy.data.objects.remove(obj,do_unlink=True)

manifest=[]
for character,source in [('Swordsman','M_Swordsman.fbx'),('Archer','F_Archer.fbx')]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(SOURCE/source), use_anim=False)
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    # Export the original armature only; no helper bones or altered rest matrices.
    scene=bpy.context.scene;scene.render.fps=FPS
    kinds={'PromiseLook':4,'KissLean':3,'ForeheadHold':4}
    if character=='Swordsman': kinds.update({'CarryWalk':1,'CarryHold':2,'SetDown':2,'ShoulderPat':3,'Wave':3})
    for kind,duration in kinds.items():
        arm.animation_data_clear()
        action=bpy.data.actions.new(f'A_{character}_{kind}');arm.animation_data_create();arm.animation_data.action=action
        scene.frame_start=1;scene.frame_end=1+round(duration*FPS)
        for frame in range(scene.frame_start,scene.frame_end+1):
            scene.frame_set(frame);pose(arm,kind,(frame-1)/FPS,character=='Archer')
            for bone in arm.pose.bones:
                bone.rotation_mode='QUATERNION'
                bone.keyframe_insert('rotation_quaternion',frame=frame,group=bone.name)
                bone.keyframe_insert('location',frame=frame,group=bone.name)
                bone.keyframe_insert('scale',frame=frame,group=bone.name)
        bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);bpy.context.view_layer.objects.active=arm
        filepath=OUT/f'A_{character}_{kind}.fbx'
        bpy.ops.export_scene.fbx(filepath=str(filepath),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,
            bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,
            axis_forward='-Z',axis_up='Y',primary_bone_axis='Y',secondary_bone_axis='X')
        sample=duration if kind=='KissLean' else duration*.5
        scene.frame_set(1+round(sample*FPS))
        points={name: list(arm.pose.bones[name].head) for name in ['head_joint','L_wrist_joint','R_wrist_joint']}
        manifest.append({'file':filepath.name,'character':character,'duration':duration,'sample_time':sample,
                         'contact_positions_armature_cm':points,'authored':True,'root':'rootSkeleton'})
    # Hide alternate customization geometry for review; use visible costume/base skin.
    prefix='M_' if character=='Swordsman' else 'F_'
    keep={prefix+'Head',prefix+'TopBody',prefix+'BottomBody',prefix+'eyes0',prefix+'mouth0',prefix+'eyebrows0',prefix+'hair_4'}
    for obj in scene.objects:
        if obj.type=='MESH':
            obj.hide_render=not(obj.name in keep or (character in obj.name and not any(w in obj.name for w in ['Sword','Scabbard','Bow','Arrow','Quiver'])))
    arm.animation_data_clear()
    render_preview(arm,'CarryHold' if character=='Swordsman' else 'ForeheadHold',1,OUT/f'{character}_pose_review.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/f'{character}_authored_poses.blend'))
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('AUTHORED',len(manifest),'CLIPS',OUT)
