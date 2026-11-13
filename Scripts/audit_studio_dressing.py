import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert '/L_DC_Act1_Promise.' in world.get_path_name()
rows=[]
for a in aes.get_all_level_actors():
    if a.get_attach_parent_actor():continue
    p=a.get_actor_location();c,e=a.get_actor_bounds(False,True)
    row=dict(name=a.get_name(),label=a.get_actor_label(),path=a.get_path_name(),cls=a.get_class().get_name(),pos=[p.x,p.y,p.z],scale=list(a.get_actor_scale3d().to_tuple()),rot=list(a.get_actor_rotation().to_tuple()),bounds=[c.x-e.x,c.x+e.x,c.y-e.y,c.y+e.y,c.z-e.z,c.z+e.z],tags=[str(t) for t in a.tags])
    if isinstance(a,u.StaticMeshActor):row['mesh']=a.static_mesh_component.static_mesh.get_path_name() if a.static_mesh_component.static_mesh else None
    if isinstance(a,u.DCWaypoint):row['performer']=a.get_editor_property('performer').get_actor_label() if a.get_editor_property('performer') else None
    rows.append(row)
(root/'Saved/FilmSetAudit/dressing_before.json').write_text(json.dumps(rows,indent=2))
assets=u.EditorAssetLibrary.list_assets('/Game/Hollywood_Vol3',True,False)
meshes=[]
for path in assets:
    data=u.EditorAssetLibrary.find_asset_data(path)
    if str(data.asset_class_path.asset_name)=='StaticMesh':
        m=u.load_asset(path);b=m.get_bounds();meshes.append({'path':path,'origin':list(b.origin.to_tuple()),'extent':list(b.box_extent.to_tuple())})
(root/'Saved/FilmSetAudit/dressing_assets.json').write_text(json.dumps(meshes,indent=2))
u.log('STUDIO_DRESSING_AUDITED')
ignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
samples=[]
for x in range(-6500,-4249,250):
    for y in range(-10250,-8749,250):
        h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
        samples.append([x,y,h[4].z if h[0] else None])
(root/'Saved/FilmSetAudit/dressing_ground.json').write_text(json.dumps(samples))
