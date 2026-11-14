"""Import the four supplied Mixamo clips and retarget stage-specific copies."""
import unreal as u,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/Revision2';eal=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
mix=u.load_asset(BASE+'/Mixamo/SK_XBot');u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
report={}
for file,stem,roles in [('Surprised.fbx','Surprised',['Swordsman','Archer']),('Terrified.fbx','Terrified',['Archer']),('Laying Hand Gesture.fbx','LayingHandGesture',['Swordsman']),('Holding Down.fbx','HoldingDown',['Archer'])]:
 path=F+'/A_Mixamo_'+stem
 if not eal.does_asset_exist(path):
  task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/Revision2'/file);task.destination_path=F;task.destination_name='A_Mixamo_'+stem;task.automated=True;task.save=True
  ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mix.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION
  ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task])
  clips=[u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence)];assert clips,task.imported_object_paths
  if clips[0].get_path_name().split('.')[0]!=path:assert eal.rename_asset(clips[0].get_path_name(),path)
 clip=u.load_asset(path);assert clip
 for role in roles:
  target=u.load_asset(BASE+'/Characters/'+role+'/SK_'+role);out=F+'/A_'+role+'_'+stem
  if not eal.does_asset_exist(out):
   result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],mix,target,u.load_asset(BASE+'/Mixamo/SupportRetarget/RTG_Support_'+role),'A_Mixamo_','A_'+role+'_','','',F,False,False,True);assert len(result)==1
   generated=str(result[0].package_name)
   if generated!=out:assert eal.rename_asset(generated,out)
  anim=u.load_asset(out);assert anim.get_editor_property('skeleton')==target.skeleton
  assert eal.save_loaded_asset(anim,False)
  report[out]={'seconds':anim.get_play_length(),'poses':[]}
  for t in [0,anim.get_play_length()/2,anim.get_play_length()]:
   bones=['pelvis_joint','chest_joint','head_joint','L_wrist_joint','R_wrist_joint','L_ankle_joint']
   poses=u.AnimationLibrary.get_bone_poses_for_time(anim,bones,t,False,target)
   report[out]['poses'].append({'time':t,'local':{str(b):{'p':list(p.translation.to_tuple()),'r':list(p.rotation.to_tuple())} for b,p in zip(bones,poses)}})
(ROOT/'Saved/Act2/Revision2/imports.json').write_text(json.dumps(report,indent=2));print('ACT2_REVISION2_IMPORTED')
