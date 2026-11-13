import unreal, json
from pathlib import Path
out = Path(r'C:\Unity Projects\EpicAIMegaJam2026\Saved\Stage1Audit')
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/DirectorsCut','/Game/WindmillValley'], True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level('/Game/WindmillValley/Levels/L_Showcase')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
rows = []
for a in actors:
    loc = a.get_actor_location()
    row = dict(name=a.get_name(),label=a.get_actor_label(),cls=a.get_class().get_name(),location=[loc.x,loc.y,loc.z])
    if any(s in a.get_actor_label().lower() for s in ['mill','bridge','bench','path','landscape','stairs','house']):
        origin, extent = a.get_actor_bounds(False)
        row['bounds'] = [[origin.x,origin.y,origin.z],[extent.x,extent.y,extent.z]]
    rows.append(row)
out.joinpath('actors.json').write_text(json.dumps(rows,indent=2))
out.joinpath('python_ready.txt').write_text(unreal.Paths.project_dir())
