"""Import Reacting and retarget the opening terror onto the existing cast rigs.

Run in a separate Unreal Python commandlet with the Act 2 maps unloaded in the editor.
This script only imports assets; patch_act2_revision3_reactions.py applies them.
"""
import unreal as u
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = '/Game/DirectorsCut'
FOLDER = BASE + '/Act2/Revision3'
OUT = ROOT / 'Saved/Act2/Revision3'
OUT.mkdir(parents=True, exist_ok=True)
eal = u.EditorAssetLibrary
at = u.AssetToolsHelpers.get_asset_tools()
mix = u.load_asset(BASE + '/Mixamo/SK_XBot')
assert mix
u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')
report = {}
for filename, stem, roles in [
    ('Reacting.fbx', 'Reacting', ['Swordsman', 'Archer']),
    ('Terrified.fbx', 'Terrified', ['Swordsman']),
]:
    path = FOLDER + '/A_Mixamo_' + stem
    if not eal.does_asset_exist(path):
        task = u.AssetImportTask()
        task.filename = str(ROOT / 'SourceArt/Act2/Revision3' / filename)
        task.destination_path = FOLDER
        task.destination_name = 'A_Mixamo_' + stem
        task.automated = True
        task.save = True
        ui = u.FbxImportUI()
        ui.automated_import_should_detect_type = False
        ui.import_mesh = False
        ui.import_as_skeletal = True
        ui.import_animations = True
        ui.import_materials = False
        ui.import_textures = False
        ui.skeleton = mix.skeleton
        ui.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION
        ui.anim_sequence_import_data.set_editor_property(
            'animation_length', u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        task.options = ui
        at.import_asset_tasks([task])
        clips = [u.load_asset(p) for p in task.imported_object_paths
                 if isinstance(u.load_asset(p), u.AnimSequence)]
        assert clips, task.imported_object_paths
        if clips[0].get_path_name().split('.')[0] != path:
            assert eal.rename_asset(clips[0].get_path_name(), path)
    clip = u.load_asset(path)
    assert clip
    assert eal.save_loaded_asset(clip, False)
    for role in roles:
        target = u.load_asset(BASE + '/Characters/' + role + '/SK_' + role)
        out = FOLDER + '/A_' + role + '_' + stem
        if not eal.does_asset_exist(out):
            result = u.IKRetargetBatchOperation.duplicate_and_retarget(
                [u.AssetRegistryHelpers.create_asset_data(clip)], mix, target,
                u.load_asset(BASE + '/Mixamo/SupportRetarget/RTG_Support_' + role),
                'A_Mixamo_', 'A_' + role + '_', '', '', FOLDER, False, False, True)
            assert len(result) == 1
            generated = str(result[0].package_name)
            if generated != out:
                assert eal.rename_asset(generated, out)
        anim = u.load_asset(out)
        assert anim.get_editor_property('skeleton') == target.skeleton
        assert eal.save_loaded_asset(anim, False)
        report[out] = {'seconds': anim.get_play_length(), 'skeleton': target.skeleton.get_path_name()}
smith = u.load_asset(BASE + '/Characters/OldMan/SK_OldMan')
hero = u.load_asset(BASE + '/Characters/Swordsman/SK_Swordsman')
assert smith.skeleton == hero.skeleton, 'Smith must use the existing shared Swordsman rig'
(OUT / 'imports.json').write_text(json.dumps(report, indent=2))
print('ACT2_REVISION3_REACTIONS_IMPORTED', json.dumps(report))
