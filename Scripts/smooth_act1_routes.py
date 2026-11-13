"""Ease departures, arrivals and turns; retain all dramatic marks and kiss blocking."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=json.loads((root/'SourceArt/Act1Motion/motion_source.json').read_text())
aes=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
assert 'L_DC_Act1_Promise' in world.get_path_name()
cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def travel(t,d,r=.35):
 t=max(0,min(d,t))
 if t<r:
  x=t/r;return r*(x**3-.5*x**4)/(d-r)
 if t>d-r:return 1-travel(d-t,d,r)
 return (t-r*.5)/(d-r)
def height(x,y,crew):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True)
 assert hit,(x,y)
 return max(1080 if crew else -9999,hit.to_tuple()[4].z)+2
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
for who in ['Romeo','Juliet','Crew - sound assistant','Crew - hillside runner']:
 if who not in cast:continue # Background crew may be intentionally removed.
 a=cast[who];a.evaluate(0);crew=who.startswith('Crew');limit=20 if crew else 12.5
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and w.get_editor_property('time')<=limit:aes.destroy_actor(w)
 for i in range(round(limit*10)+1):
  t=i/10
  if not crew:
   if who=='Romeo':
    q=travel(t,2.4);x=-5850-50*q;y=-8870+20*q;yaw=180-22*smooth(t/.4)-158*smooth((t-6.2)/.8)
   else:
    q=travel(t-4,2);x=-5850-50*q;y=-8500-265*q;yaw=-130+29.3*smooth((t-3.7)/.3)+100.7*smooth((t-6.2)/.8)
   if t>=7:
    x=-5900+900*travel(t-7,5);y=-8850 if who=='Romeo' else -8765;yaw=(90 if who=='Romeo' else -90)*smooth((t-12)/.5)
  elif 'sound' in who:
   x=-5070
   if t<=14.5:y=-10000+600*travel(t,14.5);yaw=90
   elif t<15.5:y=-9400;yaw=90-180*smooth(t-14.5)
   else:y=-9400-180*travel(t-15.5,4.5);yaw=-90
  else:
   # Rounded inbound corner, then a deliberate stationary turnaround.
   q=travel(t,11.5) if t<=11.5 else 1
   p=[(-5070,-7800),(-5070,-8000),(-4950,-8000),(-4875,-8050)]
   x=(1-q)**3*p[0][0]+3*(1-q)**2*q*p[1][0]+3*(1-q)*q*q*p[2][0]+q**3*p[3][0]
   y=(1-q)**3*p[0][1]+3*(1-q)**2*q*p[1][1]+3*(1-q)*q*q*p[2][1]+q**3*p[3][1]
   dx=3*(1-q)**2*(p[1][0]-p[0][0])+6*(1-q)*q*(p[2][0]-p[1][0])+3*q*q*(p[3][0]-p[2][0])
   dy=3*(1-q)**2*(p[1][1]-p[0][1])+6*(1-q)*q*(p[2][1]-p[1][1])+3*q*q*(p[3][1]-p[2][1])
   yaw=math.degrees(math.atan2(dy,dx))
   if t>11.5:yaw+=(-194.38-yaw)*smooth((t-11.5))
   if t>=12.5:
    q=travel(t-12.5,7.5);x=-4875-195*q;y=-8050+50*q;yaw=-194.38
  pos=u.Vector(x,y,height(x,y,crew));rot=u.Rotator(yaw=yaw)
  w=aes.spawn_actor_from_class(u.DCWaypoint,pos,rot);w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_actor_label(f'{who} smooth @ {t:.1f}');w.set_folder_path('Act1/Blocking/Smoothed motion')
  if i==0:a.set_actor_location_and_rotation(pos,rot,False,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('ACT1_SMOOTH_ROUTES_SAVED')
