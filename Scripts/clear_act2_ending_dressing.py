"""Clear visual-only ground dressing from the new stage's performance footprint."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(level);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
# Tight footprint around the bench, duel and mourning spot in this newly authored map.
lo=u.Vector(-6500,-9700,850);hi=u.Vector(-5350,-8300,1900);box=u.Box(min=lo,max=hi)
report=[]
for actor in aes.get_all_level_actors():
 p=actor.get_actor_location();label=(actor.get_actor_label()+' '+actor.get_class().get_name()).lower()
 center,extent=actor.get_actor_bounds(False)
 overlap=center.x+extent.x>=lo.x and center.x-extent.x<=hi.x and center.y+extent.y>=lo.y and center.y-extent.y<=hi.y and center.z+extent.z>=lo.z and center.z-extent.z<=hi.z
 if any(w in label for w in ['rock','grass','bush','fiore','erba']) and overlap:
  actor.set_actor_hidden_in_game(True);actor.set_actor_enable_collision(False);actor.set_is_temporarily_hidden_in_editor(True);report.append({'actor':actor.get_actor_label(),'position':list(p.to_tuple()),'hidden_group':True})
 for comp in actor.get_components_by_class(u.StaticMeshComponent):
  mesh=comp.static_mesh
  if not mesh:continue
  name=mesh.get_name().lower()
  if not any(w in name for w in ['rock','grass','weed','flower','bush','fern','wheat']):continue
  if isinstance(comp,u.InstancedStaticMeshComponent):
   ids=comp.get_instances_overlapping_box(box,True)
   if ids:
    assert comp.remove_instances(ids);report.append({'actor':actor.get_actor_label(),'mesh':name,'instances':len(ids)})
  else:
   center=actor.get_actor_location();_,extent=actor.get_actor_bounds(False)
   if lo.x<=center.x<=hi.x and lo.y<=center.y<=hi.y and center.z-extent.z<hi.z:
    comp.set_visibility(False);comp.set_hidden_in_game(True);comp.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);report.append({'actor':actor.get_actor_label(),'mesh':name,'hidden':True})
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
(root/'Saved/Act2/Revision2/ending_dressing.json').write_text(json.dumps(report,indent=2));print('ENDING_DRESSING_CLEARED',report)
