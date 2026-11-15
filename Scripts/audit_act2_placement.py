import unreal as u, json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
report = {}
for level in ['L_DC_Act2_BrokenPromise', 'L_DC_Act2_Set']:
    assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/' + level)
    rows = []
    for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        d = dict(label=a.get_actor_label(), path=a.get_path_name(), cls=a.get_class().get_name(), pos=list(a.get_actor_location().to_tuple()), tags=[str(t) for t in a.tags])
        if isinstance(a, u.DCBeat):
            d['beat'] = {p:str(a.get_editor_property(p)) for p in ['beat_id','window_start','window_end','required_hold','camera_spot','camera_spot_radius','min_distance','max_distance','anchors']}
            d['spot_world'] = list(a.get_actor_transform().transform_location(a.get_editor_property('camera_spot')).to_tuple())
        meshes=a.get_components_by_class(u.StaticMeshComponent)
        d['meshes']=[dict(mesh=c.static_mesh.get_path_name() if c.static_mesh else '', collision=str(c.get_collision_enabled())) for c in meshes]
        if isinstance(a,u.DCBeat) or isinstance(a,u.CameraActor) or level.endswith('_Set') or any(w in d['label'].lower() for w in ['tree','ground','terrain']): rows.append(d)
    report[level]=rows
(root/'Saved/act2_placement_audit.json').write_text(json.dumps(report,indent=2))
print('ACT2_PLACEMENT_AUDITED')
