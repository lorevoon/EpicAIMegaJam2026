"""Reproducible support asset pipeline; does not touch maps or existing hero retargeters.
1. Blender --background --factory-startup --python Scripts/import_support_assets.py -- --prepare-orc
2. UnrealEditor-Cmd project.uproject -run=pythonscript -script=<absolute script> -EnablePlugins=PythonScriptPlugin,IKRig
Run phase 2 only with the editor coordinated/closed; assets are skipped if present.
"""
from pathlib import Path
import json
import sys
ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / 'SourceArt/support_assets_manifest.json').read_text())
ORC_FBX = ROOT / 'SourceArt/Polysplit/OrcArmored_Rigged.fbx'


def prepare_orc():
    import bpy
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=MANIFEST['armored_orc']['source'])
    keep = set(MANIFEST['armored_orc']['retained_meshes'])
    meshes = {o.name: o for o in bpy.context.scene.objects if o.type == 'MESH'}
    assert keep <= meshes.keys(), (keep - meshes.keys())
    for name, obj in meshes.items():
        if name not in keep:
            bpy.data.objects.remove(obj, do_unlink=True)
    rigs = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
    assert len(rigs) == 1
    for obj in bpy.context.scene.objects:
        obj.select_set(obj.type in {'MESH', 'ARMATURE', 'EMPTY'})
    bpy.context.view_layer.objects.active = rigs[0]
    ORC_FBX.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.fbx(filepath=str(ORC_FBX), use_selection=True,
        object_types={'ARMATURE', 'MESH', 'EMPTY'}, add_leaf_bones=False,
        bake_anim=False, use_mesh_modifiers=True, axis_forward='-Z', axis_up='Y')
    report = {'retained_meshes': sorted(keep), 'bone_count': len(rigs[0].data.bones),
              'vertices': sum(len(o.data.vertices) for o in bpy.context.scene.objects if o.type == 'MESH'),
              'output': str(ORC_FBX)}
    (ROOT / 'SourceArt/Polysplit/OrcArmored_Rigged.report.json').write_text(json.dumps(report, indent=2))
    print('SUPPORT_ORC_PREPARED', report)


