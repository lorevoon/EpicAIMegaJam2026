import unreal as u,json,math,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
original=json.loads((root/'Saved/FilmSetAudit/original_layout.json').read_text())
report={'props':{},'collisions':[],'ground_errors':[],'min_separation':9999,'max_speed_cm_s':0,'bounds':{}}
for label,expected in original['props'].items():
 found=[a for a in actors if a.get_actor_label()==label and not a.get_attach_parent_actor()]
 assert len(found)==1,(label,len(found))
 actual=found[0].get_actor_transform().export_text()
 nums=lambda s:[float(v) for v in re.findall(r'=([-+]?[0-9]+\.[0-9]+)',s)]
 assert all(abs(a-b)<.001 for a,b in zip(nums(expected),nums(actual))),label
 report['props'][label]=actual
 c,e=found[0].get_actor_bounds(False,True);report['bounds'][label]=[c.x-e.x,c.x+e.x,c.y-e.y,c.y+e.y,c.z+e.z]
 assert c.x-e.x>-9580 and c.x+e.x<-3720 and c.y-e.y>-11330 and c.y+e.y<-6520 and c.z+e.z<4480,(label,report['bounds'][label])
cast=[a for a in actors if isinstance(a,u.DCPerformer)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard))]
ignore_ground=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
previous={}
for frame in range(201):
 t=frame*.1
 for a in cast:
  a.evaluate(t);p=a.get_actor_location();label=a.get_actor_label()
  hit=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,100),p-u.Vector(0,0,200),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore_ground,u.DrawDebugTrace.NONE,True).to_tuple()
  if not hit[0] or abs(p.z-hit[4].z-2)>3:report['ground_errors'].append([t,label,p.z,hit[4].z])
  result=u.SystemLibrary.sphere_trace_single(world,p+u.Vector(0,0,90),p+u.Vector(.1,0,90),30,u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True)
  if result and result.to_tuple()[0]:report['collisions'].append([t,label,str(result.to_tuple()[9])])
  if label in previous and not a.get_editor_property('ruins_shot'):report['max_speed_cm_s']=max(report['max_speed_cm_s'],(p-previous[label]).length()/.1)
  previous[label]=p
 for i,a in enumerate(cast):
  for b in cast[i+1:]:
   pa=a.get_actor_location();pb=b.get_actor_location();report['min_separation']=min(report['min_separation'],math.hypot(pa.x-pb.x,pa.y-pb.y))
for a in cast:a.evaluate(0)
beats=[a for a in actors if isinstance(a,u.DCBeat)]
assert len(beats)==4
for b in beats:assert max(b.get_editor_property('window_start'),b.get_editor_property('pose_ready_time'))+b.get_editor_property('required_hold')<=b.get_editor_property('window_end')<=20
report['capture_beats']=len(beats)
report['terrain_count']=len([a for a in actors if isinstance(a,u.LandscapeProxy)])
report['blueprint_api']=[n for n in dir(u.BlueprintEditorLibrary) if 'graph' in n.lower()]

(root/'Saved/FilmSetAudit/original_alignment_validation.json').write_text(json.dumps(report,indent=2))
assert not report['collisions'],report['collisions'][:10]
assert not report['ground_errors'],report['ground_errors'][:10]
assert report['min_separation']>=65
u.log('ORIGINAL_ALIGNMENT_VALIDATED')
