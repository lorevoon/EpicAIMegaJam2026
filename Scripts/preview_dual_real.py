import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for name,visible in [('L_DC_Act1_Set',False),('L_DC_Act1_Real',True)]:
 layer=u.GameplayStatics.get_streaming_level(world,name);u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
mesh=u.load_asset('/Game/DirectorsCut/Environment/SM_Act1_RealGround')
(root/'Saved/FilmSetAudit/real_ground_bounds.txt').write_text(str(mesh.get_bounds()))
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCPerformer):a.evaluate(0.)
 # Hide editor labels in the presentation preview only.
 for c in a.get_components_by_class(u.TextRenderComponent):c.set_visibility(False)
 if 'DC_SetOnly' in [str(t) for t in a.tags]:a.set_is_temporarily_hidden_in_editor(True)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('REAL_PREVIEW_READY')
