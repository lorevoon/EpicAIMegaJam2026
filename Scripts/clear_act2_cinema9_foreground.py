import unreal as u,json
from pathlib import Path
level='/Game/DirectorsCut/Levels/L_DC_Act2_Real';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(level);rows=[]
for a in aes.get_all_level_actors():
 if a.get_actor_label() in ['Act2 village blaze 06','Blaze warm bounce 6']:
  p=a.get_actor_location();old=list(p.to_tuple());p.x=-3700;p.y=-10400;a.set_actor_location(p,False,False)
  if isinstance(a,u.NiagaraActor):a.set_actor_scale3d(u.Vector(2,2,2))
  rows.append({'actor':a.get_actor_label(),'before':old,'after':list(p.to_tuple())})
assert len(rows)==2,rows
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),level)
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/Act2/CinemaRevision9/fire_clearance.json').write_text(json.dumps(rows,indent=2));print('RAID_FOREGROUND_CLEARED',rows)
