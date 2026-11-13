"""Remove background crew performers and their dependent blocking from Act 1."""
import unreal as u,json
from pathlib import Path
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert 'L_DC_Act1_Promise' in world.get_path_name(),world.get_path_name()
aes=u.get_editor_subsystem(u.EditorActorSubsystem);actors=list(aes.get_all_level_actors())
crew=[a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label().startswith('Crew -')]
waypoints=[a for a in actors if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') in crew]
report={'performers':[a.get_actor_label() for a in crew],'waypoints_removed':len(waypoints)}
for a in waypoints+crew:assert aes.destroy_actor(a)
remaining=list(aes.get_all_level_actors())
assert not any(isinstance(a,u.DCPerformer) and a.get_actor_label().startswith('Crew -') for a in remaining)
for name in ['Romeo','Juliet','Mr. Smith']:
 assert any(isinstance(a,u.DCPerformer) and a.get_actor_label()==name for a in remaining),name
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/FilmSetAudit/crew_removal.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_CREW_REMOVED '+json.dumps(report))
