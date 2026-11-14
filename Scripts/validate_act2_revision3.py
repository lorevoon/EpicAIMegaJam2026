"""Read-only saved-scene checks for continuous marching, fall clearance and V3 acting.

Run after the Revision3 blocking/reactions, AudioFacesV3 bake and sequence refresh.
Only Saved/Act2/Revision3/validation.json is written; no Unreal asset is saved.
"""
import unreal as u
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/DirectorsCut'
LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
OUT=ROOT/'Saved/Act2/Revision3'
OUT.mkdir(parents=True,exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
report={'marches':{},'smith_fall':{},'performances':{},'cameras':[],'failures':[]}

def require(ok,message):
    if not ok:report['failures'].append(message)

def load(path):
    result=u.load_asset(path)
    assert result,path
    return result

def quaternion_error(a,b):
    av=list(a.to_tuple());bv=list(b.to_tuple())
    return min(math.sqrt(sum((x-y)**2 for x,y in zip(av,bv))),
               math.sqrt(sum((x+y)**2 for x,y in zip(av,bv))))

def pose_error(a,b):
    return max((a.translation-b.translation).length(),
               (a.scale3d-b.scale3d).length(),quaternion_error(a.rotation,b.rotation))

# Actual runtime evaluator, not only the authored waypoint tables.
for i in range(1,5):
    who='Raid orc '+str(i)
    a=cast[who]
    mesh=a.get_editor_property('character_mesh')
    cue=a.get_editor_property('animation_cues')[0]
    require('/Revision3/' in cue.animation.get_path_name() and '_March' in cue.animation.get_name(),who+' must use continuous March bake')
    require(abs(cue.animation.get_play_length()-38)<.001,who+' march must last38s')
    positions=[];steps=[];gait=[];root_xy=[];previous_pose=None
    for frame in range(153):
        t=frame*.25
        a.evaluate(t)
        p=a.get_actor_location()
        positions.append(list(p.to_tuple()))
        poses=u.AnimationLibrary.get_bone_poses_for_time(
            cue.animation,['rootSkeleton','L_knee_joint','R_knee_joint','L_ankle_joint','R_ankle_joint'],t,False,mesh)
        root_xy.append(math.hypot(poses[0].translation.x,poses[0].translation.y))
        if frame:
            q=u.Vector(*positions[-2])
            steps.append(math.hypot(p.x-q.x,p.y-q.y))
            gait.append(max(pose_error(x,y) for x,y in zip(poses[1:],previous_pose[1:])))
        previous_pose=poses
    require(min(steps)>.1,who+' contains a stopped quarter-second: '+str(min(steps)))
    require(min(gait)>.0001,who+' gait contains a held quarter-second: '+str(min(gait)))
    require(max(root_xy)<.01,who+' baked root XY reintroduces walk-loop backslide')
    require(max(steps)/min(steps)<1.2,who+' has a large speed change at a path corner')
    report['marches'][who]={'quarter_second_samples':153,'min_step_cm':min(steps),
        'max_step_cm':max(steps),'minimum_gait_change':min(gait),'max_local_root_xy_cm':max(root_xy),
        'start':positions[0],'end':positions[-1]}

# Bounds checks also work for visual props whose collision has been disabled.
bench=[a for a in actors if a.get_actor_label()=='Mr. Smith - familiar bench' or a.get_actor_label()=='Bench leg']
require(len(bench)>=3,'Expected relocated bench top and two legs')
boxes=[]
for a in bench:
    center,extent=a.get_actor_bounds(False)
    boxes.append((a.get_actor_label(),center,extent))
def sphere_clearance(p,radius,center,extent):
    d=[max(0.,abs(x-c)-e) for x,c,e in zip(p.to_tuple(),center.to_tuple(),extent.to_tuple())]
    return math.sqrt(sum(v*v for v in d))-radius

smith=cast['Mr. Smith']
comp=smith.get_component_by_class(u.SkeletalMeshComponent)
body={'head_joint':20.,'neck_joint':12.,'chest_joint':25.,'waist_joint':23.,'pelvis_joint':23.,
      'L_shoulder_joint':15.,'R_shoulder_joint':15.,'L_elbow_joint':12.,'R_elbow_joint':12.,
      'L_wrist_joint':10.,'R_wrist_joint':10.,'L_thigh_joint':18.,'R_thigh_joint':18.,
      'L_knee_joint':13.,'R_knee_joint':13.,'L_ankle_joint':12.,'R_ankle_joint':12.}
segments=[('head_joint','chest_joint',20.),('chest_joint','pelvis_joint',24.),
          ('L_shoulder_joint','L_elbow_joint',12.),('L_elbow_joint','L_wrist_joint',10.),
          ('R_shoulder_joint','R_elbow_joint',12.),('R_elbow_joint','R_wrist_joint',10.),
          ('L_thigh_joint','L_knee_joint',16.),('L_knee_joint','L_ankle_joint',12.),
          ('R_thigh_joint','R_knee_joint',16.),('R_knee_joint','R_ankle_joint',12.)]
minimum=1e9;contacts=[]
for frame in range(61):
    t=7+frame*.05
    smith.evaluate(t)
    points={name:comp.get_socket_location(name) for name in body}
    samples=[(name,p,body[name]) for name,p in points.items()]
    for start,end,radius in segments:
        for alpha in [.25,.5,.75]:samples.append((start+'-'+end,points[start]+(points[end]-points[start])*alpha,radius))
    for bone,p,radius in samples:
        for label,center,extent in boxes:
            gap=sphere_clearance(p,radius,center,extent);minimum=min(minimum,gap)
            if gap<0:contacts.append({'time':t,'bone':bone,'prop':label,'penetration_cm':-gap})
require(not contacts,'Smith fall overlaps bench: '+str(contacts[:5]))
report['smith_fall']={'samples':61,'body_spheres_per_sample':len(body)+len(segments)*3,
    'minimum_visual_clearance_cm':minimum,'contacts':contacts,
    'props':[{'name':label,'center':list(c.to_tuple()),'extent':list(e.to_tuple())} for label,c,e in boxes]}

# Facial rebaking must preserve the newly authored body tracks rather than duplicate old AudioFaces.
face_bones={'mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint'}
for who in ['Romeo','Juliet','Mr. Smith']:
    slug=who.replace(' ','').replace('.','')
    actor=cast[who];mesh=actor.get_editor_property('character_mesh')
    current=actor.get_editor_property('animation_cues')[0].animation
    expected=BASE+'/Act2/AudioFacesV3/A_Act2_'+slug+'_Speaking'
    require(current.get_path_name().split('.')[0]==expected,who+' does not use AudioFacesV3')
    baseline=load(BASE+'/Act2/Revision3/A_V3_'+slug+'_Performance')
    component=actor.get_component_by_class(u.SkeletalMeshComponent)
    bones=[component.get_bone_name(i) for i in range(component.get_num_bones())
           if str(component.get_bone_name(i)) not in face_bones]
    maximum=0.
    for t in [0.,2.,5.,6.5,7.5,10.5,12.,17.5,20.,24.,28.,30.,35.,38.]:
        old=u.AnimationLibrary.get_bone_poses_for_time(baseline,bones,t,False,mesh)
        new=u.AnimationLibrary.get_bone_poses_for_time(current,bones,t,False,mesh)
        maximum=max(maximum,max(pose_error(a,b) for a,b in zip(old,new)))
    require(maximum<.025,who+' facial bake changed V3 body pose: '+str(maximum))
    report['performances'][who]={'animation':current.get_path_name(),'baseline':baseline.get_path_name(),
                               'max_body_pose_error':maximum}

beat_ids={str(a.get_editor_property('beat_id')) for a in actors if isinstance(a,u.DCBeat)}
require(beat_ids=={'2.1','2.2','2.3','2.4','2.5'},'The stage must retain exactly five scoring beats')
cams=[a for a in actors if isinstance(a,u.CameraActor) and a.actor_has_tag('DCReferenceShot')]
require(len(cams)==6,'Expected six tagged reference cameras')
for cam in cams:
    tags={str(tag).split('=',1)[0]:str(tag).split('=',1)[1] for tag in cam.tags if '=' in str(tag)}
    require(tags.get('DCBeat') in beat_ids,cam.get_actor_label()+' has invalid beat tag')
    start=float(tags.get('DCStart','-1'));fov=float(tags.get('DCFOV','0'))
    require(0<=start<38 and 0<fov<180,cam.get_actor_label()+' has invalid timing/lens')
    report['cameras'].append({'name':cam.get_actor_label(),'start':start,'beat':tags.get('DCBeat'),
                             'position':list(cam.get_actor_location().to_tuple()),'fov':fov,'tags':tags})
report['cameras'].sort(key=lambda x:x['start'])
require([r['beat'] for r in report['cameras']]==['2.1','2.2','2.3','2.4','2.4','2.5'],
        'Reference shot order must use six views across the five requested scenes')
for i,row in enumerate(report['cameras']):
    for other in report['cameras'][i+1:]:
        require(math.dist(row['position'],other['position'])>50,'Reference cameras must have distinct positions')
slow=[r for r in report['cameras'] if 'DCSlowRate' in r['tags']]
require(bool(slow),'Expected selective slow-motion tags')
for row in slow:
    tags=row['tags'];require(0<float(tags['DCSlowRate'])<1 and
        0<=float(tags['DCSlowStart'])<float(tags['DCSlowEnd'])<=38,'Invalid slow-motion window')
seq=load(BASE+'/Act2/LS_DC_Act2_OrcRaid')
cut_tracks=[t for t in seq.get_tracks() if isinstance(t,u.MovieSceneCameraCutTrack)]
require(len(cut_tracks)==1 and len(cut_tracks[0].get_sections())==6,'Editable sequence must also have six cuts')
for actor in cast.values():actor.evaluate(0)
report['passed']=not report['failures']
(OUT/'validation.json').write_text(json.dumps(report,indent=2))
assert report['passed'],report['failures']
print('ACT2_REVISION3_VALIDATION_PASSED',json.dumps({'marches':report['marches'],'smith_min_clearance':minimum,'camera_count':len(cams),'beat_count':len(beat_ids)}))
