import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];O=R/'Saved/Act1Revision14';O.mkdir(parents=True,exist_ok=True);B='/Game/DirectorsCut';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(B+'/Levels/L_DC_Act1_Promise');actors=list(aes.get_all_level_actors());report={'cast':{},'marks':[],'cameras':[],'beats':[]}
for a in actors:
 if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Romeo','Juliet']:
  c=a.get_component_by_class(u.SkeletalMeshComponent);row={'location':list(a.get_actor_location().to_tuple()),'yaw':a.get_actor_rotation().yaw,'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())],'keys':[],'cues':[q.animation.get_path_name() for q in a.get_editor_property('animation_cues')],'samples':[]}
  for w in actors:
   if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:row['keys'].append([w.get_editor_property('time'),*w.get_actor_location().to_tuple(),w.get_actor_rotation().yaw])
  row['keys'].sort()
  for t in [0,.4,.8,1.2,2,4,7,11,12,13,14,15,15.5,16,17.8,20]:
   a.evaluate(t);row['samples'].append({'t':t,'position':list(a.get_actor_location().to_tuple()),'yaw':a.get_actor_rotation().yaw,'mouth_scale':list(u.AnimationLibrary.get_bone_poses_for_time(a.get_editor_property('animation_cues')[0].animation,['mouth_joint'],t,False,a.get_editor_property('character_mesh'))[0].scale3d.to_tuple())})
  report['cast'][a.get_actor_label()]=row
 if isinstance(a,(u.DCStartMark,u.DCClapperboard)):report['marks'].append({'type':a.get_class().get_name(),'name':a.get_actor_label(),'location':list(a.get_actor_location().to_tuple()),'yaw':a.get_actor_rotation().yaw})
 if isinstance(a,u.CameraActor):report['cameras'].append({'name':a.get_actor_label(),'location':list(a.get_actor_location().to_tuple()),'tags':[str(x) for x in a.tags]})
 if isinstance(a,u.DCBeat):report['beats'].append({'id':str(a.get_editor_property('beat_id')),'start':a.get_editor_property('window_start'),'end':a.get_editor_property('window_end'),'transform':str(a.get_actor_transform()),'spot':str(a.get_editor_property('camera_spot'))})
(O/'inspection.json').write_text(json.dumps(report,indent=2));print('ACT1_INSPECTED')
