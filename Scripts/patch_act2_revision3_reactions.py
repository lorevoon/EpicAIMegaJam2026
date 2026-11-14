"""Replace selected body reactions while retaining all later acting and blocking.

Fresh Revision3 baselines intentionally precede the separate AudioFacesV3 bake.
No waypoints, props, cameras, or sequence bindings are changed here.
"""
import unreal as u
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = '/Game/DirectorsCut'
FOLDER = BASE + '/Act2/Revision3'
LEVEL = BASE + '/Levels/L_DC_Act2_BrokenPromise'
FPS = 30
OUT = ROOT / 'Saved/Act2/Revision3'
OUT.mkdir(parents=True, exist_ok=True)
les = u.get_editor_subsystem(u.LevelEditorSubsystem)
aes = u.get_editor_subsystem(u.EditorActorSubsystem)
eal = u.EditorAssetLibrary
assert les.load_level(LEVEL)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast = {a.get_actor_label(): a for a in aes.get_all_level_actors() if isinstance(a, u.DCPerformer)}

def load(path):
    result = u.load_asset(path)
    assert result, path
    return result

def smooth(x):
    x = max(0., min(1., x))
    return x*x*(3.-2.*x)

def blend(a, b, weight):
    q = list(a.rotation.to_tuple())
    r = list(b.rotation.to_tuple())
    if sum(x*y for x, y in zip(q, r)) < 0:
        r = [-v for v in r]
    values = [x+(y-x)*weight for x, y in zip(q, r)]
    length = math.sqrt(sum(v*v for v in values))
    result = u.Transform()
    result.translation = a.translation+(b.translation-a.translation)*weight
    result.rotation = u.Quat(*[v/length for v in values])
    result.scale3d = a.scale3d+(b.scale3d-a.scale3d)*weight
    return result

specs = {
    'Romeo': (BASE+'/Act2/Revision2/A_V2_Romeo_Performance', 'Swordsman', [(10.,13.,'Reacting')]),
    'Juliet': (BASE+'/Act2/Revision2/A_V2_Juliet_Performance', 'Archer', [(10.,13.,'Reacting'),(17.,29.,'Reacting')]),
    'Mr. Smith': (BASE+'/Act2/A_Raid_MrSmith_Performance', 'Swordsman', [(0.,7.,'Terrified')]),
}
report = {}
for who, (source_path, role, windows) in specs.items():
    actor = cast[who]
    source = load(source_path)
    mesh = actor.get_editor_property('character_mesh')
    assert mesh.skeleton == source.get_editor_property('skeleton')
    component = actor.get_component_by_class(u.SkeletalMeshComponent)
    bones = [component.get_bone_name(i) for i in range(component.get_num_bones())]
    clips = {name: load(FOLDER+'/A_'+role+'_'+name) for _, _, name in windows}
    for clip in clips.values():
        assert clip.get_editor_property('skeleton') == mesh.skeleton
    first_roots = {name: u.AnimationLibrary.get_bone_poses_for_time(
        clip, [bones[0]], 3. if name == 'Terrified' else 0., False, mesh)[0]
        for name, clip in clips.items()}
    name = 'A_V3_'+who.replace(' ','').replace('.','')+'_Performance'
    path = FOLDER+'/'+name
    anim = u.load_asset(path) or eal.duplicate_asset(source_path, path)
    assert anim
    data = {str(b): [[], [], []] for b in bones}
    max_changed_translation = 0.
    for frame in range(1141):
        t = frame/FPS
        poses = u.AnimationLibrary.get_bone_poses_for_time(source, bones, t, False, mesh)
        active = next(((start, end, clipname) for start, end, clipname in windows if start <= t < end), None)
        if active:
            start, end, clipname = active
            clip = clips[clipname]
            duration = clip.get_play_length()
            local = t-start
            if clipname == 'Terrified':
                # Start at the visibly frightened portion; the 20 s source has ample length.
                sample_time = min(duration, 3.+local)
                replacement = u.AnimationLibrary.get_bone_poses_for_time(clip, bones, sample_time, False, mesh)
            elif end-start <= 3.1:
                # The supplied Reacting clip is 3.667 s: fit its complete reaction into 3 s.
                replacement = u.AnimationLibrary.get_bone_poses_for_time(
                    clip, bones, local*duration/(end-start), False, mesh)
            else:
                # Smooth overlapping cycles preserve ongoing concern through the combat.
                overlap = .3
                cycle = max(.5, duration-overlap)
                phase = local % cycle
                replacement = u.AnimationLibrary.get_bone_poses_for_time(clip, bones, phase, False, mesh)
                if local >= cycle and phase < overlap:
                    tail = u.AnimationLibrary.get_bone_poses_for_time(clip, bones, cycle+phase, False, mesh)
                    replacement = [blend(a,b,smooth(phase/overlap)) for a,b in zip(tail,replacement)]
            # Actor waypoints control planar blocking. Keep all poses planted at their original root.
            root = replacement[0]
            root.translation = u.Vector(poses[0].translation.x, poses[0].translation.y,
                poses[0].translation.z+root.translation.z-first_roots[clipname].translation.z)
            replacement[0] = root
            weight = min(1. if start == 0 else smooth(local/.25), smooth((end-t)/.25))
            max_changed_translation = max(max_changed_translation,
                max((a.translation-b.translation).length()*weight for a,b in zip(poses,replacement)))
            poses = [blend(a,b,weight) for a,b in zip(poses,replacement)]
        for bone, tr in zip(bones, poses):
            data[str(bone)][0].append(tr.translation)
            data[str(bone)][1].append(tr.rotation)
            data[str(bone)][2].append(tr.scale3d)
    controller = anim.get_editor_property('controller')
    controller.open_bracket('Act 2 supplied Reacting and Smith opening terror', False)
    for bone, keys in data.items():
        assert controller.set_bone_track_keys(bone, *keys, False), bone
    controller.close_bracket(False)
    assert eal.save_loaded_asset(anim, False)
    cue = u.DCAnimationCue()
    cue.start_time = 0
    cue.animation = anim
    cue.loop = False
    cue.play_rate = 1
    actor.set_editor_property('animation_cues', [cue])
    # All late acting, including the deaths and holding gesture, remains unchanged.
    for t in [7.5,9.,13.5,16.,29.,30.,35.,38.]:
        if any(start <= t < end for start,end,_ in windows):
            continue
        old = u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh)
        new = u.AnimationLibrary.get_bone_poses_for_time(anim,bones,t,False,mesh)
        assert max((a.translation-b.translation).length() for a,b in zip(old,new)) < .02,(who,t)
        assert max((a.scale3d-b.scale3d).length() for a,b in zip(old,new)) < .02,(who,t)
    assert max_changed_translation > .1,(who,'Reaction must alter the original body pose')
    report[who] = {'source': source_path, 'output': path, 'windows': windows,
                   'max_changed_translation_cm': max_changed_translation,
                   'blocking_changed': False, 'unchanged_pose_checks': 'passed'}
for actor in cast.values():
    actor.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world, LEVEL)
(OUT/'reactions.json').write_text(json.dumps(report,indent=2))
print('ACT2_REVISION3_REACTIONS_APPLIED',json.dumps(report))
