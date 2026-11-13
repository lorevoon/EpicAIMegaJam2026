import unreal as u,math,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
cast=[a for a in actors if isinstance(a,u.DCPerformer)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard)) or a.get_actor_label() in ['Practical promise tree','Mill-side bench seat','Bench back','Bench leg']]
ground_ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 return u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ground_ignore,u.DrawDebugTrace.NONE,True).to_tuple()[4].z
def blocked(p):
 return u.SystemLibrary.sphere_trace_single(world,p+u.Vector(0,0,90),p+u.Vector(.1,0,90),35,u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True) is not None
offset=None
for dx in [200,250,300,350,400]:
 bad=False
 for step in range(81):
  for a in cast:
   if a.get_editor_property('ruins_shot'):continue
   a.evaluate(step*.25);p=a.get_actor_location();p.x+=dx;p.z=ground(p.x,p.y)+2
   if blocked(p):bad=True
 if not bad:offset=dx;break
assert offset is not None
for a in cast:a.evaluate(0)
bench=next(a for a in actors if a.get_actor_label()=='Mill-side bench seat').get_actor_location()
bench_dz=ground(bench.x+offset,bench.y)-ground(bench.x,bench.y)
for a in actors:
 if a.get_attach_parent_actor():continue
 label=a.get_actor_label();p=a.get_actor_location()
 if isinstance(a,u.DCPerformer) and not a.get_editor_property('ruins_shot') or isinstance(a,u.DCWaypoint) and not a.get_editor_property('performer').get_editor_property('ruins_shot') or isinstance(a,u.DCBeat) or label=='Practical promise tree':
  p.x+=offset;p.z=ground(p.x,p.y)+(0 if label=='Practical promise tree' else 2);a.set_actor_location(p,False,False)
  if isinstance(a,u.DCPerformer) and a.get_editor_property('set_down_time')>=0:
   t=a.get_editor_property('set_down_transform');q=t.translation;q.x+=offset;q.z+=bench_dz;t.translation=q;a.set_editor_property('set_down_transform',t)
 elif label in ['Mill-side bench seat','Bench back','Bench leg']:
  p.x+=offset;p.z+=bench_dz;a.set_actor_location(p,False,False)
# Reuse existing timing but route the runner along a clear lane east of the houses.
runner=next(a for a in cast if a.get_actor_label()=='Crew - hillside runner')
for a in actors:
 if a==runner or isinstance(a,u.DCWaypoint) and a.get_editor_property('performer')==runner:
  p=a.get_actor_location();x=-4950+(p.x+6600)*.3;y=-7900+(p.y+8650)*.5
  a.set_actor_location(u.Vector(x,y,ground(x,y)+2),False,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
(root/'Saved/FilmSetAudit/blocking_offset.json').write_text(json.dumps({'east_offset_cm':offset}))
u.log('ORIGINAL_BLOCKING_CLEARED')
