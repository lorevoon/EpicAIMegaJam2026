import unreal as u, json
from pathlib import Path
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name()=='DCPerformer':
        rows.append({'actor':a.get_actor_label(),'mesh':str(a.get_editor_property('character_mesh')),'cues':[(c.start_time,str(c.animation)) for c in a.get_editor_property('animation_cues')]})
print(json.dumps({'world':world.get_path_name(),'actors':rows},indent=2))
Path(__file__).resolve().parents[1].joinpath('Saved/requested_animation_inventory.json').write_text(json.dumps(rows,indent=2))
