"""Read-only motion continuity audit, including baked skeletal motion and route keys."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
actors=list(aes.get_all_level_actors());out={}
for actor in actors:
    if not isinstance(actor,u.DCPerformer):continue
    who=actor.get_actor_label();mesh=actor.get_editor_property('character_mesh');c=actor.get_component_by_class(u.SkeletalMeshComponent)
    keys=[]
    for w in actors:
        if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==actor:
            keys.append({'time':w.get_editor_property('time'),'pos':list(w.get_actor_location().to_tuple()),'yaw':w.get_actor_rotation().yaw,'name':w.get_name()})
    cues=[];local={}
    for cue in actor.get_editor_property('animation_cues'):
        anim=cue.animation
        cues.append({'time':cue.start_time,'path':anim.get_path_name(),'length':anim.get_play_length(),'rate':cue.play_rate,'offset':cue.start_offset,'loop':cue.loop,'root_lock':anim.get_editor_property('force_root_lock')})
        if who in ['Romeo','Juliet','Mr. Smith']:
            bones=['rootSkeleton','pelvis_joint','head_joint','L_ankle_joint','R_ankle_joint']
            samples=[]
            for i in range(601):
                poses=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,i/30,False,mesh)
                samples.append({b:{'p':list(p.translation.to_tuple()),'q':list(p.rotation.to_tuple())} for b,p in zip(bones,poses)})
            local[anim.get_path_name()]=samples
    samples=[]
    for i in range(1201):
        actor.evaluate(i/60)
        samples.append({'p':list(actor.get_actor_location().to_tuple()),'yaw':actor.get_actor_rotation().yaw,'pelvis':list(c.get_socket_location('pelvis_joint').to_tuple()),'head':list(c.get_socket_location('head_joint').to_tuple())})
    out[who]={'keys':sorted(keys,key=lambda x:x['time']),'cues':cues,'linear':actor.get_editor_property('linear_movement'),'mesh_offset':actor.get_editor_property('mesh_offset').export_text(),'samples':samples,'local':local}
    actor.evaluate(0)
(root/'Saved/FilmSetAudit/motion_before.json').write_text(json.dumps(out))
u.log('ACT1_MOTION_AUDITED')
