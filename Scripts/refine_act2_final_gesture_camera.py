import unreal as u,json,math
from pathlib import Path
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);path='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';assert les.load_level(path);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
for n,v in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,n).get_loaded_level(),v,False)
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)];ignore=[a for a in actors if isinstance(a,u.CameraActor) or a.actor_has_tag('DC_SetOnly')];rows=[]
for x in [-5500,-5400,-5300,-5200,-5100]:
 for y in [-9600,-9500,-9400,-9300,-9200]:
  h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,0),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);assert h;p=u.Vector(x,y,h.to_tuple()[4].z+170);run=0;best=0
  for step in range(29):
   t=31+step*.25
   for a in cast.values():a.evaluate(t)
   clear=True
   for who,bone in [('Juliet','head_joint'),('Romeo','pelvis_joint')]:
    a=cast[who];target=a.get_component_by_class(u.SkeletalMeshComponent).get_socket_location(bone);hit=u.SystemLibrary.line_trace_single(world,p,target,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
    if hit and hit.to_tuple()[9]!=a:clear=False
   run=run+.25 if clear else 0;best=max(best,run)
  rows.append({'x':x,'y':y,'z':p.z,'clear':best,'score':min(best,4)*10-math.hypot(x+5300,y+9300)/200})
best=max(rows,key=lambda r:r['score']);assert best['clear']>=4,best
beat=next(a for a in actors if isinstance(a,u.DCBeat) and str(a.get_editor_property('beat_id'))=='2.5');beat.set_editor_property('camera_spot',beat.get_actor_transform().inverse_transform_location(u.Vector(best['x'],best['y'],best['z'])))
for a in cast.values():a.evaluate(0)
for n,v in [('L_DC_Act2_Set',True),('L_DC_Act2_Real',False)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,n).get_loaded_level(),v,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,path)
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/Act2/Revision2/final_camera.json').write_text(json.dumps({'chosen':best,'candidates':rows},indent=2));print('FINAL_GESTURE_CAMERA',best)
