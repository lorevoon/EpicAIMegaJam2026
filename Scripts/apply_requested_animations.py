"""Import user-supplied Mixamo clips, retarget, and update non-monster cast cues."""
import unreal as u
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
BASE = '/Game/DirectorsCut'
eal = u.EditorAssetLibrary
at = u.AssetToolsHelpers.get_asset_tools()
mix = u.load_asset(BASE + '/Mixamo/SK_XBot')
assert mix
u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')
report = {'clips': [], 'actors': [], 'verified': []}

def import_clip(filename, name):
    folder = BASE + '/Mixamo/Requested'
    existing = u.load_asset(folder + '/' + name) if eal.does_asset_exist(folder + '/' + name) else None
    if existing:
        return existing
    task = u.AssetImportTask()
    task.filename = str(ROOT / 'SourceArt/Mixamo/Requested' / filename)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.save = True
    ui = u.FbxImportUI()
    ui.import_mesh = False
    ui.import_as_skeletal = True
    ui.import_animations = True
    ui.import_materials = False
    ui.import_textures = False
    ui.skeleton = mix.skeleton
    ui.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION
    ui.automated_import_should_detect_type = False
    ui.anim_sequence_import_data.set_editor_property('animation_length', u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    task.options = ui
    at.import_asset_tasks([task])
    for path in task.imported_object_paths:
        obj = u.load_asset(path)
        if isinstance(obj, u.AnimSequence):
            if obj.get_name() != name:
                assert eal.rename_asset(path, folder + '/' + name)
            return u.load_asset(folder + '/' + name)
    raise RuntimeError('Animation import failed: ' + filename)

sources = {'BreathingIdle': import_clip('Breathing_Idle.fbx', 'A_Requested_BreathingIdle'),
           'XBotWalk': import_clip('X_Bot_Walk.fbx', 'A_Requested_XBotWalk')}
walk_valid = sources['XBotWalk'].get_play_length() >= 0.2
if not walk_valid:
    report['walk_blocked'] = 'Supplied X Bot.fbx contains only two frames (0.033 seconds); retained original female walk.'
clips = {}
for who in ['Swordsman', 'Archer']:
    mesh = u.load_asset(BASE + '/Characters/' + who + '/SK_' + who)
    rtg = u.load_asset(BASE + '/Mixamo/SupportRetarget/RTG_Support_' + who)
    assert mesh and rtg
    for kind, source in sources.items():
        if kind == 'XBotWalk' and (who == 'Swordsman' or not walk_valid):
            continue
        dest = BASE + '/Animations/' + who
        name = 'A_' + who + '_' + kind
        path = dest + '/' + name
        if not eal.does_asset_exist(path):
            results = u.IKRetargetBatchOperation.duplicate_and_retarget(
                [u.AssetRegistryHelpers.create_asset_data(source)], mix, mesh, rtg,
                'A_Requested_', 'A_' + who + '_', '', '', dest, False, False, True)
            assert len(results) == 1
            generated = str(results[0].package_name)
            if generated != path:
                assert eal.rename_asset(generated, path)
        clip = u.load_asset(path)
        assert clip.get_editor_property('skeleton') == mesh.skeleton
        assert clip.get_play_length() > 0
        clip.set_editor_property('force_root_lock', True)
        clip.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.ANIM_FIRST_FRAME)
        assert eal.save_loaded_asset(clip, False)
        clips[who, kind] = clip
        report['clips'].append({'path': path, 'seconds': clip.get_play_length()})

les = u.get_editor_subsystem(u.LevelEditorSubsystem)
aes = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = sorted(ROOT.joinpath('Content/DirectorsCut/Levels').glob('*.umap'))
for file in levels:
    level = BASE + '/Levels/' + file.stem
    assert les.load_level(level)
    changed = False
    for actor in aes.get_all_level_actors():
        if not isinstance(actor, u.DCPerformer):
            continue
        mesh = actor.get_editor_property('character_mesh')
        if not mesh:
            continue
        mesh_path = mesh.get_path_name()
        if any(monster in mesh_path.lower() for monster in ['orc', 'goblin', 'monster', 'zombie']):
            continue
        who = 'Archer' if 'Archer' in mesh_path else 'Swordsman'
        assert mesh.skeleton == clips[who, 'BreathingIdle'].get_editor_property('skeleton'), mesh_path
        cues = list(actor.get_editor_property('animation_cues'))
        edits = []
        for cue in cues:
            old = cue.animation
            if not old:
                continue
            name = old.get_name()
            if name == 'A_Archer_XBotWalk' and not walk_valid:
                original = u.load_asset(BASE + '/Animations/Archer/A_Archer_Unarmed_WalkForward')
                assert original
                cue.set_editor_property('animation', original)
                edits.append({'time': cue.start_time, 'old': name, 'new': original.get_name()})
                continue
            kind = 'BreathingIdle' if ('Idle' in name or 'PromiseLook' in name) else 'XBotWalk' if who == 'Archer' and 'Walk' in name else None
            if kind == 'XBotWalk' and not walk_valid:
                continue
            if kind and old != clips[who, kind]:
                cue.set_editor_property('animation', clips[who, kind])
                cue.set_editor_property('loop', True)
                if kind == 'BreathingIdle':
                    cue.set_editor_property('play_rate', 1.0)
                edits.append({'time': cue.start_time, 'old': name, 'new': clips[who, kind].get_name()})
        if edits:
            actor.set_editor_property('animation_cues', cues)
            actor.evaluate(0)
            changed = True
            report['actors'].append({'level': level, 'actor': actor.get_actor_label(), 'changes': edits})
    if changed:
        world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
        assert u.EditorLoadingAndSavingUtils.save_map(world, level)
    for actor in aes.get_all_level_actors():
        if isinstance(actor, u.DCPerformer):
            for cue in actor.get_editor_property('animation_cues'):
                if cue.animation:
                    mesh = actor.get_editor_property('character_mesh')
                    if mesh:
                        assert cue.animation.get_editor_property('skeleton') == mesh.skeleton
            report['verified'].append(level + ':' + actor.get_actor_label())
ROOT.joinpath('SourceArt/requested_animation_report.json').write_text(json.dumps(report, indent=2))
print('REQUESTED_ANIMATIONS_COMPLETE ' + json.dumps(report))
