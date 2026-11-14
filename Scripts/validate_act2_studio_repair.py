"""Read-only checks for enclosure, matched ground, spawn marker and raid materials."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/StudioRepair'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_Set');actors=list(aes.get_all_level_actors());labels={a.get_actor_label():a for a in actors}
for label in ['Green screen - left','Green screen - right','Studio roof']:
 b,e=labels[label].get_actor_bounds(False);assert b.y-e.y<=-11050 and b.y+e.y>=-7450,(label,b,e)
ground=labels['Studio scenic ground - matches performance terrain'];assert ground.static_mesh_component.static_mesh
assert ground.static_mesh_component.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise');actors=list(aes.get_all_level_actors())
marks=[a for a in actors if isinstance(a,u.DCStartMark)];assert len(marks)==1
assert marks[0].get_editor_property('hidden')
assert all(not c.is_visible() for c in marks[0].get_components_by_class(u.StaticMeshComponent))
cast=[a for a in actors if isinstance(a,u.DCPerformer) and ('orc' in a.get_actor_label().lower() or a.get_actor_label()=='Armoured monster')];assert len(cast)==5
usage=getattr(u.MaterialUsage,next(n for n in dir(u.MaterialUsage) if 'SKELETAL' in n))
for a in cast:
 for slot in a.get_editor_property('character_mesh').materials:
  m=slot.material_interface;assert u.MaterialEditingLibrary.has_material_usage(m,usage),(a.get_actor_label(),m.get_path_name())
assert len([a for a in actors if isinstance(a,u.CameraActor) and a.actor_has_tag('DCReferenceShot')])==6
report=dict(studio_enclosed=True,matched_ground=True,ground_collision_unchanged=True,hidden_spawn_disc=True,colored_orc_count=len(cast),reference_camera_count=6)
(out/'validation.json').write_text(json.dumps(report,indent=2));print('STUDIO_REPAIR_VALIDATED')
