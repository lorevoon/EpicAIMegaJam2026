import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/ActingRevision4'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise');cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
cams={'current':(-5400,-9850,1500),'old_ending':(-5750,-10100,1231),'southwest':(-6200,-9800,1450),'west':(-6400,-9200,1450),'east':(-5000,-9300,1450),'southeast_low':(-5400,-9850,1250),'south':(-5750,-10100,1500)};report={}
for t in [18,24,26,28,29,31,35,37]:
 row={}
 for name in ['Romeo','Juliet','Armoured monster']:
  a=cast[name];a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);head=c.get_socket_location('head_joint');mouth=c.get_socket_location('mouth_joint');face=(mouth-head).normal();b,e=c.get_local_bounds() if False else a.get_actor_bounds(False)
  row[name]={'p':list(a.get_actor_location().to_tuple()),'head':list(head.to_tuple()),'mouth':list(mouth.to_tuple()),'face':list(face.to_tuple()),'pelvis':list(c.get_socket_location('pelvis_joint').to_tuple()),'bounds':[list(b.to_tuple()),list(e.to_tuple())],'camera_dots':{n:face.dot((u.Vector(*p)-head).normal()) for n,p in cams.items()}}
 report[t]=row
(out/'final_camera_audit.json').write_text(json.dumps(report,indent=2));print('FINAL_CAMERA_AUDIT_DONE')
