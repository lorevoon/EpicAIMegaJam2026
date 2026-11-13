import unreal as u
from pathlib import Path
names=['create_new_streaming_level','move_actors_to_level','add_level_to_world','set_level_visibility']
out={n:getattr(u.EditorLevelUtils,n).__doc__ for n in names}
out['dynamic']=u.LevelStreamingDynamic.__doc__
out['worldfactory']=u.WorldFactory.__doc__
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/FilmSetAudit/level_api.txt').write_text(str(out))
