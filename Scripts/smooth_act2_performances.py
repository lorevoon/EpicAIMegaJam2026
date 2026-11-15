"""Run with Unreal Editor File > Execute Python Script, with PIE stopped.

Creates separate FlowRevision5 performances from the CURRENT map's performances,
preserving facial animation, ground correction, blocking, dialogue and cameras.
The original assets remain intact. A baseline manifest makes reruns idempotent.
Rebinds runtime cues and existing Sequencer animation sections without rebuilding
the cameras. Writes verification evidence to Saved/Act2/Flow.
"""
import unreal as u
import json, math, sys, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Scripts'))
from act2_flow_plan import DUEL, active_moves, clip_phase, smooth, OVERLAP
BASE = '/Game/DirectorsCut'
LEVEL = BASE+'/Levels/L_DC_Act2_BrokenPromise'
FOLDER = BASE+'/Act2/FlowRevision5'
OUT = ROOT/'Saved/Act2/Flow'
OUT.mkdir(parents=True, exist_ok=True)
eal = u.EditorAssetLibrary
les = u.get_editor_subsystem(u.LevelEditorSubsystem)
aes = u.get_editor_subsystem(u.EditorActorSubsystem)
assert not u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world(), 'Stop Play before running.'
assert les.load_level(LEVEL)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast = {a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
assert len(cast)==8
seq = u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid')
assert seq
for rel in ['Content/DirectorsCut/Levels/L_DC_Act2_BrokenPromise.umap',
            'Content/DirectorsCut/Act2/LS_DC_Act2_OrcRaid.uasset']:
    backup = OUT/(Path(rel).name+'.before_flow')
    if not backup.exists(): shutil.copy2(ROOT/rel, backup)
manifest_path = OUT/'baseline.json'
if manifest_path.exists():
    baseline = json.loads(manifest_path.read_text())
else:
    baseline = {}
    for who,a in cast.items():
        cues = list(a.get_editor_property('animation_cues'))
        assert len(cues)==1 and abs(cues[0].start_time)<.001 and abs(cues[0].play_rate-1)<.001
        assert not cues[0].loop and abs(cues[0].start_offset)<.001
        baseline[who] = cues[0].animation.get_path_name()
    manifest_path.write_text(json.dumps(baseline,indent=2))

def load(p):
    a=u.load_asset(p)
    assert a,p
    return a

def blend(a,b,w):
    q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
    if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
    v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v))
    tr=u.Transform()
    tr.translation=a.translation+(b.translation-a.translation)*w
    tr.rotation=u.Quat(*[x/n for x in v])
    tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w
    return tr

# Body transitions across the entire act; keep all expressive facial keys intact.
SEAMS = {'Romeo':[10,13,17.5,18.8,26,29.2,33],
         'Juliet':[10,13,17,29,31],
         'Mr. Smith':[7.2],
         'Armoured monster':[6.3,8.5,16.5,18.8,26]}
