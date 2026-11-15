import unreal as u,json,math,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';O=R/'Saved/Act1Revision15';F=B+'/Animations/Act1Revision15';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
if not (O/'before.umap').exists():shutil.copy2(R/'Content/DirectorsCut/Levels/L_DC_Act1_Promise.umap',O/'before.umap')
assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());a=next(x for x in actors if isinstance(x,u.DCPerformer) and x.get_actor_label()=='Romeo');source=u.load_asset(B+'/Animations/Act1Revision14/A_Act1_Romeo');dest=F+'/A_Act1_Romeo';anim=u.load_asset(dest) or e.duplicate_asset(source.get_path_name(),dest);mesh=a.get_editor_property('character_mesh');c=a.get_component_by_class(u.SkeletalMeshComponent);poses=[u.AnimationLibrary.get_bone_poses_for_time(source,['mouth_joint'],f/30,False,mesh)[0] for f in range(601)];line=next(x for x in json.loads((R/'SourceArt/Act1Dialogue/timing.json').read_text())['lines'] if x['name']=='LetMeHelp')
for f,p in enumerate(poses):
 t=f/30
 if t<2.1:
  gate=line['mouth_gate'][f];fade=min(1,max(0,(t-line['start'])/.07),max(0,(line['end']-t)/.09));opening=(.5-.5*math.cos((t-line['start'])*2*math.pi*3.6))*gate*fade;p.scale3d=u.Vector(1-.05*opening,.3+1.5*opening if gate*fade>.01 else 1,1)
controller=anim.get_editor_property('controller');controller.open_bracket('Opening lip sync without a smile',False);assert controller.set_bone_track_keys('mouth_joint',[p.translation for p in poses],[p.rotation for p in poses],[p.scale3d for p in poses],False);controller.close_bracket(False)
if u.AnimationLibrary.does_curve_exist(anim,'FaceSmile',u.RawCurveTrackTypes.RCT_FLOAT):u.AnimationLibrary.remove_curve(anim,'FaceSmile')
u.AnimationLibrary.add_curve(anim,'FaceSmile');u.AnimationLibrary.add_float_curve_keys(anim,'FaceSmile',[0.,20.],[0.,0.]);assert e.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);original=u.load_asset(B+'/Materials/MI_Act1_FacialExpressions')
for i,slot in enumerate(mesh.materials):
 if 'Body' in str(slot.material_slot_name):c.set_material(i,original)
mark=next(x for x in actors if isinstance(x,u.DCStartMark));mark.set_actor_location_and_rotation(u.Vector(-4700,-9300,1082),u.Rotator(yaw=180),False,False);mark.set_actor_hidden_in_game(True);mark.set_actor_enable_collision(False)
# Validate against the inner faces of the current walls, not distance to a prop.
walls={x.get_actor_label():x for x in actors if x.get_actor_label().startswith('Green screen -')};right=walls['Green screen - right'];centre,extent=right.get_actor_bounds(False,True);clearance=centre.x-extent.x-mark.get_actor_location().x;assert clearance>400
for t in [0,.4,.8,1.2,2,5,10,15,19]:a.evaluate(t);assert abs(c.get_anim_instance().get_curve_value('FaceSmile'))<.001
assert abs(poses[12].scale3d.y-poses[24].scale3d.y)>.8
a.evaluate(0);assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise');report={'smile':0,'mouth_opening_samples':[poses[f].scale3d.y for f in [0,12,24,36,60]],'spawn':list(mark.get_actor_location().to_tuple()),'right_wall_clearance_cm':clearance};(O/'validation.json').write_text(json.dumps(report,indent=2));print('REVISION15_VALIDATED',report)
