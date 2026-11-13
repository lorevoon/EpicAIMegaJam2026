"""Keep film equipment outside the restored mill house footprint."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
original={a['name']:a for a in json.loads((root/'Saved/FilmSetAudit/rehearsal_audit.json').read_text())['L_DC_Act1_Set']}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 return u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()[4].z
fixed=['Green screen','Studio rear shell','Studio roof','Soundstage floor','Tracking marker','Screen support','Ceiling soft light','Studio broad fill']
for a in actors:
 if '/L_DC_Act1_Set.' not in a.get_path_name() or a.get_name() not in original or a.get_attach_parent_actor():continue
 label=a.get_actor_label()
 if any(label.startswith(s) for s in fixed):continue
 old=original[a.get_name()]['pos'];x=-6500-old[1];y=-9250+old[0]
 if old[0]==-2450 and old[1]==-1000:y=-11000
 if old[0]==0 and old[1]==2000:x=-8200;y=-7000
 z=height(x,y)+old[2]
 a.set_actor_location(u.Vector(x,y,z),False,False)
 r=a.get_actor_rotation();r.yaw+=90;a.set_actor_rotation(r,False)
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
u.log('ALIGNED_EQUIPMENT_PLACED')
exec((root/'Scripts/validate_original_act1.py').read_text())
