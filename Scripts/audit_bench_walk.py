import unreal as u,json
from pathlib import Path
A=u.get_editor_subsystem(u.EditorActorSubsystem);actors=list(A.get_all_level_actors());r={}
hero=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo')
r['carry']=hero.get_editor_property('carry_transform').export_text();r['setdown']=hero.get_editor_property('set_down_transform').export_text();r['time']=hero.get_editor_property('set_down_time')
for t in [0,2.4,3.399,3.4]:
 hero.evaluate(t);r[str(t)]={'actor':hero.get_actor_transform().export_text(),'components':{c.get_name():{'tr':c.get_world_transform().export_text(),'bounds':str(c.get_local_bounds()) if isinstance(c,u.StaticMeshComponent) else ''} for c in hero.get_components_by_class(u.SceneComponent) if c.get_name() in ['Prop','Character']}}
hero.evaluate(0)
r['bench']=[{'label':a.get_actor_label(),'name':a.get_name(),'tr':a.get_actor_transform().export_text(),'bounds':str(a.get_actor_bounds(False)),'level':a.get_outer().get_path_name()} for a in actors if 'bench' in a.get_actor_label().lower()]
for path in ['/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd','/Game/DirectorsCut/Animations/Archer/A_Archer_XBotWalk']:
 a=u.load_asset(path);r[path]={'length':a.get_play_length(),'skeleton':a.get_editor_property('skeleton').get_path_name()}
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/FilmSetAudit/bench_walk_audit.json').write_text(json.dumps(r,indent=2))
u.log('BENCH_WALK_AUDIT_COMPLETE')

