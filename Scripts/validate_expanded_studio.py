import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(level)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
cast=[a for a in actors if isinstance(a,u.DCPerformer)];beats=[a for a in actors if isinstance(a,u.DCBeat)]
ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard))]
assert len(beats)==4
for b in beats:assert max(b.get_editor_property('window_start'),b.get_editor_property('pose_ready_time'))+b.get_editor_property('required_hold')<=b.get_editor_property('window_end')<=20
assert next(a for a in actors if isinstance(a,u.DCZombieDirector)).get_editor_property('take_duration')==20
assembly=[]
for a in actors:
 if 'DC_MillAssembly' in [str(t) for t in a.tags]:
  c,e=a.get_actor_bounds(False,True);assert abs(c.x)+e.x<2780 and abs(c.y)+e.y<2380 and c.z+e.z<2800,a.get_actor_label();assembly.append(a.get_actor_label())
assert len(assembly)==7
walls=[]
for a in actors:
 for c in a.get_components_by_class(u.StaticMeshComponent):
  for i in range(c.get_num_materials()):
   m=c.get_material(i)
   if m and m.get_name()=='M_Studio_ChromaGreen':
    assert a.get_actor_label().startswith('Green screen -'),a.get_actor_label();walls.append(a.get_actor_label())
assert len(walls)==4
initial={a:a.get_actor_location() for a in cast};issues=[];minimum=99999
for n in range(81):
 t=n*.25
 for a in cast:
  a.evaluate(t);p=a.get_actor_location()
  hit=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,30),p-u.Vector(0,0,100),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
  assert hit[0] and abs(p.z-hit[5].z-2)<10,(t,a.get_actor_label(),'ground',str(hit[5]))
  result=u.SystemLibrary.sphere_trace_single(world,p+u.Vector(0,0,90),p+u.Vector(.1,0,90),30,u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE,True)
  hit=result.to_tuple() if result else None
  if hit and hit[0]:issues.append([t,a.get_actor_label(),str(hit[9])])
 for j,a in enumerate(cast):
  for b in cast[j+1:]:
   pa=a.get_actor_location();pb=b.get_actor_location();d=math.hypot(pa.x-pb.x,pa.y-pb.y);minimum=min(minimum,d);assert d>=65
for a in cast:
 a.evaluate(0);assert (a.get_actor_location()-initial[a]).length()<.01
report={'duration':20,'floor_cm':[5600,4800],'assembly':assembly,'green_walls':walls,'samples':81,'minimum_actor_separation_cm':minimum,'prop_collision_issues':issues,'rewind_passed':True,'capture_beats':4}
(root/'Saved/FilmSetAudit/expanded_validation.json').write_text(json.dumps(report,indent=2))
assert not issues,issues[:10]
u.log('EXPANDED_STUDIO_VALIDATED')

