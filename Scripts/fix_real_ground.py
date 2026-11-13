import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1];obj=root/'Saved/FilmSetAudit/Act1_RealGround.obj'
lines=[]
for line in obj.read_text().splitlines():
 if line.startswith('v '):
  _,x,h,y=line.split();line=f'v {x} {y} {h}'
 lines.append(line)
obj.write_text('\n'.join(lines))
t=u.AssetImportTask();t.filename=str(obj);t.destination_path='/Game/DirectorsCut/Environment';t.destination_name='SM_Act1_RealGround';t.automated=True;t.replace_existing=True;t.save=True
op=u.FbxImportUI();op.import_mesh=True;op.import_materials=False;op.import_textures=False;op.import_as_skeletal=False;op.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;op.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False);op.static_mesh_import_data.set_editor_property('auto_generate_collision',False);t.options=op
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
assert u.load_asset('/Game/DirectorsCut/Environment/SM_Act1_RealGround').get_bounds().box_extent.z<1000
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
aes=u.get_editor_subsystem(u.EditorActorSubsystem);actors=list(aes.get_all_level_actors())
for w in actors:
 if isinstance(w,u.DCWaypoint) and w.get_editor_property('time')==0:
  a=w.get_editor_property('performer');a.set_actor_transform(w.get_actor_transform(),False,False)
for a in actors:
 for c in a.get_components_by_class(u.TextRenderComponent):c.set_visibility(True)
 a.set_is_temporarily_hidden_in_editor(False)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for name,visible in [('L_DC_Act1_Set',False),('L_DC_Act1_Real',True)]:
 layer=u.GameplayStatics.get_streaming_level(world,name);u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
u.log('REAL_GROUND_ORIENTATION_FIXED')
