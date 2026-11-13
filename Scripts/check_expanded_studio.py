import unreal as u, math, json
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(level)
aes=u.get_editor_subsystem(u.EditorActorSubsystem);actors=list(aes.get_all_level_actors());by={a.get_actor_label():a for a in actors}
by['Practical downhill path'].set_actor_rotation(u.Rotator(roll=-math.degrees(math.atan2(50,900))),False)
r=by['Crew access ramp'];r.set_actor_location(u.Vector(1100,1000,40),False,False);r.set_actor_rotation(u.Rotator(roll=math.degrees(math.atan2(40,400))),False)
crew=by['Crew - hillside runner']
crew.set_actor_location(u.Vector(1500,1200,22),False,False)
for label in ['Romeo','Juliet']:
 a=by[label];p=a.get_actor_location();p.y=250;a.set_actor_location(p,False,False)
for a in actors:
 if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') in [by['Romeo'],by['Juliet']]:
  p=a.get_actor_location()
  if p.y in [150,50]:p.y=250;a.set_actor_location(p,False,False)
 if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer')==crew:
  p=a.get_actor_location()
  if p.x==1850:p.x=1500;p.y=1200
  if p.y==1050:p.y=1200
  elif p.y==650:p.y=800
  a.set_actor_location(p,False,False)
# Expose measured clearance and floor heights rather than relying only on waypoint coordinates.
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
cast=[a for a in actors if isinstance(a,u.DCPerformer)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard))]
rows=[]
for i in range(81):
 t=i*.25
 positions={}
 for a in cast:
  a.evaluate(t);p=a.get_actor_location();positions[a.get_actor_label()]=[p.x,p.y,p.z]
  hit=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,30),p-u.Vector(0,0,100),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
  if i%8==0:rows.append({'t':t,'actor':a.get_actor_label(),'pos':positions[a.get_actor_label()],'ground':str(hit.to_tuple())})
 for j,a in enumerate(cast):
  for b in cast[j+1:]:
   pa=a.get_actor_location();pb=b.get_actor_location();dist=math.hypot(pa.x-pb.x,pa.y-pb.y)
   assert dist>=65, (t,a.get_actor_label(),b.get_actor_label(),dist)
for a in cast:a.evaluate(0.)
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
(root/'Saved/FilmSetAudit/expanded_ground.json').write_text(json.dumps(rows,indent=2))
u.log('EXPANDED_CLEARANCE_CHECKED')
