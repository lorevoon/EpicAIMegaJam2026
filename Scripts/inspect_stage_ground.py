import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level('/Game/WindmillValley/Levels/L_Showcase')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
rows=[]
for x in range(500,4501,500):
    for y in range(-1500,4501,500):
        hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True)
        vals=hit.to_tuple() if hasattr(hit,'to_tuple') else hit
        rows.append({'x':x,'y':y,'result':str(vals)})
(root/'Saved/Stage1Audit/ground.json').write_text(json.dumps(rows,indent=2))
details={}
for path in ['/Game/DirectorsCut/Characters/Swordsman/SK_Swordsman','/Game/DirectorsCut/Characters/OldMan/SK_OldMan','/Game/DirectorsCut/Characters/Crew/SK_Crew','/Game/Stylized_Village/Assets/Props/Furniture/SM_Crate_01']:
    a=u.load_asset(path);details[path]=str(a.get_bounds()) if a else 'MISSING'
(root/'Saved/Stage1Audit/mesh_bounds.json').write_text(json.dumps(details,indent=2))
u.log('GROUND_AUDIT_COMPLETE')
