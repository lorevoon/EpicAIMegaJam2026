"""Verify both imported effects load with valid, non-looping audio."""
import unreal as u

for name in ('TimerTick', 'DirectorContent'):
    sound = u.load_asset(f'/Game/DirectorsCut/Audio/Gameplay/SW_{name}')
    assert sound and sound.get_path_name() == f'/Game/DirectorsCut/Audio/Gameplay/SW_{name}.SW_{name}'
    assert sound.duration > 0 and not sound.get_editor_property('looping')
    print('GAMEPLAY_AUDIO_LOAD_OK', name, sound.duration)

