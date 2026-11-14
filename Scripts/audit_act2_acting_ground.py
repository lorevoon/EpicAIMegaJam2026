import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/ActingRevision4';out.mkdir(parents=True,exist_ok=True)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(p):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(p.x,p.y,9000),u.Vector(p.x,p.y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert h[0];return h[4].z
report={}
for name in ['Mr. Smith','Romeo','Juliet']:
 a=cast[name];c=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(c.get_bone_name(i)) for i in range(c.get_num_bones())]
 row={'start':list(a.get_actor_location().to_tuple()),'offset':str(a.get_editor_property('mesh_offset')),'bones':bones,'waypoints':[],'poses':[]}
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:row['waypoints'].append([w.get_editor_property('time'),list(w.get_actor_location().to_tuple()),w.get_actor_rotation().yaw])
 for t in ([0,1,3,6,6.9,7,7.5,8,9,10,38] if name=='Mr. Smith' else [9.5,10,11,12,13,14,17]):
  a.evaluate(t);p=a.get_actor_location();data={'t':t,'p':list(p.to_tuple()),'yaw':a.get_actor_rotation().yaw,'ground':ground(p),'bones':{}}
  for b in bones:
   if any(k in b.lower() for k in ['ankle','toe','head','mouth','pelvis']):
    pos=c.get_socket_location(b);data['bones'][b]=[list(pos.to_tuple()),ground(pos),pos.z-ground(pos)]
  row['poses'].append(data)
 a.evaluate(0);report[name]=row
box=next(a for a in actors if a.get_actor_label()=='Mr. Smith - marked box');b,e=box.get_actor_bounds(False);report['box']={'p':list(box.get_actor_location().to_tuple()),'bounds':[list(b.to_tuple()),list(e.to_tuple())],'ground':ground(b)}
(out/'acting_audit.json').write_text(json.dumps(report,indent=2));print('ACTING_GROUND_AUDIT_DONE')
