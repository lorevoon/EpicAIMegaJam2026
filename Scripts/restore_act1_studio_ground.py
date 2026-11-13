"""Restore copied landscape in the compact studio without changing principal performances."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
ground=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Ground')
ground.set_editor_property('initially_loaded',True)
ground.set_editor_property('initially_visible',True)
ground.set_editor_property('should_be_loaded',True)
ground.set_editor_property('should_be_visible',True)
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),True,False)
actors=list(aes.get_all_level_actors())
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
    hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
    assert hit[0],(x,y)
    return hit[4].z
timber=[a for a in actors if 'DC_TimberPractical' in [str(t) for t in a.tags]]
assert timber,'Ground restoration already applied; refusing to offset equipment twice.'
# Restore crew and operator to the actual landscape. Principal blocking and animations are untouched.
for a in actors:
    crew=isinstance(a,u.DCPerformer) and a.get_editor_property('ruins_shot')
    waypoint=isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') and a.get_editor_property('performer').get_editor_property('ruins_shot')
    if crew or waypoint or isinstance(a,(u.DCStartMark,u.DCClapperboard)):
        p=a.get_actor_location();p.z=height(p.x,p.y)+(100 if isinstance(a,u.DCStartMark) else 2)
        a.set_actor_location(p,False,False)
    if '/L_DC_Act1_Set.' not in a.get_path_name() or a.get_attach_parent_actor() or a in timber:continue
    label=a.get_actor_label();p=a.get_actor_location()
    if label.startswith('Soundstage floor'):
        p.z=635;a.set_actor_location(p,False,False)
        a.set_actor_label('Soundstage floor - compact 50m x 35m below original terrain')
        a.static_mesh_component.set_material(0,u.load_asset('/Game/DirectorsCut/Materials/M_Studio_Charcoal'))
    elif label.startswith('Green screen -') or label=='Studio rear shell':
        p.z=(650+4100)/2;s=a.get_actor_scale3d();s.z=(4100-650)/100
        a.set_actor_location(p,False,False);a.set_actor_scale3d(s)
    elif any(label.startswith(prefix) for prefix in ['Studio roof','Tracking marker','Screen support upright','Overhead lighting bar','Ceiling soft light','Studio broad fill']):continue
    elif isinstance(a,(u.StaticMeshActor,u.RectLight,u.SpotLight)):
        p.z+=height(p.x,p.y)-1050;a.set_actor_location(p,False,False)
for a in timber:aes.destroy_actor(a)
for a in actors:
    if isinstance(a,u.DCPerformer):a.evaluate(0)
assert not any('DC_TimberPractical' in [str(t) for t in a.tags] for a in aes.get_all_level_actors())
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
(root/'Saved/FilmSetAudit/restored_studio_ground.json').write_text(json.dumps({'removed_timber_parts':len(timber),'landscape_visible':ground.get_editor_property('initially_visible'),'studio_size_m':[50,35]},indent=2))
u.log('STUDIO_ORIGINAL_GROUND_RESTORED')
