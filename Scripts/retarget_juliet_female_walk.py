"""Retarget the project's female mannequin locomotion to Juliet's Archer rig."""
import unreal as u
base='/Game/DirectorsCut';dest=base+'/Animations/Archer';folder=base+'/Animations/Act1Retarget'
at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
src=u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple');target=u.load_asset(base+'/Characters/Archer/SK_Archer')
clip=u.load_asset('/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd')
assert clip.get_play_length()>.2
rig=u.load_asset(folder+'/IK_Quinn_Act1') or at.create_asset('IK_Quinn_Act1',folder,u.IKRigDefinition,u.IKRigDefinitionFactory())
c=u.IKRigController.get_controller(rig);c.set_skeletal_mesh(src);c.set_retarget_root('pelvis')
chains={'Spine':('spine_01','spine_05'),'Neck':('neck_01','neck_02'),'Head':('head','head'),'LeftClavicle':('clavicle_l','clavicle_l'),'RightClavicle':('clavicle_r','clavicle_r'),'LeftArm':('upperarm_l','hand_l'),'RightArm':('upperarm_r','hand_r'),'LeftLeg':('thigh_l','ball_l'),'RightLeg':('thigh_r','ball_r')}
for n,(a,b) in chains.items():c.add_retarget_chain(n,a,b,'')
tgt=u.load_asset(base+'/Mixamo/SupportRetarget/IK_Support_Archer')
rtg=u.load_asset(folder+'/RTG_Quinn_Archer') or at.create_asset('RTG_Quinn_Archer',folder,u.IKRetargeter,u.IKRetargetFactory())
rc=u.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,rig);rc.set_ik_rig(u.RetargetSourceOrTarget.TARGET,tgt)
rc.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE,src);rc.set_preview_mesh(u.RetargetSourceOrTarget.TARGET,target)
rc.add_default_ops();rc.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.SOURCE,rig);rc.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.TARGET,tgt);rc.auto_map_chains(u.AutoMapChainType.EXACT,True)
rc.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
eal.save_loaded_asset(rig,False);eal.save_loaded_asset(rtg,False)
path=dest+'/A_Archer_FemaleWalk'
if not eal.does_asset_exist(path):
 result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],src,target,rtg,'MF_Unarmed_Walk_Fwd','A_Archer_FemaleWalk','','',dest,False,False,True)
 assert len(result)==1
 generated=str(result[0].package_name)
 if generated!=path:assert eal.rename_asset(generated,path)
anim=u.load_asset(path);assert anim.get_editor_property('skeleton')==target.skeleton
anim.set_editor_property('force_root_lock',True);anim.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.ANIM_FIRST_FRAME);eal.save_loaded_asset(anim,False)
u.log('FEMALE_WALK_RETARGETED '+str(anim.get_play_length()))
