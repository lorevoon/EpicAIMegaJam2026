"""Keep Act 2 placement hints aligned and remove the two requested set extras."""
import unreal as u, json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
les = u.get_editor_subsystem(u.LevelEditorSubsystem)
aes = u.get_editor_subsystem(u.EditorActorSubsystem)
base = '/Game/DirectorsCut/Levels/'
assert les.load_level(base + 'L_DC_Act2_Set')
removed = []
for a in aes.get_all_level_actors():
    if a.get_actor_label().lower() in ('exhausted runner', 'lighting technician'):
        removed.append(a.get_actor_label())
        assert aes.destroy_actor(a)
assert les.save_current_level()
assert les.load_level(base + 'L_DC_Act2_BrokenPromise')
actors = list(aes.get_all_level_actors())
beats = {str(a.get_editor_property('beat_id')):a for a in actors if isinstance(a,u.DCBeat)}
spots = []
for a in actors:
    if a.actor_has_tag('DCReferenceShot'):
        beat_id = next(str(t).split('=',1)[1] for t in a.tags if str(t).startswith('DCBeat='))
        beat = beats[beat_id]
        beat.set_editor_property('camera_spot', beat.get_actor_transform().inverse_transform_location(a.get_actor_location()))
        beat.set_actor_label(beat_id + ' ' + beat.get_editor_property('display_name'))
        spots.append(dict(beat=beat_id, position=list(a.get_actor_location().to_tuple())))
    if a.get_actor_label() == 'Practical promise tree':
        a.tags = list(a.tags) + ([] if a.actor_has_tag('DCNoCameraGround') else ['DCNoCameraGround'])
assert len(spots) == 5
assert les.save_current_level()
assert les.load_level(base + 'L_DC_Act2_Set')
assert not any(a.get_actor_label().lower() in ('exhausted runner','lighting technician') for a in aes.get_all_level_actors())
(root/'Saved/act2_placement_changes.json').write_text(json.dumps(dict(removed=removed,spots=spots),indent=2))
print('ACT2_PLACEMENT_SAVED', removed, len(spots))
