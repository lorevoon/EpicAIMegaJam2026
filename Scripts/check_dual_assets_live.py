import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
report={'active_world':world.get_name(),'ground_bounds':str(u.load_asset('/Game/DirectorsCut/Environment/SM_Act1_RealGround').get_bounds()),'ground_two_sided':u.load_asset('/Game/DirectorsCut/Materials/M_Act1_OutdoorGround').get_editor_property('two_sided')}
(root/'Saved/FilmSetAudit/live_final_check.json').write_text(json.dumps(report,indent=2))
u.log('FINAL_ASSETS_CHECKED')
