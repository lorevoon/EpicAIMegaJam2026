import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
studio=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Set')
u.EditorLevelUtils.make_level_current(studio)
for a in actors:
    if '/L_DC_Act1_Set.' not in a.get_path_name():continue
    p=a.get_actor_location()
    if abs(p.x+4450)<1 and abs(p.y+9925)<1:
        if a.get_actor_label() in ['Lighting stand','Lighting stand base','Softbox housing','Softbox diffusion']:aes.destroy_actor(a)
        elif isinstance(a,u.RectLight):
            a.set_actor_location(u.Vector(-4470,-8780,1320),False,False)
            a.set_actor_rotation(u.Rotator(pitch=-15,yaw=180),False)
            c=a.get_component_by_class(u.RectLightComponent)
            c.set_editor_property('source_width',70.);c.set_editor_property('source_height',80.)
            c.set_cast_shadows(False);c.set_editor_property('affect_translucent_lighting',False)
            a.set_actor_label('Production fill - practical illumination')
black=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_Charcoal')
for label,p,size in [('Equipment power cable - perimeter',(-5200,-10405,1081),(1570,2,2)),('Equipment power cable - key light',(-5650,-10250,1081),(2,305,2))]:
    a=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*p));a.set_actor_label(label);a.tags=['DC_DressedStudio','DC_Studio']
    a.set_folder_path('Act1/Studio/Filming equipment');a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
    a.static_mesh_component.set_material(0,black);a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    a.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,False)
actors=list(aes.get_all_level_actors())
crew=[a for a in actors if isinstance(a,u.DCPerformer) and a.get_editor_property('ruins_shot')]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard))]
problems=[];max_error=0
for frame in range(201):
    for a in crew:
        a.evaluate(frame*.1);p=a.get_actor_location()
        h=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,30),p-u.Vector(0,0,200),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
        error=abs(p.z-h[4].z-2) if h[0] else 999;max_error=max(error,max_error)
        if error>4:problems.append(['foot',a.get_actor_label(),frame*.1,error])
        result=u.SystemLibrary.sphere_trace_single(world,p+u.Vector(0,0,90),p+u.Vector(.1,0,90),30,u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True)
        if result and result.to_tuple()[0]:problems.append(['obstruction',a.get_actor_label(),frame*.1,str(result.to_tuple()[9])])
for a in crew:a.evaluate(0)
walls=[a for a in actors if a.get_actor_label().startswith('Green screen - backstage')]
assert len(walls)==2
assert all('/L_DC_Act1_Set.' in a.get_path_name() for a in actors if 'DC_DressedStudio' in [str(t) for t in a.tags])
report={'crew_samples':402,'max_foot_error_cm':max_error,'issues':problems,'backstage_screens':len(walls),'all_new_dressing_studio_only':True}
(root/'Saved/FilmSetAudit/dressing_validation.json').write_text(json.dumps(report,indent=2))
assert not problems,problems[:10]
u.log('STUDIO_DRESSING_VALIDATED')
