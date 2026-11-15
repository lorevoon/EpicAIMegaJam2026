import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);out={}
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 les.load_level(f'/Game/DirectorsCut/Levels/L_DC_Act{act}_Real')
 refs=[]
 for a in aes.get_all_level_actors():
  p=a.get_actor_location();label=a.get_actor_label()
  if -9200<p.x<-4300 and -11000<p.y<-7500 and any(k in label for k in ['Barrel','Cart','Mailbox','WheatStack','Fence','Rock']):
   refs.append(dict(label=label,pos=list(p.to_tuple()),components=len(a.get_components_by_class(u.StaticMeshComponent))))
 les.load_level(f'/Game/DirectorsCut/Levels/L_DC_Act{act}_{main}')
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
 ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
 points=[]
 for y in range(-10800 if act==2 else -10200,-7599,300):
  for x in range(-8900,-4399,300):
   h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,5000),u.Vector(x,y,-2000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
   points.append([x,y,h[4].z if h[0] else None])
 out[str(act)]=dict(refs=refs,terrain=points)
(R/'Saved/lively_plan.json').write_text(json.dumps(out,indent=2))
