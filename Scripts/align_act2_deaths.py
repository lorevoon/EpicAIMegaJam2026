"""Align the two death holds with the sloped village lane; rebuild review transforms."""
import unreal as u,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast};ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 r=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert r[0];return r[4].z
rows=[]
for who,start,end in [('Mr. Smith',7,9),('Romeo',24,26)]:
 a=by[who];a.evaluate(end+.5);comp=a.get_component_by_class(u.SkeletalMeshComponent);p=a.get_actor_location();head=comp.get_socket_location('head_joint');slope=(ground(head.x,head.y)-ground(p.x,p.y))/max(1,math.hypot(head.x-p.x,head.y-p.y));tilt=-math.degrees(math.atan(slope));rows.append([who,tilt])
 for w in actors:
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and w.get_editor_property('time')>=end:
   w.set_editor_property('tilt',tilt);q=w.get_actor_location();q.z=ground(p.x,p.y)+6;w.set_actor_location(q,False,False)
 for t,angle,lift in [(start,0,3),(end,tilt,6)]:
  existing=next((w for w in actors if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a and abs(w.get_editor_property('time')-t)<.01),None)
  w=existing or aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(p.x,p.y,ground(p.x,p.y)+lift),u.Rotator(yaw=a.get_actor_rotation().yaw))
  w.set_actor_label(who+' terrain alignment @ '+str(t));w.set_folder_path('Act2/Blocking');w.set_editor_property('performer',a);w.set_editor_property('time',float(t));w.set_editor_property('tilt',angle)
 a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
assert les.load_level('/Game/Levels/L_DC_Zombie');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)]
# Save a clean, final named cut, retaining the earlier drafts as source assets.
text=(ROOT/'Scripts/polish_act2_stage.py').read_text();text=text[text.index('# Bake actor motion'):].replace("name='LS_DC_Act2_38s'","name='LS_DC_Act2_BrokenPromise'");exec(text)
print('TERRAIN_ALIGNMENT_COMPLETE',rows)
