import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
ground=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Ground')
assert not ground.get_editor_property('initially_visible')
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),False,False)
cast=[a for a in actors if isinstance(a,u.DCPerformer)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard))]
errors=[];worst=0
for frame in range(201):
    for a in cast:
        a.evaluate(frame*.1);p=a.get_actor_location()
        hit=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,30),p-u.Vector(0,0,400),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
        delta=p.z-hit[4].z-2 if hit[0] else 999
        worst=max(worst,abs(delta))
        if abs(delta)>3:errors.append([frame*.1,a.get_actor_label(),delta,str(hit[9])])
for a in cast:a.evaluate(0)
report={'samples':201*len(cast),'worst_foot_clearance_error_cm':worst,'errors':errors,'timber_parts':sum('DC_TimberPractical' in [str(t) for t in a.tags] for a in actors)}
(root/'Saved/FilmSetAudit/compact_studio_validation.json').write_text(json.dumps(report,indent=2))
assert not errors,errors[:15]
u.log('COMPACT_STUDIO_VALIDATED')
