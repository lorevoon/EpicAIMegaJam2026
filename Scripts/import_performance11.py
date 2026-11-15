import unreal as u,json
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/PerformanceRevision11';out=R/'Saved/PerformanceRevision11';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);at=u.AssetToolsHelpers.get_asset_tools();mix=u.load_asset(B+'/Mixamo/SK_XBot');report={}
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
for file,stem,role in [('female walk','FemaleWalk','Archer'),('male Walking','MaleWalk','Swordsman'),('Female Kiss','FemaleKiss','Archer'),('Male Kiss','MaleKiss','Swordsman')]:
 path=F+'/A_Mixamo_'+stem
 if not e.does_asset_exist(path):
  task=u.AssetImportTask();task.filename=str(R/'SourceArt/PerformanceRevision11'/(file+'.fbx'));task.destination_path=F;task.destination_name='A_Mixamo_'+stem;task.automated=True;task.save=True
  ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mix.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION;ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task]);clips=[u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence)];assert clips
  if clips[0].get_path_name().split('.')[0]!=path:assert e.rename_asset(clips[0].get_path_name(),path)
 source=u.load_asset(path);dest=F+'/A_'+role+'_'+stem;mesh=u.load_asset(B+'/Characters/'+role+'/SK_'+role);rtg=u.load_asset(B+'/Mixamo/SupportRetarget/RTG_Support_'+role)
 if not e.does_asset_exist(dest):
  result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(source)],mix,mesh,rtg,'A_Mixamo_','A_'+role+'_','','',F,False,False,True);assert len(result)==1
  if str(result[0].package_name)!=dest:assert e.rename_asset(str(result[0].package_name),dest)
 anim=u.load_asset(dest);report[stem]={'length':anim.get_play_length(),'path':dest};assert anim.get_play_length()>.2;e.save_loaded_asset(anim,False)
for act,level in [(1,'L_DC_Act1_Promise'),(2,'L_DC_Act2_BrokenPromise')]:
 assert les.load_level(B+'/Levels/'+level);rows={}
 for a in aes.get_all_level_actors():
  if isinstance(a,u.DCPerformer) and a.get_actor_label() in ['Juliet','Romeo']:
   rows[a.get_actor_label()]={'cues':[{'t':q.start_time,'anim':q.animation.get_path_name()} for q in a.get_editor_property('animation_cues')],'carry':str(a.get_editor_property('carry_transform')),'mesh':a.get_editor_property('character_mesh').get_path_name(),'samples':[]}
   c=a.get_component_by_class(u.SkeletalMeshComponent)
   for t in ([0,4,5.6,7,11.6,15.5,16,17,18,19,20] if act==1 else [16.5,17.75,20.15]):
    a.evaluate(t);row={'t':t,'actor':list(a.get_actor_location().to_tuple()),'bones':{b:list(c.get_socket_location(b).to_tuple()) for b in ['mouth_joint','head_joint','L_wrist_joint','R_wrist_joint','pelvis_joint']}}
    if act==2 and a.get_actor_label()=='Juliet':
     prop=next(c for c in a.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Prop');row['bow']={str(p):list(prop.get_world_transform().transform_location(u.Vector(*p)).to_tuple()) for p in [(0,-36,0),(0,0,0),(0,36,0)]}
    rows[a.get_actor_label()]['samples'].append(row)
 report['act'+str(act)]=rows
(out/'inspection.json').write_text(json.dumps(report,indent=2));print('PERFORMANCE11_IMPORTED')
