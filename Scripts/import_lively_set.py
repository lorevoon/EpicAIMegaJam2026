import unreal as u,json,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1]; B='/Game/DirectorsCut'; F=B+'/LivelySet'
e=u.EditorAssetLibrary; at=u.AssetToolsHelpers.get_asset_tools()
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
mix=u.load_asset(B+'/Mixamo/SK_XBot'); hero=u.load_asset(B+'/Characters/Swordsman/SK_Swordsman')
def imp(file,name,skel=None,mesh=False,materials=False):
 path=F+'/'+name
 if e.does_asset_exist(path):
  existing=u.load_asset(path)
  if existing.get_editor_property('skeleton'):return existing
 task=u.AssetImportTask();task.filename=str(file);task.destination_path=F;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=mesh;ui.import_as_skeletal=True;ui.import_animations=not mesh;ui.import_materials=materials;ui.import_textures=materials;ui.create_physics_asset=False
 ui.skeleton=skel;ui.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION
 if not mesh:ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
 task.options=ui;at.import_asset_tasks([task])
 objs=[u.load_asset(p) for p in task.imported_object_paths];obj=next(x for x in objs if isinstance(x,u.SkeletalMesh if mesh else u.AnimSequence))
 if obj.get_path_name().split('.')[0]!=path:assert e.rename_asset(obj.get_path_name(),path)
 assert obj.get_editor_property('skeleton'),('Missing imported skeleton',name)
 e.save_loaded_asset(obj.get_editor_property('skeleton'),False)
 e.save_loaded_asset(obj,False)
 return u.load_asset(path)
names=['Crew_M01_Brown_Crop','Crew_F01_Auburn_Bun','Crew_F02_Blonde_Bob','Crew_M02_Silver_Swept','Crew_F03_Black_Ponytail']
crew=None
for name in names:
 mesh=imp(R/'SourceArt/LivelySet'/(name+'.fbx'),'SK_Live'+name,crew.skeleton if crew else None,True,True)
 if crew is None:crew=mesh
rig=e.duplicate_asset(B+'/Mixamo/SupportRetarget/IK_Support_Swordsman',F+'/IK_Crew') if not e.does_asset_exist(F+'/IK_Crew') else u.load_asset(F+'/IK_Crew')
u.IKRigController.get_controller(rig).set_skeletal_mesh(crew)
rtg=e.duplicate_asset(B+'/Mixamo/SupportRetarget/RTG_Support_Swordsman',F+'/RTG_Crew') if not e.does_asset_exist(F+'/RTG_Crew') else u.load_asset(F+'/RTG_Crew')
rc=u.IKRetargeterController.get_controller(rtg);rc.set_ik_rig(u.RetargetSourceOrTarget.TARGET,rig);rc.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.TARGET,rig)
e.save_loaded_asset(rig);e.save_loaded_asset(rtg)
director=imp(R/'SourceArt/LivelySet/SK_Director.fbx','SK_Director_Game',None,True,True)
report={}
for file,stem in [('Neutral Idle','Idle'),('Sitting Idle','Sitting'),('Standing Arguing','Arguing'),('Talking','Talking'),('Sad Idle','Tired'),('Standing Thumbs Up','ThumbsUp')]:
 source=Path('C:/Users/iris6/Downloads')/(file+'.fbx');dst=R/'SourceArt/LivelySet'/source.name
 if not dst.exists():shutil.copy2(source,dst)
 clip=imp(dst,'A_Mixamo_'+stem,mix.skeleton)
 if stem in ['Idle','ThumbsUp']:imp(dst,'A_DirectorGame_'+stem,director.skeleton)
 path=F+'/A_Crew_'+stem
 if not e.does_asset_exist(path):
  res=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],mix,crew,rtg,'A_Mixamo_','A_Crew_','','',F,False,False,True);assert len(res)==1
  if str(res[0].package_name)!=path:assert e.rename_asset(str(res[0].package_name),path)
 anim=u.load_asset(path);assert anim.get_play_length()>0
 anim.set_editor_property('enable_root_motion',False);anim.set_editor_property('force_root_lock',False);e.save_loaded_asset(anim)
 report[stem]=anim.get_play_length()
(R/'Saved/lively_assets.json').write_text(json.dumps(report,indent=2))
print('LIVELY_ASSETS_IMPORTED')
