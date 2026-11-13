import unreal as u,json
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
rows=[{'actor':a.get_name(),'location':str(a.get_actor_location())} for a in u.GameplayStatics.get_all_actors_of_class(w,u.DCPerformer)]
open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/runtime_state.json','w').write(json.dumps(rows,indent=2))
