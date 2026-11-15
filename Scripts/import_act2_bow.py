"""Import the supplied combat clips without changing existing performances."""
import unreal as u, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; BASE='/Game/DirectorsCut'; F=BASE+'/Act2/BowRevision6'
OUT=ROOT/'Saved/Act2/BowRevision6'; eal=u.EditorAssetLibrary; at=u.AssetToolsHelpers.get_asset_tools()
mix=u.load_asset(BASE+'/Mixamo/SK_XBot'); assert mix
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
report={}
for file,stem,role in [('Standing Equip Bow','EquipBow','Archer'),('Standing Draw Arrow','DrawArrow','Archer'),('Standing Walk Left','WalkLeft','Archer'),('Death From Front Headshot','DeathFront','Raider'),('Death Crouching Headshot Front','DeathCrouch','Raider')]:
 path=F+'/A_Mixamo_'+stem
 if not eal.does_asset_exist(path):
  task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/BowRevision6'/(file+'.fbx'));task.destination_path=F;task.destination_name='A_Mixamo_'+stem;task.automated=True;task.save=True
  ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mix.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION
  ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task])
  clips=[u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence)];assert clips
  if clips[0].get_path_name().split('.')[0]!=path:assert eal.rename_asset(clips[0].get_path_name(),path)
 clip=u.load_asset(path);out=F+'/A_'+role+'_'+stem
 target=u.load_asset(BASE+'/Act2/SK_Act2_OrcComplete' if role=='Raider' else BASE+'/Characters/Archer/SK_Archer')
 rtg=u.load_asset(BASE+'/Act2/RTG_Act2_Raider' if role=='Raider' else BASE+'/Mixamo/SupportRetarget/RTG_Support_Archer')
 if not eal.does_asset_exist(out):
  result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],mix,target,rtg,'A_Mixamo_','A_'+role+'_','','',F,False,False,True);assert len(result)==1
  generated=str(result[0].package_name)
  if generated!=out:assert eal.rename_asset(generated,out)
 anim=u.load_asset(out);assert anim;report[stem]=anim.get_play_length();eal.save_loaded_asset(anim,False)
for name in ['ArrowBasic','BowBasic']:
 path=F+'/SM_'+name
 if not eal.does_asset_exist(path):
  task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/BowRevision6'/(name+'.fbx'));task.destination_path=F;task.destination_name='SM_'+name;task.automated=True;task.save=True
  ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=True;ui.import_as_skeletal=False;ui.import_animations=False;ui.import_materials=False;ui.import_textures=False;ui.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;ui.static_mesh_import_data.combine_meshes=True;task.options=ui;at.import_asset_tasks([task])
 mesh=u.load_asset(path);assert mesh;mesh.set_material(0,u.load_asset(BASE+'/Materials/MI_Polysplit_Hero_Objects'));eal.save_loaded_asset(mesh,False)
 report[name]=str(mesh.get_bounding_box())
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);assert les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise')
snapshot={}
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCPerformer):
  row={'source':a.get_editor_property('animation_cues')[0].animation.get_path_name(),'mesh':a.get_editor_property('character_mesh').get_path_name(),'samples':[]}
  for t in [0,6,9,10,13,15,17,18.8,20,24,26,29,31,38]:
   a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);row['samples'].append({'t':t,'p':list(a.get_actor_location().to_tuple()),'yaw':a.get_actor_rotation().yaw,'head':list(c.get_socket_location('head_joint').to_tuple())})
  snapshot[a.get_actor_label()]=row
(OUT/'imports.json').write_text(json.dumps(report,indent=2))
if not (OUT/'baseline.json').exists():(OUT/'baseline.json').write_text(json.dumps(snapshot,indent=2))
print('BOW_IMPORTS_READY',report)
