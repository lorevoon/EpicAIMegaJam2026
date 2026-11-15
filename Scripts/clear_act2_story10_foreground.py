import unreal as u,json
from pathlib import Path
level='/Game/DirectorsCut/Levels/L_DC_Act2_Real';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(level)
a=next(a for a in aes.get_all_level_actors() if a.get_actor_label()=='Act2 village blaze 02');p=a.get_actor_location();p.y=-7100;a.set_actor_location(p,False,False);a.set_actor_scale3d(u.Vector(2.5,2.5,2.5));assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),level)
print('ENDING_FOREGROUND_CLEARED')
