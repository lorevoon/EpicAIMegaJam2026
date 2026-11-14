import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/ActingRevision4'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise');cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)}
report={};camera=u.Vector(-4550,-9250,1390)
for name in ['Romeo','Juliet']:
 a=cast[name];comp=a.get_component_by_class(u.SkeletalMeshComponent);measure=[]
 for t in [10.25,10.5,11,11.5,12,12.5,12.75]:
  a.evaluate(t);head=comp.get_socket_location('head_joint');mouth=comp.get_socket_location('mouth_joint');face=mouth-head;target=cast['Mr. Smith'].get_actor_location()-head;view=camera-head
  dot=(face.x*view.x+face.y*view.y)/math.hypot(face.x,face.y)/math.hypot(view.x,view.y)
  err=abs((math.degrees(math.atan2(face.y,face.x)-math.atan2(target.y,target.x))+180)%360-180);measure.append([t,err,dot]);assert err<1,(name,t,err);assert dot<0,(name,t,dot)
 report[name]={'animation':a.get_editor_property('animation_cues')[0].animation.get_path_name(),'samples':measure}
(out/'reaction_validation.json').write_text(json.dumps(report,indent=2));print('REACTION_ATTENTION_RELOAD',report)
