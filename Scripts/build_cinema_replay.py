"""Build the isolated cinema presentation map and annotate Act 1 emotions. Run in Unreal Python."""
import unreal as u
base='/Game/DirectorsCut/Levels/'
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
path=base+'L_DC_Cinema'
if u.EditorAssetLibrary.does_asset_exist(path):
    assert les.load_level(path)
else:
    assert les.new_level(path)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cinemas=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCCinemaReplay)]
if not cinemas:
    actor=aes.spawn_actor_from_class(u.DCCinemaReplay,u.Vector(0,0,0))
    actor.set_actor_label('Cinema - projector, audience and premiere reveal')
assert u.EditorLoadingAndSavingUtils.save_map(world,path)
assert les.load_level(base+'L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in aes.get_all_level_actors():
    if isinstance(a,u.DCBeat):
        # Explicit story direction, not a keyword heuristic applied at runtime.
        a.set_editor_property('replay_mood',u.DCCinemaMood.LOVE if str(a.get_editor_property('beat_id')) in ('1.2','1.3','1.4') else u.DCCinemaMood.NEUTRAL)
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'L_DC_Act1_Promise')
u.log('CINEMA_AUTHORING_PASS: cinema map saved; Act 1 love cues assigned')