def run_unreal():
    import unreal as u
    eal = u.EditorAssetLibrary
    at = u.AssetToolsHelpers.get_asset_tools()
    base = '/Game/DirectorsCut'
    rig_dir = base + '/Mixamo/SupportRetarget'
    mix = u.load_asset(base + '/Mixamo/SK_XBot')
    assert mix, 'Existing Mixamo XBot required'
    src_rig = u.load_asset(base + '/Mixamo/Retarget/IK_Mixamo')
    assert src_rig, 'Existing source IK rig required'
    u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')

    def import_fbx(source, folder, name, skeletal_mesh=False):
        path = folder + '/' + name
        if eal.does_asset_exist(path):
            return u.load_asset(path)
        task = u.AssetImportTask()
        task.filename = str(source)
        task.destination_path = folder
        task.destination_name = name
        task.automated = True
        task.replace_existing = False
        task.save = True
        ui = u.FbxImportUI()
        ui.import_mesh = skeletal_mesh
        ui.import_as_skeletal = True
        ui.import_animations = not skeletal_mesh
        ui.import_materials = False
        ui.import_textures = False
        ui.create_physics_asset = skeletal_mesh
        ui.mesh_type_to_import = u.FBXImportType.FBXIT_SKELETAL_MESH if skeletal_mesh else u.FBXImportType.FBXIT_ANIMATION
        if not skeletal_mesh:
            ui.skeleton = mix.skeleton
            ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        task.options = ui
        at.import_asset_tasks([task])
        for p in task.imported_object_paths:
            obj = u.load_asset(p)
            if isinstance(obj, u.SkeletalMesh if skeletal_mesh else u.AnimSequence):
                if obj.get_path_name().split('.')[0] != path:
                    if not eal.rename_asset(obj.get_path_name(), path):
                        raise RuntimeError('Failed to normalize import name: ' + p)
                return u.load_asset(path)
        raise RuntimeError('No requested asset imported from ' + str(source))

    assert ORC_FBX.is_file(), 'Run Blender --prepare-orc phase first'
    armored = import_fbx(ORC_FBX, base + '/Characters/OrcArmored', 'SK_OrcArmored', True)
    slots=armored.get_editor_property('materials')
    for index, slot in enumerate(slots):
        suffix = 'Body' if 'Body' in str(slot.material_slot_name) else 'Objects'
        mat = u.load_asset(base + '/Materials/MI_Polysplit_Orc_' + suffix)
        assert mat
        slot.material_interface=mat
        slots[index]=slot
    armored.set_editor_property('materials',slots)
    eal.save_loaded_asset(armored)

    clips = {}
    for entry in MANIFEST['clips']:
        clips[entry['name']] = import_fbx(entry['source'], base + '/Mixamo/SupportClips', entry['name'])

    chains = {
        'Spine': ('waist_joint','chest_joint'), 'Neck': ('neck_joint','neck_joint'), 'Head': ('head_joint','head_joint'),
        'LeftClavicle': ('L_clavicle_joint','L_clavicle_joint'), 'RightClavicle': ('R_clavicle_joint','R_clavicle_joint'),
        'LeftArm': ('L_shoulder_joint','L_wrist_joint'), 'RightArm': ('R_shoulder_joint','R_wrist_joint'),
        'LeftLeg': ('L_thigh_joint','L_ball_joint'), 'RightLeg': ('R_thigh_joint','R_ball_joint')}
    for side, prefix in [('Left','L'),('Right','R')]:
        for label, finger in [('Thumb','thumb'),('Index','index'),('Middle','mid'),('Ring','ring'),('Pinky','pinky')]:
            chains[side+label] = (prefix+'_'+finger+'Finger_joint1',prefix+'_'+finger+'Finger_joint3')
    report = {'armored_mesh': armored.get_path_name(), 'targets': {}}
    for target in ['Archer','Swordsman','Goblin','Orc','OrcArmored']:
        mesh = u.load_asset(base+'/Characters/'+target+'/SK_'+target)
        assert mesh, target
        rig = u.load_asset(rig_dir+'/IK_Support_'+target)
        if not rig:
            rig = at.create_asset('IK_Support_'+target,rig_dir,u.IKRigDefinition,u.IKRigDefinitionFactory())
            c = u.IKRigController.get_controller(rig)
            c.set_skeletal_mesh(mesh)
            c.set_retarget_root('pelvis_joint')
            for name,(start,end) in chains.items():
                c.add_retarget_chain(name,start,end,'')
            eal.save_loaded_asset(rig)
        rtg = u.load_asset(rig_dir+'/RTG_Support_'+target)
        if not rtg:
            rtg = at.create_asset('RTG_Support_'+target,rig_dir,u.IKRetargeter,u.IKRetargetFactory())
            rc = u.IKRetargeterController.get_controller(rtg)
            rc.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,src_rig)
            rc.set_ik_rig(u.RetargetSourceOrTarget.TARGET,rig)
            rc.add_default_ops()
            rc.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.SOURCE,src_rig)
            rc.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.TARGET,rig)
            rc.auto_map_chains(u.AutoMapChainType.EXACT,True)
            eal.save_loaded_asset(rtg)
        dest = base+'/Animations/'+target
        selected = [e for e in MANIFEST['clips'] if target in e['targets']]
        pending = [e for e in selected if not eal.does_asset_exist(dest+'/'+e['name'].replace('A_Support_','A_'+target+'_Support_',1))]
        if pending:
            data = [u.AssetRegistryHelpers.create_asset_data(clips[e['name']]) for e in pending]
            # This UE 5.8 API was successfully exercised in the recovered pipeline.
            result = u.IKRetargetBatchOperation.duplicate_and_retarget(data,mix,mesh,rtg,'A_Support_','A_'+target+'_Support_','','',dest,False,False,True)
            if len(result) != len(pending):
                raise RuntimeError('Retarget result count mismatch: '+target)
            for asset_data in result:
                path = str(asset_data.package_name)
                final = dest+'/'+path.split('/')[-1]
                if path != final:
                    assert not eal.does_asset_exist(final)
                    assert eal.rename_asset(path,final)
                eal.save_asset(final)
        paths = [dest+'/'+e['name'].replace('A_Support_','A_'+target+'_Support_',1) for e in selected]
        for path in paths:
            anim = u.load_asset(path)
            assert isinstance(anim,u.AnimSequence), path
            assert anim.get_editor_property('skeleton') == mesh.skeleton, 'Skeleton mismatch: '+path
            assert anim.get_play_length() > 0, 'Empty animation: '+path
        report['targets'][target] = paths
    (ROOT/'SourceArt/support_assets_import_report.json').write_text(json.dumps(report,indent=2))
    u.log('SUPPORT_ASSETS_COMPLETE '+json.dumps({k:len(v) for k,v in report['targets'].items()}))

if __name__ == '__main__':
    if '--prepare-orc' in sys.argv:
        prepare_orc()
    else:
        run_unreal()
