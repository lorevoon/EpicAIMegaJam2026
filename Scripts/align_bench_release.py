"""Place the bench beside Romeo's authored release mark, aligned to his hands."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];aes=u.get_editor_subsystem(u.EditorActorSubsystem);actors=list(aes.get_all_level_actors());world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
hero=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo');hero.evaluate(0)
mark=min([a for a in actors if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer')==hero],key=lambda a:abs(a.get_editor_property('time')-3.4))
p=mark.get_actor_location();yaw=mark.get_actor_rotation().yaw;forward=u.Rotator(yaw=yaw).get_forward_vector();center=p+forward*60
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True)
 assert hit;return hit.to_tuple()[4].z
seat=next(a for a in actors if a.get_actor_label()=='Mill-side bench seat');old=seat.get_actor_transform();new=u.Transform();new.translation=u.Vector(center.x,center.y,ground(center.x,center.y)+47);new.rotation=u.Rotator(yaw=yaw-90).quaternion()
parts=[a for a in actors if a.get_actor_label() in ['Mill-side bench seat','Bench back','Bench leg']]
for a in parts:
 offset=old.inverse_transform_direction(a.get_actor_location()-old.translation)
 pos=new.translation+new.transform_direction(offset)
 a.set_actor_location_and_rotation(pos,new.rotation.rotator(),False,False)
 if a.get_actor_label()=='Bench leg':
  bottom=ground(pos.x,pos.y)-1;top=new.translation.z-6
  pos.z=(bottom+top)/2;a.set_actor_location(pos,False,False);scale=a.get_actor_scale3d();scale.z=(top-bottom)/100;a.set_actor_scale3d(scale)
prop=next(c for c in hero.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Prop')
lo,hi=prop.get_local_bounds();tr=hero.get_editor_property('set_down_transform');tr.translation=u.Vector(center.x,center.y,new.translation.z+7-lo.z*tr.scale3d.z+.1);tr.rotation=u.Rotator(yaw=yaw).quaternion();hero.set_editor_property('set_down_transform',tr)
try:hero.set_editor_property('set_down_blend_duration',1.0)
except Exception:pass # Property becomes available after the editor module rebuild.
hero.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
(root/'Saved/FilmSetAudit/bench_alignment.json').write_text(json.dumps({'release':tr.export_text(),'bench':seat.get_actor_transform().export_text(),'mark':mark.get_actor_transform().export_text()},indent=2))
u.log('BENCH_RELEASE_ALIGNED')
