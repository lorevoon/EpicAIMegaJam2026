"""Plant Smith's supplied terror motion without lifting his later death pose."""
import unreal as u,json,shutil,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Saved/Act2/ActingRevision4';out.mkdir(parents=True,exist_ok=True)
level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
backup=out/'L_DC_Act2_BrokenPromise_before_ground.umap'
if not backup.exists():shutil.copy2(root/'Content/DirectorsCut/Levels/L_DC_Act2_BrokenPromise.umap',backup)
assert les.load_level(level);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
smith=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Mr. Smith');comp=smith.get_component_by_class(u.SkeletalMeshComponent)
assert not any('DC_Acting4Ground' in [str(t) for t in a.tags] for a in actors),'Already applied'
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(p):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(p.x,p.y,9000),u.Vector(p.x,p.y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert h[0];return h[4].z
samples=[]
for frame in range(215):
 t=frame/30;smith.evaluate(t);p=smith.get_actor_location();toes=[comp.get_socket_location(n) for n in ['L_toe_joint','R_toe_joint']]
 gap=min(v.z-height(v) for v in toes);offset=2-gap
 # The existing death pose is correctly grounded: retain it from7.2 onward.
 if t>7:offset*=max(0,(7.2-t)/.2)
 samples.append((t,u.Vector(p.x,p.y,p.z+offset),smith.get_actor_rotation(),offset))
smith.evaluate(0)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act2_BrokenPromise')
for t,p,r,dz in samples:
 if t==0:smith.set_actor_location(p,False,False)
 else:
  w=aes.spawn_actor_from_class(u.DCWaypoint,p,r);w.set_editor_property('performer',smith);w.set_editor_property('time',t);w.set_actor_label('Smith planted terror @ %.3f'%t);w.set_folder_path('Act2/Blocking');w.tags=['DC_Acting4Ground'];w.set_is_temporarily_hidden_in_editor(True)
smith.set_editor_property('use_waypoint_height',True)
box=next(a for a in actors if a.get_actor_label()=='Mr. Smith - marked box');b,e=box.get_actor_bounds(False)
target=max(height(u.Vector(b.x+dx,b.y+dy,b.z)) for dx in [-e.x,e.x] for dy in [-e.y,e.y])+1
delta=target-(b.z-e.z);p=box.get_actor_location();p.z+=delta;box.set_actor_location(p,False,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Set');assert les.load_level(level)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)];smith=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Mr. Smith');comp=smith.get_component_by_class(u.SkeletalMeshComponent)
validation=[]
for frame in range(211):
 t=frame/30;smith.evaluate(t);gaps=[comp.get_socket_location(n).z-height(comp.get_socket_location(n)) for n in ['L_toe_joint','R_toe_joint']];validation.append([t,*gaps])
assert min(min(r[1:]) for r in validation)>1.9
assert max(min(r[1:]) for r in validation)<2.1
smith.evaluate(0)
# Validation never saves evaluated transforms; disk was saved before the fresh evaluation.
(out/'ground_fix.json').write_text(json.dumps({'initial_raise_cm':samples[0][3],'box_raise_cm':delta,'feet':validation,'death_unchanged_from':7.2},indent=2));print('ACTING_GROUND_FIXED')
