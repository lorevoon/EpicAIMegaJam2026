"""Extract visible pack costumes; add a bone-weighted crew headset. Run in Blender."""
import bpy, json, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
src=Path(r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes\BasicHeroes')
out=root/'SourceArt/Polysplit';out.mkdir(exist_ok=True,parents=True)
report={}
for role,source in [('OldMan','M_Mage'),('Crew','M_Rogue')]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(src/(source+'.fbx')),use_anim=False)
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    for o in list(bpy.context.scene.objects):
        if o.type!='MESH': continue
        remove=o.hide_viewport or any(n in o.name for n in ['Dagger','Scabbard','Eyepatch','Headwear','Staff'])
        if remove: bpy.data.objects.remove(o,do_unlink=True)
        else:
            o.hide_viewport=False;o.hide_render=False
            if role=='OldMan' and ('hair' in o.name or 'facialHair' in o.name or 'eyebrows' in o.name):
                m=bpy.data.materials.get('GrayHair') or bpy.data.materials.new('GrayHair')
                m.diffuse_color=(.7,.68,.62,1);o.data.materials.clear();o.data.materials.append(m)
    if role=='Crew':
        mat=bpy.data.materials.new('HeadsetBlack');mat.diffuse_color=(.015,.018,.022,1)
        # Build ear cups and a segmented headband in original rig coordinates.
        head=arm.data.bones['head_joint'].head_local.copy()
        parts=[]
        for side in [-1,1]:
            bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6)
            o=bpy.context.object;o.name='Headset_Earcup';o.location=arm.matrix_world @ (head+Vector((side*13,4,0)))
            o.scale=(.035,.06,.08);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);parts.append(o)
        for i in range(9):
            angle=math.pi*i/8
            p=head+Vector((math.cos(angle)*14,5+math.sin(angle)*16,0))
            bpy.ops.mesh.primitive_uv_sphere_add(segments=8,ring_count=4)
            o=bpy.context.object;o.name='Headset_Band';o.location=arm.matrix_world@p;o.scale=(.025,.025,.025)
            bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);parts.append(o)
        for o in parts:
            o.data.materials.append(mat)
            group=o.vertex_groups.new(name='head_joint');group.add(list(range(len(o.data.vertices))),1,'REPLACE')
            mod=o.modifiers.new('Rig','ARMATURE');mod.object=arm
            o.parent=arm;o.matrix_parent_inverse=arm.matrix_world.inverted()
    arm.animation_data_clear()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.fbx(filepath=str(out/(role+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE','EMPTY'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
    report[role]={'source':str(src/(source+'.fbx')),'meshes':[o.name for o in bpy.context.scene.objects if o.type=='MESH'],'bones':len(arm.data.bones)}
(out/'cast_report.json').write_text(json.dumps(report,indent=2))
