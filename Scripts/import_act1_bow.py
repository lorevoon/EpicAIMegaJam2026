"""Import supplied Mixamo bow and retarget onto the existing shared villager rig."""
import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut';folder=base+'/Animations/Act1Bow';eal=u.EditorAssetLibrary
mix=u.load_asset(base+'/Mixamo/SK_XBot');target=u.load_asset(base+'/Characters/Swordsman/SK_Swordsman');rtg=u.load_asset(base+'/Mixamo/SupportRetarget/RTG_Support_Swordsman');assert mix and target and rtg
path=folder+'/A_Mixamo_QuickInformalBow'
if not eal.does_asset_exist(path):
 u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
 task=u.AssetImportTask();task.filename=str(root/'SourceArt/Act1Bow/QuickInformalBow.fbx');task.destination_path=folder;task.destination_name='A_Mixamo_QuickInformalBow';task.automated=True;task.save=True
 ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mix.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION
 ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui
 u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 clips=[u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence) and 'mixamo' in p.lower() and 'take_001' not in p.lower()];assert len(clips)==1,task.imported_object_paths
 if clips[0].get_path_name().split('.')[0]!=path:assert eal.rename_asset(clips[0].get_path_name(),path)
clip=u.load_asset(path);assert clip
out=folder+'/A_Smith_QuickInformalBow'
if not eal.does_asset_exist(out):
 result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],mix,target,rtg,'A_Mixamo_','A_Smith_','','',folder,False,False,True);assert len(result)==1
 generated=str(result[0].package_name)
 if generated!=out:assert eal.rename_asset(generated,out)
anim=u.load_asset(out);assert anim
anim.set_editor_property('force_root_lock',True);anim.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.ANIM_FIRST_FRAME);assert eal.save_loaded_asset(anim,False)
print('BOW_IMPORTED',anim.get_path_name(),anim.get_play_length())