report={};outputs={}
for who,a in cast.items():
    source=load(baseline[who]);mesh=a.get_editor_property('character_mesh')
    comp=a.get_component_by_class(u.SkeletalMeshComponent)
    bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones())]
    assert abs(source.get_play_length()-38)<.05,(who,source.get_play_length())
    # Facial bones and descendants retain the latest speech, blinking and death expression.
    facial=set()
    for bone in bones:
        cursor=str(bone)
        while cursor and cursor!='None':
            if any(token in cursor.lower() for token in ['mouth','eye','brow','jaw','lip']):
                facial.add(str(bone));break
            parent=str(comp.get_parent_bone(cursor))
            if parent==cursor:break
            cursor=parent
    original=[list(u.AnimationLibrary.get_bone_poses_for_time(source,bones,f/30,False,mesh)) for f in range(1141)]
    data={str(b):[[],[],[]] for b in bones}
    clips={}
    if who in DUEL:
        prefix=BASE+'/Animations/Swordsman/A_Swordsman_Support_' if who=='Romeo' else BASE+'/Act2/A_Raider_Support_'
        clips={m[2]:load(prefix+m[2]) for m in DUEL[who]}
        assert all(c.get_editor_property('skeleton')==mesh.skeleton for c in clips.values())
    def move_pose(move,t):
        clip=clips[move[2]]
        return list(u.AnimationLibrary.get_bone_poses_for_time(clip,bones,clip_phase(move,t)*clip.get_play_length(),False,mesh))
    changed=0
    for f in range(1141):
        t=f/30;poses=list(original[f])
        # Short symmetric rotation filter across clip junctions avoids a one-frame
        # acceleration spike. Root/contact translations and facial animation stay exact.
        weight=max([smooth(1-abs(t-s)/.22) for s in SEAMS.get(who,[])] or [0])
        if weight>0:
            for i,b in enumerate(bones):
                if i==0 or str(b) in facial:continue
                avg=blend(original[max(0,f-2)][i],original[min(1140,f+2)][i],.5)
                filtered=blend(poses[i],avg,weight*.5)
                filtered.translation=poses[i].translation;filtered.scale3d=poses[i].scale3d
                poses[i]=filtered
        moves=active_moves(who,t) if who in DUEL else []
        if moves:
            replacement=move_pose(moves[0],t)
            if len(moves)==2:
                w=smooth((t-moves[1][0])/OVERLAP)
                replacement=[blend(x,y,w) for x,y in zip(replacement,move_pose(moves[1],t))]
            w=min(smooth((t-18.80)/.28),smooth((26-t)/.16))
            for i,b in enumerate(bones):
                if i==0 or str(b) in facial:continue
                poses[i]=blend(poses[i],replacement[i],w)
            changed+=1
        for b,tr in zip(bones,poses):
            data[str(b)][0].append(tr.translation)
            data[str(b)][1].append(tr.rotation)
            data[str(b)][2].append(tr.scale3d)
    # Background raiders already have a continuous march; keep their original assets.
    if who not in SEAMS:
        report[who]={'source':baseline[who],'status':'continuous march retained'}
        continue
    dst=FOLDER+'/A_Flow_'+who.replace(' ','').replace('.','')
    anim=u.load_asset(dst) or eal.duplicate_asset(baseline[who],dst)
    assert anim
    controller=anim.get_editor_property('controller')
    controller.open_bracket('Continuous Act 2 performance',False)
    for b,keys in data.items():assert controller.set_bone_track_keys(b,*keys,False),b
    controller.close_bracket(False)
    assert eal.save_loaded_asset(anim,False)
    # Check every frame: no root drift, no lost face keys, no duration changes.
    for f in range(1141):
        actual=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,f/30,False,mesh)
        for i,b in enumerate(bones):
            if i==0 or str(b) in facial:
                old=original[f][i];new=actual[i]
                assert (old.translation-new.translation).length()<.03,(who,f,b,'translation')
                assert (old.scale3d-new.scale3d).length()<.03,(who,f,b,'scale')
                dot=abs(sum(x*y for x,y in zip(old.rotation.to_tuple(),new.rotation.to_tuple())))
                assert dot>.9999,(who,f,b,'rotation')
    outputs[who]=anim
    report[who]={'source':baseline[who],'output':dst,'combat_frames':changed,
                 'seams':SEAMS[who],'root_and_face_validation':'passed'}

# Bind only after all performances are generated and validated.
for who,anim in outputs.items():
    cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.loop=False;cue.play_rate=1
    cast[who].set_editor_property('animation_cues',[cue])
bound=set()
for binding in seq.get_bindings():
    parent=binding.get_parent()
    who=parent.get_name() if parent and parent.is_valid() else binding.get_name()
    if who not in outputs:continue
    for track in binding.get_tracks():
        if isinstance(track,u.MovieSceneSkeletalAnimationTrack):
            for section in track.get_sections():
                params=section.params;params.animation=outputs[who];section.params=params
                bound.add(who)
assert bound==set(outputs),('Missing Sequencer bindings',bound,set(outputs))
for a in cast.values():a.evaluate(0)
assert eal.save_loaded_asset(seq,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
report['status']='Applied; visual playback review still required'
(OUT/'report.json').write_text(json.dumps(report,indent=2))
print('ACT2_FLOW_APPLIED',json.dumps(report))
