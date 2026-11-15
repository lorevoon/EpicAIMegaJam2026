import unreal as u,math
from pathlib import Path
B='/Game/DirectorsCut';F=B+'/LivelySet';e=u.EditorAssetLibrary;mel=u.MaterialEditingLibrary
body=u.load_asset(B+'/Materials/MI_Polysplit_Hero_Body')
for path in e.list_assets(F,False,False):
 mesh=u.load_asset(path)
 if not isinstance(mesh,u.SkeletalMesh) or not mesh.get_name().startswith('SK_LiveCrew_'):continue
 slots=mesh.materials
 for i,slot in enumerate(slots):
  if 'Original_Hero_Body' in str(slot.material_slot_name):slot.material_interface=body;slots[i]=slot
 mesh.set_editor_property('materials',slots);e.save_loaded_asset(mesh,False)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level(B+'/Levels/L_DC_Act1_Promise');a=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo');c=a.get_component_by_class(u.SkeletalMeshComponent);mesh=a.get_editor_property('character_mesh')
parent=u.load_asset(F+'/M_RomeoRigSmile') or e.duplicate_asset(B+'/Materials/M_PolysplitRecolor',F+'/M_RomeoRigSmile');parent.set_editor_property('two_sided',True);mel.recompile_material(parent);e.save_loaded_asset(parent,False)
mi=u.load_asset(F+'/MI_RomeoRigSmile') or e.duplicate_asset(body.get_path_name(),F+'/MI_RomeoRigSmile');mel.set_material_instance_parent(mi,parent);e.save_loaded_asset(mi,False)
for i,slot in enumerate(mesh.materials):
 if 'Body' in str(slot.material_slot_name):c.set_material(i,mi)
source=u.load_asset(B+'/Animations/Act1Revision15/A_Act1_Romeo');anim=u.load_asset(F+'/A_RomeoHappy_A_Act1_Romeo');poses=[]
def smooth(t):t=max(0,min(1,t));return t*t*(3-2*t)
for f in range(601):
 t=f/30;p=u.AnimationLibrary.get_bone_poses_for_time(source,['mouth_joint'],t,False,mesh)[0];weight=smooth((t-2.1)/.6)*(1-smooth((t-14.8)/1.2));sc=p.scale3d;sc.y*=1-1.65*weight;p.scale3d=sc;poses.append(p)
ctrl=anim.get_editor_property('controller');ctrl.open_bracket('Subtle happy mouth through the facial rig',False);assert ctrl.set_bone_track_keys('mouth_joint',[p.translation for p in poses],[p.rotation for p in poses],[p.scale3d for p in poses],False);ctrl.close_bracket(False);e.save_loaded_asset(anim,False)
a.evaluate(0);assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),B+'/Levels/L_DC_Act1_Promise');print('RIG_SMILE_AND_CREW_SHADER_SAVED')

