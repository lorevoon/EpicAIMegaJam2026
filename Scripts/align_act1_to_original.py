"""Align the practical set to original WindmillValley, keeping a short 20 s take.
Run once on the pre-alignment scene, after restore_original_act1_environment.py.
"""
import unreal as u, json, math, re
from pathlib import Path
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut/Levels/'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(base+'L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert not u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Ground'),'Already aligned'
ground_layer=u.EditorLevelUtils.add_level_to_world(world,base+'L_DC_Act1_Ground',u.LevelStreamingDynamic)
assert ground_layer
u.EditorLevelUtils.set_level_visibility(ground_layer.get_loaded_level(),True,False)
actors=list(aes.get_all_level_actors())
main=[a for a in actors if ':PersistentLevel.' in a.get_path_name() and '/L_DC_Act1_Promise.' in a.get_path_name()]
studio=[a for a in actors if '/L_DC_Act1_Set.' in a.get_path_name()]
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
 assert hit[0],('No original terrain',x,y)
 return hit[4].z
def transform(text):
 values=[float(v) for v in re.findall(r'=([-+]?[0-9]+\.[0-9]+)',text)]
 return u.Transform(u.Vector(*values[4:7]),u.Quat(*values[:4]).rotator(),u.Vector(*values[7:10]))
def action(p,offset=2):
 x=-6100-(p.y-300);y=-8750+(p.x-1200)
 return u.Vector(x,y,height(x,y)+offset)
original=json.loads((root/'Saved/FilmSetAudit/original_layout.json').read_text())
remove={'Mill scenic platform','Tree scenic island','Practical downhill path','Mill acting apron','Crew access ramp'}
for a in main:
 if a.get_attach_parent_actor():continue
 label=a.get_actor_label();p=a.get_actor_location();rot=a.get_actor_rotation()
 if label in remove or isinstance(a,(u.ProceduralFoliageVolume,u.ProceduralFoliageBlockingVolume,u.LightmassImportanceVolume)):
  aes.destroy_actor(a);continue
 if label in original['props']:
  a.set_actor_transform(transform(original['props'][label]),False,False)
 elif isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCStartMark,u.DCClapperboard,u.DCZombieDirector)):
  offset=100 if isinstance(a,u.DCStartMark) else 2
  a.set_actor_location(action(p,offset),False,False)
  a.set_actor_rotation(u.Rotator(pitch=rot.pitch,yaw=rot.yaw+90,roll=rot.roll),False)
  if isinstance(a,u.DCPerformer):
   t=a.get_editor_property('set_down_transform');old=t.translation
   if a.get_editor_property('set_down_time')>=0:
    t.translation=action(old,old.z-60);r=t.rotation.rotator();r.yaw+=90;t.rotation=r.quaternion();a.set_editor_property('set_down_transform',t)
 elif label=='Practical promise tree':
  a.set_actor_location(action(p,0),False,False);a.set_actor_rotation(u.Rotator(yaw=rot.yaw+90),False)
 elif label in {'Mill-side bench seat','Bench back','Bench leg'}:
  # The original acting apron was 60 cm high. Keep bench parts together on the terrain.
  center=action(u.Vector(1530,430,0),0)
  a.set_actor_location(u.Vector(center.x-(p.y-430),center.y+(p.x-1530),center.z+p.z-60),False,False)
  a.set_actor_rotation(u.Rotator(pitch=rot.pitch,yaw=rot.yaw+90,roll=rot.roll),False)
# Insert height samples between travel keys. This follows the original curved slope,
# rather than interpolating through it between the two ends of the walk.
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
waypoints=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCWaypoint)]
for performer in [a for a in main if isinstance(a,u.DCPerformer)]:
 keys=sorted([a for a in waypoints if a.get_editor_property('performer')==performer],key=lambda a:a.get_editor_property('time'))
 for first,last in zip(keys,keys[1:]):
  p=first.get_actor_location();q=last.get_actor_location();distance=math.hypot(q.x-p.x,q.y-p.y)
  count=math.ceil(distance/40)
  t0=first.get_editor_property('time');t1=last.get_editor_property('time')
  for i in range(1,count):
   alpha=i/count;x=p.x+(q.x-p.x)*alpha;y=p.y+(q.y-p.y)*alpha
   r0=first.get_actor_rotation();r1=last.get_actor_rotation();delta=(r1.yaw-r0.yaw+180)%360-180
   mark=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,height(x,y)+2),u.Rotator(yaw=r0.yaw+delta*alpha))
   mark.set_actor_label(f'{performer.get_actor_label()} terrain @ {t0+(t1-t0)*alpha:.3f}')
   mark.set_editor_property('performer',performer);mark.set_editor_property('time',t0+(t1-t0)*alpha)
   for prop in ['lean','prop_raise','tilt']:
    mark.set_editor_property(prop,first.get_editor_property(prop)+(last.get_editor_property(prop)-first.get_editor_property(prop))*alpha)
   mark.set_folder_path('Act1/Blocking/Terrain samples')
# Enclose the original mill assembly and short route in a slightly larger studio.
cx,cy=-6650,-8925;floor=650.;roof=4500.;width=5900.;depth=4850.
for a in studio:
 if a.get_attach_parent_actor():continue
 p=a.get_actor_location();label=a.get_actor_label();s=a.get_actor_scale3d()
 if label.startswith('Green screen -') or label=='Studio rear shell':
  z=(floor+roof)/2;s.z=(roof-floor)/100
  if label in ['Green screen - rear','Studio rear shell']:
   p=u.Vector(cx,cy+depth/2+(40 if label=='Studio rear shell' else 0),z);s.x=width/100
  elif label=='Green screen - front':p=u.Vector(cx,cy-depth/2,z);s.x=width/100
  elif label=='Green screen - left':p=u.Vector(cx-width/2,cy,z);s.y=depth/100
  else:p=u.Vector(cx+width/2,cy,z);s.y=depth/100
  a.set_actor_scale3d(s)
 elif label.startswith('Soundstage floor') or label=='Studio roof':
  p=u.Vector(cx,cy,floor-15 if label.startswith('Soundstage') else roof+20);s.x=width/100;s.y=depth/100;a.set_actor_scale3d(s)
  if label.startswith('Soundstage'):a.set_actor_label('Soundstage floor - 59m x 48.5m below original terrain')
 elif label=='Neutral rear floor return':
  aes.destroy_actor(a);continue
 elif label.startswith('Ceiling soft light'):p=u.Vector(cx+p.x,cy+p.y,roof-320)
 elif label.startswith('Tracking marker') or label=='Screen support upright':p=u.Vector(cx+p.x,cy+depth/2-2182+p.y,p.z+floor)
 else:
  x=cx+p.x;y=cy+p.y;p=u.Vector(x,y,height(x,y)+p.z)
 a.set_actor_location(p,False,False)
for name,visible in [('L_DC_Act1_Ground',True),('L_DC_Act1_Set',True),('L_DC_Act1_Real',False)]:
 layer=u.GameplayStatics.get_streaming_level(world,name);assert layer
 layer.set_editor_property('level_transform',u.Transform());layer.set_editor_property('initially_loaded',True);layer.set_editor_property('initially_visible',visible)
 layer.set_editor_property('should_be_loaded',True);layer.set_editor_property('should_be_visible',visible)
 u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'L_DC_Act1_Promise')
u.log('ACT1_ORIGINAL_ALIGNMENT_SAVED')
