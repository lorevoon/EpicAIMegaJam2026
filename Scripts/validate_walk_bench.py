"""Enable and validate clock-driven placement after rebuilding the performer module."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
a=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo')
a.set_editor_property('set_down_blend_duration',1.0)
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
prop=next(c for c in a.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Prop');samples=[]
for i in range(241):
 a.evaluate(i/60);samples.append(prop.get_world_location())
steps=[(b-c).length() for b,c in zip(samples,samples[1:])]
assert max(steps)<2,(max(steps),'box discontinuity')
for t in [4,2.9,0,3.4,2.4,3.1,4]:
 a.evaluate(t);assert (prop.get_world_location()-samples[round(t*60)]).length()<.01
seat=next(x for x in aes.get_all_level_actors() if x.get_actor_label()=='Mill-side bench seat')
a.evaluate(4);lo,hi=prop.get_local_bounds();bottom=prop.get_world_transform().transform_location(lo).z;origin,extent=seat.get_actor_bounds(False)
gap=bottom-origin.z-extent.z;assert abs(gap-.1)<.05,gap
p=a.get_actor_location();assert (prop.get_world_location()-p).length()<110
clip=u.load_asset('/Game/DirectorsCut/Animations/Archer/A_Archer_FemaleWalk');old=u.load_asset('/Game/DirectorsCut/Animations/Archer/A_Archer_Unarmed_WalkForward');mesh=u.load_asset('/Game/DirectorsCut/Characters/Archer/SK_Archer')
bones=['L_thigh_joint','R_thigh_joint','L_shoulder_joint','R_shoulder_joint']
difference=0
for i in range(30):
 aa=u.AnimationLibrary.get_bone_poses_for_time(clip,bones,i*clip.get_play_length()/30,False,mesh);bb=u.AnimationLibrary.get_bone_poses_for_time(old,bones,i*old.get_play_length()/30,False,mesh)
 difference+=sum(1-abs(sum(x*y for x,y in zip(a.rotation.to_tuple(),b.rotation.to_tuple()))) for a,b in zip(aa,bb))
assert difference>.01,difference
a.evaluate(0)
(root/'Saved/FilmSetAudit/walk_bench_validation.json').write_text(json.dumps({'box_max_frame_step_cm':max(steps),'box_seat_gap_cm':gap,'female_walk_pose_difference':difference},indent=2))
u.log('WALK_BENCH_VALIDATION_PASSED')
p=root/'Scripts/validate_saved_motion.py';exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
