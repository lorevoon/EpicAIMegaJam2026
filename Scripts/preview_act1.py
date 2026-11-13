import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
time=float((root/'Saved/Stage1Audit/preview_time.txt').read_text())
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
rows=[]
for a in actors:
    if isinstance(a,u.DCPerformer):
        a.evaluate(time)
        c=a.get_component_by_class(u.SkeletalMeshComponent)
        loc=a.get_actor_location()
        row={'name':a.get_actor_label(),'position':[loc.x,loc.y,loc.z],'rotation':str(a.get_actor_rotation()),'mesh':str(c.get_skeletal_mesh_asset()),'bones':{}}
        for b in ['head_joint','L_wrist_joint','R_wrist_joint','L_ankle_joint','R_ankle_joint']:
            p=c.get_socket_location(b);row['bones'][b]=[p.x,p.y,p.z]
        rows.append(row)
(root/'Saved/Stage1Audit/preview_state.json').write_text(json.dumps(rows,indent=2))
u.log('ACT1_PREVIEW_TIME '+str(time))
