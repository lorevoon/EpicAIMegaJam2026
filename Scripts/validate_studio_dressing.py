import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
code=(root/'Scripts/finish_studio_dressing.py').read_text()
exec(code[code.index('actors=list(aes.get_all_level_actors())\ncrew='):])
