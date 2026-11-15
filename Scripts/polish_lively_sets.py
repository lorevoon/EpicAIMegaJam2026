import unreal as u,json,runpy
from pathlib import Path
R=Path(__file__).resolve().parents[1];F='/Game/DirectorsCut/LivelySet';e=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
for file in (R/'SourceArt/LivelySet').glob('Crew*.fbx'):
 name='SK_Live'+file.stem;mesh=u.load_asset(F+'/'+name);assert mesh
 t=u.AssetImportTask();t.filename=str(file);t.destination_path=F;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
 ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=True;ui.import_as_skeletal=True;ui.import_animations=False;ui.import_materials=False;ui.import_textures=False;ui.create_physics_asset=False;ui.skeleton=mesh.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH;t.options=ui;at.import_asset_tasks([t])
 e.save_loaded_asset(mesh.skeleton,False);e.save_loaded_asset(mesh,False)
for stem in ['Idle','Sitting','Talking','Arguing','Tired','ThumbsUp']:
 a=u.load_asset(F+'/A_Crew_'+stem);a.set_editor_property('force_root_lock',False);e.save_loaded_asset(a,False)
runpy.run_path(str(R/'Scripts/materials_lively_set.py'),run_name='__main__')
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);report={}
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 les.load_level(f'/Game/DirectorsCut/Levels/L_DC_Act{act}_{main}');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();rows=[]
 for a in aes.get_all_level_actors():
  if not isinstance(a,u.DCCrewMember):continue
  a.character.play_animation(a.get_editor_property('activity'),True);a.character.set_position(1.2,False)
  bones=['pelvis_joint','L_ball_joint','R_ball_joint','head_joint'] if not a.get_editor_property('director') else ['Hips','LeftToeBase','RightToeBase','Head']
  rows.append(dict(role=a.get_actor_label(),origin=list(a.get_actor_location().to_tuple()),sockets={b:list(a.character.get_socket_location(b).to_tuple()) for b in bones}))
 report[str(act)]=rows
(R/'Saved/lively_contacts.json').write_text(json.dumps(report,indent=2))
runpy.run_path(str(R/'Scripts/cleanup_lively_imports.py'),run_name='__main__')
