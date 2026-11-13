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
(root/'Saved/FilmSetAudit/motion_after.json').write_text(json.dumps(out))
u.log('ACT1_MOTION_AUDITED')

"""Validate saved Act 1 facial acting, close walking and deterministic lip contact."""
import unreal as u, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]

cast={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet','Mr. Smith']}
comps={n:a.get_component_by_class(u.SkeletalMeshComponent) for n,a in cast.items()}
report={'samples':{},'max_kiss_gap_cm':0,'walk_spacing_cm':[]}
for frame in range(1201):
 t=frame/60
 for a in cast.values():a.evaluate(t)
 if 7<=t<=12:
  p=cast['Romeo'].get_actor_location()-cast['Juliet'].get_actor_location()
  report['walk_spacing_cm'].append((p.x*p.x+p.y*p.y)**.5)
 if t>=17.5:
  gap=(comps['Romeo'].get_socket_location('mouth_joint')-comps['Juliet'].get_socket_location('mouth_joint')).length()
  report['max_kiss_gap_cm']=max(report['max_kiss_gap_cm'],gap)
 if frame in [0,240,420,600,750,960,1050,1080,1140,1200]:
  report['samples'][str(t)]={n:{'mouth':[c.get_socket_location('mouth_joint').x,c.get_socket_location('mouth_joint').y,c.get_socket_location('mouth_joint').z],'eye_scale':c.get_socket_transform('eyes_joint',u.RelativeTransformSpace.RTS_WORLD).scale3d.y,'smile':c.get_anim_instance().get_curve_value('FaceSmile'),'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())]} for n,c in comps.items()}
# Seeking backwards or replaying a recorded time must reproduce the same pose and material.
for t in [18.,7.,20.,4.,0.,18.,10.]:
 for n,a in cast.items():
  a.evaluate(t);c=comps[n];row=report['samples'][str(t)]
  expected=row[n];p=c.get_socket_location('mouth_joint')
  assert (p-u.Vector(*expected['mouth'])).length()<.01,(n,t,'seek pose')
  for i in range(c.get_num_materials()):
   mat=c.get_material(i)
   if 'MI_Act1_FacialExpressions' in mat.get_name():
    assert abs(mat.get_scalar_parameter_value('FaceSmile')-expected['smile'])<.01,(n,t,'material curve')
for a in cast.values():a.evaluate(0)
report['walk_spacing_cm']=[min(report['walk_spacing_cm']),max(report['walk_spacing_cm'])]
(root/'Saved/FilmSetAudit/face_validation.json').write_text(json.dumps(report,indent=2))
assert report['max_kiss_gap_cm']<1,report['max_kiss_gap_cm']
assert all(abs(s-85)<.1 for s in report['walk_spacing_cm'])
assert report['samples']['7.0']['Mr. Smith']['smile']>.95
for who in ['Romeo','Juliet']:
 assert report['samples']['18.0'][who]['eye_scale']<.06
assert all('MI_Act1_FacialExpressions' in ' '.join(row['materials']) for row in report['samples']['0.0'].values())
u.log('ACT1_FACE_VALIDATION_PASSED '+str(report['max_kiss_gap_cm']))

import math
metrics={}
for who,row in out.items():
 metrics[who]={}
 for bone in ['p','pelvis','head']:
  steps=[math.dist(a[bone],b[bone]) for a,b in zip(row['samples'],row['samples'][1:])]
  metrics[who][bone+'_max_step_cm']=max(steps)
  assert max(steps)<6,(who,bone,max(steps))
 for t in [20,5.4,0,12.5,7.2,18,4.0]:
  actor=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()==who)
  actor.evaluate(t);c=actor.get_component_by_class(u.SkeletalMeshComponent)
  expected=row['samples'][round(t*60)]
  assert math.dist(c.get_socket_location('pelvis_joint').to_tuple(),expected['pelvis'])<.01,(who,t,'determinism')
 actor.evaluate(0)
(root/'Saved/FilmSetAudit/motion_validation.json').write_text(json.dumps(metrics,indent=2))
u.log('ACT1_SMOOTH_MOTION_VALIDATION_PASSED '+json.dumps(metrics))
