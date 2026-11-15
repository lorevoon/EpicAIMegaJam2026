"""Import the supplied combat clips without changing existing performances."""
import unreal as u, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; BASE='/Game/DirectorsCut'; F=BASE+'/Act2/MobileRevision8'
OUT=ROOT/'Saved/Act2/MobileRevision8'; eal=u.EditorAssetLibrary; at=u.AssetToolsHelpers.get_asset_tools()
mix=u.load_asset(BASE+'/Mixamo/SK_XBot'); assert mix
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
report={}
for file,stem,role in [(file,stem,role) for file,stem in [('Standing To Crouch','Crouch'),('Crouch Turn Left 90','TurnLeft'),('Crouch Turn Right 90','TurnRight'),('Standing Run Forward','RunForward')] for role in ['Archer']]:
 path=F+'/A_Mixamo_'+stem
 if not eal.does_asset_exist(path):
  task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/MobileRevision8'/(file+'.fbx'));task.destination_path=F;task.destination_name='A_Mixamo_'+stem;task.automated=True;task.save=True
  ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mix.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION
  ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task])
  clips=[u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence)];assert clips
  if clips[0].get_path_name().split('.')[0]!=path:assert eal.rename_asset(clips[0].get_path_name(),path)
 clip=u.load_asset(path);out=F+'/A_'+role+'_'+stem
 target=u.load_asset(BASE+'/Act2/SK_Act2_OrcComplete' if role=='Raider' else BASE+'/Characters/'+role+'/SK_'+role)
 rtg=u.load_asset(BASE+'/Act2/RTG_Act2_Raider' if role=='Raider' else BASE+'/Mixamo/SupportRetarget/RTG_Support_'+role)
 if not eal.does_asset_exist(out):
  result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],mix,target,rtg,'A_Mixamo_','A_'+role+'_','','',F,False,False,True);assert len(result)==1
  generated=str(result[0].package_name)
  if generated!=out:assert eal.rename_asset(generated,out)
 anim=u.load_asset(out);assert anim;report[role+'_'+stem]=anim.get_play_length();eal.save_loaded_asset(anim,False)

(OUT/'imports.json').write_text(json.dumps(report,indent=2));print('SMOOTH_CLIPS_IMPORTED',report)
