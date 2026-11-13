import unreal as u,json
from pathlib import Path
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
a=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
r={'world':w.get_path_name(),'dirty':[x.get_path_name() for x in u.EditorLoadingAndSavingUtils.get_dirty_content_packages()],'cast':[]}
for p in a:
 if isinstance(p,u.DCPerformer):
  r['cast'].append({'name':p.get_actor_label(),'mesh':str(p.get_editor_property('character_mesh')),'cues':[{'start':c.start_time,'anim':c.animation.get_path_name(),'length':c.animation.get_play_length()} for c in p.get_editor_property('animation_cues')]})
 if isinstance(p,u.DCMultiCamDirector):r['director']={'name':p.get_actor_label(),'duration':p.get_editor_property('take_duration')}
print(json.dumps(r))

