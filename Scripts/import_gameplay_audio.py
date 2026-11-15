"""Import the supplied countdown and director approval effects."""
from pathlib import Path
import unreal as u

root = Path(__file__).resolve().parents[1]
folder = '/Game/DirectorsCut/Audio/Gameplay'
for name in ('TimerTick', 'DirectorContent'):
    task = u.AssetImportTask()
    task.filename = str(root / 'SourceArt/GameplayAudio' / (name + '.wav'))
    task.destination_path = folder
    task.destination_name = 'SW_' + name
    task.automated = True
    task.replace_existing = True
    task.save = True
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = u.load_asset(folder + '/SW_' + name)
    assert isinstance(sound, u.SoundWave), name
    sound.set_editor_property('sound_asset_compression_type', u.SoundAssetCompressionType.PCM)
    sound.set_editor_property('loading_behavior', u.SoundWaveLoadingBehavior.FORCE_INLINE)
    sound.set_editor_property('looping', False)
    assert u.EditorAssetLibrary.save_loaded_asset(sound, False)
    assert sound.duration > 0
    print('GAMEPLAY_AUDIO_OK', sound.get_path_name(), sound.duration)
